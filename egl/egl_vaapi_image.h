/*
 *  egl_vaapi_image.h - map vaapi image to egl
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin<Guangxin.Xu@intel.com>
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

#ifndef egl_vaapi_image_h
#define egl_vaapi_image_h

#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <va/va.h>

#include "VideoCommonDefs.h"

namespace YamiMediaCodec {

class EglVaapiImage
{
public:
    EglVaapiImage(VADisplay, int width, int height);
    bool init();
    EGLImageKHR createEglImage(EGLDisplay, EGLContext, VideoDataMemoryType);
    bool blt(const VideoFrameRawData& src);
    bool exportFrame(VideoDataMemoryType memoryType, VideoFrameRawData &frame);
    ~EglVaapiImage();
private:
    bool acquireBufferHandle(VideoDataMemoryType);
    VADisplay       m_display;
    VAImageFormat   m_format;
    VAImage         m_image;
    VABufferInfo    m_bufferInfo;

    int             m_width;
    int             m_height;
    bool            m_inited;
    bool            m_acquired; // acquired buffer handle
    VideoFrameRawData   m_frameInfo;

    EGLImageKHR     m_eglImage;
};

}//namespace YamiMediaCodec

#endif
