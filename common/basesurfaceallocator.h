/*
 *  basesurfaceallocator.h  hide allo free function hook for derive class
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
