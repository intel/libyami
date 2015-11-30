/*
 *  surfacepool.cpp - surface pool, create surfaces from allocator and pool it
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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
