/*
 *  vaapidecoder_base.cpp - basic va decoder for video
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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

#include "vaapidecoder_base.h"
#include "common/log.h"
#include "vaapi/vaapisurfaceallocator.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapiutils.h"
#include "vaapidecsurfacepool.h"
#include <string.h>
#include <stdlib.h> // for setenv
#include <va/va_backend.h>

namespace YamiMediaCodec{
typedef VaapiDecoderBase::PicturePtr PicturePtr;

inline void unrefAllocator(SurfaceAllocator* allocator)
{
    allocator->unref(allocator);
}

VaapiDecoderBase::VaapiDecoderBase():
m_renderTarget(NULL),
m_lastReference(NULL),
m_forwardReference(NULL),
m_VAStarted(false),
m_currentPTS(INVALID_PTS), m_enableNativeBuffersFlag(false)
{
    INFO("base: construct()");
    m_externalDisplay.handle = 0,
    m_externalDisplay.type = NATIVE_DISPLAY_AUTO,
    memset(&m_videoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&m_configBuffer, 0, sizeof(m_configBuffer));
}

VaapiDecoderBase::~VaapiDecoderBase()
{
    INFO("base: deconstruct()");
    stop();
}

PicturePtr VaapiDecoderBase::createPicture(int64_t timeStamp /* , VaapiPictureStructure structure = VAAPI_PICTURE_STRUCTURE_FRAME */)
{
    PicturePtr picture;
    /*accquire one surface from m_surfacePool in base decoder  */
    SurfacePtr surface = createSurface();
    if (!surface) {
        DEBUG("create surface failed");
        return picture;
    }

    picture.reset(new VaapiDecPicture(m_context, surface, timeStamp));
    return picture;
}

