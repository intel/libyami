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
#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DecodeHandler;

DecodeHandler createDecoder(const char *mimeType);

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer);

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer);

void decodeStop(DecodeHandler p);

void decodeFlush(DecodeHandler p);

Decode_Status decode(DecodeHandler p, VideoDecodeBuffer *buffer);

const VideoRenderBuffer* decodeGetOutput(DecodeHandler p, bool draining);

#ifdef __ENABLE_X11__
Decode_Status decodeGetOutput_x11(DecodeHandler p, Drawable draw, int64_t *timeStamp
        , int drawX, int drawY, int drawWidth, int drawHeight, bool draining
        , int frameX, int frameY, int frameWidth, int frameHeight, unsigned int flags);
#endif

Decode_Status decodeGetOutputRawData(DecodeHandler p, VideoFrameRawData* frame, bool draining);

const VideoFormatInfo* getFormatInfo(DecodeHandler p);

void renderDone(DecodeHandler p, VideoRenderBuffer* buffer);

void renderDoneRawData(DecodeHandler p, VideoFrameRawData* buffer);

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay * display);

void flushOutport(DecodeHandler p);

void enableNativeBuffers(DecodeHandler p);

Decode_Status getClientNativeWindowBuffer(DecodeHandler p, void *bufferHeader, void *nativeBufferHandle);

Decode_Status flagNativeBuffer(DecodeHandler p, void * pBuffer);

void releaseLock(DecodeHandler p);

void releaseDecoder(DecodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
