// Memory management tools usable by projects linked with the Synergy Core lib.

#include <stdint.h>

typedef uint8_t* ByteBuffer;

/*
	Object structure defining a segment of managed memory, usually owned by a Memory Allocator.
*/
struct ManagedBuffer
{
	// Memory exclusively managed by this allocator.
	ByteBuffer Buffer = nullptr;

	// Size of the managed memory.
	size_t BufferSize = 0;

	// Number of bytes that are currently allocated.
	size_t AllocatedByteCount = 0;
};

/*
	Object structure that defines any sort of Memory Allocator.

	A Memory Allocator does two things - it receives and responds to requests to allocate memory, and free memory.
	It does so following a specific strategy and resources that are only relevant when creating the allocator.
	Beyond that, it should be possible to pass it around in the quality of a memory allocator that does its job - no more,
	no less.

	With this idea in mind, and without using polymorphism, a Memory Allocator can therefore work with its allocated buffer
	of memory, and function pointers linking it to its actual behavior.

	Whatever specific state data the actual allocators need is contained at the beginning of the managed memory.
*/
struct MemoryAllocator
{
	ManagedBuffer Memory = {};

	void Allocate(size_t Size) { InternalAllocFuncPtr(Memory, Size); }
	void Free(void* Ptr) { InternalFreeFuncPtr(Memory, Ptr); };

	typedef void* AllocationFunction(ManagedBuffer& Buffer, size_t SizeToAllocate);
	AllocationFunction* InternalAllocFuncPtr = nullptr;

	typedef void FreeFunction(ManagedBuffer& Buffer, void* PreviousAllocPtr);
	FreeFunction* InternalFreeFuncPtr = nullptr;
};

/*
	State data structure for a Stack Allocator.
	A Stack allocator allocates and frees its memory sequentially using a single position pointer to grows or shrinks as needed.
	Pros:
	- Extremely fast allocations and deallocations

	Cons:
	- Allocations and deallocations must happen on a FILO basis (First In Last Out) which can be challenging architecturally.		
*/
struct StackAllocatorData
{
	// Current address of allocatable memory.
	uint8_t* stackPtr;

	// Size of the allocation currently at the top of the stack.
	size_t latestAllocSize;

	// Alloc and Free functions to be assigned to a new Memory Allocator object wishing to use the Stack Allocator implementation.
	static MemoryAllocator::AllocationFunction Alloc;
	static MemoryAllocator::FreeFunction Free;
};

MemoryAllocator MakeStackAllocator(ByteBuffer Buffer, size_t BufferSize);