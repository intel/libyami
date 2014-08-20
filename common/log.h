/*
 * Copyright (C) 2013 Intel Coperation.
 *    Author: Xin Tang <xin.t.tang@intel.com>
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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef ANDROID
#include <utils/Log.h>
#define ERROR(...)   ALOGE(__VA_ARGS__);
#define INFO(...)    ALOGI(__VA_ARGS__);
#define WARNING(...) ALOGV(__VA_ARGS__);
#define DEBUG(...)   ALOGV(__VA_ARGS__);
#else
#include <stdio.h>
#include <assert.h>
extern int yamiLogFlag;
extern FILE* yamiLogFn;
extern int isIni;

#define YAMI_LOG_WARNING 0x1
#define YAMI_LOG_INFO 0x2
#define YAMI_LOG_DEBUG 0x3

#ifndef YAMIMESSAGE
#define yamiMessage(stream, format, ...)  do {\
  fprintf(stream, format, ##__VA_ARGS__); \
}while (0)
#endif

#ifndef ERROR
#define ERROR(format, ...)   do { \
  if (yamiLogFn) \
   yamiMessage(yamiLogFn, "libyami error(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
   yamiMessage(stderr, "libyami error(%s, %d): " format "\n", __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifdef __ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)   do { \
  if (yamiLogFlag >= YAMI_LOG_INFO) \
   yamiMessage(yamiLogFn, "yami info(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef WARNING
#define WARNING(format, ...)   do { \
  if (yamiLogFlag >= YAMI_LOG_WARNING) \
   yamiMessage(yamiLogFn, "yami warning(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)   do { \
  if (yamiLogFlag >= YAMI_LOG_DEBUG) \
   yamiMessage(yamiLogFn, "yami debug(%s, %d): " format "\n",  __func__, __LINE__, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef DEBUG_
#define DEBUG_(format, ...)   do { \
  if (yamiLogFlag >= YAMI_LOG_DEBUG) \
    yamiMessage(yamiLogFn, format, ##__VA_ARGS__);\
}while (0)
#endif

#ifndef ASSERT
#define ASSERT(expr) assert(expr)
#endif

#else                           //__ENABLE_DEBUG__
#ifndef INFO
#define INFO(format, ...)
#endif

#ifndef WARNING
#define WARNING(format, ...)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)
#endif
#ifndef DEBUG_
#define DEBUG_(format, ...)
#endif
#endif                          //__ENABLE_DEBUG__

#ifndef ASSERT
#define ASSERT(...)
#endif

#endif                          //__ANDROID
void yamiTraceInit();

#endif                          //__LOG_H__
