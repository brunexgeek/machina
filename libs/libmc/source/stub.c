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

#include <mc/stdlib.h>


#ifdef __arm__


void *__dso_handle;


void __cxa_pure_virtual()
{
	while (1);
}


int __aeabi_idiv0(
	int result )
{
	while (1);
	return result;
}


int __aeabi_atexit (
	void * object,
	void (*destroyer)(void*),
	void *dsoHandle )
{
	(void) object;
	(void) destroyer;
	(void) dsoHandle;
	//return __cxa_atexit(destroyer, object, dsoHandle);
	return 0;
}


#endif // __arm__