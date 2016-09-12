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


void *mc_memcpy(
	void *output,
	const void *input,
	size_t size )
{
	size_t aligned;
	void *out = output;

	// deals with unaligned buffers
	if ( (size_t) output % sizeof(size_t) != 0 ||
	     (size_t) input  % sizeof(size_t) != 0)
	{
		// check if the buffers are equally unaligned
		if ( (size_t) output % sizeof(size_t) == (size_t) input  % sizeof(size_t))
		{
			aligned = ( ( (size_t) output + 0x03 ) & ~0x03U ) - (size_t) output;
			// align the buffers
			for (; aligned > 0; --size, --aligned)
				*(uint8_t*)output++ = *(uint8_t*)input++;
		}
		else
		{
			// performs a slow byte copy
			for (; size != 0; --size)
				*(uint8_t*)output++ = *(uint8_t*)input++;
			return out;
		}
	}

#if (RPIGEN > 1)
	// check if we can copy in blocks of 64-bytes (and
	// we have at least 128 bytes to copy)
	aligned = size & ~0x3FU;
	if (aligned > 127)
	{
		mc_memcpy64(output, input, aligned);
		output = (uint8_t*) output + aligned;
		input  = (uint8_t*) input + aligned;
		size &= 0x3FU;
	}

#endif

	// check if we can copy in blocks of 16-bytes
	aligned = size & ~0x0FU;
	if (aligned != 0)
	{
		mc_memcpy16(output, input, aligned);
		output = (uint8_t*) output + aligned;
		input  = (uint8_t*) input + aligned;
		size &= 0x0FU;
	}

	if (size == 0) return out;

	// copy the trailing unaligned bytes
	for (; size != 0; --size)
		*(uint8_t*)output++ = *(uint8_t*)input++;

	return out;
}
