/*
 * Copyright (C) 2013-2016 Intel Corporation. All rights reserved.
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

#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapivpppicture.h"
#include "common/log.h"
#include <va/va_vpp.h>

namespace YamiMediaCodec {

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
    m_denoise.level = DENOISE_LEVEL_NONE;
    m_sharpening.level = SHARPENING_LEVEL_NONE;
}

bool VaapiPostProcessScaler::getFilters(std::vector<VABufferID>& filters)
{
    if (m_denoise.filter) {
        filters.push_back(m_denoise.filter->getID());
    }
    if (m_sharpening.filter) {
        filters.push_back(m_sharpening.filter->getID());
    }
    if (m_deinterlace.filter) {
        filters.push_back(m_deinterlace.filter->getID());
    }

    for (ColorBalanceMapItr itr = m_colorBalance.begin(); itr != m_colorBalance.end(); itr++) {
        if (itr->second.filter) {
            filters.push_back(itr->second.filter->getID());
        }
    }
    return !filters.empty();
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
    if(src->crop.width & 0x01 || src->crop.height & 0x01){
        ERROR("unsupported odd resolution");
        return YAMI_FAIL;
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

    std::vector<VABufferID> filters;
    if (getFilters(filters)) {
        vppParam->filters = &filters[0];
        vppParam->num_filters = (unsigned int)filters.size();
    }

    return picture.process() ? YAMI_SUCCESS : YAMI_FAIL;
}

bool VaapiPostProcessScaler::mapToRange(
    float& value, float min, float max,
    int32_t level, int32_t minLevel, int32_t maxLevel)
{
    if (minLevel >= maxLevel) {
        ERROR("minLevel(%d) >= maxLevel(%d)", minLevel, maxLevel);
        return false;
    }
    if (level > maxLevel || level < minLevel) {
        ERROR("level(%d) not in the range[minLevel(%d), maxLevel(%d)]", level, minLevel, maxLevel);
        return false;
    }
    if (min >= max) {
        ERROR("min(%f) >= max(%f)", min, max);
        return false;
    }

    value = min + (max - min) / (maxLevel - minLevel) * level;

    return true;
}

bool VaapiPostProcessScaler::mapToRange(float& value, int32_t level,
    int32_t minLevel, int32_t maxLevel,
    VAProcFilterType filterType, SharedPtr<VAProcFilterCap>& caps)
{
    if (!caps) {
        //query from libva
        YamiStatus status;
        caps.reset(new VAProcFilterCap);
        status = queryVideoProcFilterCaps(filterType, caps.get());
        if (status != YAMI_SUCCESS) {
            caps.reset();
            return false;
        }
    }
    return mapToRange(value, caps->range.min_value, caps->range.max_value, level, minLevel, maxLevel);
}

YamiStatus
VaapiPostProcessScaler::setParamToNone(ProcParams& params, int32_t none)
{
    params.level = none;
    params.filter.reset();
    return YAMI_SUCCESS;
}

YamiStatus
VaapiPostProcessScaler::createFilter(BufObjectPtr& filter, VAProcFilterType type, float value)
{
    VAProcFilterParameterBuffer* f;
    filter = VaapiBuffer::create(m_context, VAProcFilterParameterBufferType, f);
    if (filter) {
        f->type = type;
        f->value = value;
        //unmap for va use.
        filter->unmap();
        return YAMI_SUCCESS;
    }
    return YAMI_FAIL;
}

YamiStatus
VaapiPostProcessScaler::setProcParams(ProcParams& params, int32_t level,
    int32_t min, int32_t max, int32_t none, VAProcFilterType type)
{
    if (params.level == level)
        return YAMI_SUCCESS;
    if (level == none)
        return setParamToNone(params, none);
    float value;
    if (!mapToRange(value, level, min, max, type, params.caps)) {
        setParamToNone(params, none);
        return YAMI_INVALID_PARAM;
    }
    return createFilter(params.filter, type, value);
}

static VAProcDeinterlacingType getDeinterlaceMode(VppDeinterlaceMode mode)
{
    switch (mode) {
    case DEINTERLACE_MODE_BOB:
        return VAProcDeinterlacingBob;
    default:
        break;
    }
    return VAProcDeinterlacingNone;
}

YamiStatus
VaapiPostProcessScaler::createDeinterlaceFilter(const VPPDeinterlaceParameters& deinterlace)
{
    VAProcFilterParameterBufferDeinterlacing* d;
    m_deinterlace.filter = VaapiBuffer::create(m_context, VAProcFilterParameterBufferType, d);
    if (!m_deinterlace.filter)
        return YAMI_DRIVER_FAIL;
    d->type = VAProcFilterDeinterlacing;
    d->algorithm = getDeinterlaceMode(deinterlace.mode);
    //unmap for va usage
    m_deinterlace.filter->unmap();
    return YAMI_SUCCESS;
}

static bool mapToVppColorBalanceMode(VppColorBalanceMode& vppMode, VAProcColorBalanceType vaMode)
{
    switch (vaMode) {
    case VAProcColorBalanceNone:
        vppMode = COLORBALANCE_NONE;
        break;
    case VAProcColorBalanceHue:
        vppMode = COLORBALANCE_HUE;
        break;
    case VAProcColorBalanceSaturation:
        vppMode = COLORBALANCE_SATURATION;
        break;
    case VAProcColorBalanceBrightness:
        vppMode = COLORBALANCE_BRIGHTNESS;
        break;
    case VAProcColorBalanceContrast:
        vppMode = COLORBALANCE_CONTRAST;
        break;
    default:
        return false;
    }
    return true;
}

YamiStatus
VaapiPostProcessScaler::createColorBalanceFilters(ColorBalanceParam& clrBalance, const VPPColorBalanceParameter& vppClrBalance)
{
    float value;

    if (!mapToRange(value, clrBalance.range.min_value, clrBalance.range.max_value, vppClrBalance.level, COLORBALANCE_LEVEL_MIN, COLORBALANCE_LEVEL_MAX)) {
        return YAMI_DRIVER_FAIL;
    }

    VAProcFilterParameterBufferColorBalance* d;
    clrBalance.filter = VaapiBuffer::create(m_context, VAProcFilterParameterBufferType, d);
    if (!clrBalance.filter)
        return YAMI_DRIVER_FAIL;
    d->type = VAProcFilterColorBalance;
    d->attrib = clrBalance.type;
    d->value = value;

    //unmap for va usage
    clrBalance.filter->unmap();
    clrBalance.level = vppClrBalance.level;
    return YAMI_SUCCESS;
}

YamiStatus VaapiPostProcessScaler::setDeinterlaceParam(const VPPDeinterlaceParameters& deinterlace)
{
    std::set<VppDeinterlaceMode>& supported = m_deinterlace.supportedModes;
    if (supported.empty()) {
        //query from libva.
        VAProcFilterCapDeinterlacing caps[VAProcDeinterlacingCount];
        uint32_t num = VAProcDeinterlacingCount;
        YamiStatus status = queryVideoProcFilterCaps(VAProcFilterDeinterlacing, caps, &num);
        if (status != YAMI_SUCCESS)
            return status;
        for (uint32_t i = 0; i < num; i++) {
            //only support bob yet
            if (caps[i].type == VAProcDeinterlacingBob) {
                supported.insert(DEINTERLACE_MODE_BOB);
            }
        }
    }
    VppDeinterlaceMode mode = deinterlace.mode;
    if (mode == DEINTERLACE_MODE_NONE) {
        m_deinterlace.filter.reset();
        return YAMI_SUCCESS;
    }
    if (supported.find(mode) == supported.end()) {
        m_deinterlace.filter.reset();
        return YAMI_UNSUPPORTED;
    }
    m_deinterlace.mode = mode;
    return createDeinterlaceFilter(deinterlace);
}

YamiStatus VaapiPostProcessScaler::setColorBalanceParam(const VPPColorBalanceParameter& colorbalance)
{
    VAProcFilterCapColorBalance caps[VAProcColorBalanceCount];
    VppColorBalanceMode vppClrBalanceMode;
    if (m_colorBalance.empty()) {
        uint32_t num = VAProcColorBalanceCount;
        //query from libva.
        YamiStatus status = queryVideoProcFilterCaps(VAProcFilterColorBalance, caps, &num);
        if (status != YAMI_SUCCESS)
            return status;
        for (uint32_t i = 0; i < num; i++) {
            if (mapToVppColorBalanceMode(vppClrBalanceMode, caps[i].type)) {
                m_colorBalance[vppClrBalanceMode].range = caps[i].range;
                m_colorBalance[vppClrBalanceMode].type = caps[i].type;
                m_colorBalance[vppClrBalanceMode].level = COLORBALANCE_LEVEL_NONE;
            }
        }
    }

    if(COLORBALANCE_NONE == colorbalance.mode){
        for (ColorBalanceMapItr itr = m_colorBalance.begin(); itr != m_colorBalance.end(); itr++) {
            if (itr->second.filter) {
                itr->second.filter.reset();
                itr->second.level = COLORBALANCE_LEVEL_NONE;
            }
        }
        return YAMI_SUCCESS;
    }

    ColorBalanceMapItr iteratorClrBalance = m_colorBalance.find(colorbalance.mode);
    if (iteratorClrBalance == m_colorBalance.end()) {
        ERROR("unsupported VppColorBalanceMode: %d", colorbalance.mode);
        return YAMI_UNSUPPORTED;
    }

    if (colorbalance.level == COLORBALANCE_LEVEL_NONE) {
        if (iteratorClrBalance->second.filter)
            iteratorClrBalance->second.filter.reset();
        iteratorClrBalance->second.level = colorbalance.level;
        return YAMI_SUCCESS;
    }

    if (colorbalance.level == iteratorClrBalance->second.level) {
        return YAMI_SUCCESS;
    }

    return createColorBalanceFilters(iteratorClrBalance->second, colorbalance);
}

YamiStatus
VaapiPostProcessScaler::setParameters(VppParamType type, void* vppParam)
{
    if (!vppParam)
        return YAMI_INVALID_PARAM;
    if (!m_context) {
        ERROR("no context");
        return YAMI_FAIL;
    }
    if (type == VppParamTypeDenoise) {
        VPPDenoiseParameters* denoise = (VPPDenoiseParameters*)vppParam;
        if (denoise->size != sizeof(VPPDenoiseParameters))
            return YAMI_INVALID_PARAM;
        return setProcParams(m_denoise, denoise->level,
            DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX, DENOISE_LEVEL_NONE, VAProcFilterNoiseReduction);
    }
    else if (type == VppParamTypeSharpening) {
        VPPSharpeningParameters* sharpening = (VPPSharpeningParameters*)vppParam;
        if (sharpening->size != sizeof(VPPSharpeningParameters))
            return YAMI_INVALID_PARAM;
        return setProcParams(m_sharpening, sharpening->level,
            SHARPENING_LEVEL_MIN, SHARPENING_LEVEL_MAX, SHARPENING_LEVEL_NONE, VAProcFilterSharpening);
    }
    else if (type == VppParamTypeDeinterlace) {
        VPPDeinterlaceParameters* deinterlace = (VPPDeinterlaceParameters*)vppParam;
        if (deinterlace->size != sizeof(VPPDeinterlaceParameters))
            return YAMI_INVALID_PARAM;
        return setDeinterlaceParam(*deinterlace);
    }
    else if (type == VppParamTypeColorBalance) {
        VPPColorBalanceParameter* colorbalance = (VPPColorBalanceParameter*)vppParam;
        if (colorbalance->size != sizeof(VPPColorBalanceParameter))
            return YAMI_INVALID_PARAM;

        return setColorBalanceParam(*colorbalance);
    }
    return VaapiPostProcessBase::setParameters(type, vppParam);
}

}
