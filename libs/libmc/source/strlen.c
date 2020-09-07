#include <mc/string.h>


size_t strlen( const CHAR_TYPE *str )
{
    if (!str) return 0;
    const CHAR_TYPE *p = str;
    while (*p) ++p;
    return (size_t) (p - str);
}