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

#include "vaapiencoder_base.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include "common/common_def.h"
#include "common/utils.h"
#include "common/scopedlogger.h"
#include "vaapicodedbuffer.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapisurfaceallocator.h"
#include "vaapi/VaapiUtils.h"

#define ADJUST_TO_RANGE(v, min, max, promp)                   \
    do {                                                      \
        if (v < min) {                                        \
            WARNING("%s: %s is out of the range", promp, #v); \
            v = min;                                          \
        }                                                     \
        if (v > max) {                                        \
            WARNING("%s: %s is out of the range", promp, #v); \
            v = max;                                          \
        }                                                     \
    } while (0)

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

    memset(&m_videoParamCommon, 0, sizeof(m_videoParamCommon));
    m_videoParamCommon.size = sizeof(m_videoParamCommon);
    m_videoParamCommon.frameRate.frameRateNum = 30;
    m_videoParamCommon.frameRate.frameRateDenom = 1;
    m_videoParamCommon.intraPeriod = 15;
    m_videoParamCommon.ipPeriod = 1;
    m_videoParamCommon.numRefFrames = 1;
    m_videoParamCommon.rcMode = RATE_CONTROL_CQP;
    m_videoParamCommon.rcParams.initQP = 26;
    m_videoParamCommon.rcParams.minQP = 1;
    m_videoParamCommon.rcParams.maxQP = 51;
    m_videoParamCommon.rcParams.disableBitsStuffing = 1;
    m_videoParamCommon.bitDepth = 8;
    memset(&m_videoParamsHRD, 0, sizeof(m_videoParamsHRD));
    m_videoParamsHRD.windowSize = 1000;
    m_videoParamsHRD.targetPercentage = 95;
    m_videoParamQualityLevelUpdate = false;
    m_videoParamQualityLevel.size = sizeof(m_videoParamQualityLevel);
    m_videoParamQualityLevel.level = 0;
    m_vaVideoParamQualityLevel = 0;
    updateMaxOutputBufferCount();
    memset(m_svctFrameRate, 0, sizeof(m_svctFrameRate));
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

YamiStatus VaapiEncoderBase::start(void)
{
    FUNC_ENTER();
    if (!initVA())
        return YAMI_FAIL;

    return YAMI_SUCCESS;
}

void VaapiEncoderBase::flush(void)
{
    /* Current version of VaapiEncoderBase::flush is empty fucntion
     * But we may add something in future.All derive class need call this in derive::flush()
     */
}

YamiStatus VaapiEncoderBase::stop(void)
{
    FUNC_ENTER();
    m_output.clear();
    cleanupVA();
    return YAMI_SUCCESS;
}

bool VaapiEncoderBase::isBusy()
{
    AutoLock l(m_lock);
    return m_output.size() >= m_maxOutputBuffer;
}

YamiStatus VaapiEncoderBase::encode(VideoEncRawBuffer* inBuffer)
{
    FUNC_ENTER();

    if (!inBuffer)
        return YAMI_SUCCESS;
    if (!inBuffer->data && !inBuffer->size) {
        // XXX handle EOS when there is B frames
        inBuffer->bufAvailable = true;
        return YAMI_SUCCESS;
    }
    VideoFrameRawData frame;
    if (!fillFrameRawData(&frame, inBuffer->fourcc, width(), height(), inBuffer->data))
        return YAMI_INVALID_PARAM;
    inBuffer->bufAvailable = true;
    if (inBuffer->forceKeyFrame)
        frame.flags |= VIDEO_FRAME_FLAGS_KEY;
    frame.timeStamp = inBuffer->timeStamp;
    return encode(&frame);
}

YamiStatus VaapiEncoderBase::encode(VideoFrameRawData* frame)
{
    if (!frame || !frame->width || !frame->height || !frame->fourcc)
        return YAMI_INVALID_PARAM;

    FUNC_ENTER();

    if (isBusy())
        return YAMI_ENCODE_IS_BUSY;
    SurfacePtr surface = createSurface(frame);
    if (!surface)
        return YAMI_OUT_MEMORY;
    return doEncode(surface, frame->timeStamp, frame->flags & VIDEO_FRAME_FLAGS_KEY);
}

YamiStatus VaapiEncoderBase::encode(const SharedPtr<VideoFrame>& frame)
{
    if (!frame)
        return YAMI_INVALID_PARAM;
    if (isBusy())
        return YAMI_ENCODE_IS_BUSY;
    SurfacePtr surface = createSurface(frame);
    if (!surface)
        return YAMI_INVALID_PARAM;
    return doEncode(surface, frame->timeStamp, frame->flags & VIDEO_FRAME_FLAGS_KEY);
}

YamiStatus VaapiEncoderBase::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    YamiStatus ret = YAMI_INVALID_PARAM;
    if (!videoEncParams)
        return ret;

    DEBUG("type = 0x%08x", type);
    switch (type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            PARAMETER_ASSIGN(*common, m_videoParamCommon);
            ret = YAMI_SUCCESS;
        }
        break;
    }
    case VideoParamsTypeHRD: {
        VideoParamsHRD* videoParamsHRD = (VideoParamsHRD*)videoEncParams;
        if (videoParamsHRD->size == sizeof(VideoParamsHRD)) {
            PARAMETER_ASSIGN(*videoParamsHRD, m_videoParamsHRD);
            ret = YAMI_SUCCESS;
        }
        break;
    }
    case VideoParamsTypeQualityLevel: {
        VideoParamsQualityLevel* qualityLevel = (VideoParamsQualityLevel*)videoEncParams;
        if (qualityLevel->size == sizeof(VideoParamsQualityLevel)) {
            PARAMETER_ASSIGN(*qualityLevel, m_videoParamQualityLevel);
            ret = YAMI_SUCCESS;
        }

        break;
    }
    default:
        ret = YAMI_SUCCESS;
        break;
    }
    return ret;
}

