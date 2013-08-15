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

#include <string.h>
#include <va/va_backend.h>
#include "vaapidecoder_base.h"
#include "common/log.h"

#ifdef ANDROID
#include <va/va_android.h>
#endif

#define INVALID_PTS ((uint64_t)-1)
#define INVALID_POC ((uint32_t)-1)
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000
#define ANDROID_DISPLAY_HANDLE 0x18C34078

VaapiDecoderBase::VaapiDecoderBase(const char *mimeType)
      :mDisplay(NULL),
      mVADisplay(NULL),
      mVAContext(VA_INVALID_ID),
      mVAConfig(VA_INVALID_ID),
      mRenderTarget(NULL),
      mLastReference(NULL),
      mForwardReference(NULL),
      mVAStarted(false),
      mCurrentPTS(INVALID_PTS),
      mEnableNativeBuffersFlag(false)
{

    INFO("base: construct()");
    memset(&mVideoFormatInfo, 0, sizeof(VideoFormatInfo));
    memset(&mConfigBuffer, 0, sizeof(mConfigBuffer));
    mVideoFormatInfo.mimeType = strdup(mimeType);
    mBufPool = NULL;
}

VaapiDecoderBase::~VaapiDecoderBase() 
{
    INFO("base: deconstruct()");
    stop();
    free(mVideoFormatInfo.mimeType);
    free(mVideoFormatInfo.ctxSurfaces);
}

Decode_Status 
VaapiDecoderBase::start(VideoConfigBuffer *buffer)
{
    INFO("base: start()");
 
    if (buffer == NULL) {
        return DECODE_INVALID_DATA;
    }

    mConfigBuffer = *buffer;
    mConfigBuffer.data = NULL;
    mConfigBuffer.size = 0;

    mVideoFormatInfo.width = buffer->width;
    mVideoFormatInfo.height = buffer->height;
    if (buffer->flag & USE_NATIVE_GRAPHIC_BUFFER) {
        mVideoFormatInfo.surfaceWidth = buffer->graphicBufferWidth;
        mVideoFormatInfo.surfaceHeight = buffer->graphicBufferHeight;
    }
    mLowDelay = buffer->flag & WANT_LOW_DELAY;
    mRawOutput = buffer->flag & WANT_RAW_OUTPUT;

    setupVA(buffer->surfaceNumber, buffer->profile);   

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderBase::reset(VideoConfigBuffer *buffer)
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

    mCurrentPTS = INVALID_PTS;
    mRenderTarget = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;

    mLowDelay = false;
    mRawOutput = false;

    mVideoFormatInfo.valid = false;
}

void VaapiDecoderBase::flush(void)
{

    INFO("base: flush()");
    if (mBufPool)
       mBufPool->flushPool();

    mCurrentPTS = INVALID_PTS;
    mRenderTarget = NULL;
    mLastReference = NULL;
    mForwardReference = NULL;
}

void VaapiDecoderBase::flushOutport(void) {
}


const VideoRenderBuffer* 
VaapiDecoderBase::getOutput(bool draining) 
{
    VideoSurfaceBuffer* surfBuf = NULL;
 
     if (mBufPool)
        surfBuf = mBufPool->getOutputByMinTimeStamp();

     if(!surfBuf)
        return NULL;

     return  &(surfBuf->renderBuffer);
}

Decode_Status
VaapiDecoderBase::signalRenderDone(void *graphicHandler)
{
    INFO("base: signalRenderDone()");
    VideoSurfaceBuffer *buf = NULL;
    if (graphicHandler == NULL) {
        return DECODE_SUCCESS;
    }

    if (!mBufPool) {
        ERROR("buffer pool is not initialized yet");
        return DECODE_FAIL;
    }

    if (!(buf = mBufPool->getBufferByHandler(graphicHandler))) {
        return DECODE_SUCCESS;
    }
 
    if(!mBufPool->recycleBuffer(buf, true))
       return DECODE_FAIL;

    return DECODE_FAIL;
}   

const VideoFormatInfo*
VaapiDecoderBase::getFormatInfo(void)
{

    INFO("base: getFormatInfo()");
    return &mVideoFormatInfo;
}
 
bool 
VaapiDecoderBase::checkBufferAvail(void) 
{

    INFO("base: checkBufferAvail()");
    if (!mBufPool) {
        ERROR("buffer pool is not initialized yet");
        return DECODE_FAIL;
    }

    if(mBufPool->searchAvailableBuffer())
        return true;

    return false;
}

void
VaapiDecoderBase::renderDone(VideoRenderBuffer *renderBuf)
{
    INFO("base: renderDone()");
    VideoSurfaceBuffer *buf = NULL;

    if (!mBufPool) {
        ERROR("buffer pool is not initialized yet");
        return ;
    }

    if (!(buf = mBufPool->getBufferBySurfaceID(renderBuf->surface))) {
        return;
    }
 
    mBufPool->recycleBuffer(buf, true);
}

