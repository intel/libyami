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

#include "v4l2_wrapper.h"
#if defined(__ENABLE_V4L2_OPS__)
#include "v4l2codec_device_ops.h"
#endif
#include "v4l2_codecbase.h"
#include <stdlib.h>
#include "common/log.h"
#include "common/lock.h"

#include <map>
typedef SharedPtr < V4l2CodecBase > V4l2CodecPtr;

/** <pre>
v4l2_wrapper implements a wrapper library for yami encoder/decoder, it translates v4l2 ioctl to yami APIs.
There are four threads in the wrapper library.
Two threads for v4l2 interface: one for device pool (pool thread), one for device operation (device thread: deque, enque etc)
Two threads are internal worker to drive data input (input thread) and output (output thread) respectively.
   - Device thread owns yami encoder before input/output thread launch and after input/output thread exit . encoder->stop() defers to device _close() instead of STREAMOFF ioctl.
   - Dynamic encoder parameter change (bitrate/framerate etc) are accepted in device operation thread, and executed in input thread, with mutex lock
Input thread keeps runing until: no input buffer available (from device enque buffer) or encode() fail (yami/dirver is busy).
Input Thread is woken up by: enqued a new input buffer, output thread run once successfully, or input stream stops.
Output thread keeps runing until: no output buffer available (from device enque buffer) or getOutput() fail (no encoded frame available from yami/driver).
Output Thread is woken up by: enqued a new output buffer, input thread run once successfully, or output stream stops.
Initial buffers status are at client side.
 </pre> */

typedef std::map<int /* fd */, V4l2CodecPtr> CodecPtrFdMap;
static CodecPtrFdMap s_codecMap;
static YamiMediaCodec::Lock s_codecMapLock;

V4l2CodecPtr _findCodecFromFd(int fd)
{
    V4l2CodecPtr v4l2Codec;
    YamiMediaCodec::AutoLock locker(s_codecMapLock);
    CodecPtrFdMap::iterator it = s_codecMap.find(fd);

    if (it != s_codecMap.end()) {
        v4l2Codec = it->second;
        ASSERT(fd == v4l2Codec->fd());
    }

    return v4l2Codec;
}

int32_t YamiV4L2_Open(const char* name, int32_t flags)
{
    V4l2CodecPtr v4l2Codec = V4l2CodecBase::createCodec(name, flags);
    {
        YamiMediaCodec::AutoLock locker(s_codecMapLock);
        s_codecMap[v4l2Codec->fd()] = v4l2Codec;
    }
    INFO("add encoder(fd: %d) to list", v4l2Codec->fd());

#if 0 // if necessary, some pre-sandbox operation goes here
    // used for chrome sandbox
    // preloadDriverHandle = dlopen( preloadDriverName, RTLD_NOW| RTLD_GLOBAL | RTLD_NODELETE); // RTLD_NODELETE??
    // create VaapiDisplay to make sure vaapi driver are loaded before sandbox,  the display can be reused by future request
    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_DRM;
    s_display = YamiMediaCodec::VaapiDisplay::create(nativeDisplay);
#endif
    return v4l2Codec->fd();
}

int32_t YamiV4L2_Close(int32_t fd)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    bool ret = true;

    ASSERT(v4l2Codec);
    ret &= v4l2Codec->close();
    ASSERT(ret);
    {
        YamiMediaCodec::AutoLock locker(s_codecMapLock);
        ret &= s_codecMap.erase(fd);
    }
    INFO("remove encoder(fd:%d) from list, ret: %d", fd, ret);
    ASSERT(ret);

    return ret ? 0 : -1;
}

// FIXME, if chromeos change to SetParameter as well, we can drop this func
int32_t YamiV4L2_FrameMemoryType(int32_t fd, VideoDataMemoryType memory_type)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->setFrameMemoryType(memory_type);
}

int32_t YamiV4L2_SvcT(int32_t fd, bool enable)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->setSvcT(enable);
}

int32_t YamiV4L2_Ioctl(int32_t fd, int command, void* arg)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->ioctl(command, arg);
}

int32_t YamiV4L2_Poll(int32_t fd, bool poll_device, bool* event_pending)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->poll(poll_device, event_pending);
 }

int32_t YamiV4L2_SetDevicePollInterrupt(int32_t fd)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->setDeviceEvent(1);
}

int32_t YamiV4L2_ClearDevicePollInterrupt(int32_t fd)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->clearDeviceEvent(1);
}

void* YamiV4L2_Mmap(void* addr, size_t length,
                      int prot, int flags, int fd, unsigned int offset)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->mmap(addr, length, prot, flags, offset);
}

int32_t YamiV4L2_Munmap(void* addr, size_t length)
{
    return 0;
}

