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

#ifndef basesurfaceallocator_h
#define basesurfaceallocator_h
#include "interface/VideoCommonDefs.h"

namespace YamiMediaCodec{

namespace details {

YamiStatus allocatSurfaces(SurfaceAllocator* thiz, SurfaceAllocParams* params);
YamiStatus freeSurfaces   (SurfaceAllocator* thiz, SurfaceAllocParams* params);
void       unrefAllocator (SurfaceAllocator* thiz);

};
/// base class for surface allocator.
/// hide details on function hooks
class BaseSurfaceAllocator : public SurfaceAllocator
{
friend YamiStatus details::allocatSurfaces(SurfaceAllocator* thiz, SurfaceAllocParams* params);
friend YamiStatus details::freeSurfaces   (SurfaceAllocator* thiz, SurfaceAllocParams* params);
friend void       details::unrefAllocator (SurfaceAllocator* thiz);

public:
    BaseSurfaceAllocator()
    {
        //hook function and data
        alloc = details::allocatSurfaces;
        free =  details::freeSurfaces;
        unref = details::unrefAllocator;
    }
    virtual ~BaseSurfaceAllocator() {}
protected:
    virtual YamiStatus doAlloc(SurfaceAllocParams* params) = 0;
    virtual YamiStatus doFree(SurfaceAllocParams* params) = 0;
    virtual void doUnref() = 0;
};

namespace details {

inline YamiStatus allocatSurfaces(SurfaceAllocator* thiz, SurfaceAllocParams* params)
{
    if (!thiz || !params)
        return YAMI_INVALID_PARAM;
    BaseSurfaceAllocator* p = (BaseSurfaceAllocator*)thiz;
    return p->doAlloc(params);
}


inline YamiStatus freeSurfaces(SurfaceAllocator* thiz, SurfaceAllocParams* params)
{
    if (!thiz || !params)
        return YAMI_INVALID_PARAM;
    BaseSurfaceAllocator* p = (BaseSurfaceAllocator*)thiz;
    return p->doFree(params);
}

inline void unrefAllocator (SurfaceAllocator* thiz)
{
    if (!thiz)
        return;
    BaseSurfaceAllocator* p = (BaseSurfaceAllocator*)thiz;
    p->doUnref();
}

} //details

} //YamiMediaCodec

#endif //basesurfaceallocator_h
