/*
 * Copyright (C) 2013-2016 Intel Corporation. All rights reserved.
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
#include "VideoDecoderHost.h"
#include "vaapidecoder_factory.h"

#if __BUILD_FAKE_DECODER__
#include "vaapidecoder_fake.h"
#endif

using namespace YamiMediaCodec;

#if __BUILD_H264_DECODER__
#include "vaapidecoder_h264.h"
const bool VaapiDecoderH264::s_registered
    = VaapiDecoderFactory::register_<VaapiDecoderH264>(YAMI_MIME_AVC)
      && VaapiDecoderFactory::register_<VaapiDecoderH264>(YAMI_MIME_H264);
#endif

#if __BUILD_H265_DECODER__
#include "vaapidecoder_h265.h"
const bool VaapiDecoderH265::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderH265>(YAMI_MIME_H265)
    && VaapiDecoderFactory::register_<VaapiDecoderH265>(YAMI_MIME_HEVC);
#endif

#if __BUILD_MPEG2_DECODER__
#include "vaapidecoder_mpeg2.h"
const bool VaapiDecoderMPEG2::s_registered
    = VaapiDecoderFactory::register_<VaapiDecoderMPEG2>(YAMI_MIME_MPEG2);
#endif

#if __BUILD_VC1_DECODER__
#include "vaapidecoder_vc1.h"
const bool VaapiDecoderVC1::s_registered
    = VaapiDecoderFactory::register_<VaapiDecoderVC1>(YAMI_MIME_VC1);
#endif

#if __BUILD_VP8_DECODER__
#include "vaapidecoder_vp8.h"
const bool VaapiDecoderVP8::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderVP8>(YAMI_MIME_VP8);
#endif

#if __BUILD_VP9_DECODER__
#include "vaapidecoder_vp9.h"
const bool VaapiDecoderVP9::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderVP9>(YAMI_MIME_VP9);
#endif

#if __BUILD_JPEG_DECODER__
#include "vaapiDecoderJPEG.h"
const bool VaapiDecoderJPEG::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderJPEG>(YAMI_MIME_JPEG);
#endif

extern "C" {

IVideoDecoder *createVideoDecoder(const char *mimeType)
{
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

std::vector<std::string> getVideoDecoderMimeTypes()
{
    return VaapiDecoderFactory::keys();
}
} // extern "C"
