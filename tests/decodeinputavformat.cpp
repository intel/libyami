/*
 *  decodeinputavformat.cpp - decode input using avformat
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decodeinputavformat.h"
#include "common/common_def.h"
#include "common/log.h"
#include "interface/VideoDecoderDefs.h"

DecodeInputAvFormat::DecodeInputAvFormat()
:m_format(NULL),m_videoId(-1), m_codecId(AV_CODEC_ID_NONE), m_isEos(true)
{
    av_register_all();

    av_init_packet(&m_packet);
}

bool DecodeInputAvFormat::initInput(const char* fileName)
{
    int ret = avformat_open_input(&m_format, fileName, NULL, NULL);
    if (ret)
        goto error;
    ret = avformat_find_stream_info(m_format, NULL);
    if (ret < 0)
        goto error;
    int i;
    for (i = 0; i < m_format->nb_streams; i++)
    {
        AVCodecContext* ctx = m_format->streams[i]->codec;
        if (AVMEDIA_TYPE_VIDEO == ctx->codec_type) {
            m_codecId = ctx->codec_id;
            if (ctx->extradata && ctx->extradata_size)
                m_codecData.append((char*)ctx->extradata, ctx->extradata_size);
            m_videoId = i;
            break;
        }
    }
    if (i == m_format->nb_streams) {
        ERROR("no video stream");
        goto error;
    }
    m_isEos = false;
    return true;
error:
    if (m_format)
        avformat_close_input(&m_format);
    return false;

}

struct MimeEntry
{
    AVCodecID id;
    const char* mime;
};

static const MimeEntry MimeEntrys[] = {
    AV_CODEC_ID_VP8, YAMI_MIME_VP8,

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(54, 40, 0)
    AV_CODEC_ID_VP9, YAMI_MIME_VP9,
#endif

    AV_CODEC_ID_H264, YAMI_MIME_H264
};

const char * DecodeInputAvFormat::getMimeType()
{
    for (int i = 0; i < N_ELEMENTS(MimeEntrys); i++) {
        if (MimeEntrys[i].id == m_codecId)
            return MimeEntrys[i].mime;
    }
    return "unknow";
}

bool DecodeInputAvFormat::getNextDecodeUnit(VideoDecodeBuffer &inputBuffer)
{
    if (!m_format || m_isEos)
        return false;
    int ret;
    while (1) {
        //free old packet
        av_free_packet(&m_packet);

        ret = av_read_frame(m_format, &m_packet);
        if (ret) {
            m_isEos = true;
            return false;
        }
        if (m_packet.stream_index == m_videoId) {
            memset(&inputBuffer, 0, sizeof(inputBuffer));
            inputBuffer.data = m_packet.data;
            inputBuffer.size = m_packet.size;
            inputBuffer.timeStamp = m_packet.dts;
            return true;
        }
    }
    return false;
}

const string& DecodeInputAvFormat::getCodecData()
{
    return m_codecData;
}

DecodeInputAvFormat::~DecodeInputAvFormat()
{
    if (m_format) {
        av_free_packet(&m_packet);
        avformat_close_input(&m_format);
    }

}
