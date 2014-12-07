/*
 *  encodeInputDecoder.cpp - use decoder as encoder input
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/log.h"
#include "encodeInputDecoder.h"
#include "assert.h"

EncodeInputDecoder::~EncodeInputDecoder()
{
    if (m_decoder) {
        m_decoder->stop();
        releaseVideoDecoder(m_decoder);
    }
    delete m_input;
}

bool EncodeInputDecoder::init(const char* /*inputFileName*/, uint32_t /*fourcc*/, int /* width */, int /*height*/)
{
    assert(m_input && "invalid input");
    m_decoder = createVideoDecoder(m_input->getMimeType());
    if (!m_decoder)
        return false;
    m_fourcc = VA_FOURCC_NV12;

    VideoConfigBuffer configBuffer;
    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;
    Decode_Status status = m_decoder->start(&configBuffer);
    assert(status == DECODE_SUCCESS);

    while (!m_width || !m_height) {
        if (!decodeOneFrame())
            return false;
    }
    return true;
}

bool EncodeInputDecoder::decodeOneFrame()
{
    if (!m_input || !m_decoder)
        return false;
    VideoDecodeBuffer inputBuffer;
    if (!m_input->getNextDecodeUnit(inputBuffer)) {
        m_isEOS = true;
        return true;
    }
    Decode_Status status = m_decoder->decode(&inputBuffer);
    if (status == DECODE_FORMAT_CHANGE) {
        const VideoFormatInfo *formatInfo = m_decoder->getFormatInfo();
        m_width = formatInfo->width;
        m_height = formatInfo->height;
        //send again
        status = m_decoder->decode(&inputBuffer);;
    }
    if (status != DECODE_SUCCESS) {
        ERROR("decode failed status = %d", status);
        return false;
    }
    return true;
}

bool EncodeInputDecoder::getOneFrameInput(VideoFrameRawData &inputBuffer)
{
    if (!m_decoder)
        return false;
    bool drain = false;
    if (m_input->isEOS()) {
        m_isEOS = true;
        drain = true;
    }
    Decode_Status status;
    do {
        memset(&inputBuffer, 0, sizeof(inputBuffer));
        inputBuffer.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
        inputBuffer.fourcc = m_fourcc;
        status = m_decoder->getOutput(&inputBuffer, drain);
        inputBuffer.flags = 0;
        if (status == RENDER_NO_AVAILABLE_FRAME) {
            if (m_isEOS)
                return false;
            if (!decodeOneFrame())
                return false;
        }
    } while (status == RENDER_NO_AVAILABLE_FRAME);
    return status == RENDER_SUCCESS;
}

bool EncodeInputDecoder::recycleOneFrameInput(VideoFrameRawData &inputBuffer)
{
    if (!m_decoder)
        return false;
    m_decoder->renderDone(&inputBuffer);
    return true;
}

bool EncodeInputDecoder::isEOS()
{
    return m_isEOS;
}
