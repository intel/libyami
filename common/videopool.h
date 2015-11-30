/*
 *  videopool.h - video pool
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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
#ifndef videopool_h
#define videopool_h
#include "interface/VideoCommonDefs.h"
#include "common/lock.h"
#include <deque>

namespace YamiMediaCodec{

template <class T>
class VideoPool : public EnableSharedFromThis<VideoPool<T> >
{
public:
    static SharedPtr<VideoPool<T> >
    create(std::deque<SharedPtr<T> >& buffers)
    {
        SharedPtr<VideoPool<T> > ptr(new VideoPool<T>(buffers));
        return ptr;
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

    VideoPool(std::deque<SharedPtr<T> >& buffers)
    {
            m_holder.swap(buffers);
            for (size_t i = 0; i < m_holder.size(); i++) {
                m_freed.push_back(m_holder[i].get());
            }
    }

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