YamiStatus VaapiEncoderBase::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    YamiStatus ret = YAMI_SUCCESS;
    if (!videoEncParams)
        return ret;

    DEBUG("type = 0x%08x", type);
    switch (type) {
    case VideoParamsTypeCommon: {
        VideoParamsCommon* common = (VideoParamsCommon*)videoEncParams;
        if (common->size == sizeof(VideoParamsCommon)) {
            PARAMETER_ASSIGN(m_videoParamCommon, *common);
            if(m_videoParamCommon.rcParams.bitRate > 0){
                if(m_videoParamCommon.rcMode != RATE_CONTROL_VBR)
                    m_videoParamCommon.rcMode = RATE_CONTROL_CBR;
            } else{
                m_videoParamCommon.rcMode = RATE_CONTROL_CQP;
            }
        } else
            ret = YAMI_INVALID_PARAM;
        m_maxCodedbufSize = 0; // resolution may change, recalculate max codec buffer size when it is requested

        break;
    }
    case VideoConfigTypeFrameRate: {
        VideoConfigFrameRate* frameRateConfig = (VideoConfigFrameRate*)videoEncParams;
        if (frameRateConfig->size == sizeof(VideoConfigFrameRate)) {
            m_videoParamCommon.frameRate = frameRateConfig->frameRate;
        } else
            ret = YAMI_INVALID_PARAM;
        }
        break;
    case VideoConfigTypeBitRate: {
        VideoConfigBitRate* rcParamsConfig = (VideoConfigBitRate*)videoEncParams;
        if (rcParamsConfig->size == sizeof(VideoConfigBitRate)) {
            m_videoParamCommon.rcParams = rcParamsConfig->rcParams;
        } else
            ret = YAMI_INVALID_PARAM;
        }
        break;
    case VideoParamsTypeHRD: {
        VideoParamsHRD* videoParamsHRD = (VideoParamsHRD*)videoEncParams;
        if (videoParamsHRD->size == sizeof(VideoParamsHRD)) {
            PARAMETER_ASSIGN(m_videoParamsHRD, *videoParamsHRD);
            ADJUST_TO_RANGE(m_videoParamsHRD.targetPercentage, 50, 100, "target percentage");
        } else
            ret = YAMI_INVALID_PARAM;
        }
        break;
        case VideoParamsTypeQualityLevel: {
            VideoParamsQualityLevel* videoQualityLevel = (VideoParamsQualityLevel*)videoEncParams;
            if (videoQualityLevel->size == sizeof(VideoParamsQualityLevel)) {
                if (videoQualityLevel->level != m_videoParamQualityLevel.level) {
                    PARAMETER_ASSIGN(m_videoParamQualityLevel, *videoQualityLevel);
                    ADJUST_TO_RANGE(m_videoParamQualityLevel.level, VIDEO_PARAMS_QUALITYLEVEL_NONE,
                        VIDEO_PARAMS_QUALITYLEVEL_MAX, "quality level");
                    m_videoParamQualityLevelUpdate = true;
                }
            }
            else
                ret = YAMI_INVALID_PARAM;
        } break;
    default:
        ret = YAMI_INVALID_PARAM;
        break;
    }
    INFO("bitrate: %d", bitRate());
    return ret;
}

