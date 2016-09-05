//    Copyright 2016 Bruno Ribeiro
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.

#ifndef MACHINA_LIBMC_STRING_H
#define MACHINA_LIBMC_STRING_H


#include <mc/stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif


int vsprintf(
	char *buffer,
	const char *format,
	va_list args );

int sprintf(
	char *buffer,
	const char *format,
	... );

void *memcpy16(
    void *output,
    const void *input,
    size_t size );

void *memcpy(
    void *output,
    const void *input,
    size_t size );

void memset(
    void *output,
    uint8_t value,
    size_t size );

size_t strlen(
    const char *text );


#ifdef __cplusplus
}
#endif


#endif // MACHINA_LIBMC_STRING_H