/*
 *  h264_encode.cpp - h264 encode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Cong Zhong<congx.zhong@intel.com>
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <X11/Xlib.h>

#include "encodehelp.h"
#include "capi/VideoEncoderCapi.h"

typedef struct StreamInput{
    FILE *m_fp;
    int m_width;
    int m_height;
    int m_frameSize;
    uint8_t *m_buffer;

    bool m_readToEOS;
}StreamInput;

StreamInput input = {NULL, 0, 0, 0, NULL, false};

bool inputinit(const char* inputFileName, const int width, const int height)
{
    input.m_width = width;
    input.m_height = height;
    input.m_frameSize = input.m_width * input.m_height * 3 / 2;

    input.m_fp = fopen(inputFileName, "r");
    if (!input.m_fp) {
        fprintf(stderr, "fail to open input file: %s", inputFileName);
        return false;
    }

    input.m_buffer = (uint8_t*)(malloc(input.m_frameSize));
    return true;
}

bool getOneFrameInput(VideoEncRawBuffer *inputBuffer)
{
    if (input.m_readToEOS)
        return false;

    int ret = fread(input.m_buffer, sizeof(uint8_t), input.m_frameSize, input.m_fp);

    if (ret <= 0) {
        input.m_readToEOS = true;
        return false;
    } else if (ret < input.m_frameSize) {
        fprintf (stderr, "data is not enough to read, maybe resolution is wrong\n");
        return false;
    } else {
        inputBuffer->data = input.m_buffer;
        inputBuffer->size = input.m_frameSize;
    }

    return true;
}

typedef struct StreamOutput{
    FILE *m_fp;
    uint8_t *m_buffer;
    VideoEncOutputBuffer outputBuffer;
}StreamOutput;

StreamOutput output = {NULL, NULL, {0, 0, 0, 0, 0, OUTPUT_BUFFER_LAST, 0}};


bool outputinit(const char* outputFileName, const uint32_t maxOutSize)
{
    output.m_fp = fopen(outputFileName, "w+");
    if (!output.m_fp) {
        fprintf(stderr, "fail to open output file: %s\n", outputFileName);
        return false;
    }

    output.m_buffer = (uint8_t*)(malloc(maxOutSize));
    output.outputBuffer.bufferSize = maxOutSize;
    output.outputBuffer.data = output.m_buffer;
    output.outputBuffer.format = OUTPUT_EVERYTHING;

    return true;
}

bool writeOneOutputFrame()
{
    if (fwrite(output.m_buffer, 1, output.outputBuffer.dataSize, output.m_fp) != output.outputBuffer.dataSize) {
        assert(0);
        return false;
    }

    return true;
}

void setEncoderParameters(VideoParamsCommon * encVideoParams)
{
    //resolution
    encVideoParams->resolution.width = videoWidth;
    encVideoParams->resolution.height = videoHeight;

    //frame rate parameters.
    encVideoParams->frameRate.frameRateDenom = 1;
    encVideoParams->frameRate.frameRateNum = fps;

    //picture type and bitrate
    encVideoParams->intraPeriod = kIPeriod;
    encVideoParams->rcMode = RATE_CONTROL_CBR;
    encVideoParams->rcParams.bitRate = bitRate;
    //encVideoParams->rcParams.initQP = 26;
    //encVideoParams->rcParams.minQP = 1;

    encVideoParams->profile = VAProfileH264Main;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;

    encVideoParams->level = 31;
}

int main(int argc, char** argv)
{
    EncodeHandler encoder = NULL;
    uint32_t maxOutSize = 0;
    Encode_Status status;
    VideoEncRawBuffer inputBuffer = {0, 0, false, 0, false};

    if (!process_cmdline(argc, argv))
        return -1;

    if (!inputinit(inputFileName, videoWidth, videoHeight)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }
    encoder = createEncoder("video/h264");

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    getParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    setEncoderParameters(&encVideoParams);
    encVideoParams.size = sizeof(VideoParamsCommon);
    setParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    status = encodeStart(encoder);

    //init output buffer
    getMaxOutSize(encoder, &maxOutSize);

    if (!outputinit(outputFileName, maxOutSize)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    while (!input.m_readToEOS)
    {
        if (getOneFrameInput(&inputBuffer)){
            status = encode(encoder, &inputBuffer);}
        else
            break;

        //get the output buffer
        do {
            status = encodeGetOutput(encoder, &output.outputBuffer, false);
            if (status == ENCODE_SUCCESS && !writeOneOutputFrame())
                assert(0);
        } while (status != ENCODE_BUFFER_NO_MORE);
    }

    // drain the output buffer
    do {
       status = encodeGetOutput(encoder, &output.outputBuffer, true);
       if (status == ENCODE_SUCCESS && !writeOneOutputFrame())
           assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);

    encodeStop(encoder);
    releaseEncoder(encoder);

    fprintf(stderr, "encode done\n");
    return 0;
}
