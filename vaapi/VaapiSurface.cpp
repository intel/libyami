/*
 * Copyright 2016 Intel Corporation
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

#include "VaapiSurface.h"

#include <string.h>

namespace YamiMediaCodec {

VaapiSurface::VaapiSurface(intptr_t id, uint32_t width, uint32_t height, uint32_t fourcc)
{
    m_frame.reset(new VideoFrame);
    memset(m_frame.get(), 0, sizeof(VideoFrame));
    m_frame->surface = id;
    m_frame->crop.x = m_frame->crop.y = 0;
    m_frame->crop.width = width;
    m_frame->crop.height = height;
    m_frame->fourcc = fourcc;
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

    if (x + width > m_width
        || y + height > m_height)
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

uint32_t VaapiSurface::getFourcc()
{
    return m_frame->fourcc;
}
}
