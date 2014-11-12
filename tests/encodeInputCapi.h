/*
 *  encodeInputCapi.h - encode test input capi
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
