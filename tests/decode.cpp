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
#include "common/utils.h"
#include "VideoDecoderHost.h"
#include "decodeinput.h"
#include "decodeoutput.h"
#include "decodehelp.h"
#ifdef __ENABLE_X11__
// #include <X11/Xlib.h>
#endif

using namespace YamiMediaCodec;

int main(int argc, char** argv)
{
    IVideoDecoder *decoder = NULL;
    DecodeInput *input;
    DecodeOutput *output;
    VideoDecodeBuffer inputBuffer;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    class CalcFps calcFpsGross, calcFpsNet;
    int skipFrameCount4NetFps = 0;
#ifdef __ENABLE_X11__
    // XInitThreads();
#endif
    calcFpsGross.setAnchor();
    yamiTraceInit();
    if (!process_cmdline(argc, argv))
        return -1;
#if !__ENABLE_TESTS_GLES__
    if (renderMode > 1) {
        fprintf(stderr, "renderMode=%d is not supported, please rebuild with --enable-tests-gles option\n", renderMode);
        return -1;
    }
#endif
#if !__ENABLE_MD5__
    if (renderMode == -2) {
        fprintf(stderr, "renderMode=%d is not supported, you must compile libyami without --disable-md5 option "
                "and package libssl-dev should be installed\n", renderMode);
        return -1;
    }
#endif
    input = DecodeInput::create(inputFileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    decoder = createVideoDecoder(input->getMimeType());
    assert(decoder != NULL);

    output = DecodeOutput::create(decoder, renderMode);
    if (!output || !configDecodeOutput(output)) {
        fprintf(stderr, "failed to config decode output of mode: %d\n", renderMode);
        delete input;
        return -1;
    }

    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;
    const string codecData = input->getCodecData();
    if (codecData.size()) {
        configBuffer.data = (uint8_t*)codecData.data();
        configBuffer.size = codecData.size();
    }

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

        renderOutputFrames(output, frameCount);
        if (output->renderFrameCount() >= 5 && !skipFrameCount4NetFps) {
            skipFrameCount4NetFps = output->renderFrameCount();
            calcFpsNet.setAnchor();
        }

        if(output->renderFrameCount() == frameCount)
            break;
    }

#if 1
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    renderOutputFrames(output, frameCount, true);

    calcFpsGross.fps(output->renderFrameCount());
    if (output->renderFrameCount() > skipFrameCount4NetFps)
        calcFpsNet.fps(output->renderFrameCount()-skipFrameCount4NetFps);

    possibleWait(input->getMimeType());

    decoder->stop();
    releaseVideoDecoder(decoder);

    delete input;
    delete output;

    if (dumpOutputName)
        free(dumpOutputName);
    fprintf(stderr, "decode done\n");
    return 0;
}
