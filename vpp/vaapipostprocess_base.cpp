/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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

#include "vaapipostprocess_base.h"
#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/VaapiUtils.h"

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

YamiStatus VaapiPostProcessBase::queryVideoProcFilterCaps(
    VAProcFilterType filterType, void* filterCaps, uint32_t* numFilterCaps)
{
    if (!filterCaps)
        return YAMI_INVALID_PARAM;

    if (!m_context) {
        ERROR("no va context");
        return YAMI_FAIL;
    }

    uint32_t tmp = 1;
    if (!numFilterCaps)
        numFilterCaps = &tmp;
    VAStatus status = vaQueryVideoProcFilterCaps(m_display->getID(), m_context->getID(),
        filterType, filterCaps, numFilterCaps);
    if (!checkVaapiStatus(status, "vaQueryVideoProcFilterCaps") || !*numFilterCaps) {
        return YAMI_UNSUPPORTED;
    }
    return YAMI_SUCCESS;
}

VaapiPostProcessBase::~VaapiPostProcessBase()
{
    cleanupVA();
}

}
