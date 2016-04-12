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
#ifndef condition_h
#define condition_h

#include "NonCopyable.h"
#include "interface/VideoCommonDefs.h"

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
