/*
 * Copyright (C) 2013 Intel Coperation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BASIC_TYPE_H__
#define __BASIC_TYPE_H__

#include <stddef.h>         
#include <string.h>         
#include <assert.h>

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif

#define RETURN_VAL_IF_FAIL(condition, value) \
    if (!(condition)) \
      return (value);

typedef signed char        schar;
typedef signed char        int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;
typedef signed char        boolean;

#define UINT8_MAX    (( uint8) 0xFF)
#define UINT16_MAX   ((uint16) 0xFFFF)
#define UINT32_MAX   ((uint32) 0xFFFFFFFF)
#define UINT64_MAX   ((uint64) 0xFFFFFFFFFFFFFFFF)

#define INT8_MIN     ((  int8) 0x80)
#define INT8_MAX     ((  int8) 0x7F)
#define INT16_MIN    (( int16) 0x8000)
#define INT16_MAX    (( int16) 0x7FFF)
#define INT32_MIN    (( int32) 0x80000000)
#define INT32_MAX    (( int32) 0x7FFFFFFF)
#define INT64_MIN    (( int64) 0x8000000000000000)
#define INT64_MAX    (( int64) 0x7FFFFFFFFFFFFFFF)

#ifdef __x86_64__
#define INT_MAX   INT64_MAX
#define INT_MIN   INT64_MIN
#define UINT_MAX  UINT64_MAX
#else
#define INT_MAX   INT32_MAX
#define INT_MIN   INT32_MIN
#define UINT_MAX  UINT32_MAX
#endif
#define G_GNUC_UNUSED
#endif  // __BASICTYPES_H_
