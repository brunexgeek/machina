//
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

#include <mc/string.h>


static void __memcpy_neon(
	void *output,
	const void *input,
	size_t size )
{
	asm ("NEONCopyPLD:\n"
	    "PLD [%0,  #0xC0]\n"
	    "VLDM %0!, {d0-d7}\n"
	    "VSTM %1!, {d0-d7}\n"
	    "SUBS %2,  %2,#0x40\n"
	    "BGE NEONCopyPLD\n" : "=r" (input), "=r" (output), "=r" (size) );
}



void *memcpy(
	void *output,
	const void *input,
	size_t size )
{
	const uint8_t *from = (const uint8_t*) input;
	uint8_t *to = (uint8_t*) output;

	for (size_t i = 0; i < size; ++i)
		*to++ = *from++;

	return output;
}