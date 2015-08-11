/*
 *  vppinputasync.h - threaded vpp input
 *
 *  Copyright (C) 2015 Intel Corporation
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
