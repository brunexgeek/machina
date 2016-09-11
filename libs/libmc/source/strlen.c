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

#include <mc/string.h>


size_t mc_strnlen(
	const char16_t *text,
	size_t size )
{
	const char16_t *ptr = text;

	for (; *ptr != 0 && size--; ++ptr);

	return (size_t) (ptr - text);
}


size_t mc_strlen(
	const char16_t *text )
{
	size_t length = 0;

	for (; *(text + length) != 0; ++length);

	return length;
}