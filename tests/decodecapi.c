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
#include <assert.h>
#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif
#include "capi/VideoDecoderCapi.h"
#include "decodeInputCapi.h"
#include "decodehelp.h"

int main(int argc, char** argv)
{
    DecodeHandler decoder = NULL;
    DecodeInputHandler input = NULL;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    NativeDisplay nativeDisplay;

    if (!process_cmdline(argc, argv))
        return -1;

    input = createDecodeInput(inputFileName);

    if (input == NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createDecoder(getMimeType(input));
    assert(decoder != NULL);

#ifdef __ENABLE_X11__
    if (renderMode > 0) {
        x11Display = XOpenDisplay(NULL);
        nativeDisplay.type = NATIVE_DISPLAY_X11;
        nativeDisplay.handle = (intptr_t)x11Display;
        decodeSetNativeDisplay(decoder, &nativeDisplay);
    }
#endif

    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;

    status = decodeStart(decoder, &configBuffer);
    assert(status == DECODE_SUCCESS);

    while(!decodeInputIsEOS(input))
    {
        if (getNextDecodeUnit(input, &inputBuffer))
            status = decode(decoder, &inputBuffer);
        else
          break;


        if (DECODE_FORMAT_CHANGE == status) {
            formatInfo = getFormatInfo(decoder);
            assert(formatInfo != NULL);
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
                XSync(x11Display, FALSE);
             }
#endif
             status = decode(decoder, &inputBuffer);
        }

        renderOutputFrames(decoder, false);
    }

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

    decodeStop(decoder);
    releaseDecoder(decoder);

    if(input)
        releaseDecodeInput(input);
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
    return 0;
}
