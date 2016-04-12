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

#include "oclpostprocess_base.h"
#include "common/log.h"
#include "ocl/oclcontext.h"
#include "vaapi/VaapiUtils.h"
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
