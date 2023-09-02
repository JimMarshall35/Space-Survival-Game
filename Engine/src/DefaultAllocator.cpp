#include "DefaultAllocator.h"
#include <stdlib.h>

void* DefaultAllocator::Malloc(size_t numBytes)
{
	return malloc(numBytes);
}

void DefaultAllocator::Free(void* ptr)
{
	free(ptr);
}

void* DefaultAllocator::Realloc(void* ptr, size_t newSize)
{
	return realloc(ptr, newSize);
}
