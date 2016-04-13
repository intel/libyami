/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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

#include "encodeInputDecoder.h"
#include "common/log.h"
#include "vaapi/VaapiUtils.h"
#include "assert.h"

EncodeInputDecoder::EncodeInputDecoder(DecodeInput* input)
    : m_input(input)
    , m_decoder(NULL)
    , m_isEOS(false)
    , m_id(0)
{
}
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
        memset(&inputBuffer, 0, sizeof(inputBuffer));
        //flush decoder
        m_decoder->decode(&inputBuffer);
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

class MyRawImage {
public:
    static SharedPtr<MyRawImage> create(VADisplay display, const SharedPtr<VideoFrame>& frame, VideoFrameRawData& inputBuffer)
    {
        SharedPtr<MyRawImage> image;
        if (!frame)
            return image;
        image.reset(new MyRawImage(display, frame));
        if (!image->init(inputBuffer)) {
            image.reset();
        }
        return image;
    }
    ~MyRawImage()
    {
        if (m_frame) {
            unmapImage(m_display, m_image);
        }
    }

private:
    MyRawImage(VADisplay display, const SharedPtr<VideoFrame>& frame)
        : m_display(display)
        , m_frame(frame)
    {
    }

    VAImage m_image;
    VADisplay m_display;
    SharedPtr<VideoFrame> m_frame;
    bool init(VideoFrameRawData& inputBuffer)
    {
        uint8_t* p = mapSurfaceToImage(m_display, m_frame->surface, m_image);
        if (!p) {
            m_frame.reset();
            return false;
        }
        memset(&inputBuffer, 0, sizeof(inputBuffer));
        inputBuffer.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
        inputBuffer.fourcc = m_frame->fourcc;
        inputBuffer.handle = (intptr_t)p;
        memcpy(inputBuffer.pitch, m_image.pitches, sizeof(inputBuffer.pitch));
        memcpy(inputBuffer.offset, m_image.offsets, sizeof(inputBuffer.offset));
        inputBuffer.timeStamp = m_frame->timeStamp;
        inputBuffer.flags = m_frame->flags;
        inputBuffer.width = m_frame->crop.width;
        inputBuffer.height = m_frame->crop.height;
        return true;
    }
    DISALLOW_COPY_AND_ASSIGN(MyRawImage)
};

bool EncodeInputDecoder::getOneFrameInput(VideoFrameRawData &inputBuffer)
{
    if (!m_decoder)
        return false;
    if (m_input->isEOS()) {
        m_isEOS = true;
    }
    SharedPtr<VideoFrame> frame;
    do {
        frame = m_decoder->getOutput();
        if (frame) {
            SharedPtr<MyRawImage> image = MyRawImage::create(m_decoder->getDisplayID(), frame, inputBuffer);
            if (!image)
                return false;
            inputBuffer.internalID = m_id;
            m_images[m_id++] = image;
        }
        else {
            if (m_isEOS)
                return false;
            if (!decodeOneFrame())
                return false;
        }
    } while (!frame);
    return true;
}

bool EncodeInputDecoder::recycleOneFrameInput(VideoFrameRawData &inputBuffer)
{
    if (!m_decoder)
        return false;
    m_images.erase(inputBuffer.internalID);
    return true;
}

bool EncodeInputDecoder::isEOS()
{
    return m_isEOS;
}
