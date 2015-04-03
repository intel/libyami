/*
 *  vaapipostprocess_base.cpp - basic class for vpp
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
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

#include "vaapipostprocess_base.h"
#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapiutils.h"

namespace YamiMediaCodec{

YamiStatus  VaapiPostProcessBase::setNativeDisplay(const NativeDisplay& display)
{
    return initVA(display);
}

YamiStatus VaapiPostProcessBase::initVA(const NativeDisplay& display)
{
    if (m_context) {
        ERROR("do not init va more than one time");
        return YAMI_FAIL;
    }
    if (display.type != NATIVE_DISPLAY_VA
        || !display.handle) {
        ERROR("vpp only support va display as NativeDisplay");
        return YAMI_FAIL;
    }
    m_display = VaapiDisplay::create(display);
    if (!m_display) {
        ERROR("failed to create display");
        return YAMI_DRIVER_FAIL;
    }

    ConfigPtr config = VaapiConfig::create(m_display, VAProfileNone, VAEntrypointVideoProc, NULL, 0);
    if (!config) {
        ERROR("failed to create config");
        return YAMI_NO_CONFIG;
    }

    //register WxH as 1x1 to make configure creation not fail.
    m_context = VaapiContext::create(config,1, 1,
                                     0, NULL, 0);
    if (!m_context) {
        ERROR("failed to create context");
        return YAMI_FAIL;
    }
    return YAMI_SUCCESS;
}

void VaapiPostProcessBase::cleanupVA()
{
    m_context.reset();
    m_display.reset();
}

VaapiPostProcessBase::~VaapiPostProcessBase()
{
    cleanupVA();
}

}
