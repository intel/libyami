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
}
