#include "IAllocator.h"

class APP_API DefaultAllocator : public IAllocator
{
public:


	// Inherited via IAllocator
	virtual void* Malloc(size_t numBytes) override;

	virtual void Free(void* ptr) override;

	virtual void* Realloc(void* ptr, size_t newSize) override;

};