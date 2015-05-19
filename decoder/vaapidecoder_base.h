/*
 *  vaapidecoder_base.h - basic va decoder for video
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


#ifndef vaapidecoder_base_h
#define vaapidecoder_base_h

#include "common/log.h"
#include "interface/VideoDecoderInterface.h"
#include "vaapi/vaapiptrs.h"
#include "vaapidecpicture.h"
#include <deque>
#include <pthread.h>
#include <va/va.h>
#include <va/va_tpi.h>
#ifdef HAVE_VA_X11
#include <va/va_x11.h>
#else
typedef unsigned int Display;
#endif

namespace YamiMediaCodec{

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define ALIGN_MB(a) (((a) + 15 ) & (~15))

#define INVALID_PTS ((uint64_t)-1)

class VaapiDecoderBase:public IVideoDecoder {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderBase();
    virtual ~ VaapiDecoderBase();

    virtual PicturePtr createPicture(int64_t timeStamp /* , VaapiPictureStructure structure = VAAPI_PICTURE_STRUCTURE_FRAME */);
    virtual Decode_Status start(VideoConfigBuffer * buffer);
    virtual Decode_Status reset(VideoConfigBuffer * buffer);
    virtual void stop(void);
    //virtual Decode_Status decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual void flushOutport(void);
    virtual const VideoRenderBuffer *getOutput(bool draining);
    virtual Decode_Status getOutput(unsigned long draw, int64_t *timeStamp
        , int drawX, int drawY, int drawWidth, int drawHeight, bool draining = false
        , int frameX = -1, int frameY = -1, int frameWidth = -1, int frameHeight = -1);
    virtual Decode_Status getOutput(VideoFrameRawData* frame, bool draining = false);
    virtual Decode_Status populateOutputHandles(VideoFrameRawData *frames, uint32_t &frameCount);
    virtual const VideoFormatInfo *getFormatInfo(void);
    virtual void renderDone(const VideoRenderBuffer * renderBuf);
    virtual void renderDone(VideoFrameRawData* frame);
    virtual SharedPtr<VideoFrame> getOutput();

    /* native window related functions */
    void setNativeDisplay(NativeDisplay * nativeDisplay);
    void enableNativeBuffers(void);
    Decode_Status getClientNativeWindowBuffer(void *bufferHeader,
                                              void *nativeBufferHandle);
    Decode_Status flagNativeBuffer(void *pBuffer);
    void releaseLock(bool lockable=false);

    //do not use this, we will remove this in near future
    virtual VADisplay getDisplayID();
  protected:
    Decode_Status setupVA(uint32_t numSurface, VAProfile profile);
    Decode_Status terminateVA(void);
    Decode_Status updateReference(void);
    Decode_Status outputPicture(const PicturePtr& picture);
    SurfacePtr createSurface();

    NativeDisplay   m_externalDisplay;
    DisplayPtr m_display;
    ContextPtr m_context;
    bool m_VAStarted;

    VideoConfigBuffer m_configBuffer;
    VideoFormatInfo m_videoFormatInfo;


    /* allocate all surfaces need for decoding & display
     * in one pool, the pool will responsable for allocating
     * empty surface, recycle used surface.
     */
    DecSurfacePoolPtr m_surfacePool;

    /* the current render target for decoder */
    // XXX, not useful. decoding bases on VaapiPicture, rendering bases on IVideoDecoder->getOutput()
    VideoSurfaceBuffer *m_renderTarget;

    /* reference picture, h264 will not use */
    // XXX, not used. reference frame management base on VaapiPicture
    VideoSurfaceBuffer *m_lastReference;
    VideoSurfaceBuffer *m_forwardReference;

    /* hold serveral decoded picture coming from the dpb,
     * and rearrange the picture output order according to
     * customer required, for example, when output
     */
    //VideoSurfaceBuffer *outputList;
    uint64_t m_currentPTS;

  private:
    bool m_lowDelay;
    bool m_rawOutput;
    bool m_enableNativeBuffersFlag;
#ifdef __ENABLE_DEBUG__
    int renderPictureCount;
#endif

};
}
#endif                          // vaapidecoder_base_h
