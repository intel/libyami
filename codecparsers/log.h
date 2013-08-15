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
#define LOG_ERROR(...)   fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...)    fprintf(stderr, __VA_ARGS__)
#define LOG_WARNING(...) fprintf(stderr, __VA_ARGS__)
#define LOG_DEBUG(...)   fprintf(stderr, __VA_ARGS__)
#endif

#define RETURN_IF_FAIL(condition) \
    if (!(condition)) \
      return;

#define RETURN_VAL_IF_FAIL(condition, value) \
    if (!(condition)) \
      return (value);

#endif
