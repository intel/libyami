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
#include "common/utils.h"
#include "v4l2/v4l2_wrapper.h"
#include "decodeinput.h"
#include "decodehelp.h"
#if __ENABLE_V4L2_GLX__
#include "./glx/glx_help.h"
#else
#include "./egl/gles2_help.h"
#endif
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
static Window x11Window = 0;
#if __ENABLE_V4L2_GLX__
static GLXContextType *glxContext;
std::vector <Pixmap> pixmaps;
std::vector <GLXPixmap> glxPixmaps;
#else
static EGLContextType *eglContext = NULL;
static std::vector<EGLImageKHR> eglImages;
#endif
static std::vector<GLuint> textureIds;
static bool isReadEOS=false;
static int32_t stagingBufferInDevice = 0;
static uint32_t renderFrameCount = 0;

bool feedOneInputFrame(DecodeInput * input, int fd, int index = -1 /* if index is not -1, simple enque it*/)
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
        stagingBufferInDevice --;
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

    stagingBufferInDevice ++;
    return true;
}

bool dumpOneVideoFrame(int32_t index)
{
    int row;

    if (!outfp) {
        char outFileName[256];
        char* baseFileName = inputFileName;
        char *s = strrchr(inputFileName, '/');
        if (s)
            baseFileName = s+1;
        // V4L2 reports fourcc as NM12 (planar NV12), use hard code here
        sprintf(outFileName, "%s/%s_%dx%d.NV12", dumpOutputDir, baseFileName, rawOutputFrames[index].width, rawOutputFrames[index].height);
        DEBUG("outFileName: %s", outFileName);
        outfp = fopen(outFileName, "w+");
    }

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

#if __ENABLE_V4L2_GLX__
static bool displayOneVideoFrameGLX(int32_t fd, int32_t index)
{
    ASSERT(glxContext && textureIds.size());
    ASSERT(index>=0 && index<textureIds.size());
    DEBUG("textureIds[%d] = 0x%x", index, textureIds[index]);

    int ret = drawTexture(glxContext, textureIds[index]);
    return ret == 0;
}
#else
bool displayOneVideoFrameEGL(int32_t fd, int32_t index)
{
    ASSERT(eglContext && textureIds.size());
    ASSERT(index>=0 && index<textureIds.size());
    DEBUG("textureIds[%d] = 0x%x", index, textureIds[index]);

    GLenum target = GL_TEXTURE_2D;
    if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        target = GL_TEXTURE_EXTERNAL_OES;
    int ret = drawTextures(eglContext, target, &textureIds[index], 1);

    return ret == 0;
}
#endif


bool takeOneOutputFrame(int fd, int index = -1/* if index is not -1, simple enque it*/)
{
    struct v4l2_buffer buf;
    struct v4l2_plane planes[k_maxOutputPlaneCount]; // YUV output, in fact, we use NV12 of 2 planes
    int ioctlRet = -1;
    bool ret = true;

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

        renderFrameCount++;
#if __ENABLE_V4L2_GLX__
        ret = displayOneVideoFrameGLX(fd, buf.index);
#else
        if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF || memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
            ret = displayOneVideoFrameEGL(fd, buf.index);
        else
            ret = dumpOneVideoFrame(buf.index);
#endif
        ASSERT(ret);
    } else {
        buf.index = index;
    }

    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);
    INFO("renderFrameCount: %d", renderFrameCount);
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

