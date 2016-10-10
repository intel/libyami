/*
 * Copyright (c) 2015 Alibaba. All rights reserved.
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
#ifndef v4l2codec_device_ops_h
#define v4l2codec_device_ops_h
#include <linux/videodev2.h>
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
typedef int32_t (*V4l2UseEglImageFunc)(int32_t fd, /*EGLDisplay*/void* egl_display, /*EGLContext*/void* egl_context,
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

