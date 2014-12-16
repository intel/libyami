/*
 *  vaapiencoder_host.cpp - create specific type of video encoder
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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

#include <stdint.h>
#include "common/common_def.h"
#include "common/log.h"
#include "interface/VideoEncoderHost.h"
#if __BUILD_H264_ENCODER__
#include "vaapiencoder_h264.h"
#endif
#if __BUILD_JPEG_ENCODER__
#include "vaapiencoder_jpeg.h"
#endif
#include "vaapi/vaapi_host.h"
#include <string.h>

using namespace YamiMediaCodec;
DEFINE_CLASS_FACTORY(Encoder)
static const EncoderEntry g_encoderEntries[] = {
#if __BUILD_H264_ENCODER__
    DEFINE_ENCODER_ENTRY("video/avc", H264),
    DEFINE_ENCODER_ENTRY("video/h264", H264),
#endif
#if __BUILD_JPEG_ENCODER__
    DEFINE_ENCODER_ENTRY("image/jpeg", Jpeg)
#endif
};
extern "C" {
IVideoEncoder* createVideoEncoder(const char* mimeType) {
    yamiTraceInit();
    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }
    INFO("mimeType: %s\n", mimeType);
    for (int i = 0; i < N_ELEMENTS(g_encoderEntries); i++) {
        const EncoderEntry *e = g_encoderEntries + i;
        if (strcasecmp(e->mime, mimeType) == 0)
            return e->create();
    }
    ERROR("Failed to create %s", mimeType);
    return NULL;
}

void releaseVideoEncoder(IVideoEncoder* p) {
    delete p;
}

bool preSandboxInitEncoder() {
    // TODO, for hybrid VP8 encoder uses mediasdk, does the prework here
    return true;
}
} // extern "C"
