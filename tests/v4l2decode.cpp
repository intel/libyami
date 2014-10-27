/*
 *  v4l2_encoder.cpp - a h264 encoder basing on v4l2 wrapper interface
 *
 *  Copyright (C) 2011-2014 Intel Corporation
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <errno.h>
#include  <sys/mman.h>
#include <vector>

#include "common/log.h"
#include "v4l2/v4l2_wrapper.h"
#include "decodeinput.h"
#include "./egl/gles2_help.h"
#include "interface/VideoCommonDefs.h"

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif

struct RawFrameData {
    uint32_t width;
    uint32_t height;
    uint32_t pitch[3];
    uint32_t offset[3];
    uint32_t fourcc;            //NV12
    uint8_t *data;
};

const uint32_t k_maxInputBufferSize = 1024*1024;
const int k_inputPlaneCount = 1;
const int k_maxOutputPlaneCount = 3;
int outputPlaneCount = 2;
int videoWidth = 0;
int videoHeight = 0;

uint32_t inputQueueCapacity = 0;
uint32_t outputQueueCapacity = 0;
static std::vector<uint8_t*> inputFrames;
static std::vector<struct RawFrameData> rawOutputFrames;
VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;

static FILE* outfp = NULL;
static Display * x11Display = NULL;
static Window window = 0;
static EGLContextType *context = NULL;
static std::vector<EGLImageKHR> eglImages;
static std::vector<GLuint> textureIds;
static bool isReadEOS=false;

bool feedOneInputFrame(DecodeStreamInput * input, int fd, int index = -1 /* if index is not -1, simple enque it*/)
{

    VideoDecodeBuffer inputBuffer;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[k_inputPlaneCount];
    int ioctlRet = -1;
    static int32_t dqCountAfterEOS = 0;

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE; // it indicates input buffer(raw frame) type
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = planes;
    buf.length = k_inputPlaneCount;

    if (index == -1) {
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_DQBUF, &buf);
        if (ioctlRet == -1)
            return true;
    } else {
        buf.index = index;
    }

    if (isReadEOS)
        return false;

    if (!input->getNextDecodeUnit(inputBuffer)) {
        // send empty buffer for EOS
        buf.m.planes[0].bytesused = 0;
        isReadEOS = true;
    } else {
        ASSERT(inputBuffer.size <= k_maxInputBufferSize);
        memcpy(inputFrames[buf.index], inputBuffer.data, inputBuffer.size);
        buf.m.planes[0].bytesused = inputBuffer.size;
        buf.m.planes[0].m.mem_offset = 0;
    }

    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);

    return true;
}

bool dumpOneVideoFrame(int32_t index)
{
    int row;

    if (!outfp)
        outfp = fopen("out.raw", "w+");

    if (!outfp)
        return false;

    // Y plane
    for (row=0; row<rawOutputFrames[index].height; row++) {
        fwrite(rawOutputFrames[index].data + rawOutputFrames[index].offset[0] + rawOutputFrames[index].pitch[0] * row, rawOutputFrames[index].width, 1, outfp);
    }
    // UV plane
    for (row=0; row<(rawOutputFrames[index].height+1)/2; row++) {
        fwrite(rawOutputFrames[index].data + rawOutputFrames[index].offset[1] + rawOutputFrames[index].pitch[1] * row, (rawOutputFrames[index].width+1)/2*2, 1, outfp);
    }

    return true;
}

EGLContextType *createEglContext()
{
    EGLContextType *eglContext = NULL;
    x11Display = XOpenDisplay(NULL);
    if (!x11Display)
        return NULL;

    ASSERT(videoWidth && videoHeight);
    window = XCreateSimpleWindow(x11Display, DefaultRootWindow(x11Display)
        , 0, 0, videoWidth, videoHeight, 0, 0
        , WhitePixel(x11Display, 0));
    XMapWindow(x11Display, window);
    XSync(x11Display, false);
    eglContext = eglInit(x11Display, window, 0 /*VA_FOURCC_RGBA*/);

    return eglContext;
}

bool displayOneVideoFrameEGL(int32_t fd, int32_t index)
{
    ASSERT(context && textureIds.size());
    ASSERT(index>=0 && index<textureIds.size());
    DEBUG("textureIds[%d] = 0x%x", index, textureIds[index]);
    drawTextures(context, &textureIds[index], 1);

    return true;
}

bool takeOneOutputFrame(int fd, int index = -1/* if index is not -1, simple enque it*/)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[k_maxOutputPlaneCount]; // YUV output, in fact, we use NV12 of 2 planes
    int ioctlRet = -1;
    bool ret = true;
    static int outputFrameCount = 0;

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE; //it indicates output buffer type
    buf.memory = V4L2_MEMORY_MMAP; // chromeos v4l2vea uses this mode only
    buf.m.planes = planes;
    buf.length = outputPlaneCount;

    if (index == -1) {
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_DQBUF, &buf);
        if (ioctlRet == -1)
            return false;

        outputFrameCount++;
        if (memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
            ret = displayOneVideoFrameEGL(fd, buf.index);
        else
            ret = dumpOneVideoFrame(buf.index);
        ASSERT(ret);
    } else {
        buf.index = index;
    }

    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);
    INFO("outputFrameCount: %d", outputFrameCount);
    return true;
}

