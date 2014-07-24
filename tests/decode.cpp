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

#include "common/log.h"
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"
#include "VideoDecoderHost.h"

const uint32_t StartCodeSize = 3;
static inline int32_t
scanForStartCode(const uint8_t * data,
                 uint32_t offset, uint32_t size)
{
    uint32_t i;
    const uint8_t *buf;

    if (offset + 3 > size)
        return -1;

    for (i = 0; i < size - offset - 3 + 1; i++) {
        buf = data + offset + i;
        if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
            return i;
    }

    return -1;
}

using namespace YamiMediaCodec;
class StreamInput {
public:
    static const int MaxNaluSize = 1024*1024; // assume max nalu size is 1M
    static const int CacheBufferSize = 4 * MaxNaluSize;
    StreamInput();
    ~StreamInput();
    bool init(const char* fileName);
    bool getOneNaluInput(VideoDecodeBuffer &inputBuffer);
    bool isEOS() {return m_parseToEOS;};

private:
    bool ensureBufferData();
    FILE *m_fp;
    uint32_t m_lastReadOffset; // data has been consumed by decoder already
    uint32_t m_availableData;  // available data in m_buffer
    uint8_t *m_buffer;

    bool m_readToEOS;
    bool m_parseToEOS;
};

StreamInput::StreamInput()
    : m_fp(NULL)
    , m_lastReadOffset(0)
    , m_availableData(0)
    , m_buffer(NULL)
    , m_readToEOS(false)
    , m_parseToEOS(false)
{
}

bool StreamInput::init(const char* fileName)
{
    int32_t offset = -1;

    m_fp = fopen(fileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s\n", fileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(CacheBufferSize));

    // locates to the first start code
    ensureBufferData();
    offset = scanForStartCode(m_buffer, m_lastReadOffset, m_availableData);
    if(offset == -1)
        return false;

    m_lastReadOffset = offset;
    return true;
}

StreamInput::~StreamInput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

bool StreamInput::ensureBufferData()
{
    int readCount = 0;

    if (m_readToEOS)
        return true;

    // available data is enough for parsing
    if (m_lastReadOffset + MaxNaluSize < m_availableData)
        return true;

    // move unused data to the begining of m_buffer
    if (m_availableData + MaxNaluSize >= CacheBufferSize) {
        memcpy(m_buffer, m_buffer+m_lastReadOffset, m_availableData-m_lastReadOffset);
        m_availableData = m_availableData-m_lastReadOffset;
        m_lastReadOffset = 0;
    }

    readCount = fread(m_buffer + m_availableData, 1, MaxNaluSize, m_fp);
    if (readCount < MaxNaluSize)
        m_readToEOS = true;

    m_availableData += readCount;
    return true;
}

bool StreamInput::getOneNaluInput(VideoDecodeBuffer &inputBuffer)
{
    bool gotOneNalu= false;
    int32_t offset = -1;

    if(m_parseToEOS)
        return false;

    // parsing data for one NAL unit
    ensureBufferData();
    DEBUG("m_lastReadOffset=0x%x, m_availableData=0x%x\n", m_lastReadOffset, m_availableData);
    offset = scanForStartCode(m_buffer, m_lastReadOffset+StartCodeSize, m_availableData);

    if (offset == -1) {
        assert(m_readToEOS);
        offset = m_availableData;
        m_parseToEOS = true;
    }

    inputBuffer.data = m_buffer + m_lastReadOffset;
    inputBuffer.size = offset;
    // inputBuffer.flag = ;
    // inputBuffer.timeStamp = ; // ignore timestamp
    if (!m_parseToEOS)
        inputBuffer.size += 3; // one inputBuffer is start and end with start code

    DEBUG("offset=%d, NALU data=%p, size=%d\n", offset, inputBuffer.data, inputBuffer.size);
    m_lastReadOffset += offset + StartCodeSize;
    return true;
}

int main(int argc, char** argv)
{
    const char *fileName = NULL;
    StreamInput input;
    IVideoDecoder *decoder = NULL;
    VideoDecodeBuffer inputBuffer;
    Display *x11Display = NULL;
    VideoConfigBuffer configBuffer;
    const VideoFormatInfo *formatInfo = NULL;
    Decode_Status status;
    Window window = 0;
    int64_t timeStamp = 0;
    int32_t videoWidth = 0, videoHeight = 0;

    if (argc <2) {
        fprintf(stderr, "no input file to decode\n");
        return -1;
    }
    fileName = argv[1];
    INFO("h264 fileName: %s\n", fileName);

    if (!input.init(fileName)) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    x11Display = XOpenDisplay(NULL);
    decoder = createVideoDecoder("video/h264");
    decoder->setXDisplay(x11Display);

    configBuffer.data = NULL;
    configBuffer.size = 0;
    configBuffer.width = -1;
    configBuffer.height = -1;
    // TODO, parse profile from stream
    configBuffer.profile = VAProfileH264Main;
    status = decoder->start(&configBuffer);

    while (!input.isEOS())
    {
        if (input.getOneNaluInput(inputBuffer)){
            status = decoder->decode(&inputBuffer);
        } else
            break;

        if (DECODE_FORMAT_CHANGE == status) {
            formatInfo = decoder->getFormatInfo();
            videoWidth = formatInfo->width;
            videoHeight = formatInfo->height;

            if (window) {
                //todo, resize window;
            } else {
                window = XCreateSimpleWindow(x11Display, RootWindow(x11Display, DefaultScreen(x11Display))
                    , 0, 0, videoWidth, videoHeight, 0, 0
                    , WhitePixel(x11Display, 0));
                XMapWindow(x11Display, window);
            }

            // resend the buffer
            status = decoder->decode(&inputBuffer);
            XSync(x11Display, false);
        }

        // render the frame if available
        do {
            status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight);
        } while (status != RENDER_NO_AVAILABLE_FRAME);
    }

#if 0
    // send EOS to decoder
    inputBuffer.data = NULL;
    inputBuffer.size = 0;
    status = decoder->decode(&inputBuffer);
#endif

    // drain the output buffer
    do {
        status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight, true);
    } while (status != RENDER_NO_AVAILABLE_FRAME);

    decoder->stop();
    releaseVideoDecoder(decoder);
    XDestroyWindow(x11Display, window);
}
