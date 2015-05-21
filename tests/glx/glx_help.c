/*
 *  glx_help.c - glx_help.c - utility to set up gles2 drawing context
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

#if __ENABLE_V4L2_GLX__
#include "glx_help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "common/log.h"

static PFNGLXBINDTEXIMAGEEXTPROC glXBindTexImageEXT_func = NULL;
static PFNGLXRELEASETEXIMAGEEXTPROC glXReleaseTexImageEXT_func = NULL;
#define DEFAULT_ROOT_WINDOW(display) RootWindow(display, DefaultScreen(display))
#define ASSERT_GL_ERROR() ASSERT(glGetError() == GL_NO_ERROR)


GLXContextType* glxInit(Display *x11Display, Window x11Window)
{
    GLXContext glxContext = NULL;
    static GLXContextType context;

    if (context.glxContext) {
        ASSERT(context.x11Display == x11Display && context.x11Window == x11Window);
        return &context;
    }

    // get func pointer for TFP extension
    int screen = DefaultScreen(x11Display);
    const char *ext = glXQueryExtensionsString(x11Display, screen);
    if (!strstr(ext, "GLX_EXT_texture_from_pixmap")) {
        fprintf(stderr, "GLX_EXT_texture_from_pixmap not supported.\n");
        return NULL;
    }

    glXBindTexImageEXT_func = (PFNGLXBINDTEXIMAGEEXTPROC)
      glXGetProcAddress((GLubyte *) "glXBindTexImageEXT");
    glXReleaseTexImageEXT_func = (PFNGLXRELEASETEXIMAGEEXTPROC)
      glXGetProcAddress((GLubyte*) "glXReleaseTexImageEXT");

    if (!glXBindTexImageEXT_func || !glXReleaseTexImageEXT_func) {
       fprintf(stderr, "glXGetProcAddress[glXBindTexImageEXT, glXReleaseTexImageEXT] failed!\n");
       return NULL;
    }

    // create GLXContext
    int attribs[] = { GLX_RGBA,
       GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1,
       GLX_DOUBLEBUFFER, None};

    XVisualInfo *visualInfo = glXChooseVisual(x11Display, screen, attribs);
    ASSERT(visualInfo);
    glxContext = glXCreateContext(x11Display, visualInfo, NULL, True);
    XFree(visualInfo);
    ASSERT(glxContext);

    context.glxContext = glxContext;
    context.x11Display = x11Display;
    context.x11Window = x11Window;

    // init gl rendering context
    glXMakeCurrent(x11Display, x11Window, glxContext);

    Window rootWindow;
    int x, y;
    unsigned int windowWidth, windowHeight, windowBorder, windowDepth;
    XGetGeometry(x11Display, x11Window, &rootWindow
        , &x, &y, &windowWidth, &windowHeight, &windowBorder, &windowDepth);
    glViewport(0, 0, windowWidth, windowHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.01, 1.01, -1.01, 1.01, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    ASSERT_GL_ERROR();

    return &context;
}

void glxRelease(GLXContextType *glxContext, Pixmap *pixmaps, GLXPixmap *glxPixmaps, int pixmapCount)
{
    int i;
    for (i=0; i<pixmapCount; i++) {
        glXReleaseTexImageEXT_func(glxContext->x11Display, glxPixmaps[i], GLX_FRONT_LEFT_EXT);
        glXMakeCurrent(glxContext->x11Display, None, NULL);
        glXDestroyPixmap(glxContext->x11Display, glxPixmaps[i]);
        XFreePixmap(glxContext->x11Display, pixmaps[i]);
    }
    glXDestroyContext(glxContext->x11Display, glxContext->glxContext);
    ASSERT_GL_ERROR();
}

int createPixmapForTexture(GLXContextType *glxContext, GLuint texture, uint32_t width, uint32_t height, Pixmap *pixmap, GLXPixmap *glxPixmap)
{
    int i;

    if (!glxContext || !texture || !width || !height || !pixmap || !glxPixmap)
        return -1;

    Display *x11Display = glxContext->x11Display;
    // query/choose config/attributes for GLXPixmap/Pixmap
    const int pixmapAttr[] = { GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
       GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
       GL_NONE};
    const int fbConfigAttr[] = { GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
        GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
        GLX_BIND_TO_TEXTURE_RGB_EXT, GL_TRUE,
        GLX_Y_INVERTED_EXT, GL_TRUE,
        GL_NONE};
    int numFbConfigs;
    GLXFBConfig *fbConfig = glXChooseFBConfig(x11Display, DefaultScreen(x11Display), fbConfigAttr, &numFbConfigs);
    ASSERT(fbConfig && numFbConfigs);

    int depth =  XDefaultDepth(x11Display, DefaultScreen(x11Display));
    *pixmap = XCreatePixmap(x11Display, DEFAULT_ROOT_WINDOW(x11Display), width, height, depth);
    ASSERT(*pixmap);
    *glxPixmap = glXCreatePixmap(x11Display, *fbConfig, *pixmap, pixmapAttr);
    XFree(fbConfig);
    ASSERT(*glxPixmap);
    glBindTexture(GL_TEXTURE_2D, texture);
    glXBindTexImageEXT_func(x11Display, *glxPixmap, GLX_FRONT_LEFT_EXT, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    ASSERT_GL_ERROR();
    return 0;
}

int drawTexture(GLXContextType *glxContext, GLuint texture)
{
    // if there is gl context switch, MakeCurrent as below
    // glXMakeCurrent(x11Display, x11Window, glxContext);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glClearColor(0.1, 0.1, 0.1, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glRotatef(1, 1, 0, 0);
    glRotatef(1, 0, 0, 1);

    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 1.0);
    glVertex2f(-1, -1);
    glTexCoord2d(1.0, 1.0);
    glVertex2f( 1, -1);
    glTexCoord2d(1.0, 0.0);
    glVertex2d(1.0, 1.0);
    glTexCoord2d(0.0, 0.0);
    glVertex2f(-1.0, 1.0);
    glEnd();

    glXSwapBuffers(glxContext->x11Display, glxContext->x11Window);
    if (glGetError() != GL_NO_ERROR)
        return -1;

    return 0;
}

#endif /* __ENABLE_V4L2_GLX__ */
