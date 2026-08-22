#include "FastAllocator.h"

#pragma warning(push)
#pragma warning(disable: 4073) // warning C4073: initializers put in library initialization area
#pragma init_seg(lib) // leet hax to insure our threads and friends are initialized before everybody else

static FastAllocatorGeneral* __general_allocator = new FastAllocatorGeneral();

FastAllocatorGeneral* FastAllocatorGeneral::Get_Allocator()
{
	return __general_allocator;	
};

// Please modify these next two functions to call a system allocator
DECLSPEC_RESTRICT void* SystemAllocate(size_t size)
{
	void* memory = malloc(size);
	// If you hit this assert, you've run out of memory. This is bad.
	assert(memory);
	return memory;
};

void SystemFree(void* memory)
{
	free(memory);
};

#pragma warning(pop)