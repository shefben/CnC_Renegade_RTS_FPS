#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "FastAllocator.h"
#include "MemTracker.h"
#include "FastCriticalSection.h"
#include "MemoryManager.h"

//#define ALWAYS_WIPE_MEMORY

#pragma warning(push)
#pragma warning(disable: 4073) // warning C4073: initializers put in library initialization area
#pragma init_seg(lib) // leet hax to insure our threads and friends are initialized before everybody else
PUSH_MEMORY_MACROS
#undef new
#undef delete

static FastCriticalSection					MemoryTrackerCS;
static FastObjectAllocator<AllocationUnit>	TagAllocator;
static DList<AllocationUnit>				Tags;
static MemoryTracker						MemoryTrackerInstance;

void* AllocationUnit::operator new(size_t size)
{
	assert(size == sizeof(AllocationUnit));
	return TagAllocator.Allocate();
};

void AllocationUnit::operator delete(void* memory)
{
	TagAllocator.Free(memory);
};


MemoryTracker::~MemoryTracker()
{
	for (AllocationUnit* tag = Tags.PopHead(); tag; tag = Tags.PopHead() )
	{
		// TODO: Dump allocation info, because it was never freed...
		Breakpoint();
 		tag->Remove();
		delete tag;
	};
};


long MemoryTracker::CurrentAllocationCount;

// each memory allocation looks like
// pointer to AllocationUnit instance
// start padding
// actual memory
// end padding

static const char* FilePathStripper(const char *source_file)
{
	const char* ptr = strrchr(source_file, '\\');
	if (ptr) return ptr + 1;
	ptr = strrchr(source_file, '/');
	if (ptr) return ptr + 1;
	return source_file;
}

const size_t MemoryTracker::GetMemoryTrackingCost()
{
	// pointer size + sentinel size
	return sizeof(AllocationUnit*) + (SENTINEL_SIZE * sizeof(int32) * 2);
};

AllocationUnit* MemoryTracker::FindAllocationUnit(void* memory)
{
	void* actual = CalculateActualAddress(memory);
	int32* sentinel_start = (int32*) (char*) actual + sizeof(AllocationUnit*);
	if (*sentinel_start != PREFIX_PATTERN) return NULL; // preliminary validation
	return *(AllocationUnit**) CalculateActualAddress(memory);
};

void MemoryTracker::WipeMemoryWithPattern(AllocationUnit* unit, int32 pattern, size_t bias)
{

#ifdef ALWAYS_WIPE_MEMORY

	// fill the bulk of the allocation
	int32* iptr = (int32*)((char*)unit->ReportedAddress + bias);
	size_t length = unit->ReportedSize - bias;
	for (size_t i = 0; i < (length / 4); ++i)
	{
		*++iptr = pattern;
	};

	// fill the remainder	
	char* cptr = (char*)iptr;
	for (size_t i = 0, shiftcount = 0; i < (length & 0x3); ++i, shiftcount += 8)
	{
		*++cptr = char(pattern & (0xFF << shiftcount) >> shiftcount);
	}
#else
	UNUSED_VARIABLE(pattern);
	UNUSED_VARIABLE(bias);
#endif

	int32 *pre = (int32*)CalculatePrefixSentinelStartAddress(unit);
	int32 *post = (int32*)CalculatePostfixSentinelStartAddress(unit);
	for (size_t i = 0; i < SENTINEL_SIZE; ++i, ++pre, ++post)
	{
		*pre = PREFIX_PATTERN;
		*post = POSTFIX_PATTERN;
	};
};


bool MemoryTracker::ValidateAllocationUnit(AllocationUnit* unit)
{
	// make sure the sentinels are untouched
	int32 *pre = (int32*)CalculatePrefixSentinelStartAddress(unit);
	int32 *post = (int32*)CalculatePostfixSentinelStartAddress(unit);
	bool error_flag = false;
	for (size_t i = 0; i < SENTINEL_SIZE; ++i, ++pre, ++post)
	{
		if (*pre != PREFIX_PATTERN)
		{
			// log this event maybe?
			error_flag = true;
		}

		if (*post != POSTFIX_PATTERN) 
		{
			// this one too?
			error_flag = true;
		}
		
		// If you hit this assert, the sentinel before the allocation has been damaged 
		assert(*pre == PREFIX_PATTERN);

		// If you hit this assert, the sentinel after the allocation has been damaged
		assert(*post == POSTFIX_PATTERN);
	};
	
	return !error_flag;
};

extern DWORD TLSIndex; // HACK: Defined in dllmain_debug
MemoryTrackerThreadLocalInformation* MemoryTracker::GetThreadLocalInformation()
{
	void* tls_mem = TlsGetValue(TLSIndex);
	return (MemoryTrackerThreadLocalInformation*)tls_mem;
};

void MemoryTracker::SetThreadLocalInformation(const char* source_file, const char* source_function, const int source_line)
{
	MemoryTrackerThreadLocalInformation* info = GetThreadLocalInformation();
	strcpy_s(info->CurrentSourceFile, sizeof(info->CurrentSourceFile), source_file ? FilePathStripper(source_file) : "(Unknown File)");
	strcpy_s(info->CurrentSourceFunction, sizeof(info->CurrentSourceFunction), source_function ? source_function : "(Unknown Function)");
	info->CurrentSourceLine = source_line;
};

