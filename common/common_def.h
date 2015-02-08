/*

common_def.h - basic va decoder for video *
Copyright (C) 2013-2014 Intel Corporation *
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public License
as published by the Free Software Foundation; either version 2.1
of the License, or (at your option) any later version. *
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details. *
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free
Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301 USA */

#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

#ifndef BOOL
#define BOOL  int
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif

#ifndef N_ELEMENTS
#define N_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef ALIGN_POW2
#define ALIGN_POW2(a, b) ((a + (b - 1)) & ~(b - 1))
#endif

#ifndef ALIGN8
#define ALIGN8(a) ALIGN_POW2(a, 8)
#endif

#ifndef ALIGN16
#define ALIGN16(a) ALIGN_POW2(a, 16)
#endif

#ifndef ALIGN32
#define ALIGN32(a) ALIGN_POW2(a, 32)
#endif

#ifndef RETURN_IF_FAIL
#define RETURN_IF_FAIL(condition) \
do{ \
  if (!(condition)) \
     return;  \
}while(0)
#endif

#ifndef RETURN_VAL_IF_FAIL
#define RETURN_VAL_IF_FAIL(condition, value) \
do{ \
  if (!(condition)) \
     return (value);  \
}while(0)
#endif

#ifndef __ENABLE_CAPI__
    #define PARAMETER_ASSIGN(a, b)  a = b
#else
    #define PARAMETER_ASSIGN(a, b)  memcpy(&(a), &(b), sizeof(b))
#endif
#endif //__COMMON_DEF_H__