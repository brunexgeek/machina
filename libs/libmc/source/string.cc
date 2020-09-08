#include <mc/string.h>


size_t strlen( const CHAR_TYPE *str )
{
    if (!str) return 0;
    const CHAR_TYPE *p = str;
    while (*p) ++p;
    return (size_t) (p - str);
}

int strcmp( const CHAR_TYPE *s1, const CHAR_TYPE *s2 )
{
    while (*s1 && *s1 == *s2) { ++s1; ++s2; }
    return ( *s1 > *s2 ) - ( *s2  > *s1 );
}

int strncmp( const CHAR_TYPE *s1, const CHAR_TYPE *s2, size_t c )
{
    if (c == 0) return 0;
    while (--c && *s1 && *s1 == *s2) { ++s1; ++s2; }
    return (int)( *s1 > *s2 ) - (int)( *s2  > *s1 );
}

CHAR_TYPE *strcpy( CHAR_TYPE *dst, const CHAR_TYPE *src )
{
    if (dst == NULL || src == NULL) return NULL;
    while (*src) { *(dst++) = *(src++); }
    *dst = 0;
    return dst;
}

CHAR_TYPE *strncpy( CHAR_TYPE *dst, const CHAR_TYPE *src, size_t num )
{
    if (dst == NULL || src == NULL || num == 0) return NULL;
    while (num-- && *src) { *dst++ = *src++; }
    while (num--) *dst++ = 0;
    return dst;
}

void *memset( void *ptr, int value, size_t num )
{
    uint8_t *p = (uint8_t*) ptr;
    while (num-- > 0) *p = (uint8_t) value;
    return ptr;
}

void *memcpy( void *output, const void *input, size_t size )
{
    uint8_t *i = (uint8_t*) input;
    uint8_t *o = (uint8_t*) output;
    while (size-- > 0) *i++ = *o++;
    return output;
}