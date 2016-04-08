/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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

#include "VideoDecoderDefs.h"
#include "capi/VideoDecoderCapi.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DecodeOutputTag
{
};

typedef struct DecodeOutputTag* DecodeOutputHandler;



DecodeOutputHandler createDecodeOutput(DecodeHandler decoder, int renderMode);
bool configDecodeOutput(DecodeOutputHandler output);
bool decodeOutputSetVideoSize(DecodeOutputHandler output, int width , int height);
int renderOutputFrames(DecodeOutputHandler output, uint32_t maxframes, bool drain);
void releaseDecodeOutput(DecodeOutputHandler output);




#ifdef __cplusplus
}
#endif
