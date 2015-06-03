/*
 *  commondef.h - basic macros and functions for codecparers
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef __COMMON_DEF_H
#define __COMMON_DEF_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include "common/log.h"

/*mixed-language compiling*/
#ifdef __cplusplus
    #define G_BEGIN_DECLS extern "C" {
#else
	#define G_BEGIN_DECLS
#endif

#ifdef __cplusplus
    #define G_END_DECLS }
#else
    #define G_END_DECLS
#endif

#define g_once_init_enter(location) true
#define g_once_init_leave(location, result) 

/***** padding for very extensible base classes *****/
#define PADDING_LARGE 20
/***** default padding of structures *****/
#define PADDING 4

#define     G_STMT_START    do
#define     G_STMT_END      while(0)

#define g_assert(expr) assert(expr)

#define G_GNUC_MALLOC __attribute__((__malloc__))
#define g_malloc(size) malloc(size)
#define g_malloc0(n_bytes) calloc(1, n_bytes)
#define g_realloc(ptr, size) realloc(ptr, size)
#define g_realloc_n(mem, n_blocks, n_block_bytes) \
		g_realloc(mem, n_blocks * n_block_bytes)
#define g_try_realloc(ptr, size) realloc(ptr, size)

#ifndef TRUE
    #define TRUE true
#endif

#ifndef FALSE
    #define FALSE false
#endif

#ifndef G_MAXUINT32
    #define G_MAXUINT32	((unsigned int)0xffffffff)
#endif

#define G_LIKELY(a) (a)
#define TRACE(...)

#ifndef MIN(a, b)
#define MIN(a, b) (((a)>(b))?(b):(a))
#endif

#ifndef MAX(a, b)
#define MAX(a, b) (((a)>(b)?(a):(b)))
#endif

#define G_N_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))

/*The maximum value which can be held in a signed char*/
#define G_MAXINT8	((signed char)  0x7f)
/*The maximum value which can be held in a unsigned short*/
#define G_MAXUINT16	((unsigned short) 0xffff)
/*The maximum value which can be held in a unsigned int*/
#define G_MAXUINT UINT_MAX

/* Define PUT and GET functions for unaligned memory */
#define READ_DATA(__data, __idx, __size, __shift) \
    (((uint##__size##_t) (((const uint8_t *) (__data))[__idx])) << (__shift))

#define _GST_PUT(__data, __idx, __size, __shift, __num) \
		    (((uint8_t *) (__data))[__idx] = (((uint##__size##_t) (__num)) >> (__shift)) & 0xff)

#define GST_READ_UINT8(data)	(READ_DATA (data, 0,  8,  0))

#define GST_READ_UINT16_BE(data)	(READ_DATA (data, 0, 16,  8) | \
				 READ_DATA (data, 1, 16,  0))

#define GST_READ_UINT16_LE(data)	(READ_DATA (data, 1, 16,  8) | \
				 READ_DATA (data, 0, 16,  0))

#define GST_READ_UINT24_BE(data)	(READ_DATA (data, 0, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 2, 32,  0))

#define GST_READ_UINT24_LE(data)	(READ_DATA (data, 2, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 0, 32,  0))

#define GST_READ_UINT32_BE(data)	(READ_DATA (data, 0, 32, 24) | \
				 READ_DATA (data, 1, 32, 16) | \
				 READ_DATA (data, 2, 32,  8) | \
				 READ_DATA (data, 3, 32,  0))

#define GST_READ_UINT32_LE(data)	(READ_DATA (data, 3, 32, 24) | \
				 READ_DATA (data, 2, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 0, 32,  0))

#define GST_READ_UINT64_BE(data)    (READ_DATA (data, 0, 64, 56) | \
				 READ_DATA (data, 1, 64, 48) | \
				 READ_DATA (data, 2, 64, 40) | \
				 READ_DATA (data, 3, 64, 32) | \
				 READ_DATA (data, 4, 64, 24) | \
				 READ_DATA (data, 5, 64, 16) | \
				 READ_DATA (data, 6, 64,  8) | \
				 READ_DATA (data, 7, 64,  0))

#define GST_READ_UINT64_LE(data)    (READ_DATA (data, 7, 64, 56) | \
				 READ_DATA (data, 6, 64, 48) | \
				 READ_DATA (data, 5, 64, 40) | \
				 READ_DATA (data, 4, 64, 32) | \
				 READ_DATA (data, 3, 64, 24) | \
				 READ_DATA (data, 2, 64, 16) | \
				 READ_DATA (data, 1, 64,  8) | \
				 READ_DATA (data, 0, 64,  0))

/**
 * GST_WRITE_UINT8:
 * @data: memory location
 * @num: value to store
 *
 * Store an 8 bit unsigned integer value into the memory buffer.
 */
#define GST_WRITE_UINT8(data, num)  do { \
		                               _GST_PUT (data, 0,  8,  0, num); \
		                            } while (0)

