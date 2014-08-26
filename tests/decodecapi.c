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
#include <X11/Xlib.h>

#include "capi/VideoDecoderCapi.h"

#ifndef bool
#define bool  int
#endif

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif

static const int MaxNaluSize = 1024*1024;
static const int CacheBufferSize = 4 * 1024 * 1024;
const uint32_t StartCodeSize = 3;

typedef struct StreamInput{
    FILE *m_fp;
    uint8_t *m_buffer;
    bool m_readToEOS;
    bool m_parseToEOS;
    uint32_t m_lastReadOffset;
    uint32_t m_availableData;
}StreamInput;

StreamInput input = {NULL, 0, 0, 0, 0, 0};

int32_t scanForStartCode(const uint8_t * data,
                 uint32_t offset, uint32_t size)
{
    uint32_t i;
    const uint8_t *buf;

    if (offset + StartCodeSize > size)
        return -1;

    for (i = 0; i < size - offset - StartCodeSize + 1; i++) {
        buf = data + offset + i;
        if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
            return i;
    }

    return -1;
}

bool ensureBufferData()
{
    int readCount = 0;

    if (input.m_readToEOS)
        return true;

    // available data is enough for parsing
    if (input.m_lastReadOffset + MaxNaluSize < input.m_availableData)
        return true;

    // move unused data to the begining of m_buffer
    if (input.m_availableData + MaxNaluSize >= CacheBufferSize) {
        memcpy(input.m_buffer, input.m_buffer+input.m_lastReadOffset, input.m_availableData-input.m_lastReadOffset);
        input.m_availableData = input.m_availableData-input.m_lastReadOffset;
        input.m_lastReadOffset = 0;
    }

    readCount = fread(input.m_buffer + input.m_availableData, 1, MaxNaluSize, input.m_fp);
    if (readCount < MaxNaluSize)
        input.m_readToEOS = true;

    input.m_availableData += readCount;

    return true;
}

bool init(const char* fileName)
{
    input.m_fp = fopen(fileName, "r");
    
    if (!input.m_fp) {
        fprintf(stderr, "fail to open input file: %s\n", fileName);
        return false;
    }
  
    input.m_buffer = (uint8_t*)(malloc(CacheBufferSize));
    
    int32_t offset = -1;

    ensureBufferData();
    offset = scanForStartCode(input.m_buffer, input.m_lastReadOffset, input.m_availableData);
    if(offset == -1)
        return false;

    input.m_lastReadOffset = offset;
    return true;
}

bool getOneClipInput(VideoDecodeBuffer *inputBuffer)
{
    bool gotOneNalu= false;
    int32_t offset = -1;

    if(input.m_parseToEOS)
        return false;

    ensureBufferData();
    offset = scanForStartCode(input.m_buffer, input.m_lastReadOffset+StartCodeSize, input.m_availableData);

    if (offset == -1) {
        assert(input.m_readToEOS);
        offset = input.m_availableData;
        input.m_parseToEOS = true;
    }

    inputBuffer->data = input.m_buffer + input.m_lastReadOffset;
    inputBuffer->size = offset;

    if(!input.m_parseToEOS)
        inputBuffer->size += StartCodeSize;

    printf("offset=%d, Clip data=%p, size=%d\n", offset, inputBuffer->data, inputBuffer->size);
    input.m_lastReadOffset += offset + StartCodeSize;
    return true;
}

int main(int argc, char** argv)
{
    const char *fileName = NULL;
    DecodeHandler* decoder = NULL;
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
    fprintf(stderr, "h264 fileName: %s\n", fileName);

    if (!init(fileName)) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    x11Display = XOpenDisplay(NULL);

    decoder = createDecoder("video/h264");
    nativeDisplay.type = NATIVE_DISPLAY_X11;
    nativeDisplay.handle = (intptr_t)x11Display;

    decodeSetNativeDisplay(decoder, &nativeDisplay);
    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.data = NULL;
    configBuffer.size = 0;
    configBuffer.width = -1;
    configBuffer.height = -1;
    configBuffer.profile = VAProfileH264Main;

    status = decodeStart(decoder, &configBuffer);

    while(!input.m_parseToEOS)
    {
        if (getOneClipInput(&inputBuffer))
            status = decode(decoder, &inputBuffer);

        if (DECODE_FORMAT_CHANGE == status) {
            formatInfo = getFormatInfo(decoder);
            videoWidth = formatInfo->width;
            videoHeight = formatInfo->height;

            if (!window) {
                window = XCreateSimpleWindow(x11Display, RootWindow(x11Display, DefaultScreen(x11Display))
                    , 0, 0, videoWidth, videoHeight, 0, 0
                    , WhitePixel(x11Display, 0));
                XMapWindow(x11Display, window);
            }

            // resend the buffer
            status = decode(decoder, &inputBuffer);
            XSync(x11Display, FALSE);
        }

        // render the frame if available
        do {
            status = decodeGetOutput_x11(decoder, window, &timeStamp, 0, 0, videoWidth, videoHeight, false, -1, -1, -1, -1);
        } while (status != RENDER_NO_AVAILABLE_FRAME);
    }

    // drain the output buffer
    do {
        status = decodeGetOutput_x11(decoder, window, &timeStamp, 0, 0, videoWidth, videoHeight, TRUE, -1, -1, -1, -1);
    } while (status != RENDER_NO_AVAILABLE_FRAME);

    decodeStop(decoder);
    releaseDecoder(decoder);
    XDestroyWindow(x11Display, window);
}
