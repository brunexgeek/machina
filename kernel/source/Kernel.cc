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

#include <machina/Kernel.hh>
#include <sys/Display.hh>


namespace machina {

void KernelPanic()
{
	Display::getInstance().drawSomething(0, 0, 0xf800);
	Display::getInstance().drawSomething(0, 1, 0xf800);
	Display::getInstance().drawSomething(0, 2, 0xf800);
	asm volatile("wfi");
}

} // machina