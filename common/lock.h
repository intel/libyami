/*
 *  lock.h - lock and autolock
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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
#ifndef lock_h
#define lock_h

#include "interface/VideoCommonDefs.h"
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