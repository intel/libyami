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
#include "vaapi/VaapiSurface.h"
#include "decoder/vaapidecoder_base.h"
#include <string.h>
#include <assert.h>

namespace YamiMediaCodec{

YamiStatus VaapiDecSurfacePool::getSurface(SurfaceAllocParams* param, intptr_t* surface)
{
    VaapiDecSurfacePool* p = (VaapiDecSurfacePool*)param->user;
    return p->getSurface(surface);
}

YamiStatus VaapiDecSurfacePool::putSurface(SurfaceAllocParams* param, intptr_t surface)
{
    VaapiDecSurfacePool* p = (VaapiDecSurfacePool*)param->user;
    return p->putSurface(surface);
}

YamiStatus VaapiDecSurfacePool::getSurface(intptr_t* surface)
{
    AutoLock lock(m_lock);

    if (m_freed.empty())
        return YAMI_DECODE_NO_SURFACE;
    *surface = m_freed.front();
    m_used.insert(*surface);
    m_freed.pop_front();
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecSurfacePool::putSurface(intptr_t surface)
{
    AutoLock lock(m_lock);

    if (m_used.find(surface) == m_used.end()) {
        ERROR("put wrong surface, id = %p", (void*)surface);
        return YAMI_INVALID_PARAM;
    }
    m_used.erase(surface);
    m_freed.push_back(surface);
    return YAMI_SUCCESS;
}

DecSurfacePoolPtr VaapiDecSurfacePool::create(VideoConfigBuffer* config,
    const SharedPtr<SurfaceAllocator>& allocator)
{
    VideoDecoderConfig conf;
    conf.width = config->surfaceWidth;
    conf.height = config->surfaceHeight;
    conf.fourcc = config->fourcc;
    conf.surfaceNumber = config->surfaceNumber;
    return create(&conf, allocator);
}

DecSurfacePoolPtr VaapiDecSurfacePool::create(VideoDecoderConfig* config,
    const SharedPtr<SurfaceAllocator>& allocator)
{
    DecSurfacePoolPtr pool(new VaapiDecSurfacePool);
    if (!pool->init(config, allocator))
        pool.reset();
    return pool;
}

bool VaapiDecSurfacePool::init(VideoDecoderConfig* config,
    const SharedPtr<SurfaceAllocator>& allocator)
{
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
    if (!m_allocParams.getSurface || !m_allocParams.putSurface) {
        m_allocParams.getSurface = getSurface;
        m_allocParams.putSurface = putSurface;
        m_allocParams.user = this;
    }

    for (uint32_t i = 0; i < size; i++) {
        intptr_t s = m_allocParams.surfaces[i];
        SurfacePtr surface(new VaapiSurface(s, width, height, fourcc));

        m_surfaceMap[s] = surface.get();
        m_surfaces.push_back(surface);

        m_freed.push_back(s);
    }
    return true;
}

VaapiDecSurfacePool::VaapiDecSurfacePool()
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
    size_t size = m_surfaces.size();
    ids.reserve(size);

    for (size_t i = 0; i < size; ++i)
        ids.push_back(m_surfaces[i]->getID());
}

struct VaapiDecSurfacePool::SurfaceRecycler
{
    SurfaceRecycler(const DecSurfacePoolPtr& pool): m_pool(pool) {}
    void operator()(VaapiSurface* surface)
    {
        SurfaceAllocParams& params = m_pool->m_allocParams;
        params.putSurface(&params, (intptr_t)surface->getID());
    }

private:
    DecSurfacePoolPtr m_pool;
};

SurfacePtr VaapiDecSurfacePool::acquire()
{
    SurfacePtr surface;
    intptr_t p;
    YamiStatus status = m_allocParams.getSurface(&m_allocParams, &p);
    if (status != YAMI_SUCCESS)
        return surface;
    //we only hold this lock after getSurface, since getSurface will do the lock
    AutoLock lock(m_lock);
    SurfaceMap::iterator it = m_surfaceMap.find(p);
    if (it == m_surfaceMap.end()) {
        ERROR("surface getter turn a invalid surface ptr, %p", (void*)p);
        return surface;
    }
    surface.reset(it->second, SurfaceRecycler(shared_from_this()));
    return surface;
}

} //namespace YamiMediaCodec
