
#include <mc/string.h>


int strncmp( const CHAR_TYPE *s1, const CHAR_TYPE *s2, size_t c )
{
    if (c == 0) return 0;
    while (--c && *s1 && *s1 == *s2) { ++s1; ++s2; }
    return (int)( *s1 > *s2 ) - (int)( *s2  > *s1 );
}
