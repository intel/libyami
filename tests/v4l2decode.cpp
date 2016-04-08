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
#include <errno.h>
#include  <sys/mman.h>
#include <vector>
#include <stdint.h>
#include <inttypes.h>

#include "common/log.h"
#include "common/utils.h"
#include "decodeinput.h"
#include "decodehelp.h"
#if ANDROID
#include <gui/SurfaceComposerClient.h>
#include <va/va_android.h>
#else
#include "./egl/gles2_help.h"
#endif
#include "interface/VideoCommonDefs.h"

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif

#if __ENABLE_V4L2_OPS__
#include "v4l2/v4l2codec_device_ops.h"
#include <dlfcn.h>
#include <fcntl.h>
#else
#include "v4l2/v4l2_wrapper.h"
#endif

#ifdef ANDROID

#ifndef CHECK_EQ
#define CHECK_EQ(a, b) assert((a) == (b))
#endif

sp<SurfaceComposerClient> mClient;
sp<SurfaceControl> mSurfaceCtl;
sp<Surface> mSurface;
sp<ANativeWindow> mNativeWindow;

std::vector <ANativeWindowBuffer*> mWindBuff;

bool createNativeWindow(__u32 pixelformat)
{
    mClient = new SurfaceComposerClient();
    mSurfaceCtl = mClient->createSurface(String8("testsurface"),
                                      800, 600, pixelformat, 0);

    // configure surface
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceCtl->setLayer(100000);
    mSurfaceCtl->setPosition(100, 100);
    mSurfaceCtl->setSize(800, 600);
    SurfaceComposerClient::closeGlobalTransaction();

    mSurface = mSurfaceCtl->getSurface();
    mNativeWindow = mSurface;

    int bufWidth = 640;
    int bufHeight = 480;
    CHECK_EQ(0,
             native_window_set_usage(
             mNativeWindow.get(),
             GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
             | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

    CHECK_EQ(0,
             native_window_set_scaling_mode(
             mNativeWindow.get(),
             NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

    CHECK_EQ(0, native_window_set_buffers_geometry(
                mNativeWindow.get(),
                bufWidth,
                bufHeight,
                pixelformat));

    return true;
}
#endif

#if __ENABLE_V4L2_OPS__
static struct V4l2CodecOps s_v4l2CodecOps;
static int32_t s_v4l2Fd = 0;

static bool loadV4l2CodecDevice(const char* libName )
{
    memset(&s_v4l2CodecOps, 0, sizeof(s_v4l2CodecOps));
    s_v4l2Fd = 0;

#ifndef ANDROID
    if (!dlopen(libName, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE)) {
#else
    if (!dlopen(libName, RTLD_NOW | RTLD_GLOBAL)) {
#endif
      ERROR("Failed to load %s\n", libName);
      return false;
    }

    V4l2codecOperationInitFunc initFunc = NULL;
    initFunc = (V4l2codecOperationInitFunc)dlsym(RTLD_DEFAULT, "v4l2codecOperationInit");

    if (!initFunc) {
        ERROR("fail to dlsym v4l2codecOperationInit\n");
        return false;
    }

    INIT_V4L2CODEC_OPS_SIZE_VERSION(&s_v4l2CodecOps);
    if (!initFunc(&s_v4l2CodecOps)) {
        ERROR("fail to init v4l2 device operation func pointers\n");
        return false;
    }

    int isVersionMatch = 0;
    IS_V4L2CODEC_OPS_VERSION_MATCH(s_v4l2CodecOps.mVersion, isVersionMatch);
    if (!isVersionMatch) {
        ERROR("V4l2CodecOps interface version doesn't match\n");
        return false;
    }
    if(s_v4l2CodecOps.mSize != sizeof(V4l2CodecOps)) {
        ERROR("V4l2CodecOps interface data structure size doesn't match\n");
        return false;
    }

    return true;
}
#define SIMULATE_V4L2_OP(OP)  s_v4l2CodecOps.m##OP##Func
#else
#define SIMULATE_V4L2_OP(OP)  YamiV4L2_##OP
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
uint32_t k_extraOutputFrameCount = 2;
static std::vector<uint8_t*> inputFrames;
static std::vector<struct RawFrameData> rawOutputFrames;

static VideoDataMemoryType memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
static const char* typeStrDrmName = "drm-name";
static const char* typeStrDmaBuf = "dma-buf";
static const char* typeStrRawData = "raw-data";
// static const char* typeStrAndroidNativeBuffer = "android-native-buffer";
static const char* memoryTypeStr = typeStrDrmName;
#define IS_DRM_NAME()   (!strcmp(memoryTypeStr, typeStrDrmName))
#define IS_DMA_BUF()   (!strcmp(memoryTypeStr, typeStrDmaBuf))
#define IS_RAW_DATA()   (!strcmp(memoryTypeStr, typeStrRawData))
// #define IS_ANDROID_NATIVE_BUFFER()   (!strcmp(memoryTypeStr, typeStrAndroidNativeBuffer))

static FILE* outfp = NULL;
#ifndef ANDROID
static Display * x11Display = NULL;
static Window x11Window = 0;
#endif
#ifndef ANDROID
static EGLContextType *eglContext = NULL;
static std::vector<EGLImageKHR> eglImages;
#endif
#ifndef ANDROID
static std::vector<GLuint> textureIds;
#endif
static bool isReadEOS=false;
static int32_t stagingBufferInDevice = 0;
static uint32_t renderFrameCount = 0;

static DecodeParameter params;

bool feedOneInputFrame(DecodeInput * input, int fd, int index = -1 /* if index is not -1, simple enque it*/)
{

    VideoDecodeBuffer inputBuffer;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[k_inputPlaneCount];
    int ioctlRet = -1;

    memset(&buf, 0, sizeof(buf));
    memset(&planes, 0, sizeof(planes));
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE; // it indicates input buffer(raw frame) type
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.planes = planes;
    buf.length = k_inputPlaneCount;

    if (index == -1) {
        ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_DQBUF, &buf);
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
        buf.flags = inputBuffer.flag;
    }

    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);

    stagingBufferInDevice ++;
    return true;
}

bool dumpOneVideoFrame(int32_t index)
{
    uint32_t row;

    if (!outfp) {
        char outFileName[256];
        char* baseFileName = params.inputFile;
        char* s = strrchr(params.inputFile, '/');
        if (s)
            baseFileName = s+1;
        // V4L2 reports fourcc as NM12 (planar NV12), use hard code here
        sprintf(outFileName, "%s/%s_%dx%d.NV12", params.outputFile.c_str(), baseFileName, rawOutputFrames[index].width, rawOutputFrames[index].height);
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

#ifdef ANDROID
static bool displayOneVideoFrameAndroid(int32_t fd, int32_t index)
{
    int32_t ioctlRet = -1;
    struct v4l2_buffer buffer;
    memset(&buffer, 0, sizeof(buffer));

    if (mNativeWindow->queueBuffer(mNativeWindow.get(), mWindBuff[index], -1) != 0) {
        fprintf(stderr, "queue buffer to native window failed\n");
        return false;
    }

    ANativeWindowBuffer* pbuf = NULL;
    status_t err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &pbuf);
    if (err != 0) {
        fprintf(stderr, "dequeueBuffer failed: %s (%d)\n", strerror(-err), -err);
        return false;
    }

    buffer.m.userptr = (unsigned long)pbuf;
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    uint32_t i;
    for (i = 0; i < mWindBuff.size(); i++) {
        if (pbuf == mWindBuff[i]) {
            buffer.index = i;
            break;
        }
    }
    if (i == mWindBuff.size())
        return false;

    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QBUF, &buffer);
    ASSERT(ioctlRet != -1);

    return true;
}
#else
bool displayOneVideoFrameEGL(int32_t fd, int32_t index)
{
    ASSERT(eglContext && textureIds.size());
    ASSERT(index>=0 && (uint32_t)index<textureIds.size());
    DEBUG("textureIds[%d] = 0x%x", index, textureIds[index]);

    GLenum target = GL_TEXTURE_2D;
    if (IS_DMA_BUF())
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
        ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_DQBUF, &buf);
        if (ioctlRet == -1)
            return false;

        renderFrameCount++;
#ifdef ANDROID
        ret = displayOneVideoFrameAndroid(fd, buf.index);
#else
        if (IS_DMA_BUF() || IS_DRM_NAME())
            ret = displayOneVideoFrameEGL(fd, buf.index);
        else
            ret = dumpOneVideoFrame(buf.index);
#endif
        // ASSERT(ret);
        if (!ret) {
            ERROR("display frame failed");
        }
    } else {
        buf.index = index;
    }

#ifndef ANDROID
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QBUF, &buf);
    ASSERT(ioctlRet != -1);
#endif
    INFO("renderFrameCount: %d", renderFrameCount);
    return true;
}

