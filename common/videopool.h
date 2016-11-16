/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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
#ifndef videopool_h
#define videopool_h
#include "VideoCommonDefs.h"
#include "common/lock.h"
#include <deque>

namespace YamiMediaCodec{

template <class T>
class VideoPool : public EnableSharedFromThis<VideoPool<T> >
{
public:
    VideoPool(std::deque<SharedPtr<T> >& buffers)
    {
            m_holder.swap(buffers);
            for (size_t i = 0; i < m_holder.size(); i++) {
                m_freed.push_back(m_holder[i].get());
            }
    }

    SharedPtr<T> alloc()
    {
        SharedPtr<T> ret;
        AutoLock _l(m_lock);
        if (!m_freed.empty()) {
            T* p = m_freed.front();
            m_freed.pop_front();
            ret.reset(p, Recycler(this->shared_from_this()));
        }
        return ret;
    }

private:

    void recycle(T* ptr)
    {
        AutoLock _l(m_lock);
        m_freed.push_back(ptr);
    }

    class Recycler
    {
    public:
        Recycler(const SharedPtr<VideoPool<T> >& pool)
            :m_pool(pool)
        {
        }
        void operator()(T* ptr) const
        {
            m_pool->recycle(ptr);
        }
    private:
        SharedPtr<VideoPool<T> > m_pool;
    };

    Lock m_lock;
    std::deque<T*> m_freed;
    std::deque<SharedPtr<T> > m_holder;
};

};
#endif  //videopool_h
