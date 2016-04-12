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
#ifndef lock_h
#define lock_h

#include "NonCopyable.h"
#include <pthread.h>

namespace YamiMediaCodec{

class Lock
{
public:
    Lock()
    {
        pthread_mutex_init(&m_lock, NULL);
    }

    ~Lock()
    {
        pthread_mutex_destroy(&m_lock);
    }

    void acquire()
    {
        pthread_mutex_lock(&m_lock);
    }

    void release()
    {
        pthread_mutex_unlock(&m_lock);
    }

    void tryLock()
    {
        pthread_mutex_trylock(&m_lock);
    }

    friend class Condition;

private:
    pthread_mutex_t m_lock;
    DISALLOW_COPY_AND_ASSIGN(Lock);
};

class AutoLock
{
public:
    explicit AutoLock(Lock& lock) : m_lock(lock)
    {
        m_lock.acquire();
    }
    ~AutoLock()
    {
        m_lock.release();
    }
    Lock& m_lock;
private:
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

};

#endif
