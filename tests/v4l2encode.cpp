/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include  <sys/mman.h>

#include "common/log.h"
#include "common/utils.h"
#include "v4l2/v4l2_wrapper.h"
#include "encodehelp.h"
#include "encodeinput.h"

static VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
uint32_t inputFramePlaneCount = 3; // I420(default) format has 3 planes
uint32_t inputFrameSize = 0;

const uint32_t kMaxFrameQueueLength = 8;
uint32_t inputQueueCapacity = 0;
uint32_t outputQueueCapacity = 0;
VideoFrameRawData inputFrames[kMaxFrameQueueLength];
uint8_t *outputFrames[kMaxFrameQueueLength];
bool isReadEOS = false;
bool isEncodeEOS = false;
bool isOutputEOS = false;

static EncodeInput* streamInput;

bool readOneFrameData(uint32_t index)
{
    static int encodeFrameCount = 0;

    if (!streamInput)
        return false;
    if (isReadEOS)
        return false;

    ASSERT(index<inputQueueCapacity);

    bool ret = streamInput->getOneFrameInput(inputFrames[index]);
    if (!ret || (frameCount && encodeFrameCount++>=frameCount)) {
        isReadEOS = true;
        return false;
    }

    return ret;
}

void fillV4L2Buffer(struct v4l2_buffer& buf, const VideoFrameRawData& frame)
{
    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_POINTER) {
        uint32_t width[3];
        uint32_t height[3];
        uint32_t planes;
        bool ret;

        ret = getPlaneResolution(frame.fourcc, frame.width, frame.height, width, height, planes);
        ASSERT(ret && "get planes resolution failed");
        unsigned long data = (unsigned long)frame.handle;
        for (uint32_t i = 0; i < planes; i++) {
            buf.m.planes[i].bytesused = width[i] * height[i];
            buf.m.planes[i].m.userptr = data + frame.offset[i];
        }
    }
    else if (memoryType == VIDEO_DATA_MEMORY_TYPE_ANDROID_NATIVE_BUFFER) {
        // !!! FIXME, v4l2 use long for userptr. so bad
        DEBUG("ANativeWindowBuffer, frame.handle: %p", (void*)frame.handle);
        buf.m.planes[0].m.userptr = (long)((intptr_t)frame.handle);
        buf.m.planes[0].bytesused = sizeof(buf.m.planes[0].m.userptr);
    }
    else
        ASSERT(0 && "unknown memory type");
}

bool feedOneInputFrame(int fd, int index = -1 /* if index is not -1, simple enque it*/)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ioctlRet = -1;
    static uint32_t dqCountAfterEOS = 0;

    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.m.planes = planes;
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE; // it indicates input buffer(raw frame) type
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.length = inputFramePlaneCount;

    if (index == -1) {
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_DQBUF, &buf);
        if (ioctlRet == -1)
            return false;
        // recycle camera buffer
        bool ret = streamInput->recycleOneFrameInput(inputFrames[buf.index]);
        ASSERT(ret);
    } else {
        buf.index = index;
    }

    if (isReadEOS) {
        dqCountAfterEOS++;
        if (dqCountAfterEOS == inputQueueCapacity)
            isEncodeEOS = true;
        return false;
    }
    if (!readOneFrameData(buf.index)) {
        ASSERT(isReadEOS);
    }

    if (isReadEOS) {
        // send EOS to device
        buf.m.planes[0].m.userptr = buf.m.planes[1].m.userptr = buf.m.planes[2].m.userptr = 0;
        buf.m.planes[0].bytesused = buf.m.planes[1].bytesused = buf.m.planes[2].bytesused = 0;
    } else {
        fillV4L2Buffer(buf, inputFrames[buf.index]);
    }

    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);

    return true;
}

bool writeOneOutputFrame(uint8_t* data, uint32_t dataSize)
{
    static FILE* fp = NULL;

    if(!fp) {
        fp = fopen(outputFileName, "w+");
        if (!fp) {
            fprintf(stderr, "fail to open file: %s\n", outputFileName);
            return false;
        }
    }

    if (fwrite(data, 1, dataSize, fp) != dataSize) {
        ASSERT(0);
        return false;
    }

    if(isOutputEOS)
        fclose(fp);

    return true;

}

bool takeOneOutputFrame(int fd, int index = -1/* if index is not -1, simple enque it*/)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[1];
    int ioctlRet = -1;

    memset(&buf, 0, sizeof(buf));
    memset(planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; //it indicates output buffer type
    buf.memory = V4L2_MEMORY_MMAP; // chromeos v4l2vea uses this mode only
    buf.m.planes = planes;
    buf.length = 1;

    if (index == -1) {
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_DQBUF, &buf);

        if (isEncodeEOS) {
            if (ioctlRet == -1) {
                isOutputEOS = true;
                YamiV4L2_SetDevicePollInterrupt(fd);
                return true;
            }
        }
        if (ioctlRet == -1)
            return false;

        if (!writeOneOutputFrame(outputFrames[buf.index], buf.bytesused)) {
            ASSERT(0);
            return false;
        }
    } else {
        buf.index = index;
    }

    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);

    return true;
}

