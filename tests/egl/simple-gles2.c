/*
 *  simple-gles2.c - simple example to draw texture with gles v2
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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

#include "gles2_help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "EGL/eglext.h"

#define ERROR printf
#define INFO printf
#define DEBUG printf
#define EGL_CHECK_RESULT_RET(result, promptStr, ret) do {   \
    if (result != EGL_TRUE) {                               \
        ERROR("%s failed", promptStr);                      \
        return ret;                                          \
    }                                                       \
}while(0)
#define CHECK_HANDLE_RET(handle, invalid, promptStr, ret) do {  \
    if (handle == invalid) {                                    \
        ERROR("%s failed", promptStr);                          \
        return ret;                                              \
    }                                                           \
} while(0)

static GLuint
createTestTexture( )
{
    GLuint textureId;
    // 2x2 Image, 4 bytes per pixel (R, G, B, A)
    GLubyte pixels[4 * 4] =
    {
      255,   0,   0, 255,   // Red
        0, 255,   0, 255,   // Green
        0,   0, 255, 255,   // Blue
      255, 255, 255, 255,   // White
    };

    glGenTextures(1, &textureId );
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureId;
}

static XID
createPixmap(Display *x11Display, Window win )
{
    GC gr_context1, gr_context2;
    XGCValues gr_values;
    int     screen;
    int i, j;

    screen = DefaultScreen(x11Display);
    XID pixmap = XCreatePixmap(x11Display, RootWindow(x11Display, screen), 256, 256, 24);

    gr_values.function =   GXcopy;
    gr_values.plane_mask = AllPlanes;
    gr_values.foreground = BlackPixel(x11Display,screen);
    gr_values.background = WhitePixel(x11Display,screen);
    gr_context1=XCreateGC(x11Display,win,
              GCFunction | GCPlaneMask | GCForeground | GCBackground,
              &gr_values);

    gr_values.function =   GXxor;
    gr_values.foreground = WhitePixel(x11Display,screen);
    gr_values.background = BlackPixel(x11Display,screen);
    gr_context2=XCreateGC(x11Display,win,
               GCFunction | GCPlaneMask | GCForeground | GCBackground,
               &gr_values);
    for (i=0; i<16; i++)
        for (j=0; j<16; j++) {
        if ((i+j)%2)
            XFillRectangle(x11Display, pixmap, gr_context1, i*16, j*16, 16, 16);
        else
            XFillRectangle(x11Display, pixmap, gr_context2, i*16, j*16, 16, 16);
    }

    return pixmap;
}

int main() {
    EGLContextType *context = NULL;
    // X display and window
    Display * x11Display = XOpenDisplay(NULL);
    CHECK_HANDLE_RET(x11Display, NULL, "XOpenDisplay", -1);
    Window x11RootWindow = DefaultRootWindow(x11Display);

    // may remove the attrib
    XSetWindowAttributes x11WindowAttrib;
    x11WindowAttrib.event_mask = ExposureMask | KeyPressMask;

    // create with video size, simplify it
    Window x11Window = XCreateWindow(x11Display, x11RootWindow,
        0, 0, 800, 600, 0, CopyFromParent, InputOutput,
        CopyFromParent, CWEventMask, &x11WindowAttrib);
    XMapWindow(x11Display, x11Window);
    XSync(x11Display, 0);

    context = eglInit(x11Display, x11Window, 0, 0);
    // GLuint textureId = createTestTexture();
    XID pixmap = createPixmap(x11Display, x11Window);
    GLuint textureId = createTextureFromPixmap(context, pixmap);
    drawTextures(context, &textureId, 1);

    EGLBoolean running = EGL_TRUE;
    while (running) {
        XEvent x_event;
        XNextEvent(x11Display, &x_event);
        switch (x_event.type) {
        case Expose:
            glClear(GL_COLOR_BUFFER_BIT
                | GL_DEPTH_BUFFER_BIT
            );
            drawTextures(context, &textureId, 1);
            break;
        case KeyPress:
            running = EGL_FALSE;
            break;
        default:
            break;
        }
    }

    XUnmapWindow(x11Display, x11Window);
    XDestroyWindow(x11Display, x11Window);
    XCloseDisplay(x11Display);
    INFO("exit successfully");
    return 0;
}


