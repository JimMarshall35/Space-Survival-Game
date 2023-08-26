#pragma once
#include <stdint.h>
#include <cstdlib>
#include <string>
#include "Core.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

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


class APP_API BasicHeap {
	friend class BasicHeapAllocatorTestHarness;
public:
	BasicHeap(unsigned int maxSize, const std::string& name);
	~BasicHeap();
	void* Malloc(size_t size);
	void* Realloc(void* ptr, size_t newSize);
	void Free(void* ptr);
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
