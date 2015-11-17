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

#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"
#include "VideoDecoderCapi.h"

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

void setNativeDisplay(DecodeHandler handler, NativeDisplay * display)
{
    if(handler)
        ((IVideoDecoder*)handler)->setNativeDisplay(display);
}

Decode_Status decodeStart(DecodeHandler handler, VideoConfigBuffer* configBuffer)
{
    if(handler)
        return ((IVideoDecoder*)handler)->start(configBuffer);
    else
        return DECODE_FAIL;
}

Decode_Status decode(DecodeHandler handler, VideoDecodeBuffer* buffer)
{
    if(handler) {
        Decode_Status status = ((IVideoDecoder*)handler)->decode(buffer);
        if (DECODE_FORMAT_CHANGE == status)
            return ((IVideoDecoder*)handler)->decode(buffer);
        return status;
    }
     else
        return DECODE_FAIL;
}

VideoFrame* getOutput(DecodeHandler handler)
{
    if (handler) {
        SharedPtr<VideoFrame> frame = ((IVideoDecoder*)handler)->getOutput();
        if (frame) {
            SharedPtrHold *hold= new SharedPtrHold(frame);
            frame->user_data = (intptr_t)hold;
            frame->free = freeHold;
            return frame.get();
        }
    }
    return NULL;
}

void renderDone(VideoFrame* frame)
{
    frame->free(frame);
}

void decodeStop(DecodeHandler handler)
{
    if (handler)
        ((IVideoDecoder*)handler)->stop();
}

void releaseDecoder(DecodeHandler handler)
{
    if (handler)
        releaseVideoDecoder((IVideoDecoder*)handler);
}