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

#ifndef __VIDEO_DECODER_CAPI_H__
#define __VIDEO_DECODER_CAPI_H__

#ifndef __ENABLE_CAPI__
#define __ENABLE_CAPI__ 1
#endif
#include "VideoDecoderDefs.h"
#include "VideoCommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* DecodeHandler;

DecodeHandler createDecoder(const char *mimeType);

void decodeSetNativeDisplay(DecodeHandler p, NativeDisplay* display);

Decode_Status decodeStart(DecodeHandler p, VideoConfigBuffer *buffer);

Decode_Status decodeReset(DecodeHandler p, VideoConfigBuffer *buffer);

void decodeStop(DecodeHandler p);

void decodeFlush(DecodeHandler p);

Decode_Status decodeDecode(DecodeHandler p, VideoDecodeBuffer* buffer);

VideoFrame* decodeGetOutput(DecodeHandler p);

const VideoFormatInfo* decodeGetFormatInfo(DecodeHandler p);

void releaseDecoder(DecodeHandler p);

/*deprecated*/
void releaseLock(DecodeHandler p);

#ifdef __cplusplus
};
#endif

#endif
