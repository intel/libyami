/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#include "common/log.h"
#include "VideoPostProcessHost.h"
#include "vaapipostprocess_factory.h"

#if __BUILD_OCL_FILTERS__
#include "oclpostprocess_blender.h"
#include "oclpostprocess_mosaic.h"
#include "oclpostprocess_osd.h"
#include "oclpostprocess_transform.h"
#include "oclpostprocess_wireframe.h"

namespace YamiMediaCodec {
const bool OclPostProcessBlender::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessBlender>(YAMI_VPP_OCL_BLENDER);
const bool OclPostProcessMosaic::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessMosaic>(YAMI_VPP_OCL_MOSAIC);
const bool OclPostProcessOsd::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessOsd>(YAMI_VPP_OCL_OSD);
const bool OclPostProcessTransform::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessTransform>(YAMI_VPP_OCL_TRANSFORM);
const bool OclPostProcessWireframe::s_registered =
    VaapiPostProcessFactory::register_<OclPostProcessWireframe>(YAMI_VPP_OCL_WIREFRAME);
}
#endif

#include "vaapipostprocess_scaler.h"
namespace YamiMediaCodec {
const bool VaapiPostProcessScaler::s_registered =
    VaapiPostProcessFactory::register_<VaapiPostProcessScaler>(YAMI_VPP_SCALER);
}

using namespace YamiMediaCodec;

extern "C" {

IVideoPostProcess *createVideoPostProcess(const char *mimeType)
{
    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }

    VaapiPostProcessFactory::Type vpp =
        VaapiPostProcessFactory::create(mimeType);

    if (!vpp)
        ERROR("Failed to create vpp for mimeType: '%s'", mimeType);
    else
        INFO("Created vpp for mimeType: '%s'", mimeType);

    return vpp;
}

void releaseVideoPostProcess(IVideoPostProcess * p)
{
    delete p;
}

std::vector<std::string> getVideoPostProcessMimeTypes()
{
    return VaapiPostProcessFactory::keys();
}

} // extern "C"
