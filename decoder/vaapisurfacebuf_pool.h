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

#include <stdint.h>
#include <limits.h>
#include "common/common_def.h"
#include "common/vaapisurface.h"
#include "interface/VideoDecoderDefs.h"
#include <pthread.h>
#include <semaphore.h>
#include <deque>

#define INVALID_PTS ((uint64_t)-1)
#define INVALID_POC INT_MAX
#define MAXIMUM_POC  0x7FFFFFFF
#define MINIMUM_POC  0x80000000
/**
 * \class VaapiSurfaceBufferPool
 * \brief surface pool used for decoding rendering
 * <pre>
 * 1. the surface status is described by 3 bitwise flag: | SURFACE_RENDERING | SURFACE_TO_RENDER | SURFACE_DECODING |
 *      SURFACE_DECODING is set when the buffer is used for decoding, usually set when decoder create a new #VaapiPicture.
 *      SURFACE_DECODING is cleared when decoder doesn't use the buffer any more, usually when decoder delete the corresponding #VaapiPicture.
 *      SURFACE_TO_RENDER is set when #VaapiPicture is ready to output (VaapiPicture::output() is called).
 *      SURFACE_TO_RENDER is cleared when VASurface is sent to client for rendering (VaapiDecoderBase::getOutput())
 *      SURFACE_RENDERING is set when VASurface is sent to client for rendering (VaapiDecoderBase::getOutput())
 *      SURFACE_RENDERING is cleared when the surface is returned back from client (VaapiDecoderBase::renderDone())
 *  if no flag is set, the buffer/surface can be reused -- associate with a new VaapiPicture
 * 2. the free surface is in a first-in-first-out queue to be friendly to graphics fence
 * 3. the output surface buffer will be managed by a queue (not done yet). (now, it depends on timestamp/poc for output order)
 *</pre>
*/
class VaapiSurfaceBufferPool {
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiSurfaceBufferPool);
  public:
    VaapiSurfaceBufferPool(VADisplay display, VideoConfigBuffer * config);
    ~VaapiSurfaceBufferPool();
    /// \brief find an free buffer/surface from pool
    VideoSurfaceBuffer *searchAvailableBuffer();
    /// \brief mark an free buffer/surface in-use for decoding (SURFACE_DECODING) if any; return NULL if no free buffer/surface available
    VideoSurfaceBuffer *acquireFreeBuffer();
    /// \brief conditionly wait until get an free buffer/surface, then mark it in-use for decoding (SURFACE_DECODING). VaapiPicture::VaapiPicture()
    VideoSurfaceBuffer *acquireFreeBufferWithWait();
    /// \brief clear corresponding flag when decoder or render(client) doesn't use the buffer surface any more.
    /// when decoder (isFromRender = false) doesn't use the buffer/surface, clear the SURFACE_DECODING flag. VaapiPicture::~VaapiPicture()
    /// when render (isFromRender = true) doesn't use the buffer/surface, clear the SURFACE_RENDERING flag. VaapiDecoderBase::renderDone().
    bool recycleBuffer(VideoSurfaceBuffer * buf, bool isFromRender);
    /// \beirf unconditionally mark all the buffer/surface are free.
    void flushPool();
    /// \brief get the oldest buffer for display
    VideoSurfaceBuffer *getOutputByMinTimeStamp();
    /// \brief get the oldest buffer for display of h264 decoder, since there is no timestamp for raw h264 stream
    VideoSurfaceBuffer *getOutputByMinPOC();
    /// \brief not interest for now, may be used for android for external buffer
    VideoSurfaceBuffer *getBufferByHandler(void *graphicHandle);
    /// \brief get the buffer structure by contained VASurfaceID
    VideoSurfaceBuffer *getBufferBySurfaceID(VASurfaceID id);
    /// \brief get the buffer structure by index number, it is useful to interate the surface pool
    VideoSurfaceBuffer *getBufferByIndex(uint32_t index);
    /// \brief get VaapiSurface from buffer structure
    VaapiSurface *getVaapiSurface(VideoSurfaceBuffer * buf);
    /// \brief mark one buffer is using by client for rendering
    bool outputBuffer(VideoSurfaceBuffer * buf,
                      uint64_t timeStamp, int32_t poc);
    /// \brief mark one surface is using by client for rendering
    bool outputSurface(VASurfaceID surface,
                      uint64_t timeStamp, int32_t poc = 0);

    /// \brief set the reference related flag for a buffer/surface.
    /// it is required for h264 dpb to evict one buffer after it is not used as reference anymore (marked by VaapiDPBManager::execRefPicMarkingSlidingWindow())
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
