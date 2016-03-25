/*
 *  vaapidisplay.h - abstract for VaContext
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

#ifndef vaapicontext_h
#define vaapicontext_h

#include "interface/VideoCommonDefs.h"
#include "vaapi/vaapiptrs.h"
#include <va/va.h>

namespace YamiMediaCodec{
class VaapiConfig
{
friend class VaapiContext;
public:
    static ConfigPtr create(const DisplayPtr&, VAProfile, VAEntrypoint,
                    VAConfigAttrib *attribList, int numAttribs);
    ~VaapiConfig();
private:
    VaapiConfig(const DisplayPtr&, VAConfigID);
    DisplayPtr m_display;
    VAConfigID  m_config;
    DISALLOW_COPY_AND_ASSIGN(VaapiConfig);
};

class VaapiContext
{
public:
    static ContextPtr create(const ConfigPtr&,
                      int width,int height,int flag,
                      VASurfaceID *render_targets,
                      int num_render_targets);
    VAContextID getID() const { return m_context; }
    DisplayPtr getDisplay() const { return m_config->m_display; }

    ~VaapiContext();
private:
    VaapiContext(const ConfigPtr&,  VAContextID);
    ConfigPtr m_config;
    VAContextID m_context;
    DISALLOW_COPY_AND_ASSIGN(VaapiContext);
};
}
#endif
