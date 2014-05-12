/*
 *  vaapisurfacepool.h - VA surface pool
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@splitted-desktop.com>
 *  Copyright (C) 2012-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li <xiaowei.li@intel.com>
 *    Author: Halley Zhao <halley.zhao@intel.com>
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

#ifndef vaapisurfacebuf_pool_h
#define vaapisurfacebuf_pool_h

#include "vaapi/vaapisurface.h"
#include "interface/VideoDecoderDefs.h"
#include <pthread.h>
#include <semaphore.h>
#include <deque>

#define INVALID_PTS ((uint64_t)-1)
#define INVALID_POC ((uint32_t)-1)
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000

class VaapiSurfaceBufferPool {
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiSurfaceBufferPool);
  public:
    VaapiSurfaceBufferPool(VADisplay display, VideoConfigBuffer * config);
    ~VaapiSurfaceBufferPool();

    VideoSurfaceBuffer *searchAvailableBuffer();
    VideoSurfaceBuffer *acquireFreeBuffer();
    VideoSurfaceBuffer *acquireFreeBufferWithWait();
    bool recycleBuffer(VideoSurfaceBuffer * buf, bool isFromRender);
    void flushPool();

    VideoSurfaceBuffer *getOutputByMinTimeStamp();
    VideoSurfaceBuffer *getOutputByMinPOC();
    VideoSurfaceBuffer *getBufferByHandler(void *graphicHandle);
    VideoSurfaceBuffer *getBufferBySurfaceID(VASurfaceID id);
    VideoSurfaceBuffer *getBufferByIndex(uint32_t index);
    VaapiSurface *getVaapiSurface(VideoSurfaceBuffer * buf);
    bool outputBuffer(VideoSurfaceBuffer * buf,
                      uint64_t timeStamp, uint32_t poc);
    bool setReferenceInfo(VideoSurfaceBuffer * buf,
                          bool referenceFrame, bool asReference);

  private:
    void debugSurfaceStatus();

    VADisplay m_display;
    VideoSurfaceBuffer **m_bufArray;
    VaapiSurface **m_surfArray;
    std::deque < uint32_t > m_freeBufferIndexList;

    uint32_t m_bufCount;
    uint32_t m_freeCount;

    bool m_useExtBuf;
    bool m_bufMapped;

    pthread_mutex_t m_lock;
    pthread_cond_t m_cond;
};

#endif
