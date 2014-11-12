/*
 *  v4l2_encode.h
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
#ifndef v4l2_decode_h
#define v4l2_decode_h

#include <tr1/memory>
#include <vector>

#include "v4l2_codecbase.h"
#include "interface/VideoDecoderInterface.h"

using namespace YamiMediaCodec;
typedef std::tr1::shared_ptr < IVideoDecoder > DecoderPtr;

class V4l2Decoder : public V4l2CodecBase
{
  public:
    V4l2Decoder();
     ~V4l2Decoder() {};

    virtual int32_t ioctl(int request, void* arg);
    virtual void* mmap(void* addr, size_t length,
                         int prot, int flags, unsigned int offset);
#if __ENABLE_V4L2_GLX__
    virtual int32_t usePixmap(int buffer_index, Pixmap pixmap);
#else
    virtual int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t buffer_index, void* egl_image);
#endif

  protected:
    virtual bool start();
    virtual bool stop();
    virtual bool acceptInputBuffer(struct v4l2_buffer *qbuf);
    virtual bool giveOutputBuffer(struct v4l2_buffer *dqbuf);
    virtual bool inputPulse(int32_t index);
    virtual bool outputPulse(int32_t &index);
    virtual bool recycleOutputBuffer(int32_t index);

  private:
#if !__ENABLE_V4L2_GLX__
    bool populateOutputFrames(EGLDisplay eglDisplay, EGLContext eglContext);
#endif

  private:
    DecoderPtr m_decoder;
    VideoConfigBuffer m_configBuffer;
    // VideoFormatInfo m_videoFormatInfo;
    // chrome requires m_maxBufferCount[OUTPUT] to be big enough (dpb size + some extra ones), it is correct when we export YUV buffer direcly
    // however, we convert YUV frame to temporary RGBX frame; so the RGBX frame pool is not necessary to be that big.
    uint32_t m_actualOutBufferCount;

    uint32_t m_maxBufferSize[2];
    uint8_t *m_bufferSpace[2];

    std::vector<VideoDecodeBuffer> m_inputFrames;
    std::vector<VideoFrameRawData> m_outputRawFrames;

    uint32_t m_videoWidth;
    uint32_t m_videoHeight;
#if __ENABLE_V4L2_GLX__
    std::vector <Pixmap> m_pixmaps;
#else
    std::vector <EGLImageKHR> m_eglImages;
#endif
};
#endif
