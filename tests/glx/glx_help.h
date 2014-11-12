/*
 *  glx_help.h - utility to set up glx drawing context
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
#ifndef __GLX_HELP_H__
#define __GLX_HELP_H__

#if __ENABLE_V4L2_GLX__
#include <X11/Xlib.h>
#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

typedef struct {
    Display    *x11Display;
    Window      x11Window;
    GLXContext  glxContext;
} GLXContextType;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

GLXContextType* glxInit(Display *x11Display, Window x11Window);
void glxRelease(GLXContextType *glxContext, Pixmap *pixmaps, GLXPixmap *glxPixmaps, int pixmapCount);
int createPixmapForTexture(GLXContextType *glxContext, GLuint texture, uint32_t width, uint32_t height, Pixmap *pixmap, GLXPixmap *glxPixmap);
int drawTexture(GLXContextType *glxContext, GLuint texture);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __ENABLE_V4L2_GLX__ */
#endif /* __GLES2_HELP_H__ */
