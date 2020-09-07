#ifndef MACHINA_TYPES_H
#define MACHINA_TYPES_H


typedef __UINT8_TYPE__   uint8_t;
typedef __UINT16_TYPE__  uint16_t;
typedef __UINT32_TYPE__  uint32_t;
typedef __UINT64_TYPE__  uint64_t;
typedef __INT8_TYPE__    int8_t;
typedef __INT16_TYPE__   int16_t;
typedef __INT32_TYPE__   int32_t;
typedef __INT64_TYPE__   int64_t;

#ifndef __cplusplus
typedef __UINT_LEAST16_TYPE__ char16_t;
#endif

#if (__SIZEOF_POINTER__ == 4)

#define ARM_32 1

// Newer GCC compilers do not accept 'uint32_t' and 'int32_t'
typedef unsigned int  size_t;
typedef signed int    ssize_t;

#elif (__SIZEOF_POINTER__ == 8)

#define ARM_64 1

typedef __UINT64_TYPE__  size_t;
typedef __INT64_TYPE__   ssize_t;

#endif

typedef uint64_t time_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint64_t ino_t;
typedef uint64_t off64_t;
typedef size_t ptrdiff_t;
typedef size_t intmax_t;
typedef size_t uintptr_t;

#ifndef NULL
	#ifdef __cplusplus
		#define NULL 0
	#else
		#define NULL (void*)0
	#endif
#endif

#ifndef __cplusplus
#define bool int
#define true 1
#define false 0
#endif

#define CHAR_TYPE  char
#define UCHAR_TYPE  uint8_t

// TODO: not the best place for this
typedef uint16_t Color;

/**
 * @brief Computes the RGB565 color from individual RGB components.
 */
#define RGB565_COLOR(rgb) \
	( ( ( (rgb >> 16) >> 3 ) << 11 ) | \
	  ( ( (rgb >> 8 & 0xFF) >> 2 ) << 5 ) | \
	  ( (rgb & 0xFF) >> 3 ) )



#endif // MACHINA_TYPES_H