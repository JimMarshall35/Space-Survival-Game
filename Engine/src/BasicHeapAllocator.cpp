#include "BasicHeapAllocator.h"
#include <cassert>
#include <iostream>

/*
Strategy 1: Inline Headers 

start of data is always immediately after the header:
pros:
	-> merging two empty blocks also automatically "deletes" the header of the higher block
	-> when new block created, it's added inline at either top of memory "stack" or part way through an existing block 
	-> fast lookup of block header from allocated pointer - cast to BlockHeader and subtract 1
cons:
	-> less cache efficient?

Heap layout?
|header|data||||||||||||||||header|data||||||header|data||||||||||||||||||||||||||||||||||header|data||||||

Strategy 2: Pooled Headers (not yet implemented)

the end of the heap is a pool of headers
cons:
	-> have to manually delete the header when two blocks are merged
	-> extra step to aquire header from pool when new block created
pros:
	-> More cache efficient?
	-> Minimum size of block that can be split off from existing block isn't limited by
	   size of header - less fragmentation?
	-> Slower lookup of BlockHeader from allocated ptr - would have to iterate through headers pool and find the one
	   with the right data pointer

|data        |data    |data                |       (gap in middle)               |header|header|header| <- end of total allocated memory

*/

BasicHeap::BasicHeap(unsigned int maxSize, const std::string& name)
	:RawMemory(BasicAlloc(maxSize)),
	EndOfRawMemory(((u8*)RawMemory) + maxSize),
	MaxMemorySizeBytes(maxSize),
	BlocksListHead(nullptr),
	TopOfUsedMemory(static_cast<u8*>(RawMemory)),
	EndBlock(nullptr),
	Name(name)
{
	memset(RawMemory, '0', maxSize);
}

BasicHeapBlockHeader* BasicHeap::AddNewMemoryBlock(size_t sizeBytes, BasicHeapBlockHeader* next, BasicHeapBlockHeader* prev, bool isFree, u8*& data)
{
	BasicHeapBlockHeader* newBlock = (BasicHeapBlockHeader*)data;

	if (!BlocksListHead)
	{
		BlocksListHead = newBlock;
	}

	if (data == TopOfUsedMemory)
	{
		// we need to check for out of memory here as well as +=
		IncreaseTopOfUsedMemory(sizeof(BasicHeapBlockHeader));
	}
	else 
	{
		data += sizeof(BasicHeapBlockHeader);
	}
	
	newBlock->CurrentSize = sizeBytes;
	newBlock->Capacity = ROUND_UP(sizeBytes, 4);
	newBlock->Data = data;

	if (data == TopOfUsedMemory)
	{
		IncreaseTopOfUsedMemory(newBlock->Capacity);
	}
	else
	{
		data += newBlock->Capacity;
	}

	newBlock->bDataIsFree = isFree;
	newBlock->Next = next;
	newBlock->Previous = prev;

	if (BasicHeapBlockHeader* prevBlock = newBlock->Previous)
	{
		prevBlock->Next = newBlock;
	}
	if (BasicHeapBlockHeader* nextBlock = newBlock->Next)
	{
		nextBlock->Previous = newBlock;
	}
	
	EndBlock = newBlock;;
	
	return newBlock;
}

BasicHeapBlockHeader* BasicHeap::FindMemoryBlockFromPtr(void* ptr)
{
	return (((BasicHeapBlockHeader*)ptr) - 1);
}

void BasicHeap::MergeFreeBlocks(BasicHeapBlockHeader* lowBlock, BasicHeapBlockHeader* highBlock)
{
	assert(lowBlock->bDataIsFree);
	assert(highBlock->bDataIsFree);
	lowBlock->CurrentSize = 0;//(lowBlock->DataSizeBytesAligned + highBlock->DataSizeBytesAligned) * sizeof(Cell);
	lowBlock->Capacity = lowBlock->Capacity + highBlock->Capacity;
	lowBlock->Next = highBlock->Next;
	if (BasicHeapBlockHeader* newNextBlock = lowBlock->Next)
	{
		newNextBlock->Previous = lowBlock;
	}
}

BasicHeapBlockHeader* BasicHeap::FindFreeMemoryBlock(size_t minSizeInclusive)
{
	BasicHeapBlockHeader* onBlock = BlocksListHead;
	while (onBlock)
	{
		if (onBlock->bDataIsFree && onBlock->Capacity >= minSizeInclusive)
		{
			return onBlock;
		}
		onBlock = onBlock->Next;
	}
	return nullptr;
}

bool BasicHeap::IncreaseTopOfUsedMemory(size_t amountToAdd)
{
	TopOfUsedMemory += amountToAdd;
	if (TopOfUsedMemory > EndOfRawMemory)
	{
		std::cerr << "[BasicHeap] Out of heap memory, heap: " << Name << "\n";
		return true;
	}
	return false;
}

