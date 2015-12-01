/*
 * Copyright (C) 2013 Intel Coperation.. All rights reserved.
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
extern int isInit;

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
