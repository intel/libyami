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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "VideoEncoderDefs.h"
#include "common/common_def.h"
#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EncodeHandler;

EncodeHandler createEncoder(const char *mimeType);

void encodeSetNativeDisplay(EncodeHandler p, NativeDisplay * display);

Encode_Status encodeStart(EncodeHandler p);

void encodeStop(EncodeHandler p);

void encodeflush(EncodeHandler p);

Encode_Status encode(EncodeHandler p, VideoEncRawBuffer * inBuffer);

Encode_Status encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer * outBuffer, bool withWait);

Encode_Status getParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status setParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

Encode_Status getMaxOutSize(EncodeHandler p, uint32_t * maxSize);

Encode_Status getStatistics(EncodeHandler p, VideoStatistics * videoStat);

Encode_Status getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

Encode_Status setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

void releaseEncoder(EncodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
