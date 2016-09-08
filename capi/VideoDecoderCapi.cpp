/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "VideoDecoderCapi.h"
#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"

#include <stdlib.h>

using namespace YamiMediaCodec;

class SharedPtrHold {
public:
    SharedPtrHold(SharedPtr<VideoFrame> frame)
        : frame(frame)
    {
    }

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

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay* display)
{
    if (p)
        ((IVideoDecoder*)p)->setNativeDisplay(display);
}

YamiStatus decodeStart(DecodeHandler p, VideoConfigBuffer* buffer)
{
    if(p)
        return ((IVideoDecoder*)p)->start(buffer);
    else
        return YAMI_FAIL;
}

YamiStatus decodeReset(DecodeHandler p, VideoConfigBuffer* buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->reset(buffer);
     else
         return YAMI_FAIL;
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

YamiStatus decodeDecode(DecodeHandler p, VideoDecodeBuffer* buffer)
{
     if(p)
        return ((IVideoDecoder*)p)->decode(buffer);
     else
         return YAMI_FAIL;
}

VideoFrame* decodeGetOutput(DecodeHandler p)
{
    if (p) {
        SharedPtr<VideoFrame> frame = ((IVideoDecoder*)p)->getOutput();
        if (frame) {
            SharedPtrHold* hold = new SharedPtrHold(frame);
            frame->user_data = (intptr_t)hold;
            frame->free = freeHold;
            return frame.get();
        }
    }
    return NULL;
}

const VideoFormatInfo* decodeGetFormatInfo(DecodeHandler p)
{
    return (p ? ((IVideoDecoder*)p)->getFormatInfo() : NULL);
}

void releaseDecoder(DecodeHandler p)
{
    if (p)
        releaseVideoDecoder((IVideoDecoder*)p);
}

void releaseLock(DecodeHandler p)
{
    if(p)
        ((IVideoDecoder*)p)->releaseLock();
}

