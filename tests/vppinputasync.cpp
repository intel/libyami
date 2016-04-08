/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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
#include "vppinputasync.h"

VppInputAsync::VppInputAsync()
    :m_cond(m_lock), m_eos(false),
     m_queueSize(0),m_quit(false)
{
}


SharedPtr<VppInput>
VppInputAsync::create(const SharedPtr<VppInput>& input, uint32_t queueSize)
{
    SharedPtr<VppInput> ret;

    if (!input || !queueSize)
        return ret;
    SharedPtr<VppInputAsync> async(new VppInputAsync());
    if (!async->init(input, queueSize)) {
        ERROR("init VppInputAsync failed");
        return ret;
    }
    ret = async;
    return ret;
}

void* VppInputAsync::start(void* async)
{
    VppInputAsync* input = (VppInputAsync*)async;
    input->loop();
    return NULL;
}

void VppInputAsync::loop()
{
    while (1) {
        {
            AutoLock lock(m_lock);
            while (m_queue.size() >= m_queueSize) {
                m_cond.wait();
                if (m_quit)
                    return;
            }

        }
        SharedPtr<VideoFrame> frame;
        bool ret = m_input->read(frame);
        AutoLock lock(m_lock);
        if (!ret) {
           m_eos = true;
           return;
        }
        m_queue.push_back(frame);
        m_cond.signal();
   }
}

bool VppInputAsync::init(const SharedPtr<VppInput>& input, uint32_t queueSize)
{
    m_input = input;
    m_queueSize = queueSize;
    if (pthread_create(&m_thread, NULL, start, this)) {
        ERROR("create thread failed");
        return false;
    }
    return true;

}

bool VppInputAsync::read(SharedPtr<VideoFrame>& frame)
{
    AutoLock lock(m_lock);
    while (m_queue.empty()) {
        if (m_eos)
            return false;
        m_cond.wait();
    }
    frame = m_queue.front();
    m_queue.pop_front();
    m_cond.signal();
    return true;
}

VppInputAsync::~VppInputAsync()
{
    {
        AutoLock lock(m_lock);
        m_quit = true;
        m_cond.signal();
    }
    pthread_join(m_thread, NULL);
}

bool VppInputAsync::init(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    assert(0 && "no need for this");
    return false;
}