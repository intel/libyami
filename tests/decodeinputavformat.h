/*
 *  decodeinputavformat.h - decode input using avformat
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: XuGuagnxin <Guangxin.Xu@intel.com>
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

#ifndef decodeinputavformat_h
#define decodeinputavformat_h

#include "decodeinput.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class DecodeInputAvFormat : public DecodeInput
{
public:
    DecodeInputAvFormat();
    virtual ~DecodeInputAvFormat();
    virtual bool isEOS() { return m_isEos; }
    virtual const char * getMimeType();
    virtual bool getNextDecodeUnit(VideoDecodeBuffer &inputBuffer);

protected:
    virtual bool initInput(const char* fileName);
private:
    AVFormatContext* m_format;
    int m_videoId;
    AVCodecID m_codecId;
    AVPacket m_packet;
    bool m_isEos;
};

#endif
