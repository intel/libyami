/*
 * Copyright 2016 Intel Corporation
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

#include "interface/VideoCommonDefs.h"
#include "VaapiUtils.h"

namespace YamiMediaCodec {

uint8_t* mapSurfaceToImage(VADisplay display, intptr_t surface, VAImage& image)
{
    uint8_t* p = NULL;
    VAStatus status = vaDeriveImage(display, (VASurfaceID)surface, &image);
    if (!checkVaapiStatus(status, "vaDeriveImage"))
        return NULL;
    status = vaMapBuffer(display, image.buf, (void**)&p);
    if (!checkVaapiStatus(status, "vaMapBuffer")) {
        checkVaapiStatus(vaDestroyImage(display, image.image_id), "vaDestroyImage");
        return NULL;
    }
    return p;
}

void unmapImage(VADisplay display, const VAImage& image)
{
    checkVaapiStatus(vaUnmapBuffer(display, image.buf), "vaUnmapBuffer");
    checkVaapiStatus(vaDestroyImage(display, image.image_id), "vaDestroyImage");
}

//return rt format, 0 for unsupported
uint32_t getRtFormat(uint32_t fourcc)
{
    switch (fourcc) {
    case YAMI_FOURCC_NV12:
    case YAMI_FOURCC_I420:
    case YAMI_FOURCC_YV12:
    case YAMI_FOURCC_IMC3:
        return VA_RT_FORMAT_YUV420;
    case YAMI_FOURCC_422H:
    case YAMI_FOURCC_422V:
    case YAMI_FOURCC_YUY2:
        return VA_RT_FORMAT_YUV422;
    case YAMI_FOURCC_444P:
        return VA_RT_FORMAT_YUV444;
    case YAMI_FOURCC_RGBX:
    case YAMI_FOURCC_RGBA:
    case YAMI_FOURCC_BGRX:
    case YAMI_FOURCC_BGRA:
        return VA_RT_FORMAT_RGB32;
    case YAMI_FOURCC_P010:
        return VA_RT_FORMAT_YUV420_10BPP;
    }
    ERROR("get rt format for %.4s failed", (char*)&fourcc);
    return 0;
}
}
