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

    DEBUG("inputFourcc: %.4s", (char*)(&(inputFourcc)));
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
        delete input;
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

    // configure AVC encoding parameters
    VideoParamsAVC encVideoParamsAVC;
    encVideoParamsAVC.size = sizeof(VideoParamsAVC);
    encoder->getParameters(VideoParamsTypeAVC, &encVideoParamsAVC);
    encVideoParamsAVC.idrInterval = idrInterval;
    encVideoParamsAVC.size = sizeof(VideoParamsAVC);
    encoder->setParameters(VideoParamsTypeAVC, &encVideoParamsAVC);

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
        delete input;
        delete output;
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

        encodeFrameCount++;

        if (frameCount && encodeFrameCount >= frameCount)
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
    delete input;
#ifdef __BUILD_GET_MV__
    free(MVBuffer.data);
    fclose(MVFp);
#endif
    fprintf(stderr, "encode done\n");
    return 0;
}
