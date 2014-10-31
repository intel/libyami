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

#include "common/log.h"
#include "VideoDecoderHost.h"
#include "decodeinput.h"
#include "decodeoutput.h"
#include "decodehelp.h"

using namespace YamiMediaCodec;

int main(int argc, char** argv)
{
    IVideoDecoder *decoder = NULL;
    DecodeStreamInput *input;
    DecodeStreamOutput *output;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;

    yamiTraceInit();
    if (!process_cmdline(argc, argv))
        return -1;
    if (renderMode > 1) {
        fprintf(stderr, "renderMode=%d is not supported, please rebuild with --enable-tests-gles option\n", renderMode);
        return -1;
    }

    input = DecodeStreamInput::create(inputFileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createVideoDecoder(input->getMimeType());
    assert(decoder != NULL);

    output = DecodeStreamOutput::create(decoder, renderMode);
    if (!configDecodeOutput(output)) {
        fprintf(stderr, "failed to config decode output");
        return -1;
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
            if (!output->setVideoSize(formatInfo->width, formatInfo->height)) {
                assert(0 && "set video size failed");
                break;
            }
            // resend the buffer
            status = decoder->decode(&inputBuffer);
        }

        renderOutputFrames(output);
    }

#if 0
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    renderOutputFrames(output, true);
    possibleWait(input->getMimeType());

    decoder->stop();
    releaseVideoDecoder(decoder);

    delete input;
    delete output;

    if (dumpOutputDir)
        free(dumpOutputDir);
    fprintf(stderr, "decode done\n");
}
