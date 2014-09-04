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
#include <assert.h>
#include <X11/Xlib.h>

#include "common/log.h"
#include "VideoDecoderHost.h"
#include "decodeinput.h"

using namespace YamiMediaCodec;

int main(int argc, char** argv)
{
    char *fileName = NULL;
    DecodeStreamInput *input;
    IVideoDecoder *decoder = NULL;
    VideoDecodeBuffer inputBuffer;
    Display *x11Display = NULL;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    Window window = 0;
    int64_t timeStamp = 0;
    int32_t videoWidth = 0, videoHeight = 0;
    NativeDisplay nativeDisplay;

    if (argc <2) {
        fprintf(stderr, "no input file to decode\n");
        return -1;
    }
    fileName = argv[1];
    INFO("h264 fileName: %s\n", fileName);

    input = DecodeStreamInput::create(fileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    x11Display = XOpenDisplay(NULL);
    decoder = createVideoDecoder(input->getMimeType());
    assert(decoder != NULL);

    nativeDisplay.type = NATIVE_DISPLAY_X11;
    nativeDisplay.handle = (intptr_t)x11Display;
    decoder->setNativeDisplay(&nativeDisplay);

    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;

    status = decoder->start(&configBuffer);
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

            if (window) {
                //todo, resize window;
            } else {
                window = XCreateSimpleWindow(x11Display, RootWindow(x11Display, DefaultScreen(x11Display))
                    , 0, 0, videoWidth, videoHeight, 0, 0
                    , WhitePixel(x11Display, 0));
                XMapWindow(x11Display, window);
            }

            // resend the buffer
            status = decoder->decode(&inputBuffer);
            XSync(x11Display, false);
        }

        // render the frame if available
        do {
            status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight);
        } while (status != RENDER_NO_AVAILABLE_FRAME);
    }

#if 0
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    do {
        status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight, true);
    } while (status != RENDER_NO_AVAILABLE_FRAME);

    decoder->stop();
    releaseVideoDecoder(decoder);
    XDestroyWindow(x11Display, window);
    if(input)
        delete input;
}
