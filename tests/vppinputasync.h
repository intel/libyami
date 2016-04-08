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
#ifndef vppinputasync_h
#define vppinputasync_h
#include "common/condition.h"
#include "common/lock.h"
#include <deque>

#include "vppinputoutput.h"

using namespace YamiMediaCodec;
class VppInputAsync : public VppInput
{
public:

    bool read(SharedPtr<VideoFrame>& frame);

    static SharedPtr<VppInput>
    create(const SharedPtr<VppInput>& input, uint32_t queueSize);

    VppInputAsync();
    virtual ~VppInputAsync();

    //do not use this
    bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
private:
    bool init(const SharedPtr<VppInput>& input, uint32_t queueSize);
    static void* start(void* async);
    void loop();

    Condition  m_cond;
    Lock       m_lock;
    SharedPtr<VppInput> m_input;
    bool       m_eos;

    typedef std::deque<SharedPtr<VideoFrame> > FrameQueue;
    FrameQueue m_queue;
    uint32_t   m_queueSize;

    pthread_t  m_thread;
    bool       m_quit;

};
#endif //vppinputasync_h
