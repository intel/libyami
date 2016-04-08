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
    bool blt(const SharedPtr<VideoFrame>& src);
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
