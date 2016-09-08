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
#ifndef v4l2_encode_h
#define v4l2_encode_h

#include <vector>

#include "v4l2_codecbase.h"
#include "VideoEncoderInterface.h"

using namespace YamiMediaCodec;
typedef SharedPtr < IVideoEncoder > EncoderPtr;

class V4l2Encoder : public V4l2CodecBase
{
  public:
    V4l2Encoder();
     ~V4l2Encoder() {};

    virtual int32_t ioctl(int request, void* arg);
    virtual void* mmap(void* addr, size_t length,
                         int prot, int flags, unsigned int offset);

  protected:
    virtual bool start();
    virtual bool stop();
    virtual bool acceptInputBuffer(struct v4l2_buffer *qbuf);
    virtual bool giveOutputBuffer(struct v4l2_buffer *dqbuf);
    virtual bool inputPulse(uint32_t index);
    virtual bool outputPulse(uint32_t &index);

  private:
    bool UpdateVideoParameters(bool isInputThread=false);
    EncoderPtr m_encoder;

    VideoParamsCommon m_videoParams;
    Lock m_videoParamsLock;
    bool m_videoParamsChanged;

    uint32_t m_maxOutputBufferSize;
    uint8_t *m_outputBufferSpace;

    std::vector<VideoEncRawBuffer> m_inputFrames;
    std::vector<VideoEncOutputBuffer> m_outputFrames;

    bool m_separatedStreamHeader;
    bool m_requestStreamHeader;
    bool m_forceKeyFrame;
};
#endif
