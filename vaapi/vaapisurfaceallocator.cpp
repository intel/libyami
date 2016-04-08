/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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
#include "vaapi/vaapisurfaceallocator.h"
#include "vaapi/vaapiutils.h"
#include <vector>

namespace YamiMediaCodec{

VaapiSurfaceAllocator::VaapiSurfaceAllocator(VADisplay display, uint32_t extraSize)
    :m_display(display),
    m_extraSize(extraSize)
{
}

YamiStatus VaapiSurfaceAllocator::doAlloc(SurfaceAllocParams* params)
{
    if (!params)
        return YAMI_INVALID_PARAM;
    uint32_t size = params->size;
    uint32_t width = params->width;
    uint32_t height = params->height;
    if (!width || !height || !size)
        return YAMI_INVALID_PARAM;
    if (params->fourcc != YAMI_FOURCC_NV12) {
        ERROR("fix me for 10 bits");
        return YAMI_INVALID_PARAM;
    }

    size += m_extraSize;

    std::vector<VASurfaceID> v(size);
    VAStatus status = vaCreateSurfaces(m_display,
        VA_RT_FORMAT_YUV420, width, height,
        &v[0], size, NULL, 0);
    if (!checkVaapiStatus(status, "vaCreateSurfaces"))
        return YAMI_OUT_MEMORY;
    params->surfaces = new intptr_t[size];
    for (uint32_t i = 0; i < size; i++) {
        params->surfaces[i] = (intptr_t)v[i];
    }
    params->size = size;
    return YAMI_SUCCESS;
}

YamiStatus VaapiSurfaceAllocator::doFree(SurfaceAllocParams* params)
{
    if (!params || !params->size || !params->surfaces)
        return YAMI_INVALID_PARAM;
    uint32_t size = params->size;
    std::vector<VASurfaceID> v(size);
    for (uint32_t i = 0; i < size; i++) {
        v[i] = (VASurfaceID)params->surfaces[i];
    }
    checkVaapiStatus(vaDestroySurfaces(m_display, &v[0], size), "vaDestroySurfaces");
    delete[] params->surfaces;
    return YAMI_SUCCESS;
}

void VaapiSurfaceAllocator::doUnref()
{
    delete this;
}

} //YamiMediaCodec