bool handleResolutionChange(int32_t fd)
{
    bool resolutionChanged = false;
    // check resolution change
    struct v4l2_event ev;
    memset(&ev, 0, sizeof(ev));

    while (SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_DQEVENT, &ev) == 0) {
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
    if (SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_G_FMT, &format) == -1) {
        return false;
    }

    // resolution and pixelformat got here
    outputPlaneCount = format.fmt.pix_mp.num_planes;
    ASSERT(outputPlaneCount == 2);
    videoWidth = format.fmt.pix_mp.width;
    videoHeight = format.fmt.pix_mp.height;

    return true;
}

extern uint32_t v4l2PixelFormatFromMime(const char* mime);

int main(int argc, char** argv)
{
    DecodeInput *input;
    int32_t fd = -1;
    uint32_t i = 0;
    int32_t ioctlRet = -1;
    YamiMediaCodec::CalcFps calcFps;

    yamiTraceInit();
#if __ENABLE_X11__
    XInitThreads();
#endif

#if __ENABLE_V4L2_OPS__
    // FIXME, use libv4l2codec_hw.so instead
    if (!loadV4l2CodecDevice("libyami_v4l2.so")) {
        ERROR("fail to init v4l2codec device with __ENABLE_V4L2_OPS__\n");
        return -1;
    }
#endif

    if (!processCmdLine(argc, argv, &params))
        return -1;

    switch (params.renderMode) {
    case 0:
        memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_COPY;
        memoryTypeStr = typeStrRawData;
    break;
    case 3:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
        memoryTypeStr = typeStrDrmName;
    break;
    case 4:
        memoryType = VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
        memoryTypeStr = typeStrDmaBuf;
    break;
    default:
        ASSERT(0 && "unsupported render mode, -m [0,3,4] are supported");
    break;
    }

    input = DecodeInput::create(params.inputFile);
    if (input==NULL) {
        ERROR("fail to init input stream\n");
        return -1;
    }

    renderFrameCount = 0;
    calcFps.setAnchor();
    // open device
    fd = SIMULATE_V4L2_OP(Open)("decoder", 0);
    ASSERT(fd!=-1);

#if __ENABLE_X11__
    x11Display = XOpenDisplay(NULL);
    ASSERT(x11Display);
    DEBUG("x11display: %p", x11Display);
    #if __ENABLE_V4L2_OPS__
    char displayStr[32];
    sprintf(displayStr, "%" PRIu64 "", (uint64_t)x11Display);
    ioctlRet = SIMULATE_V4L2_OP(SetParameter)(fd, "x11-display", displayStr);
    #else
    ioctlRet = SIMULATE_V4L2_OP(SetXDisplay)(fd, x11Display);
    #endif
#endif
    // set output frame memory type
#if __ENABLE_V4L2_OPS__
    SIMULATE_V4L2_OP(SetParameter)(fd, "frame-memory-type", memoryTypeStr);
#else
    SIMULATE_V4L2_OP(FrameMemoryType)(fd, memoryType);
#endif

    // query hw capability
    struct v4l2_capability caps;
    memset(&caps, 0, sizeof(caps));
    caps.capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_VIDEO_OUTPUT_MPLANE | V4L2_CAP_STREAMING;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QUERYCAP, &caps);
    ASSERT(ioctlRet != -1);

    // set input/output data format
    uint32_t codecFormat = v4l2PixelFormatFromMime(input->getMimeType());
    if (!codecFormat) {
        ERROR("unsupported mimetype, %s", input->getMimeType());
        return -1;
    }

    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    format.fmt.pix_mp.pixelformat = codecFormat;
    format.fmt.pix_mp.num_planes = 1;
    format.fmt.pix_mp.plane_fmt[0].sizeimage = k_maxInputBufferSize;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    // set preferred output format
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    uint8_t* data = (uint8_t*)input->getCodecData().data();
    uint32_t size = input->getCodecData().size();
    //save codecdata, size+data, the type of format.fmt.raw_data is __u8[200]
    //we must make sure enough space (>=sizeof(uint32_t) + size) to store codecdata
    memcpy(format.fmt.raw_data, &size, sizeof(uint32_t));
    if(sizeof(format.fmt.raw_data) >= size + sizeof(uint32_t))
        memcpy(format.fmt.raw_data + sizeof(uint32_t), data, size);
    else {
        ERROR("No enough space to store codec data");
        return -1;
    }
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_S_FMT, &format);
    ASSERT(ioctlRet != -1);

    // input port starts as early as possible to decide output frame format
    __u32 type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);

    // setup input buffers
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 2;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_REQBUFS, &reqbufs);
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
        ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QUERYBUF, &buffer);
        ASSERT(ioctlRet != -1);

        // length and mem_offset should be filled by VIDIOC_QUERYBUF above
        void* address = SIMULATE_V4L2_OP(Mmap)(NULL,
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
        if (SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_G_FMT, &format) != 0) {
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

#ifdef ANDROID
    __u32 pixelformat = format.fmt.pix_mp.pixelformat;
    if (!createNativeWindow(pixelformat)) {
        fprintf(stderr, "create native window error\n");
        return -1;
    }

    int minUndequeuedBuffs = 0;
    status_t err = mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeuedBuffs);
    if (err != 0) {
        fprintf(stderr, "query native window min undequeued buffers error\n");
        return err;
    }
