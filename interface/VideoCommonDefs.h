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
#include <stdbool.h>

#ifdef __cplusplus

#if __cplusplus > 199711L
    #include <memory>
#else
    #include <tr1/memory>
    namespace std {
        using std::tr1::shared_ptr;
        using std::tr1::weak_ptr;
        using std::tr1::enable_shared_from_this;
        using std::tr1::dynamic_pointer_cast;
        using std::tr1::static_pointer_cast;
    }
#endif // __cplusplus > 199711L

#define SharedPtr std::shared_ptr
#define WeakPtr std::weak_ptr
#define DynamicPointerCast std::dynamic_pointer_cast
#define StaticPointerCast std::static_pointer_cast
#define EnableSharedFromThis std::enable_shared_from_this

#endif // __cplusplus

#ifdef __cplusplus
extern "C" {
#endif

#define YAMI_FOURCC(ch0, ch1, ch2, ch3) \
        ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
         ((uint32_t)(uint8_t)(ch2) << 16)  | ((uint32_t)(uint8_t)(ch3) << 24))
//Y only
#define YAMI_FOURCC_Y800 YAMI_FOURCC('Y', '8', '0', '0')

//411
#define YAMI_FOURCC_411P YAMI_FOURCC('4', '1', '1', 'P')

//420
#define YAMI_FOURCC_NV12 YAMI_FOURCC('N', 'V', '1', '2')
#define YAMI_FOURCC_I420 YAMI_FOURCC('I', '4', '2', '0')
#define YAMI_FOURCC_YV12 YAMI_FOURCC('Y', 'V', '1', '2')
#define YAMI_FOURCC_IMC3 YAMI_FOURCC('I', 'M', 'C', '3')

//422
#define YAMI_FOURCC_422H YAMI_FOURCC('4', '2', '2', 'H')
#define YAMI_FOURCC_422V YAMI_FOURCC('4', '2', '2', 'V')
#define YAMI_FOURCC_YUY2 YAMI_FOURCC('Y', 'U', 'Y', '2')
#define YAMI_FOURCC_UYVY YAMI_FOURCC('U', 'Y', 'V', 'Y')

//444
#define YAMI_FOURCC_444P YAMI_FOURCC('4', '4', '4', 'P')

//10 bits
#define YAMI_FOURCC_P010 YAMI_FOURCC('P', '0', '1', '0')

//RGB
#define YAMI_FOURCC_RGBX YAMI_FOURCC('R', 'G', 'B', 'X')
#define YAMI_FOURCC_RGBA YAMI_FOURCC('R', 'G', 'B', 'A')
#define YAMI_FOURCC_BGRX YAMI_FOURCC('B', 'G', 'R', 'X')
#define YAMI_FOURCC_BGRA YAMI_FOURCC('B', 'G', 'R', 'A')
#define YAMI_FOURCC_XRGB YAMI_FOURCC('X', 'R', 'G', 'B')
#define YAMI_FOURCC_ARGB YAMI_FOURCC('A', 'R', 'G', 'B')
#define YAMI_FOURCC_XBGR YAMI_FOURCC('X', 'B', 'G', 'R')
#define YAMI_FOURCC_ABGR YAMI_FOURCC('A', 'B', 'G', 'R')

#define YAMI_FOURCC_R210 YAMI_FOURCC('R', '2', '1', '0')

