#include <mc/string.h>


CHAR_TYPE *strcpy( CHAR_TYPE *dst, const CHAR_TYPE *src )
{
    if (dst == NULL || src == NULL) return NULL;
    while (*src) { *(dst++) = *(src++); }
    *dst = 0;
    return dst;
}