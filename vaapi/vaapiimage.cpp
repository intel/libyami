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
#include "vaapisurface.h"
#include <stdlib.h>
#include <string.h>

namespace YamiMediaCodec{

ImageRawPtr VaapiImageRaw::create(const DisplayPtr& display, const ImagePtr& image, VideoDataMemoryType memoryType)
{
    ImageRawPtr raw;
    RealeaseCallback release;
    uintptr_t handle;
    VAStatus status;
    VAImagePtr& vaImage = image->m_image;
    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_POINTER || memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        void* data;
        status = vaMapBuffer(display->getID(), vaImage->buf, &data);
        release = vaUnmapBuffer;
        handle = (uintptr_t)data;
    } else {
        VABufferInfo bufferInfo;
        if (memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
        else if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        else
            ASSERT(0);

        status = vaAcquireBufferHandle(display->getID(), vaImage->buf, &bufferInfo);
        release = vaReleaseBufferHandle;
        handle = (uintptr_t)bufferInfo.handle;
    }
    if (!checkVaapiStatus(status, "VaapiImageRaw::create()"))
        return raw;
    raw.reset(new VaapiImageRaw(display, image, memoryType, handle, release));
    return raw;
}

VaapiImageRaw::VaapiImageRaw(const DisplayPtr& display, const ImagePtr& image,
    VideoDataMemoryType memoryType, uintptr_t handle, RealeaseCallback release)
    :m_display(display), m_image(image), m_handle(handle),
     m_memoryType(memoryType), m_release(release)
{

}

bool VaapiImageRaw::getHandle(intptr_t& handle, uint32_t offsets[3], uint32_t pitches[3])
{
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        ERROR("you need map to handle type, if you want get handle");
        return false;
    }
    VAImagePtr& image =  m_image->m_image;
    handle = m_handle;
    for (int i = 0; i < 3; i++) {
        offsets[i] = image->offsets[i];
        pitches[i] = image->pitches[i];
    }
    return true;
}

bool VaapiImageRaw::copyTo(void* dest, const uint32_t offsets[3], const uint32_t pitches[3])
{
    if (!dest)
        return false;
    VAImagePtr& image =  m_image->m_image;
    return copy((uint8_t*)dest, offsets, pitches,
        (uint8_t*)m_handle, image->offsets, image->pitches);
}

bool VaapiImageRaw::copyFrom(const void* src, const uint32_t offsets[3], const uint32_t pitches[3])
{
    if (!src)
        return false;
    VAImagePtr& image =  m_image->m_image;
    uint8_t* dest = reinterpret_cast<uint8_t*>(m_handle);
    return copy(dest, image->offsets, image->pitches,
        src, offsets, pitches);
}

bool VaapiImageRaw::copyFrom(const void* src, uint32_t size)
{
    if (!src || !size)
        return false;

    uint32_t width[3];
    uint32_t height[3];
    uint32_t offset[3];
    uint32_t off = 0;
    getPlaneResolution(width, height);
    for (int i = 0; i < N_ELEMENTS(offset); i++) {
        offset[i] = off;
        off += width[i] * height[i];
    }
    if (size != off)
        return false;
    VAImagePtr& image =  m_image->m_image;
    uint8_t* dest = reinterpret_cast<uint8_t*>(m_handle);
    return copy(dest, image->offsets, image->pitches,
        (uint8_t*)src, offset, width, width, height);
}

void VaapiImageRaw::getPlaneResolution(uint32_t width[3], uint32_t height[3])
{
    VAImagePtr& image = m_image->m_image;
    uint32_t fourcc = image->format.fourcc;
    switch (fourcc) {
        case VA_FOURCC_NV12:
        case VA_FOURCC_YV12:{
            int w = image->width;
            int h = image->height;
            width[0] = w;
            height[0] = h;
            if (fourcc == VA_FOURCC_NV12) {
                width[1]  = w + (w & 1);
                height[1] = (h + 1) >> 1;
                width[2] = height[2] = 0;
            } else {
                width[1] = width[2] = (w + 1) >> 1;
                height[1] = height[2] = (h + 1) >> 1;
            }
        }
        break;
        case VA_FOURCC_RGBX:
        case VA_FOURCC_RGBA:
        case VA_FOURCC_BGRX:
        case VA_FOURCC_BGRA: {
            width[0] = image->width * 4;
            height[0] = image->height;
            width[1] = width[2] = height[1] = height[2] = 0;
        }
        break;
        default:
            ASSERT(0 && "do not support this format");
    }
}

bool VaapiImageRaw::copy(uint8_t* destBase,
              const uint32_t destOffsets[3], const uint32_t destPitches[3],
              const uint8_t* srcBase,
              const uint32_t srcOffsets[3], const uint32_t srcPitches[3],
              const uint32_t width[3], const uint32_t height[3])
{
    for (int i = 0; i < 3; i++) {
        uint32_t w = width[i];
        uint32_t h = height[i];
        if (w) {
            if (w > destPitches[i] || w > srcPitches[i]) {
                ERROR("can't copy, plane = %d,  width = %d, srcPitch = %d, destPitch = %d",
                    i, w, srcPitches[i], destPitches[i]);
                return false;
            }
            const uint8_t* src = srcBase + srcOffsets[i];
            uint8_t* dest = destBase + destOffsets[i];


            for (int j = 0; j < h; j++) {
                memcpy(dest, src, w);
                src += srcPitches[i];
                dest += destPitches[i];
            }
        }
    }
    return true;

}

