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
#include "common/log.h"
#include "interface/VideoEncoderHost.h"
#include "vaapiencoder_h264.h"
#include <string.h>

IVideoEncoder* createVideoEncoder(const char* mimeType) {
    if (mimeType == NULL) {
        ERROR("NULL mime type.");
        return NULL;
    }
    if (strcasecmp(mimeType, "video/avc") == 0 ||
            strcasecmp(mimeType, "video/h264") == 0) {
        DEBUG("Create H264 encoder ");
        IVideoEncoder *p = new VaapiEncoderH264();
        return (IVideoEncoder *)p;
    } else {
        ERROR("Unsupported mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoEncoder(IVideoEncoder* p) {
    delete p;
}
