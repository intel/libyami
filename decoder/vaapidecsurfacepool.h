/*
 *  vaapisurfacepool.h - decoder surface pool
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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


#ifndef vaapidecsurfacepool_h
#define vaapidecsurfacepool_h

#include "common/condition.h"
#include "common/common_def.h"
#include "common/lock.h"
#include "vaapi/vaapiptrs.h"
#include "interface/VideoCommonDefs.h"
#include "interface/VideoDecoderDefs.h"
#include <deque>
#include <map>
#include <vector>
#include <va/va.h>

namespace YamiMediaCodec{

/***
 * \class VaapiDecSurfacePool
 * \brief surface pool used for decoding rendering
 * <pre>
 * 1. the surface status is described by 2 bitwise flag: | SURFACE_RENDERING | SURFACE_DECODING |
 *      SURFACE_DECODING is set when the buffer is used for decoding, usually set when decoder create a new #VaapiPicture.
 *      SURFACE_DECODING is cleared when decoder doesn't use the buffer any more, usually when decoder delete the corresponding #VaapiPicture.
 *      SURFACE_TO_RENDER is set when #VaapiPicture is ready to output (VaapiPicture::output() is called).
 *      SURFACE_TO_RENDER is cleared when VASurface is sent to client for rendering (VaapiDecoderBase::getOutput())
 *      SURFACE_RENDERING is set when VASurface is sent to client for rendering (VaapiDecoderBase::getOutput())
 *      SURFACE_RENDERING is cleared when the surface is returned back from client (VaapiDecoderBase::renderDone())
 *  if no flag is set, the buffer/surface can be reused -- associate with a new VaapiPicture
 * 2. the free surface is in a first-in-first-out queue to be friendly to graphics fence
 * 3. most functions in this class do not support multithread except recycle.
 * 4. flush need called in decoder thread and it will make all following acuireWithWait return null surface.
 *    until all surface recycled.
 *</pre>
*/
class VaapiDecSurfacePool : public EnableSharedFromThis <VaapiDecSurfacePool>
{
public:
    static DecSurfacePoolPtr create(const DisplayPtr&, VideoConfigBuffer* config,
        const SharedPtr<SurfaceAllocator>& allocator);
    void getSurfaceIDs(std::vector<VASurfaceID>& ids);
    /// get a free surface,
    /// it always return null buffer if it's flushed.
    SurfacePtr acquireWithWait();
    /// push surface to output queue
    bool output(const SurfacePtr&, int64_t timetamp);
    /// get surface from output queue
    VideoRenderBuffer* getOutput();
    /// recycle to surface pool
    void recycle(const VideoRenderBuffer * renderBuf);
    /// recycle exported video frame to surface/image pool
    //after this, acquireWithWait will always return null surface,
    //until all SurfacePtr and VideoRenderBuffer returned.
    void flush();

    /// decode thread may be blocked at acquireWithWait, wake it up and escape if required. it doesn't release internal surfaces.
    /// it can be used for drain or unclear termination (in v4l2 wrapper, STREAMOFF can be used for either flush or drain).
    void setWaitable(bool waitable);
    ~VaapiDecSurfacePool();


private:
    enum SurfaceState{
        SURFACE_FREE      = 0x00000000,
        SURFACE_DECODING  = 0x00000001,
        SURFACE_TO_RENDER = 0x00000002,
        SURFACE_RENDERING = 0x00000004
    };

    VaapiDecSurfacePool();
    bool init(const DisplayPtr& display,
              VideoConfigBuffer* config,
              const SharedPtr<SurfaceAllocator>& allocator);

    void recycleLocked(VASurfaceID, SurfaceState);
    void recycle(VASurfaceID, SurfaceState);

    //following member only change in constructor.
    DisplayPtr m_display;
    std::vector<VideoRenderBuffer> m_renderBuffers;
    std::vector<SurfacePtr> m_surfaces;
    typedef std::map<VASurfaceID, VideoRenderBuffer*> RenderMap;
    RenderMap m_renderMap;
    typedef std::map<VASurfaceID, VaapiSurface*> SurfaceMap;
    SurfaceMap m_surfaceMap;

    //free and allocted.
    std::deque<VASurfaceID> m_freed;
    typedef std::map<VASurfaceID, uint32_t> Allocated;
    Allocated m_allocated;

    /* output queue*/
    typedef std::deque<VideoRenderBuffer*> OutputQueue;
    OutputQueue m_output;

    Lock m_lock;
    Condition m_cond;
    bool m_flushing;

    //for external allocator
    SharedPtr<SurfaceAllocator> m_allocator;
    SurfaceAllocParams m_allocParams;

    struct SurfaceRecycler;
    struct SurfaceRecyclerRender;

    DISALLOW_COPY_AND_ASSIGN(VaapiDecSurfacePool);
};

} //namespace YamiMediaCodec

#endif //vaapidecsurfacepool_h
