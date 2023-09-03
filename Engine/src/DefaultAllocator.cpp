#include "DefaultAllocator.h"
#include <stdlib.h>

void* DefaultAllocator::Malloc(size_t numBytes)
{
	std::lock_guard<std::mutex> lock(Mtx);
	return malloc(numBytes);
}

void DefaultAllocator::Free(void* ptr)
{
	std::lock_guard<std::mutex> lock(Mtx);
	free(ptr);
}

void* DefaultAllocator::Realloc(void* ptr, size_t newSize)
{
	std::lock_guard<std::mutex> lock(Mtx);
	return realloc(ptr, newSize);
}
