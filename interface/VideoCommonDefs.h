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
#include <tr1/memory>
namespace YamiMediaCodec{
//use tr1's shared_ptr until we implemented ours.
#define SharedPtr std::tr1::shared_ptr
#define EnableSharedFromThis std::tr1::enable_shared_from_this
#define SharedFromThis shared_from_this


#define DISALLOW_COPY_AND_ASSIGN(className) \
      className(const className&); \
      className & operator=(const className&); \

}
#endif

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

#define YAMI_FOURCC(ch0, ch1, ch2, ch3) \
        ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
         ((uint32_t)(uint8_t)(ch2) << 16)  | ((uint32_t)(uint8_t)(ch3) << 24))

typedef enum {
    NATIVE_DISPLAY_AUTO,    // decided by yami
    NATIVE_DISPLAY_X11,
    NATIVE_DISPLAY_DRM,
    NATIVE_DISPLAY_WAYLAND,
    NATIVE_DISPLAY_VA,      /* client need init va*/
} YamiNativeDisplayType;

typedef struct NativeDisplay{
    intptr_t handle;
    YamiNativeDisplayType type;
} NativeDisplay;

typedef enum {
    VIDEO_DATA_MEMORY_TYPE_RAW_POINTER,  // pass data pointer to client
    VIDEO_DATA_MEMORY_TYPE_RAW_COPY,     // copy data to client provided buffer
    VIDEO_DATA_MEMORY_TYPE_DRM_NAME,
    VIDEO_DATA_MEMORY_TYPE_DMA_BUF,
    VIDEO_DATA_MEMORY_TYPE_SURFACE_ID,  // it can be used for surface sharing of transcoding, benefits suppressed rendering as well.
                                        //it is discouraged to use it for video rendering.
} VideoDataMemoryType;

typedef struct VideoFrameRawData{
    VideoDataMemoryType memoryType;
    uint32_t width;
    uint32_t height;
    uint32_t pitch[3];
    uint32_t offset[3];
    uint32_t fourcc;
    uint32_t size;
    intptr_t handle;        // planar data has one fd for now, raw data also uses one pointer (+ offset)
    uint32_t internalID; // internal identification for image/surface recycle
    int64_t  timeStamp;
    uint32_t flags;             //see VIDEO_FRAME_FLAGS_XXX
}VideoFrameRawData;

#define VIDEO_FRAME_FLAGS_KEY 1

typedef enum {
    YAMI_SUCCESS = 0,
    /* need more input to get output frame*/
    YAMI_MORE_DATA,
    YAMI_NOT_IMPLEMENT,
    YAMI_INVALID_PARAM,
    YAMI_OUT_MEMORY,

    YAMI_FATAL_ERROR = 128,
    YAMI_FAIL,
    YAMI_NO_CONFIG,
    YAMI_DRIVER_FAIL,
} YamiStatus;

typedef struct VideoRect
{
    int32_t  x;
    int32_t  y;
    uint32_t width;
    uint32_t height;
} VideoRect;

/**
 * slim version of VideoFrameRawData, only useful information here
 * hope we can use this for decode, encode and vpp in final.
 */
typedef struct VideoFrame {
    /**
     * generic id for video surface,
     * for libva it's VASurfaceID
     */
    intptr_t    surface;

    int64_t     timeStamp;

    VideoRect   crop;

    /**
     * VIDEO_FRAME_FLAGS_XXX
     */
    uint32_t    flags;

#ifdef __ENABLE_CAPI__
    /**
     * for frame destory, cpp should not touch here
     */
    intptr_t    user_data;
    void        (*free)(struct VideoFrame* );
#endif
} VideoFrame;

#define YAMI_MIME_H264 "video/h264"
#define YAMI_MIME_AVC  "video/avc"
#define YAMI_MIME_VP8  "video/x-vnd.on2.vp8"
#define YAMI_MIME_VP9  "video/x-vnd.on2.vp9"
#define YAMI_MIME_JPEG "image/jpeg"
#define YAMI_VPP_SCALER "vpp/scaler"

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_COMMON_DEFS_H_
