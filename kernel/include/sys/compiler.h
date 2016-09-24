#ifndef MACHINA_COMPILER_H
#define MACHINA_COMPILER_H


#define INLINE_ALWAYS                  __attribute__ ((always_inline))
#define INLINE_NEVER                   __attribute__ ((noinline))
#define OPTIMIZE_0                     __attribute__((optimize("O0")))

#endif // MACHINA_COMPILER_H