/*
 *  surfacepool.h - surface pool, create surfaces from allocator and pool it
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

#ifndef surfacepool_h
#define surfacepool_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/log.h"
#include "common/videopool.h"
#include "interface/VideoCommonDefs.h"
/* we should not include vaapi surface here,
 * but we have no choose before we make VaapiSurface more generic,
 * and rename it to Surface
 */
#include "vaapi/vaapisurface.h"
#include <vector>

namespace YamiMediaCodec{

class SurfacePool
{
public:
    static SharedPtr<SurfacePool>
    create(const SharedPtr<SurfaceAllocator>& alloc,
           uint32_t fourcc, uint32_t width, uint32_t height, uint32_t size);

    /**
     * allocator surface from pool, if no avaliable surface it will return NULL
     */
    SurfacePtr alloc();

    /**
     * peek surfaces from surface pool.
     */
    template <class S>
    void peekSurfaces(std::vector<S>& surfaces);
    ~SurfacePool();
private:
    SurfacePool();

    YamiStatus init(const SharedPtr<SurfaceAllocator>& alloc,
           uint32_t fourcc, uint32_t width, uint32_t height, uint32_t size);

    SharedPtr<SurfaceAllocator>         m_alloc;
    SurfaceAllocParams                  m_params;
    SharedPtr<VideoPool<VaapiSurface> > m_pool;
    DISALLOW_COPY_AND_ASSIGN(SurfacePool)

};

template <class S>
void SurfacePool::peekSurfaces(std::vector<S>& surfaces)
{
    ASSERT(surfaces.size() == 0);
    ASSERT(m_alloc);
    for (uint32_t i = 0; i < m_params.size; i++) {
        surfaces.push_back((S)m_params.surfaces[i]);
    }
}

} //YamiMediaCodec

#endif //#define surfacepool_h
