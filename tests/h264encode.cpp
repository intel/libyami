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

#include "common/log.h"
#include "VideoEncoderDef.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"
#include "encodehelp.h"

using namespace YamiMediaCodec;

class StreamInput {
public:
    StreamInput();
    ~StreamInput();
    bool init(const char* inputFileName, const int width, const int height);
    bool getOneFrameInput(VideoEncRawBuffer &inputBuffer);
    bool isEOS() {return m_readToEOS;};

private:
    FILE *m_fp;
    int m_width;
    int m_height;
    int m_frameSize;
    uint8_t *m_buffer;

    bool m_readToEOS;
};

StreamInput::StreamInput()
    : m_fp(NULL)
    , m_width(0)
    , m_height(0)
    , m_frameSize(0)
    , m_buffer(NULL)
    , m_readToEOS(false)
{
}

bool StreamInput::init(const char* inputFileName, const int width, const int height)
{
    m_width = width;
    m_height = height;
    m_frameSize = m_width * m_height * 3 / 2;

    m_fp = fopen(inputFileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s", inputFileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(m_frameSize));
    return true;
}

bool StreamInput::getOneFrameInput(VideoEncRawBuffer &inputBuffer)
{
    if (m_readToEOS)
        return false;

    int ret = fread(m_buffer, sizeof(uint8_t), m_frameSize, m_fp);

    if (ret <= 0) {
        m_readToEOS = true;
        return false;
    } else if (ret < m_frameSize) {
        fprintf (stderr, "data is not enough to read, maybe resolution is wrong\n");
        return false;
    } else {
        inputBuffer.data = m_buffer;
        inputBuffer.size = m_frameSize;
    }

    return true;
}

StreamInput::~StreamInput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

class StreamOutput {
public:
    StreamOutput();
    ~StreamOutput();
    bool init(const char* outputFileName, const uint32_t maxOutSize);
    bool writeOneOutputFrame();
    VideoEncOutputBuffer outputBuffer;

private:
    FILE *m_fp;
    uint8_t *m_buffer;
};

StreamOutput::StreamOutput()
    : m_fp(NULL)
    , m_buffer(NULL)
{
}

bool StreamOutput::init(const char* outputFileName, const uint32_t maxOutSize)
{
    m_fp = fopen(outputFileName, "w+");
    if (!m_fp) {
        fprintf(stderr, "fail to open output file: %s\n", outputFileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(maxOutSize));
    outputBuffer.bufferSize = maxOutSize;
    outputBuffer.data = m_buffer;
    outputBuffer.format = OUTPUT_EVERYTHING;

    return true;
}

bool StreamOutput::writeOneOutputFrame()
{
    if (fwrite(m_buffer, 1, outputBuffer.dataSize, m_fp) != outputBuffer.dataSize) {
        assert(0);
        return false;
    }

    return true;
}

StreamOutput::~StreamOutput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
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
    IVideoEncoder *encoder = NULL;
    uint32_t maxOutSize = 0;
    StreamInput input;
    StreamOutput output;
    Encode_Status status;
    VideoEncRawBuffer inputBuffer;

    if (!process_cmdline(argc, argv))
        return -1;

    if (!input.init(inputFileName, videoWidth, videoHeight)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    encoder = createVideoEncoder("video/h264");

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encoder->getParameters(&encVideoParams);
    setEncoderParameters(&encVideoParams);
    encoder->setParameters(&encVideoParams);
    status = encoder->start();

    //init output buffer
    encoder->getMaxOutSize(&maxOutSize);
    if (!output.init(outputFileName, maxOutSize)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    while (!input.isEOS())
    {
        if (input.getOneFrameInput(inputBuffer))
            status = encoder->encode(&inputBuffer);
        else
            break;

        //get the output buffer
        do {
            status = encoder->getOutput(&output.outputBuffer, false);
            if (status == ENCODE_SUCCESS && !output.writeOneOutputFrame())
                assert(0);
        } while (status != ENCODE_BUFFER_NO_MORE);
    }

    // drain the output buffer
    do {
       status = encoder->getOutput(&output.outputBuffer, true);
       if (status == ENCODE_SUCCESS && !output.writeOneOutputFrame())
           assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);

    encoder->stop();
    releaseVideoEncoder(encoder);

    fprintf(stderr, "encode done\n");
    return 0;
}
