/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifndef vaapicontext_h
#define vaapicontext_h

#include "common/NonCopyable.h"
#include "VideoCommonDefs.h"
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
