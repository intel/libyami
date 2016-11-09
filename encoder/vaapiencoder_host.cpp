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
#include "VideoEncoderHost.h"
#include "vaapiencoder_factory.h"

using namespace YamiMediaCodec;

#if __BUILD_H264_ENCODER__
#include "vaapiencoder_h264.h"
const bool VaapiEncoderH264::s_registered =
    VaapiEncoderFactory::register_<VaapiEncoderH264>(YAMI_MIME_AVC)
    && VaapiEncoderFactory::register_<VaapiEncoderH264>(YAMI_MIME_H264);
#endif

#if __BUILD_H265_ENCODER__
#include "vaapiencoder_hevc.h"
const bool VaapiEncoderHEVC::s_registered =
    VaapiEncoderFactory::register_<VaapiEncoderHEVC>(YAMI_MIME_HEVC)
    && VaapiEncoderFactory::register_<VaapiEncoderHEVC>(YAMI_MIME_H265);
#endif

#if __BUILD_JPEG_ENCODER__
#include "vaapiencoder_jpeg.h"
const bool VaapiEncoderJpeg::s_registered =
    VaapiEncoderFactory::register_<VaapiEncoderJpeg>(YAMI_MIME_JPEG);
#endif

#if __BUILD_VP8_ENCODER__
#include "vaapiencoder_vp8.h"
const bool VaapiEncoderVP8::s_registered =
    VaapiEncoderFactory::register_<VaapiEncoderVP8>(YAMI_MIME_VP8);
#endif

#if __BUILD_VP9_ENCODER__
#include "vaapiencoder_vp9.h"
const bool VaapiEncoderVP9::s_registered
    = VaapiEncoderFactory::register_<VaapiEncoderVP9>(YAMI_MIME_VP9);
#endif

extern "C" {

IVideoEncoder* createVideoEncoder(const char* mimeType) {
    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }

    VaapiEncoderFactory::Type enc = VaapiEncoderFactory::create(mimeType);

    if (!enc)
        ERROR("Failed to create encoder for mimeType: '%s'", mimeType);
    else
        INFO("Created encoder for mimeType: '%s'", mimeType);

    return enc;
}

void releaseVideoEncoder(IVideoEncoder* p) {
    delete p;
}

std::vector<std::string> getVideoEncoderMimeTypes()
{
    return VaapiEncoderFactory::keys();
}

} // extern "C"
