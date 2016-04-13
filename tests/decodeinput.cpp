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

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "decodeinput.h"
#include "common/NonCopyable.h"
#include "common/log.h"

#ifdef __ENABLE_AVFORMAT__
#include "decodeinputavformat.h"
#endif

using namespace YamiMediaCodec;

class MyDecodeInput : public DecodeInput{
public:
    static const size_t MaxNaluSize = 1024*1024*4; // assume max nalu size is 4M
    static const size_t CacheBufferSize = 8 * MaxNaluSize;
    MyDecodeInput();
    virtual ~MyDecodeInput();
    bool initInput(const char* fileName);
    virtual bool isEOS() {return m_parseToEOS;}
    virtual bool init() = 0;
    virtual const string& getCodecData();
protected:
    FILE *m_fp;
    uint8_t *m_buffer;
    bool m_readToEOS;
    bool m_parseToEOS;
private:
   DISALLOW_COPY_AND_ASSIGN(MyDecodeInput);
};

class DecodeInputVPX :public MyDecodeInput
{
public:
    DecodeInputVPX();
    ~DecodeInputVPX();
    const char * getMimeType();
    bool init();
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
private:
    const size_t m_ivfFrmHdrSize;
    const size_t m_maxFrameSize;
    const char* m_mimeType;
};

class DecodeInputRaw:public MyDecodeInput
{
public:
    DecodeInputRaw();
    ~DecodeInputRaw();
    bool init();
    bool ensureBufferData();
    int32_t scanForStartCode(const uint8_t * data, uint32_t offset, uint32_t size);
    bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
    virtual bool isSyncWord(const uint8_t* buf) = 0;

public:
    uint32_t m_lastReadOffset; // data has been consumed by decoder already
    uint32_t m_availableData;  // available data in m_buffer
    uint32_t StartCodeSize;
};

class DecodeInputH26x:public DecodeInputRaw
{
public:
    DecodeInputH26x(const char* mime);
    ~DecodeInputH26x();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
    const char* m_mime;
};

class DecodeInputJPEG:public DecodeInputRaw
{
public:
    DecodeInputJPEG();
    ~DecodeInputJPEG();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
private:
    int m_countSOI;
};

DecodeInput::DecodeInput()
: m_width(0), m_height(0)
{
}

DecodeInput* DecodeInput::create(const char* fileName)
{
    DecodeInput* input = NULL;
    if(fileName==NULL)
        return NULL;
    const char *ext = strrchr(fileName,'.');
    if(ext==NULL)
        return NULL;
    ext++;//h264;264;jsv;avc;26l;jvt;ivf
    if(strcasecmp(ext,"h264")==0 ||
        strcasecmp(ext,"264")==0 ||
        strcasecmp(ext,"jsv")==0 ||
        strcasecmp(ext,"avc")==0 ||
        strcasecmp(ext,"26l")==0 ||
        strcasecmp(ext,"jvt")==0 ) {
        input = new DecodeInputH26x(YAMI_MIME_H264);
    } else if (strcasecmp(ext,"265") == 0 ||
               strcasecmp(ext,"h265") == 0 ||
               strcasecmp(ext,"bin") == 0 ) {
        input = new DecodeInputH26x(YAMI_MIME_H265);
    } else if((strcasecmp(ext,"ivf")==0) ||
            (strcasecmp(ext,"vp8")==0) ||
            (strcasecmp(ext,"vp9")==0)) {
            input = new DecodeInputVPX();
        }
    else if(strcasecmp(ext,"jpg")==0 ||
            strcasecmp(ext,"jpeg")==0 ||
            strcasecmp(ext,"mjpg")==0 ||
            strcasecmp(ext,"mjpeg")==0) {
            input = new DecodeInputJPEG();
        }
    else {
#ifdef __ENABLE_AVFORMAT__
            input = new DecodeInputAvFormat();
#else
            return NULL;
#endif
        }

    if(!input->initInput(fileName)) {
        delete input;
        return NULL;
    }
    return input;
}

void DecodeInput::setResolution(const uint16_t width, const uint16_t height)
{
  m_width = width;
  m_height = height;
}

MyDecodeInput::MyDecodeInput()
    : m_fp(NULL)
    , m_buffer(NULL)
    , m_readToEOS(false)
    , m_parseToEOS(false)
{
}

MyDecodeInput::~MyDecodeInput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

