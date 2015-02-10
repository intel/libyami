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

#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif
#include "vaapidecoder_base.h"
#include "common/log.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapiutils.h"
#include "vaapidecsurfacepool.h"
#include <string.h>
#include <stdlib.h> // for setenv
#include <va/va_backend.h>

#ifdef ANDROID
#include <va/va_android.h>
#endif

#define ANDROID_DISPLAY_HANDLE 0x18C34078

namespace YamiMediaCodec{
typedef VaapiDecoderBase::PicturePtr PicturePtr;

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
        ERROR("create surface failed");
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

void VaapiDecoderBase::flushOutport(void)
{
}

const VideoRenderBuffer *VaapiDecoderBase::getOutput(bool draining)
{
    if (!m_surfacePool)
        return NULL;
    if (draining)
        flushOutport();

    VideoRenderBuffer *buffer = m_surfacePool->getOutput();

#ifdef __ENABLE_DEBUG__
    if (buffer) {
        renderPictureCount++;
        DEBUG("getOutput renderPictureCount: %d", renderPictureCount);
    }
#endif

    return buffer;
}

Decode_Status VaapiDecoderBase::getOutput(unsigned long draw, int64_t *timeStamp
    , int drawX, int drawY, int drawWidth, int drawHeight, bool draining
    , int frameX, int frameY, int frameWidth, int frameHeight)
{
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    const VideoRenderBuffer *renderBuffer = getOutput(draining);

    if (!renderBuffer)
        return RENDER_NO_AVAILABLE_FRAME;

    if (frameX == -1 && frameY == -1 && frameWidth == -1 && frameHeight == -1) {
        frameX = 0;
        frameY = 0;
        frameWidth = m_videoFormatInfo.width;
        frameHeight = m_videoFormatInfo.height;
    }

    if (!draw || drawX < 0 || drawY < 0 || drawWidth <=0 || drawHeight <=0
        || frameX < 0 || frameY < 0 || frameWidth <= 0 || frameHeight <= 0)
        return RENDER_INVALID_PARAMETER;

#if __ENABLE_X11__
    vaStatus = vaPutSurface(m_display->getID(), renderBuffer->surface,
            draw, drawX, drawY, drawWidth, drawHeight,
            frameX, frameY, frameWidth, frameHeight,
            NULL,0,0);
#else
    vaStatus = VA_STATUS_ERROR_OPERATION_FAILED;
    ERROR("vaPutSurface is not supported when libva-x11 backend is disable during build (--disable-x11)");
#endif

    if (vaStatus != VA_STATUS_SUCCESS)
        return RENDER_FAIL;

    *timeStamp = renderBuffer->timeStamp;

    renderDone(renderBuffer);
    return RENDER_SUCCESS;
}

const VideoFormatInfo *VaapiDecoderBase::getFormatInfo(void)
{
    INFO("base: getFormatInfo()");

    if (!m_VAStarted)
        return NULL;

    return &m_videoFormatInfo;
}

Decode_Status VaapiDecoderBase::getOutput(VideoFrameRawData* frame, bool draining)
{
    if (!m_surfacePool)
        return RENDER_NO_AVAILABLE_FRAME;

    if (!frame)
        return DECODE_INVALID_DATA;

    if (draining)
        flushOutport();

    if (!m_surfacePool->getOutput(frame))
        return RENDER_NO_AVAILABLE_FRAME;

#ifdef __ENABLE_DEBUG__
    renderPictureCount++;
    DEBUG("getOutput renderPictureCount=%d, timeStamp=%ld", renderPictureCount, frame->timeStamp);
#endif

    return RENDER_SUCCESS;
}

Decode_Status VaapiDecoderBase::populateOutputHandles(VideoFrameRawData *frames, uint32_t &frameCount)
{
    if (!m_surfacePool)
        return RENDER_NO_AVAILABLE_FRAME;

    if (!m_surfacePool->populateOutputHandles(frames, frameCount))
        return RENDER_NO_AVAILABLE_FRAME;
    return RENDER_SUCCESS;
}

void VaapiDecoderBase::renderDone(const VideoRenderBuffer * renderBuf)
{
    INFO("base: renderDone()");
    if (!m_surfacePool) {
        ERROR("surface pool is not initialized yet");
        return;
    }
    m_surfacePool->recycle(renderBuf);
}

void VaapiDecoderBase::renderDone(VideoFrameRawData* frame)
{
    INFO("base: renderDone()");
    if (!m_surfacePool) {
        ERROR("surface pool is not initialized yet");
        return;
    }
    m_surfacePool->recycle(frame);
}

Decode_Status VaapiDecoderBase::updateReference(void)
{
    Decode_Status status;
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
    uint32_t i;
    VideoSurfaceBuffer *buf;
    VaapiSurface *suf;
    Decode_Status status;
    VAStatus vaStatus = VA_STATUS_SUCCESS;
    const char* driver_name = NULL;

    if (m_enableNativeBuffersFlag == true) {
        numSurface = 20;        //NATIVE_WINDOW_COUNT;
    }

    if (m_VAStarted) {
        return DECODE_SUCCESS;
    }

    if (m_display != NULL) {
        WARNING("VA is partially started.");
        return DECODE_FAIL;
    }

#if __PLATFORM_BYT__
    driver_name = "wrapper";
    if (setenv("LIBVA_DRIVER_NAME", "wrapper", 1) == 0) {
        INFO("setting LIBVA_DRIVER_NAME to wrapper for chromeos");
    }
#endif

    if (profile == VAProfileVP9Version0) {
       driver_name = "hybrid";
       if (setenv("LIBVA_DRIVER_NAME", "hybrid", 1) == 0) {
           INFO("setting LIBVA_DRIVER_NAME to hybrid for VP9 profile");
       }
    }
    else {
           INFO("profile: %d, will use legacy i965 driver", profile);
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

    m_configBuffer.surfaceNumber = numSurface;
    m_surfacePool = VaapiDecSurfacePool::create(m_display, &m_configBuffer);
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
        return DECODE_FAIL;
    }


    if (!(m_configBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {
        m_videoFormatInfo.surfaceWidth = m_videoFormatInfo.width;
        m_videoFormatInfo.surfaceHeight = m_videoFormatInfo.height;
    }

    // unset the env variable here
    if (driver_name) {
       unsetenv("LIBVA_DRIVER_NAME");
    }

    m_VAStarted = true;
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::terminateVA(void)
{
    INFO("base: terminate VA");
    m_surfacePool.reset();
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

/* not used funtion here */
void VaapiDecoderBase::enableNativeBuffers(void)
{
}

Decode_Status
    VaapiDecoderBase::getClientNativeWindowBuffer(void *bufferHeader,
                                                  void *nativeBufferHandle)
{
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::flagNativeBuffer(void *pBuffer)
{
    return DECODE_SUCCESS;
}

void VaapiDecoderBase::releaseLock(bool lockable=false)
{
    if (!m_surfacePool)
        return;

    m_surfacePool->setWaitable(lockable);
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

} //namespace YamiMediaCodec
