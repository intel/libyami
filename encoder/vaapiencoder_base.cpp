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
#include "common/utils.h"
#include "scopedlogger.h"
#include "vaapicodedbuffer.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapiutils.h"

const uint32_t MaxOutputBuffer=5;
namespace YamiMediaCodec{
VaapiEncoderBase::VaapiEncoderBase():
    m_entrypoint(VAEntrypointEncSlice),
    m_maxOutputBuffer(MaxOutputBuffer),
    m_maxCodedbufSize(0)
{
    FUNC_ENTER();
    m_externalDisplay.handle = 0,
    m_externalDisplay.type = NATIVE_DISPLAY_AUTO,

    m_videoParamCommon.size = sizeof(m_videoParamCommon);
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

    updateMaxOutputBufferCount();
}

VaapiEncoderBase::~VaapiEncoderBase()
{
    cleanupVA();
    INFO("~VaapiEncoderBase");
}

void VaapiEncoderBase::setNativeDisplay(NativeDisplay * nativeDisplay)
{
    if (!nativeDisplay || nativeDisplay->type == NATIVE_DISPLAY_AUTO)
        return;

    m_externalDisplay = *nativeDisplay;
}

Encode_Status VaapiEncoderBase::start(void)
{
    FUNC_ENTER();
    if (!initVA())
        return ENCODE_FAIL;

    return ENCODE_SUCCESS;
}

void VaapiEncoderBase::flush(void)
{
    AutoLock l(m_lock);
    m_output.clear();
}

Encode_Status VaapiEncoderBase::stop(void)
{
    FUNC_ENTER();
    cleanupVA();
    return ENCODE_SUCCESS;
}

bool VaapiEncoderBase::isBusy()
{
    AutoLock l(m_lock);
    return m_output.size() >= m_maxOutputBuffer;
}


Encode_Status VaapiEncoderBase::encode(VideoEncRawBuffer *inBuffer)
{
    FUNC_ENTER();

    if (!inBuffer || !inBuffer->data && !inBuffer->size) {
        // XXX handle EOS when there is B frames
        inBuffer->bufAvailable = true;
        return ENCODE_SUCCESS;
    }
    VideoFrameRawData frame;
    if (!fillFrameRawData(&frame, inBuffer->fourcc, width(), height(), inBuffer->data))
        return ENCODE_INVALID_PARAMS;
    inBuffer->bufAvailable = true;
    if (inBuffer->forceKeyFrame)
        frame.flags |= VIDEO_FRAME_FLAGS_KEY;
    frame.timeStamp = inBuffer->timeStamp;
    return encode(&frame);
}

Encode_Status VaapiEncoderBase::encode(VideoFrameRawData* frame)
{
    if (!frame || !frame->width || !frame->height || !frame->fourcc)
        return ENCODE_INVALID_PARAMS;

    FUNC_ENTER();
    Encode_Status ret;

    if (isBusy())
        return ENCODE_IS_BUSY;
    SurfacePtr surface = createSurface(frame);
    if (!surface)
        return ENCODE_NO_MEMORY;
    return doEncode(surface, frame->timeStamp, frame->flags & VIDEO_FRAME_FLAGS_KEY);
}

Encode_Status VaapiEncoderBase::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    Encode_Status ret = ENCODE_INVALID_PARAMS;
    if (!videoEncParams)
        return ret;

    DEBUG("type = 0x%08x", type);
    switch (type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            PARAMETER_ASSIGN(*common, m_videoParamCommon);
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

Encode_Status VaapiEncoderBase::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    Encode_Status ret = ENCODE_SUCCESS;
    if (!videoEncParams)
        return ret;

    DEBUG("type = 0x%08x", type);
    switch (type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            PARAMETER_ASSIGN(m_videoParamCommon, *common);
            //dirty work around, force rcmod before we support more
            m_videoParamCommon.rcMode = RATE_CONTROL_CQP;
            //end
        } else
            ret = ENCODE_INVALID_PARAMS;
        m_maxCodedbufSize = 0; // resolution may change, recalculate max codec buffer size when it is requested
        break;
    }
    case VideoConfigTypeFrameRate: {
        VideoConfigFrameRate* frameRateConfig = (VideoConfigFrameRate*)videoEncParams;
        if (frameRateConfig->size == sizeof(VideoConfigFrameRate)) {
            m_videoParamCommon.frameRate = frameRateConfig->frameRate;
        } else
            ret = ENCODE_INVALID_PARAMS;
        }
        break;
    case VideoConfigTypeBitRate: {
        VideoConfigBitRate* rcParamsConfig = (VideoConfigBitRate*)videoEncParams;
        if (rcParamsConfig->size == sizeof(VideoConfigFrameRate)) {
            m_videoParamCommon.rcParams = rcParamsConfig->rcParams;
        } else
            ret = ENCODE_INVALID_PARAMS;
        }
        break;
    default:
        ret = ENCODE_INVALID_PARAMS;
        break;
    }
    INFO("bitrate: %d", bitRate());
    return ret;
}

