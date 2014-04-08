/*
 *  vaapidecoder_base.cpp - basic va decoder for video
 *
 *  Copyright (C) 2013 Intel Corporation
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
#include <string.h>
#include <va/va_backend.h>

#ifdef ANDROID
#include <va/va_android.h>
#endif

#define ANDROID_DISPLAY_HANDLE 0x18C34078

VaapiDecoderBase::VaapiDecoderBase(const char *mimeType)
:m_display(NULL),
m_VADisplay(NULL),
m_VAContext(VA_INVALID_ID),
m_VAConfig(VA_INVALID_ID),
m_renderTarget(NULL),
m_lastReference(NULL),
m_forwardReference(NULL),
m_VAStarted(false),
m_currentPTS(INVALID_PTS), m_enableNativeBuffersFlag(false)
{
    INFO("base: construct()");
    memset(&m_videoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&m_configBuffer, 0, sizeof(m_configBuffer));
    m_videoFormatInfo.mimeType = strdup(mimeType);
    m_bufPool = NULL;
}

VaapiDecoderBase::~VaapiDecoderBase()
{
    INFO("base: deconstruct()");
    stop();
    free(m_videoFormatInfo.mimeType);
    free(m_videoFormatInfo.ctxSurfaces);
}

Decode_Status VaapiDecoderBase::start(VideoConfigBuffer * buffer)
{
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
    }
    m_lowDelay = buffer->flag & WANT_LOW_DELAY;
    m_rawOutput = buffer->flag & WANT_RAW_OUTPUT;

    setupVA(buffer->surfaceNumber, buffer->profile);

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::reset(VideoConfigBuffer * buffer)
{
    INFO("base: reset()");
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    flush();
    terminateVA();
    start(buffer);

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
    if (m_bufPool)
        m_bufPool->flushPool();

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
    VideoSurfaceBuffer *surfBuf = NULL;

    if (m_bufPool)
        surfBuf = m_bufPool->getOutputByMinTimeStamp();

    if (!surfBuf)
        return NULL;

    return &(surfBuf->renderBuffer);
}

Decode_Status VaapiDecoderBase::signalRenderDone(void *graphicHandler)
{
    INFO("base: signalRenderDone()");
    VideoSurfaceBuffer *buf = NULL;
    if (graphicHandler == NULL) {
        return DECODE_SUCCESS;
    }

    if (!m_bufPool) {
        ERROR("buffer pool is not initialized yet");
        return DECODE_FAIL;
    }

    if (!(buf = m_bufPool->getBufferByHandler(graphicHandler))) {
        return DECODE_SUCCESS;
    }

    if (!m_bufPool->recycleBuffer(buf, true))
        return DECODE_FAIL;

    return DECODE_FAIL;
}

const VideoFormatInfo *VaapiDecoderBase::getFormatInfo(void)
{
    INFO("base: getFormatInfo()");
    return &m_videoFormatInfo;
}

bool VaapiDecoderBase::checkBufferAvail(void)
{
    INFO("base: checkBufferAvail()");
    if (!m_bufPool) {
        ERROR("buffer pool is not initialized yet");
        return DECODE_FAIL;
    }

    if (m_bufPool->searchAvailableBuffer())
        return true;

    return false;
}

void VaapiDecoderBase::renderDone(VideoRenderBuffer * renderBuf)
{
    INFO("base: renderDone()");
    VideoSurfaceBuffer *buf = NULL;

    if (!m_bufPool) {
        ERROR("buffer pool is not initialized yet");
        return;
    }

    if (!(buf = m_bufPool->getBufferBySurfaceID(renderBuf->surface))) {
        return;
    }

    m_bufPool->recycleBuffer(buf, true);
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
    VASurfaceID *surfaces;
    VideoSurfaceBuffer *buf;
    VaapiSurface *suf;
    Decode_Status status;
    VAStatus vaStatus = VA_STATUS_SUCCESS;

    if (m_enableNativeBuffersFlag == true) {
        numSurface = 20;        //NATIVE_WINDOW_COUNT;
    }

    if (m_VAStarted) {
        return DECODE_SUCCESS;
    }

    if (m_VADisplay != NULL) {
        WARNING("VA is partially started.");
        return DECODE_FAIL;
    }
#ifdef ANDROID
    m_display = new Display;
    *m_display = ANDROID_DISPLAY_HANDLE;
#else
    if (!m_display)
        m_display = XOpenDisplay(NULL);
    #if __PLATFORM_BYT__
    if(setenv("LIBVA_DRIVER_NAME","wrapper",1) == 0) {
        INFO("setting LIBVA_DRIVER_NAME to wrapper for chromeos");
    }
    #endif
#endif

    m_VADisplay = vaGetDisplay(m_display);
    if (m_VADisplay == NULL) {
        ERROR("vaGetDisplay failed.");
        return DECODE_DRIVER_FAIL;
    }

    int majorVersion, minorVersion;
    vaStatus = vaInitialize(m_VADisplay, &majorVersion, &minorVersion);
    checkVaapiStatus(vaStatus, "vaInitialize");

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    INFO("base:the profile = %d", profile);
    vaStatus = vaCreateConfig(m_VADisplay,
                              profile,
                              VAEntrypointVLD, &attrib, 1, &m_VAConfig);
    checkVaapiStatus(vaStatus, "vaCreateConfig");

    m_configBuffer.surfaceNumber = numSurface;
    m_bufPool = new VaapiSurfaceBufferPool(m_VADisplay, &m_configBuffer);
    surfaces = new VASurfaceID[numSurface];
    for (i = 0; i < numSurface; i++) {
        buf = m_bufPool->getBufferByIndex(i);
        suf = m_bufPool->getVaapiSurface(buf);
        surfaces[i] = suf->getID();
    }

    vaStatus = vaCreateContext(m_VADisplay,
                               m_VAConfig,
                               m_videoFormatInfo.width,
                               m_videoFormatInfo.height,
                               0, surfaces, numSurface, &m_VAContext);
    checkVaapiStatus(vaStatus, "vaCreateContext");

    VADisplayAttribute rotate;
    rotate.type = VADisplayAttribRotation;
    rotate.value = VA_ROTATION_NONE;
    if (m_configBuffer.rotationDegrees == 0)
        rotate.value = VA_ROTATION_NONE;
    else if (m_configBuffer.rotationDegrees == 90)
        rotate.value = VA_ROTATION_90;
    else if (m_configBuffer.rotationDegrees == 180)
        rotate.value = VA_ROTATION_180;
    else if (m_configBuffer.rotationDegrees == 270)
        rotate.value = VA_ROTATION_270;

    vaStatus = vaSetDisplayAttributes(m_VADisplay, &rotate, 1);

    m_videoFormatInfo.surfaceNumber = numSurface;
    m_videoFormatInfo.ctxSurfaces = surfaces;
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
    if (m_bufPool) {
        delete m_bufPool;
        m_bufPool = NULL;
    }

    if (m_VAContext != VA_INVALID_ID) {
        vaDestroyContext(m_VADisplay, m_VAContext);
        m_VAContext = VA_INVALID_ID;
    }

    if (m_VAConfig != VA_INVALID_ID) {
        vaDestroyConfig(m_VADisplay, m_VAConfig);
        m_VAConfig = VA_INVALID_ID;
    }

    if (m_VADisplay) {
        vaTerminate(m_VADisplay);
        m_VADisplay = NULL;
    }

    if (m_display) {
        delete m_display;
        m_display = NULL;
    }

    m_VAStarted = false;
    return DECODE_SUCCESS;
}

/* not used funtion here */
void VaapiDecoderBase::setXDisplay(Display * xDisplay)
{
    m_display = xDisplay;
}

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

void VaapiDecoderBase::releaseLock()
{
}
