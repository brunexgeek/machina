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

#ifndef MACHINA_LIBMC_STDLIB_H
#define MACHINA_LIBMC_STDLIB_H


#include <sys/types.h>


#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


#ifdef __cplusplus
extern "C" {
#endif




void memmove(
    void *output,
    const void *input,
    size_t size );


#ifdef __cplusplus
}
#endif


#endif // MACHINA_LIBMC_STDLIB_H