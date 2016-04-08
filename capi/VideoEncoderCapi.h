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
