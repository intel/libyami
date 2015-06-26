/*
 *  gles2_help.h - utility to set up gles2 drawing context
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
