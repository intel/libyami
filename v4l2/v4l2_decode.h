/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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
#ifndef v4l2_decode_h
#define v4l2_decode_h

#include <vector>

#include "v4l2_codecbase.h"
#include "VideoDecoderInterface.h"

#if __ENABLE_EGL__
namespace YamiMediaCodec {
    class EglVaapiImage;
}
#endif

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
#if __ENABLE_EGL__
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
    virtual bool recycleInputBuffer(struct v4l2_buffer *qbuf);
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
    uint32_t m_outputBufferCountOnInit;

    // debug
    uint32_t m_outputBufferCountQBuf;
    uint32_t m_outputBufferCountPulse;
    uint32_t m_outputBufferCountGive;
#if __ENABLE_EGL__
    std::vector <SharedPtr<EglVaapiImage> > m_eglVaapiImages;
#endif
};
#endif
