/*
 *  oclpostprocess_base.cpp - base class for opencl based vpp
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

#include "oclpostprocess_base.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vaapi/vaapiutils.h"
#include <va/va_drmcommon.h>

namespace YamiMediaCodec{

OclPostProcessBase::OclPostProcessBase()
    :m_kernel(0)
{
}

YamiStatus OclPostProcessBase::setNativeDisplay(const NativeDisplay& display)
{
    if (display.type != NATIVE_DISPLAY_VA
        || !display.handle) {
        ERROR("vpp only support va display as NativeDisplay");
        return YAMI_INVALID_PARAM;
    }
    m_display = (VADisplay)display.handle;
    return YAMI_SUCCESS;
}

YamiStatus OclPostProcessBase::ensureContext(const char* kernalName)
{
    if (m_kernel)
        return YAMI_SUCCESS;
    m_context = OclContext::create();
    if (!m_context || !m_context->createKernel(kernalName,m_kernel))
        return YAMI_DRIVER_FAIL;
    return YAMI_SUCCESS;
}

SharedPtr<OclPostProcessBase::OclImage>
OclPostProcessBase::createCLImage(const SharedPtr<VideoFrame>& frame,
                                  const cl_image_format& fmt)
{
    SharedPtr<OclImage> clImage(new OclImage(m_display));
    VASurfaceID surfaceId = (VASurfaceID)frame->surface;
    VABufferInfo bufferInfo;
    cl_import_image_info_intel importInfo;
    uint32_t height[3], i;

    VAImage image;
    if (!checkVaapiStatus(vaDeriveImage(m_display, surfaceId, &image), "DeriveImage")) {
        clImage.reset();
        goto done;
    }

    clImage->m_imageId = image.image_id;
    clImage->m_bufId = image.buf;
    bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    if (!checkVaapiStatus(vaAcquireBufferHandle(m_display, image.buf, &bufferInfo),
        "AcquireBufferHandle")) {
        clImage.reset();
        goto done;
    }

    switch (image.format.fourcc) {
    case VA_FOURCC_RGBA:
        height[0] = image.height;
        break;
    case VA_FOURCC_NV12:
        height[0] = image.height;
        height[1] = image.height / 2;
        break;
    default:
        ERROR("unsupported format");
        clImage.reset();
        goto done;
    }
    clImage->m_format = image.format.fourcc;

    clImage->m_numPlanes = image.num_planes;
    for (i = 0; i < image.num_planes; i++) {
        importInfo.fd = bufferInfo.handle;
        importInfo.type = CL_MEM_OBJECT_IMAGE2D;
        importInfo.fmt.image_channel_order = fmt.image_channel_order;
        importInfo.fmt.image_channel_data_type = fmt.image_channel_data_type;
        importInfo.row_pitch = image.pitches[i];
        importInfo.offset = image.offsets[i];
        importInfo.width = image.width;
        importInfo.height = height[i];
        importInfo.size = importInfo.row_pitch * importInfo.height;
        if (YAMI_SUCCESS != m_context->createImageFromFdIntel(&importInfo, &clImage->m_mem[i])) {
            clImage.reset();
            goto done;
        }
    }

done:
    return clImage;
}

uint32_t OclPostProcessBase::getPixelSize(const cl_image_format& fmt)
{
    uint32_t size = 0;

    switch (fmt.image_channel_order) {
    case CL_R:
    case CL_A:
    case CL_Rx:
        size = 1;
        break;
    case CL_RG:
    case CL_RA:
    case CL_RGx:
        size = 2;
        break;
    case CL_RGB:
    case CL_RGBx:
        size = 3;
        break;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
        size = 4;
        break;
    case CL_INTENSITY:
    case CL_LUMINANCE:
        size = 1;
        break;
    default:
        ERROR("invalid image channel order: %u", fmt.image_channel_order);
        return 0;
    }

    switch (fmt.image_channel_data_type) {
    case CL_UNORM_INT8:
    case CL_SNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
        size *= 1;
        break;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_UNORM_SHORT_565:
    case CL_UNORM_SHORT_555:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
        size *= 2;
        break;
    case CL_UNORM_INT24:
        size *= 3;
        break;
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_UNORM_INT_101010:
    case CL_FLOAT:
        size *= 4;
        break;
    default:
        ERROR("invalid image channel data type: %d", fmt.image_channel_data_type);
        return 0;
    }

    return size;
}

OclPostProcessBase::~OclPostProcessBase()
{
    if (m_kernel) {
        checkOclStatus(clReleaseKernel(m_kernel), "ReleaseKernel");
    }
}

OclPostProcessBase::OclImage::OclImage(VADisplay d)
    : m_numPlanes(0), m_display(d), m_imageId(VA_INVALID_ID), m_bufId(VA_INVALID_ID)
{
     memset(m_mem, 0, sizeof(m_mem));
}

OclPostProcessBase::OclImage::~OclImage()
{
    for (int i = 0; i < m_numPlanes; i++)
        checkOclStatus(clReleaseMemObject(m_mem[i]), "ReleaseMemObject");

    checkVaapiStatus(vaReleaseBufferHandle(m_display, m_bufId), "ReleaseBufferHandle");
    checkVaapiStatus(vaDestroyImage(m_display, m_imageId), "DestroyImage");
}
}
