/*
 *  vaapidecoder_fake.cpp - fake decoder
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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

Decode_Status VaapiDecoderFake::start(VideoConfigBuffer * buffer)
{
    DEBUG("VP9: start() buffer size: %d x %d", buffer->width,
          buffer->height);

    VideoConfigBuffer config;
    config = *buffer;
    config.profile = VAProfileH264Main;
    config.surfaceNumber = FAKE_EXTRA_SURFACE_NUMBER;
    config.flag &= ~USE_NATIVE_GRAPHIC_BUFFER;
    config.width = m_width;
    config.height = m_height;
    config.surfaceWidth = m_width;
    config.surfaceHeight = m_height;
    return VaapiDecoderBase::start(&config);
}

Decode_Status VaapiDecoderFake::decode(VideoDecodeBuffer *buffer)
{
    if (m_first) {
        m_first = false;
        return DECODE_FORMAT_CHANGE;
    }
    PicturePtr picture = createPicture(buffer->timeStamp);
    if (!picture)
        return DECODE_MEMORY_FAIL;
    return outputPicture(picture);
}

}

