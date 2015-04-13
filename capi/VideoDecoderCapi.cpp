/*
 *  VideoDecoderCapi.cpp - capi wrapper for decoder
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "VideoDecoderCapi.h"
#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"

using namespace YamiMediaCodec;

DecodeHandler createDecoder(const char *mimeType)
{
    return createVideoDecoder(mimeType);
}

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer)
{
    if(p)
        return ((IVideoDecoder*)p)->start(buffer);
    else
        return DECODE_FAIL;
}

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->reset(buffer);
     else
        return DECODE_FAIL;
}

void decodeStop(DecodeHandler p)
{
     if(p)
        ((IVideoDecoder*)p)->stop();
}

void decodeFlush(DecodeHandler p)
{
     if(p)
        ((IVideoDecoder*)p)->flush();
}

Decode_Status decode(DecodeHandler p, VideoDecodeBuffer *buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->decode(buffer);
     else
        return DECODE_FAIL;
}

const VideoRenderBuffer* decodeGetOutput(DecodeHandler p, bool draining)
{
     if(p)
        return ((IVideoDecoder*)p)->getOutput(draining);
}

#ifdef __ENABLE_X11__
Decode_Status decodeGetOutput_x11(DecodeHandler p, Drawable draw, int64_t *timeStamp
        , int drawX, int drawY, int drawWidth, int drawHeight, bool draining
        , int frameX, int frameY, int frameWidth, int frameHeight, unsigned int flags)
{
    if(p)
        return ((IVideoDecoder*)p)->getOutput(draw, timeStamp, drawX, drawY, drawWidth, drawHeight, draining
                        , frameX, frameY, frameWidth, frameHeight, flags);
    else
        return DECODE_FAIL;
}
#endif

Decode_Status decodeGetOutputRawData(DecodeHandler p, VideoFrameRawData* frame, bool draining)
{
     if(p)
        return ((IVideoDecoder*)p)->getOutput(frame, draining);
}

const VideoFormatInfo* getFormatInfo(DecodeHandler p)
{
    if(p)
        return ((IVideoDecoder*)p)->getFormatInfo();
}

void renderDone(DecodeHandler p, VideoRenderBuffer* buffer)
{
    if(p)
        ((IVideoDecoder*)p)->renderDone(buffer);
}

void renderDoneRawData(DecodeHandler p, VideoFrameRawData* buffer)
{
    if(p)
        ((IVideoDecoder*)p)->renderDone(buffer);
}

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay * display)
{
    if(p)
        ((IVideoDecoder*)p)->setNativeDisplay(display);
}

void flushOutport(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->flushOutport();
}

void enableNativeBuffers(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->enableNativeBuffers();
}

Decode_Status getClientNativeWindowBuffer(DecodeHandler p, void *bufferHeader, void *nativeBufferHandle)
{
    if(p)
        return ((IVideoDecoder*)p)->getClientNativeWindowBuffer(bufferHeader, nativeBufferHandle);
    else
        return DECODE_FAIL;
}

Decode_Status flagNativeBuffer(DecodeHandler p, void * pBuffer)
{
    if(p)
        return ((IVideoDecoder*)p)->flagNativeBuffer(pBuffer);
    else
        return DECODE_FAIL;
}

void releaseLock(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->releaseLock();
}

void releaseDecoder(DecodeHandler p)
{
    if(p)
        releaseVideoDecoder((IVideoDecoder*)p);
}
