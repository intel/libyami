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

#include "capi/VideoEncoderCapi.h"
#include "encodeInputCapi.h"
#include "VideoEncoderDefs.h"
#include "encodehelp.h"

int main(int argc, char** argv)
{
    EncodeHandler encoder = NULL;
    uint32_t maxOutSize = 0;
    EncodeInputHandler input = createEncodeInput();
    EncodeOutputHandler output;
    Encode_Status status;
    VideoEncRawBuffer inputBuffer = {0, 0, false, 0, false};
    VideoEncOutputBuffer outputBuffer;

    if (!process_cmdline(argc, argv))
        return -1;

    if (!initInput(input, inputFileName, videoWidth, videoHeight)) {
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

    //configure encoding parameters
    VideoParamsCommon encVideoParams;
    encVideoParams.size = sizeof(VideoParamsCommon);
    getParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    setEncoderParameters(&encVideoParams);
    encVideoParams.size = sizeof(VideoParamsCommon);
    setParameters(encoder, VideoParamsTypeCommon, &encVideoParams);
    status = encodeStart(encoder);
    assert(status == ENCODE_SUCCESS);

    //init output buffer
    getMaxOutSize(encoder, &maxOutSize);

    if (!createOutputBuffer(&outputBuffer, maxOutSize)) {
        fprintf (stderr, "fail to init input stream\n");
        return -1;
    }

    while (!encodeInputIsEOS(input))
    {
        if (getOneFrameInput(input, &inputBuffer)){
            status = encode(encoder, &inputBuffer);}
        else
            break;

        //get the output buffer
        do {
            status = encodeGetOutput(encoder, &outputBuffer, false);
            if (status == ENCODE_SUCCESS
              && !writeOutput(output, outputBuffer.data, outputBuffer.dataSize))
                assert(0);
        } while (status != ENCODE_BUFFER_NO_MORE);
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
