/*
 *  v4l2_encode.cpp
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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
#include <linux/videodev2.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "v4l2_encode.h"
#include "interface/VideoEncoderHost.h"
#include "common/log.h"

V4l2Encoder::V4l2Encoder()
    : m_videoParamsChanged(false)
    , m_maxOutputBufferSize(0)
    , m_outputBufferSpace(NULL)
    , m_separatedStreamHeader(false)
    , m_requestStreamHeader(true)
    , m_forceKeyFrame(false)
{
    m_memoryMode[INPUT] = V4L2_MEMORY_USERPTR; // dma_buf hasn't been supported yet
    m_pixelFormat[INPUT] = V4L2_PIX_FMT_YUV420M;
    m_bufferPlaneCount[INPUT] = 3; // decided by m_pixelFormat[INPUT]
    m_memoryMode[OUTPUT] = V4L2_MEMORY_MMAP;
    m_pixelFormat[OUTPUT] = V4L2_PIX_FMT_H264;
    m_bufferPlaneCount[OUTPUT] = 1;
    m_maxBufferCount[INPUT] = 3;
    m_maxBufferCount[OUTPUT] = 3;

    m_inputFrames.resize(m_maxBufferCount[INPUT]);
    m_outputFrames.resize(m_maxBufferCount[OUTPUT]);
}

bool V4l2Encoder::start()
{
    Encode_Status status = ENCODE_SUCCESS;
    ASSERT(m_encoder);

    VideoConfigAVCStreamFormat streamFormat;
    streamFormat.size = sizeof(VideoConfigAVCStreamFormat);
    streamFormat.streamFormat = AVC_STREAM_FORMAT_ANNEXB;
    m_encoder->setParameters(VideoConfigTypeAVCStreamFormat, &streamFormat);

    status = m_encoder->setParameters(VideoParamsTypeCommon, &m_videoParams);
    ASSERT(status == ENCODE_SUCCESS);

    NativeDisplay nativeDisplay;
    nativeDisplay.type = NATIVE_DISPLAY_DRM;
    nativeDisplay.handle = 0;
    m_encoder->setNativeDisplay(&nativeDisplay);

    status = m_encoder->start();
    ASSERT(status == ENCODE_SUCCESS);
    status = m_encoder->getParameters(VideoParamsTypeCommon, &m_videoParams);
    ASSERT(status == ENCODE_SUCCESS);

    return true;
}

bool V4l2Encoder::stop()
{
    Encode_Status encodeStatus = ENCODE_SUCCESS;
    if (m_encoder)
        encodeStatus = m_encoder->stop();
    return encodeStatus == ENCODE_SUCCESS;
}


bool V4l2Encoder::UpdateVideoParameters(bool isInputThread)
{
    Encode_Status status = ENCODE_SUCCESS;
    AutoLock locker(m_videoParamsLock); // make sure the caller has released m_videoParamsLock

    if (!m_videoParamsChanged)
        return true;

    if (isInputThread || !m_streamOn[INPUT]) {
        status = m_encoder->setParameters(VideoParamsTypeCommon, &m_videoParams);
        ASSERT(status == ENCODE_SUCCESS);
        m_videoParamsChanged = false;
    }

    return status == ENCODE_SUCCESS;
}
bool V4l2Encoder::inputPulse(uint32_t index)
{
    Encode_Status status = ENCODE_SUCCESS;

    if(m_videoParamsChanged )
        UpdateVideoParameters(true);

    status = m_encoder->encode(&m_inputFrames[index]);

    if (status != ENCODE_SUCCESS)
        return false;

    ASSERT(m_inputFrames[index].bufAvailable); // check it at a later time when yami does encode in async
    return true;
}

bool V4l2Encoder::outputPulse(uint32_t &index)
{
    Encode_Status status = ENCODE_SUCCESS;

    VideoEncOutputBuffer *outputBuffer = &(m_outputFrames[index]);
    if (m_separatedStreamHeader) {
        outputBuffer->format = OUTPUT_FRAME_DATA;
        if (m_requestStreamHeader) {
            outputBuffer->format = OUTPUT_CODEC_DATA;
        }
    } else
        outputBuffer->format = OUTPUT_EVERYTHING;

    status = m_encoder->getOutput(outputBuffer, false);

    if (status != ENCODE_SUCCESS)
        return false;

    ASSERT(m_maxOutputBufferSize > 0); // update m_maxOutputBufferSize after VIDIOC_S_FMT
    ASSERT(m_outputBufferSpace);
    ASSERT(outputBuffer->dataSize <= m_maxOutputBufferSize);

    if (m_separatedStreamHeader) {
        if (m_requestStreamHeader)
            m_requestStreamHeader = false;
    }

    return true;
}

bool V4l2Encoder::acceptInputBuffer(struct v4l2_buffer *qbuf)
{
    uint32_t i;
    VideoEncRawBuffer *inputBuffer = &(m_inputFrames[qbuf->index]);
    // XXX todo: add multiple planes support for yami
    inputBuffer->data = reinterpret_cast<uint8_t*>(qbuf->m.planes[0].m.userptr);
    inputBuffer->size = 0;
    for (i=0; i<qbuf->length; i++) {
       inputBuffer->size += qbuf->m.planes[i].bytesused;
    }
    inputBuffer->bufAvailable = false;
    DEBUG("qbuf->index: %d, inputBuffer: %p, bufAvailable: %d", qbuf->index, inputBuffer, inputBuffer->bufAvailable);
    inputBuffer->timeStamp = qbuf->timestamp.tv_sec * 1000000 + qbuf->timestamp.tv_usec; // XXX
    if (m_forceKeyFrame) {
        inputBuffer->forceKeyFrame = true;
        m_forceKeyFrame = false;
    }
    switch(m_pixelFormat[INPUT]) {
    case V4L2_PIX_FMT_YUV420M:
        inputBuffer->fourcc = VA_FOURCC('I', '4', '2', '0');
        break;
    case V4L2_PIX_FMT_YUYV:
        inputBuffer->fourcc = VA_FOURCC_YUY2;
        break;
    default:
        ASSERT(0);
        break;
    }

    return true;
}

bool V4l2Encoder::giveOutputBuffer(struct v4l2_buffer *dqbuf)
{
    ASSERT(dqbuf->index>=0 && dqbuf->index<m_maxBufferCount[OUTPUT]);
    VideoEncOutputBuffer *outputBuffer = &(m_outputFrames[dqbuf->index]);
    dqbuf->m.planes[0].bytesused = outputBuffer->dataSize;
    dqbuf->bytesused = m_outputFrames[dqbuf->index].dataSize;
    dqbuf->m.planes[0].m.mem_offset = 0;
    ASSERT(m_maxOutputBufferSize > 0);
    ASSERT(m_outputBufferSpace);
    if (outputBuffer->flag & ENCODE_BUFFERFLAG_SYNCFRAME)
        dqbuf->flags = V4L2_BUF_FLAG_KEYFRAME;

    return true;
}

int32_t V4l2Encoder::ioctl(int command, void* arg)
{
    Encode_Status encodeStatus = ENCODE_SUCCESS;
    int32_t ret = 0;

    DEBUG("fd: %d, ioctl command: %s", m_fd[0], IoctlCommandString(command));
    switch (command) {
    case VIDIOC_QUERYCAP:
    case VIDIOC_STREAMON:
    case VIDIOC_STREAMOFF:
    case VIDIOC_REQBUFS:
    case VIDIOC_QBUF:
    case VIDIOC_DQBUF:
        ret = V4l2CodecBase::ioctl(command, arg);
        break;
    case VIDIOC_QUERYBUF: {
        struct v4l2_buffer *buffer = static_cast<struct v4l2_buffer*>(arg);
        ASSERT (buffer->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
        ASSERT(buffer->memory == V4L2_MEMORY_MMAP);
        ASSERT(buffer->index>=0 && buffer->index<m_maxBufferCount[OUTPUT]);
        ASSERT(buffer->length == m_bufferPlaneCount[OUTPUT]);
        ASSERT(m_maxOutputBufferSize > 0);

        buffer->m.planes[0].length = m_maxOutputBufferSize;
        buffer->m.planes[0].m.mem_offset = m_maxOutputBufferSize * buffer->index;
    }
    break;
    case VIDIOC_S_EXT_CTRLS: {
        uint32_t i;
        struct v4l2_ext_controls *control = static_cast<struct v4l2_ext_controls *>(arg);
        DEBUG("V4L2_CTRL_CLASS_MPEG: %d, control->ctrl_class: %d", V4L2_CTRL_CLASS_MPEG, control->ctrl_class);
        if (control->ctrl_class == V4L2_CTRL_CLASS_MPEG) {
            AutoLock locker(m_videoParamsLock);
            struct v4l2_ext_control *ctrls = control->controls;
            DEBUG("control->count: %d", control->count);
            for (i=0; i<control->count; i++) {
                DEBUG("VIDIOC_S_EXT_CTRLS:V4L2_CTRL_CLASS_MPEG:%d", ctrls->id);
                switch (ctrls->id) {
                    case V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE:
                        // ::EncodeTask
                        ASSERT(ctrls->value == V4L2_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE_I_FRAME);
                        m_forceKeyFrame = true;
                        break;
                    case V4L2_CID_MPEG_VIDEO_BITRATE: {
                        // ::RequestEncodingParametersChangeTask
                        m_videoParams.rcParams.bitRate = ctrls->value;
                        m_videoParamsChanged = true;
                    }
                        break;
                    case V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE:
                        INFO("enable bitrate control");
                        m_videoParams.rcMode = RATE_CONTROL_CBR;
                        m_videoParamsChanged = true;
                        break;
                    case V4L2_CID_MPEG_VIDEO_HEADER_MODE:
                        // Separate stream header so we can cache it and insert into the stream.
                        if (ctrls->value == V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE)
                            m_separatedStreamHeader = true;
                        INFO("use separated stream header: %d", m_separatedStreamHeader);
                        break;
                    case V4L2_CID_MPEG_VIDEO_B_FRAMES:
                    case V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF:
                    case V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT:
                    case V4L2_CID_MPEG_VIDEO_H264_MAX_QP:
                    case V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE:
                    case V4L2_CID_MPEG_VIDEO_H264_LEVEL:
                    case V4L2_CID_MPEG_VIDEO_GOP_SIZE:
                    default:
                        break;
                }
                ctrls++;
            }
        }
        UpdateVideoParameters();
    }
    break;
    case VIDIOC_S_PARM: {
        struct v4l2_streamparm *parms = static_cast<struct v4l2_streamparm *>(arg);
        if (parms->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            AutoLock locker(m_videoParamsLock);
            // ::RequestEncodingParametersChangeTask
            m_videoParams.frameRate.frameRateNum = parms->parm.output.timeperframe.denominator;
            m_videoParams.frameRate.frameRateDenom = parms->parm.output.timeperframe.numerator;
            int framerate = m_videoParams.frameRate.frameRateNum/m_videoParams.frameRate.frameRateDenom;
            if (framerate * 2 < m_videoParams.intraPeriod) {
                m_videoParams.intraPeriod = framerate * 2;
            }
            m_videoParamsChanged = true;
        }
        UpdateVideoParameters();
    }
    break;
    case VIDIOC_S_FMT: {
        struct v4l2_format *format = static_cast<struct v4l2_format *>(arg);
        ASSERT(!m_streamOn[INPUT] && !m_streamOn[OUTPUT]);
        if (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            // ::SetOutputFormat
            switch (format->fmt.pix_mp.pixelformat) {
                case V4L2_PIX_FMT_H264: {
                    m_encoder.reset(createVideoEncoder(YAMI_MIME_H264), releaseVideoEncoder);
                    if (!m_encoder) {
                        ret = -1;
                        break;
                    }
                    m_videoParams.size = sizeof(m_videoParams);
                    encodeStatus = m_encoder->getParameters(VideoParamsTypeCommon, &m_videoParams);
                    ASSERT(encodeStatus == ENCODE_SUCCESS);
                    m_videoParams.profile = VAProfileH264Main;
                    encodeStatus = m_encoder->setParameters(VideoParamsTypeCommon, &m_videoParams);
                    ASSERT(encodeStatus == ENCODE_SUCCESS);
                }
                break;
                case V4L2_PIX_FMT_VP8:
                default:
                    ret = -1;
                break;
            }

        } else if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
            // ::NegotiateInputFormat
            ASSERT(format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_YUV420M || format->fmt.pix_mp.pixelformat == V4L2_PIX_FMT_YUYV);
            m_pixelFormat[INPUT] = format->fmt.pix_mp.pixelformat;
            switch (m_pixelFormat[INPUT]) {
            case V4L2_PIX_FMT_YUV420M:
                m_bufferPlaneCount[INPUT] = 3;
            break;
            case V4L2_PIX_FMT_YUYV:
                m_bufferPlaneCount[INPUT] = 1;
                break;
            default:
                ASSERT(0);
                break;
            }
            ASSERT(m_encoder);
            m_videoParams.resolution.width = format->fmt.pix_mp.width;
            m_videoParams.resolution.height= format->fmt.pix_mp.height;
            encodeStatus = m_encoder->setParameters(VideoParamsTypeCommon, &m_videoParams);
            ASSERT(encodeStatus == ENCODE_SUCCESS);
            encodeStatus = m_encoder->getMaxOutSize(&m_maxOutputBufferSize);
            ASSERT(encodeStatus == ENCODE_SUCCESS);
            INFO("resolution: %d x %d, m_maxOutputBufferSize: %d", m_videoParams.resolution.width,
                m_videoParams.resolution.height, m_maxOutputBufferSize);
            format->fmt.pix_mp.plane_fmt[0].bytesperline = m_videoParams.resolution.width;
            format->fmt.pix_mp.plane_fmt[0].sizeimage = m_videoParams.resolution.width * m_videoParams.resolution.height;
            format->fmt.pix_mp.plane_fmt[1].bytesperline = m_videoParams.resolution.width/2;
            format->fmt.pix_mp.plane_fmt[1].sizeimage = m_videoParams.resolution.width * m_videoParams.resolution.height /4;
            format->fmt.pix_mp.plane_fmt[2].bytesperline = m_videoParams.resolution.width/2;
            format->fmt.pix_mp.plane_fmt[2].sizeimage = m_videoParams.resolution.width * m_videoParams.resolution.height /4;
        } else {
            ret = -1;
            ERROR("unknow type: %d of setting format VIDIOC_S_FMT", format->type);
        }
    }
    break;
    case VIDIOC_S_CROP: {
        // ::SetFormats
        //struct v4l2_crop *crop = static_cast<struct v4l2_crop *>(arg);
        INFO("ignore crop for now (the difference between buffer size and real size)");
    }
    break;
    default:
        ret = -1;
        ERROR("unknown ioctrl command: %d", command);
    break;
    }

    if (ret == -1 && errno != EAGAIN) {
        ERROR("ioctl failed");
        WARNING("ioctl command: %s failed", IoctlCommandString(command));
    }

    return ret;
}


void* V4l2Encoder::mmap (void* addr, size_t length,
                      int prot, int flags, unsigned int offset)
{
    uint32_t i;
    ASSERT((prot == PROT_READ) | PROT_WRITE);
    ASSERT(flags == MAP_SHARED);

    ASSERT(m_maxOutputBufferSize > 0);
    ASSERT(length <= m_maxOutputBufferSize);
    if (!m_outputBufferSpace) {
        m_outputBufferSpace = static_cast<uint8_t*>(malloc(m_maxOutputBufferSize * m_maxBufferCount[OUTPUT]));
        for (i=0; i<m_maxBufferCount[OUTPUT]; i++) {
            m_outputFrames[i].data = m_outputBufferSpace + m_maxOutputBufferSize*i;
            m_outputFrames[i].bufferSize = m_maxOutputBufferSize;
            m_outputFrames[i].format = OUTPUT_EVERYTHING;
        }
    }
    ASSERT(m_outputBufferSpace);

    return m_outputBufferSpace + offset;
}

