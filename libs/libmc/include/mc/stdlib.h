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

#ifndef MACHINA_LIBMC_STDLIB_H
#define MACHINA_LIBMC_STDLIB_H


#include <sys/types.h>


#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define STACK_BUFFER_ALIGNED(name_, count_, align_, type_) \
    uint8_t *name_##_tmp[count_*sizeof(type_)+align_-1];  \
    type_ *name_ = (type_*) (((uintptr_t) &name_##_tmp + (align_-1)) & (~(align_-1)))

#define STACK_STRUCT_ALIGNED(name_, size_, align_, type_) \
    uint8_t *name_##_tmp[size_+align_-1];  \
    type_ &name_ = *((type_*) (((uintptr_t) &name_##_tmp + (align_-1)) & (~(align_-1))))

#endif // MACHINA_LIBMC_STDLIB_H