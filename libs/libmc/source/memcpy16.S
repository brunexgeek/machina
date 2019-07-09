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

/*
 * For more details, check the article "What is the fastest way to copy memory on a Cortex-A8?"
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka13544.html
 */


#include <sys/arm.h>


/**
 * @brief Copy @c r2 bytes from @c r1 to @c r0.
 *
 * The input address and output address must be word aligned. The size must be
 * 16-bytes aligned.
 *
 * This function follows the ABI and can be called from C code.
 */

BEGIN_ASM_FUNCTION(memcpy16)
	push        {r0, r4-r6}
1:	pld         [r1, #64]
	ldmia       r1!, {r3-r6}
	stmia       r0!, {r3-r6}
	subs        r2, #16
	bne	        1b
	pop         {r0, r4-r6}
	bx          lr
END_ASM_FUNCTION(memcpy16)