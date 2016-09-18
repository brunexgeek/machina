#include <machina/Heap.hh>
#include <sys/system.h>


using machina::Heap;


void *operator new(
	size_t size )
{
	return Heap::getInstance().allocate(size);
}


void *operator new[](
	size_t size )
{
	return Heap::getInstance().allocate(size);
}


void operator delete (
	void *ptr ) noexcept
{
	return Heap::getInstance().free(ptr);
}


void operator delete[] (
	void *ptr ) noexcept
{
	return Heap::getInstance().free(ptr);
}
