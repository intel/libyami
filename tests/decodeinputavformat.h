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

#ifndef decodeinputavformat_h
#define decodeinputavformat_h

#include "decodeinput.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(54, 51, 100)
    typedef CodecID AVCodecID;
    #define AV_CODEC_ID_NONE   CODEC_ID_NONE
    #define AV_CODEC_ID_VP8    CODEC_ID_VP8
    #define AV_CODEC_ID_H264   CODEC_ID_H264
#endif
}

class DecodeInputAvFormat : public DecodeInput
{
public:
    DecodeInputAvFormat();
    virtual ~DecodeInputAvFormat();
    virtual bool isEOS() { return m_isEos; }
    virtual const char * getMimeType();
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);
    virtual const string& getCodecData();

protected:
    virtual bool initInput(const char* fileName);
private:
    AVFormatContext* m_format;
    int m_videoId;
    AVCodecID m_codecId;
    AVPacket m_packet;
    bool m_isEos;
    string m_codecData;
};

#endif
