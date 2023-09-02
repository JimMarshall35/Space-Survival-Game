#pragma once
#include <cstdlib>
#include <string>
#include "CommonTypedefs.h"
#include "Core.h"
#include "IAllocator.h"


#define BasicAlloc(numBytes) malloc(numBytes)
#define BasicFree(ptr) free(ptr)

#define Alignment 4

#if Alignment == 4
typedef u32 Cell;
#elif Alignment == 8
typedef u64 Cell;
#endif
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N,S) ((N / S) * S)

struct APP_API BasicHeapBlockHeader
{
	BasicHeapBlockHeader* Next;
	BasicHeapBlockHeader* Previous;
	u8* Data;
	// the amount of d
	size_t CurrentSize;
	size_t Capacity;
	u8 bDataIsFree : 1;
#define PrePaddingSize (sizeof(void*) * 3 + sizeof(size_t) * 2 + sizeof(u8) * 1)
#define MemoryBlockStructPaddingBytes ROUND_UP(PrePaddingSize, Alignment) - PrePaddingSize
	u8 Padding[MemoryBlockStructPaddingBytes];
};


class APP_API BasicHeap : public IAllocator{
	friend class BasicHeapAllocatorTestHarness;
public:
	BasicHeap(unsigned int maxSize, const std::string& name);
	~BasicHeap();
	virtual void* Malloc(size_t size) override;
	virtual void* Realloc(void* ptr, size_t newSize) override;
	virtual void Free(void* ptr) override;
	
	void DebugPrintAllBlocks();
	void DebugPrintOverview();
private:
	void Free(BasicHeapBlockHeader* block);
	BasicHeapBlockHeader* AddNewMemoryBlock(size_t sizeBytes, BasicHeapBlockHeader* next, BasicHeapBlockHeader* prev, bool isFree, u8*& data);
	BasicHeapBlockHeader* FindMemoryBlockFromPtr(void* ptr);
	void MergeFreeBlocks(BasicHeapBlockHeader* lowBlock, BasicHeapBlockHeader* highBlock);
	BasicHeapBlockHeader* FindFreeMemoryBlock(size_t minSizeInclusive);
	bool IncreaseTopOfUsedMemory(size_t amountToAdd);
private:
	void* RawMemory;
	u8* EndOfRawMemory;
	size_t MaxMemorySizeBytes;
	BasicHeapBlockHeader* BlocksListHead;
	u8* TopOfUsedMemory;
	u32 NumBlocks;
	BasicHeapBlockHeader* EndBlock;
	std::string Name;
};
