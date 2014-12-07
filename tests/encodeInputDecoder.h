/*
 *  encodeInputDecoder.h - use decoder as encoder input
 *
 *  Copyright (C) 2011-2014 Intel Corporation
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
#ifndef encodeInputDecoder_h
#define  encodeInputDecoder_h

#include "decodeinput.h"
#include "encodeinput.h"

#include "VideoDecoderHost.h"

using namespace YamiMediaCodec;

class EncodeInputDecoder : public  EncodeInput {
public:
    EncodeInputDecoder(DecodeInput* input):m_input(input), m_decoder(NULL), m_isEOS(false){}
    ~EncodeInputDecoder();
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
    virtual bool getOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool recycleOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool isEOS();
private:
    bool decodeOneFrame();
    DecodeInput* m_input;
    IVideoDecoder* m_decoder;
    bool m_isEOS;
};
#endif