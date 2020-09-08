/*
 *    Copyright 2016 Bruno Ribeiro
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef MACHINA_LIBMC_STRING_H
#define MACHINA_LIBMC_STRING_H


#include <mc/stdarg.h>
#include <sys/types.h>
#include <sys/compiler.h>


#ifdef __cplusplus
extern "C" {
#endif


int strcmp( const CHAR_TYPE *str1, const CHAR_TYPE *str2 );

int strncmp( const CHAR_TYPE *str1, const CHAR_TYPE *str2, size_t count );

CHAR_TYPE *strncpy( CHAR_TYPE *dst, const CHAR_TYPE *src, size_t num );

CHAR_TYPE *strcpy( CHAR_TYPE *dst, const CHAR_TYPE *src );

size_t strlen( const CHAR_TYPE *str );

void *memset( void *ptr, int value, size_t num );

void *memcpy( void *output, const void *input, size_t size );

#ifdef __cplusplus
}
#endif


#endif // MACHINA_LIBMC_STRING_H
