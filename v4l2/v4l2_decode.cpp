/*
 *  v4l2_decode.cpp
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
#include <linux/videodev2.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

#include "v4l2_decode.h"
#include "interface/VideoDecoderHost.h"
#include "common/log.h"
#include "egl/egl_util.h"

V4l2Decoder::V4l2Decoder()
    : m_videoWidth(0)
    , m_videoHeight(0)
{
    int i;
    m_memoryMode[INPUT] = V4L2_MEMORY_MMAP; // dma_buf hasn't been supported yet
    m_pixelFormat[INPUT] = V4L2_PIX_FMT_H264;
    m_bufferPlaneCount[INPUT] = 1; // decided by m_pixelFormat[INPUT]
    m_memoryMode[OUTPUT] = V4L2_MEMORY_MMAP;
    m_pixelFormat[OUTPUT] = V4L2_PIX_FMT_NV12M;
    m_bufferPlaneCount[OUTPUT] = 2;

    m_maxBufferCount[INPUT] = 8;
    m_maxBufferCount[OUTPUT] = 26; // use a bigger # to satify chrome, the effective count is m_actualOutBufferCount
    m_actualOutBufferCount = m_maxBufferCount[OUTPUT];

    m_inputFrames.resize(m_maxBufferCount[INPUT]);
    m_outputRawFrames.resize(m_maxBufferCount[OUTPUT]);

    for (i=0; i<m_maxBufferCount[INPUT]; i++) {
        memset(&m_inputFrames[i], 0, sizeof(VideoDecodeBuffer));
    }
    for (i=0; i<m_maxBufferCount[OUTPUT]; i++) {
        memset(&m_outputRawFrames[i], 0, sizeof(VideoFrameRawData));
    }

    m_maxBufferSize[INPUT] = 0;
    m_bufferSpace[INPUT] = NULL;
    m_maxBufferSize[OUTPUT] = 0;
    m_bufferSpace[OUTPUT] = NULL;

    m_memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
}

bool V4l2Decoder::start()
{
    Decode_Status status = DECODE_SUCCESS;
    if (m_started)
        return true;
    ASSERT(m_decoder);
    ASSERT(m_configBuffer.profile && m_configBuffer.surfaceNumber);
    status = m_decoder->start(&m_configBuffer);
    ASSERT(status == DECODE_SUCCESS);
    m_started = true;

    return true;
}

bool V4l2Decoder::stop()
{
    m_decoder->stop();

    m_started = false;
    return true;
}

bool V4l2Decoder::inputPulse(int32_t index)
{
    Decode_Status status = DECODE_SUCCESS;

    VideoDecodeBuffer *inputBuffer = &m_inputFrames[index];

    ASSERT(index >= 0 && index < m_maxBufferCount[INPUT]);
    ASSERT(m_maxBufferSize[INPUT] > 0); // update m_maxBufferSize[INPUT] after VIDIOC_S_FMT
    ASSERT(m_bufferSpace[INPUT]);
    ASSERT(inputBuffer->size <= m_maxBufferSize[INPUT]);

    status = m_decoder->decode(inputBuffer);

    if (status == DECODE_FORMAT_CHANGE) {
        setCodecEvent();
        // we can continue decoding no matter what client does reconfiguration now. otherwise, a tri-state ret is required
        status = m_decoder->decode(inputBuffer);
    }

    return true; // always return true for decode; simply ignored unsupported nal
}

bool V4l2Decoder::outputPulse(int32_t &index)
{
    Decode_Status status = DECODE_SUCCESS;
    VideoFrameRawData tempFrame, *frame=NULL;

    ASSERT(index >= 0 && index < m_maxBufferCount[OUTPUT]);
    DEBUG("index: %d", index);

    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        if (!m_bufferSpace[OUTPUT])
            return false;
        frame = &m_outputRawFrames[index];
        ASSERT(frame->handle);
        frame->fourcc = 0;
    }
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME || m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF) {
        if (m_eglImages.empty())
            return false;
        frame = &tempFrame;
        frame->fourcc = VA_FOURCC_BGRX; // XXX, assumed fourcc here
    }
    frame->memoryType = m_memoryType;

    status = m_decoder->getOutput(frame);
    if (status != RENDER_SUCCESS)
        return false;

    DEBUG("got output frame with timestamp: %ld", frame->timeStamp);
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME || m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF) {
        int i;
        bool found = false;
        for (i=0; i< m_actualOutBufferCount; i++) {
            if (frame->internalID == m_outputRawFrames[i].internalID) {
                index = i; // update deque index
                m_outputRawFrames[i].timeStamp = frame->timeStamp;
                found = true;
            }
        }
        ASSERT(found);
    }

    DEBUG("outputPulse: index=%d, timeStamp=%ld, %ld", index, m_outputRawFrames[index].timeStamp, frame->timeStamp);
    return true;
}

bool V4l2Decoder::recycleOutputBuffer(int32_t index)
{
    ASSERT(index >= 0 && index < m_maxBufferCount[OUTPUT]);
    DEBUG("index: %d, handle: 0x%x", index, m_outputRawFrames[index].handle);
    if (m_outputRawFrames[index].handle)
        m_decoder->renderDone(&m_outputRawFrames[index]);

    return true;
}

bool V4l2Decoder::acceptInputBuffer(struct v4l2_buffer *qbuf)
{
    VideoDecodeBuffer *inputBuffer = &(m_inputFrames[qbuf->index]);
    ASSERT(m_maxBufferSize[INPUT] > 0);
    ASSERT(m_bufferSpace[INPUT]);
    ASSERT(qbuf->index >= 0 && qbuf->index < m_maxBufferCount[INPUT]);
    ASSERT(qbuf->length == 1);
    inputBuffer->size = qbuf->m.planes[0].bytesused; // one plane only
    if (!inputBuffer->size) // EOS
        inputBuffer->data = NULL;
    else
        inputBuffer->data = m_bufferSpace[INPUT] + m_maxBufferSize[INPUT]*qbuf->index;
    inputBuffer->timeStamp = qbuf->timestamp.tv_sec;
    // set buffer unit-mode if possible, nal, frame?
    DEBUG("qbuf->index: %d, inputBuffer: %p, timestamp: %ld", qbuf->index, inputBuffer->data, inputBuffer->timeStamp);

    return true;
}

bool V4l2Decoder::giveOutputBuffer(struct v4l2_buffer *dqbuf)
{
    ASSERT(dqbuf);
    int index = dqbuf->index;
    ASSERT(dqbuf->index >= 0 && dqbuf->index < m_actualOutBufferCount);
    // simple set size data to satify chrome even in texture mode
    dqbuf->m.planes[0].bytesused = m_videoWidth * m_videoHeight;
    dqbuf->m.planes[1].bytesused = m_videoWidth * m_videoHeight/2;
    dqbuf->timestamp.tv_sec = m_outputRawFrames[dqbuf->index].timeStamp;

    return true;
}

int32_t V4l2Decoder::ioctl(int command, void* arg)
{
    Decode_Status encodeStatus = DECODE_SUCCESS;
    int32_t ret = 0;
    int port = -1;

    DEBUG("fd: %d, ioctl command: %s", m_fd[0], IoctlCommandString(command));
    switch (command) {
    case VIDIOC_STREAMON:
    case VIDIOC_STREAMOFF:
    case VIDIOC_REQBUFS:
    case VIDIOC_QBUF:
    case VIDIOC_DQBUF:
    case VIDIOC_QUERYCAP:
        ret = V4l2CodecBase::ioctl(command, arg);
        break;
    case VIDIOC_QUERYBUF: {
        struct v4l2_buffer *buffer = static_cast<struct v4l2_buffer*>(arg);
        if (buffer->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            port = INPUT;
        } else if (buffer->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            port = OUTPUT;

        } else {
            ret = -1;
            ERROR("unknow type: %d of query buffer info VIDIOC_QUERYBUF", buffer->type);
            break;
        }

        ASSERT(buffer->memory == m_memoryMode[port]);
        ASSERT(buffer->index >= 0 && buffer->index < m_maxBufferCount[port]);
        ASSERT(buffer->length == m_bufferPlaneCount[port]);
        ASSERT(m_maxBufferSize[port] > 0);

        if (port == INPUT) {
            buffer->m.planes[0].length = m_maxBufferSize[INPUT];
            buffer->m.planes[0].m.mem_offset = m_maxBufferSize[INPUT] * buffer->index;
        } else if (port == OUTPUT) {
            ASSERT(m_maxBufferSize[INPUT] && m_maxBufferCount[INPUT]);
            // plus input buffer space size, it will be minused in mmap
            buffer->m.planes[0].m.mem_offset =  m_maxBufferSize[OUTPUT] * buffer->index;
            buffer->m.planes[0].m.mem_offset += m_maxBufferSize[INPUT] * m_maxBufferCount[INPUT];
            buffer->m.planes[0].length = m_videoWidth * m_videoHeight;
            buffer->m.planes[1].m.mem_offset = buffer->m.planes[0].m.mem_offset + buffer->m.planes[0].length;
            buffer->m.planes[1].length = ((m_videoWidth+1)/2*2) * ((m_videoHeight+1)/2);
        }
    }
    break;
    case VIDIOC_S_FMT: {
        struct v4l2_format *format = static_cast<struct v4l2_format *>(arg);
        ASSERT(!m_streamOn[INPUT] && !m_streamOn[OUTPUT]);
        if (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            // ::Initialize
            ASSERT(format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_NV12M);
        } else if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            // ::CreateInputBuffers
            ASSERT(format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_H264
                || format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_VP8
                || format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_JPEG); // XXX V4L2_PIX_FMT_MJPEG
            ASSERT(format->fmt.pix_mp.num_planes == 1);
            ASSERT(format->fmt.pix_mp.plane_fmt[0].sizeimage);
            switch (format->fmt.pix_mp.pixelformat) {
                case V4L2_PIX_FMT_H264: {
                    m_decoder.reset(createVideoDecoder("video/h264"), releaseVideoDecoder);
                    memset(&m_configBuffer, 0, sizeof(m_configBuffer));
                    m_configBuffer.profile = VAProfileH264Main;
                    m_configBuffer.surfaceNumber = 16;
                    m_configBuffer.data = NULL;
                    m_configBuffer.size = 0;
                }
                break;
                case V4L2_PIX_FMT_VP8:
                    m_decoder.reset(createVideoDecoder("video/vp8"), releaseVideoDecoder);
                    m_configBuffer.profile = VAProfileVP8Version0_3;
                    m_configBuffer.surfaceNumber = 20;
                    m_configBuffer.data = NULL;
                    m_configBuffer.size = 0;
                default:
                    ret = -1;
                break;
            }
            ASSERT(m_decoder);
            m_maxBufferSize[INPUT] = format->fmt.pix_mp.plane_fmt[0].sizeimage;
        } else {
            ret = -1;
            ERROR("unknow type: %d of setting format VIDIOC_S_FMT", format->type);
        }
    }
    break;
    case VIDIOC_SUBSCRIBE_EVENT: {
        // ::Initialize
        struct v4l2_event_subscription *sub = static_cast<struct v4l2_event_subscription*>(arg);
        ASSERT(sub->type == V4L2_EVENT_RESOLUTION_CHANGE);
        // resolution change event is must, we always do so
    }
    break;
    case VIDIOC_DQEVENT: {
        // ::DequeueEvents
        struct v4l2_event *ev = static_cast<struct v4l2_event*>(arg);
        // notify resolution change
        if (hasCodecEvent()) {
            ev->type = V4L2_EVENT_RESOLUTION_CHANGE;
            clearCodecEvent();
        } else
            ret = -1;
    }
    break;
    case VIDIOC_G_FMT: {
        // ::GetFormatInfo
        struct v4l2_format* format = static_cast<struct v4l2_format*>(arg);
        ASSERT(format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
        ASSERT(m_decoder);

        DEBUG();
        const VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
        if (format && outFormat && outFormat->width && outFormat->height) {
            format->fmt.pix_mp.num_planes = m_bufferPlaneCount[OUTPUT];
            format->fmt.pix_mp.width = outFormat->width;
            format->fmt.pix_mp.height = outFormat->height;
            // XXX assumed output format and pitch
            format->fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
            format->fmt.pix_mp.plane_fmt[0].bytesperline = outFormat->width;
            format->fmt.pix_mp.plane_fmt[1].bytesperline = outFormat->width % 2 ? outFormat->width+1 : outFormat->width;
            m_videoWidth = outFormat->width;
            m_videoHeight = outFormat->height;
            m_maxBufferSize[OUTPUT] = m_videoWidth * m_videoHeight + ((m_videoWidth +1)/2*2) * ((m_videoHeight+1)/2);
        } else {
            ret = -1;
            // chromeos accepts EINVAL as not enough input data yet, will try it again.
            errno = EINVAL;
        }
      }
    break;
    case VIDIOC_G_CTRL: {
        // ::CreateOutputBuffers
        struct v4l2_control* ctrl = static_cast<struct v4l2_control*>(arg);
        ASSERT(ctrl->id == V4L2_CID_MIN_BUFFERS_FOR_CAPTURE);
        ASSERT(m_decoder);
        // VideoFormatInfo* outFormat = m_decoder->getFormatInfo();
        ctrl->value = 16; // todo,  outFormat->surfaceNumber;
    }
    break;
    default:
        ret = -1;
        ERROR("unknown ioctrl command: %d", command);
    break;
    }

    if (ret == -1 && errno != EAGAIN) {
        ERROR("ioctl failed");
        WARNING("ioctl command: %s failed", IoctlCommandString(command));
    }

    return ret;
}

/**
 * in order to distinguish input/output buffer, output is <assumed> after input buffer.
 * additional m_maxBufferSize[INPUT] && m_maxBufferCount[INPUT] is added to input buffer offset in VIDIOC_QUERYBUF
 * then minus back in mmap.
*/
void* V4l2Decoder::mmap (void* addr, size_t length,
                      int prot, int flags, unsigned int offset)
{
    int i;
    ASSERT(prot == PROT_READ | PROT_WRITE);
    ASSERT(flags == MAP_SHARED);

    ASSERT(m_maxBufferSize[INPUT] && m_maxBufferCount[INPUT]);

    if (offset < m_maxBufferSize[INPUT] * m_maxBufferCount[INPUT]) { // assume it is input buffer
        if (!m_bufferSpace[INPUT]) {
            m_bufferSpace[INPUT] = static_cast<uint8_t*>(malloc(m_maxBufferSize[INPUT] * m_maxBufferCount[INPUT]));
            for (i=0; i<m_maxBufferCount[INPUT]; i++) {
                m_inputFrames[i].data = m_bufferSpace[INPUT] + m_maxBufferSize[INPUT]*i;
                m_inputFrames[i].size = m_maxBufferSize[INPUT];
            }
        }
        ASSERT(m_bufferSpace[INPUT]);
        return m_bufferSpace[INPUT] + offset;
    } else { // it is output buffer
        offset -= m_maxBufferSize[INPUT] * m_maxBufferCount[INPUT];
        ASSERT(offset <= m_maxBufferSize[OUTPUT] * m_maxBufferCount[OUTPUT]);
        if (!m_bufferSpace[OUTPUT]) {
            m_bufferSpace[OUTPUT] = static_cast<uint8_t*>(malloc(m_maxBufferSize[OUTPUT] * m_maxBufferCount[OUTPUT]));
            for (i=0; i<m_maxBufferCount[OUTPUT]; i++) {
                m_outputRawFrames[i].handle = (intptr_t)(m_bufferSpace[OUTPUT] + m_maxBufferSize[OUTPUT]*i);
                m_outputRawFrames[i].size = m_maxBufferSize[OUTPUT];
                m_outputRawFrames[i].width = m_videoWidth;
                m_outputRawFrames[i].height = m_videoHeight;

                // m_outputRawFrames[i].fourcc = VA_FOURCC_NV12;
                m_outputRawFrames[i].offset[0] = 0;
                m_outputRawFrames[i].offset[1] = m_videoWidth * m_videoHeight;
                m_outputRawFrames[i].pitch[0] = m_videoWidth;
                m_outputRawFrames[i].pitch[1] = m_videoWidth % 2 ? m_videoWidth+1 : m_videoWidth;
            }
        }
        ASSERT(m_bufferSpace[OUTPUT]);
        return m_bufferSpace[OUTPUT] + offset;
    }
}

