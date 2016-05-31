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
#include "VideoEncoderCapi.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"

using namespace YamiMediaCodec;

EncodeHandler createEncoder(const char *mimeType)
{
    return createVideoEncoder(mimeType);
}

void encodeSetNativeDisplay(EncodeHandler p, NativeDisplay * display)
{
    if(p)
        ((IVideoEncoder*)p)->setNativeDisplay(display);
}

YamiStatus encodeStart(EncodeHandler p)
{
    if(p)
        return ((IVideoEncoder*)p)->start();
    else
        return YAMI_FAIL;
}

void encodeStop(EncodeHandler p)
{
    if(p)
        ((IVideoEncoder*)p)->stop();
}

static void freeFrame(VideoFrame* frame)
{
    if (frame && frame->free) {
        frame->free(frame);
    }
}

YamiStatus encodeEncode(EncodeHandler p, VideoFrame* frame)
{
    if (!p)
        return YAMI_INVALID_PARAM;
    SharedPtr<VideoFrame> f(frame, freeFrame);
    return ((IVideoEncoder*)p)->encode(f);
}

YamiStatus encodeEncodeRawData(EncodeHandler p, VideoFrameRawData* inBuffer)
{
    if(p)
        return ((IVideoEncoder*)p)->encode(inBuffer);
    else
        return YAMI_FAIL;
}

YamiStatus encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer* outBuffer, bool withWait)
{
    if(p)
        return ((IVideoEncoder*)p)->getOutput(outBuffer, withWait);
    else
        return YAMI_FAIL;
}

YamiStatus encodeGetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams)
{
    if(p)
        return ((IVideoEncoder*)p)->getParameters(type, videoEncParams);
    else
        return YAMI_FAIL;
}

YamiStatus encodeSetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams)
{
    if(p)
        return ((IVideoEncoder*)p)->setParameters(type, videoEncParams);
    else
        return YAMI_FAIL;
}

YamiStatus encodeGetMaxOutSize(EncodeHandler p, uint32_t* maxSize)
{
    if(p)
        return ((IVideoEncoder*)p)->getMaxOutSize(maxSize);
    else
        return YAMI_FAIL;
}

void releaseEncoder(EncodeHandler p)
{
    if (p)
        releaseVideoEncoder((IVideoEncoder*)p);
}

YamiStatus getStatistics(EncodeHandler p, VideoStatistics* videoStat)
{
    if(p)
        return ((IVideoEncoder*)p)->getStatistics(videoStat);
    else
        return YAMI_FAIL;
}

YamiStatus getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    if(p)
        return ((IVideoEncoder*)p)->getConfig(type, videoEncConfig);
    else
        return YAMI_FAIL;
}

YamiStatus setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    if(p)
        return ((IVideoEncoder*)p)->setConfig(type, videoEncConfig);
    else
        return YAMI_FAIL;
}
