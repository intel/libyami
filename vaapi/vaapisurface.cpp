/*
 *  vaapisurface.cpp just proxy for SharedPtr<VideoFrame>
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapisurface.h"

#include <string.h>

namespace YamiMediaCodec{

VaapiSurface::VaapiSurface(intptr_t id, uint32_t width, uint32_t height)
{
    m_frame.reset(new VideoFrame);
    memset(m_frame.get(), 0, sizeof(VideoFrame));
    m_frame->surface = id;
    m_frame->crop.x = m_frame->crop.y = 0;
    m_frame->crop.width = width;
    m_frame->crop.height = height;
    m_width = width;
    m_height = height;
}

VaapiSurface::VaapiSurface(const SharedPtr<VideoFrame>& frame)
{
    m_frame = frame;

    VideoRect& r = m_frame->crop;
    m_width = r.x + r.width;
    m_height = r.y + r.height;
}

VASurfaceID VaapiSurface::getID()
{
    return (VASurfaceID)m_frame->surface;
}

bool VaapiSurface::setCrop(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VideoRect& r = m_frame->crop;

    if (x + width > r.width
        || y + height > r.height)
        return false;
    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;
    return true;
}

void VaapiSurface::getCrop(uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height)
{
    const VideoRect& r = m_frame->crop;
    x = r.x;
    y = r.y;
    width = r.width;
    height = r.height;
}

}