YamiStatus VaapiEncoderBase::setConfig(VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    FUNC_ENTER();
    DEBUG("type = %d", type);
    return YAMI_SUCCESS;
}

YamiStatus VaapiEncoderBase::getConfig(VideoParamConfigType type, Yami_PTR videoEncConfig)
{
    FUNC_ENTER();
    return YAMI_SUCCESS;
}

YamiStatus VaapiEncoderBase::getMaxOutSize(uint32_t* maxSize)
{
    FUNC_ENTER();
    *maxSize = 0;
    return YAMI_SUCCESS;
}

#ifdef __BUILD_GET_MV__
YamiStatus VaapiEncoderBase::getMVBufferSize(uint32_t* Size)
{
    FUNC_ENTER();
    *Size = 0;
    return YAMI_SUCCESS;
}
#endif

struct SurfaceDestroyer {
    SurfaceDestroyer(DisplayPtr display)
        : m_display(display)
    {
    }
    void operator()(VaapiSurface* surface)
    {
        VASurfaceID id = surface->getID();
        checkVaapiStatus(vaDestroySurfaces(m_display->getID(), &id, 1),
            "vaDestroySurfaces");
        delete surface;
    }

private:
    DisplayPtr m_display;
};

SurfacePtr VaapiEncoderBase::createNewSurface(uint32_t fourcc)
{
    VASurfaceAttrib attrib;
    uint32_t rtFormat;
    SurfacePtr surface;

    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = fourcc;

    rtFormat = getRtFormat(fourcc);
    if (!rtFormat) {
        ERROR("unsupported fourcc %x", fourcc);
        return surface;
    }

    VASurfaceID id;
    uint32_t width = m_videoParamCommon.resolution.width;
    uint32_t height = m_videoParamCommon.resolution.height;
    VAStatus status = vaCreateSurfaces(m_display->getID(), rtFormat, width, height,
        &id, 1, &attrib, 1);
    if (!checkVaapiStatus(status, "vaCreateSurfaces"))
        return surface;
    surface.reset(new VaapiSurface((intptr_t)id, width, height, fourcc),
        SurfaceDestroyer(m_display));
    return surface;
}

SurfacePtr VaapiEncoderBase::createSurface()
{
    SurfacePtr s;
    if (m_pool) {
        s = m_pool->alloc();
    } else {
        ERROR("BUG!: surface pool not created");
    }
    return s;
}

static bool copyImage(uint8_t* destBase,
    const uint32_t destOffsets[3], const uint32_t destPitches[3],
    const uint8_t* srcBase,
    const uint32_t srcOffsets[3], const uint32_t srcPitches[3],
    const uint32_t width[3], const uint32_t height[3], uint32_t planes)
{
    for (uint32_t i = 0; i < planes; i++) {
        uint32_t w = width[i];
        uint32_t h = height[i];
        if (w > destPitches[i] || w > srcPitches[i]) {
            ERROR("can't copy, plane = %d,  width = %d, srcPitch = %d, destPitch = %d",
                i, w, srcPitches[i], destPitches[i]);
            return false;
        }
        const uint8_t* src = srcBase + srcOffsets[i];
        uint8_t* dest = destBase + destOffsets[i];

        for (uint32_t j = 0; j < h; j++) {
            memcpy(dest, src, w);
            src += srcPitches[i];
            dest += destPitches[i];
        }
    }
    return true;
}

SurfacePtr VaapiEncoderBase::createSurface(VideoFrameRawData* frame)
{
    uint32_t fourcc = frame->fourcc;

    SurfacePtr surface = createNewSurface(fourcc);
    SurfacePtr nil;
    if (!surface)
        return nil;

    uint32_t width[3];
    uint32_t height[3];
    uint32_t planes;
    if (!getPlaneResolution(fourcc, frame->width, frame->height, width, height, planes)) {
        ERROR("invalid input format");
        return nil;
    }

    VAImage image;
    VADisplay display = m_display->getID();
    uint8_t* dest = mapSurfaceToImage(display, surface->getID(), image);
    if (!dest) {
        ERROR("map image failed");
        return nil;
    }
    uint8_t* src = reinterpret_cast<uint8_t*>(frame->handle);
    if (!copyImage(dest, image.offsets, image.pitches, src,
            frame->offset, frame->pitch, width, height, planes)) {
        ERROR("failed to copy image");
        unmapImage(display, image);
        return nil;
    }
    unmapImage(display, image);
    return surface;
}

