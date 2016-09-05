#ifndef MACHINA_STRING_H
#define MACHINA_STRING_H

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif


size_t strlen(
    const char *text );

void memset(
    void *output,
    uint8_t value,
    size_t size );

void memmove(
    void *output,
    const void *input,
    size_t size );

void memcpy(
    void *output,
    const void *input,
    size_t size );


#ifdef __cplusplus
}
#endif


#endif // MACHINA_STRING_H