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

#ifndef __EGL_UTIL_H__
#define __EGL_UTIL_H__
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "VideoCommonDefs.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

EGLImageKHR createImage(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer, const EGLint *);
EGLBoolean destroyImage(EGLDisplay, EGLImageKHR);
EGLImageKHR createEglImageFromHandle(EGLDisplay eglDisplay, EGLContext eglContext, VideoDataMemoryType type, uint32_t dmaBuf, int width, int height, int pitch);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
