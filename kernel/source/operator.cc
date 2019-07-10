#include <sys/heap.h>
#include <sys/system.h>


void *operator new(
	size_t size )
{
	return heap_allocate(size);
}


void *operator new[](
	size_t size )
{
	return heap_allocate(size);
}


void operator delete (
	void *ptr ) noexcept
{
	heap_free(ptr);
}


void operator delete[] (
	void *ptr ) noexcept
{
	heap_free(ptr);
}