SurfacePtr VaapiEncoderBase::createSurface(const SharedPtr<VideoFrame>& frame)
{
    SurfacePtr surface(new VaapiSurface(frame));
    return surface;
}

void VaapiEncoderBase::fill(VAEncMiscParameterHRD* hrd) const
{
    if (m_videoParamsHRD.bufferSize && m_videoParamsHRD.initBufferFullness) {
        hrd->buffer_size = m_videoParamsHRD.bufferSize;
        hrd->initial_buffer_fullness = m_videoParamsHRD.initBufferFullness;
    }
    else {
        hrd->initial_buffer_fullness = m_videoParamCommon.rcParams.bitRate;
        hrd->buffer_size = hrd->initial_buffer_fullness * 2;
    }

    DEBUG("bitRate: %d, hrd->buffer_size: %d, hrd->initial_buffer_fullness: %d",
        m_videoParamCommon.rcParams.bitRate, hrd->buffer_size,hrd->initial_buffer_fullness);
}

void VaapiEncoderBase::fill(VAEncMiscParameterRateControl* rateControl, uint32_t temporalID) const
{
#if VA_CHECK_VERSION(0, 39, 4)
    rateControl->rc_flags.bits.temporal_id = temporalID;
#endif
    //The highest layer's bitrate is bitRate()
    rateControl->bits_per_second = (temporalID == m_videoParamCommon.temporalLayers.numLayersMinus1) ? bitRate() : m_videoParamCommon.temporalLayers.bitRate[temporalID];
    rateControl->initial_qp =  m_videoParamCommon.rcParams.initQP;
    rateControl->min_qp =  m_videoParamCommon.rcParams.minQP;
    /*FIXME: where to find max_qp */
    rateControl->window_size = m_videoParamsHRD.windowSize;
    rateControl->target_percentage = m_videoParamsHRD.targetPercentage;
    rateControl->rc_flags.bits.disable_frame_skip = m_videoParamCommon.rcParams.disableFrameSkip;
    rateControl->rc_flags.bits.disable_bit_stuffing = m_videoParamCommon.rcParams.disableBitsStuffing;

}

void VaapiEncoderBase::fill(VAEncMiscParameterFrameRate* frameRate, uint32_t temporalID) const
{
#if VA_CHECK_VERSION(0, 39, 4)
    frameRate->framerate_flags.bits.temporal_id = temporalID;
#endif
    //The highest layer's framerate is fps()
    frameRate->framerate = (temporalID == m_videoParamCommon.temporalLayers.numLayersMinus1) ? fps() : m_svctFrameRate[temporalID].frameRateNum | (m_svctFrameRate[temporalID].frameRateDenom << 16);
}

bool VaapiEncoderBase::ensureRateControl(VaapiEncPicture* picture, uint32_t temporalID)
{
    VAEncMiscParameterRateControl* rateControl = NULL;
    if (!picture->newMisc(VAEncMiscParameterTypeRateControl, rateControl))
        return false;
    if (rateControl)
        fill(rateControl, temporalID);
    return true;
}
bool VaapiEncoderBase::ensureFrameRate(VaapiEncPicture* picture, uint32_t temporalID)
{
    VAEncMiscParameterFrameRate* frameRate = NULL;
    if (!picture->newMisc(VAEncMiscParameterTypeFrameRate, frameRate))
        return false;
    if (frameRate)
        fill(frameRate, temporalID);
    return true;
}

/* Generates additional control parameters */
bool VaapiEncoderBase::ensureMiscParams (VaapiEncPicture* picture)
{
    VAEncMiscParameterHRD* hrd = NULL;
    if (!picture->newMisc(VAEncMiscParameterTypeHRD, hrd))
        return false;
    if (hrd)
        fill(hrd);

    if (!fillQualityLevel(picture))
        return false;

    VideoRateControl mode = rateControlMode();
    if (mode == RATE_CONTROL_CBR ||
            mode == RATE_CONTROL_VBR) {
        //+1 for the highest layer
        uint32_t layers = m_videoParamCommon.temporalLayers.numLayersMinus1 + 1;
        for (uint32_t i = 0; i < layers; i++) {
            if (!ensureRateControl(picture, i))
                return false;
            if (!ensureFrameRate(picture, i))
                return false;
        }
    }
    return true;
}