void BasicHeap::DebugPrintAllBlocks()
{
	BasicHeapBlockHeader* onBlock = BlocksListHead;
	u32 i = 0;
	while (onBlock)
	{
		std::cout << "on block: " << i++ << ". currentSize: "<< onBlock->CurrentSize << ". capacity: " << onBlock->Capacity << ". isFree: " << (onBlock->bDataIsFree ? " true" : "false") << ".\n";
		for(int i=0; i<onBlock->CurrentSize; i++)
		{
			std::cout << (char)onBlock->Data[i];
		}
		std::cout << "\n";
		onBlock = onBlock->Next;
	}
	std::cout << "\n\n\n\n";
}

void BasicHeap::DebugPrintOverview()
{
}

BasicHeap::~BasicHeap()
{
	BasicFree(RawMemory);
}

void* BasicHeap::Malloc(size_t size)
{
	//BasicHeapBlockHeader* onBlock = BlocksListHead;
	size_t requiredSizeInBytes = ROUND_UP(size, Alignment);
	void* returnVal = nullptr;

	if (BasicHeapBlockHeader* onBlock = FindFreeMemoryBlock(requiredSizeInBytes))
	{
		// we've found a block in the middle of the chain with enough free space
		onBlock->bDataIsFree = false;
		onBlock->CurrentSize = size;
		u8* writeStartPtr = (u8*)onBlock->Data + ROUND_UP(onBlock->CurrentSize, Alignment);
		u8* writeEndPtr = (u8*)(onBlock->Data + onBlock->Capacity);
		i64 availableBytes = (size_t)writeEndPtr - (size_t)writeStartPtr;
		size_t heapBlockSize = sizeof(BasicHeapBlockHeader);
		availableBytes -= heapBlockSize;
		const i64 MIN_BLOCK_SIZE = 1;
		if (availableBytes >= MIN_BLOCK_SIZE)
		{
			// if there is enough free space space left over for another BasicHeapBlockHeader + MIN_BLOCK_SIZE bytes
			// after the new allocation then add a new header for a new free block representing the rest
			onBlock->Capacity = ROUND_UP(onBlock->CurrentSize, Alignment);

			BasicHeapBlockHeader* newBlock = AddNewMemoryBlock(availableBytes, onBlock->Next, onBlock, true, writeStartPtr);
		}
		return onBlock->Data;
	}
	BasicHeapBlockHeader* newBlock = AddNewMemoryBlock(size, nullptr, EndBlock, false, TopOfUsedMemory);

	return newBlock->Data;
}

void* BasicHeap::Realloc(void* ptr, size_t newSize)
{
	BasicHeapBlockHeader* block = FindMemoryBlockFromPtr(ptr);
	assert(block);
	size_t freeSpaceInBlock = block->Capacity - block->CurrentSize;
	
	if (!block->Next)
	{
		// block is the last one in the chain which is also the one whos memory ends at the free unused area of the heap
		// so we can just expand into that
		block->CurrentSize = newSize;
		size_t oldSize = block->Capacity;
		block->Capacity = ROUND_UP(newSize, Alignment);
		IncreaseTopOfUsedMemory (block->Capacity - oldSize);
		return block->Data;
	}

	if (newSize >= block->CurrentSize && newSize <= block->Capacity)
	{
		// there is enough space in the current block that hasn't been allocated,
		// this will happen when:
		//	- the re-allocation is only a few bytes more than the previous allocation and can fit within
		//    the padding for alignment
		//  - the free space after the previous allocation into the block was < the size of the block header + MIN_SIZE
		//    and so was not split off into a new empty block, therefore was left as empty, un-allocated
		block->CurrentSize = newSize;
		return block->Data;
	}

	// neither of the above cases are true so
	// we have no choice but to make a new block and memcpy into it
	void* newAlloc = this->Malloc(newSize);
	memcpy(newAlloc, block->Data, block->CurrentSize);
	this->Free(block);
	return newAlloc;
}

void BasicHeap::Free(void* ptr)
{
	BasicHeapBlockHeader* block = FindMemoryBlockFromPtr(ptr);
	assert(block, "pointer not found in heap " + std::string(this));
	Free(block);
}

void BasicHeap::Free(BasicHeapBlockHeader* block)
{
	
	block->bDataIsFree = true;
	block->CurrentSize = 0;
	if (block->Next->bDataIsFree)
	{
		MergeFreeBlocks(block, block->Next);
	}
	if (block->Previous->bDataIsFree)
	{
		MergeFreeBlocks(block->Previous, block);
	}
}