bool V4l2Decoder::populateOutputFrames(EGLDisplay eglDisplay, EGLContext eglContext)
{
    int i;
    VideoFrameRawData frame;
    Decode_Status status = DECODE_SUCCESS;

    DEBUG("m_videoWidth=%d, m_videoHeight=%d", m_videoWidth, m_videoHeight);
    // negotiate the count of exported frame (image pool) size,
    // for native target fourcc (NV12), it is decided by surface pool size
    // for non-native target fourcc, it is decided (guessed) by the life time of texture/frame rendering
    m_actualOutBufferCount = 0;
    frame.memoryType = m_memoryType;
    frame.fourcc = VA_FOURCC_BGRX;
    status = m_decoder->populateOutputHandles(&frame, m_actualOutBufferCount);
    ASSERT(status == RENDER_SUCCESS && m_actualOutBufferCount);
    ASSERT(m_maxBufferCount[OUTPUT] >= m_actualOutBufferCount);
    DEBUG("m_actualOutBufferCount: %d", m_actualOutBufferCount);

    // populate all possible exported frames
    for (i=0; i<m_maxBufferCount[OUTPUT] ; i++) {
        m_outputRawFrames[i].memoryType = m_memoryType;
        m_outputRawFrames[i].fourcc = VA_FOURCC_BGRX; // VAAPI BGRA match MESA ARGB
        m_outputRawFrames[i].width = m_videoWidth;
        m_outputRawFrames[i].height = m_videoHeight;
        m_outputRawFrames[i].handle = -1;
        m_outputRawFrames[i].internalID = 0;
    }
    status = m_decoder->populateOutputHandles(&m_outputRawFrames[0], m_actualOutBufferCount /* actual count only*/);
    ASSERT(status == RENDER_SUCCESS);

    // binding EGLimage for each frame
    m_eglImages.resize(m_actualOutBufferCount);
    for (i=0; i<m_eglImages.size(); i++) {
        m_eglImages[i]  = createEglImageFromHandle(eglDisplay, eglContext, m_outputRawFrames[i].memoryType, m_outputRawFrames[i].handle,
            m_outputRawFrames[i].width, m_outputRawFrames[i].height, m_outputRawFrames[i].pitch[0]);

        m_decoder->renderDone(&m_outputRawFrames[i]);
        ASSERT(m_eglImages[i]);
    }

    return true;
}

int32_t V4l2Decoder::useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t bufferIndex, void* eglImage)
{
    EGLImageKHR image = EGL_NO_IMAGE_KHR;

    // EGLImage is requested before the first frame (associated with it) is ready; we can't create EGLImage when it is requested for the first time.
    if (m_eglImages.empty())
        populateOutputFrames(eglDisplay, eglContext);

    ASSERT(m_memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME || m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
    ASSERT(bufferIndex>=0 && bufferIndex<m_maxBufferCount[OUTPUT]);
    bufferIndex = std::min(bufferIndex, m_actualOutBufferCount);
    *(EGLImageKHR*)eglImage = m_eglImages[bufferIndex];

    return 0;
}

