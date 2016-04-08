/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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
#ifndef vppinputdecode_h
#define vppinputdecode_h
#include "VideoDecoderHost.h"
#include "decodeinput.h"

#include "vppinputoutput.h"

class VppInputDecode : public VppInput
{
public:
    VppInputDecode()
        : m_eos(false)
        , m_error(false)
    {
    }
    bool init(const char* inputFileName, uint32_t fourcc = 0, int width = 0, int height = 0);
    bool read(SharedPtr<VideoFrame>& frame);

    bool config(NativeDisplay& nativeDisplay);
    virtual ~VppInputDecode() {}
private:
    bool m_eos;
    bool m_error;
    SharedPtr<IVideoDecoder> m_decoder;
    SharedPtr<DecodeInput>   m_input;
    SharedPtr<VideoFrame>    m_first;
};
#endif //vppinputdecode_h

