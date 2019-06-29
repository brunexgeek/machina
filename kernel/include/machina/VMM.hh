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

#ifndef MACHINA_VMM_H
#define MACHINA_VMM_H


#include <sys/types.h>
#include <sys/Screen.hh>


namespace machina {


class TextScreen;


/**
 * @brief Structure to hold a short-descriptor section entry.
 */
struct SectionDescriptor
{
	uint32_t
		type   : 2,     // PTE type (section/supersection == 0x02)
		B      : 1,     // Buffer bit
		C      : 1,     // Cache bit
		XN     : 1,     // Execute-never restrictions on instruction fetching [B3-1359]
		domain : 4,     // Domain field [B3-1362]
		impl   : 1,     // Implementation defined
		AP10   : 2,     // Access permissions bit [B3-1356]
		TEX    : 3,     // Memory region attributes [B3-1366]
		AP2    : 1,     // Access permissions bit [B3-1356]
		S      : 1,     // Shareable bit [B3-1366]
		NG     : 1,     // Non-global bit [B3-1378]
		zero   : 1,     // Section (0)
		NS     : 1,     // Non-secure bit
		base   : 12;    // Base address (1MB aligned)
} __attribute__((packed));


struct PageTable
{
	/**
	 * @brief L1 page table.
	 */
	uint32_t L1[4096];

	/**
	 * @brief Pointer to preallocated physical frames to be
	 * used to create more L2 tables.
	 */
	uint8_t *buffer;

	/**
	 * @brief Number of bytes of available memory in the buffer.
	 */
	size_t bufferSize;
};


class VMM
{
	public:
		VMM();

		~VMM();

		static VMM &getInstance();

		void initialize();

		static void printL1();

	private:
		void setDomainPerm(
			size_t domain,
			size_t perm );
};


PageTable *VmCreatePageTable();


} // machina

#endif // MACHINA_VMM_H