/**
 * GST_WRITE_UINT16_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 16 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT16_LE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 16,  0, num); \
		                                   _GST_PUT (__put_data, 1, 16,  8, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT16_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 16 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT16_BE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 16,  8, num); \
		                                   _GST_PUT (__put_data, 1, 16,  0, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT24_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 24 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT24_LE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 32,  0, num); \
		                                   _GST_PUT (__put_data, 1, 32,  8, num); \
		                                   _GST_PUT (__put_data, 2, 32,  16, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT24_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 24 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT24_BE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 32,  16, num); \
		                                   _GST_PUT (__put_data, 1, 32,  8, num); \
		                                   _GST_PUT (__put_data, 2, 32,  0, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT32_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT32_LE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 32,  0, num); \
		                                   _GST_PUT (__put_data, 1, 32,  8, num); \
		                                   _GST_PUT (__put_data, 2, 32, 16, num); \
		                                   _GST_PUT (__put_data, 3, 32, 24, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT32_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 32 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT32_BE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 32, 24, num); \
		                                   _GST_PUT (__put_data, 1, 32, 16, num); \
		                                   _GST_PUT (__put_data, 2, 32,  8, num); \
		                                   _GST_PUT (__put_data, 3, 32,  0, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT64_LE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit unsigned integer value in little endian format into the memory buffer.
 */
#define GST_WRITE_UINT64_LE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 64,  0, num); \
		                                   _GST_PUT (__put_data, 1, 64,  8, num); \
		                                   _GST_PUT (__put_data, 2, 64, 16, num); \
		                                   _GST_PUT (__put_data, 3, 64, 24, num); \
		                                   _GST_PUT (__put_data, 4, 64, 32, num); \
		                                   _GST_PUT (__put_data, 5, 64, 40, num); \
		                                   _GST_PUT (__put_data, 6, 64, 48, num); \
		                                   _GST_PUT (__put_data, 7, 64, 56, num); \
		                                } while (0)

/**
 * GST_WRITE_UINT64_BE:
 * @data: memory location
 * @num: value to store
 *
 * Store a 64 bit unsigned integer value in big endian format into the memory buffer.
 */
#define GST_WRITE_UINT64_BE(data, num)  do { \
		                                   void* __put_data = data; \
		                                   _GST_PUT (__put_data, 0, 64, 56, num); \
		                                   _GST_PUT (__put_data, 1, 64, 48, num); \
		                                   _GST_PUT (__put_data, 2, 64, 40, num); \
		                                   _GST_PUT (__put_data, 3, 64, 32, num); \
		                                   _GST_PUT (__put_data, 4, 64, 24, num); \
		                                   _GST_PUT (__put_data, 5, 64, 16, num); \
		                                   _GST_PUT (__put_data, 6, 64,  8, num); \
		                                   _GST_PUT (__put_data, 7, 64,  0, num); \
		                                } while (0)

inline static float
GST_READ_FLOAT_LE(const uint8_t *data)
{
    union
    {
        uint32_t i;
        float f;
    } u;

    u.i = GST_READ_UINT32_LE (data);
    return u.f;
}

inline static float
GST_READ_FLOAT_BE(const uint8_t *data)
{
    union
    {
        uint32_t i;
        float f;
    } u;

    u.i = GST_READ_UINT32_BE (data);
    return u.f;
}

