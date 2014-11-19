/*
 *  v4l2_wrapper.cpp
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

#include "v4l2_wrapper.h"
#include "v4l2_codecbase.h"

#include "common/log.h"
#include "common/lock.h"

#include <map>
typedef std::tr1::shared_ptr < V4l2CodecBase > V4l2CodecPtr;

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
    yamiTraceInit();
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

int32_t YamiV4L2_FrameMemoryType(int32_t fd, VideoDataMemoryType memory_type)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->setFrameMemoryType(memory_type);
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

#if __ENABLE_V4L2_GLX__
int32_t YamiV4L2_SetXDisplay(int32_t fd, Display *x11Display)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
     bool ret = true;

     ASSERT(v4l2Codec);
     ret &= v4l2Codec->setXDisplay(x11Display);

     return ret;
}

int32_t YamiV4L2_UsePixmap(int fd, int bufferIndex, Pixmap pixmap)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
     bool ret = true;

     ASSERT(v4l2Codec);
     ret &= v4l2Codec->usePixmap(bufferIndex, pixmap);

     return ret;
}

int32_t YamiV4L2_Stop(int32_t fd)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    bool ret = true;

    ASSERT(v4l2Codec);
    ret &= v4l2Codec->stop();
    INFO("stop codec(fd:%d) , ret: %d", fd, ret);
    ASSERT(ret);

    return ret ? 0 : -1;
}

#else
int32_t YamiV4L2_UseEglImage(int fd, EGLDisplay eglDisplay, EGLContext eglContext, unsigned int bufferIndex, void* eglImage)
{
    V4l2CodecPtr v4l2Codec = _findCodecFromFd(fd);
    ASSERT(v4l2Codec);
    return v4l2Codec->useEglImage(eglDisplay, eglContext, bufferIndex, eglImage);
}
#endif
