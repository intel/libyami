/*
 *  vaapisurface.cpp - VA surface abstraction
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li<xiaowei.li@intel.com>
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

#include <assert.h>
#include "vaapidisplay.h"
#include "vaapisurface.h"
#include "common/log.h"
#include "vaapiutils.h"


namespace YamiMediaCodec{
/* FIXME: find a better place for this*/
static uint32_t vaapiChromaToVaChroma(VaapiChromaType chroma)
{

    struct chromaItem {
        VaapiChromaType vaapiChroma;
        uint32_t vaChroma;
    };
    static const chromaItem chromaMap[] = {
        {VAAPI_CHROMA_TYPE_YUV420, VA_RT_FORMAT_YUV420},
        {VAAPI_CHROMA_TYPE_YUV422, VA_RT_FORMAT_YUV422},
        {VAAPI_CHROMA_TYPE_YUV444, VA_RT_FORMAT_YUV444}
    };
    for (int i = 0; i < N_ELEMENTS(chromaMap); i++) {
        if (chromaMap[i].vaapiChroma == chroma)
            return chromaMap[i].vaChroma;
    }
    return VA_RT_FORMAT_YUV420;
}

SurfacePtr VaapiSurface::create(const DisplayPtr& display,
                                VaapiChromaType chromaType,
                                uint32_t width,
                                uint32_t height,
                                void *surfAttribArray,
                                uint32_t surfAttribNum)
{
    VAStatus status;
    uint32_t format, i;
    VASurfaceAttrib *surfAttribs = (VASurfaceAttrib *) surfAttribArray;
    SurfacePtr surface;
    VASurfaceID id;

    assert((surfAttribs && surfAttribNum)
           || (!surfAttribs && !surfAttribNum));


    format = vaapiChromaToVaChroma(chromaType);
    uint32_t externalBufHandle = 0;
    status = vaCreateSurfaces(display->getID(), format, width, height,
                              &id, 1, surfAttribs, surfAttribNum);
    if (!checkVaapiStatus(status, "vaCreateSurfacesWithAttribute()"))
        return surface;
    surface.reset(new VaapiSurface(display, id, chromaType,
                                    width, height));
    return surface;
}

VaapiSurface::VaapiSurface(const DisplayPtr& display,
                           VASurfaceID id,
                           VaapiChromaType chromaType,
                           uint32_t width,
                           uint32_t height)
    : m_display(display), m_chromaType(chromaType)
    , m_allocWidth(width), m_allocHeight(height), m_width(width), m_height(height)
    , m_ID(id), m_owner(true)
{

}

VaapiSurface::VaapiSurface(const DisplayPtr& display, VASurfaceID id)
    : m_display(display), m_chromaType(0)
    , m_allocWidth(0), m_allocHeight(0), m_width(0), m_height(0)
    , m_ID(id), m_owner(false)
{

}

VaapiSurface::~VaapiSurface()
{
    VAStatus status = VA_STATUS_SUCCESS;

    if (m_owner)
        status = vaDestroySurfaces(m_display->getID(), &m_ID, 1);

    if (!checkVaapiStatus(status, "vaDestroySurfaces()"))
        WARNING("failed to destroy surface");

}

VaapiChromaType VaapiSurface::getChromaType(void)
{
    return m_chromaType;
}

VASurfaceID VaapiSurface::getID(void) const
{
    return m_ID;
}

uint32_t VaapiSurface::getWidth(void)
{
    return m_width;
}

uint32_t VaapiSurface::getHeight(void)
{
    return m_height;
}

bool VaapiSurface::resize(uint32_t width, uint32_t height)
{
    if (width>m_allocWidth || height>m_allocHeight)
        return false;

    m_width = width;
    m_height = height;
    return true;
}

DisplayPtr VaapiSurface::getDisplay()
{
    return m_display;
}

bool VaapiSurface::getImage(ImagePtr image)
{
    VAImageID imageID;
    VAStatus status;
    uint32_t width, height;

    if (!image)
        return false;

    width = image->getWidth();
    height = image->getHeight();

    if (width != m_width || height != m_height) {
        ERROR("resolution not matched \n");
        return false;
    }

    imageID = image->getID();

    DEBUG("Display: 0x%x, surface: 0x%x, width: %d, height: %d, image: 0x%x", m_display->getID(), m_ID, width, height, imageID);
    status = vaGetImage(m_display->getID(), m_ID, 0, 0, width, height, imageID);

    if (!checkVaapiStatus(status, "vaGetImage()"))
        return false;

    return true;
}

bool VaapiSurface::putImage(ImagePtr image)
{
    VAImageID imageID;
    VAStatus status;
    uint32_t width, height;

    if (!image)
        return false;

    width = image->getWidth();
    height = image->getHeight();

    if (width != m_width || height != m_height) {
        ERROR("Image resolution does not match with surface");
        return false;
    }

    imageID = image->getID();

    status = vaPutImage(m_display->getID(), m_ID, imageID, 0, 0, width, height, 0, 0, width, height);

    if (!checkVaapiStatus(status, "vaPutImage()"))
        return false;

    return true;
}

bool VaapiSurface::sync()
{
    VAStatus status;

    status = vaSyncSurface((VADisplay) m_display->getID(), (VASurfaceID) m_ID);

    if (!checkVaapiStatus(status, "vaSyncSurface()"))
        return false;

    return true;
}

bool VaapiSurface::queryStatus(VaapiSurfaceStatus * pStatus)
{
    VASurfaceStatus surfaceStatus;
    VAStatus status;

    if (!pStatus)
        return false;

    status = vaQuerySurfaceStatus((VADisplay) m_display->getID(),
                                  (VASurfaceID) m_ID, &surfaceStatus);

    if (!checkVaapiStatus(status, "vaQuerySurfaceStatus()"))
        return false;

    *pStatus = (VaapiSurfaceStatus) toVaapiSurfaceStatus(surfaceStatus);
    return true;
}

uint32_t VaapiSurface::toVaapiSurfaceStatus(uint32_t vaFlags)
{
    uint32_t flags;
    const uint32_t vaFlagMask = (VASurfaceReady |
                                 VASurfaceRendering |
                                 VASurfaceDisplaying | VASurfaceSkipped);

    switch (vaFlags & vaFlagMask) {
    case VASurfaceReady:
        flags = VAAPI_SURFACE_STATUS_IDLE;
        break;
    case VASurfaceRendering:
        flags = VAAPI_SURFACE_STATUS_RENDERING;
        break;
    case VASurfaceDisplaying:
        flags = VAAPI_SURFACE_STATUS_DISPLAYING;
        break;
    case VASurfaceSkipped:
        flags = VAAPI_SURFACE_STATUS_SKIPPED;
        break;
    default:
        flags = 0;
        break;
    }

    return flags;
}
}
