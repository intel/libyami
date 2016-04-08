/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    int frames = 0;
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

        frames = renderOutputFrames(output, frameCount, false);
        if((frames == -1) || (frames == frameCount))
            break;
    }

    renderOutputFrames(output, frameCount, true);

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
