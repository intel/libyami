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

#ifndef VIDEO_DECODER_DEFS_H_
#define VIDEO_DECODER_DEFS_H_
// config.h should NOT be included in header file, especially for the header file used by external

#include <va/va.h>
#include <stdint.h>
#include <VideoCommonDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

// flags for VideoDecodeBuffer, VideoConfigBuffer and VideoRenderBuffer
typedef enum {
    // indicate whether surfaceNumber field  in the VideoConfigBuffer is valid
    HAS_SURFACE_NUMBER = 0x04,

    // indicate whether profile field in the VideoConfigBuffer is valid
    HAS_VA_PROFILE = 0x08,
} VIDEO_BUFFER_FLAG;

typedef struct {
    uint8_t *data;
    size_t size;
    int64_t timeStamp;
    uint32_t flag;
}VideoDecodeBuffer;

typedef struct {
    uint8_t *data;
    int32_t size;
    /// it is the actual frame size, height is 1080 for h264 1080p stream
    uint32_t width;
    uint32_t height;
    /// surfaceWidth and surfaceHeight are the resolution to config output buffer (dirver surface size or client buffer like in sw decode mode)
    /// take h264 1080p as example, it is enlarged to 1088
    int32_t surfaceWidth;
    int32_t surfaceHeight;
    int32_t frameRate;
    int32_t surfaceNumber;
    uint32_t extraSurfaceNum;
    VAProfile profile;
    uint32_t flag;
    uint32_t fourcc;
    //xxxLayer - how many layers to decode; if 0, decode all layers.
    uint32_t temporalLayer;
    uint32_t spacialLayer;
    uint32_t qualityLayer;

    //if set this flag to true, AVC decoder will output the ready frames ASAP.
    bool enableLowLatency;
}VideoConfigBuffer;

typedef struct {
    bool valid;                 // indicates whether format info is valid. MimeType is always valid.
    char *mimeType;
    //width, height, this equals to same value in VideoFrame.crop
    //if your video is 1920x1080, width will be 1920,and height will be 1080
    uint32_t width;
    uint32_t height;

    /* Same thing as SurfaceAllocParams.width and SurfaceAllocParams.height
     * You only need pay attention surfaceXXXX only when you are:
     *  1. using IVideoDecoder.setAllocator.
     * and
     *  2. you want pre-allocate surface before we call SurfaceAllocator.alloc
     * This size is depends on codec type or decoder hardware requirements
     * For h264 1920x1080 it is usually 1920x1088 since h264 need 16 aligned surface
     * Other *important* thing is, we may only call SurfaceAllocator.alloc when surface resolution increased
     * For example, when resolution increase from 640x480 to 1920x1080, we will call
     * SurfaceAllocator.alloc for 1920x1080, but when video change from 1920x1080 to 640x480, we
     * may not call SurfaceAllocator.alloc, since we have enough surface resolution for decoding.
     */
    uint32_t surfaceWidth;
    uint32_t surfaceHeight;
    //same thing as SurfaceAllocParams.size
    uint32_t surfaceNumber;

    //unused thing yet, may remove in near future
    int32_t aspectX;
    int32_t aspectY;
    int32_t cropLeft;
    int32_t cropRight;
    int32_t cropTop;
    int32_t cropBottom;
    int32_t colorMatrix;
    int32_t videoRange;
    int32_t bitrate;
    int32_t framerateNom;
    int32_t framerateDenom;
    //unused end

    uint32_t fourcc;
}VideoFormatInfo;

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_DECODER_DEFS_H_
