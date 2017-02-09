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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapidecsurfacepool.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/VaapiSurface.h"
#include "decoder/vaapidecoder_base.h"
#include <string.h>
#include <assert.h>

namespace YamiMediaCodec{

DecSurfacePoolPtr VaapiDecSurfacePool::create(const DisplayPtr& display, VideoConfigBuffer* config,
    const SharedPtr<SurfaceAllocator>& allocator )
{
    VideoDecoderConfig conf;
    conf.width = config->width;
    conf.height = config->height;
    conf.fourcc = config->fourcc;
    conf.surfaceNumber = config->surfaceNumber;
    return create(display, &conf, allocator);
}

DecSurfacePoolPtr VaapiDecSurfacePool::create(const DisplayPtr& display, VideoDecoderConfig* config,
    const SharedPtr<SurfaceAllocator>& allocator)
{
    DecSurfacePoolPtr pool(new VaapiDecSurfacePool);
    if (!pool->init(display, config, allocator))
        pool.reset();
    return pool;
}

bool VaapiDecSurfacePool::init(const DisplayPtr& display, VideoDecoderConfig* config,
    const SharedPtr<SurfaceAllocator>& allocator)
{
    m_display = display;
    m_allocator = allocator;
    m_allocParams.width = config->width;
    m_allocParams.height = config->height;
    m_allocParams.fourcc = config->fourcc;
    m_allocParams.size = config->surfaceNumber;
    if (m_allocator->alloc(m_allocator.get(), &m_allocParams) != YAMI_SUCCESS) {
        ERROR("allocate surface failed (%dx%d), size = %d",
            m_allocParams.width, m_allocParams.height , m_allocParams.size);
        return false;
    }
    uint32_t size = m_allocParams.size;
    uint32_t width = m_allocParams.width;
    uint32_t height = m_allocParams.height;
    uint32_t fourcc = config->fourcc;

    m_renderBuffers.resize(size);
    for (uint32_t i = 0; i < size; i++) {
        SurfacePtr s(new VaapiSurface(m_allocParams.surfaces[i], width, height, fourcc));
        VASurfaceID id = s->getID();

        m_renderBuffers[i].display = m_display->getID();
        m_renderBuffers[i].surface = id;
        m_renderBuffers[i].timeStamp = 0;

        m_renderMap[id] = &m_renderBuffers[i];
        m_surfaceMap[id] = s.get();
        m_freed.push_back(id);
        m_surfaces.push_back(s);
    }
    return true;
}

VaapiDecSurfacePool::VaapiDecSurfacePool()
    :m_cond(m_lock),
    m_flushing(false)
{
    memset(&m_allocParams, 0, sizeof(m_allocParams));
}

VaapiDecSurfacePool::~VaapiDecSurfacePool()
{
    if (m_allocator && m_allocParams.surfaces) {
        m_allocator->free(m_allocator.get(), &m_allocParams);
    }
}

void VaapiDecSurfacePool::getSurfaceIDs(std::vector<VASurfaceID>& ids)
{
    //no need hold lock, it never changed from start
    assert(!ids.size());
    size_t size = m_renderBuffers.size();
    ids.reserve(size);

    for (size_t i = 0; i < size; ++i)
        ids.push_back(m_renderBuffers[i].surface);
}

struct VaapiDecSurfacePool::SurfaceRecycler
{
    SurfaceRecycler(const DecSurfacePoolPtr& pool): m_pool(pool) {}
    void operator()(VaapiSurface* surface) { m_pool->recycle(surface->getID(), SURFACE_DECODING);}
private:
    DecSurfacePoolPtr m_pool;
};

SurfacePtr VaapiDecSurfacePool::acquireWithWait()
{
    SurfacePtr surface;
    AutoLock lock(m_lock);
    while (m_freed.empty() && !m_flushing) {
        DEBUG("wait because there is no available surface from pool");
        m_cond.wait();
    }

    if (m_flushing) {
        DEBUG("input flushing, return nil surface");
        return surface;
    }

    assert(!m_freed.empty());
    VASurfaceID id = m_freed.front();
    m_freed.pop_front();
    m_allocated[id] = SURFACE_DECODING;
    VaapiSurface* s = m_surfaceMap[id];
    surface.reset(s, SurfaceRecycler(shared_from_this()));
    return surface;
}

bool VaapiDecSurfacePool::output(const SurfacePtr& surface, int64_t timeStamp)
{
    VASurfaceID id = surface->getID();

    AutoLock lock(m_lock);
    const Allocated::iterator it = m_allocated.find(id);
    if (it == m_allocated.end())
        return false;
    assert(it->second == SURFACE_DECODING);
    it->second |= SURFACE_TO_RENDER;
    VideoRenderBuffer* buffer = m_renderMap[id];

    VideoRect& crop = buffer->crop;
    surface->getCrop(crop.x, crop.y, crop.width, crop.height);
    buffer->fourcc = surface->getFourcc();

    buffer->timeStamp = timeStamp;
    DEBUG("surface=0x%x is output-able with timeStamp=%ld", surface->getID(), timeStamp);
    m_output.push_back(buffer);
    return true;
}

VideoRenderBuffer* VaapiDecSurfacePool::getOutput()
{
    AutoLock lock(m_lock);
    if (m_output.empty())
        return NULL;
    VideoRenderBuffer* buffer = m_output.front();
    m_output.pop_front();
    const Allocated::iterator it = m_allocated.find(buffer->surface);
    assert(it != m_allocated.end());
    assert(it->second & SURFACE_TO_RENDER);
    assert(!(it->second & SURFACE_RENDERING));
    //clear SURFACE_TO_RENDER and set SURFACE_RENDERING
    it->second ^= SURFACE_RENDERING | SURFACE_TO_RENDER;
    return buffer;
}

struct VaapiDecSurfacePool::SurfaceRecyclerRender
{
    SurfaceRecyclerRender(const DecSurfacePoolPtr& pool, VideoRenderBuffer* buffer): m_pool(pool), m_buffer(buffer) {}
    void operator()(VaapiSurface* surface) { m_pool->recycle(m_buffer);}
private:
    DecSurfacePoolPtr m_pool;
    VideoRenderBuffer* m_buffer;
};

void VaapiDecSurfacePool::setWaitable(bool waitable)
{
    m_flushing = !waitable;

    if (!waitable) {
        m_cond.signal();
    }
}

void VaapiDecSurfacePool::flush()
{
    AutoLock lock(m_lock);
    for (OutputQueue::iterator it = m_output.begin();
        it != m_output.end(); ++it) {
        recycleLocked((*it)->surface, SURFACE_TO_RENDER);
    }
    m_output.clear();
    //still have unreleased surface
    if (!m_allocated.empty())
        m_flushing = true;
}

void VaapiDecSurfacePool::recycleLocked(VASurfaceID id, SurfaceState flag)
{
    const Allocated::iterator it = m_allocated.find(id);
    if (it == m_allocated.end()) {
        ERROR("try to recycle %u from state %d, it's not an allocated buffer", id, flag);
        return;
    }
    it->second &= ~flag;
    if (it->second == SURFACE_FREE) {
        m_allocated.erase(it);
        m_freed.push_back(id);
        if (m_flushing && m_allocated.size() == 0)
            m_flushing = false;
        m_cond.signal();
    }
}

void VaapiDecSurfacePool::recycle(VASurfaceID id, SurfaceState flag)
{
    AutoLock lock(m_lock);
    recycleLocked(id,flag);
}

void VaapiDecSurfacePool::recycle(const VideoRenderBuffer * renderBuf)
{
    if (renderBuf < &m_renderBuffers[0]
        || renderBuf >= &m_renderBuffers[m_renderBuffers.size()]) {
        ERROR("recycle invalid render buffer");
        return;
    }
    recycle(renderBuf->surface, SURFACE_RENDERING);
}

} //namespace YamiMediaCodec
