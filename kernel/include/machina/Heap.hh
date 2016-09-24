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

#ifndef MACHINA_HEAP_HH
#define MACHINA_HEAP_HH


#include <sys/types.h>
#include <sys/Screen.hh>


namespace machina {


class Heap
{
	public:
		~Heap();

		static Heap &getInstance();

		void *allocate(
			size_t size );

		void free(
			void * address );

		void print(
			TextScreen &screen );

		void initialize();

	private:
		static Heap instance;

		Heap();
};


} //machina


#endif // MACHINA_HEAP_HH