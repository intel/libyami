/*
 *  vaapidecsurfacepool.cpp - surface pool for decoder
 *
 *  Copyright (C) 2014 Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "vaapidecsurfacepool.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapisurface.h"
#include "vaapi/vaapiimagepool.h"
#include <string.h>
#include <assert.h>

namespace YamiMediaCodec{
#define IMAGE_POOL_SIZE 8
class VaapiDecSurfacePool::ExportFrameInfo {
  public:
    ExportFrameInfo(SurfacePtr surface, ImagePtr image):m_surface(surface), m_image(image) {};
    ~ExportFrameInfo() {};
  private:
    SurfacePtr  m_surface;
    ImagePtr    m_image;
};

DecSurfacePoolPtr VaapiDecSurfacePool::create(const DisplayPtr& display, VideoConfigBuffer* config)
{
    DecSurfacePoolPtr pool;
    std::vector<SurfacePtr> surfaces;
    size_t size = config->surfaceNumber;
    surfaces.reserve(size);
    assert(!(config->flag & WANT_SURFACE_PROTECTION));
    assert(!(config->flag & USE_NATIVE_GRAPHIC_BUFFER));
    assert(!(config->flag & WANT_RAW_OUTPUT));
    for (size_t i = 0; i < size; ++i) {
        SurfacePtr s = VaapiSurface::create(display, VAAPI_CHROMA_TYPE_YUV420,
                                   config->width,config->height,NULL,0);
        if (!s)
            return pool;
        surfaces.push_back(s);
    }
    pool.reset(new VaapiDecSurfacePool(display, surfaces));
}

VaapiDecSurfacePool::VaapiDecSurfacePool(const DisplayPtr& display, std::vector<SurfacePtr> surfaces):
    m_display(display),
    m_cond(m_lock),
    m_flushing(false)
{
    size_t size = surfaces.size();
    m_surfaces.swap(surfaces);
    m_renderBuffers.resize(size);
    for (size_t i = 0; i < size; ++i) {
        const SurfacePtr& s = m_surfaces[i];
        VASurfaceID id = m_surfaces[i]->getID();
        m_renderBuffers[i].display = display->getID();
        m_renderBuffers[i].surface = id;
        m_renderBuffers[i].timeStamp = 0;

        m_renderMap[id] = &m_renderBuffers[i];
        m_surfaceMap[id] = s.get();
        m_freed.push_back(id);
    }
}

void VaapiDecSurfacePool::getSurfaceIDs(std::vector<VASurfaceID>& ids)
{
    //no need hold lock, it never changed from start
    assert(!ids.size());
    size_t size = m_renderBuffers.size();
    ids.reserve(size);

    for (size_t i = 0; i < size; ++i)
        ids.push_back(m_renderBuffers[i].surface);
}

struct VaapiDecSurfacePool::SurfaceRecycler
{
    SurfaceRecycler(const DecSurfacePoolPtr& pool): m_pool(pool) {}
    void operator()(VaapiSurface* surface) { m_pool->recycle(surface->getID(), SURFACE_DECODING);}
private:
    DecSurfacePoolPtr m_pool;
};

struct VaapiDecSurfacePool::SurfaceRecyclerRender
{
    SurfaceRecyclerRender(const DecSurfacePoolPtr& pool): m_pool(pool) {}
    void operator()(VaapiSurface* surface) { m_pool->recycle(surface->getID(), SURFACE_RENDERING);}
private:
    DecSurfacePoolPtr m_pool;
};

SurfacePtr VaapiDecSurfacePool::acquireWithWait()
{
    SurfacePtr surface;
    AutoLock lock(m_lock);
    if (m_flushing) {
        ERROR("uppper layer bug, only support flush in decode thread");
        return surface;
    }
    while (m_freed.empty())
        m_cond.wait();

    if (m_flushing) {
        ERROR("uppper layer bug, only support flush in decode thread");
        return surface;
    }

    assert(!m_freed.empty());
    VASurfaceID id = m_freed.front();
    m_freed.pop_front();
    m_allocated[id] = SURFACE_DECODING;
    VaapiSurface* s = m_surfaceMap[id];
    surface.reset(s, SurfaceRecycler(shared_from_this()));
    return surface;
}

bool VaapiDecSurfacePool::output(const SurfacePtr& surface, int64_t timeStamp)
{
    VASurfaceID id = surface->getID();

    AutoLock lock(m_lock);
    const Allocated::iterator it = m_allocated.find(id);
    if (it == m_allocated.end())
        return false;
    assert(it->second == SURFACE_DECODING);
    it->second |= SURFACE_TO_RENDER;
    VideoRenderBuffer* buffer = m_renderMap[id];;
    m_output.push_back(buffer);
    return true;
}

VideoRenderBuffer* VaapiDecSurfacePool::getOutput()
{
    AutoLock lock(m_lock);
    if (m_output.empty())
        return NULL;
    VideoRenderBuffer* buffer = m_output.front();
    m_output.pop_front();
    const Allocated::iterator it = m_allocated.find(buffer->surface);
    assert(it != m_allocated.end());
    assert(it->second & SURFACE_TO_RENDER);
    assert(!(it->second & SURFACE_RENDERING));
    //clear SURFACE_TO_RENDER and set SURFACE_RENDERING
    it->second ^= SURFACE_RENDERING | SURFACE_TO_RENDER;
    return buffer;
}

bool VaapiDecSurfacePool::getOutput(VideoFrameRawData* frame)
{
    VideoRenderBuffer *buffer = getOutput();
    SurfacePtr surface;
    bool useConvertImage = false;

    if (!frame || !buffer)
        return false;

    VaapiSurface *srf = m_surfaceMap[buffer->surface];
    ASSERT(srf);
    surface.reset(srf, SurfaceRecyclerRender(shared_from_this()));
    ImagePtr image = VaapiImage::derive(surface);
    ASSERT(image);

    if (frame->fourcc && image->getFormat() != frame->fourcc) {
        if (!m_imagePool) {
            int width = frame->width;
            int height = frame->height;
            if (!width || !height) {
                width = surface->getWidth();
                height = surface->getHeight();
            }
            DEBUG_FOURCC("create image pool with fourcc", frame->fourcc);
            m_imagePool = VaapiImagePool::create(m_display, frame->fourcc, width, height, IMAGE_POOL_SIZE);
        }
        ASSERT(m_imagePool);
        image = m_imagePool->acquireWithWait();
        ASSERT(image);
        if (!surface->getImage(image)) {
            ASSERT(0);
            return false;
        }
        useConvertImage = true;
    }

    VaapiImageRaw *rawImage = image->map(frame->memoryType);
    switch (frame->memoryType) {
    case VIDEO_DATA_MEMORY_TYPE_RAW_COPY: {
        ASSERT(frame->handle);
        int row, col;
        switch(frame->fourcc) {
        case 0:
        case VA_FOURCC_NV12: {
            // Y plane
            int rowCopy = std::min(rawImage->height, frame->height);
            int colCopy = std::min(image->getWidth(), frame->width);
            for (row = 0; row < rowCopy; row++) {
                memcpy ((uint8_t*)(frame->handle) + frame->offset[0] + frame->pitch[0] * row, (uint8_t*)rawImage->handles[0]+rawImage->strides[0] * row, colCopy);
            }

            // UV plane
            rowCopy = std::min((rawImage->height+1)/2, (frame->height+1)/2);
            colCopy = std::min((rawImage->width+1)/2*2, (frame->width+1)/2*2);
            for (row = 0; row < rowCopy; row++) {
                memcpy ((uint8_t*)(frame->handle) + frame->offset[1] + frame->pitch[1] * row, (uint8_t*)rawImage->handles[1]+rawImage->strides[1] * row, colCopy);
            }
        }
        break;
        case VA_FOURCC_RGBX:
        case VA_FOURCC_RGBA:
        case VA_FOURCC_BGRX:
        case VA_FOURCC_BGRA: {
            int rowCopy = std::min(rawImage->height, frame->height);
            int colCopy = std::min(image->getWidth(), frame->width) * 4;
            for (row = 0; row < rowCopy; row++) {
                memcpy ((uint8_t*)(frame->handle) + frame->offset[0] + frame->pitch[0] * row, (uint8_t*)rawImage->handles[0]+rawImage->strides[0] * row, colCopy);
            }
        }
        break;
        default:
            ASSERT(0);
        break;
        }
        image->unmap();
    }
    break;
    case VIDEO_DATA_MEMORY_TYPE_RAW_POINTER:
    case VIDEO_DATA_MEMORY_TYPE_DRM_NAME:
    case VIDEO_DATA_MEMORY_TYPE_DMA_BUF:
        frame->width = rawImage->width;
        frame->height= rawImage->height;
        frame->fourcc = rawImage->format;

        for (int i=0; i<rawImage->numPlanes; i++) {
            frame->offset[i] = rawImage->handles[i] - rawImage->handles[0];
            frame->pitch[i] = rawImage->strides[i];
        }
        frame->handle = rawImage->handles[0];
    break;
    default:
        ASSERT(0);
    break;
    }


    if (frame->memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        return true;
    }

    if (useConvertImage) {
        surface.reset();
    }

    frame->internalID = image->getID();
    ExportFramePtr exportedBuffer(new ExportFrameInfo(surface, image));
    {
        AutoLock lock(m_exportFramesLock);
        m_exportFrames[image->getID()] = exportedBuffer;
    }
    return true;
}
void VaapiDecSurfacePool::flush()
{
    AutoLock lock(m_lock);
    for (OutputQueue::iterator it = m_output.begin();
        it != m_output.end(); ++it) {
        recycleLocked((*it)->surface, SURFACE_TO_RENDER);
    }
    m_output.clear();
    //still have unreleased surface
    if (!m_allocated.empty())
        m_flushing = true;
}

void VaapiDecSurfacePool::recycleLocked(VASurfaceID id, SurfaceState flag)
{
    const Allocated::iterator it = m_allocated.find(id);
    if (it == m_allocated.end()) {
        ERROR("try to recycle %p from state %d, it's not an allocated buffer", id, flag);
        return;
    }
    it->second &= ~flag;
    if (it->second == SURFACE_FREE) {
        m_allocated.erase(it);
        m_freed.push_back(id);
        if (m_flushing && m_allocated.size() == 0)
            m_flushing = false;
        m_cond.signal();
    }
}

void VaapiDecSurfacePool::recycle(VASurfaceID id, SurfaceState flag)
{
    AutoLock lock(m_lock);
    recycleLocked(id,flag);
}

void VaapiDecSurfacePool::recycle(VideoRenderBuffer * renderBuf)
{
    if (renderBuf < &m_renderBuffers[0]
        || renderBuf >= &m_renderBuffers[m_renderBuffers.size()]) {
        ERROR("recycle invalid render buffer");
        return;
    }
    recycle(renderBuf->surface, SURFACE_RENDERING);
}

void VaapiDecSurfacePool::recycle(VideoFrameRawData* frame)
{
    AutoLock lock(m_exportFramesLock);

    if (!frame || frame->memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY)
        return;

    m_exportFrames.erase(frame->internalID);
}

} //namespace YamiMediaCodec
