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
#include "tests/vppinputdecode.h"

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
    configBuffer.width = m_input->getWidth();
    configBuffer.height = m_input->getHeight();
    Decode_Status status = m_decoder->start(&configBuffer);
    if (status == DECODE_SUCCESS) {
        //read first frame to update width height
        if (!read(m_first))
            status = DECODE_FAIL;
    }
    return status == DECODE_SUCCESS;
}

bool VppInputDecode::read(SharedPtr<VideoFrame>& frame)
{
    if (m_first) {
        frame = m_first;
        m_first.reset();
        return true;
    }

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

                //update width height
                const VideoFormatInfo* info = m_decoder->getFormatInfo();
                m_width = info->width;
                m_height = info->height;

                //resend the buffer
                status = m_decoder->decode(&inputBuffer);
            }
        } else { /*EOS, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
            status = m_decoder->decode(&inputBuffer);
            m_eos = true;
        }
        if (status != DECODE_SUCCESS) { /*failed, need to flush*/
            inputBuffer.data = NULL;
            inputBuffer.size = 0;
            m_decoder->decode(&inputBuffer);
            m_error = true;
        }
    }

}
