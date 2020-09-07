#include <mc/string.h>


int strcmp( const CHAR_TYPE *s1, const CHAR_TYPE *s2 )
{
    while (*s1 && *s1 == *s2) { ++s1; ++s2; }
    return ( *s1 > *s2 ) - ( *s2  > *s1 );
}