bool MyDecodeInput::initInput(const char* fileName)
{
    m_fp = fopen(fileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s\n", fileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(CacheBufferSize));
    return init();
}

const string& MyDecodeInput::getCodecData()
{
    //no codec data;
    static const string dummy;
    return dummy;
}

struct IvfHeader {
    uint32_t tag;
    uint32_t version;
    uint32_t fourcc;
    uint16_t width;
    uint16_t height;
    uint32_t dummy[4];
};

DecodeInputVPX::DecodeInputVPX()
    : m_ivfFrmHdrSize(12)
    , m_maxFrameSize(4096*4096*3/2)
    , m_mimeType("unknown")
{
}

DecodeInputVPX::~DecodeInputVPX()
{
}

const char * DecodeInputVPX::getMimeType()
{
    return m_mimeType;
}

bool DecodeInputVPX::init()
{
    IvfHeader header;
    size_t size = sizeof(header);
    assert(size == 32);
    if (size != fread (&header, 1, size, m_fp)) {
        fprintf (stderr, "fail to read ivf header, quit\n");
        return false;
    }
    if (header.tag != YAMI_FOURCC('D', 'K', 'I', 'F'))
        return false;
    if (header.fourcc == YAMI_FOURCC('V', 'P', '8', '0'))
        m_mimeType = YAMI_MIME_VP8;
    else if (header.fourcc == YAMI_FOURCC('V', 'P', '9', '0'))
        m_mimeType = YAMI_MIME_VP9;

    setResolution(header.width, header.height);

    return true;
}

bool DecodeInputVPX::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
{
    if(m_ivfFrmHdrSize == fread (m_buffer, 1, m_ivfFrmHdrSize, m_fp)) {
        size_t framesize = 0;
        framesize = (uint32_t)(m_buffer[0]) + ((uint32_t)(m_buffer[1])<<8) + ((uint32_t)(m_buffer[2])<<16);
        assert (framesize < m_maxFrameSize);
        assert (framesize <= CacheBufferSize);

        if (framesize != fread (m_buffer, 1, framesize, m_fp)) {
            fprintf (stderr, "fail to read frame data, quit\n");
            return false;
        }
        inputBuffer.data = m_buffer;
        inputBuffer.size = framesize;
    }
    else {
        m_parseToEOS = true;
        return false;
    }
    return true;
}

DecodeInputRaw::DecodeInputRaw()
    : m_lastReadOffset(0)
    , m_availableData(0)
{
}

DecodeInputRaw::~DecodeInputRaw()
{
}

bool DecodeInputRaw::init()
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

bool DecodeInputRaw::ensureBufferData()
{
    size_t readCount = 0;

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

    readCount = fread(m_buffer + m_availableData, 1, CacheBufferSize-m_availableData, m_fp);
    if (readCount < CacheBufferSize-m_availableData)
        m_readToEOS = true;

    m_availableData += readCount;
    return true;
}

int32_t DecodeInputRaw::scanForStartCode(const uint8_t * data,
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

bool DecodeInputRaw::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
{
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
    inputBuffer.flag = IS_NAL_UNIT;
    // inputBuffer.timeStamp = ; // ignore timestamp
    if (!m_parseToEOS)
       inputBuffer.size += StartCodeSize; // one inputBuffer is start and end with start code

    DEBUG("offset=%d, NALU data=%p, size=%zu\n", offset, inputBuffer.data, inputBuffer.size);
    m_lastReadOffset += offset + StartCodeSize;
    return true;
}

DecodeInputH26x::DecodeInputH26x(const char* mime)
    :m_mime(mime)
{
    StartCodeSize = 3;
}

DecodeInputH26x::~DecodeInputH26x()
{

}

const char *DecodeInputH26x::getMimeType()
{
    return m_mime;
}

bool DecodeInputH26x::isSyncWord(const uint8_t* buf)
{
    return buf[0] == 0 && buf[1] == 0 && buf[2] == 1;
}

DecodeInputJPEG::DecodeInputJPEG()
{
    StartCodeSize = 2;
    m_countSOI = 0;
}

DecodeInputJPEG::~DecodeInputJPEG()
{

}

const char *DecodeInputJPEG::getMimeType()
{
    return YAMI_MIME_JPEG;
}

bool DecodeInputJPEG::isSyncWord(const uint8_t* buf)
{
    if (buf[0] != 0xff)
        return false;

    if (buf[1] == 0xD8)
        m_countSOI++;
    if (buf[1] == 0xD9)
        m_countSOI--;

    if (buf[1] == 0xD8 && m_countSOI == 1)
        return true;

    return false;
}
