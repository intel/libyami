/*
 *  decode.cpp - h264 decode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <X11/Xlib.h>

#include "common/log.h"
#include "egl/egl_util.h"
#include "VideoDecoderHost.h"
#include "decodeinput.h"
#include "./egl/gles2_help.h"

using namespace YamiMediaCodec;

static int renderMode = 1;
static bool waitBeforeQuit = false;
char *fileName = NULL;
static IVideoDecoder *decoder = NULL;
static FILE * outFile = NULL;
static Display *x11Display = NULL;
static Window window = 0;
static int64_t timeStamp = 0;
static int32_t videoWidth = 0, videoHeight = 0;
#if __ENABLE_TESTS_GLES__
static EGLContextType *eglContext = NULL;
static XID pixmap = 0;
static GLuint textureId = 0;
#endif

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i media file to decode\n");
    printf("   -w wait before quit\n");
    printf("   -m <render mode>\n");
    printf("      0: dump video frame to file\n");
    printf("      1: render to X window\n");
    printf("      2: texture: render to Pixmap + texture from Pixmap\n");
    printf("      3: texture: export video frame as drm name (RGBX) + texture from drm name\n");
    printf("      4: texture: export video frame as dma_buf(RGBX) + texutre from dma_buf\n");
    printf("      5: texture: export video frame as dma_buf(NV12) + texture from dma_buf\n");
}

bool renderOutputFrames(bool drain = false)
{
    Decode_Status status;
    VideoFrameRawData frame;

    if (renderMode > 0 && !window)
        return false;

    if (renderMode > 1 && !eglContext)
        eglContext = eglInit(x11Display, window, VA_FOURCC_RGBA);

    do {
        switch (renderMode) {
        case 0:
            frame.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
            frame.fourcc = 0;
            frame.width = 0;
            frame.height = 0;
            status = decoder->getOutput(&frame);
            if (status == RENDER_SUCCESS) {
                if (!outFile) {
                    uint32_t fourcc = frame.fourcc;
                    char *ch = (char*)&fourcc;
                    char outFileName[256];
                    sprintf(outFileName, "%s_dump_%dx%d_%c%c%c%c.raw", fileName, frame.width, frame.height, ch[0], ch[1], ch[2], ch[3]);
                    DEBUG("outFileName: %s", outFileName);
                    outFile = fopen(outFileName, "w+");
                }
                ASSERT(outFile);
                switch (frame.fourcc) {
                case VA_FOURCC_NV12: {
                    int row, ret;
                    const char *data = NULL;

                    // Y plane
                    data = (char*)frame.handle + frame.offset[0];
                    for (row = 0; row < frame.height; row++) {
                        ret = fwrite(data, frame.width, 1, outFile);
                        ASSERT(ret = 1);
                        data += frame.pitch[0];
                    }
                    // UV plane
                    int uvWidth = frame.width % 2 ? frame.width+1 : frame.width;
                    int uvHeight = (frame.height +1 )/2;
                    data = (char*) frame.handle + frame.offset[1];
                    for (row = 0; row < uvHeight; row++) {
                        ret = fwrite(data, uvWidth, 1, outFile);
                        ASSERT(ret == 1);
                        data += frame.pitch[1];
                    }
                }
                break;
                default:
                    ASSERT(0);
                    break;
                }
                decoder->renderDone(&frame);
            }
            break;
        case 1:
            status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight, drain);
            break;
#if __ENABLE_TESTS_GLES__
        case 2:
            if (!pixmap) {
                int screen = DefaultScreen(x11Display);
                pixmap = XCreatePixmap(x11Display, DefaultRootWindow(x11Display), videoWidth, videoHeight, XDefaultDepth(x11Display, screen));
                XSync(x11Display, false);
                textureId = createTextureFromPixmap(eglContext, pixmap);
            }
            status = decoder->getOutput(pixmap, &timeStamp, 0, 0, videoWidth, videoHeight);
            if (status == RENDER_SUCCESS) {
                drawTextures(eglContext, &textureId, 1);
            }
            break;
        case 3:
        case 4:
        if (!textureId)
                glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            frame.memoryType = renderMode == 3 ? VIDEO_DATA_MEMORY_TYPE_DRM_NAME : VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
            frame.fourcc = VA_FOURCC_RGBX;
            frame.width = videoWidth;
            frame.height = videoHeight;
            status = decoder->getOutput(&frame);
            if (status == RENDER_SUCCESS) {
                EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
                if (renderMode == 3)
                    eglImage = createEglImageFromDrmBuffer(eglContext->eglContext.display, eglContext->eglContext.context, frame.handle, frame.width, frame.height, frame.pitch[0]);
                else
                    eglImage = createEglImageFromDmaBuf(eglContext->eglContext.display, eglContext->eglContext.context, frame.handle, frame.width, frame.height, frame.pitch[0]);

                if (eglImage != EGL_NO_IMAGE_KHR) {
                    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    drawTextures(eglContext, &textureId, 1);

                    eglDestroyImageKHR(eglContext->eglContext.display, eglImage);
                } else {
                    ERROR("fail to create EGLImage from dma_buf");
                }

                decoder->renderDone(&frame);
            }
            break;
#endif
        default:
            ASSERT(0);
            break;
        }
    } while (status != RENDER_NO_AVAILABLE_FRAME);

    return true;
}

int main(int argc, char** argv)
{
    DecodeStreamInput *input;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    NativeDisplay nativeDisplay;
    char opt;

    while ((opt = getopt(argc, argv, "h:m:i:w?")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
            fileName = optarg;
            break;
        case 'w':
            waitBeforeQuit = true;
            break;
        case 'm':
            renderMode = atoi(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    if (!fileName) {
        fprintf(stderr, "no input media file specified\n");
        return -1;
    }
    fprintf(stderr, "input file: %s, renderMode: %d", fileName, renderMode);

    input = DecodeStreamInput::create(fileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createVideoDecoder(input->getMimeType());
    assert(decoder != NULL);

    if (renderMode > 0) {
        x11Display = XOpenDisplay(NULL);
        nativeDisplay.type = NATIVE_DISPLAY_X11;
        nativeDisplay.handle = (intptr_t)x11Display;
        decoder->setNativeDisplay(&nativeDisplay);
    }

    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;

    status = decoder->start(&configBuffer);
    assert(status == DECODE_SUCCESS);

    while (!input->isEOS())
    {
        if (input->getNextDecodeUnit(inputBuffer)){
            status = decoder->decode(&inputBuffer);
        } else
            break;

        if (DECODE_FORMAT_CHANGE == status) {
            formatInfo = decoder->getFormatInfo();
            videoWidth = formatInfo->width;
            videoHeight = formatInfo->height;

            if (renderMode > 0) {
                if (window) {
                    //todo, resize window;
                } else {
                    int screen = DefaultScreen(x11Display);

                    XSetWindowAttributes x11WindowAttrib;
                    x11WindowAttrib.event_mask = KeyPressMask;
                    window = XCreateWindow(x11Display, DefaultRootWindow(x11Display),
                        0, 0, videoWidth, videoHeight, 0, CopyFromParent, InputOutput,
                        CopyFromParent, CWEventMask, &x11WindowAttrib);
                    XMapWindow(x11Display, window);
                }
                XSync(x11Display, false);
            }

            // resend the buffer
            status = decoder->decode(&inputBuffer);
        }

        renderOutputFrames();
    }

#if 0
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    renderOutputFrames(true);

    while (waitBeforeQuit) {
        XEvent x_event;
        XNextEvent(x11Display, &x_event);
        switch (x_event.type) {
        case KeyPress:
            waitBeforeQuit = false;
            break;
        default:
            break;
        }
    }

    decoder->stop();
    releaseVideoDecoder(decoder);

    if(input)
        delete input;
    if(outFile)
        fclose(outFile);

#if __ENABLE_TESTS_GLES__
    if(textureId)
        glDeleteTextures(1, &textureId);
    if(eglContext)
        eglRelease(eglContext);
    if(pixmap)
        XFreePixmap(x11Display, pixmap);
#endif

    if (x11Display && window)
        XDestroyWindow(x11Display, window);
    if (x11Display)
        XCloseDisplay(x11Display);

    fprintf(stderr, "decode done\n");
}
