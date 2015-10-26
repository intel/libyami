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

OclPostProcessBase::~OclPostProcessBase()
{
    if (m_kernel) {
        checkOclStatus(clReleaseKernel(m_kernel), "ReleaseKernel");
    }
}

}

