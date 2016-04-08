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
#include "vaapivpppicture.h"
#include "common/log.h"

namespace YamiMediaCodec{
VaapiVppPicture::VaapiVppPicture(const ContextPtr& context,
                                 const SurfacePtr & surface)
:VaapiPicture(context, surface, 0)
{
}

bool VaapiVppPicture::editVppParam(VAProcPipelineParameterBuffer*& vppParm)
{
    return editObject(m_vppParam, VAProcPipelineParameterBufferType, vppParm);
}

bool VaapiVppPicture::process()
{
    return render();
}

bool VaapiVppPicture::doRender()
{
    RENDER_OBJECT(m_vppParam);
    return true;
}

}
