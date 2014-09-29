/*
 *  encodeInputCamera.cpp - camera video stream for encode input
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include "common/log.h"
#include "encodeinput.h"

#define IOCTL_CHECK_RET(fd, cmd, cmdStr, arg, retVal)  do {     \
        int ret = 0;                                            \
        do {                                                    \
            ret = ioctl(fd, cmd, &(arg));                       \
        } while (-1 == ret && EINTR == errno);                  \
                                                                \
        if (-1 == ret) {                                        \
            ERROR("io cmd %s failed", cmdStr);                  \
            return retVal;                                      \
        }                                                       \
    }while(0)

bool EncodeStreamInputCamera::initDevice(const char *cameraDevicePath)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    bool ret = true;

    INFO();

    m_fd = open(cameraDevicePath, O_RDWR | O_NONBLOCK, 0);

    if (-1 == m_fd) {
        ERROR("Cannot open '%s': %d, %s\n", cameraDevicePath, errno, strerror(errno));
        return false;
    }

    IOCTL_CHECK_RET(m_fd, VIDIOC_QUERYCAP, "VIDIOC_QUERYCAP", cap, false);

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        ERROR("the device doesn't support video capture!!");
        return false;
    }

    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            ERROR("camera device does not support video streaming\n");
            return false;
        }
    break;
    default:
        ERROR("unsupported camera data mode");
    break;
    }

    // set video format and resolution, XXX get supported formats/resolutions first
    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = m_width;
    fmt.fmt.pix.height      = m_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    IOCTL_CHECK_RET(m_fd, VIDIOC_S_FMT, "VIDIOC_S_FMT", fmt, false);
    DEBUG("video resolution: %dx%d = %dx%d", m_width, m_height, fmt.fmt.pix.width, fmt.fmt.pix.height);

    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        ret = initMmap();
        ASSERT(ret);
        break;
    default:
        ERROR("unsupported yet");
        break;
    }

    return true;
}

bool EncodeStreamInputCamera::initMmap(void)
{
    struct v4l2_requestbuffers rqbufs;
    int index;

    INFO();
    memset(&rqbufs, 0, sizeof(rqbufs));
    rqbufs.count = m_frameBufferCount;
    rqbufs.memory = V4L2_MEMORY_MMAP;
    rqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    IOCTL_CHECK_RET(m_fd, VIDIOC_REQBUFS, "VIDIOC_REQBUFS", rqbufs, false);
    INFO("rqbufs.count: %d\n", rqbufs.count);

    m_frameBuffers.resize(rqbufs.count);
    m_frameBufferCount = rqbufs.count;

    DEBUG("map video frames: ");
    for (index = 0; index < rqbufs.count; ++index) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = index;

        IOCTL_CHECK_RET(m_fd, VIDIOC_QUERYBUF, "VIDIOC_QUERYBUF", buf, false);
        if (m_frameBufferSize)
            ASSERT(m_frameBufferSize == buf.length);
        m_frameBufferSize = buf.length;

        m_frameBuffers[index]= (uint8_t*)mmap(NULL, buf.length,
                  PROT_READ | PROT_WRITE, MAP_SHARED,
                  m_fd, buf.m.offset);

        if (MAP_FAILED == m_frameBuffers[index]) {
            ERROR("mmap failed");
            return false;
        }

        DEBUG("index: %d, buf.length: %d, addr: %p", buf.index, buf.length, m_frameBuffers[index]);
    }

    return true;
}

bool EncodeStreamInputCamera::startCapture(void)
{
    unsigned int i;
    enum v4l2_buf_type type;
    int err;

    INFO();
    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        for (i = 0; i < m_frameBufferCount; ++i) {
            struct v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            IOCTL_CHECK_RET(m_fd, VIDIOC_QBUF, "VIDIOC_QBUF", buf, false);
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        IOCTL_CHECK_RET(m_fd, VIDIOC_STREAMON, "VIDIOC_STREAMON", type, false);
        INFO("STREAMON ok\n");
        break;
    default:
        ASSERT(0);
        break;
    }

    return true;
}


bool EncodeStreamInputCamera::init(const char* cameraPath, uint32_t fourcc, int width, int height)
{
    bool ret = true;
    if (!width || !height) {
        return false;
    }
    m_width = width;
    m_height = height;
    m_fourcc = fourcc;
    if (!m_fourcc)
        m_fourcc = VA_FOURCC_YUY2;

    ret = initDevice(cameraPath);
    ASSERT(ret);
    ret = startCapture();
    ASSERT(ret);
    m_dataMode = CAMERA_DATA_MODE_MMAP;

    return true;
}

int32_t EncodeStreamInputCamera::dequeFrame(void)
{
    struct v4l2_buffer buf;
    int ret = 0;
    unsigned int i;
    int retry = 8;

    INFO();
    memset(&buf, 0, sizeof(buf));

    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP: {
        // poll until there is available frames
        while(1) {
            fd_set fds;
            struct timeval tv;

            FD_ZERO(&fds);
            FD_SET(m_fd, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            ret= select(m_fd + 1, &fds, NULL, NULL, &tv);

            if (-1 == ret) {
                if (EINTR == errno)
                    continue;
                ERROR("select failed");
                return -1;
            } else if (0 == ret) {
                ERROR("select timeout\n");
                return -1;
            } else
                break;
        }


        DEBUG("get one frame");
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = m_frameBufferSize;

        do {
            ret = ioctl(m_fd, VIDIOC_DQBUF, &buf);
            if (-1 == ret) {
                usleep(5000);
            } else
                break;
        } while (errno == EAGAIN || EINTR == errno);

        ASSERT(ret != -1);
        ASSERT(buf.index >=0 && buf.index < m_frameBufferCount);
    }
    break;
    default:
        ASSERT(0);
        break;
    }

    return buf.index;
}

bool EncodeStreamInputCamera::enqueFrame(int32_t index)
{
    assert(index >=0 && index < m_frameBufferCount);
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));

    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        DEBUG("recycle one frame (index: %d)\n", index);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = index;
        buf.length = m_frameBufferSize;

        IOCTL_CHECK_RET(m_fd, VIDIOC_QBUF, "VIDIOC_QBUF", buf, false);
        break;
    default:
        ASSERT(0);
        break;
    }
    return true;
}

bool EncodeStreamInputCamera::getOneFrameInput(VideoEncRawBuffer &inputBuffer)
{
    int frameIndex = dequeFrame();
    ASSERT(frameIndex>=0 && frameIndex<m_frameBufferCount);

    memset(&inputBuffer, 0, sizeof(inputBuffer));
    inputBuffer.data = m_frameBuffers[frameIndex];
    inputBuffer.fourcc = m_fourcc;
    inputBuffer.bufAvailable = 0;
    inputBuffer.size = m_frameBufferSize;
    inputBuffer.timeStamp = 0;
    inputBuffer.id = frameIndex;

    return true;
}

bool EncodeStreamInputCamera::recycleOneFrameInput(VideoEncRawBuffer &inputBuffer)
{
    INFO();

    ASSERT(inputBuffer.id>=0 && inputBuffer.id<m_frameBufferCount);
    bool ret = enqueFrame(inputBuffer.id);
    ASSERT(ret);
    return ret;
}

bool EncodeStreamInputCamera::stopCapture(void)
{
    enum v4l2_buf_type type;

    INFO();
    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        IOCTL_CHECK_RET(m_fd, VIDIOC_STREAMOFF, "VIDIOC_STREAMOFF", type, false);
    break;
    default:
        ASSERT(0);
    break;
    }

    return true;
}

bool EncodeStreamInputCamera::uninitDevice()
{
    unsigned int i;

    INFO();
    switch (m_dataMode) {
    case CAMERA_DATA_MODE_MMAP:
        for (i = 0; i < m_frameBufferCount; ++i)
            if (-1 == munmap(m_frameBuffers[i], m_frameBufferSize)) {
                ERROR("munmap failed\n");
                return false;
            }
        break;
    default:
        ASSERT(0);
        break;
    }

    if (-1 == close(m_fd)) {
        ERROR("close device failed\n");
        return false;
    }

    m_fd = -1;
    return true;
}

EncodeStreamInputCamera::~EncodeStreamInputCamera()
{
    bool ret = true;

    ret = stopCapture();
    ASSERT(ret);
    ret = uninitDevice();
    ASSERT(ret);
}


