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
#ifndef __GLES2_HELP_H__
#define __GLES2_HELP_H__

#if __ENABLE_X11__
#include <X11/Xlib.h>
#endif
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdint.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include "egl/egl_util.h"

typedef struct {
    EGLDisplay          display;
    EGLConfig           config;
    EGLContext          context;
    EGLSurface          surface;
} EGLContext_t;

typedef struct {
    GLuint  vertexShader;
    GLuint  fragShader;
    GLuint  program;
    GLint   attrPosition;
    GLint   attrTexCoord;
    GLint   uniformTex[3];
    int     texCount;
} GLProgram;

typedef struct {
    EGLContext_t    eglContext;
    GLProgram       *glProgram;
} EGLContextType;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

EGLContextType* eglInit(Display *x11Display, XID window, uint32_t fourcc, int isExternalTexture);
void eglRelease(EGLContextType *context);
GLuint createTextureFromPixmap(EGLContextType *context, XID pixmap);
int drawTextures(EGLContextType *context, GLenum target, GLuint *textureIds, int texCount);
void imageTargetTexture2D(EGLenum, EGLImageKHR);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __GLES2_HELP_H__ */
