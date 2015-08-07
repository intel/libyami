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
#include <sstream>

using namespace YamiMediaCodec;

class DecodeOutput
{
public:
    static DecodeOutput* create(IVideoDecoder* decoder, int mode);
    virtual bool setVideoSize(int width, int height);
    virtual Decode_Status renderOneFrame(bool drain) = 0;
    Decode_Status processOneFrame(bool drain);
    uint32_t renderFrameCount() { return m_renderFrames; };

    virtual ~DecodeOutput() {};

protected:
    DecodeOutput(IVideoDecoder* decoder);
    IVideoDecoder* m_decoder;
    int m_width;
    int m_height;
    uint32_t m_renderFrames;
    VideoFrameRawData m_frame;
};

class DecodeOutputNull: public DecodeOutput
{
friend DecodeOutput* DecodeOutput::create(IVideoDecoder* decoder, int mode);
public:
    virtual Decode_Status renderOneFrame(bool drain);
    ~DecodeOutputNull() {};

private:
    DecodeOutputNull(IVideoDecoder* decoder):DecodeOutput(decoder) {};
};

class ColorConvert;
class DecodeOutputRaw : public DecodeOutput
{
friend DecodeOutput* DecodeOutput::create(IVideoDecoder* decoder, int mode);
public:
    virtual Decode_Status renderOneFrame(bool drain);
    ~DecodeOutputRaw();

protected:
    virtual bool render(VideoFrameRawData* frame) = 0;
    void setFourcc(uint32_t fourcc);
    uint32_t getFourcc();
    DecodeOutputRaw(IVideoDecoder* decoder);

private:
    ColorConvert* m_convert;
    uint32_t m_srcFourcc;
    uint32_t m_destFourcc;
    bool m_enableSoftI420Convert;
};

class DecodeOutputFileDump : public DecodeOutputRaw
{
friend DecodeOutput* DecodeOutput::create(IVideoDecoder* decoder, int mode);
public:
    bool config(const char* source, const char* dest, uint32_t fourcc);
    virtual bool setVideoSize(int width, int height);
    ~DecodeOutputFileDump();

protected:
    DecodeOutputFileDump(IVideoDecoder* decoder);
    virtual bool render(VideoFrameRawData* frame);

private:
    std::ostringstream m_name;
    FILE* m_fp;
    bool m_appendSize;

};


//help functions
bool renderOutputFrames(DecodeOutput* output, uint32_t maxframes, bool drain = false);
bool configDecodeOutput(DecodeOutput* output);

#endif //decodeoutput_h
