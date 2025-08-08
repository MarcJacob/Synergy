SOURCE_INC_FILE()

#include "SynergyCore.h"

// STACK ALLOCATOR IMPLEMENTATION

void* StackAllocatorData::Alloc(ManagedBuffer& Buffer, size_t Size)
{
	StackAllocatorData& stack = *(StackAllocatorData*)Buffer.Buffer;

	// Use the current stack pointer as allocated address then advance the stack pointer by the allocation size.
	uint8_t* newAlloc = stack.stackPtr;
	stack.stackPtr += Size;

	// Write about this allocation at current stack pointer so that its size can be found again when freeing the
	// next allocation. Then advance the stack pointer once more to set it at the start of the next potential allocation.
	
	*(size_t*)(stack.stackPtr) = Size;
	stack.stackPtr += sizeof(size_t);

	size_t allocSizeTotal = (stack.stackPtr - newAlloc);

	// Update buffer info.
	Buffer.AllocatedByteCount += allocSizeTotal;

	// Set the latest allocation pointer to this allocation.
	stack.latestAllocSize = allocSizeTotal;

	return newAlloc;
}

void StackAllocatorData::Free(ManagedBuffer& Buffer, void* Ptr)
{
	StackAllocatorData& stack = *(StackAllocatorData*)Buffer.Buffer;

	uint8_t* latestAllocPtr = stack.stackPtr - stack.latestAllocSize;

	// Check that we are freeing the latest allocated pointer.
	if (Ptr != latestAllocPtr)
	{
		// TODO Build an assert system. Doing so without using static memory may be difficult...
		// Perhaps the rule of not using static memory in client and core library code could be only for non-debug stuff ?
		// But then even for a standard logging system it will be challenging.

		// Fail the free operation.
		return;
	}

	// Decrement allocated byte count by latest allocation size.
	Buffer.AllocatedByteCount -= stack.latestAllocSize;

	// Move stack pointer back. If the bottom of the stack was reached (allocated byte count == size of this Stack allocator data structure),
	// reset the stack data to its default empty state. Otherwise read in the size of the new "latest" allocation from just before the stack pointer.
	stack.stackPtr -= stack.latestAllocSize;
	
	if (Buffer.AllocatedByteCount > sizeof(StackAllocatorData))
	{
		stack.latestAllocSize = *(size_t*)(stack.stackPtr - sizeof(size_t));
	}
	else
	{
		// Bottom was reached.
		stack.latestAllocSize = 0;

		if (Buffer.AllocatedByteCount < sizeof(StackAllocatorData))
		{
			// ASSERT HERE (Something wrong with the freeing algo or stack data)
		}
	}

}

MemoryAllocator MakeStackAllocator(ByteBuffer Buffer, size_t BufferSize)
{
	MemoryAllocator newAllocator{};

	newAllocator.Memory.Buffer = Buffer;
	newAllocator.Memory.BufferSize = BufferSize;

	newAllocator.InternalAllocFuncPtr = StackAllocatorData::Alloc;
	newAllocator.InternalFreeFuncPtr = StackAllocatorData::Free;

	// Initialize stack allocator data
	StackAllocatorData& stackAllocData = *(StackAllocatorData*)Buffer;
	{
		stackAllocData.stackPtr = Buffer + sizeof(StackAllocatorData);
		stackAllocData.latestAllocSize = 0;
	}

	return newAllocator;
}

// ---------------------------------------------------------------------