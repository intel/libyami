/*
 *  vaapisurface.h just proxy for SharedPtr<VideoFrame>
 *
 *  Copyright (C) 2016 Intel Corporation
 *    Author: Xu Guangxin <Guangxin.Xu@intel.com>
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
#ifndef vaapisurface_h
#define vaapisurface_h

#include "interface/VideoCommonDefs.h"
#include <va/va.h>
#include <stdint.h>

namespace YamiMediaCodec{

class VaapiSurface {
public:
    VaapiSurface(intptr_t id, uint32_t width, uint32_t height);
    VaapiSurface(const SharedPtr<VideoFrame>&);

    VASurfaceID getID();

    bool setCrop(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void getCrop(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height);

private:
    SharedPtr<VideoFrame> m_frame;
    uint32_t m_width;
    uint32_t m_height;
    DISALLOW_COPY_AND_ASSIGN(VaapiSurface)
};
}

#endif //vaapisurface_h
