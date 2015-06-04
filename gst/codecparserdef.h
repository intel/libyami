#ifndef __CODEC_PARSER_DEF__
#define __CODEC_PARSER_DEF__

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

#endif /*end of codecparserdef.h*/
