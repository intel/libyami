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
    uint16_t width = 0, height = 0;
    int ret = avformat_open_input(&m_format, fileName, NULL, NULL);
    if (ret)
        goto error;
    uint32_t i;
    for (i = 0; i < m_format->nb_streams; ++i) {
        AVCodecContext* ctx = m_format->streams[i]->codec;
        //VP9: width and height of the IVF header,VP8: width and height is zero
        if (AVMEDIA_TYPE_VIDEO == ctx->codec_type) {
            width = ctx->width;
            height = ctx->height;
            break;
        }
    }

    ret = avformat_find_stream_info(m_format, NULL);
    if (ret < 0)
        goto error;
    for (i = 0; i < m_format->nb_streams; i++)
    {
        AVCodecContext* ctx = m_format->streams[i]->codec;
        if (AVMEDIA_TYPE_VIDEO == ctx->codec_type) {
            m_codecId = ctx->codec_id;
            if (ctx->extradata && ctx->extradata_size)
                m_codecData.append((char*)ctx->extradata, ctx->extradata_size);
            m_videoId = i;
            //VP9: display_width and display_height of the first frame
            if (ctx->coded_width > width)
                width = ctx->coded_width;
            if (ctx->coded_height > height)
                height = ctx->coded_height;
            setResolution(width, height);
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

    AV_CODEC_ID_H264, YAMI_MIME_H264,
    AV_CODEC_ID_H265, YAMI_MIME_H265
};

const char * DecodeInputAvFormat::getMimeType()
{
    for (size_t i = 0; i < N_ELEMENTS(MimeEntrys); i++) {
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
        av_packet_unref(&m_packet);

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
        av_packet_unref(&m_packet);
        avformat_close_input(&m_format);
    }

}
