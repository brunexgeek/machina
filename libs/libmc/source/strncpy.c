#include <mc/string.h>


CHAR_TYPE *strncpy( CHAR_TYPE *dst, const CHAR_TYPE *src, size_t num )
{
    if (dst == NULL || src == NULL || num == 0) return NULL;
    while (num-- && *src) { *dst++ = *src++; }
    while (num--) *dst++ = 0;
    return dst;
}