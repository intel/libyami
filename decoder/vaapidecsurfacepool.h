/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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


#ifndef vaapidecsurfacepool_h
#define vaapidecsurfacepool_h

#include "common/condition.h"
#include "common/common_def.h"
#include "common/lock.h"
#include "vaapi/vaapiptrs.h"
#include "VideoCommonDefs.h"
#include "VideoDecoderDefs.h"
#include <deque>
#include <map>
#include <vector>
#include <set>
#include <va/va.h>

namespace YamiMediaCodec{

struct VideoDecoderConfig;
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
    /* TODO: remove this after all caller change to VideoDecoderConfig*/
    static DecSurfacePoolPtr create(VideoConfigBuffer* config,
        const SharedPtr<SurfaceAllocator>& allocator);
    static DecSurfacePoolPtr create(VideoDecoderConfig* config,
        const SharedPtr<SurfaceAllocator>& allocator);
    void getSurfaceIDs(std::vector<VASurfaceID>& ids);
    /// get a free surface
    SurfacePtr acquire();
    ~VaapiDecSurfacePool();


private:
    VaapiDecSurfacePool();
    bool init(VideoDecoderConfig* config,
        const SharedPtr<SurfaceAllocator>& allocator);

    static YamiStatus getSurface(SurfaceAllocParams* param, intptr_t* surface);
    static YamiStatus putSurface(SurfaceAllocParams* param, intptr_t surface);
    YamiStatus getSurface(intptr_t* surface);
    YamiStatus putSurface(intptr_t surface);

    //following member only change in constructor.
    std::vector<SurfacePtr> m_surfaces;

    typedef std::map<intptr_t, VaapiSurface*> SurfaceMap;
    SurfaceMap m_surfaceMap;

    //free and allocted.
    std::deque<intptr_t> m_freed;
    std::set<intptr_t> m_used;

    Lock m_lock;

    //for external allocator
    SharedPtr<SurfaceAllocator> m_allocator;
    SurfaceAllocParams m_allocParams;

    struct SurfaceRecycler;

    DISALLOW_COPY_AND_ASSIGN(VaapiDecSurfacePool);
};

} //namespace YamiMediaCodec

#endif //vaapidecsurfacepool_h
