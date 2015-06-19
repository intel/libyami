/*
 *  vaapidecoder_host.cpp - create specific type of video decoder
 *
 *  Copyright (C) 2013 Intel Corporation
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
#include "interface/VideoDecoderHost.h"
#include "vaapidecoder_factory.h"

#if __BUILD_FAKE_DECODER__
#include "vaapidecoder_fake.h"
#endif

using namespace YamiMediaCodec;

extern "C" {

IVideoDecoder *createVideoDecoder(const char *mimeType)
{
    yamiTraceInit();

    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }

#if __BUILD_FAKE_DECODER__
    INFO("Created fake decoder");
    return new VaapiDecoderFake(1920, 1080);
#endif

    VaapiDecoderFactory::Type dec = VaapiDecoderFactory::create(mimeType);

    if (!dec)
        ERROR("Failed to create decoder for mimeType: '%s'", mimeType);
    else
        INFO("Created decoder for mimeType: '%s'", mimeType);

    return dec;
}

void releaseVideoDecoder(IVideoDecoder * p)
{
    delete p;
}

} // extern "C"
