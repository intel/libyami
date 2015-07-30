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
#include "decodeOutputCapi.h"
#include "decodehelp.h"

int main(int argc, char** argv)
{
    DecodeHandler decoder = NULL;
    DecodeInputHandler input = NULL;
    DecodeOutputHandler output = NULL;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;

    if (!process_cmdline(argc, argv))
        return -1;

    input = createDecodeInput(inputFileName);

    if (input == NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createDecoder(getMimeType(input));
    assert(decoder != NULL);

    output = createDecodeOutput(decoder, renderMode);
    assert(output != NULL);
    if (!configDecodeOutput(output)) {
        fprintf(stderr, "fail to config decoder output");
        return -1;
    }

    if (renderMode == 0) {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_DRM;
        nativeDisplay.handle = 0;
        decodeSetNativeDisplay(decoder, &nativeDisplay);
    } else {
        // TODO, XXX, NativeDisplay should set here, not output->setVideoSize().
    }

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
            if (!decodeOutputSetVideoSize(output, formatInfo->width, formatInfo->height)) {
                assert(0 && "set video size failed");
            }
            status = decode(decoder, &inputBuffer);
        }

        renderOutputFrames(output, false);
    }

    renderOutputFrames(output, true);

    possibleWait(getMimeType(input));

    decodeStop(decoder);
    releaseDecoder(decoder);

    if(input)
        releaseDecodeInput(input);
    if (output)
        releaseDecodeOutput(output);
    if (dumpOutputName)
        free(dumpOutputName);


    fprintf(stderr, "decode done\n");
    return 0;
}
