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

#include <stdint.h>
#include <stddef.h>
#if ANDROID
#else
    #if __ENABLE_X11__
    #include <X11/Xlib.h>
    #endif
    #include <EGL/egl.h>
#endif
#include "VideoCommonDefs.h"

#ifndef v4l2_wrapper_h
#define v4l2_wrapper_h
#ifndef V4L2_EVENT_RESOLUTION_CHANGE
    #define V4L2_EVENT_RESOLUTION_CHANGE 5
#endif

#ifdef __cplusplus
extern "C" {
#endif
int32_t YamiV4L2_Open(const char* name, int32_t flags);
int32_t YamiV4L2_Close(int32_t fd);
int32_t YamiV4L2_FrameMemoryType(int32_t fd, VideoDataMemoryType memory_type);
int32_t YamiV4L2_Ioctl(int32_t fd, int request, void* arg);
int32_t YamiV4L2_Poll(int32_t fd, bool poll_device, bool* event_pending);
int32_t YamiV4L2_SetDevicePollInterrupt(int32_t fd);
int32_t YamiV4L2_ClearDevicePollInterrupt(int32_t fd);
void* YamiV4L2_Mmap(void* addr, size_t length,
                     int prot, int flags, int fd, unsigned int offset);
int32_t YamiV4L2_Munmap(void* addr, size_t length);
#if ANDROID
#else
    #if __ENABLE_X11__
    /// it should be called before driver initialization (immediate after _Open()).
    int32_t YamiV4L2_SetXDisplay(int32_t fd, Display *x11Display);
    #endif
    int32_t YamiV4L2_UseEglImage(int fd, EGLDisplay eglDisplay, EGLContext eglContext, unsigned int buffer_index, void* egl_image);
    int32_t YamiV4L2_SetDrmFd(int32_t fd, int drm_fd);
#endif
#ifdef __cplusplus
} // extern "C"
#endif

#endif