struct ProfileMapItem {
    VideoProfile videoProfile;
    VAProfile    vaProfile;
};

const ProfileMapItem g_profileMap[] = {
    { PROFILE_H264_CONSTRAINED_BASELINE, VAProfileH264ConstrainedBaseline },
    { PROFILE_H264_MAIN, VAProfileH264Main },
    { PROFILE_H264_HIGH, VAProfileH264High },
    { PROFILE_JPEG_BASELINE, VAProfileJPEGBaseline },
#if VA_CHECK_VERSION(0, 38, 0)
    { PROFILE_H265_MAIN, VAProfileHEVCMain },
    { PROFILE_H265_MAIN10, VAProfileHEVCMain10 },
#endif
};

VideoProfile VaapiEncoderBase::profile() const
{
    for (size_t i = 0; i < N_ELEMENTS(g_profileMap); i++) {
        if (m_videoParamCommon.profile == g_profileMap[i].vaProfile)
            return g_profileMap[i].videoProfile;
    }
    return PROFILE_INVALID;
}

void VaapiEncoderBase::cleanupVA()
{
    m_pool.reset();
    m_alloc.reset();
    m_context.reset();
    m_display.reset();
}

void unrefAllocator(SurfaceAllocator* allocator)
{
    allocator->unref(allocator);
}

bool VaapiEncoderBase::initVA()
{
    VAConfigAttrib attrib, *pAttrib = NULL;
    ConfigPtr config;
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

    YamiStatus status = VaapiConfig::create(m_display, m_videoParamCommon.profile, m_entrypoint, pAttrib, attribCount, config);
    if (YAMI_SUCCESS != status) {
        ERROR("failed to create config");
        return false;
    }

    m_alloc.reset(new VaapiSurfaceAllocator(m_display->getID()), unrefAllocator);

    int32_t surfaceWidth = ALIGN16(m_videoParamCommon.resolution.width);
    int32_t surfaceHeight = ALIGN16(m_videoParamCommon.resolution.height);
    uint32_t fourcc = YAMI_FOURCC_NV12;
    if (m_videoParamCommon.bitDepth != 10 && m_videoParamCommon.bitDepth != 8) {
        ERROR("unsupported bit depth(%d)", m_videoParamCommon.bitDepth);
        return false;
    }
    if (10 == m_videoParamCommon.bitDepth)
        fourcc = YAMI_FOURCC_P010;
    m_pool = SurfacePool::create(m_alloc, fourcc, (uint32_t)surfaceWidth, (uint32_t)surfaceHeight, m_maxOutputBuffer);
    if (!m_pool)
        return false;

    std::vector<VASurfaceID> surfaces;
    m_pool->peekSurfaces(surfaces);

    m_context = VaapiContext::create(config,
                             surfaceWidth,
                             surfaceHeight,
                             VA_PROGRESSIVE, &surfaces[0], surfaces.size());
    if (!m_context) {
        ERROR("failed to create context");
        return false;
    }

    return true;
}

YamiStatus VaapiEncoderBase::checkEmpty(VideoEncOutputBuffer* outBuffer, bool* outEmpty)
{
    bool isEmpty;
    FUNC_ENTER();
    if (!outBuffer)
        return YAMI_INVALID_PARAM;

    AutoLock l(m_lock);
    isEmpty = m_output.empty();
    INFO("output queue size: %zu\n", m_output.size());

    *outEmpty = isEmpty;

    if (isEmpty) {
        if (outBuffer->format == OUTPUT_CODEC_DATA)
           return getCodecConfig(outBuffer);
        return YAMI_ENCODE_BUFFER_NO_MORE;
    }
    return YAMI_SUCCESS;
}

void VaapiEncoderBase::getPicture(PicturePtr &outPicture)
{
    outPicture = m_output.front();
    outPicture->sync();
}

YamiStatus VaapiEncoderBase::checkCodecData(VideoEncOutputBuffer* outBuffer)
{
    if (outBuffer->format != OUTPUT_CODEC_DATA) {
        AutoLock l(m_lock);
        m_output.pop_front();
    }
    return YAMI_SUCCESS;
}

