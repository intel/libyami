/*
 *  vaapipostprocess_scaler.cpp - scaler and color space convert
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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
    SurfacePtr surface(new VaapiSurface(m_display,(VASurfaceID)dest->surface));
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