#endif

    // setup output buffers
    // Number of output buffers we need.
    struct v4l2_control ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.id = V4L2_CID_MIN_BUFFERS_FOR_CAPTURE;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_G_CTRL, &ctrl);
#ifndef ANDROID
    uint32_t minOutputFrameCount = ctrl.value + k_extraOutputFrameCount;
#else
    uint32_t minOutputFrameCount = ctrl.value + k_extraOutputFrameCount + minUndequeuedBuffs;
#endif

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = minOutputFrameCount;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);
    ASSERT(reqbufs.count>0);
    outputQueueCapacity = reqbufs.count;

#if __ENABLE_X11__
    if (!IS_RAW_DATA()) {
        x11Window = XCreateSimpleWindow(x11Display, DefaultRootWindow(x11Display)
            , 0, 0, videoWidth, videoHeight, 0, 0
            , WhitePixel(x11Display, 0));
        XMapWindow(x11Display, x11Window);
        textureIds.resize(outputQueueCapacity);
    }
#endif

#ifndef ANDROID
    if (IS_RAW_DATA()) {
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
            ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QUERYBUF, &buffer);
            ASSERT(ioctlRet != -1);

            rawOutputFrames[i].width = format.fmt.pix_mp.width;
            rawOutputFrames[i].height = format.fmt.pix_mp.height;
            rawOutputFrames[i].fourcc = format.fmt.pix_mp.pixelformat;

            for (int j=0; j<outputPlaneCount; j++) {
                // length and mem_offset are filled by VIDIOC_QUERYBUF above
                void* address = SIMULATE_V4L2_OP(Mmap)(NULL,
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
    } else if (IS_DMA_BUF() || IS_DRM_NAME()) {
        // setup all textures and eglImages
        eglImages.resize(outputQueueCapacity);

        if (!eglContext)
            eglContext = eglInit(x11Display, x11Window, 0 /*VA_FOURCC_RGBA*/, IS_DMA_BUF());

        glGenTextures(outputQueueCapacity, &textureIds[0] );
        for (i=0; i<outputQueueCapacity; i++) {
             int ret = 0;
             ret = SIMULATE_V4L2_OP(UseEglImage)(fd, eglContext->eglContext.display, eglContext->eglContext.context, i, &eglImages[i]);
             ASSERT(ret == 0);

             GLenum target = GL_TEXTURE_2D;
             if (IS_DMA_BUF())
                 target = GL_TEXTURE_EXTERNAL_OES;
             glBindTexture(target, textureIds[i]);
             imageTargetTexture2D(target, eglImages[i]);

             glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
             glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
             DEBUG("textureIds[%d]: 0x%x, eglImages[%d]: 0x%p", i, textureIds[i], i, eglImages[i]);
        }
    }
#endif

#ifndef ANDROID
    // feed output frames first
    for (i=0; i<outputQueueCapacity; i++) {
        if (!takeOneOutputFrame(fd, i)) {
            ASSERT(0);
        }
    }
#else
    struct v4l2_buffer buffer;

    err = native_window_set_buffer_count(mNativeWindow.get(), outputQueueCapacity);
    if (err != 0) {
        fprintf(stderr, "native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);
        return -1;
    }

    //queue buffs
    for (uint32_t i = 0; i < outputQueueCapacity; i++) {
        ANativeWindowBuffer* pbuf = NULL;
        memset(&buffer, 0, sizeof(buffer));

        err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &pbuf);
        if (err != 0) {
            fprintf(stderr, "dequeueBuffer failed: %s (%d)\n", strerror(-err), -err);
            return -1;
        }

        buffer.m.userptr = (unsigned long)pbuf;
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buffer.index = i;

        ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_QBUF, &buffer);
        ASSERT(ioctlRet != -1);
        mWindBuff.push_back(pbuf);
    }

    for (uint32_t i = 0; i < minUndequeuedBuffs; i++) {
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

        ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_DQBUF, &buffer);
        ASSERT(ioctlRet != -1);

        err = mNativeWindow->cancelBuffer(mNativeWindow.get(), mWindBuff[buffer.index], -1);
        if (err) {
            fprintf(stderr, "queue empty window buffer error\n");
            return -1;
        }
    }
