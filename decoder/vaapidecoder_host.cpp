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
#include "vaapidecoder_h264.h"
#include "vaapidecoder_vp8.h"
#include <string.h>

IVideoDecoder *createVideoDecoder(const char *mimeType)
{
    if (mimeType == NULL) {
        ERROR("NULL mime type.");
        return NULL;
    }
    INFO("mimeType: %s\n", mimeType);
/*
    if (strcasecmp(mimeType, "video/wmv") == 0 ||
        strcasecmp(mimeType, "video/vc1") == 0) {
        VideoDecoderWMV *p = new VideoDecoderWMV(mimeType);
        return (IVideoDecoder *)p;
    } else */ if (strcasecmp(mimeType, "video/avc") == 0 ||
                  strcasecmp(mimeType, "video/h264") == 0) {
        DEBUG("Create H264 decoder ");
        IVideoDecoder *p = new VaapiDecoderH264(mimeType);
        return (IVideoDecoder *) p;
#if 0
    } else if (strcasecmp(mimeType, "video/mp4v-es") == 0 ||
               strcasecmp(mimeType, "video/mpeg4") == 0 ||
               strcasecmp(mimeType, "video/h263") == 0 ||
               strcasecmp(mimeType, "video/3gpp") == 0) {
        VideoDecoderMPEG4 *p = new VideoDecoderMPEG4(mimeType);
        return (IVideoDecoder *) p;
    } else if (strcasecmp(mimeType, "video/pavc") == 0) {
        VideoDecoderAVC *p = new VideoDecoderPAVC(mimeType);
        return (IVideoDecoder *) p;
    } else if (strcasecmp(mimeType, "video/avc-secure") == 0) {
        VideoDecoderAVC *p = new VideoDecoderAVCSecure(mimeType);
        return (IVideoDecoder *) p;
#endif
    } else if (strcasecmp(mimeType, "video/x-vnd.on2.vp8") == 0) {
        DEBUG("Create VP8 decoder ");
        IVideoDecoder *p = new VaapiDecoderVP8(mimeType);
        return (IVideoDecoder *) p;
    } else {
        ERROR("Unsupported mime type: %s", mimeType);
    }
    return NULL;
}

void releaseVideoDecoder(IVideoDecoder * p)
{
    if (p) {
        const VideoFormatInfo *info = p->getFormatInfo();
        if (info && info->mimeType) {
            DEBUG("Deleting decoder for %s", info->mimeType);
        }
    }
    delete p;
}
