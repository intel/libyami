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

using namespace YamiMediaCodec;
class StreamInput {
public:
    static const int MaxNaluSize = 1024*1024; // assume max nalu size is 1M
    static const int CacheBufferSize = 4 * MaxNaluSize;
    StreamInput();
    virtual ~StreamInput();
    static StreamInput * create(char* fileName);
    bool initInput(char* fileName);
    virtual bool isEOS() {return m_parseToEOS;}
    virtual const char * getMimeType() = 0;
    virtual bool init() = 0;
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer) = 0;

public:
    FILE *m_fp;
    uint8_t *m_buffer;
    bool m_readToEOS;
    bool m_parseToEOS;
};

class StreamInputVP8 :public StreamInput
{
public:
    StreamInputVP8();
    ~StreamInputVP8();
    const char * getMimeType();
    bool init();
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
private:
    const int m_ivfHdrSiz;
    const int m_ivfFrmHdrSize;
    const int m_maxFrameSize;
};


class StreamInputRaw:public StreamInput
{
public:
    StreamInputRaw();
    ~StreamInputRaw();
    bool init();
    bool ensureBufferData();
    int32_t scanForStartCode(const uint8_t * data, uint32_t offset, uint32_t size);
    bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
    virtual const char * getMimeType() = 0;
    virtual bool isSyncWord(const uint8_t* buf) = 0;

public:
    uint32_t m_lastReadOffset; // data has been consumed by decoder already
    uint32_t m_availableData;  // available data in m_buffer
    uint32_t StartCodeSize;
};

class StreamInputH264:public StreamInputRaw
{
public:
    StreamInputH264();
    ~StreamInputH264();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
};

class StreamInputJPEG:public StreamInputRaw
{
public:
    StreamInputJPEG();
    ~StreamInputJPEG();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
};

StreamInput::StreamInput()
    : m_fp(NULL)
    , m_buffer(NULL)
    , m_readToEOS(false)
    , m_parseToEOS(false)
{
}

StreamInput::~StreamInput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

