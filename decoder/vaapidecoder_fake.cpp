/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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

#include "common/log.h"
#include "vaapidecoder_fake.h"

namespace YamiMediaCodec{
typedef VaapiDecoderFake::PicturePtr PicturePtr;

VaapiDecoderFake::VaapiDecoderFake(int32_t width, int32_t height)
    :m_width(width), m_height(height), m_first(true)
{
}

VaapiDecoderFake::~VaapiDecoderFake()
{
    stop();
}

YamiStatus VaapiDecoderFake::start(VideoConfigBuffer* buffer)
{
    DEBUG("VP9: start() buffer size: %d x %d", buffer->width,
          buffer->height);

    VideoConfigBuffer config;
    config = *buffer;
    config.profile = VAProfileH264Main;
    config.surfaceNumber = FAKE_EXTRA_SURFACE_NUMBER;
    config.width = m_width;
    config.height = m_height;
    config.surfaceWidth = m_width;
    config.surfaceHeight = m_height;
    return VaapiDecoderBase::start(&config);
}

YamiStatus VaapiDecoderFake::decode(VideoDecodeBuffer* buffer)
{
    if (m_first) {
        m_first = false;
        return YAMI_DECODE_FORMAT_CHANGE;
    }
    PicturePtr picture;
    YamiStatus status = createPicture(picture, buffer->timeStamp);
    if (status != YAMI_SUCCESS)
        return status;
    return outputPicture(picture);
}

}

