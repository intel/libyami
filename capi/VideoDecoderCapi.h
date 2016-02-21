/*
 *  VideoDecoderCapi.h - capi wrapper for decoder
 *
 *  Copyright (C) 2014 Intel Corporation
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

#ifndef __VIDEO_DECODER_CAPI_H__
#define __VIDEO_DECODER_CAPI_H__

#ifndef __ENABLE_CAPI__
#define __ENABLE_CAPI__ 1
#endif
#include "VideoDecoderDefs.h"
#include "VideoCommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DecodeHandler;

DecodeHandler createDecoder(const char *mimeType);

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay* display);

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer);

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer);

void decodeStop(DecodeHandler p);

void decodeFlush(DecodeHandler p);

Decode_Status decodeDecode(DecodeHandler p, VideoDecodeBuffer* buffer);

VideoFrame* decodeGetOutput(DecodeHandler p);

const VideoFormatInfo* decodeGetFormatInfo(DecodeHandler p);

void releaseDecoder(DecodeHandler p);

/*deprecated*/
void releaseLock(DecodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