Encode_Status VaapiEncoderBase::setConfig(VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    FUNC_ENTER();
    DEBUG("type = %d", type);
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::getConfig(VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    FUNC_ENTER();
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::getMaxOutSize(uint32_t *maxSize)
{
    FUNC_ENTER();
    *maxSize = 0;
    return ENCODE_SUCCESS;
}

SurfacePtr VaapiEncoderBase::createSurface(uint32_t fourcc)
{
    VASurfaceAttrib attrib;
    VaapiChromaType chroma;

    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = fourcc;

    switch(fourcc) {
    case VA_FOURCC_NV12:
    case VA_FOURCC_I420:
        chroma = VAAPI_CHROMA_TYPE_YUV420;
    break;
    case VA_FOURCC_YUY2:
        chroma = VAAPI_CHROMA_TYPE_YUV422;
    break;
    default:
        ASSERT(0);
        break;
    }
    return VaapiSurface::create(m_display, chroma,
                                m_videoParamCommon.resolution.width, m_videoParamCommon.resolution.height, &attrib, 1);

}

SurfacePtr VaapiEncoderBase::createSurface(VideoFrameRawData* frame)
{
    SurfacePtr surface = createSurface(frame->fourcc);
    SurfacePtr nil;
    if (!surface)
        return nil;

    ImagePtr image = VaapiImage::derive(surface);
    if (!image) {
        ERROR("VaapiImage::derive() failed");
        return nil;
    }
    ImageRawPtr raw = image->map();
    if (!raw) {
        ERROR("image->map() failed");
        return nil;
    }

    uint8_t* src = reinterpret_cast<uint8_t*>(frame->handle);
    if (!raw->copyFrom(src, frame->offset, frame->pitch)) {
        ERROR("copyfrom in buffer failed");
        return nil;
    }
    return surface;
}

void VaapiEncoderBase::fill(VAEncMiscParameterHRD* hrd) const
{
    hrd->buffer_size = m_videoParamCommon.rcParams.bitRate * m_videoParamCommon.rcParams.windowSize/1000;
    hrd->initial_buffer_fullness = hrd->buffer_size/2;
    DEBUG("bitRate: %d, hrd->buffer_size: %d, hrd->initial_buffer_fullness: %d",
        m_videoParamCommon.rcParams.bitRate, hrd->buffer_size,hrd->initial_buffer_fullness);
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
    {VAAPI_PROFILE_JPEG_BASELINE,VAProfileJPEGBaseline},
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
    VAConfigAttrib attrib, *pAttrib = NULL;
    int32_t attribCount = 0;
    FUNC_ENTER();

    m_display = VaapiDisplay::create(m_externalDisplay);
    if (!m_display) {
        ERROR("failed to create display");
        return false;
    }

    if (RATE_CONTROL_NONE != m_videoParamCommon.rcMode) {
        attrib.type = VAConfigAttribRateControl;
        attrib.value = m_videoParamCommon.rcMode;
        pAttrib = &attrib;
        attribCount = 1;
    }
    ConfigPtr config = VaapiConfig::create(m_display, m_videoParamCommon.profile, m_entrypoint, pAttrib, attribCount);
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

Encode_Status VaapiEncoderBase::getOutput(VideoEncOutputBuffer * outBuffer, bool withWait)
{
    bool isEmpty;
    FUNC_ENTER();
    if (!outBuffer)
        return ENCODE_INVALID_PARAMS;

    {
        AutoLock l(m_lock);
        isEmpty = m_output.empty();
        INFO("output queue size: %ld\n", m_output.size());
    }

    if (isEmpty) {
        if (outBuffer->format == OUTPUT_CODEC_DATA)
           return getCodecConfig(outBuffer);
        return ENCODE_BUFFER_NO_MORE;
    }

    PicturePtr& picture = m_output.front();
    picture->sync();
    Encode_Status ret = picture->getOutput(outBuffer);
    if (ret != ENCODE_SUCCESS)
        return ret;
    if (outBuffer->format != OUTPUT_CODEC_DATA) {
        AutoLock l(m_lock);
        m_output.pop_front();
    }
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderBase::getCodecConfig(VideoEncOutputBuffer * outBuffer)
{
    ASSERT(outBuffer && (outBuffer->format == OUTPUT_CODEC_DATA));
    outBuffer->dataSize  = 0;
    return ENCODE_SUCCESS;
}

}
