/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __CODEC_PARSER_DEF__
#define __CODEC_PARSER_DEF__

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

#ifndef TRUE
    #define TRUE true
#endif
#ifndef FALSE
    #define FALSE false
#endif

#define     G_STMT_START    do
#define     G_STMT_END      while(0)

#define g_once_init_enter(location) true
#define g_once_init_leave(location, result) *location=result

#define TRACE(...)
#define G_LIKELY(a) (a)
#define G_GSIZE_FORMAT "lu"
#define g_assert(expr) assert(expr)
#define G_UNLIKELY(expr) (__builtin_expect (_G_BOOLEAN_EXPR(expr), 0))

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

typedef struct _DebugCategory {
    /*< private >*/
    int32_t threshold;
    uint32_t color;		/* see defines above */
    const char* name;
    const char* description;
}DebugCategory;

#define DEBUG_CATEGORY(cat) DebugCategory *cat = NULL
#define DEBUG_CATEGORY_INIT(var,name,color,desc) G_STMT_START{ }G_STMT_END
#define _gst_debug_category_new(...) 0

typedef struct _GArray {
    char* data;
    uint32_t len;
}GArray;

typedef void(*GDestroyNotify)(void* data);  /*for h265parser.c:2482*/

#endif /*end of codecparserdef.h*/
