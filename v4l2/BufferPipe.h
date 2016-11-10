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
#ifndef BufferPipe_h
#define BufferPipe_h

#include "common/lock.h"
#include <deque>

namespace YamiMediaCodec {

template <class T>
class BufferPipe {
    typedef T ValueType;

public:
    //queue to incoming
    void queue(const ValueType& v)
    {
        queue(m_incoming, v);
    }

    //deque from outgoing
    bool deque(ValueType& v)
    {
        return peek(m_outgoing, v, true);
    }

    //queue to outgoing
    void put(const ValueType& v)
    {
        queue(m_outgoing, v);
    }

    //deque from incoming
    bool get(ValueType& v)
    {
        return peek(m_incoming, v, true);
    }

    //peek from incoming
    bool peek(ValueType& v)
    {
        return peek(m_incoming, v);
    }

    //clear incoming and outgoing
    void clearPipe()
    {
        clearQueue(m_incoming);
        clearQueue(m_outgoing);
    }

private:
    struct BufferQue {
        std::deque<ValueType> m_queue;
        Lock m_lock;
    };
    BufferQue m_incoming;
    BufferQue m_outgoing;
    static void queue(BufferQue& q, const ValueType& v)
    {
        AutoLock l(q.m_lock);
        q.m_queue.push_back(v);
    }
    static bool peek(BufferQue& q, ValueType& v, bool pop = false)
    {
        AutoLock l(q.m_lock);
        if (q.m_queue.empty())
            return false;
        v = q.m_queue.front();
        if (pop)
            q.m_queue.pop_front();
        return true;
    }
    static void clearQueue(BufferQue& q)
    {
        AutoLock l(q.m_lock);
        q.m_queue.clear();
    }
};
}

#endif //BufferPipe_h
