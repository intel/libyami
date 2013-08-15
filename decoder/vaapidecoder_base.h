/*
 *  vaapidecoder_base.h - basic va decoder for video 
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


#ifndef VAAPI_DECODER_BASE_H_
#define VAAPI_DECODER_BASE_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pthread.h>
#include <va/va.h>
#include <va/va_tpi.h>
#ifdef HAVE_VA_X11
#include <va/va_x11.h>
#endif
#include "interface/VideoDecoderInterface.h"
#include "vaapisurfacebuf_pool.h"
#include "common/log.h"

#ifndef HAVE_VA_X11
typedef unsigned int Display;
#endif

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define ALIGN_MB(a) (((a) + 15 ) & (~15))

class VaapiDecoderBase : public IVideoDecoder {
public:
    VaapiDecoderBase(const char *mimeType);
    virtual ~VaapiDecoderBase();

    virtual Decode_Status start(VideoConfigBuffer *buffer);
    virtual Decode_Status reset(VideoConfigBuffer *buffer) ;
    virtual void stop(void);
    //virtual Decode_Status decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual void flushOutport(void);
    virtual const VideoRenderBuffer* getOutput(bool draining = false);
    virtual Decode_Status signalRenderDone(void * graphichandler);
    virtual const VideoFormatInfo* getFormatInfo(void);
    virtual bool checkBufferAvail(void);
    virtual void renderDone(VideoRenderBuffer *renderBuf);

    /* native window related functions */
    void enableNativeBuffers(void);
    Decode_Status getClientNativeWindowBuffer(void *bufferHeader,
                  void *nativeBufferHandle);
    Decode_Status flagNativeBuffer(void *pBuffer);
    void releaseLock();

protected:
    Decode_Status setupVA(uint32_t numSurface, VAProfile profile);
    Decode_Status terminateVA(void);
    Decode_Status updateReference(void);

    Display     *mDisplay;
    VADisplay    mVADisplay;
    VAContextID  mVAContext;
    VAConfigID   mVAConfig;
    bool         mVAStarted;
 
    VideoConfigBuffer mConfigBuffer;
    VideoFormatInfo mVideoFormatInfo;

    /* allocate all surfaces need for decoding & display
     * in one pool, the pool will responsable for allocating
     * empty surface, recycle used surface.
     */
    VaapiSurfaceBufferPool* mBufPool;
    /* the current render target for decoder */
    VideoSurfaceBuffer *mRenderTarget;

    /* reference picture, h264 will not use */
    VideoSurfaceBuffer *mLastReference;
    VideoSurfaceBuffer *mForwardReference;
    
    /* hold serveral decoded picture coming from the dpb,
     * and rearrange the picture output order according to
     * customer required, for example, when output
     */
    //VideoSurfaceBuffer *outputList;
    uint64_t mCurrentPTS; 
 
private:
    bool mLowDelay;
    bool mRawOutput;
    bool mEnableNativeBuffersFlag;
};

#endif  // VIDEO_DECODER_BASE_H_