DECLSPEC_RESTRICT void* MemoryTracker::Allocate(const size_t size, const AllocType type, const char* source_file, const char* source_function, const int source_line)
{
	// If you hit this assert, then this allocation call was made from a source that isn't setup to use our
	// memory tracking system. Use the callstack to locate the source and include our memory tracker header.
	assert(type != AllocType_Unknown);
	
	AllocationUnit* tag = new AllocationUnit();

	// If you've hit this assert, then you've run out of memory for the allocation tag, which is really really bad.
	assert(tag != NULL);

	tag->ActualSize = GetMemoryTrackingCost() + size;
	tag->ActualAddress = FastAllocatorGeneral::Get_Allocator()->Allocate(tag->ActualSize);

	// If you've hit this assert, then you've run out of memory
	assert(tag->ActualAddress != NULL);

	tag->ReportedSize = size;
	tag->ReportedAddress = CalculateReportedAddress(tag->ActualAddress);
	tag->AllocationType = type;
	
	if (source_file) strcpy_s(tag->SourceFile, sizeof(tag->SourceFile), FilePathStripper(source_file));
	if (source_function) strcpy_s(tag->SourceFunction, sizeof(tag->SourceFunction), source_function);
	tag->SourceLine	= source_line;

	tag->AllocationType = type;
	tag->AllocationNumber = _InterlockedIncrement((long*)&CurrentAllocationCount);

	tag->BreakOnFree = false;
	tag->BreakOnRealloc = false;

	// Store the allocation unit tag address within the allocation memory
	*(AllocationUnit**)tag->ActualAddress = tag;

	// We'll wipe the memory here with our "Unused" pattern so we can later estimate how much of that allocated
	// memory was actually "used"
	WipeMemoryWithPattern(tag, UNUSED_PATTERN);

	// And we'll wipe the memory here *again* if it's allocated via calloc, which expects all the memory to be zero-init'd
	if (type == AllocType_Calloc) memset(tag->ReportedAddress, 0x00, tag->ReportedSize);

	// Clearing the tracking information insures that if at some later time somebody calls our memory tracker
	// from an unknown source, we don't think it was the last allocation.
	ClearThreadLocalInformation();

	// Add the memory tag to our "tracked" list.
	{
		FastCriticalSection::Lock lock(MemoryTrackerCS);
		Tags.PushTail(tag);
	}

	return tag->ReportedAddress;
};

DECLSPEC_RESTRICT void* MemoryTracker::Reallocate(void* memory, const size_t size, const AllocType type, const char* source_file, const char* source_function, const int source_line)
{
	UNUSED_VARIABLE(memory);
	UNUSED_VARIABLE(size);
	UNUSED_VARIABLE(type);
	UNUSED_VARIABLE(source_file);
	UNUSED_VARIABLE(source_function);
	UNUSED_VARIABLE(source_line);
	Breakpoint(); // If you hit this assert, you tried to reallocate memory. Right now, you can't.
	return (void*) 0xdeadc0de;
};

void MemoryTracker::Free(void* memory, const DeallocType type, const char* source_file, const char* source_function, const int source_line)
{
	UNUSED_VARIABLE(source_file);
	UNUSED_VARIABLE(source_function);
	UNUSED_VARIABLE(source_line);
	if (!memory) return; // standard defines free(NULL) as basically a no-op, so do so here.
	AllocationUnit* tag = FindAllocationUnit(memory);

	// If you hit this assert, you tried to deallocate memory that was either not allocated with this allocator 
	// or so badly damaged that it failed the preliminary test
	assert(tag != NULL);

	// We've warned the user about the tag being NULL, let's not actually try to do anything further with the memory
	if (tag == NULL) return;

	// If you hit this assert, the memory that is about to be deallocated is damaged, but you should have seen
	// an earlier assert in ValidateAllocationUnit
	assert(ValidateAllocationUnit(tag));

	// If you hit this assert, then this deallocation call was made from a source that isn't setup to use our
	// memory tracking system. Use the callstack to locate the source and include our memory tracker header.
	assert(type != DeallocType_Unknown);

	// If you hit this assert, you were trying to deallocate RAM that wasn't allocated in a way that's compatible
	// with the deallocation method requested. Simply put, your allocation/deallocation calls are mismatched.
	assert (
		(tag->AllocationType == AllocType_New		&& type == DeallocType_Delete) ||
		(tag->AllocationType == AllocType_VectorNew	&& type == DeallocType_VectorDelete) ||
		(tag->AllocationType == AllocType_Malloc	&& type == DeallocType_Free) ||
		(tag->AllocationType == AllocType_Calloc	&& type == DeallocType_Free) ||
		(tag->AllocationType == AllocType_Realloc	&& type == DeallocType_Free) ||
		(tag->AllocationType == AllocType_Unvalidated) || 
		(type == DeallocType_Unvalidated) 	 
	);

	// If you hit this assert, you requested that we break when this piece of memory came through to be deallocated
	assert(!tag->BreakOnFree);
	
	// We'll wipe the memory here with our "Released" pattern even though it likely won't do much good.
	WipeMemoryWithPattern(tag, RELEASED_PATTERN);

	// Nuke the allocation unit tag address
	*(AllocationUnit**)tag->ActualAddress = NULL;

	// Free the actual memory
	FastAllocatorGeneral::Get_Allocator()->Free(tag->ActualAddress);

	// Clearing the tracking information insures that if at some later time somebody calls our memory tracker
	// from an unknown source, we don't think it was the last allocation.
	ClearThreadLocalInformation();

	// Let's also stop tracking the now free'd memory and free our tracking tag.
	{
		FastCriticalSection::Lock lock(MemoryTrackerCS);
		tag->Remove();
		delete tag;
	}
};

POP_MEMORY_MACROS
#pragma warning(pop)
