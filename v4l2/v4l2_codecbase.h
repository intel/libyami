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
#include <vector>
#include <list>
#include "common/condition.h"
#if ANDROID
#include "VideoPostProcessHost.h"
#include "interface/VideoDecoderInterface.h"
#else
    #if __ENABLE_X11__
    #include <X11/Xlib.h>
    #endif
    #include <EGL/egl.h>
    #define EGL_EGLEXT_PROTOTYPES
    #include "EGL/eglext.h"
#endif
#include "interface/VideoCommonDefs.h"
#if ANDROID
#include <va/va_android.h>
#endif

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
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
#if ANDROID
    inline bool setVaDisplay();
    inline bool createVpp();
#else
    #if __ENABLE_X11__
    bool setXDisplay(Display *x11Display) { m_x11Display = x11Display; return true; };
    virtual int32_t usePixmap(uint32_t bufferIndex, Pixmap pixmap) {return 0;};
    #endif
    virtual int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t buffer_index, void* egl_image) {return 0;};
    bool setDrmFd(int drm_fd) {m_drmfd = drm_fd; return true;};

#endif
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
#if ANDROID
    SharedPtr<VideoFrame> createVaSurface(const ANativeWindowBuffer* buf);
    bool mapVideoFrames();
#endif

    VideoDataMemoryType m_memoryType;
    uint32_t m_maxBufferCount[2];
    uint32_t m_bufferPlaneCount[2];
    uint32_t m_memoryMode[2];
    uint32_t m_pixelFormat[2]; // (it should be a set)
    bool m_streamOn[2];
    bool m_threadOn[2];
    int32_t m_fd[2]; // 0 for device event, 1 for interrupt
    bool m_started;
#if ANDROID
    VADisplay m_vaDisplay;
    SharedPtr<IVideoPostProcess> m_vpp;
    uint32_t m_reqBuffCnt;
    std::vector<SharedPtr<VideoFrame> > m_videoFrames;
    std::vector<ANativeWindowBuffer*> m_winBuff;
    gralloc_module_t* m_pGralloc;
#else
    #if __ENABLE_X11__
    Display *m_x11Display;
    #endif
    int m_drmfd;
#endif

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
