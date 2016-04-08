/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
