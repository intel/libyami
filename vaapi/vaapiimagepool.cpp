/*
 *  vaapidecimagepool.cpp - image pool
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Zhao, Halley <halley.zhao@intel.com>
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
#include "vaapiimagepool.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapiimage.h"
#include <string.h>

namespace YamiMediaCodec{

ImagePoolPtr VaapiImagePool::create(const DisplayPtr& display, uint32_t format, int32_t width, int32_t height, int32_t count)
{
    ImagePoolPtr pool;
    std::vector<ImagePtr> images;
    images.reserve(count);
    for (int32_t i = 0; i < count; ++i) {
        ImagePtr image = VaapiImage::create(display, format, width, height);
        if (!image)
            return pool;
        images.push_back(image);
    }
    ImagePoolPtr temp(new VaapiImagePool(images));
    pool = temp;
    return pool;
}

VaapiImagePool::VaapiImagePool(std::vector<ImagePtr> images):
    m_cond(m_lock),
    m_flushing(false)
{
    m_poolSize = images.size();
    m_images.swap(images);
    DEBUG("m_poolSize: %d", m_poolSize);
    for (int32_t i = 0; i < m_poolSize; ++i) {
        m_freeIndex.push_back(i);
        m_indexMap[m_images[i]->getID()] = i;
        DEBUG("image pool index: %d, image ID: 0x%x", i, m_images[i]->getID());
    }
}

struct VaapiImagePool::ImageRecycler
{
    ImageRecycler(const ImagePoolPtr& pool): m_pool(pool) {}
    void operator()(VaapiImage* image)
    {
        if (!image)
            return;
        m_pool->recycleID(image->getID());
    }
private:
    ImagePoolPtr m_pool;
};

void VaapiImagePool::setWaitable(bool waitable)
{
    m_flushing = !waitable;
    if (!waitable)
        m_cond.signal();
}

ImagePtr VaapiImagePool::acquireWithWait()
{
    ImagePtr image;
    AutoLock lock(m_lock);

    if (m_flushing) {
        ERROR("flushing, no new image available");
        return image;
    }
    while (m_freeIndex.empty() && !m_flushing)
        m_cond.wait();

    if (m_flushing) {
        ERROR("flushing, no new image available");
        return image;
    }

    ASSERT(!m_freeIndex.empty());

    int32_t index = m_freeIndex.front();
    ASSERT(index >=0 && index < m_poolSize);
    m_freeIndex.pop_front();

    image.reset(m_images[index].get(), ImageRecycler(shared_from_this()));
    return image;
}

void VaapiImagePool::flush()
{
    AutoLock lock(m_lock);
    if ( m_freeIndex.size() != (uint32_t)m_poolSize)
        m_flushing = true;
}

void VaapiImagePool::recycleID(VAImageID imageID)
{
    AutoLock lock(m_lock);
    int32_t index = -1;
    const MapImageIDIndex::iterator it = m_indexMap.find(imageID);

    if (it == m_indexMap.end())
        return;

    index = it->second;
    ASSERT(index >=0 && index < m_poolSize);
    m_freeIndex.push_back(index);

    if (m_flushing && m_freeIndex.size() == (uint32_t)m_poolSize)
        m_flushing = false;

    m_cond.signal();
}

} //namespace YamiMediaCodec
