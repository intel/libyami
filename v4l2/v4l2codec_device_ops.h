/*********************************************************************
 * Copyright (c) 2015 Alibaba copyright
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Alibaba Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *   Created on: 2015-11-26
 *       Author:aihua.zah@alibaba-inc.com
 */
#ifndef v4l2codec_device_ops_h
#define v4l2codec_device_ops_h
#include <linux/videodev2.h>
#include <EGL/egl.h>
#include <stdint.h>

#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif
#ifndef V4L2_MEMORY_DMABUF
    #define V4L2_MEMORY_DMABUF      4
#endif
#ifndef V4L2_MEMORY_ANDROID_BUFFER_HANDLE
    #define V4L2_MEMORY_ANDROID_BUFFER_HANDLE   (V4L2_MEMORY_DMABUF+1)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// func pointer of v4l2 device
typedef int32_t (*V4l2OpenFunc)(const char* name, int32_t flags);
typedef int32_t (*V4l2CloseFunc)(int32_t fd);
typedef int32_t (*V4l2IoctlFunc)(int32_t fd, int32_t cmd, void* arg);
typedef int32_t (*V4l2PollFunc)(int32_t fd, bool poll_device, bool* event_pending);
typedef int32_t (*V4l2SetDevicePollInterruptFunc)(int32_t fd);
typedef int32_t (*V4l2ClearDevicePollInterruptFunc)(int32_t fd);
typedef void*   (*V4l2MmapFunc)(void* addr, size_t length, int32_t prot,
                  int32_t flags, int32_t fd, unsigned int offset);
typedef int32_t (*V4l2MunmapFunc)(void* addr, size_t length);
typedef int32_t (*V4l2SetParameterFunc)(int32_t fd, const char* key, const char* value);
typedef int32_t (*V4l2UseEglImageFunc)(int32_t fd, EGLDisplay egl_display, EGLContext egl_context,
                  uint32_t buffer_index, void* egl_image);

#define V4L2CODEC_VENDOR_STRING_SIZE  16
#define V4L2CODEC_VERSION_MAJOR        0
#define V4L2CODEC_VERSION_MINOR        1
#define V4L2CODEC_VERSION_REVISION     0
#define V4L2CODEC_VERSION_STEP         0

typedef union V4l2CodecVersion{
    struct {
        uint8_t mMajor;
        uint8_t mMinor;
        uint8_t mRevision;
        uint8_t mStep;
    } mDetail;
    uint32_t mVersion;
} V4l2CodecVersion;

typedef struct V4l2CodecOps {
    uint32_t mSize;
    V4l2CodecVersion mVersion;
    char mVendorString[V4L2CODEC_VENDOR_STRING_SIZE];   // for example yami-0.4.0
    V4l2OpenFunc mOpenFunc;
    V4l2CloseFunc mCloseFunc;
    V4l2IoctlFunc mIoctlFunc;
    V4l2PollFunc mPollFunc;
    V4l2SetDevicePollInterruptFunc mSetDevicePollInterruptFunc;
    V4l2ClearDevicePollInterruptFunc mClearDevicePollInterruptFunc;
    V4l2MmapFunc mMmapFunc;
    V4l2MunmapFunc mMunmapFunc;
    V4l2SetParameterFunc mSetParameterFunc;
    V4l2UseEglImageFunc mUseEglImageFunc;
} V4l2CodecOps;

#define INIT_V4L2CODEC_OPS_VERSION(version) do {                \
    (version).mDetail.mMajor = V4L2CODEC_VERSION_MAJOR;         \
    (version).mDetail.mMinor = V4L2CODEC_VERSION_MINOR;         \
    (version).mDetail.mRevision = V4L2CODEC_VERSION_REVISION;   \
    (version).mDetail.mStep = V4L2CODEC_VERSION_STEP;           \
} while(0)

// do not cast ops to V4l2CodecOps*; assume caller use it correctly (it will not pass compile if not)
#define INIT_V4L2CODEC_OPS_SIZE_VERSION(ops) do {               \
    if((ops) == NULL) break;                                    \
    memset(ops, 0, sizeof(V4l2CodecOps));                       \
    (ops)->mSize = sizeof(V4l2CodecOps);                        \
    INIT_V4L2CODEC_OPS_VERSION((ops)->mVersion);                \
} while(0)

#define IS_V4L2CODEC_OPS_VERSION_MATCH(version, isMatch) do {   \
    V4l2CodecVersion v;                                         \
    INIT_V4L2CODEC_OPS_VERSION(v);                              \
    if (version.mVersion == v.mVersion) {                       \
        isMatch = 1;                                            \
    } else {                                                    \
        isMatch = 0;                                            \
    }                                                           \
} while(0)

typedef bool (*V4l2codecOperationInitFunc)(struct V4l2CodecOps *OpFuncs);
// fill all the func ptrs implemented by platform
bool v4l2codecOperationInit(V4l2CodecOps *OpFuncs);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // v4l2codec_device_ops_h

