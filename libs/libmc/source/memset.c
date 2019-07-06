#include <mc/string.h>
#include <mc/memory.h>


void *memset( void *ptr, int value, size_t num )
{
    return FillMemory(ptr, value, num);
}
