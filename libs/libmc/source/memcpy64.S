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


#include <sys/arm.h>


#if (RPIGEN != 1)


/**
 * @brief Copy @c r2 bytes from @c r1 to @c r0.
 *
 * The input address and output address must be word aligned. The size must be
 * 64-bytes aligned. This function uses NEON to perform fast copy without
 * trashing ARM registers.
 *
 * This function follows the ABI and can be called from C code.
 */

BEGIN_ASM_FUNCTION(memcpy64)
	mov         r3, r0
1:  pld         [r1, #0xC0]
	vldm        r1!, {d0-d7}
	vstm        r0!, {d0-d7}
	subs        r2, r2, #0x40
	bge         1b
	mov         r0, r3
	bx          lr
END_ASM_FUNCTION(memcpy64)

#endif