/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef VIDEO_COMMON_DEFS_H_
#define VIDEO_COMMON_DEFS_H_
// config.h should NOT be included in header file, especially for the header file used by external

#include <stdint.h>

#ifdef __cplusplus
#ifdef ANDROID
#include <memory>
#else
#include <tr1/memory>
#endif
namespace YamiMediaCodec{
#ifdef ANDROID
    #define SharedPtr std::shared_ptr
    #define WeakPtr std::weak_ptr
    #define DynamicPointerCast std::dynamic_pointer_cast
    #define StaticPointerCast std::static_pointer_cast
    #define EnableSharedFromThis std::enable_shared_from_this
#else
    #define SharedPtr std::tr1::shared_ptr
    #define WeakPtr std::tr1::weak_ptr
    #define DynamicPointerCast std::tr1::dynamic_pointer_cast
    #define StaticPointerCast std::tr1::static_pointer_cast
    #define EnableSharedFromThis std::tr1::enable_shared_from_this
#endif

#define SharedFromThis shared_from_this

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

#define YAMI_FOURCC_NV12 YAMI_FOURCC('N','V','1','2')
#define YAMI_FOURCC_RGBX YAMI_FOURCC('R','G','B','X')
#define YAMI_FOURCC_RGBA YAMI_FOURCC('R','G','B','A')
#define YAMI_FOURCC_BGRX YAMI_FOURCC('B','G','R','X')
#define YAMI_FOURCC_BGRA YAMI_FOURCC('B','G','R','A')

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
    VIDEO_DATA_MEMORY_TYPE_RAW_COPY,     // copy data to client provided buffer, renderDone() is not necessary
    VIDEO_DATA_MEMORY_TYPE_DRM_NAME,     // render output frame by egl/gles, connect with OpenCL
    VIDEO_DATA_MEMORY_TYPE_DMA_BUF,      // share buffer with camera device etc
    VIDEO_DATA_MEMORY_TYPE_SURFACE_ID,  // it can be used for surface sharing of transcoding, benefits suppressed rendering as well.
                                        //it is discouraged to use it for video rendering.
    VIDEO_DATA_MEMORY_TYPE_ANDROID_NATIVE_BUFFER, // ANativeWindowBuffer for android
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

typedef struct {
    //in
    uint32_t fourcc;
    uint32_t width;
    uint32_t height;

    //in, out, number of surfaces
    //for decoder, this min surfaces size need for decoder
    //you should alloc extra size for perfomance or display queue
    //you have two chance to get this size
    // 1. from VideoFormatInfo.surfaceNumber, after you got DECODE_FORMAT_CHANGE
    // 2. in SurfaceAllocator::alloc
    uint32_t size;

    //out
    intptr_t* surfaces;
} SurfaceAllocParams;

typedef struct _SurfaceAllocator SurfaceAllocator;
//allocator for surfaces, yami uses this to get
// 1. decoded surface for decoder.
// 2. reconstruct surface for encoder.
typedef struct _SurfaceAllocator
{
    void*      *user;   /* you can put your private data here, yami will not touch it */
    /* alloc and free surfaces */
    YamiStatus (*alloc) (SurfaceAllocator* thiz, SurfaceAllocParams* params);
    YamiStatus (*free)  (SurfaceAllocator* thiz, SurfaceAllocParams* params);
    /* after this called, yami will not use the allocator */
    void       (*unref) (SurfaceAllocator* thiz);
} SurfaceAllocator;

typedef struct VideoRect
{
    uint32_t  x;
    uint32_t  y;
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

    /* android's csc need this to set VAProcColorStandardType
     * see vaapipostprocess_scaler.cpp for details
     */
    uint32_t    fourcc;

#ifdef __ENABLE_CAPI__
    /**
     * for frame destory, cpp should not touch here
     */
    intptr_t    user_data;
    void        (*free)(struct VideoFrame* );
#endif
} VideoFrame;

#define YAMI_MIME_MPEG2 "video/mpeg2"
#define YAMI_MIME_H264 "video/h264"
#define YAMI_MIME_AVC  "video/avc"
#define YAMI_MIME_H265 "video/h265"
#define YAMI_MIME_HEVC "video/hevc"
#define YAMI_MIME_VP8  "video/x-vnd.on2.vp8"
#define YAMI_MIME_VP9  "video/x-vnd.on2.vp9"
#define YAMI_MIME_JPEG "image/jpeg"
#define YAMI_MIME_VC1 "video/vc1"
#define YAMI_VPP_SCALER "vpp/scaler"
#define YAMI_VPP_OCL_BLENDER "vpp/ocl_blender"
#define YAMI_VPP_OCL_OSD "vpp/ocl_osd"
#define YAMI_VPP_OCL_TRANSFORM "vpp/ocl_transform"
#define YAMI_VPP_OCL_MOSAIC "vpp/ocl_mosaic"

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_COMMON_DEFS_H_
