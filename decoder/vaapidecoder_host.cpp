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
#include "vaapidecoder_jpeg.h"
#include "vaapi_host.h"
#include <string.h>

DEFINE_CLASS_FACTORY(Decoder)
static const DecoderEntry g_decoderEntries[] = {
    DEFINE_DECODER_ENTRY("video/avc", H264),
    DEFINE_DECODER_ENTRY("video/h264", H264),
    DEFINE_DECODER_ENTRY("video/jpeg", Jpeg),
    DEFINE_DECODER_ENTRY("video/x-vnd.on2.vp8", VP8)
};

IVideoDecoder *createVideoDecoder(const char *mimeType)
{
    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }
    INFO("mimeType: %s\n", mimeType);
    for (int i = 0; i < N_ELEMENTS(g_decoderEntries); i++) {
        const DecoderEntry *e = g_decoderEntries + i;
        if (strcasecmp(e->mime, mimeType) == 0)
            return e->create();
    }
    ERROR("Failed to create %s", mimeType);
    return NULL;
}

void releaseVideoDecoder(IVideoDecoder * p)
{
    delete p;
}

