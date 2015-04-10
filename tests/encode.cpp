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
#if __ENABLE_X11__
#include <X11/Xlib.h>
#endif
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
    EncodeInput* input;
    EncodeOutput* output;
    Encode_Status status;
    VideoFrameRawData inputBuffer;
    VideoEncOutputBuffer outputBuffer;
    int encodeFrameCount = 0;

    yamiTraceInit();
    if (!process_cmdline(argc, argv))
        return -1;

    DEBUG("inputFourcc: %.4s", &(inputFourcc));
    input = EncodeInput::create(inputFileName, inputFourcc, videoWidth, videoHeight);
    if (!input) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    videoWidth = input->getWidth();
    videoHeight = input->getHeight();

    output = EncodeOutput::create(outputFileName, videoWidth, videoHeight);
    if (!output) {
        fprintf (stderr, "fail to init ouput stream\n");
        return -1;
    }

    encoder = createVideoEncoder(output->getMimeType());
    assert(encoder != NULL);

    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_DRM;
    nativeDisplay.handle = -1;
    encoder->setNativeDisplay(&nativeDisplay);

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->getParameters(VideoParamsTypeCommon, &encVideoParams);
    setEncoderParameters(&encVideoParams);
    encVideoParams.size = sizeof(VideoParamsCommon);
    encoder->setParameters(VideoParamsTypeCommon, &encVideoParams);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);

    status = encoder->start();
    assert(status == ENCODE_SUCCESS);

    //init output buffer
    encoder->getMaxOutSize(&maxOutSize);

#ifdef __BUILD_GET_MV__
    uint32_t size;
    VideoEncMVBuffer MVBuffer;
    MVFp = fopen("feimv.bin","wb");
    encoder->getMVBufferSize(&size);
    if (!createMVBuffer(&MVBuffer, size)) {
        fprintf (stderr, "fail to create MV buffer\n");
        return -1;
    }
#endif
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
#ifndef __BUILD_GET_MV__
            status = encoder->getOutput(&outputBuffer, false);
#else
            status = encoder->getOutput(&outputBuffer, &MVBuffer, false);
#endif
            if (status == ENCODE_SUCCESS
                && !output->write(outputBuffer.data, outputBuffer.dataSize))
                assert(0);
#ifdef __BUILD_GET_MV__
            if (status == ENCODE_SUCCESS) {
                fwrite(MVBuffer.data, MVBuffer.bufferSize, 1, MVFp);
            }
#endif
        } while (status != ENCODE_BUFFER_NO_MORE);

        if (frameCount &&  encodeFrameCount++ > frameCount)
            break;
    }

    // drain the output buffer
    do {
#ifndef __BUILD_GET_MV__
       status = encoder->getOutput(&outputBuffer, true);
#else
       status = encoder->getOutput(&outputBuffer, &MVBuffer, true);
#endif
       if (status == ENCODE_SUCCESS
           && !output->write(outputBuffer.data, outputBuffer.dataSize))
           assert(0);
#ifdef __BUILD_GET_MV__
        if (status == ENCODE_SUCCESS) {
            fwrite(MVBuffer.data, MVBuffer.bufferSize, 1, MVFp);
        }
#endif
    } while (status != ENCODE_BUFFER_NO_MORE);

    encoder->stop();
    releaseVideoEncoder(encoder);
    free(outputBuffer.data);
    delete output;
#ifdef __BUILD_GET_MV__
    free(MVBuffer.data);
    fclose(MVFp);
#endif
    fprintf(stderr, "encode done\n");
    return 0;
}
