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

#include "vaapisurface.h"
#include "log.h"
#include "vaapiutils.h"
#include "common_def.h"

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

SurfacePtr VaapiSurface::create(VADisplay display,
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
    status = vaCreateSurfaces(display, format, width, height,
                              &id, 1, surfAttribs, surfAttribNum);
    if (!checkVaapiStatus(status, "vaCreateSurfacesWithAttribute()"))
        return surface;

    for (int i = 0; i < surfAttribNum; i++) {
        if (surfAttribs[i].type == VASurfaceAttribExternalBufferDescriptor) {
            VASurfaceAttribExternalBuffers *surfAttribExtBuf
                =
                (VASurfaceAttribExternalBuffers *) surfAttribs[i].value.
                value.p;
            externalBufHandle = surfAttribExtBuf->buffers[0];
            break;
        }
    }
    surface.reset(new
                  VaapiSurface(display, id, chromaType, width, height,
                               externalBufHandle));
    return surface;
}

VaapiSurface::VaapiSurface(VADisplay display,
                           VASurfaceID id,
                           VaapiChromaType chromaType,
                           uint32_t width,
                           uint32_t height, uint32_t externalBufHandle)
:m_display(display), m_chromaType(chromaType), m_width(width),
m_height(height),m_externalBufHandle(externalBufHandle), m_ID(id),
m_derivedImage(NULL)
{

}

VaapiSurface::VaapiSurface(VADisplay display,
                           VaapiChromaType chromaType,
                           uint32_t width,
                           uint32_t height,
                           void *surfAttribArray, uint32_t surfAttribNum)
:m_display(display), m_chromaType(chromaType), m_width(width),
m_height(height)
{
    VAStatus status;
    uint32_t format, i;
    VASurfaceAttrib *surfAttribs = (VASurfaceAttrib *) surfAttribArray;

    switch (m_chromaType) {
    case VAAPI_CHROMA_TYPE_YUV420:
        format = VA_RT_FORMAT_YUV420;
        break;
    case VAAPI_CHROMA_TYPE_YUV422:
        format = VA_RT_FORMAT_YUV422;
        break;
    case VAAPI_CHROMA_TYPE_YUV444:
        format = VA_RT_FORMAT_YUV444;
        break;
    default:
        format = VA_RT_FORMAT_YUV420;
        break;
    }

    m_externalBufHandle = NULL;
    if (surfAttribs && surfAttribNum) {
        status = vaCreateSurfaces(m_display, format, width, height,
                                  &m_ID, 1, surfAttribs, surfAttribNum);

        for (i = 0; i < surfAttribNum; i++)
            if (surfAttribs[i].type ==
                VASurfaceAttribExternalBufferDescriptor) {
                VASurfaceAttribExternalBuffers *surfAttribExtBuf =
                    (VASurfaceAttribExternalBuffers *)
                    surfAttribs[i].value.value.p;
                m_externalBufHandle = surfAttribExtBuf->buffers[0];
            }
    } else {
        status = vaCreateSurfaces(m_display, format, width, height,
                                  &m_ID, 1, NULL, 0);
    }

    if (!checkVaapiStatus(status, "vaCreateSurfacesWithAttribute()"))
        return;

    m_derivedImage = NULL;
}

VaapiSurface::~VaapiSurface()
{
    VAStatus status;

    delete m_derivedImage;

    status = vaDestroySurfaces(m_display, &m_ID, 1);

    if (!checkVaapiStatus(status, "vaDestroySurfaces()"))
        WARNING("failed to destroy surface");

}

VaapiChromaType VaapiSurface::getChromaType(void)
{
    return m_chromaType;
}

VASurfaceID VaapiSurface::getID(void)
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

uint32_t VaapiSurface::getExtBufHandle(void)
{
    return m_externalBufHandle;
}

bool VaapiSurface::getImage(VaapiImage * image)
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

    status = vaGetImage(m_display, m_ID, 0, 0, width, height, imageID);

    if (!checkVaapiStatus(status, "vaGetImage()"))
        return false;

    return true;
}

bool VaapiSurface::putImage(VaapiImage * image)
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

    status = vaGetImage(m_display, m_ID, 0, 0, width, height, imageID);

    if (!checkVaapiStatus(status, "vaPutImage()"))
        return false;

    return true;
}

VaapiImage *VaapiSurface::getDerivedImage()
{
    VAImage va_image;
    VAStatus status;

    if (m_derivedImage)
        return m_derivedImage;

    va_image.image_id = VA_INVALID_ID;
    va_image.buf = VA_INVALID_ID;

    status = vaDeriveImage(m_display, m_ID, &va_image);
    if (!checkVaapiStatus(status, "vaDeriveImage()"))
        return NULL;

    m_derivedImage = new VaapiImage(m_display, &va_image);

    return m_derivedImage;
}

bool VaapiSurface::sync()
{
    VAStatus status;

    status = vaSyncSurface((VADisplay) m_display, (VASurfaceID) m_ID);

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

    status = vaQuerySurfaceStatus((VADisplay) m_display,
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
