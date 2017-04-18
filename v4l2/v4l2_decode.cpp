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

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <vector>
#include <map>
#include <deque>
#include <set>
#include <va/va_drmcommon.h>
#if defined(__ENABLE_WAYLAND__)
#include <va/va_wayland.h>
#endif
#include "v4l2_decode.h"
#include "VideoDecoderHost.h"
#include "common/log.h"
#include "common/common_def.h"
#include "common/basesurfaceallocator.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/VaapiUtils.h"

#define INT64_TO_TIMEVAL(i64, time_val)                 \
    do {                                                \
        time_val.tv_sec = (int32_t)(i64 >> 31);         \
        time_val.tv_usec = (int32_t)(i64 & 0x7fffffff); \
    } while (0)
#define TIMEVAL_TO_INT64(i64, time_val)       \
    do {                                      \
        i64 = time_val.tv_sec;                \
        i64 = (i64 << 31) + time_val.tv_usec; \
    } while (0)

//return if we have error
#define ERROR_RETURN(no) \
    do {                 \
        if (no) {        \
            errno = no;  \
            return -1;   \
        }                \
    } while (0)

#define CHECK(cond)                      \
    do {                                 \
        if (!(cond)) {                   \
            ERROR("%s is false", #cond); \
            ERROR_RETURN(EINVAL);        \
        }                                \
    } while (0)

//check condition in post messsage.
#define PCHECK(cond)                     \
    do {                                 \
        if (!(cond)) {                   \
            m_state = kError;            \
            ERROR("%s is false", #cond); \
            return;                      \
        }                                \
    } while (0)

#define SEND(func)                     \
    do {                               \
        int32_t ret_ = sendTask(func); \
        ERROR_RETURN(ret_);            \
    } while (0)

const static uint32_t kDefaultInputSize = 1024 * 1024;

//#undef DEBUG
//#define DEBUG ERROR

using namespace std;

struct FrameInfo {
    int64_t timeStamp;
};

class V4l2Decoder::Output {
public:
    Output(V4l2Decoder* decoder)
        : m_decoder(decoder)
    {
    }
    virtual void setDecodeAllocator(DecoderPtr decoder)
    {
        /* nothing */
    }
    virtual int32_t requestBuffers(uint32_t count) = 0;
    virtual void output(SharedPtr<VideoFrame>& frame) = 0;
    virtual bool isAlloccationDone() = 0;
    virtual bool isOutputReady() = 0;
    virtual int32_t deque(struct v4l2_buffer* buf) = 0;
    virtual int32_t queue(struct v4l2_buffer* buf)
    {
        m_decoder->m_out.queue(buf->index);
        return 0;
    }
    virtual int32_t createBuffers(struct v4l2_create_buffers* createBuffers)
    {
        return 0;
    }
    virtual void streamOff()
    {
    }

protected:
    void requestFrameInfo(uint32_t count)
    {
        m_frameInfo.resize(count);
    }
    void outputFrame(uint32_t index, SharedPtr<VideoFrame>& frame)
    {
        setTimeStamp(index, frame->timeStamp);
        m_decoder->m_out.put(index);
        m_decoder->setDeviceEvent(0);
    }
    void getTimeStamp(uint32_t index, struct timeval& timeStamp)
    {
        ASSERT(index < m_frameInfo.size());
        INT64_TO_TIMEVAL(m_frameInfo[index].timeStamp, timeStamp);
    }
    void setTimeStamp(uint32_t index, int64_t timeStamp)
    {
        ASSERT(index < m_frameInfo.size());
        m_frameInfo[index].timeStamp = timeStamp;
    }

    void getBufferInfo(struct v4l2_buffer* buf, uint32_t index)
    {

        buf->index = index;
        //chrome will use this value.
        buf->m.planes[0].bytesused = 1;
        buf->m.planes[1].bytesused = 1;
        getTimeStamp(index, buf->timestamp);
    }

    V4l2Decoder* m_decoder;
    vector<FrameInfo> m_frameInfo;
    Lock m_lock;
};

class SurfaceGetter {
    typedef map<intptr_t, uint32_t> UsedMap;
    const static uint32_t kHoldByRenderer = 1;
    const static uint32_t kHoldByDecoder = 2;

public:
    static YamiStatus getSurface(SurfaceAllocParams* param, intptr_t* surface)
    {
        SurfaceGetter* p = (SurfaceGetter*)param->user;
        return p->getSurface(surface);
    }

    static YamiStatus putSurface(SurfaceAllocParams* param, intptr_t surface)
    {
        SurfaceGetter* p = (SurfaceGetter*)param->user;
        return p->putSurface(surface);
    }

    YamiStatus getSurface(intptr_t* surface)
    {
        AutoLock l(m_lock);
        if (!takeOneFree(surface, kHoldByDecoder))
            return YAMI_DECODE_NO_SURFACE;
        DEBUG("getSurface: %x", (uint32_t)*surface);
        return YAMI_SUCCESS;
    }

    YamiStatus putSurface(intptr_t surface)
    {
        AutoLock l(m_lock);

        clearFlag(surface, kHoldByDecoder);
        DEBUG("putSurface: %x", (uint32_t)surface);
        return YAMI_SUCCESS;
    }

    bool holdByRenderer(intptr_t surface)
    {
        AutoLock l(m_lock);
        return setFlag(surface, kHoldByRenderer);
    }

    bool freeByRenderer(intptr_t surface)
    {
        AutoLock l(m_lock);
        return clearFlag(surface, kHoldByRenderer);
    }

    void holdAllByRenderer()
    {
        DEBUG("hold all by renderer");

        AutoLock l(m_lock);
        m_freed.clear();

        UsedMap::iterator it = m_used.begin();
        while (it != m_used.end()) {
            it->second = kHoldByRenderer;
            ++it;
        }
    }

    bool takeOneFreeByRenderer(intptr_t* surface)
    {
        AutoLock l(m_lock);
        return takeOneFree(surface, kHoldByRenderer);
    }
    bool addNewSurface(intptr_t surface)
    {
        AutoLock l(m_lock);
        UsedMap::iterator it = m_used.find(surface);
        if (it != m_used.end()) {
            ERROR("add duplicate surface %x, it's hold by %s", (uint32_t)surface, getOwnerName(it->second));
        }
        m_freed.push_back(surface);
        m_used[surface] = 0;
        return true;
    }

private:
    bool takeOneFree(intptr_t* surface, uint32_t flag)
    {
        if (m_freed.empty()) {
            DEBUG("no free surface");
            UsedMap::iterator it = m_used.begin();
            while (it != m_used.end()) {
                DEBUG("surface %x, it's hold by %s", (uint32_t)it->first, getOwnerName(it->second));
                ++it;
            }
            return false;
        }
        *surface = m_freed.front();
        m_used[*surface] = flag;
        m_freed.pop_front();
        return true;
    }

    bool setFlag(intptr_t surface, uint32_t flag)
    {
        UsedMap::iterator it = m_used.find(surface);
        DEBUG("set %x to %s", (uint32_t)surface, getOwnerName(flag));
        if (it == m_used.end()) {
            ERROR("set owner to %s failed for %x", getOwnerName(flag), (uint32_t)surface);
            return false;
        }
        it->second |= flag;
        return true;
    }
    YamiStatus clearFlag(intptr_t surface, uint32_t flag)
    {
        UsedMap::iterator it = m_used.find(surface);
        DEBUG("clear %x to %s", (uint32_t)surface, getOwnerName(flag));

        if (it == m_used.end()) {
            ERROR("clear wrong surface id = %x", (uint32_t)surface);
            return YAMI_INVALID_PARAM;
        }
        if (!(it->second & flag)) {
            ERROR("%x is not hold by %s, it hold by %s", (uint32_t)surface, getOwnerName(flag), getOwnerName(it->second));
            return YAMI_INVALID_PARAM;
        }
        it->second &= ~flag;
        if (!it->second) {
            m_freed.push_back(surface);
        }
        return YAMI_SUCCESS;
    }

private:
    const char* getOwnerName(uint32_t flag)
    {
        if (flag == kHoldByRenderer)
            return "renderer";
        if (flag == kHoldByDecoder)
            return "decoder";
        if (flag == (kHoldByRenderer | kHoldByDecoder))
            return "decoder and render";
        if (!flag)
            return "no one";
        return "unknow";
    }
    Lock m_lock;
    UsedMap m_used;
    deque<intptr_t> m_freed;
};

//the buffer resource is created from V4L2's user
class ExternalBufferOutput : public V4l2Decoder::Output, BaseSurfaceAllocator {
    typedef map<intptr_t, uint32_t> SurfaceMap;

public:
    ExternalBufferOutput(V4l2Decoder* decoder)
        : V4l2Decoder::Output(decoder)
        , m_getter(new SurfaceGetter)
    {
    }
    virtual void setDecodeAllocator(DecoderPtr decoder)
    {
        decoder->setAllocator(this);
        /* nothing */
    }
    virtual int32_t requestBuffers(uint32_t count)
    {
        AutoLock l(m_lock);
        m_count = count;
        if (!count) {
            for (size_t i = 0; i < m_surfaces.size(); i++) {
                if (m_surfaces[i] != VA_INVALID_SURFACE) {
                    destorySurface(m_surfaces[i]);
                }
            }
        }
        m_surfaces.resize(count, VA_INVALID_SURFACE);
        requestFrameInfo(count);

        return 0;
    }
    virtual void output(SharedPtr<VideoFrame>& frame)
    {
        ASSERT(m_getter.get());
        m_getter->holdByRenderer(frame->surface);

        AutoLock l(m_lock);

        SurfaceMap::iterator it = m_surfaceMap.find(frame->surface);
        ASSERT(it != m_surfaceMap.end());
        outputFrame(it->second, frame);
    }
    virtual bool isAlloccationDone()
    {
        return m_surfaceMap.size() == m_count;
    }
    virtual bool isOutputReady()
    {
        return true;
    }
    int32_t queue(struct v4l2_buffer* buf)
    {
        uint32_t index;
        bool streamOn;
        //create and update index
        intptr_t surface;
        {
            //we need hold lock in freeVideoFrame, so we keep the lock in {}
            AutoLock l(m_lock);
            index = buf->index;

            CHECK(index < m_surfaces.size());
            streamOn = m_decoder->m_outputOn;
            if (!m_decoder->m_outputOn) {
                CHECK(createSurface(surface, buf));
                m_surfaces[index] = surface;
                m_surfaceMap[surface] = index;
            }
            surface = m_surfaces[index];
        }

        ASSERT(m_getter.get());
        if (!streamOn)
            m_getter->addNewSurface(surface);
        else
            m_getter->freeByRenderer(surface);
        return 0;
    }
    virtual int32_t deque(struct v4l2_buffer* buf)
    {
        AutoLock l(m_lock);
        uint32_t index;
        if (m_decoder->m_outputOn) {
            if (!m_decoder->m_out.deque(index)) {
                ERROR_RETURN(EAGAIN);
            }
        }
        else {
            CHECK(m_getter);
            intptr_t p;
            //this mean we need deque the surface from free list
            CHECK(m_getter->takeOneFreeByRenderer(&p));
            SurfaceMap::iterator it = m_surfaceMap.find(p);
            CHECK(it != m_surfaceMap.end());
            index = it->second;
        }
        getBufferInfo(buf, index);
        return 0;
    }
    virtual void streamOff()
    {
        m_getter->holdAllByRenderer();
    }

protected:
    virtual YamiStatus doAlloc(SurfaceAllocParams* params)
    {
        AutoLock l(m_lock);
        if (m_surfaces.size() != m_surfaceMap.size()) {
            ERROR("surfaces size did not match map size (%d != %d)", (int)m_surfaces.size(), (int)m_surfaceMap.size());
            return YAMI_FAIL;
        }
        if (params->size > m_surfaces.size()) {
            ERROR("require size > surface size (%d > %d)", (int)params->size, (int)m_surfaces.size());
            return YAMI_FAIL;
        }
        params->surfaces = &m_surfaces[0];
        params->size = m_surfaces.size();
        params->user = m_getter.get();
        params->getSurface = SurfaceGetter::getSurface;
        params->putSurface = SurfaceGetter::putSurface;
        return YAMI_SUCCESS;
    }
    virtual YamiStatus doFree(SurfaceAllocParams* params)
    {
        /* nothing */
        return YAMI_SUCCESS;
    }
    virtual void doUnref()
    {
        /* nothing */
    }

    virtual bool createExternalSurface(intptr_t& surface, uint32_t rtFormat, VASurfaceAttribExternalBuffers& external)
    {
        VASurfaceAttrib attribs[2];
        attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[0].type = VASurfaceAttribMemoryType;
        attribs[0].value.type = VAGenericValueTypeInteger;
        attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;

        attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
        attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
        attribs[1].value.type = VAGenericValueTypePointer;
        attribs[1].value.value.p = &external;

        VASurfaceID id;
        VAStatus vaStatus = vaCreateSurfaces(m_decoder->m_display->getID(), rtFormat, external.width, external.height,
            &id, 1, attribs, N_ELEMENTS(attribs));
        if (!checkVaapiStatus(vaStatus, "vaCreateSurfaces"))
            return false;
        surface = static_cast<intptr_t>(id);
        return true;
    }

    virtual bool createSurface(intptr_t& surface, struct v4l2_buffer* buf) = 0;
    virtual bool destorySurface(intptr_t& surface) = 0;

    uint32_t m_count;
    vector<intptr_t> m_surfaces;
    SurfaceMap m_surfaceMap;
    SharedPtr<SurfaceGetter> m_getter;
};

class ExternalDmaBufOutput : public ExternalBufferOutput {
public:
    ExternalDmaBufOutput(V4l2Decoder* decoder)
        : ExternalBufferOutput(decoder)
    {
    }

    virtual int32_t createBuffers(struct v4l2_create_buffers* createBuffers)
    {
        m_createBuffers.reset(new v4l2_create_buffers);
        *m_createBuffers = *createBuffers;
        return 0;
    }

    virtual bool createSurface(intptr_t& surface, struct v4l2_buffer* buf)
    {
        if (!m_createBuffers) {
            ERROR("you need create buffer first");
            return false;
        }

        v4l2_pix_format_mplane& format = m_createBuffers->format.fmt.pix_mp;
        VASurfaceAttribExternalBuffers external;
        memset(&external, 0, sizeof(external));
        external.pixel_format = format.pixelformat;
        uint32_t rtFormat = getRtFormat(format.pixelformat);
        if (!rtFormat) {
            ERROR("failed get rtformat for %.4s", (char*)&format.pixelformat);
            return false;
        }
        external.width = format.width;
        external.height = format.height;
        external.num_planes = format.num_planes;
        for (uint32_t i = 0; i < format.num_planes; i++) {
            external.pitches[i] = format.plane_fmt[i].bytesperline;
            //not really right, but we use sizeimage to deliver offset
            external.offsets[i] = format.plane_fmt[i].sizeimage;
        }
        external.buffers = &buf->m.userptr;
        external.num_buffers = 1;

        return createExternalSurface(surface, rtFormat, external);
    }
    virtual bool destorySurface(intptr_t& surface)
    {
        VASurfaceID s = (VASurfaceID)surface;
        return checkVaapiStatus(vaDestroySurfaces(m_decoder->m_display->getID(), &s, 1), "vaDestroySurfaces");
    }
    SharedPtr<v4l2_create_buffers> m_createBuffers;
};

#ifdef __ENABLE_EGL__
#include "egl/egl_vaapi_image.h"
class EglOutput : public V4l2Decoder::Output {
public:
    EglOutput(V4l2Decoder* decoder, VideoDataMemoryType memory_type)
        : V4l2Decoder::Output(decoder)
        , m_memoryType(memory_type)
    {
    }
    virtual int32_t requestBuffers(uint32_t count)
    {
        v4l2_pix_format_mplane& format = m_decoder->m_outputFormat.fmt.pix_mp;
        DisplayPtr& display = m_decoder->m_display;
        CHECK(bool(display));
        CHECK(format.width && format.height);
        m_eglVaapiImages.clear();
        for (uint32_t i = 0; i < count; i++) {
            SharedPtr<EglVaapiImage> image(new EglVaapiImage(display->getID(), format.width, format.height));
            if (!image->init()) {
                ERROR("Create egl vaapi image failed");
                m_eglVaapiImages.clear();
                return EINVAL;
            }
            m_eglVaapiImages.push_back(image);
        }
        requestFrameInfo(count);
        return 0;
    }
    int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t bufferIndex, void* eglImage)
    {
        CHECK(m_memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME || m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
        CHECK(bufferIndex < m_eglVaapiImages.size());
        *(EGLImageKHR*)eglImage = m_eglVaapiImages[bufferIndex]->createEglImage(eglDisplay, eglContext, m_memoryType);
        return 0;
    }
    void output(SharedPtr<VideoFrame>& frame)
    {
        uint32_t index;
        if (!m_decoder->m_out.get(index)) {
            ERROR("bug: can't get index");
            return;
        }
        ASSERT(index < m_eglVaapiImages.size());
        m_eglVaapiImages[index]->blt(frame);
        outputFrame(index, frame);
    }
    bool isAlloccationDone()
    {
        return !m_eglVaapiImages.empty();
    }
    bool isOutputReady()
    {
        uint32_t index;
        return m_decoder->m_out.peek(index);
    }

    int32_t deque(struct v4l2_buffer* buf)
    {
        uint32_t index;
        if (!m_decoder->m_out.deque(index)) {
            ERROR_RETURN(EAGAIN);
        }
        getBufferInfo(buf, index);
        return 0;
    }

private:
    std::vector<SharedPtr<EglVaapiImage> > m_eglVaapiImages;
    VideoDataMemoryType m_memoryType;
};
#endif

V4l2Decoder::V4l2Decoder()
    : m_inputOn(false)
    , m_outputOn(false)
    , m_state(kUnStarted)
{
    memset(&m_inputFormat, 0, sizeof(m_inputFormat));
    memset(&m_outputFormat, 0, sizeof(m_outputFormat));
    memset(&m_lastFormat, 0, sizeof(m_lastFormat));
    m_output.reset(new ExternalDmaBufOutput(this));
}

V4l2Decoder::~V4l2Decoder()
{
}

void V4l2Decoder::releaseCodecLock(bool lockable)
{
}

bool V4l2Decoder::start()
{
    return false;
}

bool V4l2Decoder::stop()
{
    return true;
}

bool V4l2Decoder::inputPulse(uint32_t index)
{

    return true; // always return true for decode; simply ignored unsupported nal
}

#ifdef __ENABLE_WAYLAND__
bool V4l2Decoder::outputPulse(uint32_t& index)
{
    return true;
}
#elif __ENABLE_EGL__
bool V4l2Decoder::outputPulse(uint32_t& index)
{
    return true;
}
#endif

bool V4l2Decoder::recycleOutputBuffer(int32_t index)
{
    // FIXME, after we remove the extra vpp, renderDone() should come here
    return true;
}

bool V4l2Decoder::recycleInputBuffer(struct v4l2_buffer* dqbuf)
{

    return true;
}

bool V4l2Decoder::acceptInputBuffer(struct v4l2_buffer* qbuf)
{

    return true;
}

bool V4l2Decoder::giveOutputBuffer(struct v4l2_buffer* dqbuf)
{

    return true;
}

#ifndef V4L2_PIX_FMT_VP9
#define V4L2_PIX_FMT_VP9 YAMI_FOURCC('V', 'P', '9', '0')
#endif

int32_t V4l2Decoder::ioctl(int command, void* arg)
{
    DEBUG("fd: %d, ioctl command: %s", m_fd[0], IoctlCommandString(command));
    switch (command) {
    case VIDIOC_QBUF: {
        struct v4l2_buffer* qbuf = static_cast<struct v4l2_buffer*>(arg);
        return onQueueBuffer(qbuf);
    }
    case VIDIOC_DQBUF: {
        struct v4l2_buffer* dqbuf = static_cast<struct v4l2_buffer*>(arg);
        return onDequeBuffer(dqbuf);
    }
    case VIDIOC_STREAMON: {
        __u32 type = *((__u32*)arg);
        return onStreamOn(type);
    }
    case VIDIOC_STREAMOFF: {
        __u32 type = *((__u32*)arg);
        return onStreamOff(type);
    }
    case VIDIOC_QUERYCAP: {
        return V4l2CodecBase::ioctl(command, arg);
    }
    case VIDIOC_REQBUFS: {
        struct v4l2_requestbuffers* reqbufs = static_cast<struct v4l2_requestbuffers*>(arg);
        return onRequestBuffers(reqbufs);
    }
    case VIDIOC_S_FMT: {
        struct v4l2_format* format = static_cast<struct v4l2_format*>(arg);
        return onSetFormat(format);
    }
    case VIDIOC_QUERYBUF: {
        struct v4l2_buffer* buf = static_cast<struct v4l2_buffer*>(arg);
        return onQueryBuffer(buf);
    }
    case VIDIOC_SUBSCRIBE_EVENT: {
        struct v4l2_event_subscription* sub = static_cast<struct v4l2_event_subscription*>(arg);
        return onSubscribeEvent(sub);
    }
    case VIDIOC_DQEVENT: {
        // ::DequeueEvents
        struct v4l2_event* ev = static_cast<struct v4l2_event*>(arg);
        return onDequeEvent(ev);
    }
    case VIDIOC_G_FMT: {
        // ::GetFormatInfo
        struct v4l2_format* format = static_cast<struct v4l2_format*>(arg);
        return onGetFormat(format);
    }
    case VIDIOC_G_CTRL: {
        // ::CreateOutputBuffers
        struct v4l2_control* ctrl = static_cast<struct v4l2_control*>(arg);
        return onGetCtrl(ctrl);
    }
    case VIDIOC_ENUM_FMT: {
        struct v4l2_fmtdesc* fmtdesc = static_cast<struct v4l2_fmtdesc*>(arg);
        return onEnumFormat(fmtdesc);
    }
    case VIDIOC_G_CROP: {
        struct v4l2_crop* crop = static_cast<struct v4l2_crop*>(arg);
        return onGetCrop(crop);
    }
    case VIDIOC_CREATE_BUFS: {
        struct v4l2_create_buffers* req = static_cast<struct v4l2_create_buffers*>(arg);
        return onCreateBuffers(req);
    }
    default: {
        ERROR("unknown ioctrl command: %d", command);
        return -1;
    }
    }
}

bool V4l2Decoder::needReallocation(const VideoFormatInfo* format)
{
    bool ret = m_lastFormat.surfaceWidth != format->surfaceWidth
        || m_lastFormat.surfaceHeight != format->surfaceHeight
        || m_lastFormat.surfaceNumber != format->surfaceNumber
        || m_lastFormat.fourcc != format->fourcc;
    m_lastFormat = *format;
    return ret;
}

VideoDecodeBuffer* V4l2Decoder::peekInput()
{
    uint32_t index;
    if (!m_in.peek(index))
        return NULL;
    ASSERT(index < m_inputFrames.size());
    VideoDecodeBuffer* inputBuffer = &(m_inputFrames[index]);
    return inputBuffer;
}

void V4l2Decoder::consumeInput()
{
    PCHECK(m_thread.isCurrent());
    uint32_t index;
    if (!m_in.get(index)) {
        ERROR("bug: can't get from input");
        return;
    }
    m_in.put(index);
    setDeviceEvent(0);
}

void V4l2Decoder::getInputJob()
{
    PCHECK(m_thread.isCurrent());
    PCHECK(bool(m_decoder));
    if (m_state != kGetInput) {
        DEBUG("early out, state = %d", m_state);
        return;
    }
    VideoDecodeBuffer* inputBuffer = peekInput();
    if (!inputBuffer) {
        DEBUG("early out, no input buffer");
        m_state = kWaitInput;
        return;
    }
    YamiStatus status = m_decoder->decode(inputBuffer);
    DEBUG("decode %d, return %d", (int)inputBuffer->size, (int)status);
    if (status == YAMI_DECODE_FORMAT_CHANGE) {
        const VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
        PCHECK(outFormat);

        if (needReallocation(outFormat)) {
            m_state = kWaitAllocation;
        }
        setCodecEvent();
        DEBUG("early out, format changed to %dx%d, surface size is %dx%d",
            outFormat->width, outFormat->height, outFormat->surfaceWidth, outFormat->surfaceHeight);
        return;
    }
    if (status == YAMI_DECODE_NO_SURFACE) {
        m_state = kWaitSurface;
        DEBUG("early out, no surface");
        return;
    }
    consumeInput();
    if (!m_decoder->getFormatInfo()) {
        DEBUG("need more data to detect output format");
        post(bind(&V4l2Decoder::getInputJob, this));
        return;
    }
    m_state = kGetOutput;
    post(bind(&V4l2Decoder::getOutputJob, this));
}

void V4l2Decoder::inputReadyJob()
{
    PCHECK(m_thread.isCurrent());
    if (m_state != kWaitInput) {
        DEBUG("early out, state = %d", m_state);
        return;
    }
    m_state = kGetInput;
    getInputJob();
}

void V4l2Decoder::getOutputJob()
{
    PCHECK(m_thread.isCurrent());
    PCHECK(bool(m_decoder));
    if (m_state != kGetOutput) {
        DEBUG("early out, state = %d", m_state);
        return;
    }
    while (m_output->isOutputReady()) {
        SharedPtr<VideoFrame> frame = m_decoder->getOutput();
        if (!frame) {
            DEBUG("early out, no frame");
            m_state = kGetInput;
            post(bind(&V4l2Decoder::getInputJob, this));
            return;
        }
        m_output->output(frame);
    }
    m_state = kWaitOutput;
}

void V4l2Decoder::outputReadyJob()
{
    PCHECK(m_thread.isCurrent());
    if (m_state == kWaitOutput) {
        m_state = kGetOutput;
        getOutputJob();
    }
    else if (m_state == kWaitSurface) {
        m_state = kGetInput;
        getInputJob();
    }
}

void V4l2Decoder::checkAllocationJob()
{
    PCHECK(m_thread.isCurrent());
    if (m_state == kWaitAllocation) {
        if (m_output->isAlloccationDone()) {
            m_state = kGetInput;
            getInputJob();
        }
    }
}

int32_t V4l2Decoder::onQueueBuffer(v4l2_buffer* buf)
{
    CHECK(buf);
    uint32_t type = buf->type;
    CHECK(type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        || V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        CHECK(buf->memory == V4L2_MEMORY_MMAP);
        CHECK(buf->length == 1);
        CHECK(buf->index < m_inputFrames.size());
        DEBUG("queue input size = %d", buf->m.planes[0].bytesused);

        VideoDecodeBuffer& inputBuffer = m_inputFrames[buf->index];
        inputBuffer.size = buf->m.planes[0].bytesused; // one plane only
        if (!inputBuffer.size) // EOS
            inputBuffer.data = NULL;
        TIMEVAL_TO_INT64(inputBuffer.timeStamp, buf->timestamp);

        m_in.queue(buf->index);
        post(bind(&V4l2Decoder::inputReadyJob, this));
        return 0;
    }

    m_output->queue(buf);
    post(bind(&V4l2Decoder::outputReadyJob, this));
    return 0;
}

int32_t V4l2Decoder::onDequeBuffer(v4l2_buffer* buf)
{
    CHECK(buf);
    uint32_t type = buf->type;
    CHECK(type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        || V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        CHECK(m_inputOn);
        uint32_t index;
        if (!m_in.deque(index)) {
            ERROR_RETURN(EAGAIN);
        }
        buf->index = index;
        return 0;
    }
    return m_output->deque(buf);
}
int32_t V4l2Decoder::onStreamOn(uint32_t type)
{
    CHECK(type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        || V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {

        if (!m_display) {
            m_display = VaapiDisplay::create(m_nativeDisplay);
            CHECK(bool(m_display));
        }

        CHECK(!m_inputOn);
        m_inputOn = true;
        CHECK(m_thread.start());

        post(bind(&V4l2Decoder::startDecoderJob, this));
        return 0;
    }
    CHECK(!m_outputOn);
    m_outputOn = true;
    post(bind(&V4l2Decoder::checkAllocationJob, this));
    return 0;
}

void V4l2Decoder::flushDecoderJob()
{
    if (m_decoder)
        m_decoder->flush();
    m_out.clearPipe();
    m_state = kStopped;
}
int32_t V4l2Decoder::onStreamOff(uint32_t type)
{
    CHECK(type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        || V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        if (m_inputOn) {
            post(bind(&V4l2Decoder::flushDecoderJob, this));
            m_thread.stop();
            m_in.clearPipe();
            m_inputOn = false;
            m_state = kUnStarted;
        }
        return 0;
    }
    m_outputOn = false;
    m_output->streamOff();
    return 0;
}

int32_t V4l2Decoder::requestInputBuffers(uint32_t count)
{
    CHECK(m_thread.isCurrent());

    uint32_t size = m_inputFormat.fmt.pix_mp.plane_fmt[0].sizeimage;

    m_inputFrames.resize(count);
    if (count) {
        //this means we really need allocate space
        if (!size)
            m_inputFormat.fmt.pix_mp.plane_fmt[0].sizeimage = kDefaultInputSize;
    }
    m_inputSpace.resize(count * size);
    m_inputFrames.resize(count);
    for (uint32_t i = 0; i < count; i++) {
        VideoDecodeBuffer& frame = m_inputFrames[i];
        memset(&frame, 0, sizeof(frame));
        frame.data = &m_inputSpace[i * size];
    }
    return 0;
}

int32_t V4l2Decoder::onRequestBuffers(const v4l2_requestbuffers* req)
{
    CHECK(req);
    uint32_t type = req->type;
    uint32_t count = req->count;
    CHECK(type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
        || V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        CHECK(req->memory == V4L2_MEMORY_MMAP);
        return sendTask(bind(&V4l2Decoder::requestInputBuffers, this, count));
    }
    CHECK(req->memory == V4L2_MEMORY_MMAP);
    return sendTask(bind(&Output::requestBuffers, m_output, count));
}

int32_t V4l2Decoder::onSetFormat(v4l2_format* format)
{
    CHECK(format);
    CHECK(!m_inputOn && !m_outputOn);

    if (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        uint32_t size;
        memcpy(&size, format->fmt.raw_data, sizeof(uint32_t));

        CHECK(size <= (sizeof(format->fmt.raw_data) - sizeof(uint32_t)));

        uint8_t* ptr = format->fmt.raw_data;
        ptr += sizeof(uint32_t);
        m_codecData.assign(ptr, ptr + size);
        return 0;
    }

    if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        CHECK(format->fmt.pix_mp.num_planes == 1);
        CHECK(format->fmt.pix_mp.plane_fmt[0].sizeimage);
        memcpy(&m_inputFormat, format, sizeof(*format));
        return 0;
    }

    ERROR("unknow type: %d of setting format VIDIOC_S_FMT", format->type);
    return -1;
}

int32_t V4l2Decoder::onQueryBuffer(v4l2_buffer* buf)
{
    CHECK(buf);
    CHECK(buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
    CHECK(buf->memory == V4L2_MEMORY_MMAP);
    CHECK(m_inputFormat.fmt.pix_mp.num_planes == 1);

    uint32_t idx = buf->index;
    uint32_t size = m_inputFormat.fmt.pix_mp.plane_fmt[0].sizeimage;
    CHECK(size);
    buf->m.planes[0].length = size;
    buf->m.planes[0].m.mem_offset = size * idx;

    return 0;
}

int32_t V4l2Decoder::onSubscribeEvent(v4l2_event_subscription* sub)
{
    CHECK(sub->type == V4L2_EVENT_RESOLUTION_CHANGE);
    /// resolution change event is must, we always do so
    return 0;
}

int32_t V4l2Decoder::onDequeEvent(v4l2_event* ev)
{
    CHECK(ev);
    if (hasCodecEvent()) {
        ev->type = V4L2_EVENT_RESOLUTION_CHANGE;
        clearCodecEvent();
        return 0;
    }
    return -1;
}

int32_t V4l2Decoder::onGetFormat(v4l2_format* format)
{
    CHECK(format && format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    CHECK(m_inputOn);

    SEND(bind(&V4l2Decoder::getFormatTask, this, format));

    //save it.
    m_outputFormat = *format;
    return 0;
}

int32_t V4l2Decoder::onGetCtrl(v4l2_control* ctrl)
{
    CHECK(ctrl);
    CHECK(ctrl->id == V4L2_CID_MIN_BUFFERS_FOR_CAPTURE);

    SEND(bind(&V4l2Decoder::getCtrlTask, this, ctrl));
    return 0;
}

int32_t V4l2Decoder::onEnumFormat(v4l2_fmtdesc* fmtdesc)
{
    uint32_t type = fmtdesc->type;
    uint32_t index = fmtdesc->index;
    if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
        CHECK(!index);
        fmtdesc->pixelformat = V4L2_PIX_FMT_NV12M;
        return 0;
    }

    if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
        //TODO: query from libyami when capablity api is ready.
        const static uint32_t supported[] = {
            V4L2_PIX_FMT_H264,
            V4L2_PIX_FMT_VC1,
            V4L2_PIX_FMT_MPEG2,
            V4L2_PIX_FMT_JPEG,
            V4L2_PIX_FMT_VP8,
            V4L2_PIX_FMT_VP9,
        };
        CHECK(index < N_ELEMENTS(supported));
        fmtdesc->pixelformat = supported[index];
        return 0;
    }
    return -1;
}

int32_t V4l2Decoder::onGetCrop(v4l2_crop* crop)
{
    CHECK(crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
    ASSERT(0);
    return 0;
    //return sendTask(m_decoderThread, std::bind(getCrop,crop)));
}

int32_t V4l2Decoder::onCreateBuffers(v4l2_create_buffers* req)
{
    return m_output->createBuffers(req);
}

void V4l2Decoder::startDecoderJob()
{
    PCHECK(m_state == kUnStarted);

    if (m_decoder) {
        m_state = kGetInput;
        getInputJob();
        return;
    }

    const char* mime = mimeFromV4l2PixelFormat(m_inputFormat.fmt.pix_mp.pixelformat);

    m_decoder.reset(
        createVideoDecoder(mime), releaseVideoDecoder);
    if (!m_decoder) {
        ERROR("create display failed");
        m_display.reset();
        return;
    }

    m_output->setDecodeAllocator(m_decoder);

    YamiStatus status;
    VideoConfigBuffer config;
    memset(&config, 0, sizeof(config));
    config.width = m_inputFormat.fmt.pix_mp.width;
    config.height = m_inputFormat.fmt.pix_mp.height;
    config.data = &m_codecData[0];
    config.size = m_codecData.size();

    status = m_decoder->start(&config);
    if (status != YAMI_SUCCESS) {
        ERROR("start decoder failed");
        return;
    }
    const VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
    if (outFormat && outFormat->width && outFormat->height) {
        //we got format now, we are waiting for surface allocation.
        m_state = kWaitAllocation;
    }
    else {
        m_state = kGetInput;
        getInputJob();
    }
}

void V4l2Decoder::post(Job job)
{
    m_thread.post(job);
}

static void taskWrapper(int32_t& ret, Task& task)
{
    ret = task();
}

int32_t V4l2Decoder::sendTask(Task task)
{
    //if send fail, we will return EINVAL;
    int32_t ret = EINVAL;
    m_thread.send(bind(taskWrapper, ref(ret), ref(task)));
    return ret;
}

int32_t V4l2Decoder::getFormatTask(v4l2_format* format)
{
    CHECK(m_thread.isCurrent());
    CHECK(format);
    CHECK(bool(m_decoder));

    const VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
    if (!outFormat)
        return EINVAL;

    memset(format, 0, sizeof(*format));
    format->fmt.pix_mp.width = outFormat->width;
    format->fmt.pix_mp.height = outFormat->height;

    //TODO: add support for P010
    format->fmt.pix_mp.num_planes = 2; //for NV12
    format->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;

    //we can't fill format->fmt.pix_mp.plane_fmt[0].bytesperline
    //yet, since we did not creat surface.
    return 0;
}

int32_t V4l2Decoder::getCtrlTask(v4l2_control* ctrl)
{
    CHECK(m_thread.isCurrent());
    CHECK(ctrl);
    CHECK(bool(m_decoder));

    const VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
    if (!outFormat)
        return EINVAL;

    //TODO: query this from outFormat;
    ctrl->value = outFormat->surfaceNumber;
    return 0;
}

#define MCHECK(cond)                     \
    do {                                 \
        if (!(cond)) {                   \
            ERROR("%s is false", #cond); \
            return NULL;                 \
        }                                \
    } while (0)

void* V4l2Decoder::mmap(void* addr, size_t length,
    int prot, int flags, unsigned int offset)
{
    MCHECK(prot == (PROT_READ | PROT_WRITE));
    MCHECK(flags == MAP_SHARED);
    uint32_t size = m_inputFormat.fmt.pix_mp.plane_fmt[0].sizeimage;
    MCHECK(size);
    MCHECK(length == size);
    MCHECK(!(offset % size));
    MCHECK(offset / size < m_inputFrames.size());
    MCHECK(offset + size <= m_inputSpace.size());

    return &m_inputSpace[offset];
}

#undef MCHECK

int32_t V4l2Decoder::setFrameMemoryType(VideoDataMemoryType memory_type)
{
    if (memory_type == VIDEO_DATA_MEMORY_TYPE_EXTERNAL_DMA_BUF)
        m_output.reset(new ExternalDmaBufOutput(this));
#ifdef __ENABLE_EGL__
    if (memory_type == VIDEO_DATA_MEMORY_TYPE_DRM_NAME
        || memory_type == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        m_output.reset(new EglOutput(this, memory_type));
#endif
    if (!m_output) {
        ERROR("unspported memory type %d", memory_type);
        return -1;
    }
    return 0;
}

void V4l2Decoder::flush()
{
}

#ifdef __ENABLE_EGL__
int32_t V4l2Decoder::useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t bufferIndex, void* eglImage)
{
    SharedPtr<EglOutput> output = DynamicPointerCast<EglOutput>(m_output);
    if (!output) {
        ERROR("can't cast m_output to EglOutput");
        return -1;
    }
    return output->useEglImage(eglDisplay, eglContext, bufferIndex, eglImage);
}
#endif
