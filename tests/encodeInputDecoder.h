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
#ifndef encodeInputDecoder_h
#define  encodeInputDecoder_h

#include "decodeinput.h"
#include "encodeinput.h"

#include "VideoDecoderHost.h"
#include <map>

using namespace YamiMediaCodec;

class MyRawImage;
class EncodeInputDecoder : public  EncodeInput {
public:
    EncodeInputDecoder(DecodeInput* input);
    ~EncodeInputDecoder();
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
    virtual bool getOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool recycleOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool isEOS();
private:
    bool decodeOneFrame();
    DecodeInput* m_input;
    IVideoDecoder* m_decoder;
    bool m_isEOS;

    typedef std::map<uint32_t, SharedPtr<MyRawImage> > ImageMap;

    ImageMap m_images;
    uint32_t m_id;
    DISALLOW_COPY_AND_ASSIGN(EncodeInputDecoder);
};
#endif