////////////////////////////////////////////
Decode_Status 
VaapiDecoderBase::updateReference(void)
{
    Decode_Status status;
    // update reference frames
    if (mRenderTarget->referenceFrame) {
        // managing reference for MPEG4/H.263/WMV.
        // AVC should manage reference frame in a different way
        if (mForwardReference != NULL) {
            // this foward reference is no longer needed
            mForwardReference->asReferernce = false;
        }
        // Forware reference for either P or B frame prediction
        mForwardReference = mLastReference;
        mRenderTarget->asReferernce = true;

        // the last reference frame.
        mLastReference = mRenderTarget;
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

    if (mEnableNativeBuffersFlag == true) {
       numSurface = 20; //NATIVE_WINDOW_COUNT;
    }

    if (mVAStarted) {
        return DECODE_SUCCESS;
    }

    if (mVADisplay != NULL) {
        WARNING("VA is partially started.");
        return DECODE_FAIL;
    }

#ifdef ANDROID
    mDisplay = new Display;
    *mDisplay = ANDROID_DISPLAY_HANDLE;
#else
    mDisplay = XOpenDisplay(NULL);
#endif

    mVADisplay = vaGetDisplay(mDisplay);
    if (mVADisplay == NULL) {
        ERROR("vaGetDisplay failed.");
        return DECODE_DRIVER_FAIL;
    }

    int majorVersion, minorVersion;
    vaStatus = vaInitialize(mVADisplay, &majorVersion, &minorVersion);
    vaapi_check_status(vaStatus,"vaInitialize");

    VAConfigAttrib attrib;
    attrib.type = VAConfigAttribRTFormat;
    attrib.value = VA_RT_FORMAT_YUV420;

    INFO("base:the profile = %d", profile);
    vaStatus = vaCreateConfig(
                mVADisplay,
                profile,
                VAEntrypointVLD,
                &attrib,
                1,
                &mVAConfig);
    vaapi_check_status(vaStatus,"vaCreateConfig");

    mConfigBuffer.surfaceNumber = numSurface;
    mBufPool = new VaapiSurfaceBufferPool(mVADisplay, &mConfigBuffer);
    surfaces = new VASurfaceID [numSurface];
    for (i = 0; i < numSurface; i ++) {
        buf = mBufPool->getBufferByIndex(i);
        suf = mBufPool->getVaapiSurface(buf);
        surfaces[i] = suf->getID();
    }
        
    vaStatus = vaCreateContext(
                mVADisplay,
                mVAConfig,
                mVideoFormatInfo.width,
                mVideoFormatInfo.height,
                0,
                surfaces,
                numSurface,
                &mVAContext);
    vaapi_check_status(vaStatus,"vaCreateContext");

    VADisplayAttribute rotate;
    rotate.type = VADisplayAttribRotation;
    rotate.value = VA_ROTATION_NONE;
    if (mConfigBuffer.rotationDegrees == 0)
        rotate.value = VA_ROTATION_NONE;
    else if (mConfigBuffer.rotationDegrees == 90)
        rotate.value = VA_ROTATION_90;
    else if (mConfigBuffer.rotationDegrees == 180)
        rotate.value = VA_ROTATION_180;
    else if (mConfigBuffer.rotationDegrees == 270)
        rotate.value = VA_ROTATION_270;

    vaStatus = vaSetDisplayAttributes(mVADisplay, &rotate, 1);

    mVideoFormatInfo.surfaceNumber = numSurface;
    mVideoFormatInfo.ctxSurfaces = surfaces;
    if (!(mConfigBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER)) {
        mVideoFormatInfo.surfaceWidth = mVideoFormatInfo.width;
        mVideoFormatInfo.surfaceHeight = mVideoFormatInfo.height;
    }

    mVAStarted = true;
    return DECODE_SUCCESS;
}

Decode_Status 
VaapiDecoderBase::terminateVA(void) 
{

    INFO("base: terminate VA");
    if (mBufPool) {
        delete mBufPool;
        mBufPool = NULL;
    }

    if (mVAContext != VA_INVALID_ID) {
         vaDestroyContext(mVADisplay, mVAContext);
         mVAContext = VA_INVALID_ID;
    }

    if (mVAConfig != VA_INVALID_ID) {
        vaDestroyConfig(mVADisplay, mVAConfig);
        mVAConfig = VA_INVALID_ID;
    }

    if (mVADisplay) {
        vaTerminate(mVADisplay);
        mVADisplay = NULL;
    }

    if (mDisplay) {
        delete mDisplay;
        mDisplay = NULL;
    }

    mVAStarted = false;
    return DECODE_SUCCESS;
}

/* not used funtion here */
void 
VaapiDecoderBase::enableNativeBuffers(void)
{
}

Decode_Status 
VaapiDecoderBase::getClientNativeWindowBuffer(
                          void *bufferHeader,
                          void *nativeBufferHandle)
{ 
    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderBase::flagNativeBuffer(void *pBuffer)
{
    return DECODE_SUCCESS;
}

void 
VaapiDecoderBase::releaseLock()
{
}

