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
#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif

#include "common/log.h"
#include "VideoDecoderHost.h"
#include "decodeinput.h"
#include "decodehelp.h"

#ifdef __ENABLE_TESTS_GLES__
#include "./egl/gles2_help.h"
#include "egl/egl_util.h"
#endif

using namespace YamiMediaCodec;

int main(int argc, char** argv)
{
    IVideoDecoder *decoder = NULL;
    DecodeStreamInput *input;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    NativeDisplay nativeDisplay;

    yamiTraceInit();
    if (!process_cmdline(argc, argv))
        return -1;
    input = DecodeStreamInput::create(inputFileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createVideoDecoder(input->getMimeType());
    assert(decoder != NULL);

#ifdef __ENABLE_X11__
    if (renderMode > 0) {
        x11Display = XOpenDisplay(NULL);
        nativeDisplay.type = NATIVE_DISPLAY_X11;
        nativeDisplay.handle = (intptr_t)x11Display;
        decoder->setNativeDisplay(&nativeDisplay);
    }
#endif

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

#ifdef __ENABLE_X11__
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
#endif

            // resend the buffer
            status = decoder->decode(&inputBuffer);
        }

        renderOutputFrames(decoder);
    }

#if 0
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    renderOutputFrames(decoder, true);

#ifdef __ENABLE_X11__
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
#endif

    decoder->stop();
    releaseVideoDecoder(decoder);

    if(input)
        delete input;
    if(outFile)
        fclose(outFile);
    if (dumpOutputDir)
        free(dumpOutputDir);

#if __ENABLE_TESTS_GLES__
    if(textureId)
        glDeleteTextures(1, &textureId);
    if(eglContext)
        eglRelease(eglContext);
    if(pixmap)
        XFreePixmap(x11Display, pixmap);
#endif

#ifdef __ENABLE_X11__
    if (x11Display && window)
        XDestroyWindow(x11Display, window);
    if (x11Display)
        XCloseDisplay(x11Display);
#endif

    fprintf(stderr, "decode done\n");
}
