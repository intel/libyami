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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef ANDROID
#include <utils/Log.h>
#define LOG_ERROR(...)   ALOGE(__VA_ARGS__);
#define LOG_INFO(...)    ALOGI(__VA_ARGS__);
#define LOG_WARNING(...) ALOGV(__VA_ARGS__);
#define LOG_DEBUG(...)   ALOGV(__VA_ARGS__);
#else
#include <stdio.h>
#ifndef LOG_ERROR
#define LOG_ERROR(format, ...)   do { \
   fprintf(stderr, "codecparsers error: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifdef __ENABLE_DEBUG__
#ifndef LOG_INFO
#define LOG_INFO(format, ...)   do { \
   fprintf(stderr, "codecparsers info: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifndef LOG_WARNING
#define LOG_WARNING(format, ...)   do { \
   fprintf(stderr, "codecparsers warning: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(format, ...)   do { \
   fprintf(stderr, "codecparsers debug: "format"\n", ##__VA_ARGS__);\
}while (0)
#endif

#else //__ENABLE_DEBUG__
#ifndef LOG_INFO
#define LOG_INFO(format, ...)
#endif

#ifndef LOG_WARNING
#define LOG_WARNING(format, ...)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(format, ...)
#endif
#endif //__ENABLE_DEBUG__

#endif //__ANDROID

#endif //__LOG_H__
