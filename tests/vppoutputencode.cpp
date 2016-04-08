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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common/common_def.h"
#include "vppoutputencode.h"

EncodeParams::EncodeParams()
    : rcMode(RATE_CONTROL_CQP)
    , initQp(26)
    , bitRate(0)
    , fps(30)
    , ipPeriod(1)
    , intraPeriod(30)
    , numRefFrames(1)
    , idrInterval(0)
    , codec("AVC")
{
    /*nothing to do*/
}

TranscodeParams::TranscodeParams()
    : m_encParams()
    , frameCount(UINT_MAX)
    , oWidth(0)
    , oHeight(0)
    , fourcc(VA_FOURCC_NV12)
{
    /*nothing to do*/
}

bool VppOutputEncode::init(const char* outputFileName, uint32_t /*fourcc*/, int width, int height)
{

    if(!width || !height)
        if (!guessResolution(outputFileName, width, height))
            return false;

    m_fourcc = VA_FOURCC('N', 'V', '1', '2');
    m_width = width;
    m_height = height;
    m_output.reset(EncodeOutput::create(outputFileName, m_width, m_height));
    return m_output;
}

void VppOutputEncode::initOuputBuffer()
{
    uint32_t maxOutSize;
    m_encoder->getMaxOutSize(&maxOutSize);
    m_buffer.resize(maxOutSize);
    m_outputBuffer.bufferSize = maxOutSize;
    m_outputBuffer.format = OUTPUT_EVERYTHING;
    m_outputBuffer.data = &m_buffer[0];

}

static void setEncodeParam(const SharedPtr<IVideoEncoder>& encoder,
                           int width, int height, const EncodeParams* encParam)
{
       //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
    encVideoParams.resolution.width = width;
    encVideoParams.resolution.height = height;

    //frame rate parameters.
    encVideoParams.frameRate.frameRateDenom = 1;
    encVideoParams.frameRate.frameRateNum = encParam->fps;

    //picture type and bitrate
    encVideoParams.intraPeriod = encParam->intraPeriod;
    encVideoParams.ipPeriod = encParam->ipPeriod;
    encVideoParams.rcParams.bitRate = encParam->bitRate;
    encVideoParams.rcParams.initQP = encParam->initQp;
    encVideoParams.rcMode = encParam->rcMode;
    encVideoParams.numRefFrames = encParam->numRefFrames;

    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);

    // configure AVC encoding parameters
    VideoParamsAVC encVideoParamsAVC;
    encVideoParamsAVC.size = sizeof(VideoParamsAVC);
    encoder->getParameters(VideoParamsTypeAVC, &encVideoParamsAVC);
    encVideoParamsAVC.idrInterval = encParam->idrInterval;
    encVideoParamsAVC.size = sizeof(VideoParamsAVC);
    encoder->setParameters(VideoParamsTypeAVC, &encVideoParamsAVC);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);
}

bool VppOutputEncode::config(NativeDisplay& nativeDisplay, const EncodeParams* encParam)
{
    m_encoder.reset(createVideoEncoder(m_output->getMimeType()), releaseVideoEncoder);
    if (!m_encoder)
        return false;
    m_encoder->setNativeDisplay(&nativeDisplay);
    setEncodeParam(m_encoder, m_width, m_height, encParam);

    Encode_Status status = m_encoder->start();
    assert(status == ENCODE_SUCCESS);
    initOuputBuffer();
    return true;
}

bool VppOutputEncode::output(const SharedPtr<VideoFrame>& frame)
{
    Encode_Status status = ENCODE_SUCCESS;
    bool drain = !frame;
    if (frame) {
        status = m_encoder->encode(frame);
        if (status != ENCODE_SUCCESS) {
            fprintf(stderr, "encode failed status = %d\n", status);
            return false;
        }
    }
    do {
        status = m_encoder->getOutput(&m_outputBuffer, drain);
        if (status == ENCODE_SUCCESS
            && !m_output->write(m_outputBuffer.data, m_outputBuffer.dataSize))
             assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);
    return true;

}
