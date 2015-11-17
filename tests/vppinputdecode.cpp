/*
 *  vppinputdecode.cpp - vpp input from decoded file
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
#include "tests/vppinputdecode.h"

VppInputDecode::~VppInputDecode()
{
    if (m_decoder)
        m_decoder->stop();
}

bool VppInputDecode::init(const char* inputFileName, uint32_t /*fourcc*/, int /*width*/, int /*height*/)
{
    m_input.reset(DecodeInput::create(inputFileName));
    if (!m_input)
        return false;
    m_decoder.reset(createVideoDecoder(m_input->getMimeType()), releaseVideoDecoder);
    if (!m_decoder) {
        fprintf(stderr, "failed create decoder for %s", m_input->getMimeType());
        return false;
    }
    return true;
}

bool VppInputDecode::config(NativeDisplay& nativeDisplay)
{
    m_decoder->setNativeDisplay(&nativeDisplay);

    VideoConfigBuffer configBuffer;
    memset(&configBuffer, 0, sizeof(configBuffer));
    configBuffer.profile = VAProfileNone;
    const string codecData = m_input->getCodecData();
    if (codecData.size()) {
        configBuffer.data = (uint8_t*)codecData.data();
        configBuffer.size = codecData.size();
    }

    Decode_Status status = m_decoder->start(&configBuffer);
    return status == DECODE_SUCCESS;
}

bool VppInputDecode::read(SharedPtr<VideoFrame>& frame)
{
    while (1)  {
        frame = m_decoder->getOutput();
        if (frame)
            return true;
        if (m_error || m_eos)
            return false;
        VideoDecodeBuffer inputBuffer;
        Decode_Status status = DECODE_FAIL;
        if (m_input->getNextDecodeUnit(inputBuffer)) {
            status = m_decoder->decode(&inputBuffer);
            if (DECODE_FORMAT_CHANGE == status) {
                //resend the buffer
                status = m_decoder->decode(&inputBuffer);
            }
        } else { /*EOS, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
            status = m_decoder->decode(&inputBuffer);
            m_eos = true;
        }
        if (status != DECODE_SUCCESS){  /*failed, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
            m_decoder->decode(&inputBuffer);
            m_error = true;
        }

    }

}
