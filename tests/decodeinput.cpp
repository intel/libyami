/*
 *  decodeinput.cpp - decode test input
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Changzhi Wei<changzhix.wei@intel.com>
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

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "decodeinput.h"
#include "common/log.h"

using namespace YamiMediaCodec;

DecodeStreamInput::DecodeStreamInput()
    : m_fp(NULL)
    , m_buffer(NULL)
    , m_readToEOS(false)
    , m_parseToEOS(false)
{
}

DecodeStreamInput::~DecodeStreamInput()
{
    if(m_fp)
        fclose(m_fp);

    if(m_buffer)
        free(m_buffer);
}

bool DecodeStreamInput::initInput(char* fileName)
{
    m_fp = fopen(fileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s\n", fileName);
        return false;
    }

    m_buffer = static_cast<uint8_t*>(malloc(CacheBufferSize));
    return init();
}

DecodeStreamInput* DecodeStreamInput::create(char* fileName)
{
    DecodeStreamInput* input = NULL;
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
            input = new DecodeStreamInputH264();
        }
    else if((strcasecmp(ext,"ivf")==0) ||
            (strcasecmp(ext,"vp8")==0)) {
            input = new DecodeStreamInputVP8();
        }
    else if(strcasecmp(ext,"jpg")==0) {
            input = new DecodeStreamInputJPEG();
        }
    else
        return NULL;

    if(!input->initInput(fileName)) {
        delete input;
        return NULL;
    }
    return input;
}

DecodeStreamInputVP8::DecodeStreamInputVP8()
    : m_ivfHdrSiz(32)
    , m_ivfFrmHdrSize(12)
    , m_maxFrameSize(256*1024)
{
}

DecodeStreamInputVP8::~DecodeStreamInputVP8()
{
}

const char * DecodeStreamInputVP8::getMimeType()
{
    return "video/x-vnd.on2.vp8";
}

bool DecodeStreamInputVP8::init()
{
    if (m_ivfHdrSiz != fread (m_buffer, 1, m_ivfHdrSiz, m_fp)) {
        fprintf (stderr, "fail to read ivf header, quit\n");
        return false;
    }
    return true;
}

bool DecodeStreamInputVP8::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
{
    if(m_ivfFrmHdrSize == fread (m_buffer, 1, m_ivfFrmHdrSize, m_fp)) {
        int framesize = 0;
        framesize = (uint32_t)(m_buffer[0]) + ((uint32_t)(m_buffer[1])<<8) + ((uint32_t)(m_buffer[2])<<16);
        assert (framesize < (uint32_t) m_maxFrameSize);
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

DecodeStreamInputRaw::DecodeStreamInputRaw()
    : m_lastReadOffset(0)
    , m_availableData(0)
{
}

DecodeStreamInputRaw::~DecodeStreamInputRaw()
{
}

bool DecodeStreamInputRaw::init()
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

bool DecodeStreamInputRaw::ensureBufferData()
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

int32_t DecodeStreamInputRaw::scanForStartCode(const uint8_t * data,
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

bool DecodeStreamInputRaw::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
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

DecodeStreamInputH264::DecodeStreamInputH264()
{
    StartCodeSize = 3;
}

DecodeStreamInputH264::~DecodeStreamInputH264()
{

}

const char *DecodeStreamInputH264::getMimeType()
{
    return "video/h264";
}

bool DecodeStreamInputH264::isSyncWord(const uint8_t* buf)
{
    return buf[0] == 0 && buf[1] == 0 && buf[2] == 1;
}

DecodeStreamInputJPEG::DecodeStreamInputJPEG()
{
    StartCodeSize = 2;
    m_countSOI = 0;
}

DecodeStreamInputJPEG::~DecodeStreamInputJPEG()
{

}

const char *DecodeStreamInputJPEG::getMimeType()
{
    return "image/jpeg";
}

bool DecodeStreamInputJPEG::isSyncWord(const uint8_t* buf)
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
