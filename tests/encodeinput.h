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

#include "VideoEncoderDefs.h"
#include "VideoEncoderInterface.h"

using namespace YamiMediaCodec;

class EncodeStreamInput {
public:
    EncodeStreamInput();
    ~EncodeStreamInput();
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

class EncodeStreamOutput {
public:
    EncodeStreamOutput();
    ~EncodeStreamOutput();
    static  EncodeStreamOutput* create(const char* outputFileName, int width , int height);
    virtual bool write(void* data, int size);
    virtual const char* getMimeType() = 0;
protected:
    virtual bool init(const char* outputFileName, int width , int height);
    FILE *m_fp;
};

class EncodeStreamOutputH264 : public EncodeStreamOutput
{
    virtual const char* getMimeType();
};

class EncodeStreamOutputVP8 : public EncodeStreamOutput {
public:
    EncodeStreamOutputVP8();
    ~EncodeStreamOutputVP8();
    virtual const char* getMimeType();
    virtual bool write(void* data, int size);
protected:
    virtual bool init(const char* outputFileName, int width , int height);
private:
    int m_frameCount;
};

bool createOutputBuffer(VideoEncOutputBuffer* outputBuffer, int maxOutSize);