bool VaapiImageRaw::copy(uint8_t* destBase, const uint32_t destOffsets[3], const uint32_t destPitches[3],
    const uint8_t* srcBase, const uint32_t srcOffsets[3], const uint32_t srcPitches[3])
{
    ASSERT(srcBase && destBase);
    if (m_memoryType != VIDEO_DATA_MEMORY_TYPE_RAW_COPY)
        return false;

    uint32_t width[3];
    uint32_t height[3];
    getPlaneResolution(width,height);
    return copy(destBase, destOffsets, destPitches, srcBase, srcOffsets, srcPitches, width, height);
}

VaapiImageRaw::~VaapiImageRaw()
{
    VAImagePtr& image = m_image->m_image;
    VAStatus status = m_release(m_display->getID(), image->buf);
    checkVaapiStatus(status, "vaReleaseBufferHandle()");
}

VideoDataMemoryType VaapiImageRaw::getMemoryType()
{
    return m_memoryType;
}

ImagePtr VaapiImage::create(const DisplayPtr& display,
                           uint32_t format,
                           uint32_t width, uint32_t height)
{
    ImagePtr image;
    VAStatus status;

    if (!display || !width || !height)
        return image;

    DEBUG_FOURCC("create image with fourcc: ", format);
    const VAImageFormat *vaFormat = display->getVaFormat(format);
    if (!vaFormat) {
        ERROR("Create image failed, not supported fourcc");
        return image;
    }

    VAImagePtr vaImage(new VAImage);

    status = vaCreateImage(display->getID(), vaFormat, width, height, vaImage.get());
    if (status != VA_STATUS_SUCCESS ||
        vaImage->format.fourcc != vaFormat->fourcc) {
        ERROR("fourcc mismatch wated = 0x%x, got = 0x%x", vaFormat->fourcc, vaImage->format.fourcc);
        return image;
    }
    image.reset(new VaapiImage(display, vaImage));

    return image;
}

ImagePtr VaapiImage::derive(const SurfacePtr& surface)
{
    ImagePtr image;
    if (!surface)
        return image;

    DisplayPtr display = surface->getDisplay();
    VAImagePtr vaImage(new VAImage);

    VAStatus status = vaDeriveImage(display->getID(), surface->getID(), vaImage.get());
    if (!checkVaapiStatus(status, "vaDeriveImage()")) {
        return image;
    }
    image.reset(new VaapiImage(display, surface, vaImage));
    return image;

}

VaapiImage::VaapiImage(const DisplayPtr& display, const VAImagePtr& image)
    :m_display(display), m_image(image)
{

}

VaapiImage::VaapiImage(const DisplayPtr& display, const SurfacePtr& surface, const VAImagePtr& image)
    :m_display(display), m_surface(surface), m_image(image)
{

}

VaapiImage::~VaapiImage()
{
    VAStatus status;

    status = vaDestroyImage(m_display->getID(), m_image->image_id);

    if (!checkVaapiStatus(status, "vaDestoryImage()"))
        return;
}

ImageRawPtr VaapiImage::map(VideoDataMemoryType memoryType)
{
    ImageRawPtr raw = m_rawImage.lock();
    if (raw) {
        if (raw->getMemoryType() != memoryType) {
            ERROR("map image to different memory type, wanted %d, mapped = %d", memoryType, raw->getMemoryType());
            raw.reset();
        }
        return raw;
    }
    raw = VaapiImageRaw::create(m_display, shared_from_this(), memoryType);
    m_rawImage = raw;
    return raw;
}

#if 0
VaapiImageRaw *VaapiImage::map(VideoDataMemoryType memoryType)
{
    uint32_t i;
    void *data;
    VAStatus status;

    if (m_isMapped) {
        return &m_rawImage;
    }

    m_rawImage.format = m_image->format.fourcc;
    m_rawImage.width = m_image->width;
    m_rawImage.height = m_image->height;
    m_rawImage.numPlanes = m_image->num_planes;
    m_rawImage.size = m_image->data_size;
    for (i = 0; i < m_image->num_planes; i++) {
        m_rawImage.strides[i] = m_image->pitches[i];
    }

    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_POINTER || memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        status = vaMapBuffer(m_display->getID(), m_image->buf, &data);
        if (!checkVaapiStatus(status, "vaMapBuffer()"))
            return NULL;
        for (i = 0; i < m_image->num_planes; i++) {
            m_rawImage.handles[i] = (intptr_t) data + m_image->offsets[i];
        }
    } else {
        VABufferInfo bufferInfo;
        if (memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
        else if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
            bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
        else
            ASSERT(0);

        status = vaAcquireBufferHandle(m_display->getID(), m_image->buf, &bufferInfo);
        if (!checkVaapiStatus(status, "vaAcquireBufferHandle()"))
            return NULL;

       for (i = 0; i < m_image->num_planes; i++) {
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
        status = vaUnmapBuffer(m_display->getID(), m_image->buf);
        if (!checkVaapiStatus(status, "vaUnmapBuffer()"))
            return false;
    break;
    case VIDEO_DATA_MEMORY_TYPE_DRM_NAME:
    case VIDEO_DATA_MEMORY_TYPE_DMA_BUF:
        status = vaReleaseBufferHandle(m_display->getID(), m_image->buf);
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
#endif

uint32_t VaapiImage::getFormat()
{
    return m_image->format.fourcc;
}

VAImageID VaapiImage::getID()
{
    return m_image->image_id;
}

uint32_t VaapiImage::getWidth()
{
    return m_image->width;
}

uint32_t VaapiImage::getHeight()
{
    return m_image->height;
}
}