bool StreamInput::initInput(char* fileName)
{
    m_fp = fopen(fileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s\n", fileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(CacheBufferSize));
    return init();
}

StreamInput *StreamInput::create(char* fileName)
{
    StreamInput * input = NULL;
    IVideoDecoder* decoder;
    if(fileName==NULL)
        return NULL;
    char *ext = strrchr(fileName,'.');
    if(ext==NULL)
        return NULL;
    ext++;//h264;264;jsv;avc;26l;jvt;ivf
    if(strcasecmp(ext,"h264")==0 ||
        strcasecmp(ext,"264")==0 ||
        strcasecmp(ext,"jsv")==0 ||
        strcasecmp(ext,"avc")==0 ||
        strcasecmp(ext,"26l")==0 ||
        strcasecmp(ext,"jvt")==0 ) {
            input = new StreamInputH264();
        }
    else if((strcasecmp(ext,"ivf")==0) ||
            (strcasecmp(ext,"vp8")==0)) {
            input = new StreamInputVP8();
        }
    else if(strcasecmp(ext,"jpg")==0) {
            input = new StreamInputJPEG();
        }
    else
        return NULL;

    if(!input->initInput(fileName)) {
        delete input;
        return NULL;
    }
    return input;
}

StreamInputVP8::StreamInputVP8()
    : m_ivfHdrSiz(32)
    , m_ivfFrmHdrSize(12)
    , m_maxFrameSize(256*1024)
{
}

StreamInputVP8::~StreamInputVP8()
{
}

const char * StreamInputVP8::getMimeType()
{
    return "video/x-vnd.on2.vp8";
}

bool StreamInputVP8::init()
{
    if (m_ivfHdrSiz != fread (m_buffer, 1, m_ivfHdrSiz, m_fp)) {
        fprintf (stderr, "fail to read ivf header, quit\n");
        return -1;
    }
    return true;
}

bool StreamInputVP8::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
{
    if(m_ivfFrmHdrSize == fread (m_buffer, 1, m_ivfFrmHdrSize, m_fp)) {
        int framesize = 0;
        framesize = (uint32_t)(m_buffer[0]) + ((uint32_t)(m_buffer[1])<<8) + ((uint32_t)(m_buffer[2])<<16);
        assert (framesize < (uint32_t) m_maxFrameSize);
        if (framesize != fread (m_buffer, 1, framesize, m_fp)) {
            fprintf (stderr, "fail to read frame data, quit\n");
            return -1;
        }
        inputBuffer.data = m_buffer;
        inputBuffer.size = framesize;
    }
    else {
        m_parseToEOS = true;
    }
    return 1;
}


StreamInputRaw::StreamInputRaw()
    : m_lastReadOffset(0)
    , m_availableData(0)
{
}

StreamInputRaw::~StreamInputRaw()
{

}

bool StreamInputRaw::init()
{
    int32_t offset = -1;
    // locates to the first start code
    ensureBufferData();
    offset = scanForStartCode(m_buffer, m_lastReadOffset, m_availableData);
    if(offset == -1)
        return false;

    m_lastReadOffset = offset;
    return true;
}

bool StreamInputRaw::ensureBufferData()
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

int32_t StreamInputRaw::scanForStartCode(const uint8_t * data,
                 uint32_t offset, uint32_t size)
{
    uint32_t i;
    const uint8_t *buf;

    if (offset + StartCodeSize > size)
        return -1;

    for (i = 0; i < size - offset - StartCodeSize + 1; i++) {
        buf = data + offset + i;
        if (isSyncWord(buf))
            return i;
    }

    return -1;
}

bool StreamInputRaw::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
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
        offset = m_availableData - m_lastReadOffset;
        m_parseToEOS = true;
    }

    inputBuffer.data = m_buffer + m_lastReadOffset;
    inputBuffer.size = offset;
    // inputBuffer.flag = ;
    // inputBuffer.timeStamp = ; // ignore timestamp
    if (!m_parseToEOS)
       inputBuffer.size += StartCodeSize; // one inputBuffer is start and end with start code

    DEBUG("offset=%d, NALU data=%p, size=%d\n", offset, inputBuffer.data, inputBuffer.size);
    m_lastReadOffset += offset + StartCodeSize;
    return true;
}

StreamInputH264::StreamInputH264()
{
    StartCodeSize = 3;
}

StreamInputH264::~StreamInputH264()
{

}

const char *StreamInputH264::getMimeType()
{
    return "video/h264";
}

bool StreamInputH264::isSyncWord(const uint8_t* buf)
{
    return buf[0] == 0 && buf[1] == 0 && buf[2] == 1;
}

StreamInputJPEG::StreamInputJPEG()
{
    StartCodeSize = 2;
}

StreamInputJPEG::~StreamInputJPEG()
{

}

const char *StreamInputJPEG::getMimeType()
{
    return "image/jpeg";
}

bool StreamInputJPEG::isSyncWord(const uint8_t* buf)
{
    return buf[0] == 0xff && buf[1] == 0xD8;
}

int main(int argc, char** argv)
{
    char *fileName = NULL;
    StreamInput *input;
    IVideoDecoder *decoder = NULL;
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
    INFO("h264 fileName: %s\n", fileName);

    input = StreamInput::create(fileName);

    if (input==NULL) {
        fprintf(stderr, "fail to init input stream\n");
        return -1;
    }

    x11Display = XOpenDisplay(NULL);
    decoder = createVideoDecoder(input->getMimeType());
    nativeDisplay.type = NATIVE_DISPLAY_X11;
    nativeDisplay.handle = (intptr_t)x11Display;
    decoder->setNativeDisplay(&nativeDisplay);

    memset(&configBuffer,0,sizeof(VideoConfigBuffer));
    configBuffer.profile = VAProfileNone;

    status = decoder->start(&configBuffer);
    while (!input->isEOS())
    {
        if (input->getNextDecodeUnit(inputBuffer)){
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
    if(input)
        delete input;
}
