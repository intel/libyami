/*
 *  vaapisurfaceallocator.cpp - create va surface for encoder, encoder
 *
 *  Copyright (C) 2015 Intel Corporation
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
    for (int i = 0; i < size; i++) {
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
    for (int i = 0; i < size; i++) {
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
