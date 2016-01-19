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

#include <vector>

#if ANDROID
#include <va/va_android.h>
#endif

#include "v4l2_codecbase.h"
#include "interface/VideoDecoderInterface.h"

namespace YamiMediaCodec {
    class EglVaapiImage;
}

using namespace YamiMediaCodec;
typedef SharedPtr < IVideoDecoder > DecoderPtr;
class V4l2Decoder : public V4l2CodecBase
{
  public:
    V4l2Decoder();
     ~V4l2Decoder();

    virtual int32_t ioctl(int request, void* arg);
    virtual void* mmap(void* addr, size_t length,
                         int prot, int flags, unsigned int offset);
#if ANDROID
    SharedPtr<VideoFrame> createVaSurface(const ANativeWindowBuffer* buf);
    bool mapVideoFrames();
#elif __ENABLE_V4L2_GLX__
    virtual int32_t usePixmap(uint32_t buffer_index, Pixmap pixmap);
#else
    virtual int32_t useEglImage(EGLDisplay eglDisplay, EGLContext eglContext, uint32_t buffer_index, void* egl_image);
#endif

  protected:
    virtual bool start();
    virtual bool stop();
    virtual bool acceptInputBuffer(struct v4l2_buffer *qbuf);
    virtual bool giveOutputBuffer(struct v4l2_buffer *dqbuf);
    virtual bool inputPulse(uint32_t index);
    virtual bool outputPulse(uint32_t &index);
    virtual bool recycleOutputBuffer(int32_t index);
    virtual void releaseCodecLock(bool lockable);
    virtual void flush();

  private:
    DecoderPtr m_decoder;
    std::vector<uint8_t> m_codecData;
    bool m_bindEglImage;
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
#if ANDROID
    std::vector < SharedPtr<VideoFrame> > m_videoFrames;
    std::vector <ANativeWindowBuffer*> m_winBuff;
    uint32_t m_reqBuffCnt;
    gralloc_module_t* m_pGralloc;
#elif __ENABLE_V4L2_GLX__
    std::vector <Pixmap> m_pixmaps;
#else
    std::vector <SharedPtr<EglVaapiImage> > m_eglVaapiImages;
#endif
};
#endif
