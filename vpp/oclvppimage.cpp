/*
 *  oclvppimage.cpp - image wrappers for opencl vpp modules
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Jia Meng<jia.meng@intel.com>
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

#include "common/log.h"
#include "ocl/oclcontext.h"
#include "oclvppimage.h"
#include "vaapi/vaapiutils.h"
#include <va/va_drmcommon.h>

namespace YamiMediaCodec {

SharedPtr<OclVppRawImage> OclVppRawImage::create(
    VADisplay d,
    const SharedPtr<VideoFrame>& f)
{
    SharedPtr<OclVppRawImage> image(new OclVppRawImage(d, f));
    if (image->init())
        return image;
    ERROR("Failed to create OclVppRawImage");
    image.reset();
    return image;
}

bool OclVppRawImage::init()
{
    VASurfaceID surfaceId = (VASurfaceID)m_frame->surface;
    if (!checkVaapiStatus(vaDeriveImage(m_display, surfaceId, &m_image), "DeriveImage"))
        return false;

    uint8_t* buf = 0;
    if (!checkVaapiStatus(vaMapBuffer(m_display, m_image.buf, (void**)&buf), "vaMapBuffer"))
        return false;

    for (uint32_t n = 0; n < m_image.num_planes; n++) {
        m_mem[n] = buf + m_image.offsets[n];
    }
    return true;
}

OclVppRawImage::~OclVppRawImage()
{
    checkVaapiStatus(vaUnmapBuffer(m_display, m_image.buf), "ReleaseBufferHandle");
    checkVaapiStatus(vaDestroyImage(m_display, m_image.image_id), "DestroyImage");
}

SharedPtr<OclVppCLImage> OclVppCLImage::create(
    VADisplay d,
    const SharedPtr<VideoFrame>& f,
    const SharedPtr<OclContext> ctx,
    const cl_image_format& fmt)
{
    SharedPtr<OclVppCLImage> image(new OclVppCLImage(d, f, ctx, fmt));
    if (image->init())
        return image;
    ERROR("Failed to create OclVppCLImage");
    image.reset();
    return image;
}

bool OclVppCLImage::init()
{
    VASurfaceID surfaceId = (VASurfaceID)m_frame->surface;
    if (!checkVaapiStatus(vaDeriveImage(m_display, surfaceId, &m_image), "DeriveImage"))
        return false;

    VABufferInfo bufferInfo;
    bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    if (!checkVaapiStatus(vaAcquireBufferHandle(m_display, m_image.buf, &bufferInfo),
            "AcquireBufferHandle"))
        return false;

    cl_import_image_info_intel importInfo;
    uint32_t height[3];
    switch (m_image.format.fourcc) {
    case VA_FOURCC_RGBA:
        height[0] = m_image.height;
        break;
    case VA_FOURCC_NV12:
        height[0] = m_image.height;
        height[1] = m_image.height / 2;
        break;
    default:
        ERROR("unsupported format");
        return false;
    }

    for (uint32_t n = 0; n < m_image.num_planes; n++) {
        importInfo.fd = bufferInfo.handle;
        importInfo.type = CL_MEM_OBJECT_IMAGE2D;
        importInfo.fmt.image_channel_order = m_fmt.image_channel_order;
        importInfo.fmt.image_channel_data_type = m_fmt.image_channel_data_type;
        importInfo.row_pitch = m_image.pitches[n];
        importInfo.offset = m_image.offsets[n];
        importInfo.width = m_image.width;
        importInfo.height = height[n];
        importInfo.size = importInfo.row_pitch * importInfo.height;
        if (YAMI_SUCCESS != m_context->createImageFromFdIntel(&importInfo, &m_mem[n])) {
            ERROR("createImageFromFdIntel failed");
            return false;
        }
    }

    return true;
}

OclVppCLImage::~OclVppCLImage()
{
    for (uint32_t n = 0; n < m_image.num_planes; n++)
        checkOclStatus(clReleaseMemObject(m_mem[n]), "ReleaseMemObject");

    checkVaapiStatus(vaReleaseBufferHandle(m_display, m_image.buf), "ReleaseBufferHandle");
    checkVaapiStatus(vaDestroyImage(m_display, m_image.image_id), "DestroyImage");
}
}
