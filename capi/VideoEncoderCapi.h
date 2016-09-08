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

#include <VideoEncoderDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EncodeHandler;

EncodeHandler createEncoder(const char *mimeType);

void encodeSetNativeDisplay(EncodeHandler p, NativeDisplay * display);

YamiStatus encodeStart(EncodeHandler p);

void encodeStop(EncodeHandler p);

/* encoder will can frame->free, no matter it return fail or not*/
YamiStatus encodeEncode(EncodeHandler p, VideoFrame* frame);

YamiStatus encodeGetOutput(EncodeHandler p, VideoEncOutputBuffer* outBuffer, bool withWait);

YamiStatus encodeGetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

YamiStatus encodeSetParameters(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncParams);

YamiStatus encodeGetMaxOutSize(EncodeHandler p, uint32_t* maxSize);

void releaseEncoder(EncodeHandler p);

/*deprecated*/
YamiStatus encodeEncodeRawData(EncodeHandler p, VideoFrameRawData* inBuffer);

/*deprecated*/
YamiStatus getStatistics(EncodeHandler p, VideoStatistics* videoStat);

/*deprecated*/
YamiStatus getConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

/*deprecated*/
YamiStatus setConfig(EncodeHandler p, VideoParamConfigType type, Yami_PTR videoEncConfig);

#ifdef __cplusplus
};
#endif

#endif
