/*
 *  v4l2_codecbase.cpp
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/eventfd.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "v4l2_encode.h"
#include "v4l2_decode.h"
#include "common/log.h"

typedef std::tr1::shared_ptr < V4l2CodecBase > V4l2CodecPtr;
 V4l2CodecPtr V4l2CodecBase::createCodec(const char* name, int32_t flags)
{
    V4l2CodecPtr codec;
    if (!strcmp(name, "encoder"))
        codec.reset(new V4l2Encoder());
    else if (!strcmp(name, "decoder"))
        codec.reset(new V4l2Decoder());

    ASSERT(codec);
    codec->open(name, flags);

    return codec;
}
V4l2CodecBase::V4l2CodecBase()
    : m_memoryType(VIDEO_DATA_MEMORY_TYPE_RAW_COPY)
    , m_inputThreadCond(m_frameLock[INPUT])
    , m_outputThreadCond(m_frameLock[OUTPUT])
    , m_hasEvent(false)
{
    m_streamOn[INPUT] = false;
    m_streamOn[OUTPUT] = false;
    m_threadOn[INPUT] = false;
    m_threadOn[OUTPUT] = false;
    m_threadCond[INPUT] = &m_inputThreadCond;
    m_threadCond[OUTPUT] = &m_outputThreadCond;

    m_fd[0] = -1;
    m_fd[1] = -1;
#ifdef __ENABLE_DEBUG__
    m_frameCount[INPUT] = 0;
    m_frameCount[OUTPUT] = 0;
#endif
}

V4l2CodecBase::~V4l2CodecBase()
{
    ASSERT(!m_streamOn[INPUT]);
    ASSERT(!m_streamOn[OUTPUT]);
    ASSERT(!m_threadOn[INPUT]);
    ASSERT(!m_threadOn[OUTPUT]);

    INFO("codec is released, m_fd[0]: %d", m_fd[0]);
}

bool V4l2CodecBase::open(const char* name, int32_t flags)
{
    m_fd[0] = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC); // event for codec library, block on read(), one event per read()
    m_fd[1] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // for interrupt (escape from poll)
    return true;
}

bool V4l2CodecBase::close()
{
    bool ret = true;
    int timeOut = 10;

    INFO("m_streamOn[INPUT]: %d, m_streamOn[OUTPUT]: %d, m_threadOn[INPUT]: %d, m_threadOn[OUTPUT]: %d",
        m_streamOn[INPUT], m_streamOn[OUTPUT], m_threadOn[INPUT], m_threadOn[OUTPUT]);
    // wait until input/output thread terminate
    while ((m_threadOn[INPUT] || m_threadOn[OUTPUT]) && timeOut) {
        usleep(10000);
        timeOut--;
    }

    ASSERT(!m_threadOn[INPUT]);
    ASSERT(!m_threadOn[OUTPUT]);
    ret = stop();
    ASSERT(ret);
    ::close(m_fd[0]);
    ::close(m_fd[1]);

    return true;
}

void V4l2CodecBase::workerThread()
{
    bool ret = true;
    int thread = -1;

    m_codecLock.acquire();
    if (m_streamOn[INPUT] && !m_threadOn[INPUT])
        thread = INPUT;
    else if (m_streamOn[OUTPUT] && !m_threadOn[OUTPUT])
        thread = OUTPUT;
    else {
        ERROR("fail to create v4l2 work thread, unexpected status");
        return;
    }
    INFO("create work thread for %s", thread == INPUT ? "input" : "output");
    m_threadOn[thread] = true;
    m_codecLock.release();

    while (m_streamOn[thread]) {
        {
            AutoLock locker(m_frameLock[thread]);
            if (m_framesTodo[thread].empty()) {
                DEBUG("%s therad wait because m_framesTodo is empty", thread == INPUT ? "INPUT" : "OUTPUT");
                m_threadCond[thread]->wait(); // wait if no todo frame is available
                continue;
            }
        }

        int index = m_framesTodo[thread].front();
        ret = thread == INPUT ? inputPulse(index) : outputPulse(index);

        {
            AutoLock locker(m_frameLock[thread]);
            if (ret) {
                m_framesTodo[thread].pop_front();
                m_framesDone[thread].push_back(index);
                setDeviceEvent(0);
                #ifdef __ENABLE_DEBUG__
                m_frameCount[thread]++;
                DEBUG("m_frameCount[%s]: %d", thread == INPUT ? "input" : "output", m_frameCount[thread]);
                #endif
                DEBUG("%s therad wake up %s thread after process one frame", thread == INPUT ? "INPUT" : "OUTPUT", thread == INPUT ? "OUTPUT" : "INPUT");
                m_threadCond[!thread]->signal(); // encode/getOutput one frame success, wakeup the other thread
            } else {
                DEBUG("%s therad wait because operation on yami fails", thread == INPUT ? "INPUT" : "OUTPUT");
                m_threadCond[thread]->wait(); // wait if encode/getOutput fail (encode hw is busy or no available output)
            }
        }
        DEBUG("fd: %d", m_fd[0]);
    }

    if(thread == INPUT) {
        sendEOS();
        m_threadCond[OUTPUT]->signal();
    }

    m_threadOn[thread] = false;
}

static void* _workerThread(void *arg)
{
    V4l2CodecBase *v4l2Codec = static_cast<V4l2CodecBase*>(arg);
    v4l2Codec->workerThread();
    return NULL;
}

#if __ENABLE_DEBUG__
const char* V4l2CodecBase::IoctlCommandString(int command)
{
    static const char* unknown = "Unkonwn command";
#define IOCTL_COMMAND_STRING_MAP(cmd)   {cmd, #cmd}
    static struct IoctlCommanMap{
        int command;
        const char* cmdStr;
        } ioctlCommandMap[] = {
            IOCTL_COMMAND_STRING_MAP(VIDIOC_QUERYCAP),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_STREAMON),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_STREAMOFF),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_QUERYBUF),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_REQBUFS),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_QBUF),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_DQBUF),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_S_EXT_CTRLS),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_S_PARM),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_S_FMT),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_S_CROP),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_SUBSCRIBE_EVENT),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_DQEVENT),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_G_FMT),
            IOCTL_COMMAND_STRING_MAP(VIDIOC_G_CTRL)
        };

    int i;
    for (i=0; i<sizeof(ioctlCommandMap)/sizeof(IoctlCommanMap); i++)
        if (ioctlCommandMap[i].command == command)
            return ioctlCommandMap[i].cmdStr;

    return unknown;
}
#endif

int32_t V4l2CodecBase::ioctl(int command, void* arg)
{
    int32_t ret = 0;
    int port = -1;

    switch (command) {
        case VIDIOC_QUERYCAP: {
            // ::Initialize
            struct v4l2_capability *caps = static_cast<struct v4l2_capability *>(arg);
            caps->capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
        }
        break;
        case VIDIOC_STREAMON: {
            // ::Enqueue
            __u32 type = * ((__u32*)arg);
            if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                // XXX, setup X display
                start();
                port = INPUT;
            } else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                port = OUTPUT;
            } else {
                ret = -1;
                ERROR("unkown stream type: %d", type);
                break;
            }

            m_streamOn[port] = true;
            if (pthread_create(&m_worker[port], NULL, _workerThread, this) != 0) {
                ret = -1;
                ERROR("fail to create input worker thread");
            }
        }
        break;
        case VIDIOC_STREAMOFF: {
            // ::StopDevicePoll
            __u32 type = * ((__u32*)arg);
            if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                port = INPUT;
            }
            else if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                port = OUTPUT;
            } else {
                ret = -1;
                ERROR("unkown stream type: %d", type);
                break;
            }

            m_streamOn[port] = false;
            m_threadCond[port]->broadcast();
        }
        break;
        case VIDIOC_REQBUFS: {
            int i;
            struct v4l2_requestbuffers *reqbufs = static_cast<struct v4l2_requestbuffers *>(arg);
            if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                port = INPUT;
            } else if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                port = OUTPUT;
            } else {
                ret = -1;
                ERROR("unknown request buffer type: %d", reqbufs->type);
                break;
            }
            ASSERT(reqbufs->memory == m_memoryMode[port]);
            m_frameLock[port].acquire();
            // initial status of buffers are at client side
            m_framesTodo[port].clear();
            m_framesDone[port].clear();
            m_frameLock[port].release();
            if (reqbufs->count > 0) {
                // ::CreateInputBuffers()/CreateOutputBuffers()
                ASSERT(reqbufs->count <= m_maxBufferCount[port]);
                reqbufs->count = m_maxBufferCount[port];
            } else {
                // ::DestroyInputBuffers()/:DestroyOutputBuffers()
            }
        }
        break;
        case VIDIOC_QBUF: {
            struct v4l2_buffer *qbuf = static_cast<struct v4l2_buffer *>(arg);
            if (qbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                port  = INPUT;
            } else if (qbuf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                port = OUTPUT;
            } else {
                ret = -1;
                ERROR("unknown enqueued (VIDIOC_QBUF) buffer type: %d", qbuf->type);
                break;
            }
            // ::EnqueueInputRecord/EnqueueOutputRecord
            ASSERT(qbuf->memory == m_memoryMode[port]);
            ASSERT (qbuf->length == m_bufferPlaneCount[port]);
            if (port == INPUT) {
                bool _ret = acceptInputBuffer(qbuf);
                ASSERT(_ret);
            } else {
                bool _ret = recycleOutputBuffer(qbuf->index);
                ASSERT(_ret);
            }

            m_frameLock[port].acquire();
            m_framesTodo[port].push_back(qbuf->index);
            m_frameLock[port].release();
            m_threadCond[port]->signal();
        }
        break;
        case VIDIOC_DQBUF: {
            // ::Dequeue
            struct v4l2_buffer *dqbuf = static_cast<struct v4l2_buffer *>(arg);
            if (dqbuf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                port = INPUT;
            } else if (dqbuf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                port = OUTPUT;
            } else {
                ret = -1;
                ERROR("unknown buffer type: %d in command VIDIOC_DQBUF", dqbuf->type);
                break;
            }

            {
                AutoLock locker (m_frameLock[port]);
                if (m_framesDone[port].empty()) {
                    ret = -1;
                    errno = EAGAIN;
                    break;
                }
            }
            // ASSERT(dqbuf->memory == m_memoryMode[port]);
            ASSERT(dqbuf->length == m_bufferPlaneCount[port]);
            dqbuf->index = m_framesDone[port].front();
            ASSERT(dqbuf->index >= 0 && dqbuf->index < m_maxBufferCount[port]);
            if (port == OUTPUT) {
                bool _ret = giveOutputBuffer(dqbuf);
                ASSERT(_ret);
            }
            m_frameLock[port].acquire();
            m_framesDone[port].pop_front();
            m_frameLock[port].release();
            DEBUG("%s port dqbuf->index: %d", port == INPUT ? "input" : "output", dqbuf->index);
        }
        break;
        default:
            ERROR("unknown command type");
            ret = -1;
        break;
    }

    return ret;
}

int32_t V4l2CodecBase::setDeviceEvent(int index)
{
    uint64_t buf = 1;
    int32_t ret = -1;

    ASSERT(index == 0 || index == 1);
    ret = write(m_fd[index], &buf, sizeof(buf));
    if (ret != sizeof(uint64_t)) {
      ERROR ("SetDevicePollInterrupt(): write() failed");
      return -1;
    }

    return 0;
}

int32_t V4l2CodecBase::clearDeviceEvent(int index)
{
    uint64_t buf;
    int ret = -1;

    ASSERT(index == 0 || index == 1);
    ret = read(m_fd[index], &buf, sizeof(buf));
    if (ret == -1) {
      if (errno == EAGAIN) {
        // No interrupt flag set, and we're reading nonblocking.  Not an error.
        return 0;
      } else {
        ERROR("ClearDevicePollInterrupt(): read() failed");
        return -1;
      }
    }

    return 0;
}

int32_t V4l2CodecBase::poll(bool poll_device, bool* event_pending)
{
    struct pollfd pollfds[2];
    nfds_t nfds;
    int pollfd = -1;

    pollfds[0].fd = m_fd[1];
    pollfds[0].events = POLLIN | POLLERR;
    nfds = 1;

    if (poll_device) {
      DEBUG("Poll(): adding device fd to poll() set");
      pollfds[nfds].fd = m_fd[0];
      pollfds[nfds].events = POLLIN | POLLOUT | POLLERR | POLLPRI;
      pollfd = nfds;
      nfds++;
    }

    if (::poll(pollfds, nfds, -1) == -1) {
      ERROR("poll() failed");
      return -1;
    }
    // *event_pending = (pollfd != -1 && pollfds[pollfd].revents & (POLLPRI | POLLIN | POLLOUT));
    *event_pending = m_hasEvent;

    // clear event
    clearDeviceEvent(0);

    return 0;
}

void V4l2CodecBase::setCodecEvent()
{
    YamiMediaCodec::AutoLock locker(m_codecLock);
    m_hasEvent = true;
}

void V4l2CodecBase::clearCodecEvent()
{
    YamiMediaCodec::AutoLock locker(m_codecLock);
    m_hasEvent = false;
}

