/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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
#include "vaapiimagepool.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapiimage.h"
#include <string.h>

namespace YamiMediaCodec{

ImagePoolPtr VaapiImagePool::create(const DisplayPtr& display, uint32_t format, int32_t width, int32_t height, size_t count)
{
    ImagePoolPtr pool;
    std::vector<ImagePtr> images;
    images.reserve(count);
    for (size_t i = 0; i < count; ++i) {
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
    DEBUG("m_poolSize: %lu", m_poolSize);
    for (size_t i = 0; i < m_poolSize; ++i) {
        m_freeIndex.push_back(i);
        m_indexMap[m_images[i]->getID()] = i;
        DEBUG("image pool index: %lu, image ID: 0x%x", i, m_images[i]->getID());
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
    ASSERT(index >= 0 && (size_t)index < m_poolSize);
    m_freeIndex.pop_front();

    image.reset(m_images[index].get(), ImageRecycler(shared_from_this()));
    return image;
}

void VaapiImagePool::flush()
{
    AutoLock lock(m_lock);
    if ( m_freeIndex.size() != m_poolSize)
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
    ASSERT(index >= 0 && (size_t)index < m_poolSize);
    m_freeIndex.push_back(index);

    if (m_flushing && m_freeIndex.size() == m_poolSize)
        m_flushing = false;

    m_cond.signal();
}

} //namespace YamiMediaCodec
