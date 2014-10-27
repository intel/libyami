/*
 *  v4l2_codecbase.h
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
#ifndef v4l2_codecbase_h
#define v4l2_codecbase_h

#include <assert.h>
#include <deque>
#include <list>
#include <tr1/memory>
#include "common/condition.h"
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/eglext.h"
#include "interface/VideoCommonDefs.h"

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif

#if defined(INPUT) || defined(OUTPUT)
    #error("conflict define for INPUT/OUTPUT")
#else
    #define INPUT 0
    #define OUTPUT 1
#endif

class V4l2CodecBase {
  public:
    V4l2CodecBase();
     ~V4l2CodecBase();

    typedef std::tr1::shared_ptr < V4l2CodecBase > V4l2CodecPtr;
    static V4l2CodecPtr createCodec(const char* name, int32_t flags);
    bool close ();
    virtual int32_t setFrameMemoryType(VideoDataMemoryType memory_type) {m_memoryType = memory_type; return 0;} ;
    virtual int32_t ioctl(int request, void* arg);
    int32_t poll(bool poll_device, bool* event_pending);
    int32_t setDeviceEvent(int index);
    int32_t clearDeviceEvent(int index);
    virtual void* mmap(void* addr, size_t length,
                         int prot, int flags, unsigned int offset) {return NULL;};
    // virtual int32_t munmap(void* addr, size_t length) {return 0;};
    virtual int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t buffer_index, void* egl_image) {return 0;};

    void workerThread();
    int32_t fd() { return m_fd[0];};

  protected:
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool acceptInputBuffer(struct v4l2_buffer *qbuf) = 0;
    virtual bool giveOutputBuffer(struct v4l2_buffer *dqbuf) = 0;
    virtual bool inputPulse(int32_t index) = 0;
    virtual bool outputPulse(int32_t &index) = 0; // index of decode output is decided by libyami, not FIFO of m_framesTodo[OUTPUT]
    virtual bool recycleOutputBuffer(int32_t index) {return true;};
    virtual bool hasCodecEvent() {return m_hasEvent;}
    virtual void setCodecEvent();
    virtual void clearCodecEvent();

    VideoDataMemoryType m_memoryType;
    int m_maxBufferCount[2];
    uint32_t m_bufferPlaneCount[2];
    uint32_t m_memoryMode[2];
    uint32_t m_pixelFormat[2]; // (it should be a set)
    bool m_streamOn[2];
    bool m_threadOn[2];
    int32_t m_fd[2]; // 0 for device event, 1 for interrupt
    bool m_started;

  private:
    bool m_hasEvent;

    pthread_t m_worker[2];
    // to be processed by codec.
    // encoder: (0:INPUT):filled with input frame data, input worker thread will send them to yami
    //          (1:OUTPUT): empty output buffer, output worker thread will fill it with coded data
    // decoder: (0:INPUT):filled with compressed frame data, input worker thread will send them to yami
    //          (1:OUTPUT): frames at codec side under processing; when output worker get one frame from yami, it should be in this set
    std::list<int> m_framesTodo[2]; // INPUT port FIFO, OUTPUT port in random order
    // processed by codec already, ready for dque
    // (0,INPUT): ready to deque for input buffer.
    // (1, OUTPUT): filled with coded data (encoder) or decoded frame (decoder).
    std::deque<int> m_framesDone[2];  // ready for deque, processed by codec already for input buffer.
    YamiMediaCodec::Lock m_frameLock[2]; // lock for INPUT/OUTPUT frames respectively

    // Condition must be initialized with Lock, but array (without default construct function) doesn't work
    YamiMediaCodec::Condition m_inputThreadCond;
    YamiMediaCodec::Condition m_outputThreadCond;
    YamiMediaCodec::Condition *m_threadCond[2];

    YamiMediaCodec::Lock m_codecLock;
    bool open(const char* name, int32_t flags);

#ifdef __ENABLE_DEBUG__
  protected:
    const char* IoctlCommandString(int command);

    uint32_t m_frameCount[2];
#endif
};
#endif
