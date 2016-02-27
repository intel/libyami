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

#if (defined(ANDROID) && defined(__USE_LOGCAT__))
#include <utils/Log.h>
#define ERROR(...)
#define INFO(...)
#define WARNING(...)
#define DEBUG(...)

#ifndef PRINT_FOURCC
#define DEBUG_FOURCC(promptStr, fourcc)
#endif

#else

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#define GETTID()    syscall(__NR_gettid)

extern int yamiLogFlag;
extern FILE* yamiLogFn;
extern int isIni;

#ifndef YAMIMESSAGE
#define yamiMessage(stream, format, ...)  do {\
  fprintf(stream, format, ##__VA_ARGS__); \
}while (0)
#endif

#define YAMI_DEBUG_MESSAGE(LEVEL, prefix, format, ...) \
    do {\
        if (yamiLogFlag >= YAMI_LOG_##LEVEL) { \
            const char* name = strrchr(__FILE__, '/'); \
            name = (name ? (name + 1) : __FILE__); \
            yamiMessage(yamiLogFn, "libyami %s %ld (%s, %d): " format "\n", #prefix, (long int)GETTID(), name, __LINE__, ##__VA_ARGS__); \
        } \
    } while (0)

#ifndef ERROR
#define ERROR(format, ...)  YAMI_DEBUG_MESSAGE(ERROR, error, format, ##__VA_ARGS__)
#endif

#ifdef __ENABLE_DEBUG__

#ifndef WARNING
#define WARNING(format, ...)   YAMI_DEBUG_MESSAGE(WARNING, warning, format, ##__VA_ARGS__)
#endif

#ifndef INFO
#define INFO(format, ...)   YAMI_DEBUG_MESSAGE(INFO, info, format, ##__VA_ARGS__)
#endif

#ifndef DEBUG
#define DEBUG(format, ...)   YAMI_DEBUG_MESSAGE(DEBUG, debug, format, ##__VA_ARGS__)
#endif

#ifndef DEBUG_
#define DEBUG_(format, ...)   DEBUG(format, ##__VA_ARGS__)
#endif

#ifndef DEBUG_FOURCC
#define DEBUG_FOURCC(promptStr, fourcc) do { \
  if (yamiLogFlag >= YAMI_LOG_DEBUG) { \
    uint32_t i_fourcc = fourcc; \
    char *ptr = (char*)(&(i_fourcc)); \
    DEBUG("%s, fourcc: 0x%x, %c%c%c%c\n", promptStr, i_fourcc, *(ptr), *(ptr+1), *(ptr+2), *(ptr+3)); \
  } \
} while(0)
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

#ifndef PRINT_FOURCC
#define DEBUG_FOURCC(promptStr, fourcc)
#endif

#endif                          //__ENABLE_DEBUG__
#endif                          //__ANDROID

#ifndef ASSERT
#define ASSERT(expr)               \
    do {                           \
        if (!(expr)) {             \
            ERROR("assert fails"); \
            assert(0 && (expr));   \
        }                          \
    } while (0)
#endif

#define YAMI_LOG_ERROR 0x1
#define YAMI_LOG_WARNING 0x2
#define YAMI_LOG_INFO 0x3
#define YAMI_LOG_DEBUG 0x4

void yamiTraceInit();

#endif                          //__LOG_H__
