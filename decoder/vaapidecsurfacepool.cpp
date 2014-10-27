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
const uint32_t IMAGE_POOL_SIZE = 8;

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
    VideoRenderBuffer* buffer = m_renderMap[id];
    buffer->timeStamp = timeStamp;
    DEBUG("surface=0x%x is output-able with timeStamp=%ld", surface->getID(), timeStamp);
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

struct VaapiDecSurfacePool::SurfaceRecyclerRender
{
    SurfaceRecyclerRender(const DecSurfacePoolPtr& pool, VideoRenderBuffer* buffer): m_pool(pool), m_buffer(buffer) {}
    void operator()(VaapiSurface* surface) { m_pool->recycle(m_buffer);}
private:
    DecSurfacePoolPtr m_pool;
    VideoRenderBuffer* m_buffer;
};

bool VaapiDecSurfacePool::getOutput(VideoFrameRawData* frame)
{
    if (!frame)
        return false;

    VideoRenderBuffer *buffer = getOutput();

    if (!buffer)
        return false;

    SurfacePtr surface;
    VaapiSurface *srf = m_surfaceMap[buffer->surface];
    ASSERT(srf);
    surface.reset(srf, SurfaceRecyclerRender(shared_from_this(), buffer));
    ImagePtr image = VaapiImage::derive(surface);
    if (!image)
        return false;

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
    }

    VideoDataMemoryType memoryType = frame->memoryType;
    ImageRawPtr rawImage = image->map(frame->memoryType);
    if (!rawImage)
        return false;
    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        return rawImage->copyTo((uint8_t *)frame->handle, frame->offset, frame->pitch);
    }
    if (!rawImage->getHandle(frame->handle, frame->offset, frame->pitch))
        return false;

    frame->width = image->getWidth();
    frame->height = image->getHeight();
    frame->internalID = image->getID();
    frame->fourcc = image->getFormat();
    frame->timeStamp = buffer->timeStamp;
    {
        AutoLock lock(m_exportFramesLock);
        m_exportFrames[image->getID()] = rawImage;
    }
    return true;
}

bool VaapiDecSurfacePool::populateOutputHandles(VideoFrameRawData *frames, uint32_t &frameCount)
{
    int i;
    if (!frameCount) { // output frame count negotiation
        ASSERT(frames);
        if (frames[0].fourcc && frames[0].fourcc != VA_FOURCC_NV12)
            frameCount = IMAGE_POOL_SIZE;
        else //  export the video frame as its internal format
            frameCount = m_renderBuffers.size();
        return true;
    }

    ASSERT(frameCount);
    // if necessary, create the image pool for fourcc conversion
    if (frames[0].fourcc && frames[0].fourcc != VA_FOURCC_NV12) {
        if (!m_imagePool) {
            DEBUG_FOURCC("create image pool with fourcc", frames[0].fourcc);
            ASSERT(frames[0].fourcc && frames[0].width && frames[0].height);
            m_imagePool = VaapiImagePool::create(m_display, frames[0].fourcc, frames[0].width, frames[0].height, frameCount);
        }
        ASSERT(m_imagePool);
    } else {
        // TODO, export the video frame with native format, mesa hasn't support planar YUV texture yet
        ASSERT(0);
    }

    VideoDataMemoryType memoryType = frames[0].memoryType;
    ASSERT(memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME || memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
    for (i=0; i<frameCount; i++) {
        ImagePtr image = m_imagePool->acquireWithWait();
        ASSERT(image);
        ImageRawPtr rawImage = image->map(frames[i].memoryType);
        ASSERT(rawImage);
        if (!rawImage)
            return false;
        if (!rawImage->getHandle(frames[i].handle, frames[i].offset, frames[i].pitch)) {
            ASSERT(0);
            return false;
        }

        frames[i].width = image->getWidth();
        frames[i].height = image->getHeight();
        frames[i].internalID = image->getID();
        frames[i].fourcc = image->getFormat();

        {
            AutoLock lock(m_exportFramesLock);
            m_exportFrames[image->getID()] = rawImage; // hold a reference of rawImage until an associate EGLImage is created and bound to a texture
        }
    }

    // assume texture binding (between video frame and texture) is kept even after vaReleaseBufferHandle().
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
