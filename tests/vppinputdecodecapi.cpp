/*
 *  vppinputdecodecapi.cpp - vpp input from decoded file for capi
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
 #include "vppinputdecodecapi.h"

static void freeFrame(VideoFrame* frame)
{
	frame->free(frame);
}

VppInputDecodeCapi::~VppInputDecodeCapi()
{
	decodeStop(m_decoder);
	releaseDecoder(m_decoder);
}

bool VppInputDecodeCapi::init(const char* inputFileName, uint32_t /*fourcc*/, int /*width*/, int /*height*/)
{
    m_input.reset(DecodeInput::create(inputFileName));
    if (!m_input)
        return false;
    m_decoder = createDecoder(m_input->getMimeType());
    if (!m_decoder) {
        fprintf(stderr, "failed create decoder for %s", m_input->getMimeType());
        return false;
    }
    return true;
}

bool VppInputDecodeCapi::config(NativeDisplay& nativeDisplay)
{
    setNativeDisplay(m_decoder, &nativeDisplay);

    VideoConfigBuffer configBuffer;
    memset(&configBuffer, 0, sizeof(configBuffer));
    configBuffer.profile = VAProfileNone;
    const string codecData = m_input->getCodecData();
    if (codecData.size()) {
        configBuffer.data = (uint8_t*)codecData.data();
        configBuffer.size = codecData.size();
    }

    Decode_Status status = decodeStart(m_decoder, &configBuffer);
    return status == DECODE_SUCCESS;
}

bool VppInputDecodeCapi::read(SharedPtr<VideoFrame>& frame)
{
    while (1)  {
        VideoFrame* tmp = getOutput(m_decoder);
        if (tmp) {
            frame.reset(tmp, freeFrame);
            return true;
        }
        if (m_error || m_eos)
            return false;
        VideoDecodeBuffer inputBuffer;
        Decode_Status status = DECODE_FAIL;
        if (m_input->getNextDecodeUnit(inputBuffer)) {
            status = decode(m_decoder, &inputBuffer);
            if (DECODE_FORMAT_CHANGE == status) {
                //resend the buffer
                status = decode(m_decoder, &inputBuffer);
            }
        } else { /*EOS, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
			decode(m_decoder, &inputBuffer);
            m_eos = true;
        }
        if (status != DECODE_SUCCESS){  /*failed, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
			decode(m_decoder, &inputBuffer);
            m_error = true;
        }

    }

}