bool handleResolutionChange(int32_t fd)
{
    bool resolutionChanged = false;
    // check resolution change
    struct v4l2_event ev;
    memset(&ev, 0, sizeof(ev));

    while (YamiV4L2_Ioctl(fd, VIDIOC_DQEVENT, &ev) == 0) {
        if (ev.type == V4L2_EVENT_RESOLUTION_CHANGE) {
            resolutionChanged = true;
            break;
        }
    }

    if (!resolutionChanged)
        return false;

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (YamiV4L2_Ioctl(fd, VIDIOC_G_FMT, &format) == -1) {
        return false;
    }

    // resolution and pixelformat got here
    outputPlaneCount = format.fmt.pix_mp.num_planes;
    ASSERT(outputPlaneCount == 2);
    videoWidth = format.fmt.pix_mp.width;
    videoHeight = format.fmt.pix_mp.height;

    return true;
}

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i media file to decode\n");
    printf("   -m <render mode>\n");
    printf("      0: dump video frame to file\n");
    printf("      1: texture: export video frame as drm name (RGBX) + texture from drm name\n");
    printf("      2: texture: export video frame as dma_buf(RGBX) + texutre from dma_buf\n");
    printf("      3: texture: export video frame as dma_buf(NV12) + texture from dma_buf\n");
}

