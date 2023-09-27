#pragma once
#include "Core.h"
#include <new.h>
class APP_API IAllocator
{
public:
	virtual void* Malloc(size_t numBytes) = 0;
	virtual void Free(void* ptr) = 0;
	virtual void* Realloc(void* ptr, size_t newSize) = 0;

	template<typename T>
	static T* New(IAllocator* a)
	{
		return (T*)a->Malloc(sizeof(T));
	} 

	template<typename T>
	static T* NewArray(IAllocator* a, size_t size)
	{
		return (T*)a->Malloc(sizeof(T) * size);
	}
};

