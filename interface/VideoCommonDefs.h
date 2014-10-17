/*
 *  VideoCommonDefs.h - basic va decoder for video
 *
 *  Copyright (C) 2013 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef VIDEO_COMMON_DEFS_H_
#define VIDEO_COMMON_DEFS_H_
// config.h should NOT be included in header file, especially for the header file used by external

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#ifndef bool
#define bool  int
#endif

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif
#endif

typedef enum {
    NATIVE_DISPLAY_AUTO,    // decided by yami
    NATIVE_DISPLAY_X11,
    NATIVE_DISPLAY_DRM,
    NATIVE_DISPLAY_WAYLAND,
} YamiNativeDisplayType;

typedef struct {
    intptr_t handle;
    YamiNativeDisplayType type;
} NativeDisplay;

typedef enum {
    VIDEO_DATA_MEMORY_TYPE_RAW_POINTER,  // pass data pointer to client
    VIDEO_DATA_MEMORY_TYPE_RAW_COPY,     // copy data to client provided buffer
    VIDEO_DATA_MEMORY_TYPE_DRM_NAME,
    VIDEO_DATA_MEMORY_TYPE_DMA_BUF,
} VideoDataMemoryType;

typedef struct {
    VideoDataMemoryType memoryType;
    uint32_t width;
    uint32_t height;
    uint32_t pitch[3];
    uint32_t offset[3];
    uint32_t fourcc;
    uint32_t size;
    intptr_t handle;        // planar data has one fd for now, raw data also uses one pointer (+ offset)
    uint32_t internalID; // internal identification for image/surface recycle
}VideoFrameRawData;

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_COMMON_DEFS_H_
