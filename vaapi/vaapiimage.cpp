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
#include "log.h"
#include "vaapiutils.h"
#include <stdlib.h>

const VAImageFormat *VaapiImage::getVaFormat(VaapiImageFormat format)
{
    const VaapiImageFormatMap *map = NULL;
    for (map = vaapiImageFormats; map->format; map++) {
        if (map->format == format)
            return &map->vaFormat;
    }
    return NULL;
}

VaapiImage::VaapiImage(VADisplay display,
                       VaapiImageFormat format,
                       uint32_t width, uint32_t height)
{
    VAStatus status;
    VAImageFormat *vaFormat;

    m_display = display;
    m_format = format;
    m_width = width;
    m_height = height;
    m_isMapped = false;

    vaFormat = (VAImageFormat *) getVaFormat(format);
    if (!vaFormat) {
        ERROR("Create image failed, not supported fourcc");
        return;
    }

    status =
        vaCreateImage(m_display, vaFormat, m_width, m_height, &m_image);

    if (status != VA_STATUS_SUCCESS ||
        m_image.format.fourcc != vaFormat->fourcc) {
        ERROR("Create image failed");
        return;
    }
}

VaapiImage::VaapiImage(VADisplay display, VAImage * image)
{
    VAStatus status;

    m_display = display;
    m_width = image->width;
    m_height = image->height;
    m_ID = image->image_id;
    m_isMapped = false;

    memcpy((void *) &m_image, (void *) image, sizeof(VAImage));
}

VaapiImage::~VaapiImage()
{
    VAStatus status;

    if (m_isMapped) {
        unmap();
        m_isMapped = false;
    }

    status = vaDestroyImage(m_display, m_image.image_id);

    if (!checkVaapiStatus(status, "vaDestoryImage()"))
        return;
}

VaapiImageRaw *VaapiImage::map()
{
    uint32_t i;
    void *data;
    VAStatus status;

    if (m_isMapped) {
        return &m_rawImage;
    }

    status = vaMapBuffer(m_display, m_image.buf, &data);

    if (!checkVaapiStatus(status, "vaMapBuffer()"))
        return NULL;

    m_rawImage.format = m_format;
    m_rawImage.width = m_width;
    m_rawImage.height = m_height;
    m_rawImage.numPlanes = m_image.num_planes;
    m_rawImage.size = m_image.data_size;

    for (i = 0; i < m_image.num_planes; i++) {
        m_rawImage.pixels[i] = (uint8_t *) data + m_image.offsets[i];
        m_rawImage.strides[i] = m_image.pitches[i];
    }
    m_isMapped = true;

    return &m_rawImage;
}

bool VaapiImage::unmap()
{
    VAStatus status;

    if (!m_isMapped)
        return true;

    status = vaUnmapBuffer(m_display, m_image.buf);
    if (!checkVaapiStatus(status, "vaUnmapBuffer()"))
        return false;

    m_isMapped = false;

    return true;
}

bool VaapiImage::isMapped()
{
    return m_isMapped;
}

VaapiImageFormat VaapiImage::getFormat()
{
    return m_format;
}

VAImageID VaapiImage::getID()
{
    return m_ID;
}

uint32_t VaapiImage::getWidth()
{
    return m_width;
}

uint32_t VaapiImage::getHeight()
{
    return m_height;
}
