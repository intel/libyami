/*
 *  vppoutputencodecapi.cpp - take vpp output as encode input
 *
 *  Copyright (C) 2015 Intel Corporation
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
#include "common/common_def.h"
#include "vppoutputencodecapi.h"
#include "vppoutputencode.h"


VppOutputEncodeCapi::~VppOutputEncodeCapi()
{
    encodeStop(m_encoder);
    releaseEncoder(m_encoder);
}

bool VppOutputEncodeCapi::init(const char* outputFileName, uint32_t fourcc, int width, int height)
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

void VppOutputEncodeCapi::initOuputBuffer()
{
    uint32_t maxOutSize;
    getMaxOutSize(m_encoder, &maxOutSize);
    m_buffer.resize(maxOutSize);
    m_outputBuffer.bufferSize = maxOutSize;
    m_outputBuffer.format = OUTPUT_EVERYTHING;
    m_outputBuffer.data = &m_buffer[0];

}

static void setEncodeParam(const EncodeHandler& encoder,
                           int width, int height, const EncodeParams* encParam)
{
       //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    getParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    encVideoParams.resolution.width = width;
    encVideoParams.resolution.height = height;

    //frame rate parameters.
    encVideoParams.frameRate.frameRateDenom = 1;
    encVideoParams.frameRate.frameRateNum = encParam->fps;

    //picture type and bitrate
    encVideoParams.intraPeriod = encParam->kIPeriod;
    encVideoParams.ipPeriod = encParam->ipPeriod;
    encVideoParams.rcParams.bitRate = encParam->bitRate;
    encVideoParams.rcParams.initQP = encParam->initQp;
    encVideoParams.rcMode = encParam->rcMode;

    encVideoParams.size = sizeof(VideoParamsCommon);
    setParameters(encoder, VideoParamsTypeCommon, &encVideoParams);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    setParameters(encoder, VideoConfigTypeAVCStreamFormat, &streamFormat);
}

bool VppOutputEncodeCapi::config(NativeDisplay& nativeDisplay, const EncodeParams* encParam)
{
    m_encoder = createEncoder(m_output->getMimeType());
    if (!m_encoder)
        return false;
    encodeSetNativeDisplay(m_encoder, &nativeDisplay);
    setEncodeParam(m_encoder, m_width, m_height, encParam);

    Encode_Status status = encodeStart(m_encoder);
    assert(status == ENCODE_SUCCESS);
    initOuputBuffer();
    return true;
}


class SharedPtrHold
{
public:
    SharedPtrHold(SharedPtr<VideoFrame> frame):frame(frame){}
private:
    SharedPtr<VideoFrame> frame;
};

static void freeHold(VideoFrame* frame)
{
    delete (SharedPtrHold*)frame->user_data;
}

bool VppOutputEncodeCapi::output(const SharedPtr<VideoFrame>& frame)
{
    Encode_Status status = ENCODE_SUCCESS;
    bool drain = !frame;
    if (frame) {
        SharedPtrHold *hold= new SharedPtrHold(frame);
        frame->user_data = (intptr_t)hold;
        frame->free = freeHold;
        status = encode(m_encoder, frame.get());
        if (status != ENCODE_SUCCESS) {
            fprintf(stderr, "encode failed status = %d\n", status);
            return false;
        }
    }
    do {
        status = encodeGetOutput(m_encoder, &m_outputBuffer, drain);
        if (status == ENCODE_SUCCESS
            && !m_output->write(m_outputBuffer.data, m_outputBuffer.dataSize))
             assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);
    return true;
}
