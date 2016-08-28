#include <sys/stdlib.hh>


#ifdef __arm__


#ifdef __cplusplus
extern "C" {
#endif


void *__dso_handle;


void __cxa_pure_virtual()
{
    while (true);
}


int __aeabi_idiv0(
    int result )
{
    while (true);
    return result;
}


int __aeabi_atexit (
    void *object,
    void (*destroyer)(void*),
    void *dsoHandle )
{
	//return __cxa_atexit(destroyer, object, dsoHandle);
    return 0;
}



size_t strlen(
    const char *text )
{
    size_t length = 0;

    for (; *(text + length) != 0; ++length);

    return length;
}


void memset(
    void *output,
    uint8_t value,
    size_t size )
{
    uint8_t *ptr = (uint8_t*) output;
    for (size_t i = 0; i < size; ++i)
        *ptr++ = value;
}


void memmove(
    void *output,
    const void *input,
    size_t size )
{
    memcpy(output, input, size);
}


void memcpy(
    void *output,
    const void *input,
    size_t size )
{
    uint8_t *from = (uint8_t*) input;
    uint8_t *to   = (uint8_t*) output;

    for (size_t i = 0; i < size; ++i)
        *to++ = *from++;
}


#ifdef __cplusplus
}
#endif


#endif // __arm__