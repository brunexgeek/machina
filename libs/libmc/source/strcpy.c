#include <mc/string.h>


char16_t *strcpy( char16_t *dst, const char16_t *src )
{
    if (dst == NULL || src == NULL) return NULL;
    while (*src) { *(dst++) = *(src++); }
    *dst = 0;
    return dst;
}