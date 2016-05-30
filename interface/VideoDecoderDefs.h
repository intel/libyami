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
#include "VideoCommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif
// format specific data, for future extension.
typedef struct {
    int32_t extType;
    int32_t extSize;
    uint8_t *extData;
}VideoExtensionBuffer;

typedef enum {
    PACKED_FRAME_TYPE,
} VIDEO_EXTENSION_TYPE;

typedef struct {
    int64_t timestamp;
    int32_t offSet;
}PackedFrameData;

// flags for VideoDecodeBuffer, VideoConfigBuffer and VideoRenderBuffer
typedef enum {
    // indicates if sample has discontinuity in time stamp (happen after seeking usually)
    HAS_DISCONTINUITY = 0x01,

    // indicates wheter the sample contains a complete frame or end of frame.
    HAS_COMPLETE_FRAME = 0x02,

    // indicate whether surfaceNumber field  in the VideoConfigBuffer is valid
    HAS_SURFACE_NUMBER = 0x04,

    // indicate whether profile field in the VideoConfigBuffer is valid
    HAS_VA_PROFILE = 0x08,

    // indicate whether output order will be the same as decoder order
    WANT_LOW_DELAY = 0x10,      // make display order same as decoding order

    // indicates whether error concealment algorithm should be enabled to automatically conceal error.
    WANT_ERROR_CONCEALMENT = 0x20,

    // indicate wheter raw data should be output.
    WANT_RAW_OUTPUT = 0x40,

    // indicate sample is decoded but should not be displayed.
    WANT_DECODE_ONLY = 0x80,

    // indicate surfaceNumber field is valid and it contains minimum surface number to allocate.
    HAS_MINIMUM_SURFACE_NUMBER = 0x100,

    // indicates surface created will be protected
    WANT_SURFACE_PROTECTION = 0x400,

    // indicates if extra data is appended at end of buffer
    HAS_EXTRADATA = 0x800,

    // indicates if buffer contains codec data
    HAS_CODECDATA = 0x1000,

    // indicate if it use graphic buffer.
    USE_NATIVE_GRAPHIC_BUFFER = 0x2000,

    // indicate whether it is a sync frame in container
    IS_SYNC_FRAME = 0x4000,

    // indicate whether video decoder buffer contains secure data
    IS_SECURE_DATA = 0x8000,

    // indicate the input data is a NAL unit for h264
    IS_NAL_UNIT = IS_SECURE_DATA << 1, // 0x10000

    // the input data is in avcC format (not byte stream)  for h264
    IS_AVCC = IS_NAL_UNIT << 1, // 0x20000

} VIDEO_BUFFER_FLAG;

typedef struct {
    uint8_t *data;
    size_t size;
    int64_t timeStamp;
    uint32_t flag;
    VideoExtensionBuffer *ext;
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
    VAProfile profile;
    uint32_t flag;
}VideoConfigBuffer;

typedef struct {
    VASurfaceID surface;
    VADisplay display;
    int64_t timeStamp;          // presentation time stamp
}VideoRenderBuffer;

typedef struct SurfaceBuffer{
    VideoRenderBuffer renderBuffer;
    int32_t pictureOrder;       // picture order count, valid only for AVC format
    bool referenceFrame;        // indicated whether frame associated with this surface is a reference I/P frame
    bool asReferernce;          // indicated wheter frame is used as reference (as a result surface can not be used for decoding)
    VideoFrameRawData *mappedData;
    struct SurfaceBuffer *next;
    uint32_t status;
}VideoSurfaceBuffer;

typedef struct {
    bool valid;                 // indicates whether format info is valid. MimeType is always valid.
    char *mimeType;
    uint32_t width;
    uint32_t height;
    int32_t surfaceWidth;
    int32_t surfaceHeight;
    int32_t surfaceNumber;
    VASurfaceID *ctxSurfaces;
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
    VideoExtensionBuffer *ext;
}VideoFormatInfo;

// TODO: categorize the follow errors as fatal and non-fatal.
typedef enum {
    RENDER_INVALID_PARAMETER = -21,
    RENDER_FAIL = -20,
    DECODE_NOT_STARTED = -10,
    DECODE_NEED_RESTART = -9,
    DECODE_NO_CONFIG = -8,
    DECODE_NO_SURFACE = -7,
    DECODE_NO_REFERENCE = -6,
    DECODE_NO_PARSER = -5,
    DECODE_INVALID_DATA = -4,
    DECODE_DRIVER_FAIL = -3,
    DECODE_PARSER_FAIL = -2,
    DECODE_MEMORY_FAIL = -1,
    DECODE_FAIL = 0,
    DECODE_SUCCESS = 1,
    DECODE_FORMAT_CHANGE = 2,
    DECODE_FRAME_DROPPED = 3,
    DECODE_MULTIPLE_FRAME = 4,
    RENDER_SUCCESS = 20,
    RENDER_NO_AVAILABLE_FRAME = 21,
} VIDEO_DECODE_STATUS;

typedef int32_t Decode_Status;

#ifndef NULL
#define NULL 0
#endif

inline bool checkFatalDecoderError(Decode_Status status)
{
    if (status == DECODE_NOT_STARTED ||
        status == DECODE_NEED_RESTART ||
        status == DECODE_NO_PARSER ||
        status == DECODE_INVALID_DATA ||
        status == DECODE_MEMORY_FAIL || status == DECODE_FAIL) {
        return true;
    } else {
        return false;
    }
}

#ifdef __cplusplus
}
#endif
#endif                          // VIDEO_DECODER_DEFS_H_
