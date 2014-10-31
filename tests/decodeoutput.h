/*
 *  decodeoutput.h - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#ifndef decodeoutput_h
#define decodeoutput_h

#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"

#include <stdio.h>
#include <vector>
#include <tr1/memory>

#include <sstream>

using namespace YamiMediaCodec;

class DecodeStreamOutput
{
public:
    static DecodeStreamOutput* create(IVideoDecoder* decoder, int mode);
    virtual bool setVideoSize(int width, int height);
    virtual Decode_Status renderOneFrame(bool drain) = 0;
    virtual ~DecodeStreamOutput() {};

protected:
    DecodeStreamOutput(IVideoDecoder* decoder);
    int m_width;
    int m_height;
    IVideoDecoder* m_decoder;
};

class DecodeStreamOutputNull: public DecodeStreamOutput
{
friend DecodeStreamOutput* DecodeStreamOutput::create(IVideoDecoder* decoder, int mode);
public:
    virtual Decode_Status renderOneFrame(bool drain);
    ~DecodeStreamOutputNull() {};

private:
    DecodeStreamOutputNull(IVideoDecoder* decoder):DecodeStreamOutput(decoder) {};
};

class ColorConvert;
class DecodeStreamOutputRaw : public DecodeStreamOutput
{
friend DecodeStreamOutput* DecodeStreamOutput::create(IVideoDecoder* decoder, int mode);
public:
    virtual Decode_Status renderOneFrame(bool drain);
    ~DecodeStreamOutputRaw();

protected:
    virtual bool render(VideoFrameRawData* frame) = 0;
    void setFourcc(uint32_t fourcc);
    uint32_t getFourcc();
    DecodeStreamOutputRaw(IVideoDecoder* decoder);

private:
    ColorConvert* m_convert;
    uint32_t m_srcFourcc;
    uint32_t m_destFourcc;
    bool m_enableSoftI420Convert;
};

class DecodeStreamOutputFileDump : public DecodeStreamOutputRaw
{
friend DecodeStreamOutput* DecodeStreamOutput::create(IVideoDecoder* decoder, int mode);
public:
    bool config(const char* dir, const char* source, const char* dest, uint32_t fourcc);
    virtual bool setVideoSize(int width, int height);
    ~DecodeStreamOutputFileDump();

protected:
    DecodeStreamOutputFileDump(IVideoDecoder* decoder);
    virtual bool render(VideoFrameRawData* frame);

private:
    std::ostringstream m_name;
    bool m_appendSize;
    FILE* m_fp;

};


//help functions
bool renderOutputFrames(DecodeStreamOutput* output, bool drain = false);
bool configDecodeOutput(DecodeStreamOutput* output);

#endif //decodeoutput_h