inline static double
GST_READ_DOUBLE_BE(const uint8_t *data)
{
    union
    {
        uint64_t i;
        double d;
    } u;

    u.i = GST_READ_UINT64_BE (data);
    return u.d;
}

inline static double
GST_READ_DOUBLE_LE(const uint8_t *data)
{
    union
    {
        uint64_t i;
        double d;
    } u;

    u.i = GST_READ_UINT64_LE (data);
    return u.d;
}

inline static void
GST_WRITE_FLOAT_LE(uint8_t *data, float num)
{
    union
    {
        uint32_t i;
        float f;
    } u;

    u.f = num;
    GST_WRITE_UINT32_LE (data, u.i);
}

inline static void
GST_WRITE_FLOAT_BE(uint8_t *data, float num)
{
    union
    {
        uint32_t i;
        float f;
    } u;

    u.f = num;
    GST_WRITE_UINT32_BE (data, u.i);
}

inline static void
GST_WRITE_DOUBLE_LE(uint8_t *data, double num)
{
    union
    {
        uint64_t i;
        double d;
    } u;

    u.d = num;
    GST_WRITE_UINT64_LE (data, u.i);
}

inline static void
GST_WRITE_DOUBLE_BE(uint8_t *data, double num)
{
    union
    {
        uint64_t i;
        double d;
    } u;

    u.d = num;
    GST_WRITE_UINT64_BE (data, u.i);
}

static inline uint32_t 
g_bit_storage(uint32_t value)
{
  uint32_t bits = 0;
  while (value >> bits)
     bits ++;
  
  return bits;
}

#define g_new(struct_type, n_structs)  \
    (struct_type*)malloc(sizeof(struct_type)*(n_structs))

#define g_new0(struct_type, n_structs)  \
    ({ struct_type* tmp = (struct_type*)malloc(sizeof(struct_type)*(n_structs)); \
    if(tmp) \
        memset(tmp, 0, sizeof(struct_type)*(n_structs));\
	tmp; })

#define g_slice_new(struct_type) \
    (struct_type*)malloc(sizeof(struct_type))

#define g_slice_new0(struct_type) \
    ({ struct_type* tmp = (struct_type*)malloc(sizeof(struct_type)); \
    if(tmp) \
        memset(tmp, 0, sizeof(struct_type));\
	tmp; })

#define g_slice_free(type, mem) \
    free(mem)

inline static void* 
g_memdup (const void* mem, uint32_t byte_size)
{
    void* new_mem;

    if (mem)
    {
        new_mem = malloc(byte_size);
        memcpy (new_mem, mem, byte_size);
    }
    else
    new_mem = NULL;

    return new_mem;
}

inline static void
g_free (void* mem)
{
    if(mem == NULL)
        return;
    else
        free(mem);
}

#define g_return_if_fail(expr) { \
    if (!(expr)) \
    { \
        return;	\
    }; }

#define g_return_val_if_fail(expr, val)	{ \
    if (!(expr)) \
    { \
        return val; \
    }; }

#define _G_BOOLEAN_EXPR(expr) \
    __extension__ ({ \
        int _g_boolean_var_; \
        if (expr) \
            _g_boolean_var_ = 1; \
        else \
            _g_boolean_var_ = 0; \
            _g_boolean_var_; \
	})

#define G_UNLIKELY(expr) (__builtin_expect (_G_BOOLEAN_EXPR(expr), 0))

#define G_GSIZE_FORMAT "lu"

typedef struct _DebugCategory {
    /*< private >*/
    int32_t threshold;
    uint32_t color;		/* see defines above */
    const char* name;
    const char* description;
}DebugCategory;

#define DEBUG_CATEGORY(cat) DebugCategory *cat = NULL
#define DEBUG_CATEGORY_INIT
#define _gst_debug_category_new(...) 0

typedef struct _GArray {
    char* data;
    uint32_t len;
}GArray;

#define VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME    3   /* frame-tag */
#define VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME        10  /* frame tag + magic number + frame width/height */

typedef void(*GDestroyNotify)(void* data);  /*for h265parser.c:2482*/

#endif /*end of commondef.h*/
