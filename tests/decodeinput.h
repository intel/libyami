/*
 *  decodeinput.h - decode test input
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

#include <stdio.h>
#include "VideoDecoderDefs.h"
#include "VideoDecoderInterface.h"

class DecodeStreamInput {
public:
    static const int MaxNaluSize = 1024*1024; // assume max nalu size is 1M
    static const int CacheBufferSize = 4 * MaxNaluSize;
    DecodeStreamInput();
    virtual ~DecodeStreamInput();
    static DecodeStreamInput * create(const char* fileName);
    bool initInput(const char* fileName);
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

class DecodeStreamInputVPX :public DecodeStreamInput
{
public:
    DecodeStreamInputVPX();
    ~DecodeStreamInputVPX();
    const char * getMimeType();
    bool init();
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
private:
    const char* m_mimeType;
    const int m_ivfFrmHdrSize;
    const int m_maxFrameSize;
};

class DecodeStreamInputRaw:public DecodeStreamInput
{
public:
    DecodeStreamInputRaw();
    ~DecodeStreamInputRaw();
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

class DecodeStreamInputH264:public DecodeStreamInputRaw
{
public:
    DecodeStreamInputH264();
    ~DecodeStreamInputH264();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
};

class DecodeStreamInputJPEG:public DecodeStreamInputRaw
{
public:
    DecodeStreamInputJPEG();
    ~DecodeStreamInputJPEG();
    const char * getMimeType();
    bool isSyncWord(const uint8_t* buf);
private:
    int m_countSOI;
};
