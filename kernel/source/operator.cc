#include <sys/Memory.hh>
#include <sys/system.h>


using machina::Memory;

/*
static void *__allocate(
	size_t size )
{
	// we use the first bytes to store the amount of allocated frames
	size += sizeof(size_t);

	// compute the amount of frames required
	size_t frames = (size + SYS_PAGE_SIZE - 1) & ~(SYS_PAGE_SIZE - 1);
	frames /= SYS_PAGE_SIZE;
	// allocate contiguous frames
	size_t *address = (size_t*) PhysicalMemory::getInstance().allocate(frames);
	if (address == nullptr) return nullptr;

	// store the amount of allocated pages
	*address = frames;
	++address;

	return address;
}


static void __free(
	void *ptr )
{
	ptr = (size_t*) ptr - 1;
	PhysicalMemory::getInstance().free( ptr, *((size_t*) ptr) );
}
*/

void *operator new(
	size_t size )
{
	return Memory::getInstance().allocate(size);
}


void *operator new[](
	size_t size )
{
	return Memory::getInstance().allocate(size);
}


void operator delete (
	void *ptr ) noexcept
{
	return Memory::getInstance().free(ptr);
}


void operator delete[] (
	void *ptr ) noexcept
{
	return Memory::getInstance().free(ptr);
}
