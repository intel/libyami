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
#ifndef v4l2_codecbase_h
#define v4l2_codecbase_h

#include <assert.h>
#include <deque>
#include <vector>
#include <list>
#include "common/condition.h"
#include "VideoPostProcessHost.h"
#include "VideoDecoderInterface.h"
#if defined(__ENABLE_X11__)
#include <X11/Xlib.h>
#endif
#if defined(__ENABLE_EGL__)
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include "EGL/eglext.h"
#endif
#include "VideoCommonDefs.h"
#include "vaapi/vaapiptrs.h"
#include "v4l2codec_device_ops.h"

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif

#ifndef V4L2_PIX_FMT_VP9
#define V4L2_PIX_FMT_VP9 YAMI_FOURCC('V', 'P', '9', '0')
#endif

#ifndef V4L2_PIX_FMT_VC1
#define V4L2_PIX_FMT_VC1 YAMI_FOURCC('V', 'C', '1', '0')
#endif

#if defined(INPUT) || defined(OUTPUT)
    #error("conflict define for INPUT/OUTPUT")
#else
    #define INPUT 0
    #define OUTPUT 1
#endif
#define GET_PORT_INDEX(_port, _type, _ret) do {                     \
        if (_type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {           \
            _port = INPUT;                                          \
        } else if (_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {   \
            _port = OUTPUT;                                         \
        } else {                                                    \
            _ret = -1;                                              \
            ERROR("unkown port, type num: %d", _type);              \
            break;                                                  \
        }                                                           \
    } while (0)

using namespace YamiMediaCodec;
class V4l2CodecBase {
  public:
    V4l2CodecBase();
     virtual ~V4l2CodecBase();

    typedef SharedPtr < V4l2CodecBase > V4l2CodecPtr;
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
    virtual bool stop() = 0;

#if defined(__ENABLE_WAYLAND__)
    bool setWaylandDisplay(struct wl_display*);
#elif defined(__ENABLE_X11__)
    bool setXDisplay(Display*);
#endif

#if defined(__ENABLE_EGL__)
    virtual int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t buffer_index, void* egl_image) {return 0;};
#endif

    bool setDrmFd(int fd);

    void workerThread();
    int32_t fd() { return m_fd[0];};

  protected:
    virtual bool start() = 0;
    virtual bool acceptInputBuffer(struct v4l2_buffer *qbuf) = 0;
    virtual bool giveOutputBuffer(struct v4l2_buffer *dqbuf) = 0;
    virtual bool inputPulse(uint32_t index) = 0;
    virtual bool outputPulse(uint32_t &index) = 0; // index of decode output is decided by libyami, not FIFO of m_framesTodo[OUTPUT]
    virtual bool recycleInputBuffer(struct v4l2_buffer *qbuf) {return true; }
    virtual bool recycleOutputBuffer(int32_t index) {return true;};
    virtual bool hasCodecEvent() {return m_hasEvent;}
    virtual void setCodecEvent();
    virtual void clearCodecEvent();
    virtual void releaseCodecLock(bool lockable) {};
    virtual void flush() {}

    VideoDataMemoryType m_memoryType;
    uint32_t m_maxBufferCount[2];
    uint32_t m_bufferPlaneCount[2];
    uint32_t m_memoryMode[2];
    uint32_t m_pixelFormat[2]; // (it should be a set)
    bool m_streamOn[2];
    bool m_threadOn[2];
    int32_t m_fd[2]; // 0 for device event, 1 for interrupt
    bool m_started;

    NativeDisplay m_nativeDisplay;

    enum EosState{
        EosStateNormal,
        EosStateInput,
        EosStateOutput,
    };
    // EOS state is detected(EosStateInput) or transit to EosStateOutput in subclass (V4l2Decoder/V4l2Encoder).
    // it is cleared in base class (V4l2Codec) after input thread unblock, and used for special synchronization between INPUT and OUTPUT thread
    // so, we keep its operation func in base class with lock (and m_eosState private).
    virtual EosState eosState() { return m_eosState; };
    virtual void setEosState(EosState eosState);

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

    enum ReqBufState {
        RBS_Normal,         // normal running state
        RBS_Request,        // receive REQBUF in I/O thread
        RBS_Acknowledge,    // work thread acknowledge REQBUF (pause buffer processing)
        RBS_Released,       //buffer released, which means cannot call Qbuf or Debuf
        RBS_FormatChanged,  //buffer released, which means can still call Qbuf or Debuf
    };
    ReqBufState m_reqBufState[2];

    YamiMediaCodec::Lock m_codecLock;
    EosState  m_eosState;

    bool open(const char* name, int32_t flags);

#ifdef __ENABLE_DEBUG__
  protected:
    const char* IoctlCommandString(int command);

    uint32_t m_frameCount[2];
#endif
};

uint32_t v4l2PixelFormatFromMime(const char* mime);
const char* mimeFromV4l2PixelFormat(uint32_t pixelFormat);

#endif
