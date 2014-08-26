/*
 *  VideoDecoderDefs.h - basic va decoder for video
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

#ifndef VIDEO_DECODER_DEFS_H_
#define VIDEO_DECODER_DEFS_H_

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
    int32_t width;
    int32_t height;
    int32_t pitch[3];
    int32_t offset[3];
    uint32_t fourcc;            //NV12
    int32_t size;
    uint8_t *data;
    // own data or derived from surface. If true, the library will release the memory during clearnup
    bool own;
}VideoFrameRawData;

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

} VIDEO_BUFFER_FLAG;

typedef struct {
    uint8_t *data;
    int32_t size;
    int64_t timeStamp;
    uint32_t flag;
    VideoExtensionBuffer *ext;
}VideoDecodeBuffer;


#define MAX_GRAPHIC_BUFFER_NUM  (16 + 1 + 11)   // max DPB + 1 + AVC_EXTRA_NUM

typedef struct {
    uint8_t *data;
    int32_t size;
    int32_t width;
    int32_t height;
    int32_t frameRate;
    int32_t surfaceNumber;
    VAProfile profile;
    uint32_t flag;
    void *graphicBufferHandler[MAX_GRAPHIC_BUFFER_NUM];
    uint32_t graphicBufferStride;
    uint32_t graphicBufferColorFormat;
    uint32_t graphicBufferWidth;
    uint32_t graphicBufferHeight;
    VideoExtensionBuffer *ext;
    void *nativeWindow;
    uint32_t rotationDegrees;

    void *parser_handle;
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
    int32_t width;
    int32_t height;
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