int main(int argc, char** argv)
{
    char *fileName = NULL;
    int renderMode = 1;
    DecodeStreamInput *input;
    int32_t fd = -1;
    int32_t i = 0;
    int32_t ioctlRet = -1;
    char opt;

    yamiTraceInit();

    while ((opt = getopt(argc, argv, "h:m:i:?:")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
            fileName = optarg;
            break;
        case 'm':
            renderMode = atoi(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    if (!fileName) {
        fprintf(stderr, "no input media file specified\n");
        return -1;
    }
    fprintf(stderr, "input file: %s, renderMode: %d", fileName, renderMode);

    switch (renderMode) {
    case 0:
        memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_COPY;
    break;
    case 1:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
    break;
    case 2:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
    break;
    default:
        ASSERT("");
    break;
    }

    input = DecodeStreamInput::create(fileName);
    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    // open device
    fd = YamiV4L2_Open("decoder", 0);
    ASSERT(fd!=-1);

    // set output frame memory type
    YamiV4L2_FrameMemoryType(fd, memoryType);

    // query hw capability
    struct v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    caps.capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYCAP, &caps);
    ASSERT(ioctlRet != -1);

    // set input/output data format
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    format.fmt.pix_mp.num_planes = 1;
    format.fmt.pix_mp.plane_fmt[0].sizeimage = k_maxInputBufferSize;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    // set preferred output format
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12M;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    // start input port
    __u32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);

    // start output port
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);

    // setup input buffers
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 2;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);
    ASSERT(reqbufs.count>0);
    inputQueueCapacity = reqbufs.count;
    inputFrames.resize(inputQueueCapacity);

    for (i=0; i<inputQueueCapacity; i++) {
        struct v4l2_plane planes[k_inputPlaneCount];
        struct v4l2_buffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        memset(planes, 0, sizeof(planes));
        buffer.index = i;
        buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.m.planes = planes;
        buffer.length = k_inputPlaneCount;
        ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYBUF, &buffer);
        ASSERT(ioctlRet != -1);

        // length and mem_offset should be filled by VIDIOC_QUERYBUF above
        void* address = YamiV4L2_Mmap(NULL,
                                      buffer.m.planes[0].length,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED, fd,
                                      buffer.m.planes[0].m.mem_offset);
        ASSERT(address);
        inputFrames[i] = static_cast<uint8_t*>(address);
        DEBUG("inputFrames[%d] = %p", i, inputFrames[i]);
    }

    // feed input frames first
    for (i=0; i<inputQueueCapacity; i++) {
        if (!feedOneInputFrame(input, fd, i)) {
            ASSERT(0);
        }
    }

    // query video resolution
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    while (1) {
        if (YamiV4L2_Ioctl(fd, VIDIOC_G_FMT, &format) != 0) {
            if (errno != EINVAL) {
                // EINVAL means we haven't seen sufficient stream to decode the format.
                INFO("ioctl() failed: VIDIOC_G_FMT, haven't get video resolution during start yet, waiting");
            }
        } else {
            break;
        }
        usleep(50);
    }
    outputPlaneCount = format.fmt.pix_mp.num_planes;
    ASSERT(outputPlaneCount == 2);
    videoWidth = format.fmt.pix_mp.width;
    videoHeight = format.fmt.pix_mp.height;
    ASSERT(videoWidth && videoHeight);

    // setup output buffers
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = outputPlaneCount;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);
    ASSERT(reqbufs.count>0);
    outputQueueCapacity = reqbufs.count;

    if (memoryType == VIDEO_DATA_MEMORY_TYPE_RAW_COPY) {
        rawOutputFrames.resize(outputQueueCapacity);
        for (i=0; i<outputQueueCapacity; i++) {
            struct v4l2_plane planes[k_maxOutputPlaneCount];
            struct v4l2_buffer buffer;
            memset(&buffer, 0, sizeof(buffer));
            memset(planes, 0, sizeof(planes));
            buffer.index = i;
            buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buffer.memory = V4L2_MEMORY_MMAP;
            buffer.m.planes = planes;
            buffer.length = outputPlaneCount;
            ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYBUF, &buffer);
            ASSERT(ioctlRet != -1);

            rawOutputFrames[i].width = format.fmt.pix_mp.width;
            rawOutputFrames[i].height = format.fmt.pix_mp.height;
            rawOutputFrames[i].fourcc = format.fmt.pix_mp.pixelformat;

            for (int j=0; j<outputPlaneCount; j++) {
                // length and mem_offset are filled by VIDIOC_QUERYBUF above
                void* address = YamiV4L2_Mmap(NULL,
                                              buffer.m.planes[j].length,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, fd,
                                              buffer.m.planes[j].m.mem_offset);
                ASSERT(address);
                if (j == 0) {
                    rawOutputFrames[i].data = static_cast<uint8_t*>(address);
                    rawOutputFrames[i].offset[0] = 0;
                } else {
                    rawOutputFrames[i].offset[j] = static_cast<uint8_t*>(address) - rawOutputFrames[i].data;
                }

                rawOutputFrames[i].pitch[j] = format.fmt.pix_mp.plane_fmt[j].bytesperline;
            }
        }
    } else if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF || memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME) {
        // setup all textures and eglImages
        eglImages.resize(outputQueueCapacity);
        textureIds.resize(outputQueueCapacity);

        if (!context)
            context = createEglContext();
        glGenTextures(outputQueueCapacity, &textureIds[0] );
        for (i=0; i<outputQueueCapacity; i++) {
             int ret = 0;
             ret = YamiV4L2_UseEglImage(fd, context->eglContext.display, context->eglContext.context, i, &eglImages[i]);
             ASSERT(ret == 0);
             glBindTexture(GL_TEXTURE_2D, textureIds[i]);
             glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImages[i]);

             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
             glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
             DEBUG("textureIds[%d]: 0x%x, eglImages[%d]: 0x%x", i, textureIds[i], i, eglImages[i]);
        }
    }

    // feed output frames first
    for (i=0; i<outputQueueCapacity; i++) {
        if (!takeOneOutputFrame(fd, i)) {
            ASSERT(0);
        }
    }

    bool event_pending=true; // try to get video resolution.
    int dqCountAfterEOS = 0;
    do {
        if (event_pending) {
            handleResolutionChange(fd);
        }

        takeOneOutputFrame(fd);
        if (!feedOneInputFrame(input, fd)) {
            dqCountAfterEOS++;
        }
        if (dqCountAfterEOS == inputQueueCapacity)  // input drain
            break;
    } while (YamiV4L2_Poll(fd, true, &event_pending) == 0);

    // drain output buffer
    int retry = 3;
    while (takeOneOutputFrame(fd) || (--retry)>0) { // output drain
        usleep(10000);
    }

    // YamiV4L2_Munmap(void* addr, size_t length)

    // release queued input/output buffer
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 0;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 0;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    // stop input port
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);

    // stop output port
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);

    if(textureIds.size())
        glDeleteTextures(textureIds.size(), &textureIds[0]);

    for (i=0; i<eglImages.size(); i++) {
        eglDestroyImageKHR(context->eglContext.display, eglImages[i]);
    }
    /*
    there is still randomly fail in mesa; no good idea for it. seems mesa bug
    0  0x00007ffff079c343 in _mesa_symbol_table_dtor () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    1  0x00007ffff073c55d in glsl_symbol_table::~glsl_symbol_table() () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    2  0x00007ffff072a4d5 in ?? () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    3  0x00007ffff072a4bd in ?? () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    4  0x00007ffff064b48f in _mesa_reference_shader () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    5  0x00007ffff0649397 in ?? () from /usr/lib/x86_64-linux-gnu/libdricore9.2.1.so.1
    6  0x000000000040624d in releaseShader (program=0x77cd90) at ./egl/gles2_help.c:158
    7  eglRelease (context=0x615920) at ./egl/gles2_help.c:310
    8  0x0000000000402ca8 in main (argc=<optimized out>, argv=<optimized out>) at v4l2decode.cpp:531
    */
    if (context)
        eglRelease(context);

    // close device
    ioctlRet = YamiV4L2_Close(fd);
    ASSERT(ioctlRet != -1);

    if(input)
        delete input;

    if (outfp)
        fclose(outfp);

    if (x11Display && window)
        XDestroyWindow(x11Display, window);
    if (x11Display)
        XCloseDisplay(x11Display);

    fprintf(stderr, "decode done\n");
}

