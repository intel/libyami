/*
 *  encodeinput.cpp - encode test input
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Guangxin Xu<Guangxin.Xu@intel.com>
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
#include "vppoutputencode.h"

struct MimeEntry
{
    const char* ext;
    const char* mime;
};

static const MimeEntry MimeEntrys[] = {
    "vp8", YAMI_MIME_VP8,
    "264", YAMI_MIME_H264
};

const char* guessMime(const char* filename)
{
    if(filename==NULL)
        return NULL;
    const char *ext = strrchr(filename,'.');
    if(ext==NULL)
        return NULL;
    ext++;
    for (int i = 0; i < N_ELEMENTS(MimeEntrys); i++) {
        const MimeEntry* entry = MimeEntrys + i;
        if (strcasecmp(entry->ext, ext) == 0) {
            return entry->mime;
        }
    }
    return NULL;
}

bool VppOutputEncode::init(const char* outputFileName, uint32_t fourcc, int width, int height)
{
    m_mime = guessMime(outputFileName);
    if (!m_mime)
        return false;

    if(!width || !height)
        if (!guessResolution(outputFileName, width, height))
            return false;

    if (!fourcc)
        fourcc = guessFourcc(outputFileName);
    if (!fourcc)
        fourcc = VA_FOURCC('I', '4', '2', '0');

    m_fourcc = fourcc;
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

static void setEncodeParam(const SharedPtr<IVideoEncoder>& encoder, int width, int height)
{
       //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
    encVideoParams.resolution.width = width;
    encVideoParams.resolution.height = height;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);
}

bool VppOutputEncode::config(NativeDisplay& nativeDisplay)
{
    m_encoder.reset(createVideoEncoder(m_mime), releaseVideoEncoder);
    if (!m_encoder)
        return false;
    m_encoder->setNativeDisplay(&nativeDisplay);
    setEncodeParam(m_encoder, m_width, m_height);

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
