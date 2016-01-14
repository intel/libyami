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

#include "Thread.h"
#include "Functional.h"
#include "log.h"

#include <assert.h>

namespace YamiMediaCodec {

Thread::Thread(const char* name)
    : m_name(name)
    , m_started(false)
    , m_thread(INVALID_ID)
    , m_cond(m_lock)
    , m_sent(m_lock)
{
}

bool Thread::start()
{
    AutoLock lock(m_lock);
    if (m_started)
        return false;
    if (pthread_create(&m_thread, NULL, init, this)) {
        ERROR("create thread %s failed", m_name.c_str());
        m_thread = INVALID_ID;
        return false;
    }
    m_started = true;
    return true;
}

void* Thread::init(void* thread)
{
    Thread* t = (Thread*)thread;
    t->loop();
    return NULL;
}

void Thread::loop()
{
    while (1) {
        AutoLock lock(m_lock);
        if (m_queue.empty()) {
            if (!m_started)
                return;
            m_cond.wait();
        }
        else {
            Job& r = m_queue.front();
            m_lock.release();
            r();
            m_lock.acquire();
            m_queue.pop_front();
        }
    }
}

void Thread::enqueue(const Job& r)
{
    m_queue.push_back(r);
    m_cond.signal();
}

void Thread::post(const Job& r)
{
    AutoLock lock(m_lock);
    if (!m_started) {
        ERROR("%s: post job after stop()", m_name.c_str());
        return;
    }
    enqueue(r);
}

void Thread::sendJob(const Job& r, bool& flag)
{
    r();

    //flag need protect here since we check it in other thread
    AutoLock lock(m_lock);
    flag = true;
    m_sent.broadcast();
}

bool Thread::send(const Job& c)
{
    bool flag = false;

    AutoLock lock(m_lock);
    if (!m_started) {
        ERROR("%s: sent job after stop()", m_name.c_str());
        return false;
    }
    enqueue(std::bind(&Thread::sendJob, this, std::ref(c), std::ref(flag)));
    //wait for result;
    while (!flag) {
        m_sent.wait();
    }
    return true;
}

void Thread::stop()
{
    {
        AutoLock lock(m_lock);
        if (!m_started)
            return;
        m_started = false;
        m_cond.signal();
    }
    pthread_join(m_thread, NULL);
    m_thread = INVALID_ID;
    assert(m_queue.empty());
}

bool Thread::isCurrent()
{
    return pthread_self() == m_thread;
}

Thread::~Thread()
{
    stop();
}
}
