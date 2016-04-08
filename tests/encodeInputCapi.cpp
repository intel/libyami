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

#include <stdio.h>
#include <stdlib.h>
#include "encodeInputCapi.h"
#include "encodeinput.h"

using namespace YamiMediaCodec;

EncodeInputHandler createEncodeInput(const char * inputFileName, uint32_t fourcc, int width, int height)
{
    return EncodeInput::create(inputFileName, fourcc, width, height);
}

EncodeOutputHandler createEncodeOutput(const char * outputFileName, int width, int height)
{
    return EncodeOutput::create(outputFileName, width, height);
}

bool encodeInputIsEOS(EncodeInputHandler input)
{
    if(input)
        return ((EncodeInput*)input)->isEOS();
    else
        return false;
}

const char * getOutputMimeType(EncodeOutputHandler output)
{
    if(output)
        return ((EncodeOutput*)output)->getMimeType();
    else
        return NULL;
}

bool initInput(EncodeInputHandler input, const char* inputFileName, uint32_t fourcc, const int width, const int height)
{
    return ((EncodeInput*)input)->init(inputFileName, fourcc, width, height);
}

int getInputWidth(EncodeInputHandler input)
{
    if (input)
        return ((EncodeInput*)input)->getWidth();
    return 0;
}

int getInputHeight(EncodeInputHandler input)
{
    if (input)
        return ((EncodeInput*)input)->getHeight();
    return 0;
}

bool getOneFrameInput(EncodeInputHandler input, VideoFrameRawData *inputBuffer)
{
    if(input)
        return ((EncodeInput*)input)->getOneFrameInput(*inputBuffer);
    else
        return false;
}

bool recycleOneFrameInput(EncodeInputHandler input, VideoFrameRawData *inputBuffer)
{
    if(input)
        return ((EncodeInput*)input)->recycleOneFrameInput(*inputBuffer);
    else
        return false;
}

bool writeOutput(EncodeOutputHandler output, void* data, int size)
{
    if(output)
        return ((EncodeOutput*)output)->write(data, size);
    else
        return false;
}

void releaseEncodeInput(EncodeInputHandler input)
{
    if(input)
        delete ((EncodeInput*)input);
}

void releaseEncodeOutput(EncodeOutputHandler output)
{
    if(output)
        delete ((EncodeOutput*)output);
}

bool createOutputBuffer(VideoEncOutputBuffer* outputBuffer, int maxOutSize)
{
    outputBuffer->data = static_cast<uint8_t*>(malloc(maxOutSize));
    if (!outputBuffer->data)
        return false;
    outputBuffer->bufferSize = maxOutSize;
    outputBuffer->format = OUTPUT_EVERYTHING;
    return true;
}