int main(int argc, char** argv)
{
    DecodeInput *input;
    int32_t fd = -1;
    int32_t i = 0;
    int32_t ioctlRet = -1;
    char opt;
    YamiMediaCodec::CalcFps calcFps;

    renderMode = 3; // set default render mode to 3

    yamiTraceInit();
    XInitThreads();

    if (!process_cmdline(argc, argv))
        return -1;

    if (!inputFileName) {
        ERROR("no input media file specified\n");
        return -1;
    }
    INFO("input file: %s, renderMode: %d", inputFileName, renderMode);

    if (!dumpOutputDir)
        dumpOutputDir = strdup ("./");

#if !__ENABLE_V4L2_GLX__
    switch (renderMode) {
    case 0:
        memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_COPY;
    break;
    case 3:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
    break;
    case 4:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
    break;
    default:
        ASSERT(0 && "unsupported render mode, -m [0,3,4] are supported");
    break;
    }
#endif

    input = DecodeInput::create(inputFileName);
    if (input==NULL) {
        ERROR("fail to init input stream\n");
        return -1;
    }

    renderFrameCount = 0;
    calcFps.setAnchor();
    // open device
    fd = YamiV4L2_Open("decoder", 0);
    ASSERT(fd!=-1);

    x11Display = XOpenDisplay(NULL);
    ASSERT(x11Display);
#if __ENABLE_V4L2_GLX__
    ioctlRet = YamiV4L2_SetXDisplay(fd, x11Display);
#endif
    // set output frame memory type
    YamiV4L2_FrameMemoryType(fd, memoryType);

    // query hw capability
    struct v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    caps.capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
    ioctlRet = YamiV4L2_Ioctl(fd, VIDIOC_QUERYCAP, &caps);
    ASSERT(ioctlRet != -1);

    // set input/output data format
    uint32_t codecFormat = 0;
    const char* mimeType = input->getMimeType();
    if (!strcmp(mimeType, "video/h264"))
        codecFormat = V4L2_PIX_FMT_H264;
    else if (!strcmp(mimeType, "video/x-vnd.on2.vp8"))
        codecFormat = V4L2_PIX_FMT_VP8;
    else if (!strcmp(mimeType, "image/jpeg"))
        codecFormat = V4L2_PIX_FMT_MJPEG;
    else {
        ERROR("unsupported mimetype");
        return -1;
    }

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    format.fmt.pix_mp.pixelformat = codecFormat;
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
            break;
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

    x11Window = XCreateSimpleWindow(x11Display, DefaultRootWindow(x11Display)
        , 0, 0, videoWidth, videoHeight, 0, 0
        , WhitePixel(x11Display, 0));
    XMapWindow(x11Display, x11Window);
#if __ENABLE_V4L2_GLX__
    pixmaps.resize(outputQueueCapacity);
    glxPixmaps.resize(outputQueueCapacity);
    textureIds.resize(outputQueueCapacity);

    if (!glxContext) {
        glxContext = glxInit(x11Display, x11Window);
    }
    ASSERT(glxContext);

    glGenTextures(outputQueueCapacity, &textureIds[0] );
    for (i=0; i<outputQueueCapacity; i++) {
        int ret = createPixmapForTexture(glxContext, textureIds[i], videoWidth, videoHeight, &pixmaps[i], &glxPixmaps[i]);
        DEBUG("textureIds[%d]: 0x%x, pixmaps[%d]=0x%lx, glxPixmaps[%d]: 0x%lx", i, textureIds[i], i, pixmaps[i], i, glxPixmaps[i]);
        ASSERT(ret == 0);
        ret = YamiV4L2_UsePixmap(fd, i, pixmaps[i]);
        ASSERT(ret == 0);
    }
#else
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

        if (!eglContext)
            eglContext = eglInit(x11Display, x11Window, 0 /*VA_FOURCC_RGBA*/, memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);

        glGenTextures(outputQueueCapacity, &textureIds[0] );
        for (i=0; i<outputQueueCapacity; i++) {
             int ret = 0;
             ret = YamiV4L2_UseEglImage(fd, eglContext->eglContext.display, eglContext->eglContext.context, i, &eglImages[i]);
             ASSERT(ret == 0);

             GLenum target = GL_TEXTURE_2D;
             if (memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
                 target = GL_TEXTURE_EXTERNAL_OES;
             glBindTexture(target, textureIds[i]);
             glEGLImageTargetTexture2DOES(target, eglImages[i]);

             glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
             glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
             DEBUG("textureIds[%d]: 0x%x, eglImages[%d]: 0x%x", i, textureIds[i], i, eglImages[i]);
        }
    }
#endif

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
            if (stagingBufferInDevice == 0)
                break;
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

    calcFps.fps(renderFrameCount);
    // YamiV4L2_Munmap(void* addr, size_t length)
    possibleWait(input->getMimeType());

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
    ASSERT(glGetError() == GL_NO_ERROR);

#if __ENABLE_V4L2_GLX__
    glxRelease(glxContext, &pixmaps[0], &glxPixmaps[0], pixmaps.size());
#else
    for (i=0; i<eglImages.size(); i++) {
        eglDestroyImageKHR(eglContext->eglContext.display, eglImages[i]);
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
    if (eglContext)
        eglRelease(eglContext);
#endif

    // close device
    ioctlRet = YamiV4L2_Close(fd);
    ASSERT(ioctlRet != -1);

    if(input)
        delete input;

    if (outfp)
        fclose(outfp);

    if (dumpOutputDir)
        free(dumpOutputDir);

    if (x11Display && x11Window)
        XDestroyWindow(x11Display, x11Window);
    if (x11Display)
        XCloseDisplay(x11Display);

    fprintf(stdout, "decode done\n");
}