#ifndef __BUILD_GET_MV__
YamiStatus VaapiEncoderBase::getOutput(VideoEncOutputBuffer* outBuffer, bool withWait)
{
    bool isEmpty;
    PicturePtr picture;
    YamiStatus ret;
    FUNC_ENTER();
    ret = checkEmpty(outBuffer, &isEmpty);
    if (isEmpty)
        return ret;

    getPicture(picture);
    ret = picture->getOutput(outBuffer);
    if (ret != YAMI_SUCCESS)
        return ret;

    outBuffer->timeStamp = picture->m_timeStamp;
    outBuffer->temporalID = picture->m_temporalID;
    checkCodecData(outBuffer);
    return YAMI_SUCCESS;
}

#else

YamiStatus VaapiEncoderBase::getOutput(VideoEncOutputBuffer* outBuffer, VideoEncMVBuffer* MVBuffer, bool withWait)
{
    void *data = NULL;
    uint32_t mappedSize;
    bool isEmpty;
    PicturePtr picture;
    YamiStatus ret;
    FUNC_ENTER();

    ret = checkEmpty(outBuffer, &isEmpty);
    if (isEmpty)
        return ret;
    getPicture(picture);

    ret = picture->getOutput(outBuffer);
    if (ret != YAMI_SUCCESS)
        return ret;
    if (!picture->editMVBuffer(data, &mappedSize))
        return ret;
    if (data)
        memcpy(MVBuffer->data, data, mappedSize);
    outBuffer->timeStamp = picture->m_timeStamp;
    outBuffer->temporalID = picture->m_temporalID;
    checkCodecData(outBuffer);
    return YAMI_SUCCESS;
}

#endif

YamiStatus VaapiEncoderBase::getCodecConfig(VideoEncOutputBuffer* outBuffer)
{
    ASSERT(outBuffer && (outBuffer->format == OUTPUT_CODEC_DATA));
    outBuffer->dataSize  = 0;
    return YAMI_SUCCESS;
}

bool VaapiEncoderBase::mapToRange(uint32_t& value,
    uint32_t min, uint32_t max,
    uint32_t level,
    uint32_t minLevel, uint32_t maxLevel)
{
    float fValue;
    if (minLevel >= maxLevel) {
        ERROR("minLevel(%d) >= maxLevel(%d)", minLevel, maxLevel);
        return false;
    }
    if (level > maxLevel || level < minLevel) {
        ERROR("level(%d) not in the range[minLevel(%d), maxLevel(%d)]", level, minLevel, maxLevel);
        return false;
    }
    if (min > max) {
        ERROR("min(%d) > max(%d)", min, max);
        return false;
    }

    fValue = min + 1.0 * (max - min) / (maxLevel - minLevel) * (level - minLevel);
    value = roundf(fValue);

    return true;
}

bool VaapiEncoderBase::mapQualityLevel()
{
    VAConfigAttrib attrib;
    uint32_t qualityLevel;

    attrib.type = VAConfigAttribEncQualityRange;
    VAStatus vaStatus = vaGetConfigAttributes(m_display->getID(),
        m_videoParamCommon.profile, m_entrypoint,
        &attrib, 1);
    if (vaStatus != VA_STATUS_SUCCESS || VA_ATTRIB_NOT_SUPPORTED == attrib.value) {
        ERROR("unsupported params encode quality level setting!");
        return false;
    }

    if (!mapToRange(qualityLevel,
            0, attrib.value,
            m_videoParamQualityLevel.level,
            VIDEO_PARAMS_QUALITYLEVEL_NONE, VIDEO_PARAMS_QUALITYLEVEL_MAX))
        return false;

    m_vaVideoParamQualityLevel = qualityLevel;
    return true;
}

bool VaapiEncoderBase::fillQualityLevel(VaapiEncPicture* picture)
{
    if (m_videoParamQualityLevelUpdate) {
        if (mapQualityLevel())
            m_videoParamQualityLevelUpdate = false;
        else
            return false;
    }

    if (0 == m_vaVideoParamQualityLevel)
        return true;

    VAEncMiscParameterBufferQualityLevel* qualityLevel = NULL;
    if (picture->newMisc(VAEncMiscParameterTypeQualityLevel, qualityLevel))
        if (qualityLevel) {
            qualityLevel->quality_level = m_vaVideoParamQualityLevel;
            return true;
        }

    return false;
}
}
