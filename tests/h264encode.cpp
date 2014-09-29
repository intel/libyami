/*
 *  h264_encode.cpp - h264 encode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Cong Zhong<congx.zhong@intel.com>
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <X11/Xlib.h>

#include "common/log.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"
#include "encodeinput.h"
#include "encodehelp.h"

using namespace YamiMediaCodec;

int main(int argc, char** argv)
{
    IVideoEncoder *encoder = NULL;
    uint32_t maxOutSize = 0;
    EncodeStreamInputPtr input;
    EncodeStreamOutput* output;
    Encode_Status status;
    VideoEncRawBuffer inputBuffer;
    VideoEncOutputBuffer outputBuffer;
    int encodeFrameCount = 0;

    yamiTraceInit();

    if (!process_cmdline(argc, argv))
        return -1;

    DEBUG("inputFourcc: %.4s", &(inputFourcc));
    input = EncodeStreamInput::create(inputFileName, inputFourcc, videoWidth, videoHeight);
    if (!input) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    videoWidth = input->getWidth();
    videoHeight = input->getHeight();

    output = EncodeStreamOutput::create(outputFileName, videoWidth, videoHeight);
    if (!output) {
        fprintf (stderr, "fail to init ouput stream\n");
        return -1;
    }

    encoder = createVideoEncoder(output->getMimeType());
    assert(encoder != NULL);

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
    setEncoderParameters(&encVideoParams);
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);
    status = encoder->start();
    assert(status == ENCODE_SUCCESS);

    //init output buffer
    encoder->getMaxOutSize(&maxOutSize);

    if (!createOutputBuffer(&outputBuffer, maxOutSize)) {
        fprintf (stderr, "fail to create output\n");
        return -1;
    }

    while (!input->isEOS())
    {
        memset(&inputBuffer, 0, sizeof(inputBuffer));
        if (input->getOneFrameInput(inputBuffer)) {
            status = encoder->encode(&inputBuffer);
            ASSERT(status == ENCODE_SUCCESS);
            input->recycleOneFrameInput(inputBuffer);
        }
        else
            break;

        //get the output buffer
        do {
            status = encoder->getOutput(&outputBuffer, false);
            if (status == ENCODE_SUCCESS
                && !output->write(outputBuffer.data, outputBuffer.dataSize))
                assert(0);
        } while (status != ENCODE_BUFFER_NO_MORE);

        if (frameCount &&  encodeFrameCount++ > frameCount)
            break;
    }

    // drain the output buffer
    do {
       status = encoder->getOutput(&outputBuffer, true);
       if (status == ENCODE_SUCCESS
           && !output->write(outputBuffer.data, outputBuffer.dataSize))
           assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);

    encoder->stop();
    releaseVideoEncoder(encoder);
    free(outputBuffer.data);
    delete output;
    fprintf(stderr, "encode done\n");
    return 0;
}
