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

YamiStatus OclPostProcessBase::ensureContext(const char* name)
{
    if (m_kernels.size())
        return YAMI_SUCCESS;
    m_context = OclContext::create();
    if (!m_context
        || !m_context->createKernel(name, m_kernels)
        || !prepareKernels())
        return YAMI_DRIVER_FAIL;
    return YAMI_SUCCESS;
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

cl_kernel OclPostProcessBase::prepareKernel(const char* name)
{
    OclKernelMap::iterator it = m_kernels.find(name);
    if (it == m_kernels.end())
        return NULL;
    else
        return it->second;
}

OclPostProcessBase::~OclPostProcessBase()
{
    if (m_context)
        m_context->releaseKernel(m_kernels);
}
}
