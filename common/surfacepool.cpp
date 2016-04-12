/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

#include "surfacepool.h"

#include "common/log.h"

namespace YamiMediaCodec{

SharedPtr<SurfacePool>
SurfacePool::create(const SharedPtr<SurfaceAllocator>& alloc,
    uint32_t fourcc, uint32_t width, uint32_t height, uint32_t size)
{
    SharedPtr<SurfacePool> pool(new SurfacePool);
    if (YAMI_SUCCESS != pool->init(alloc, fourcc, width, height, size))
        pool.reset();
    return pool;
}

SurfacePool::SurfacePool()
{
    memset(&m_params, 0, sizeof(m_params));
}

YamiStatus SurfacePool::init(const SharedPtr<SurfaceAllocator>& alloc,
    uint32_t fourcc, uint32_t width, uint32_t height, uint32_t size)
{
    m_params.fourcc = fourcc;
    m_params.width = width;
    m_params.height = height;
    m_params.size = size;
    YamiStatus status = alloc->alloc(alloc.get(), &m_params);
    if (status != YAMI_SUCCESS)
        return status;

    //prepare surfaces for pool
    std::deque<SurfacePtr> surfaces;
    for (uint32_t i = 0; i < m_params.size; i++) {
        SurfacePtr s(new VaapiSurface(m_params.surfaces[i], width, height));
        surfaces.push_back(s);
    }
    m_pool = VideoPool<VaapiSurface>::create(surfaces);
    if (!m_pool) {
        ERROR("failed to create Surface Pool");
        return YAMI_OUT_MEMORY;
    }

    m_alloc = alloc;
    return YAMI_SUCCESS;
}

SurfacePool::~SurfacePool()
{
    if (m_alloc) {
        m_alloc->free(m_alloc.get(), &m_params);
    }
}

SurfacePtr SurfacePool::alloc()
{
    return m_pool->alloc();
}

} //YamiMediaCodec
