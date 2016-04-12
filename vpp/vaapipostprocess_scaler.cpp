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

#include "vaapipostprocess_scaler.h"
#include "vaapivpppicture.h"
#include "vaapipostprocess_factory.h"
#include "common/log.h"
#include <va/va_vpp.h>

namespace YamiMediaCodec{

static bool fillRect(VARectangle& vaRect, const VideoRect& rect)
{
    vaRect.x = rect.x;
    vaRect.y = rect.y;
    vaRect.width = rect.width;
    vaRect.height = rect.height;
    return rect.x || rect.y || rect.width || rect.height;
}

static void copyVideoFrameMeta(const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest)
{
    dest->timeStamp = src->timeStamp;
    dest->flags = src->flags;
}

//android csc need us set color standard
VAProcColorStandardType fourccToColorStandard(uint32_t fourcc)
{
    if (fourcc == YAMI_FOURCC_RGBX
        || fourcc == YAMI_FOURCC_RGBA
        || fourcc == YAMI_FOURCC_BGRX
        || fourcc == YAMI_FOURCC_BGRA) {
        //we should return VAProcColorStandardSRGB here
        //but it only exist on libva staging. so we return None
        //side effect is android libva will print error info when we convert it to RGB
        //we will change this after staing merged to master
        //return VAProcColorStandardSRGB;
        return VAProcColorStandardNone;
    }
    if (fourcc == YAMI_FOURCC_NV12)
        return VAProcColorStandardBT601;
    return VAProcColorStandardBT601;
}

YamiStatus
VaapiPostProcessScaler::process(const SharedPtr<VideoFrame>& src,
                                const SharedPtr<VideoFrame>& dest)
{
    if (!m_context) {
        ERROR("NO context for scaler");
        return YAMI_FAIL;
    }
    if (!src || !dest) {
        return YAMI_INVALID_PARAM;
    }
    copyVideoFrameMeta(src, dest);
    SurfacePtr surface(new VaapiSurface(dest));
    VaapiVppPicture picture(m_context, surface);
    VAProcPipelineParameterBuffer* vppParam;
    if (!picture.editVppParam(vppParam))
        return YAMI_OUT_MEMORY;
    VARectangle srcCrop, destCrop;
    if (fillRect(srcCrop, src->crop))
        vppParam->surface_region = &srcCrop;
    vppParam->surface = (VASurfaceID)src->surface;
    vppParam->surface_color_standard = fourccToColorStandard(src->fourcc);

    if (fillRect(destCrop, dest->crop))
        vppParam->output_region = &destCrop;
    vppParam->output_color_standard = fourccToColorStandard(dest->fourcc);
    return picture.process() ? YAMI_SUCCESS : YAMI_FAIL;
}

const bool VaapiPostProcessScaler::s_registered =
    VaapiPostProcessFactory::register_<VaapiPostProcessScaler>(YAMI_VPP_SCALER);

}
