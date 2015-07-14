/*
 *  encodeinput.cpp - encode test input
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
#include <ctype.h>
#include "common/log.h"
#include "common/utils.h"

#include "encodeinput.h"
#include "encodeInputDecoder.h"

using namespace YamiMediaCodec;

#define MAX_WIDTH  8192
#define MAX_HEIGHT 4320
EncodeInput * EncodeInput::create(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    EncodeInput *input = NULL;
    if (!inputFileName)
        return NULL;

    DecodeInput* decodeInput = DecodeInput::create(inputFileName);
    if (decodeInput) {
        input = new EncodeInputDecoder(decodeInput);
    } else if (!strncmp(inputFileName, "/dev/video", strlen("/dev/video"))) {
        input = new EncodeInputCamera;
    } else {
        input =  new EncodeInputFile;
    }

    if (!input)
        return NULL;

    if (!(input->init(inputFileName, fourcc, width, height))) {
        delete input;
        return NULL;
    }

    return input;
}

EncodeInputFile::EncodeInputFile()
    : m_fp(NULL)
    , m_buffer(NULL)
    , m_readToEOS(false)
{
}

bool EncodeInputFile::init(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    if (!width || !height) {
        if (!guessResolution(inputFileName, width, height)) {
            fprintf(stderr, "failed to guess input width and height\n");
            return false;
        }
    }
    m_width = width;
    m_height = height;
    m_fourcc = fourcc;

    if ((m_width <= 0) ||(m_height <= 0) ||(m_width > MAX_WIDTH) || (m_height > MAX_HEIGHT)) {
        fprintf(stderr, "input width and height is invalid\n");
        return false;
    }

    if (!m_fourcc)
        m_fourcc = guessFourcc(inputFileName);

    if (!m_fourcc)
        m_fourcc = VA_FOURCC('I', '4', '2', '0');

    switch (m_fourcc) {
    case VA_FOURCC_NV12:
    case VA_FOURCC('I', '4', '2', '0'):
    case VA_FOURCC_YV12:
        m_frameSize = m_width * m_height * 3 / 2;
    break;
    case VA_FOURCC_YUY2:
        m_frameSize = m_width * m_height * 2;
    break;
    default:
        ASSERT(0);
    break;
    }

    m_fp = fopen(inputFileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s", inputFileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(m_frameSize));
    return true;
}

bool EncodeInputFile::getOneFrameInput(VideoFrameRawData &inputBuffer)
{
    if (m_readToEOS)
        return false;

    uint8_t *buffer = m_buffer;
    if (inputBuffer.handle)
        buffer = reinterpret_cast<uint8_t*>(inputBuffer.handle);

    int ret = fread(buffer, sizeof(uint8_t), m_frameSize, m_fp);

    if (ret <= 0) {
        m_readToEOS = true;
        return false;
    }

    if (ret < m_frameSize) {
        fprintf (stderr, "data is not enough to read, maybe resolution is wrong\n");
        return false;
    }

    return fillFrameRawData(&inputBuffer, m_fourcc, m_width, m_height, buffer);
}

EncodeInputFile::~EncodeInputFile()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

EncodeOutput::EncodeOutput():m_fp(NULL)
{
}

EncodeOutput::~EncodeOutput()
{
    if (m_fp)
        fclose(m_fp);
}

EncodeOutput* EncodeOutput::create(const char* outputFileName, int width , int height)
{
    EncodeOutput * output = NULL;
    if(outputFileName==NULL)
        return NULL;
    const char *ext = strrchr(outputFileName,'.');
    if(ext==NULL)
        return NULL;
    ext++;//h264;264;jsv;avc;26l;jvt;ivf
    if(strcasecmp(ext,"h264")==0 ||
        strcasecmp(ext,"264")==0 ||
        strcasecmp(ext,"jsv")==0 ||
        strcasecmp(ext,"avc")==0 ||
        strcasecmp(ext,"26l")==0 ||
        strcasecmp(ext,"jvt")==0 ) {
            output = new EncodeOutputH264();
    }
    else if((strcasecmp(ext,"ivf")==0) ||
            (strcasecmp(ext,"vp8")==0)) {
            output = new EncodeOutputVP8();
    }
    else if((strcasecmp(ext,"jpg")==0) ||
               (strcasecmp(ext,"jpeg")==0)) {
               output = new EncodeStreamOutputJpeg();
   }
    else if((strcasecmp(ext,"h265")==0) ||
               (strcasecmp(ext,"265")==0) ||
               (strcasecmp(ext,"hevc")==0)) {
               output = new EncodeOutputHEVC();
    }
    else
        return NULL;

    if(!output->init(outputFileName, width, height)) {
        delete output;
        return NULL;
    }
    return output;
}

bool EncodeOutput::init(const char* outputFileName, int width , int height)
{
    m_fp = fopen(outputFileName, "w+");
    if (!m_fp) {
        fprintf(stderr, "fail to open output file: %s\n", outputFileName);
        return false;
    }
    return true;
}

bool EncodeOutput::write(void* data, int size)
{
    return fwrite(data, 1, size, m_fp) == size;
}

const char* EncodeOutputH264::getMimeType()
{
    return YAMI_MIME_H264;
}

const char* EncodeStreamOutputJpeg::getMimeType()
{
    return "image/jpeg";
}

const char* EncodeOutputVP8::getMimeType()
{
    return YAMI_MIME_VP8;
}

const char* EncodeOutputHEVC::getMimeType()
{
    return YAMI_MIME_HEVC;
}

void setUint32(uint8_t* header, uint32_t value)
{
    uint32_t* h = (uint32_t*)header;
    *h = value;
}

void setUint16(uint8_t* header, uint16_t value)
{
    uint16_t* h = (uint16_t*)header;
    *h = value;
}

static void get_ivf_file_header(uint8_t *header, int width, int height,int count)
{
    setUint32(header, YAMI_FOURCC('D','K','I','F'));
    setUint16(header+4, 0);                     /* version */
    setUint16(header+6,  32);                   /* headersize */
    setUint32(header+8,  YAMI_FOURCC('V','P','8','0'));    /* headersize */
    setUint16(header+12, width);                /* width */
    setUint16(header+14, height);               /* height */
    setUint32(header+16, 30);                    /* rate */
    setUint32(header+20, 1);                    /* scale */
    setUint32(header+24, count);                /* length */
    setUint32(header+28, 0);                    /* unused */
}

