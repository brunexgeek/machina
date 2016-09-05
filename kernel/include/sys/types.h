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


#if (__SIZEOF_POINTER__ == 4)

#define ARM_32 1

typedef __UINT32_TYPE__  size_t;
typedef __INT32_TYPE__   ssize_t;

#elif (__SIZEOF_POINTER__ == 8)

#define ARM_64 1

typedef __UINT64_TYPE__  size_t;
typedef __INT64_TYPE__   ssize_t;

#endif


#ifndef NULL
#define NULL ((void*)0)
#endif

#endif // MACHINA_TYPES_H