#endif

    // output port starts as late as possible to adopt user provide output buffer
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_STREAMON, &type);
    ASSERT(ioctlRet != -1);

    bool event_pending=true; // try to get video resolution.
    uint32_t dqCountAfterEOS = 0;
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
    } while (SIMULATE_V4L2_OP(Poll)(fd, true, &event_pending) == 0);

    // drain output buffer
    int retry = 3;
    while (takeOneOutputFrame(fd) || (--retry)>0) { // output drain
        usleep(10000);
    }

    calcFps.fps(renderFrameCount);
    // SIMULATE_V4L2_OP(Munmap)(void* addr, size_t length)
    possibleWait(input->getMimeType(), &params);

    // release queued input/output buffer
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 0;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    reqbufs.count = 0;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_REQBUFS, &reqbufs);
    ASSERT(ioctlRet != -1);

    // stop input port
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);

    // stop output port
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    ioctlRet = SIMULATE_V4L2_OP(Ioctl)(fd, VIDIOC_STREAMOFF, &type);
    ASSERT(ioctlRet != -1);

#ifndef ANDROID
    if(textureIds.size())
        glDeleteTextures(textureIds.size(), &textureIds[0]);
    ASSERT(glGetError() == GL_NO_ERROR);
#endif

#ifdef ANDROID
    //TODO, some resources need to destroy?
#else
    for (i=0; i<eglImages.size(); i++) {
        destroyImage(eglContext->eglContext.display, eglImages[i]);
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
    ioctlRet = SIMULATE_V4L2_OP(Close)(fd);
    ASSERT(ioctlRet != -1);

    if(input)
        delete input;

    if (outfp)
        fclose(outfp);

#if __ENABLE_X11__
    if (x11Display && x11Window)
        XDestroyWindow(x11Display, x11Window);
    if (x11Display)
        XCloseDisplay(x11Display);
#endif

    fprintf(stdout, "decode done\n");
}