bool EncodeOutputVP8::init(const char* outputFileName, int width , int height)
{
    if (!EncodeOutput::init(outputFileName, width, height))
        return false;
    uint8_t header[32];
    get_ivf_file_header(header, width, height, m_frameCount);
    return EncodeOutput::write(header, sizeof(header));
}

bool EncodeOutputVP8::write(void* data, int size)
{
    uint8_t header[12];
    memset(header, 0, sizeof(header));
    setUint32(header, size);
    if (!EncodeOutput::write(&header, sizeof(header)))
        return false;
    if (!EncodeOutput::write(data, size))
        return false;
    m_frameCount++;
    return true;
}
EncodeOutputVP8::EncodeOutputVP8():m_frameCount(0){}

EncodeOutputVP8::~EncodeOutputVP8()
{
    if (m_fp && !fseek(m_fp, 24,SEEK_SET)) {
        EncodeOutput::write(&m_frameCount, sizeof(m_frameCount));
    }
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

#ifdef __BUILD_GET_MV__
bool createMVBuffer(VideoEncMVBuffer* MVBuffer, int Size)
{
    MVBuffer->data = static_cast<uint8_t*>(malloc(Size));
    if (!MVBuffer->data)
        return false;
    MVBuffer->bufferSize = Size;
    return true;
}
#endif
