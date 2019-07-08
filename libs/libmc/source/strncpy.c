#include <mc/string.h>


char16_t *strncpy( char16_t *dst, const char16_t *src, size_t num )
{
    if (dst == NULL || src == NULL || num == 0) return NULL;
    while (num-- && *src) { *dst++ = *src++; }
    while (num--) *dst++ = 0;
    return dst;
}