#ifndef V4L2_BUF_FLAG_NON_REF
#define V4L2_BUF_FLAG_NON_REF 0x00200000
#endif

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
    VIDEO_DATA_MEMORY_TYPE_ANDROID_BUFFER_HANDLE, // buffer_handle_t for android
    VIDEO_DATA_MEMORY_TYPE_EXTERNAL_DMA_BUF, //external dma buffer, the buffer is allocate by user
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
    YAMI_FATAL_ERROR = -1024,
    /* encoder fatal */

    /* decoder fatal */
    YAMI_DECODE_PARSER_FAIL,

    /* common fatal */
    YAMI_FAIL,
    YAMI_NO_CONFIG,
    YAMI_DRIVER_FAIL,
    /* yami unimplemented */
    YAMI_NOT_IMPLEMENT,
    /* driver or yami unsupported*/
    YAMI_UNSUPPORTED,
    YAMI_INVALID_PARAM,
    YAMI_OUT_MEMORY,

    YAMI_SUCCESS = 0,
    /* recoverable error */
    /* need more input to get output frame*/
    YAMI_MORE_DATA,

    /* decoder no fatal */
    YAMI_DECODE_INVALID_DATA,
    YAMI_DECODE_FORMAT_CHANGE,
    YAMI_DECODE_NO_SURFACE,

    /* encoder no fatal */
    YAMI_ENCODE_BUFFER_TOO_SMALL, // The buffer passed to encode is too small to contain encoded data
    YAMI_ENCODE_BUFFER_NO_MORE, //No more output buffers.
    YAMI_ENCODE_IS_BUSY, // driver is busy, there are too many buffers under encoding in parallel.
    YAMI_ENCODE_NO_REQUEST_DATA,

} YamiStatus;

typedef YamiStatus Decode_Status;
typedef YamiStatus Encode_Status;

/* following code just for compatible, do not use it in new code*/
#define DECODE_INVALID_DATA YAMI_DECODE_INVALID_DATA
#define DECODE_PARSER_FAIL YAMI_DECODE_PARSER_FAIL
#define DECODE_SUCCESS YAMI_SUCCESS
#define DECODE_FORMAT_CHANGE YAMI_DECODE_FORMAT_CHANGE
#define DECODE_FAIL YAMI_FAIL
#define DECODE_NO_SURFACE YAMI_DECODE_NO_SURFACE
#define DECODE_MEMORY_FAIL YAMI_OUT_MEMORY
#define DECODE_NOT_STARTED YAMI_FAIL

#define ENCODE_SUCCESS YAMI_SUCCESS
#define ENCODE_FAIL YAMI_FAIL
#define ENCODE_INVALID_PARAMS YAMI_INVALID_PARAM
#define ENCODE_BUFFER_TOO_SMALL YAMI_ENCODE_BUFFER_TOO_SMALL
#define ENCODE_BUFFER_NO_MORE YAMI_ENCODE_BUFFER_NO_MORE
#define ENCODE_IS_BUSY YAMI_ENCODE_IS_BUSY
#define ENCODE_NO_REQUEST_DATA YAMI_ENCODE_NO_REQUEST_DATA
#define ENCODE_NO_MEMORY YAMI_OUT_MEMORY
/*compatible code end*/

typedef struct _SurfaceAllocParams SurfaceAllocParams;
typedef struct _SurfaceAllocParams {
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

    /**
     * yami will call this when it want new surface to put decode data
     *
     * it's optional
     * @param user SurfaceAllocParams::user
     * @param surface you want decode to
     */
    YamiStatus (*getSurface)(SurfaceAllocParams* thiz, intptr_t* surface);

    /**
     * yami will call this when all surface usage have been done
     *
     * it's optional
     * @param user SurfaceAllocParams::user
     * @param surface need to recycle
     */
    YamiStatus (*putSurface)(SurfaceAllocParams* thiz, intptr_t surface);

    //you can set your private data for get and put surface
    //yami will not touch it
    void* user;

} SurfaceAllocParams;

typedef struct _SurfaceAllocator SurfaceAllocator;
//allocator for surfaces, yami uses this to get
// 1. decoded surface for decoder.
// 2. reconstruct surface for encoder.
typedef struct _SurfaceAllocator
{
    void* user; /* you can put your private data here, yami will not touch it */
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

    /**
     * for frame destory, cpp should not touch here
     */
    intptr_t    user_data;
    void        (*free)(struct VideoFrame* );
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
#define YAMI_VPP_OCL_WIREFRAME "vpp/ocl_wireframe"

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_COMMON_DEFS_H_
