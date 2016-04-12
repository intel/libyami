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
#include "vaapi/vaapiptrs.h"
#include "vaapi/VaapiSurface.h"
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
