/*
 *  vaapiimage.cpp - VA image abstraction
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

#include "vaapiimage.h"
#include "common/log.h"
#include "vaapiutils.h"
#include <stdlib.h>
#include <string.h>

namespace YamiMediaCodec{
ImagePtr VaapiImage::create(DisplayPtr display,
                           uint32_t format,
                           uint32_t width, uint32_t height)
{
    ImagePtr image;
    VAStatus status;
    VAImageFormat *vaFormat;
    VAImage vaImage;

    ASSERT(display && width && height);
    DEBUG_FOURCC("create image with fourcc: ", format);
    vaFormat = display->getVaFormat(format);
    if (!vaFormat) {
        ERROR("Create image failed, not supported fourcc");
        return image;
    }

    status = vaCreateImage(display->getID(), vaFormat, width, height, &vaImage);
    if (status != VA_STATUS_SUCCESS ||
        vaImage.format.fourcc != vaFormat->fourcc) {
        ERROR("Create image failed");
        return image;
    }

    image.reset(new VaapiImage(display, &vaImage));
    return image;
}

ImagePtr VaapiImage::create(DisplayPtr display, VAImage * vaImage)
{
    ImagePtr image;
    image.reset(new VaapiImage(display, vaImage));
    return image;
}

VaapiImage::VaapiImage(DisplayPtr display, VAImage * image)
    :m_isMapped(false)
{
    ASSERT(display && image);
    m_display = display;
    memcpy((void *) &m_image, (void *) image, sizeof(VAImage));
}

VaapiImage::~VaapiImage()
{
    VAStatus status;

    if (m_isMapped) {
        unmap();
        m_isMapped = false;
    }

    status = vaDestroyImage(m_display->getID(), m_image.image_id);

    if (!checkVaapiStatus(status, "vaDestoryImage()"))
        return;
}

VaapiImageRaw *VaapiImage::map(VideoDataMemoryType memoryType)
{
    uint32_t i;
    void *data;
    VAStatus status;

    if (m_isMapped) {
        return &m_rawImage;
    }

    m_rawImage.format = m_image.format.fourcc;
    m_rawImage.width = m_image.width;
    m_rawImage.height = m_image.height;
    m_rawImage.numPlanes = m_image.num_planes;
    m_rawImage.size = m_image.data_size;
    for (i = 0; i < m_image.num_planes; i++) {
        m_rawImage.strides[i] = m_image.pitches[i];
    }

    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_POINTER || memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        status = vaMapBuffer(m_display->getID(), m_image.buf, &data);
        if (!checkVaapiStatus(status, "vaMapBuffer()"))
            return NULL;
        for (i = 0; i < m_image.num_planes; i++) {
            m_rawImage.handles[i] = (intptr_t) data + m_image.offsets[i];
        }
    } else {
        VABufferInfo bufferInfo;
        if (memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
        else if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        else
            ASSERT(0);

        status = vaAcquireBufferHandle(m_display->getID(), m_image.buf, &bufferInfo);
        if (!checkVaapiStatus(status, "vaAcquireBufferHandle()"))
            return NULL;

       for (i = 0; i < m_image.num_planes; i++) {
           m_rawImage.handles[i] = (intptr_t) bufferInfo.handle;
        }
    }
    m_rawImage.memoryType = memoryType;
    m_isMapped = true;

    return &m_rawImage;
}

bool VaapiImage::unmap()
{
    VAStatus status;

    if (!m_isMapped)
        return true;

    switch(m_rawImage.memoryType) {
    case VIDEO_DATA_MEMORY_TYPE_RAW_POINTER:
    case VIDEO_DATA_MEMORY_TYPE_RAW_COPY:
        status = vaUnmapBuffer(m_display->getID(), m_image.buf);
        if (!checkVaapiStatus(status, "vaUnmapBuffer()"))
            return false;
    break;
    case VIDEO_DATA_MEMORY_TYPE_DRM_NAME:
    case VIDEO_DATA_MEMORY_TYPE_DMA_BUF:
        status = vaReleaseBufferHandle(m_display->getID(), m_image.buf);
        if (!checkVaapiStatus(status, "vaReleaseBufferHandle()"))
            return false;
    break;
    default:
        ASSERT(0);
    break;
    }

    m_isMapped = false;
    return true;
}

bool VaapiImage::isMapped()
{
    return m_isMapped;
}

uint32_t VaapiImage::getFormat()
{
    return m_image.format.fourcc;
}

VAImageID VaapiImage::getID()
{
    return m_image.image_id;
}

uint32_t VaapiImage::getWidth()
{
    return m_image.width;
}

uint32_t VaapiImage::getHeight()
{
    return m_image.height;
}
}
