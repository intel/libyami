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
#include "interface/VideoPostProcessHost.h"
#include "vaapi/vaapi_host.h"
#include "vpp/vaapipostprocess_scaler.h"
#include <string.h>

using namespace YamiMediaCodec;
DEFINE_CLASS_FACTORY(PostProcess)
static const PostProcessEntry g_vppEntries[] = {
    DEFINE_VPP_ENTRY(YAMI_VPP_SCALER, Scaler)
};

#ifndef N_ELEMENTS
#define N_ELEMENTS(array) (sizeof(array)/sizeof(array[0]))
#endif
extern "C" {
IVideoPostProcess *createVideoPostProcess(const char *mimeType)
{
    yamiTraceInit();
    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }
    INFO("mimeType: %s\n", mimeType);
    for (int i = 0; i < N_ELEMENTS(g_vppEntries); i++) {
        const PostProcessEntry *e = g_vppEntries + i;
        if (strcasecmp(e->mime, mimeType) == 0)
            return e->create();
    }
    ERROR("Failed to create %s", mimeType);
    return NULL;
}

void releaseVideoPostProcess(IVideoPostProcess * p)
{
    delete p;
}

} // extern "C"
