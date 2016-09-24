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


size_t StringLengthEx(
	const char16_t *text,
	size_t size );

size_t StringLength(
	const char16_t *text );

int FormatStringEx(
	char16_t *output,
	size_t outputSize,
	const char16_t *format,
	va_list args );

int FormatString(
	char16_t *buffer,
	size_t size,
	const char16_t *format,
	... );


#ifdef __cplusplus
}
#endif


#endif // MACHINA_LIBMC_STRING_H
