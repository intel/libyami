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

#include "common/log.h"
#include "capi/VideoEncoderCapi.h"
#include "encodeInputCapi.h"
#include "VideoEncoderDefs.h"
#include "encodehelp.h"

int main(int argc, char** argv)
{
    EncodeHandler encoder = NULL;
    uint32_t maxOutSize = 0;
    EncodeInputHandler input;
    EncodeOutputHandler output;
    Encode_Status status;
    VideoFrameRawData inputBuffer;
    VideoEncOutputBuffer outputBuffer;
    int encodeFrameCount = 0;

    if (!process_cmdline(argc, argv))
        return -1;

    DEBUG("inputFourcc: %.4s", (char*)(&inputFourcc));
    input = createEncodeInput(inputFileName, inputFourcc, videoWidth, videoHeight);

    if (!input) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    videoWidth = getInputWidth(input);
    videoHeight = getInputHeight(input);

    output = createEncodeOutput(outputFileName, videoWidth, videoHeight);
    if(!output) {
      fprintf(stderr, "fail to init ourput stream\n");
      return -1;
    }

    encoder = createEncoder(getOutputMimeType(output));
    assert(encoder != NULL);

    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_DRM;
    nativeDisplay.handle = 0;
    encodeSetNativeDisplay(encoder, &nativeDisplay);

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    encodeGetParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    setEncoderParameters(&encVideoParams);
    encVideoParams.size = sizeof(VideoParamsCommon);
    encodeSetParameters(encoder, VideoParamsTypeCommon, &encVideoParams);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    encodeSetParameters(encoder, VideoConfigTypeAVCStreamFormat, &streamFormat);

    status = encodeStart(encoder);
    assert(status == ENCODE_SUCCESS);

    //init output buffer
    encodeGetMaxOutSize(encoder, &maxOutSize);

    if (!createOutputBuffer(&outputBuffer, maxOutSize)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    while (!encodeInputIsEOS(input))
    {
        memset(&inputBuffer, 0, sizeof(inputBuffer));
        if (getOneFrameInput(input, &inputBuffer)){
            status = encodeEncodeRawData(encoder, &inputBuffer);
            recycleOneFrameInput(input, &inputBuffer);
        }
        else
            break;

        //get the output buffer
        do {
            status = encodeGetOutput(encoder, &outputBuffer, false);
            if (status == ENCODE_SUCCESS
              && !writeOutput(output, outputBuffer.data, outputBuffer.dataSize))
                assert(0);
        } while (status != ENCODE_BUFFER_NO_MORE);

        if (frameCount &&  encodeFrameCount++ > frameCount)
            break;
    }

    // drain the output buffer
    do {
       status = encodeGetOutput(encoder, &outputBuffer, true);
       if (status == ENCODE_SUCCESS
           && !writeOutput(output, outputBuffer.data, outputBuffer.dataSize))
           assert(0);
    } while (status != ENCODE_BUFFER_NO_MORE);

    encodeStop(encoder);
    releaseEncoder(encoder);
    releaseEncodeInput(input);
    releaseEncodeOutput(output);
    free(outputBuffer.data);

    fprintf(stderr, "encode done\n");
    return 0;
}
