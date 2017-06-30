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

#include "vaapidecoder_base.h"
#include "common/log.h"
#include "vaapi/vaapisurfaceallocator.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/VaapiUtils.h"
#include "vaapidecsurfacepool.h"
#include <string.h>
#include <stdlib.h> // for setenv
#include <va/va_backend.h>
#include <unistd.h>

namespace YamiMediaCodec{
typedef VaapiDecoderBase::PicturePtr PicturePtr;

inline void unrefAllocator(SurfaceAllocator* allocator)
{
    allocator->unref(allocator);
}

VaapiDecoderBase::VaapiDecoderBase()
    : m_VAStarted(false)
    , m_currentPTS(INVALID_PTS)
{
    INFO("base: construct()");
    m_externalDisplay.handle = 0,
    m_externalDisplay.type = NATIVE_DISPLAY_AUTO,
    memset(&m_videoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&m_configBuffer, 0, sizeof(m_configBuffer));
    m_configBuffer.fourcc = YAMI_FOURCC_NV12;
}

VaapiDecoderBase::~VaapiDecoderBase()
{
    INFO("base: deconstruct()");
    stop();
}

YamiStatus VaapiDecoderBase::createPicture(PicturePtr& picture, int64_t timeStamp /* , VaapiPictureStructure structure = VAAPI_PICTURE_STRUCTURE_FRAME */)
{
    /*accquire one surface from m_surfacePool in base decoder  */
    SurfacePtr surface = createSurface();
    if (!surface) {
        DEBUG("create surface failed");
        return YAMI_DECODE_NO_SURFACE;
    }

    picture.reset(new VaapiDecPicture(m_context, surface, timeStamp));
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderBase::start(VideoConfigBuffer* buffer)
{
    YamiStatus status;

    INFO("base: start()");

    if (buffer == NULL) {
        return YAMI_DECODE_INVALID_DATA;
    }

    m_configBuffer = *buffer;
    m_configBuffer.data = NULL;
    m_configBuffer.size = 0;

    m_videoFormatInfo.width = buffer->width;
    m_videoFormatInfo.height = buffer->height;
    m_videoFormatInfo.surfaceWidth = buffer->surfaceWidth;
    m_videoFormatInfo.surfaceHeight = buffer->surfaceHeight;
    m_videoFormatInfo.surfaceNumber = buffer->surfaceNumber;
    if (!m_configBuffer.fourcc) {
        /* This just a workaround, user usually memset the VideoConfigBuffer to zero, and we will copy it to m_configBuffer.
           We need remove fields only for internal user from VideoConfigBuffer
           i.e., following thing should removed:
            int32_t surfaceWidth;
            int32_t surfaceHeight;
            int32_t frameRate;
            int32_t surfaceNumber;
            VAProfile profile;
            uint32_t flag;
            uint32_t fourcc;
        */
        m_videoFormatInfo.fourcc = m_configBuffer.fourcc = YAMI_FOURCC_NV12;
    }
    else {
        m_videoFormatInfo.fourcc = m_configBuffer.fourcc;
    }

    status = setupVA(buffer->surfaceNumber, buffer->profile);
    if (status != YAMI_SUCCESS)
        return status;

    DEBUG
        ("m_videoFormatInfo video size: %d x %d, m_videoFormatInfo surface size: %d x %d",
         m_videoFormatInfo.width, m_videoFormatInfo.height,
         m_videoFormatInfo.surfaceWidth, m_videoFormatInfo.surfaceHeight);

#ifdef __ENABLE_DEBUG__
    renderPictureCount = 0;
    if (access("/tmp/yami", F_OK) == 0) {
        m_dumpSurface = true;
    }
    else {
        m_dumpSurface = false;
    }
    DEBUG("m_dumpSurface: %d", m_dumpSurface);
#endif
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderBase::reset(VideoConfigBuffer* buffer)
{
    YamiStatus status;

    INFO("base: reset()");
    if (buffer == NULL) {
        return YAMI_DECODE_INVALID_DATA;
    }

    flush();

    status = terminateVA();
    if (status != YAMI_SUCCESS)
        return status;

    status = start(buffer);
    if (status != YAMI_SUCCESS)
        return status;

    return YAMI_SUCCESS;
}

void VaapiDecoderBase::stop(void)
{
    INFO("base: stop()");
    terminateVA();

    m_currentPTS = INVALID_PTS;

    m_videoFormatInfo.valid = false;
}

void VaapiDecoderBase::flush(void)
{

    INFO("base: flush()");
    m_output.clear();

    m_currentPTS = INVALID_PTS;
}

SharedPtr<VideoFrame> VaapiDecoderBase::getOutput()
{
    SharedPtr<VideoFrame> frame;
    if (m_output.empty())
        return frame;
    frame = m_output.front();
    m_output.pop_front();
    return frame;
}

const VideoFormatInfo *VaapiDecoderBase::getFormatInfo(void)
{
    INFO("base: getFormatInfo()");

    if (!m_VAStarted)
        return NULL;

    return &m_videoFormatInfo;
}

YamiStatus
VaapiDecoderBase::setupVA(uint32_t numSurface, VAProfile profile)
{
    INFO("base: setup VA");

    if (m_VAStarted) {
        return YAMI_SUCCESS;
    }

    if (m_display) {
        WARNING("VA is partially started.");
        return YAMI_FAIL;
    }

    m_display = VaapiDisplay::create(m_externalDisplay);

    if (!m_display) {
        ERROR("failed to create display");
        return YAMI_FAIL;
    }

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;
    ConfigPtr config;

    YamiStatus status = VaapiConfig::create(m_display, profile, VAEntrypointVLD, &attrib, 1, config);
    if (YAMI_SUCCESS != status) {
        ERROR("failed to create config");
        return status;
    }

    if (!m_externalAllocator) {
        //use internal allocator
        m_allocator.reset(new VaapiSurfaceAllocator(m_display->getID()), unrefAllocator);
    } else {
        m_allocator = m_externalAllocator;
    }

    m_configBuffer.surfaceNumber = numSurface;
    m_surfacePool = VaapiDecSurfacePool::create(&m_configBuffer, m_allocator);
    DEBUG("surface pool is created");
    if (!m_surfacePool)
        return YAMI_FAIL;
    std::vector<VASurfaceID> surfaces;
    m_surfacePool->getSurfaceIDs(surfaces);
    if (surfaces.empty())
        return YAMI_FAIL;
    int size = surfaces.size();
    m_context = VaapiContext::create(config,
                                       m_videoFormatInfo.width,
                                       m_videoFormatInfo.height,
                                       0, &surfaces[0], size);

    if (!m_context) {
        ERROR("create context failed");
        return YAMI_FAIL;
    }

    m_videoFormatInfo.surfaceWidth = m_videoFormatInfo.width;
    m_videoFormatInfo.surfaceHeight = m_videoFormatInfo.height;

    m_VAStarted = true;
    return YAMI_SUCCESS;
}

bool VaapiDecoderBase::createAllocator()
{
    if (m_allocator)
        return true;
    m_display = VaapiDisplay::create(m_externalDisplay);
    if (!m_display) {
        ERROR("failed to create display");
        return false;
    }

    if (!m_externalAllocator) {
        //use internal allocator
        m_allocator.reset(new VaapiSurfaceAllocator(m_display->getID()), unrefAllocator);
    }
    else {
        m_allocator = m_externalAllocator;
    }
    if (!m_allocator) {
        m_display.reset();
        ERROR("failed to create allocator");
        return false;
    }
    return true;
}

bool VaapiDecoderBase::isSurfaceGeometryChanged() const
{
    return m_config.width < m_videoFormatInfo.surfaceWidth
        || m_config.height < m_videoFormatInfo.surfaceHeight
        || m_config.surfaceNumber != m_videoFormatInfo.surfaceNumber
        || m_config.fourcc != m_videoFormatInfo.fourcc;
}

YamiStatus VaapiDecoderBase::ensureSurfacePool()
{

    if (!isSurfaceGeometryChanged())
        return YAMI_SUCCESS;
    VideoFormatInfo& info = m_videoFormatInfo;
    m_config.width = info.surfaceWidth;
    m_config.height = info.surfaceHeight;
    m_config.surfaceNumber = info.surfaceNumber;
    m_config.fourcc = info.fourcc;

    if (!createAllocator())
        return YAMI_FAIL;

    m_surfacePool = VaapiDecSurfacePool::create(&m_config, m_allocator);
    if (!m_surfacePool)
        return YAMI_FAIL;
    DEBUG("surface pool is created");

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderBase::ensureProfile(VAProfile profile)
{
    YamiStatus status;
    status = ensureSurfacePool();
    if (status != YAMI_SUCCESS)
        return status;

    if (!m_display || !m_surfacePool) {
        ERROR("bug: no display or surface pool");
        return YAMI_FAIL;
    }
    if (m_config.profile == profile)
        return YAMI_SUCCESS;

    m_config.profile = profile;
    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;
    ConfigPtr config;

    status = VaapiConfig::create(m_display, profile, VAEntrypointVLD, &attrib, 1, config);
    if (YAMI_SUCCESS != status) {
        ERROR("failed to create config");
        return status;
    }

    std::vector<VASurfaceID> surfaces;
    m_surfacePool->getSurfaceIDs(surfaces);
    if (surfaces.empty())
        return YAMI_FAIL;
    int size = surfaces.size();
    m_context = VaapiContext::create(config,
        m_videoFormatInfo.width,
        m_videoFormatInfo.height,
        0, &surfaces[0], size);

    if (!m_context) {
        ERROR("create context failed");
        return YAMI_FAIL;
    }
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderBase::terminateVA(void)
{
    INFO("base: terminate VA");
    m_config.resetConfig();
    m_surfacePool.reset();
    m_allocator.reset();
    DEBUG("surface pool is reset");
    m_context.reset();
    m_display.reset();

    m_VAStarted = false;
    return YAMI_SUCCESS;
}

void VaapiDecoderBase::setNativeDisplay(NativeDisplay * nativeDisplay)
{
    if (!nativeDisplay || nativeDisplay->type == NATIVE_DISPLAY_AUTO)
        return;

    m_externalDisplay = *nativeDisplay;
}

void VaapiDecoderBase::releaseLock(bool lockable)
{
    if (!m_surfacePool)
        return;

}

void VaapiDecoderBase::setAllocator(SurfaceAllocator* allocator)
{
    m_externalAllocator.reset(allocator,unrefAllocator);
}

SurfacePtr VaapiDecoderBase::createSurface()
{
    SurfacePtr surface;
    if (m_surfacePool) {
        surface = m_surfacePool->acquire();
    }
    return surface;
}

struct VaapiDecoderBase::VideoFrameRecycler {
    VideoFrameRecycler(const SurfacePtr& surface)
        : m_surface(surface)
    {
    }
    void operator()(VideoFrame* frame) {}
private:
    SurfacePtr m_surface;
};

YamiStatus VaapiDecoderBase::outputPicture(const PicturePtr& picture)
{
    SurfacePtr surface = picture->getSurface();
    SharedPtr<VideoFrame> frame(surface->m_frame.get(), VideoFrameRecycler(surface));
    frame->timeStamp = picture->m_timeStamp;
    m_output.push_back(frame);
    return YAMI_SUCCESS;
}

VADisplay VaapiDecoderBase::getDisplayID()
{
    if (!m_display)
        return NULL;
    return m_display->getID();
}

#define CHECK(f)                        \
    do {                                \
        if (m_videoFormatInfo.f != f) { \
            m_videoFormatInfo.f = f;    \
            changed = true;             \
        }                               \
    } while (0)

bool VaapiDecoderBase::setFormat(uint32_t width, uint32_t height, uint32_t surfaceWidth, uint32_t surfaceHeight,
    uint32_t surfaceNumber, uint32_t fourcc)
{
    bool changed = false;
    CHECK(width);
    CHECK(height);
    CHECK(surfaceWidth);
    CHECK(surfaceHeight);
    CHECK(surfaceNumber);
    CHECK(fourcc);
    //this not true, but it's only way to let user get format info
    m_VAStarted = true;
    return changed;
}

} //namespace YamiMediaCodec
