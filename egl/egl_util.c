/*
 *  egl_util.c - help utility for egl
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

#include "egl_util.h"
#include "common/log.h"
#if __ENABLE_DMABUF__
#include "libdrm/drm_fourcc.h"
#endif

EGLImageKHR createEglImageFromDrmBuffer(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t drmName, int width, int height, int pitch)
{
    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
    EGLint attribs[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_DRM_BUFFER_STRIDE_MESA, pitch/4,
      EGL_DRM_BUFFER_FORMAT_MESA,
      EGL_DRM_BUFFER_FORMAT_ARGB32_MESA,
      EGL_DRM_BUFFER_USE_MESA,
      EGL_DRM_BUFFER_USE_SHARE_MESA,
      EGL_NONE
    };

    eglImage = eglCreateImageKHR(eglDisplay, eglContext, EGL_DRM_BUFFER_MESA,
                     (EGLClientBuffer)(intptr_t)drmName, attribs);
    return eglImage;
}

EGLImageKHR createEglImageFromDmaBuf(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t dmaBuf, int width, int height, int pitch)
{
    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
#if __ENABLE_DMABUF__
    EGLint attribs[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_RGBX8888,
      EGL_DMA_BUF_PLANE0_FD_EXT, dmaBuf,
      EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
      EGL_DMA_BUF_PLANE0_PITCH_EXT, pitch,
      EGL_NONE
    };

    eglImage = eglCreateImageKHR(eglDisplay, eglContext,
                     EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
    return eglImage;
#else
    ERROR("dma_buf is enabled with --enable-dmabuf option");
    return eglImage;
#endif
}

