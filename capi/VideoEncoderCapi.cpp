/*
 *  VideoEncoderCapi.cpp - capi wrapper for encoder
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

Encode_Status encodeStart(EncodeHandler p)
{
    if(p)
        return ((IVideoEncoder*)p)->start();
    else
        return ENCODE_FAIL;
}

void encodeStop(EncodeHandler p)
{
    if(p)
        ((IVideoEncoder*)p)->stop();
}

Encode_Status encode(EncodeHandler p, VideoFrame * frame)
{
    if(p) {
		SharedPtr<VideoFrame> f;
		f.reset(frame, frame->free);
		return ((IVideoEncoder*)p)->encode(f);
    } else
        return ENCODE_FAIL;
}

Encode_Status encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer * outBuffer, bool withWait)
{
    if(p)
        return ((IVideoEncoder*)p)->getOutput(outBuffer, withWait);
    else
        return ENCODE_FAIL;
}

Encode_Status getParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams)
{
    if(p)
        return ((IVideoEncoder*)p)->getParameters(type, videoEncParams);
    else
        return ENCODE_FAIL;
}

Encode_Status setParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams)
{
    if(p)
        return ((IVideoEncoder*)p)->setParameters(type, videoEncParams);
    else
        return ENCODE_FAIL;
}

Encode_Status getMaxOutSize(EncodeHandler p, uint32_t * maxSize)
{
    if(p)
        return ((IVideoEncoder*)p)->getMaxOutSize(maxSize);
    else
        return ENCODE_FAIL;
}

Encode_Status getStatistics(EncodeHandler p, VideoStatistics * videoStat)
{
    if(p)
        return ((IVideoEncoder*)p)->getStatistics(videoStat);
    else
        return ENCODE_FAIL;
}

Encode_Status getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    if(p)
        return ((IVideoEncoder*)p)->getConfig(type, videoEncConfig);
    else
        return ENCODE_FAIL;
}

Encode_Status setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    if(p)
        return ((IVideoEncoder*)p)->setConfig(type, videoEncConfig);
    else
        return ENCODE_FAIL;
}

void releaseEncoder(EncodeHandler p)
{
    if(p)
        releaseVideoEncoder((IVideoEncoder*)p);
}
