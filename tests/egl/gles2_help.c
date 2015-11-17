/*
 *  gles2_help.c - gles2_help.c - utility to set up gles2 drawing context
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

#include "gles2_help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef USE_INTERNAL_LOG
#include "common/log.h"
#else
#define ERROR printf
#define INFO printf
#define DEBUG printf
#define ASSERT assert
#endif

#define EGL_CHECK_RESULT_RET(result, promptStr, ret) do {   \
    if (result != EGL_TRUE) {                               \
        ERROR("%s failed", promptStr);                                         \
        goto error;                                          \
    }                                                       \
}while(0)
#define CHECK_HANDLE_RET(handle, invalid, promptStr, ret) do {  \
    if (handle == invalid) {                                    \
        ERROR("%s failed", promptStr);                                                   \
        goto error;                                              \
    }                                                           \
} while(0)

static const char *fragShaderText_rgba =
  "precision mediump float;\n"
  "uniform sampler2D tex0;\n"
  "varying vec2 v_texcoord;\n"
  "void main() {\n"
  "   gl_FragColor = texture2D(tex0, v_texcoord);\n"
  "   gl_FragColor.a = 1.0;\n"
  "}\n";
static const char *fragShaderText_rgba_ext =
  "#extension GL_OES_EGL_image_external : require\n"
  "precision mediump float;\n"
  "uniform samplerExternalOES tex0;\n"
  "varying vec2 v_texcoord;\n"
  "void main() {\n"
  "   gl_FragColor = texture2D(tex0, v_texcoord);\n"
  "   gl_FragColor.a = 1.0;\n"
  "}\n";

static const char *vertexShaderText_rgba =
  "attribute vec4 pos;\n"
  "attribute vec2 texcoord;\n"
  "varying vec2 v_texcoord;\n"
  "void main() {\n"
  "   gl_Position = pos;\n"
  "   v_texcoord  = texcoord;\n"
  "}\n";

static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2DProc = NULL;

void imageTargetTexture2D(EGLenum target, EGLImageKHR image)
{
    if (!imageTargetTexture2DProc) {
        imageTargetTexture2DProc =
            (void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    }
    imageTargetTexture2DProc(target, image);
}

GLuint
createTextureFromPixmap(EGLContextType *context, XID pixmap)
{
    GLuint textureId;

    EGLImageKHR image = createImage(context->eglContext.display, context->eglContext.context, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmap, NULL);
    glGenTextures(1, &textureId );
    glBindTexture(GL_TEXTURE_2D, textureId);
    imageTargetTexture2D(GL_TEXTURE_2D, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureId;
}

static GLProgram*
createShaders(const char *vertexShaderText, const char *fragShaderText, int texCount)
{
    GLProgram *glProgram = NULL;
    GLint stat;
    #define BUFFER_SIZE 256
    char log[BUFFER_SIZE];
    GLsizei logSize;

    glProgram = calloc(1, sizeof(GLProgram));
    if (!glProgram)
        return NULL;

    glProgram->vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(glProgram->vertexShader, 1, (const char **) &vertexShaderText, NULL);
    glCompileShader(glProgram->vertexShader);
    glGetShaderiv(glProgram->vertexShader, GL_COMPILE_STATUS, &stat);
    if (!stat) {
        glGetShaderInfoLog(glProgram->vertexShader, BUFFER_SIZE, &logSize, log);
        ERROR(" vertex shader fail to compile!: %s", log);
        free(glProgram);
        return NULL;
    }

    glProgram->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(glProgram->fragShader, 1, (const char **) &fragShaderText, NULL);
    glCompileShader(glProgram->fragShader);
    glGetShaderiv(glProgram->fragShader, GL_COMPILE_STATUS, &stat);
    if (!stat) {
        glGetShaderInfoLog(glProgram->fragShader, BUFFER_SIZE, &logSize, log);
        ERROR(" fragment shader fail to compile!: %s", log);
        free(glProgram);
        return NULL;
    }

    glProgram->program = glCreateProgram();
    glAttachShader(glProgram->program, glProgram->fragShader);
    glAttachShader(glProgram->program, glProgram->vertexShader);
    glLinkProgram(glProgram->program);

    glGetProgramiv(glProgram->program, GL_LINK_STATUS, &stat);
    if (!stat) {
       glGetProgramInfoLog(glProgram->program, BUFFER_SIZE, &logSize, log);
       printf("Shader fail to link!: %s\n", log);
       free(glProgram);
       return NULL;
    }

    glUseProgram(glProgram->program);

    glProgram->texCount = texCount;
    glProgram->attrPosition = glGetAttribLocation(glProgram->program, "pos");
    glProgram->attrTexCoord = glGetAttribLocation(glProgram->program, "texcoord");
    glProgram->uniformTex[0] = glGetUniformLocation(glProgram->program, "tex0");
    // glProgram->uniformTex[1] = glGetUniformLocation(glProgram->program, "tex1");
    // glProgram->uniformTex[2] = glGetUniformLocation(glProgram->program, "tex2");

    INFO("Attrib pos at %d\n", glProgram->attrPosition);
    INFO("Attrib texcoord at %d\n", glProgram->attrTexCoord);
    INFO("texture (0, %d), (1, %d), (2, %d) \n", glProgram->uniformTex[0], glProgram->uniformTex[1], glProgram->uniformTex[2]);
    return glProgram;
}

static void
releaseShader(GLProgram *program)
{
    if (!program)
        return;

    if (program->program)
        glDeleteProgram(program->program);
    if (program->fragShader)
        glDeleteShader(program->fragShader);
    if (program->vertexShader)
        glDeleteShader(program->vertexShader);
    free(program);
}

#define MAX_RECT_SIZE 100
#define MIN_RECT_SIZE 10

int
drawTextures(EGLContextType *context, GLenum target, GLuint *textureIds, int texCount)
{
    unsigned int i;
    static int rectSize1 = MAX_RECT_SIZE;
    static int rectSize2 = MIN_RECT_SIZE;
    static int direction = 1;
    const GLfloat positions[4][2] = {
        { -rectSize1/100.f, -rectSize1/100.f},
        {  rectSize2/100.f, -rectSize2/100.f},
        {  rectSize1/100.f,  rectSize1/100.f},
        { -rectSize2/100.f,  rectSize2/100.f}
    };
    static const GLfloat texcoords[4][2] = {
        {  0.0f,  1.0f },
        {  1.0f,  1.0f },
        {  1.0f,  0.0f },
        {  0.0f,  0.0f }
    };

    if (direction) {
        rectSize1--;
        rectSize2++;
    } else {
        rectSize1++;
        rectSize2--;
    }

    if (rectSize1 < 10 || rectSize2 > 100) {
        rectSize1 = 10;
        rectSize2 = 100;
        direction = 0;
    }
    if (rectSize1 > 100 || rectSize2 < 10) {
        rectSize1 = 100;
        rectSize2 = 10;
        direction = 1;
    }


    if (!context)
        return -1;
    GLProgram *glProgram = context->glProgram;

    ASSERT(texCount == glProgram->texCount);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(glProgram->program);
    // glUniformMatrix4fv(program->proj_uniform, 1, GL_FALSE, egl->proj);
    glEnableVertexAttribArray(glProgram->attrPosition);
    glVertexAttribPointer(glProgram->attrPosition, 2, GL_FLOAT, GL_FALSE, 0, positions);
    glEnableVertexAttribArray(glProgram->attrTexCoord);
    glVertexAttribPointer(glProgram->attrTexCoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);

    for (i = 0; i < glProgram->texCount; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(target, textureIds[i]);
        glUniform1i(glProgram->uniformTex[i], i);
    }
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);

    eglSwapBuffers(context->eglContext.display, context->eglContext.surface);
    int glError = glGetError();
    if (glError != GL_NO_ERROR)
        return glError;

    return 0;
}

EGLContextType *eglInit(Display *x11Display, XID x11Window, uint32_t fourcc, int isExternalTexture)
{
    EGLContextType *context = NULL;
    GLProgram *glProgram = NULL;

    context = calloc(1, sizeof(EGLContextType));
    if (!context)
        return NULL;
    EGLDisplay eglDisplay = eglGetDisplay(x11Display);
    if (eglDisplay == EGL_NO_DISPLAY) {
        ERROR("eglGetDisplay fail");
    }
    CHECK_HANDLE_RET(eglDisplay, EGL_NO_DISPLAY, "eglGetDisplay", NULL);
    context->eglContext.display = eglDisplay;

    EGLint major, minor;
    EGLBoolean result = eglInitialize(eglDisplay, &major, &minor);
    if (result != EGL_TRUE) {
        ERROR("eglInitialize fail");
    }
    EGL_CHECK_RESULT_RET(result, "eglInitialize", NULL);

    // eglBindAPI(EGL_OPENGL_ES_API); // gles is the default one

    EGLint const eglConfigAttribs[] = {
         EGL_RED_SIZE, 8,
         EGL_GREEN_SIZE, 8,
         EGL_BLUE_SIZE, 8,
         EGL_ALPHA_SIZE, 8,
         EGL_DEPTH_SIZE, 8,
         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
         EGL_NONE
    };
    EGLConfig eglConfig;
    EGLint eglConfigCount;
    result = eglChooseConfig(eglDisplay, eglConfigAttribs, &eglConfig, 1, &eglConfigCount);
    EGL_CHECK_RESULT_RET(result, "eglChooseConfig", NULL);
    context->eglContext.config = eglConfig;

    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, x11Window, NULL);
    CHECK_HANDLE_RET(eglSurface, EGL_NO_SURFACE, "eglCreateWindowSurface", NULL);
    context->eglContext.surface = eglSurface;

    EGLint const eglContextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, eglContextAttribs);
    CHECK_HANDLE_RET(eglContext, EGL_NO_CONTEXT, "eglCreateContext", NULL);
    context->eglContext.context = eglContext;

    result = eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    EGL_CHECK_RESULT_RET(result, "eglMakeCurrent", NULL);

    INFO("Runing GL version: %s, please make sure it support GL 2.0 API", glGetString(GL_VERSION));

    // clear to middle blue
    glClearColor(0.0, 0.0, 0.5, 0.0);
    glEnable(GL_DEPTH_TEST);
    glClearDepthf(1.0f);
    {
        unsigned int width=800, height=600;
#ifdef __ENABLE_X11__
        Window root;
        int x, y;
        unsigned int borderWidth, depth;
        XGetGeometry(x11Display, x11Window, &root, &x, &y, &width, &height, &borderWidth, &depth);
#endif
        glViewport(0, 0, width, height);
    }
    if (isExternalTexture)
        glProgram = createShaders(vertexShaderText_rgba, fragShaderText_rgba_ext, 1);
    else
        glProgram = createShaders(vertexShaderText_rgba, fragShaderText_rgba, 1);
    CHECK_HANDLE_RET(glProgram, NULL, "createShaders", NULL);
    context->glProgram = glProgram;

    return context;
error:
    if (context)
        free(context);
    return NULL;
}

void eglRelease(EGLContextType *context)
{
    if (!context)
        return;

    releaseShader(context->glProgram);
    eglMakeCurrent(context->eglContext.display, NULL, NULL, NULL);
    eglDestroySurface(context->eglContext.display, context->eglContext.surface);
    eglDestroyContext(context->eglContext.display, context->eglContext.context);
    eglTerminate(context->eglContext.display);
    free(context);
}

