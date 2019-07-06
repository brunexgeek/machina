
#include <mc/string.h>


int strncmp( const char16_t *str1, const char16_t *str2, size_t count )
{
    const uint16_t *p1 = (const uint16_t*) str1;
    const uint16_t *p2 = (const uint16_t*) str2;
    while (--count && *p1 && *p1 == *p2) { ++p1; ++p2; }
    return (int)( *p1 > *p2 ) - (int)( *p2  > *p1 );
}
