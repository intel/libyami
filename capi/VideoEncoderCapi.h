/*
 *  VideoEncoderCapi.h - capi wrapper for encoder
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

#ifndef __VIDEO_ENCODER_CAPI_H__
#define __VIDEO_ENCODER_CAPI_H__

#ifndef __ENABLE_CAPI__
#define __ENABLE_CAPI__ 1
#endif
#include "VideoEncoderDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EncodeHandler;

EncodeHandler createEncoder(const char *mimeType);

void encodeSetNativeDisplay(EncodeHandler p, NativeDisplay * display);

Encode_Status encodeStart(EncodeHandler p);

void encodeStop(EncodeHandler p);

/* encoder will can frame->free, no matter it return fail or not*/
Encode_Status encodeEncode(EncodeHandler p, VideoFrame* frame);

Encode_Status encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer * outBuffer, bool withWait);

Encode_Status encodeGetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status encodeSetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status encodeGetMaxOutSize(EncodeHandler p, uint32_t* maxSize);

void releaseEncoder(EncodeHandler p);

/*deprecated*/
Encode_Status encodeEncodeRawData(EncodeHandler p, VideoFrameRawData* inBuffer);

/*deprecated*/
Encode_Status getStatistics(EncodeHandler p, VideoStatistics * videoStat);

/*deprecated*/
Encode_Status getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

/*deprecated*/
Encode_Status setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

#ifdef __cplusplus
};
#endif

#endif
