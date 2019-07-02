#include <sys/heap.hh>
#include <sys/system.h>


void *operator new(
	size_t size )
{
	return machina::heap_allocate(size);
}


void *operator new[](
	size_t size )
{
	return machina::heap_allocate(size);
}


void operator delete (
	void *ptr ) noexcept
{
	machina::heap_free(ptr);
}


void operator delete[] (
	void *ptr ) noexcept
{
	machina::heap_free(ptr);
}
