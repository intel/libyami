/*
 *  vppoutputencode.h - take vpp output as encode input
 *
 *  Copyright (C) 2015 Intel Corporation
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
#ifndef vppoutputencode_h
#define vppoutputencode_h
#include "VideoEncoderHost.h"
#include "encodeinput.h"
#include <string>
#include <vector>

#include "vppinputoutput.h"
using std::string;
//#include "yamitranscodehelp.h"

class EncodeParams
{
public:
    EncodeParams();

    VideoRateControl rcMode;
    int32_t initQp;
    int32_t bitRate;
    int32_t fps;
    int32_t ipPeriod;
    int32_t ipbMode;
    int32_t kIPeriod;
    string codec;
};

class TranscodeParams
{
public:
    TranscodeParams();

    EncodeParams m_encParams;
    uint32_t frameCount;
    int32_t oWidth; /*output video width*/
    int32_t oHeight; /*output vide height*/
    uint32_t fourcc;
    string inputFileName;
    string outputFileName;
};

class VppOutputEncode : public VppOutput
{
public:
    virtual bool output(const SharedPtr<VideoFrame>& frame);
    virtual ~VppOutputEncode(){}
    bool config(NativeDisplay& nativeDisplay, const EncodeParams* encParam = NULL);
protected:
    virtual bool init(const char* outputFileName, uint32_t fourcc, int width, int height);
private:
    void initOuputBuffer();
    const char* m_mime;
    SharedPtr<IVideoEncoder> m_encoder;
    VideoEncOutputBuffer m_outputBuffer;
    std::vector<uint8_t> m_buffer;
    SharedPtr<EncodeOutput> m_output;
};

#endif
