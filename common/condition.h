/*
 *  condition.h - conditions
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
#ifndef condition_h
#define condition_h

//TODO: remove this when we put DISALLOW_COPY_AND_ASSIGN to common/
#include "vaapi/vaapitypes.h"

#include "lock.h"

namespace YamiMediaCodec{

class Condition
{
public:
    explicit Condition(Lock& lock):m_lock(lock)
    {
        pthread_cond_init(&m_cond, NULL);
    }

    ~Condition()
    {
        pthread_cond_destroy(&m_cond);
    }

    void wait()
    {
        pthread_cond_wait(&m_cond, &m_lock.m_lock);
    }

    void signal()
    {
        pthread_cond_signal(&m_cond);
    }

    void broadcast()
    {
        pthread_cond_broadcast(&m_cond);
    }

private:
    Lock& m_lock;
    pthread_cond_t m_cond;
    DISALLOW_COPY_AND_ASSIGN(Condition);

};

};

#endif