#if defined(__ENABLE_WAYLAND__)
int32_t YamiV4L2_SetWaylandDisplay(int32_t fd, struct wl_display* wlDisplay)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    bool ret = true;

    ASSERT(v4l2Codec);
    ret &= v4l2Codec->setWaylandDisplay(wlDisplay);

    return ret;
}
#else
#if defined(__ENABLE_X11__)
int32_t YamiV4L2_SetXDisplay(int32_t fd, Display *x11Display)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
     bool ret = true;

     ASSERT(v4l2Codec);
     DEBUG("x11display: %p", x11Display);
     ret &= v4l2Codec->setXDisplay(x11Display);

     return ret;
}
#endif
#if defined(__ENABLE_EGL__)
int32_t YamiV4L2_UseEglImage(int fd, /*EGLDisplay*/void* eglDisplay, /*EGLContext*/void* eglContext, unsigned int bufferIndex, void* eglImage)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->useEglImage(eglDisplay, eglContext, bufferIndex, eglImage);
}
#endif

int32_t YamiV4L2_SetDrmFd(int32_t fd, int drm_fd)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
     bool ret = true;

     ASSERT(v4l2Codec);
     ret &= v4l2Codec->setDrmFd(drm_fd);

     return ret;
}
#endif
#if defined(__ENABLE_V4L2_OPS__)
extern "C" int32_t YamiV4L2_SetParameter(int32_t fd, const char* key, const char* value);
int32_t YamiV4L2_SetParameter(int32_t fd, const char* key, const char* value)
{
    int ret = -1;

    if (!key || !value) {
        ERROR("invalue parameter\n");
        return -1;
    }

    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);

    // FIXME, usually, it can be detected by v4l2_requestbuffers.memory
    // however, chrome always set it to MMAP even for egl/texture usage
    if (!strcmp(key, "frame-memory-type")) {
        VideoDataMemoryType memoryType;
        if (!strcmp(value, "raw-data")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_COPY;
        } else if (!strcmp(value, "drm-name")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_DRM_NAME;
        } else if (!strcmp(value, "dma-buf")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
        } else if (!strcmp(value, "surface-id")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_SURFACE_ID;
        } else if (!strcmp(value, "android-buffer-handle")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_ANDROID_BUFFER_HANDLE;
        }
        else if (!strcmp(value, "external-dma-buf")) {
            memoryType = VIDEO_DATA_MEMORY_TYPE_EXTERNAL_DMA_BUF;
        } else {
            ERROR("unknow output frame memory type: %s\n", value);
            return -1;
        }
        ret = v4l2Codec->setFrameMemoryType(memoryType);
#ifdef __ENABLE_WAYLAND__
    }
    else if (!strcmp(key, "wayland-display")) {
        uintptr_t ptr = (uintptr_t)atoll(value);
        struct wl_display* wlDisplay = (struct wl_display*)ptr;
        DEBUG("wlDisplay: %p", wlDisplay);
        ret = v4l2Codec->setWaylandDisplay(wlDisplay);
#elif defined(__ENABLE_X11__)
    } else if (!strcmp(key, "x11-display")) {
        uintptr_t ptr = (uintptr_t)atoll(value);
        Display* x11Display = (Display*)ptr;
        DEBUG("x11Display: %p", x11Display);
        ret = v4l2Codec->setXDisplay(x11Display);
#endif
    } else if(!(strcmp(key, "encode-mode"))) {
        if (!strcmp(value, "svct")) {
            ret = v4l2Codec->setSvcT(true);
        }
    } else {
        ERROR("unsupported parameter key: %s\n", key);
    }

    return ret;
}


bool v4l2codecOperationInit(V4l2CodecOps *opFuncs)
{
    if (!opFuncs)
        return false;

    int isVersionMatch = 0;
    IS_V4L2CODEC_OPS_VERSION_MATCH(opFuncs->mVersion, isVersionMatch);
    if (!isVersionMatch) {
        ERROR("V4l2CodecOps interface version doesn't match\n");
        return false;
    }
    ASSERT(opFuncs->mSize == sizeof(V4l2CodecOps));

    memset(opFuncs->mVendorString, 0, V4L2CODEC_VENDOR_STRING_SIZE);
    strncpy(opFuncs->mVendorString, "yami", V4L2CODEC_VENDOR_STRING_SIZE-1);

#define V4L2_DLSYM_OR_RETURN_ON_ERROR(name) opFuncs->m##name##Func = YamiV4L2_##name

    V4L2_DLSYM_OR_RETURN_ON_ERROR(Open);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(Close);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(Ioctl);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(Poll);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(SetDevicePollInterrupt);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(ClearDevicePollInterrupt);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(Mmap);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(Munmap);
    V4L2_DLSYM_OR_RETURN_ON_ERROR(SetParameter);
#if defined(__ENABLE_EGL__)
    V4L2_DLSYM_OR_RETURN_ON_ERROR(UseEglImage);
#endif
#undef V4L2_DLSYM_OR_RETURN_ON_ERROR

    return true;
}
#endif // __ENABLE_V4L2_OPS__
