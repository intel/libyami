/*
 *  encodeInputCapi.cpp - encode test input capi
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xin Tang<xin.t.tang@intel.com>
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
