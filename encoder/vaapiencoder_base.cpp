/*
 *  vaapiencoder_base.cpp - basic va encoder for video
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
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
#include "vaapiencoder_base.h"
#include <assert.h>
#include <stdint.h>
#include "common/common_def.h"
#include "scopedlogger.h"
#include "vaapicodedbuffer.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapiutils.h"

namespace YamiMediaCodec{
VaapiEncoderBase::VaapiEncoderBase():
    m_entrypoint(VAEntrypointEncSlice),
    m_externalDisplay(NULL)
{
    FUNC_ENTER();
    m_videoParamCommon.rawFormat = RAW_FORMAT_NV12;
    m_videoParamCommon.frameRate.frameRateNum = 30;
    m_videoParamCommon.frameRate.frameRateDenom = 1;
    m_videoParamCommon.intraPeriod = 15;
    m_videoParamCommon.rcMode = RATE_CONTROL_CQP;
    m_videoParamCommon.rcParams.initQP = 26;
    m_videoParamCommon.rcParams.minQP = 1;
    m_videoParamCommon.rcParams.targetPercentage= 70;
    m_videoParamCommon.rcParams.windowSize = 500;
    m_videoParamCommon.rcParams.disableBitsStuffing = 1;
    m_videoParamCommon.cyclicFrameInterval = 30;
    m_videoParamCommon.refreshType = VIDEO_ENC_NONIR;
    m_videoParamCommon.airParams.airAuto = 1;
}

VaapiEncoderBase::~VaapiEncoderBase()
{
    cleanupVA();
    INFO("~VaapiEncoderBase");
}

void VaapiEncoderBase::setXDisplay(Display * xdisplay)
{
    m_externalDisplay = xdisplay;
}

Encode_Status VaapiEncoderBase::start(void)
{
    FUNC_ENTER();
    if (!initVA())
        return ENCODE_FAIL;

    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::stop(void)
{
    FUNC_ENTER();
    cleanupVA();
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::encode(VideoEncRawBuffer *inBuffer)
{
    FUNC_ENTER();
    Encode_Status ret;
    SurfacePtr surface = createSurface(inBuffer);
    if (!surface)
        ret = ENCODE_NO_MEMORY;
    else
        ret = reorder(surface, inBuffer->timeStamp, inBuffer->forceKeyFrame);
    return ret;
}

Encode_Status VaapiEncoderBase::getParameters(VideoParamConfigSet *videoEncParams)
{
    FUNC_ENTER();
    Encode_Status ret = ENCODE_INVALID_PARAMS;
    if (!videoEncParams)
        return ret;

    DEBUG("type = %d", videoEncParams->type);
    switch (videoEncParams->type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            *common = m_videoParamCommon;
            ret = ENCODE_SUCCESS;
        }
        break;
    }
    default:
        ret = ENCODE_SUCCESS;
        break;
    }
    return ret;
}

Encode_Status VaapiEncoderBase::setParameters(VideoParamConfigSet *videoEncParams)
{
    FUNC_ENTER();
    Encode_Status ret = ENCODE_INVALID_PARAMS;
    if (!videoEncParams)
        return ret;

    DEBUG("type = %d", videoEncParams->type);
    switch (videoEncParams->type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            m_videoParamCommon = *common;
            ret = ENCODE_SUCCESS;
        }
        break;
    }
    default:
        ret = ENCODE_SUCCESS;
        break;
    }
    return ret;
}

Encode_Status VaapiEncoderBase::setConfig(VideoParamConfigSet *videoEncConfig)
{
    FUNC_ENTER();
    DEBUG("type = %d", videoEncConfig->type);
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::getConfig(VideoParamConfigSet *videoEncConfig)
{
    FUNC_ENTER();
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::getMaxOutSize(uint32_t *maxSize)
{
    FUNC_ENTER();
    return ENCODE_SUCCESS;
}

SurfacePtr VaapiEncoderBase::createSurface()
{
    VASurfaceAttrib attrib;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = VA_FOURCC_NV12;
    return VaapiSurface::create(m_display->getID(), VAAPI_CHROMA_TYPE_YUV420,
                                m_videoParamCommon.resolution.width, m_videoParamCommon.resolution.height, &attrib, 1);

}

SurfacePtr VaapiEncoderBase::createSurface(VideoEncRawBuffer* inBuffer)
{
    SurfacePtr surface = createSurface();
    if (!surface)
        return surface;

    VaapiImage* image = surface->getDerivedImage();
    if (!image) {
        ERROR("surface->getDerivedImage() failed");
        surface.reset();
        return surface;
    }
    VaapiImageRaw* raw = image->map();
    if (!raw) {
        ERROR("image->map() failed");
        surface.reset();
        return surface;
    }

    assert(inBuffer->size == raw->width * raw->height * 3 / 2);

    uint8_t* src = inBuffer->data;
    uint8_t* dest = raw->pixels[0];
    for (int i = 0; i < raw->height; i++) {
        memcpy(dest, src, raw->width);
        dest += raw->strides[0];
        src += raw->width;
    }
    dest = raw->pixels[1];
    for (int i = 0; i < raw->height/2; i++) {
        memcpy(dest, src, raw->width);
        dest += raw->strides[1];
        src += raw->width;
    }

    return surface;
}

Encode_Status VaapiEncoderBase::copyCodedBuffer(VideoEncOutputBuffer *outBuffer, const CodedBufferPtr& codedBuffer) const
{
    uint32_t size = codedBuffer->size();
    if (size > outBuffer->bufferSize) {
        outBuffer->dataSize = 0;
        return ENCODE_BUFFER_TOO_SMALL;
    }
    if (size > 0) {
        codedBuffer->copyInto(outBuffer->data);
        outBuffer->flag = ENCODE_BUFFERFLAG_ENDOFFRAME | ENCODE_BUFFERFLAG_SYNCFRAME;
    }
    outBuffer->dataSize = size;
    return ENCODE_SUCCESS;
}

void VaapiEncoderBase::fill(VAEncMiscParameterHRD* hrd) const
{
    hrd->buffer_size = m_videoParamCommon.rcParams.bitRate;
    hrd->initial_buffer_fullness = m_videoParamCommon.rcParams.bitRate/2;
}

void VaapiEncoderBase::fill(VAEncMiscParameterRateControl* rateControl) const
{
    rateControl->bits_per_second = m_videoParamCommon.rcParams.bitRate;
    rateControl->initial_qp =  m_videoParamCommon.rcParams.initQP;
    rateControl->min_qp =  m_videoParamCommon.rcParams.minQP;
    /*FIXME: where to find max_qp */
    rateControl->window_size = m_videoParamCommon.rcParams.windowSize;
    rateControl->target_percentage = m_videoParamCommon.rcParams.targetPercentage;
    rateControl->rc_flags.bits.disable_frame_skip = m_videoParamCommon.rcParams.disableFrameSkip;
    rateControl->rc_flags.bits.disable_bit_stuffing = m_videoParamCommon.rcParams.disableBitsStuffing;
}

