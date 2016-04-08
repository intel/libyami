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

#include "egl_util.h"
#include "common/log.h"
#if __ENABLE_DMABUF__
#include "libdrm/drm_fourcc.h"
#endif

static PFNEGLCREATEIMAGEKHRPROC createImageProc = NULL;
static PFNEGLDESTROYIMAGEKHRPROC destroyImageProc = NULL;

EGLImageKHR createImage(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                        EGLClientBuffer buffer, const EGLint *attrib_list)
{
    if (!createImageProc) {
        createImageProc = (void *) eglGetProcAddress("eglCreateImageKHR");
    }
    return createImageProc(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean destroyImage(EGLDisplay dpy, EGLImageKHR image)
{
    if (!destroyImageProc) {
        destroyImageProc = (void *) eglGetProcAddress("eglDestroyImageKHR");
    }
    return destroyImageProc(dpy, image);
}

static EGLImageKHR createEglImageFromDrmBuffer(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t drmName, int width, int height, int pitch)
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

    eglImage = createImage(eglDisplay, eglContext, EGL_DRM_BUFFER_MESA,
                     (EGLClientBuffer)(intptr_t)drmName, attribs);
    return eglImage;
}

static EGLImageKHR createEglImageFromDmaBuf(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t dmaBuf, int width, int height, int pitch)
{
    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
#if __ENABLE_DMABUF__
    EGLint attribs[] = {
      EGL_WIDTH, width,
      EGL_HEIGHT, height,
      EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_XRGB8888,
      EGL_DMA_BUF_PLANE0_FD_EXT, dmaBuf,
      EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
      EGL_DMA_BUF_PLANE0_PITCH_EXT, pitch,
      EGL_NONE
    };

    eglImage = createImage(eglDisplay, EGL_NO_CONTEXT,
                     EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
    ASSERT(eglImage != EGL_NO_IMAGE_KHR);
    return eglImage;
#else
    ERROR("dma_buf is enabled with --enable-dmabuf option");
    return eglImage;
#endif
}

EGLImageKHR createEglImageFromHandle(EGLDisplay eglDisplay, EGLContext eglContext, VideoDataMemoryType type, uint32_t handle, int width, int height, int pitch)
{
    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
    if (type == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
        eglImage = createEglImageFromDrmBuffer(eglDisplay, eglContext, handle, width, height, pitch);
    else if (type == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        eglImage = createEglImageFromDmaBuf(eglDisplay, eglContext, handle, width, height, pitch);

    return eglImage;
}
