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


#ifndef vaapidecoder_base_h
#define vaapidecoder_base_h

#include "common/log.h"
#include "common/common_def.h"
#include "VideoDecoderInterface.h"
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

template <class B, class C> class FactoryTest;

namespace YamiMediaCodec{

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define ALIGN_MB(a) (((a) + 15 ) & (~15))

#define INVALID_PTS ((uint64_t)-1)

struct VideoDecoderConfig {
    /// it is the frame size, height is 1088 for h264 1080p stream
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;
    uint32_t surfaceNumber;
    VAProfile profile;

    VideoDecoderConfig()
    {
        resetConfig();
    }
    void resetConfig()
    {
        memset(this, 0, sizeof(*this));
        profile = VAProfileNone;
    }
};

class VaapiDecoderBase:public IVideoDecoder {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;

    VaapiDecoderBase();
    virtual ~ VaapiDecoderBase();

    virtual YamiStatus createPicture(PicturePtr& picture, int64_t timeStamp /* , VaapiPictureStructure structure = VAAPI_PICTURE_STRUCTURE_FRAME */);
    virtual YamiStatus start(VideoConfigBuffer* buffer);
    virtual YamiStatus reset(VideoConfigBuffer* buffer);
    virtual void stop(void);
    //virtual YamiStatus decode(VideoDecodeBuffer *buffer);
    virtual void flush(void);
    virtual const VideoFormatInfo *getFormatInfo(void);
    virtual SharedPtr<VideoFrame> getOutput();

    /* native window related functions */
    void setNativeDisplay(NativeDisplay * nativeDisplay);
    void releaseLock(bool lockable=false);

    void  setAllocator(SurfaceAllocator* allocator);

    //do not use this, we will remove this in near future
    virtual VADisplay getDisplayID();
  protected:
      YamiStatus setupVA(uint32_t numSurface, VAProfile profile);
      YamiStatus terminateVA(void);
      YamiStatus updateReference(void);
      YamiStatus outputPicture(const PicturePtr& picture);
    SurfacePtr createSurface();
    YamiStatus ensureProfile(VAProfile profile);

    //set format to m_videoFormatInfo, return true if something changed.
    bool setFormat(uint32_t width, uint32_t height, uint32_t surfaceWidth, uint32_t surfaceHeight,
        uint32_t surfaceNumber, uint32_t fourcc = YAMI_FOURCC_NV12);
    bool isSurfaceGeometryChanged() const;

    NativeDisplay   m_externalDisplay;
    DisplayPtr m_display;
    ContextPtr m_context;

    VideoConfigBuffer m_configBuffer;
    VideoFormatInfo m_videoFormatInfo;


    /* allocate all surfaces need for decoding & display
     * in one pool, the pool will responsable for allocating
     * empty surface, recycle used surface.
     */
    DecSurfacePoolPtr m_surfacePool;
    SharedPtr<SurfaceAllocator> m_allocator;
    SharedPtr<SurfaceAllocator> m_externalAllocator;

    bool m_VAStarted;

    uint64_t m_currentPTS;

  private:
      bool createAllocator();
      YamiStatus ensureSurfacePool();
      VideoDecoderConfig m_config;

#ifdef __ENABLE_DEBUG__
    int renderPictureCount;
    bool m_dumpSurface;
#endif

};
}
#endif                          // vaapidecoder_base_h