/* Generates additional control parameters */
bool VaapiEncoderBase::ensureMiscParams (VaapiEncPicture* picture)
{
    VAEncMiscParameterHRD* hrd;
    if (!picture->newMisc(VAEncMiscParameterTypeHRD, hrd))
        return false;
    fill(hrd);
    VideoRateControl mode = rateControlMode();
    if (mode == RATE_CONTROL_CBR ||
            mode == RATE_CONTROL_VBR) {
        VAEncMiscParameterRateControl* rateControl;
        if (!picture->newMisc(VAEncMiscParameterTypeRateControl, rateControl))
            return false;
        fill(rateControl);
    }
    return true;
}

struct ProfileMapItem {
    VaapiProfile vaapiProfile;
    VAProfile    vaProfile;
};

const ProfileMapItem g_profileMap[] =
{
    {VAAPI_PROFILE_H264_BASELINE,  VAProfileH264Baseline},
    {VAAPI_PROFILE_H264_CONSTRAINED_BASELINE,VAProfileH264ConstrainedBaseline},
    {VAAPI_PROFILE_H264_MAIN, VAProfileH264Main},
    {VAAPI_PROFILE_H264_HIGH,VAProfileH264High},
};

VaapiProfile VaapiEncoderBase::profile() const
{
    for (int i = 0; i < N_ELEMENTS(g_profileMap); i++) {
        if (m_videoParamCommon.profile == g_profileMap[i].vaProfile)
            return g_profileMap[i].vaapiProfile;
    }
    return VAAPI_PROFILE_UNKNOWN;
}

void VaapiEncoderBase::cleanupVA()
{
    m_context.reset();
    m_display.reset();
}

bool VaapiEncoderBase::initVA()
{
    FUNC_ENTER();

    m_display = VaapiDisplay::create(m_externalDisplay);
    if (!m_display) {
        ERROR("failed to create display");
        return false;
    }

    ConfigPtr config = VaapiConfig::create(m_display, m_videoParamCommon.profile, m_entrypoint, NULL, 0);
    if (!config) {
        ERROR("failed to create config");
        return false;
    }

    m_context = VaapiContext::create(config,
                             m_videoParamCommon.resolution.width,
                             m_videoParamCommon.resolution.height,
                             VA_PROGRESSIVE, 0, 0);
    if (!m_context) {
        ERROR("failed to create context");
        return false;
    }
    return true;
}
}
