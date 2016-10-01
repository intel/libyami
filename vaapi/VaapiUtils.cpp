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
#if VA_CHECK_VERSION(0, 38, 1)
    case YAMI_FOURCC_P010:
        return VA_RT_FORMAT_YUV420_10BPP;
#endif
    case YAMI_FOURCC_RGBX:
    case YAMI_FOURCC_RGBA:
    case YAMI_FOURCC_BGRX:
    case YAMI_FOURCC_BGRA:
        return VA_RT_FORMAT_RGB32;
    }
    ERROR("get rt format for %.4s failed", (char*)&fourcc);
    return 0;
}

bool dumpSurface(VADisplay display, intptr_t surface)
{
    static uint32_t _frameCount = 0;
    const uint8_t* p = NULL;
    uint32_t fourcc = 0;
    const char* ptrC = (char*)(&fourcc);
    char fileName[256];
    VAImage image;
    static FILE* fp = NULL;
    bool useSingleFile = true;

    memset(&image, 0, sizeof(VAImage));
    p = mapSurfaceToImage(display, surface, image);
    fourcc = image.format.fourcc;

    if (fourcc != VA_FOURCC_NV12) {
        WARNING("format: (%c%c%c%c, 0x%x) is not support to dump surface", ptrC[0], ptrC[1], ptrC[2], ptrC[3], fourcc);
        return false;
    }
    if (!image.width || !image.height) {
        WARNING("invalid surface/image width: %d, height: %d", image.width, image.height);
        return false;
    }

    if (!fp) {
        memset(fileName, 0, sizeof(fileName));
        if (useSingleFile) {
            sprintf(fileName, "%s/%c%c%c%c_%dx%d", "/tmp/yami", ptrC[0], ptrC[1], ptrC[2], ptrC[3], image.width, image.height);
        }
        else {
            sprintf(fileName, "%s/%04d_%c%c%c%c_%dx%d", "/tmp/yami", _frameCount, ptrC[0], ptrC[1], ptrC[2], ptrC[3], image.width, image.height);
            _frameCount++;
        }
        fp = fopen(fileName, "w+");
    }
    if (!fp)
        return false;

    int widths[3], heights[3];
    widths[0] = image.width;
    widths[1] = ((image.width + 1) / 2) * 2;
    widths[2] = 0;
    heights[0] = image.height;
    heights[1] = (image.height + 1) / 2;
    heights[2] = 0;

    uint32_t plane = 0;
    for (plane = 0; plane < image.num_planes; plane++) {
        int h = 0;
        for (h = 0; h < heights[plane]; h++) {
            fwrite(p + image.offsets[plane] + image.pitches[plane] * h, widths[plane], 1, fp);
        }
    }

    if (!useSingleFile) {
        fclose(fp);
        fp = NULL;
    }

    unmapImage(display, image);

    return true;
}
}
