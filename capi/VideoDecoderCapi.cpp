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

#include <stdlib.h>

using namespace YamiMediaCodec;

class SharedPtrHold
{
public:
    SharedPtrHold(SharedPtr<VideoFrame> frame):frame(frame){}
private:
    SharedPtr<VideoFrame> frame;
};

static void freeHold(VideoFrame* frame)
{
    delete (SharedPtrHold*)frame->user_data;
}

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

VideoFrame* getOutput(DecodeHandler p)
{
    if (p) {
        SharedPtr<VideoFrame> frame = ((IVideoDecoder*)p)->getOutput();
        if (frame) {
            SharedPtrHold *hold= new SharedPtrHold(frame);
            frame->user_data = (intptr_t)hold;
            frame->free = freeHold;
            return frame.get();
        }
    }
    return NULL;
}

const VideoFormatInfo* getFormatInfo(DecodeHandler p)
{
    return (p ? ((IVideoDecoder*)p)->getFormatInfo() : NULL);
}

void renderDone(DecodeHandler p, const VideoRenderBuffer* buffer)
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
