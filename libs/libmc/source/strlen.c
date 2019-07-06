#include <mc/string.h>


size_t strlen( const char16_t *str )
{
    if (!str) return 0;
    const char16_t *p = str;
    while (*p) ++p;
    return (size_t) (p - str);
}