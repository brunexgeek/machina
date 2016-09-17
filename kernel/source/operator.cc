#include <sys/Memory.hh>
#include <sys/system.h>


using machina::Memory;


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
