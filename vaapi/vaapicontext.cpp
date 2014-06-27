/*
 *  vaapicontext.cpp - abstract for VaContext
 *
 *  Copyright (C) 2014 Intel Corporation
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
#include "vaapi/vaapicontext.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapiutils.h"

ConfigPtr VaapiConfig::create(const DisplayPtr& display,
                                     VAProfile profile, VAEntrypoint entry,
                                     VAConfigAttrib *attribList, int numAttribs)
{
    ConfigPtr ret;
    if (!display)
        return ret;
    VAStatus vaStatus;
    VAConfigID config;
    vaStatus = vaCreateConfig(display->getID(), profile, entry, attribList, numAttribs, &config);

    // VAProfileH264Baseline is super profile for VAProfileH264ConstrainedBaseline
    // old i965 driver incorrectly claims supporting VAProfileH264Baseline, but not VAProfileH264ConstrainedBaseline
    if (vaStatus == VA_STATUS_ERROR_UNSUPPORTED_PROFILE
        && profile == VAProfileH264ConstrainedBaseline
        && entry == VAEntrypointVLD)
        vaStatus = vaCreateConfig(display->getID(),
                                  VAProfileH264Baseline,
                                  VAEntrypointVLD, attribList, numAttribs, &config);

    if (!checkVaapiStatus(vaStatus, "vaCreateConfig "))
        return ret;
    ret.reset(new VaapiConfig(display, config));
    return ret;
}

VaapiConfig::VaapiConfig(const DisplayPtr& display, VAConfigID config)
:m_display(display), m_config(config)
{
}

VaapiConfig::~VaapiConfig()
{
    vaDestroyConfig(m_display->getID(), m_config);
}

ContextPtr VaapiContext::create(const ConfigPtr& config,
                                       int width,int height,int flag,
                                       VASurfaceID *render_targets,
                                       int num_render_targets)
{
    ContextPtr ret;
    if (!config) {
        ERROR("No display");
        return ret;
    }
    VAStatus vaStatus;
    VAContextID context;
    vaStatus = vaCreateContext(config->m_display->getID(), config->m_config,
                               width, height, flag,
                               render_targets, num_render_targets, &context);
    if (!checkVaapiStatus(vaStatus, "vaCreateContext "))
        return ret;
    ret.reset(new VaapiContext(config, context));
    return ret;
}

VaapiContext::VaapiContext(const ConfigPtr& config, VAContextID context)
:m_config(config), m_context(context)
{
}

VaapiContext::~VaapiContext()
{
    vaDestroyContext(m_config->m_display->getID(), m_context);
}
