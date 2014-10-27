/*
 *  egl_util.h - help utility for egl
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

#ifndef __EGL_UTIL_H__
#define __EGL_UTIL_H__
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include "VideoCommonDefs.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

EGLImageKHR createEglImageFromHandle(EGLDisplay eglDisplay, EGLContext eglContext, VideoDataMemoryType type, uint32_t dmaBuf, int width, int height, int pitch);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