int main(int argc, char** argv)
{
    int32_t fd = -1;
    uint32_t i = 0;
    int32_t ioctlRet = -1;

    yamiTraceInit();

    // parse command line parameters
    if (!process_cmdline(argc, argv))
        return -1;

#if ANDROID
    if (!inputFileName) {
        memoryType = VIDEO_DATA_MEMORY_TYPE_ANDROID_NATIVE_BUFFER;
        inputFourcc = VA_FOURCC_NV12;
    }
#endif
    streamInput = EncodeInput::create(inputFileName, inputFourcc, videoWidth, videoHeight);
    ASSERT(streamInput);

    // open device
    fd = YamiV4L2_Open("encoder", 0);
    ASSERT(fd!=-1);

    // query hw capability
    struct v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    caps.capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYCAP, &caps);
    ASSERT(ioctlRet != -1);

    // set input/output data format
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    DEBUG_FOURCC("inputFourcc", inputFourcc);
    switch (inputFourcc) {
    case VA_FOURCC_YV12:
    case VA_FOURCC('I', '4', '2', '0'):
        format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420M;
        inputFramePlaneCount = 3;
        inputFrameSize = videoWidth*videoHeight*3/2;
        break;
    case VA_FOURCC_NV12:
        format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        inputFramePlaneCount = 2;
        inputFrameSize = videoWidth*videoHeight*3/2;
        break;
    case VA_FOURCC_YUY2:
        format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUYV;
        inputFramePlaneCount = 1;
        inputFrameSize = videoWidth*videoHeight*2;
        break;
    default:
        ASSERT(0);
        break;
    }
    format.fmt.pix_mp.width = videoWidth;
    format.fmt.pix_mp.height = videoHeight;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    // set input buffer type
    YamiV4L2_FrameMemoryType(fd, memoryType);

    // set framerate
    struct v4l2_streamparm parms;
    memset(&parms, 0, sizeof(parms));
    parms.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    parms.parm.output.timeperframe.denominator = 1;
    parms.parm.output.timeperframe.numerator = fps;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_PARM, &parms);
    ASSERT(ioctlRet != -1);

    // set bitrate
    struct v4l2_ext_control ctrls[1];
    struct v4l2_ext_controls control;
    memset(&ctrls, 0, sizeof(ctrls));
    memset(&control, 0, sizeof(control));
    ctrls[0].id = V4L2_CID_MPEG_VIDEO_BITRATE;
    ctrls[0].value = bitRate;
    control.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    control.count = 1;
    control.controls = ctrls;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_EXT_CTRLS, &control);
    ASSERT(ioctlRet != -1);

    // other controls
    memset(&ctrls, 0, sizeof(ctrls));
    memset(&control, 0, sizeof(control));
    // No B-frames, for lowest decoding latency.
    ctrls[0].id = V4L2_CID_MPEG_VIDEO_B_FRAMES;
    ctrls[0].value = 0;
    control.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    control.count = 1;
    control.controls = ctrls;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_EXT_CTRLS, &control);
    ASSERT(ioctlRet != -1);

    // start
    __u32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);

    // setup input buffers
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_USERPTR;
    reqbufs.count = 2;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);
    ASSERT(reqbufs.count>0 && reqbufs.count <= kMaxFrameQueueLength);
    inputQueueCapacity = reqbufs.count;

#if ANDROID
    if (memoryType != VIDEO_DATA_MEMORY_TYPE_ANDROID_NATIVE_BUFFER)
#endif
    {
        for (i = 0; i < inputQueueCapacity; i++) {
            inputFrames[i].handle = reinterpret_cast<intptr_t>(malloc(inputFrameSize));
        }
        for (i = inputQueueCapacity; i < kMaxFrameQueueLength; i++)
            inputFrames[i].handle = 0;
    }

    // setup output buffers
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 2;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);
    ASSERT(reqbufs.count>0 && reqbufs.count <= kMaxFrameQueueLength);
    outputQueueCapacity = reqbufs.count;

    for (i=0; i<outputQueueCapacity; i++) {
        struct v4l2_plane planes[1];
        struct v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        memset(planes, 0, sizeof(planes));
        buffer.index = i;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.m.planes = planes;
        buffer.length = 1;
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYBUF, &buffer);
        ASSERT(ioctlRet != -1);
        void* address = YamiV4L2_Mmap(NULL,
                                      buffer.m.planes[0].length,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd,
                                      buffer.m.planes[0].m.mem_offset);
        ASSERT(address);
        outputFrames[i] = static_cast<uint8_t*>(address);
        // ignore lenght here
    }
    for (i=outputQueueCapacity; i<kMaxFrameQueueLength; i++) {
        outputFrames[i] = NULL;
    }

    // feed input/output frames first
    for (i=0; i<inputQueueCapacity; i++) {
        if (!feedOneInputFrame(fd, i)) {
            ASSERT(0);
        }
    }

    for (i=0; i<outputQueueCapacity; i++) {
        if (!takeOneOutputFrame(fd, i)) {
            ASSERT(0);
        }
    }

    bool event_pending=true;
    do {
        takeOneOutputFrame(fd);
        feedOneInputFrame(fd);
        if (isReadEOS)
            break;
    } while (YamiV4L2_Poll(fd, true, &event_pending) == 0);

    // drain input buffer
    ASSERT(isReadEOS);
    while (!isEncodeEOS) {
        takeOneOutputFrame(fd);
        feedOneInputFrame(fd);
        usleep(10000);
    }

    // drain output buffer
    // stop input port to indicate EOS
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);
    ASSERT(isEncodeEOS);
    while (!isOutputEOS) {
        usleep(10000);
        takeOneOutputFrame(fd);
    }

    ASSERT(isOutputEOS);
    // YamiV4L2_Munmap(void* addr, size_t length)

    // release queued input/output buffer
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_USERPTR;
    reqbufs.count = 0;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 0;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    // stop output prot
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);

    // close device
    ioctlRet = YamiV4L2_Close(fd);
    ASSERT(ioctlRet != -1);

    fprintf(stderr, "encode done\n");
}