Decode_Status VaapiDecoderBase::start(VideoConfigBuffer * buffer)
{
    Decode_Status status;

    INFO("base: start()");

    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    m_configBuffer = *buffer;
    m_configBuffer.data = NULL;
    m_configBuffer.size = 0;

    m_videoFormatInfo.width = buffer->width;
    m_videoFormatInfo.height = buffer->height;
    if (buffer->flag & USE_NATIVE_GRAPHIC_BUFFER) {
        m_videoFormatInfo.surfaceWidth = buffer->graphicBufferWidth;
        m_videoFormatInfo.surfaceHeight = buffer->graphicBufferHeight;
    } else {
        m_videoFormatInfo.surfaceWidth = buffer->surfaceWidth;
        m_videoFormatInfo.surfaceHeight = buffer->surfaceHeight;
    }
    m_videoFormatInfo.surfaceNumber = buffer->surfaceNumber;
    m_lowDelay = buffer->flag & WANT_LOW_DELAY;
    m_rawOutput = buffer->flag & WANT_RAW_OUTPUT;

    status = setupVA(buffer->surfaceNumber, buffer->profile);
    if (status != DECODE_SUCCESS)
        return status;

    DEBUG
        ("m_videoFormatInfo video size: %d x %d, m_videoFormatInfo surface size: %d x %d",
         m_videoFormatInfo.width, m_videoFormatInfo.height,
         m_videoFormatInfo.surfaceWidth, m_videoFormatInfo.surfaceHeight);

#ifdef __ENABLE_DEBUG__
    renderPictureCount = 0;
#endif
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::reset(VideoConfigBuffer * buffer)
{
    Decode_Status status;

    INFO("base: reset()");
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    flush();

    status = terminateVA();
    if (status != DECODE_SUCCESS)
        return status;

    status = start(buffer);
    if (status != DECODE_SUCCESS)
        return status;

    return DECODE_SUCCESS;
}

void VaapiDecoderBase::stop(void)
{
    INFO("base: stop()");
    terminateVA();

    m_currentPTS = INVALID_PTS;
    m_renderTarget = NULL;
    m_lastReference = NULL;
    m_forwardReference = NULL;

    m_lowDelay = false;
    m_rawOutput = false;

    m_videoFormatInfo.valid = false;
}

void VaapiDecoderBase::flush(void)
{

    INFO("base: flush()");
    if (m_surfacePool) {
        m_surfacePool->flush();
    }

    m_currentPTS = INVALID_PTS;
    m_renderTarget = NULL;
    m_lastReference = NULL;
    m_forwardReference = NULL;
}

struct BufferRecycler
{
    BufferRecycler(const DecSurfacePoolPtr&  pool, VideoRenderBuffer* buffer)
        :m_pool(pool), m_buffer(buffer)
    {
    }
    void operator()(VideoFrame* frame)
    {
        m_pool->recycle(m_buffer);
        delete frame;
    }

private:
    DecSurfacePoolPtr  m_pool;
    VideoRenderBuffer* m_buffer;
};

SharedPtr<VideoFrame> VaapiDecoderBase::getOutput()
{
    SharedPtr<VideoFrame> frame;
    DecSurfacePoolPtr pool = m_surfacePool;
    if (!pool)
        return frame;
    VideoRenderBuffer *buffer = pool->getOutput();
    if (buffer) {
        frame.reset(new VideoFrame, BufferRecycler(pool, buffer));
        memset(frame.get(), 0, sizeof(VideoFrame));
        frame->surface = (intptr_t)buffer->surface;
        frame->timeStamp = buffer->timeStamp;
        frame->crop.x = 0;
        frame->crop.y = 0;
        frame->crop.width = m_videoFormatInfo.width;
        frame->crop.height = m_videoFormatInfo.height;
        //TODO: get fourcc directly from surface allocator
        frame->fourcc = YAMI_FOURCC_NV12;
    }
    return frame;
}

const VideoFormatInfo *VaapiDecoderBase::getFormatInfo(void)
{
    INFO("base: getFormatInfo()");

    if (!m_VAStarted)
        return NULL;

    return &m_videoFormatInfo;
}

Decode_Status VaapiDecoderBase::updateReference(void)
{
    // update reference frames
    if (m_renderTarget->referenceFrame) {
        // managing reference for MPEG4/H.263/WMV.
        // AVC should manage reference frame in a different way
        if (m_forwardReference != NULL) {
            // this foward reference is no longer needed
            m_forwardReference->asReferernce = false;
        }
        // Forware reference for either P or B frame prediction
        m_forwardReference = m_lastReference;
        m_renderTarget->asReferernce = true;

        // the last reference frame.
        m_lastReference = m_renderTarget;
    }
    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderBase::setupVA(uint32_t numSurface, VAProfile profile)
{
    INFO("base: setup VA");

    if (m_enableNativeBuffersFlag == true) {
        numSurface = 20;        //NATIVE_WINDOW_COUNT;
    }

    if (m_VAStarted) {
        return DECODE_SUCCESS;
    }

    if (m_display) {
        WARNING("VA is partially started.");
        return DECODE_FAIL;
    }

    m_display = VaapiDisplay::create(m_externalDisplay);

    if (!m_display) {
        ERROR("failed to create display");
        return DECODE_FAIL;
    }

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;


    ConfigPtr config = VaapiConfig::create(m_display, profile, VAEntrypointVLD,&attrib, 1);
    if (!config) {
        ERROR("failed to create config");
        return DECODE_FAIL;
    }

    if (!m_externalAllocator) {
        //use internal allocator
        m_allocator.reset(new VaapiSurfaceAllocator(m_display->getID()), unrefAllocator);
    } else {
        m_allocator = m_externalAllocator;
    }

    m_configBuffer.surfaceNumber = numSurface;
    m_surfacePool = VaapiDecSurfacePool::create(m_display, &m_configBuffer, m_allocator);
    DEBUG("surface pool is created");
    if (!m_surfacePool)
        return DECODE_FAIL;
    std::vector<VASurfaceID> surfaces;
    m_surfacePool->getSurfaceIDs(surfaces);
    if (surfaces.empty())
        return DECODE_FAIL;
    int size = surfaces.size();
    m_context = VaapiContext::create(config,
                                       m_videoFormatInfo.width,
                                       m_videoFormatInfo.height,
                                       0, &surfaces[0], size);

    if (!m_context) {
        ERROR("create context failed");
        return DECODE_FAIL;
    }

    if (!m_display->setRotation(m_configBuffer.rotationDegrees)) {
        WARNING("set rotation failed");
    }


    if (!(m_configBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {
        m_videoFormatInfo.surfaceWidth = m_videoFormatInfo.width;
        m_videoFormatInfo.surfaceHeight = m_videoFormatInfo.height;
    }

    m_VAStarted = true;
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::terminateVA(void)
{
    INFO("base: terminate VA");
    m_surfacePool.reset();
    m_allocator.reset();
    DEBUG("surface pool is reset");
    m_context.reset();
    m_display.reset();

    m_VAStarted = false;
    return DECODE_SUCCESS;
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

    m_surfacePool->setWaitable(lockable);
}

void VaapiDecoderBase::setAllocator(SurfaceAllocator* allocator)
{
    m_externalAllocator.reset(allocator,unrefAllocator);
}

SurfacePtr VaapiDecoderBase::createSurface()
{
    SurfacePtr surface;
    if (m_surfacePool) {
        surface = m_surfacePool->acquireWithWait();
    }
    return surface;
}

Decode_Status VaapiDecoderBase::outputPicture(const PicturePtr& picture)
{
    //TODO: reorder poc
    return m_surfacePool->output(picture->getSurface(),
        picture->m_timeStamp)?DECODE_SUCCESS:DECODE_FAIL;
}

VADisplay VaapiDecoderBase::getDisplayID()
{
    if (!m_display)
        return NULL;
    return m_display->getID();
}

} //namespace YamiMediaCodec
