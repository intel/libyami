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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "VideoEncoderDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* EncodeInputHandler;
typedef void* EncodeOutputHandler;

EncodeInputHandler createEncodeInput(const char * inputFileName, uint32_t fourcc, int width, int height);

EncodeOutputHandler createEncodeOutput(const char * outputFileName, int width, int height);

bool encodeInputIsEOS(EncodeInputHandler input);

const char * getOutputMimeType(EncodeOutputHandler output);

bool initInput(EncodeInputHandler input, const char* inputFileName, uint32_t fourcc, const int width, const int height);

int getInputWidth(EncodeInputHandler input);

int getInputHeight(EncodeInputHandler input);

bool getOneFrameInput(EncodeInputHandler input, VideoFrameRawData *inputBuffer);

bool recycleOneFrameInput(EncodeInputHandler input, VideoFrameRawData *inputBuffer);

bool writeOutput(EncodeOutputHandler output, void* data, int size);

void releaseEncodeInput(EncodeInputHandler input);

void releaseEncodeOutput(EncodeOutputHandler output);

bool createOutputBuffer(VideoEncOutputBuffer* outputBuffer, int maxOutSize);

#ifdef __cplusplus
}
#endif
