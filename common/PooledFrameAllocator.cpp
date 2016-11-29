/*
 * Copyright (C) 2016 Intel Corporation. All rights reserved.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "PooledFrameAllocator.h"
#include <common/log.h>

namespace YamiMediaCodec {

struct SurfaceDestoryer
{
private:
    SharedPtr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
public:
    SurfaceDestoryer(const SharedPtr<VADisplay>& display, std::vector<VASurfaceID>& surfaces)
        :m_display(display)
    {
        m_surfaces.swap(surfaces);
    }
    void operator()(VideoPool<VideoFrame>* pool)
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
        delete pool;
    }

};


PooledFrameAllocator::PooledFrameAllocator(const SharedPtr<VADisplay>& display, int poolsize)
    :m_display(display)
     , m_poolsize(poolsize)
{
}
bool PooledFrameAllocator::setFormat(uint32_t fourcc, int width, int height)
{
    std::vector<VASurfaceID> surfaces;
    surfaces.resize(m_poolsize);

    VASurfaceAttrib attrib;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = fourcc;

    VAStatus status = vaCreateSurfaces(*m_display, VA_RT_FORMAT_YUV420, width, height,
            &surfaces[0], surfaces.size(),
            &attrib, 1);

    if (status != VA_STATUS_SUCCESS) {
        ERROR("create surface failed, %s", vaErrorStr(status));
        return false;
    }
    std::deque<SharedPtr<VideoFrame> > buffers;
    for (size_t i = 0;  i < surfaces.size(); i++) {
        SharedPtr<VideoFrame> f(new VideoFrame);
        memset(f.get(), 0, sizeof(VideoFrame));
        f->crop.width = width;
        f->crop.height = height;
        f->surface = (intptr_t)surfaces[i];
        f->fourcc = fourcc;
        buffers.push_back(f);
    }
    m_pool.reset(new VideoPool<VideoFrame>(buffers), SurfaceDestoryer(m_display, surfaces));
    return true;
}

SharedPtr<VideoFrame> PooledFrameAllocator::alloc()
{
    return  m_pool->alloc();
}

} //namespace
