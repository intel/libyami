/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    int32_t intraPeriod;
    int32_t numRefFrames;
    int32_t idrInterval;
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
