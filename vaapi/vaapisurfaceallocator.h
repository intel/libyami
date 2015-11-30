/*
 *  vaapisurfaceallocator.cpp - create va surface for encoder, encoder
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#ifndef vaapisurfaceallocator_h
#define vaapisurfaceallocator_h
#include "common/basesurfaceallocator.h"
#include <va/va.h>

namespace YamiMediaCodec{

class VaapiSurfaceAllocator : public BaseSurfaceAllocator
{
    //extra buffer size for performance
    static const uint32_t EXTRA_BUFFER_SIZE = 5;
public:
    //we do not use DisplayPtr here since we may give this to user someday.
    //@extraSize[in] extra size add to SurfaceAllocParams.size
    VaapiSurfaceAllocator(VADisplay display, uint32_t extraSize = EXTRA_BUFFER_SIZE);
protected:
    virtual YamiStatus doAlloc(SurfaceAllocParams* params);
    virtual YamiStatus doFree(SurfaceAllocParams* params);
    virtual void doUnref();
private:
    VADisplay m_display;
    uint32_t  m_extraSize;
    DISALLOW_COPY_AND_ASSIGN(VaapiSurfaceAllocator);
};

}
#endif //vaapisurfaceallocator_h
