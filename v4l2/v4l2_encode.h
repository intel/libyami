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
#ifndef v4l2_encode_h
#define v4l2_encode_h

#include <vector>

#include "v4l2_codecbase.h"
#include "interface/VideoEncoderInterface.h"

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
