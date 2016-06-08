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
#include "vaapi/vaapicontext.h"

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

VaapiPostProcessScaler::VaapiPostProcessScaler()
{
    memset(&m_vppContext, 0, sizeof(m_vppContext));
    memset(&m_vppParam, 0, sizeof(m_vppParam));
}

YamiStatus VaapiPostProcessScaler::setParameters(VppParamType type, void* vppParam)
{
    YamiStatus status = YAMI_INVALID_PARAM;

    switch (type) {
    case VppParamTypeProcFilter: {
        VPPFilterParameters* param = (VPPFilterParameters*)vppParam;
        if (param->size == sizeof(VPPFilterParameters)) {
            PARAMETER_ASSIGN(m_vppParam, *param);
            status = YAMI_SUCCESS;
        }
    } break;
    default:
        status = VaapiPostProcessBase::setParameters(type, vppParam);
        break;
    }

    return status;
}

bool VaapiPostProcessScaler::EnableDeinterlace(VaapiVppPicture *picture)
{
    VAProcFilterCapDeinterlacing deinterlacing_caps[VAProcDeinterlacingCount];
    unsigned int num_deinterlacing_caps = VAProcDeinterlacingCount;

    bool ret = picture->queryProcFilter(VAProcFilterDeinterlacing,&deinterlacing_caps,&num_deinterlacing_caps);

    if (!ret || num_deinterlacing_caps == 0) {
        return false;
    }

    for (unsigned int j = 0; j < num_deinterlacing_caps; j++) {
        VAProcFilterCapDeinterlacing * const cap = &deinterlacing_caps[j];
        if (cap->type == m_vppParam.deinterlace_algorithm) {
            break;
        }
    }

    m_vppContext.deint_param.type = VAProcFilterDeinterlacing;
	m_vppContext.deint_param.algorithm = (VAProcDeinterlacingType)m_vppParam.deinterlace_algorithm;
    VAProcFilterParameterBufferDeinterlacing *deint_param = &(m_vppContext.deint_param);
    m_vppContext.deinterlace_filter_buffer_id = picture->editDeinterlaceParam(deint_param);
    m_vppContext.vpp_filters[m_vppContext.num_vpp_filter_used++] = m_vppContext.deinterlace_filter_buffer_id;

    return true;
}

bool VaapiPostProcessScaler::EnableDenoise(VaapiVppPicture *picture)
{
    VAProcFilterCap denoise_caps = {{0}};
    unsigned int num_denoise_caps = 1;

    bool ret = picture->queryProcFilter(VAProcFilterNoiseReduction,&denoise_caps,&num_denoise_caps);

    if (!ret || num_denoise_caps == 0) {
        printf("Driver doesn't support denoise\n");
        return false;
    }

    if (m_vppParam.denoising_value < denoise_caps.range.min_value ||
        m_vppParam.denoising_value > denoise_caps.range.max_value) {
        // Input value out of range
        printf("Input denoising value %d is invalid, range is [%f,%f]",
                m_vppParam.denoising_value,
                denoise_caps.range.min_value,
                denoise_caps.range.max_value);
        return false;
    }

    m_vppContext.denoise_param.type  = VAProcFilterNoiseReduction;
    m_vppContext.denoise_param.value = m_vppParam.denoising_value;
    VAProcFilterParameterBuffer *denoise_param = &(m_vppContext.denoise_param);
    m_vppContext.denoise_filter_buffer_id = picture->editProcFilterParam(denoise_param);
	m_vppContext.vpp_filters[m_vppContext.num_vpp_filter_used++] = m_vppContext.denoise_filter_buffer_id;

    return true;
}

bool VaapiPostProcessScaler::EnableSharp(VaapiVppPicture *picture)
{
    VAProcFilterCap sharp_caps = {{0}};
    unsigned int num_sharp_caps = 1;

    bool ret = picture->queryProcFilter(VAProcFilterSharpening,&sharp_caps,&num_sharp_caps);
    if (!ret || num_sharp_caps == 0) {
        printf("Driver doesn't support sharp\n");
        return false;
    }

    if (m_vppParam.sharp_value < sharp_caps.range.min_value ||
        m_vppParam.sharp_value> sharp_caps.range.max_value) {
        // Input value out of range
        printf("Input sharp value %d is invalid, range is [%f,%f]",
                m_vppParam.sharp_value,
                sharp_caps.range.min_value,
                sharp_caps.range.max_value);
        return false;
    }

    m_vppContext.sharp_param.type  = VAProcFilterSharpening;
    m_vppContext.sharp_param.value = m_vppParam.sharp_value;
    VAProcFilterParameterBuffer *sharp_param = &(m_vppContext.sharp_param);
    m_vppContext.sharp_filter_buffer_id = picture->editProcFilterParam(sharp_param);	
    m_vppContext.vpp_filters[m_vppContext.num_vpp_filter_used++] = m_vppContext.sharp_filter_buffer_id;

    return true;
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

    memset(&m_vppContext, 0, sizeof(m_vppContext));

    if (m_vppParam.deinterlace_algorithm != VAProcDeinterlacingNone) {
        if (!EnableDeinterlace(&picture)) {
            printf("Deinterlace altorithm %d is invalid\n", m_vppParam.deinterlace_algorithm);
        }
    }

    if (m_vppParam.denoising_value) {
        if (!EnableDenoise(&picture)) {
            printf("Denoise value %d is invalid\n", m_vppParam.denoising_value);
        }
    }

    if (m_vppParam.sharp_value) {
        if (!EnableSharp(&picture)) {
            printf("Sharpness value %d is invalid\n", m_vppParam.sharp_value);
        }
    }

    VARectangle srcCrop, destCrop;
    if (fillRect(srcCrop, src->crop))
        m_vppContext.pipeline_param.surface_region = &srcCrop;
    m_vppContext.pipeline_param.surface = (VASurfaceID)src->surface;
    m_vppContext.pipeline_param.surface_color_standard = fourccToColorStandard(src->fourcc);

    if (fillRect(destCrop, dest->crop))
        m_vppContext.pipeline_param.output_region = &destCrop;
    m_vppContext.pipeline_param.output_color_standard = fourccToColorStandard(dest->fourcc);

    m_vppContext.pipeline_param.filters = m_vppContext.vpp_filters;
    m_vppContext.pipeline_param.num_filters = m_vppContext.num_vpp_filter_used;
    VAProcPipelineParameterBuffer* vppParam;
    vppParam = &(m_vppContext.pipeline_param);
    bool ret = picture.editVppParam(vppParam);
    if (!ret) {
        printf("Error: Fail to create pipeline buffer!\n");
        return YAMI_FAIL;
    }

    picture.process();	
    return YAMI_SUCCESS;
}

const bool VaapiPostProcessScaler::s_registered =
    VaapiPostProcessFactory::register_<VaapiPostProcessScaler>(YAMI_VPP_SCALER);

}
