/*
 * Copyright 2016 Intel Corporation
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

#ifndef vaapidecoder_vc1_h
#define vaapidecoder_vc1_h

#include "codecparsers/vc1Parser.h"
#include "vaapidecoder_base.h"
#include "va/va.h"

namespace YamiMediaCodec {
class VaapiDecoderVC1 : public VaapiDecoderBase {
public:
    VaapiDecoderVC1();
    virtual ~VaapiDecoderVC1();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual void stop(void);
    virtual void flush(void);
    virtual YamiStatus decode(VideoDecodeBuffer*);

private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderVC1>;
    friend class VaapiDecoderVC1Test;
    int32_t searchStartCode(uint8_t*, uint32_t);
    YamiStatus ensureContext();
    YamiStatus decode(uint8_t*, uint32_t, uint64_t);
    bool ensureSlice(PicturePtr&, void*, int);
    bool ensurePicture(PicturePtr&);
    bool makeBitPlanes(PicturePtr&, VAPictureParameterBufferVC1*);
    YamiParser::VC1::Parser m_parser;
    PicturePtr m_forwardPicture;
    bool m_sliceFlag;
    static const bool s_registered;
};
}
#endif
