#include <sys/PhysicalMemory.hh>
#include <sys/system.h>


using machina::PhysicalMemory;


static void *__allocate(
	size_t size )
{
	// we use the first bytes to store the amount of allocated frames
	size += sizeof(size_t);

	// compute the amount of frames required
	size_t frames = (size + SYS_PAGE_SIZE - 1) & ~(SYS_PAGE_SIZE - 1);
	frames /= SYS_PAGE_SIZE;
	// allocate contiguous frames
	size_t index = PhysicalMemory::getInstance().allocate(frames);
	if (index == ~((size_t)0x00)) return nullptr;

	// store the size
	size_t *ptr = (size_t*) (index * SYS_PAGE_SIZE);
	*ptr = frames;

	return ptr + 1;
}


static void __free(
	void *ptr )
{
	size_t size = *((size_t*) ptr);
	size_t frame = (size_t) ptr / SYS_PAGE_SIZE;
	PhysicalMemory::getInstance().free(frame, size);
}


void *operator new(
	size_t size )
{
	return __allocate(size);
}


void *operator new[](
	size_t size )
{
	return __allocate(size);
}


void operator delete (
	void *ptr ) noexcept
{
	if (ptr != nullptr) __free(ptr);
}


void operator delete[] (
	void *ptr ) noexcept
{
	if (ptr != nullptr) __free(ptr);
}
