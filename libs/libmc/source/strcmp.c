#include <mc/string.h>


int strcmp( const char16_t *str1, const char16_t *str2 )
{
    const uint16_t *p1 = (const uint16_t*) str1;
    const uint16_t *p2 = (const uint16_t*) str2;
    while (*p1 && *p1 == *p2) { ++p1; ++p2; }
    return ( *p1 > *p2 ) - ( *p2  > *p1 );
}