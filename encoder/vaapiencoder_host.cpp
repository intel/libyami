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

#include "common/log.h"
#include "interface/VideoEncoderHost.h"
#include "vaapiencoder_factory.h"

using namespace YamiMediaCodec;

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

} // extern "C"
