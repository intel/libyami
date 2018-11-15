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
#include "common/Functional.h"
#include "va/va.h"
#include "vaapidecoder_base.h"
#include <deque>

namespace YamiMediaCodec {

class VaapiDecoderVC1 : public VaapiDecoderBase {
public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderVC1();
    virtual ~VaapiDecoderVC1();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual void stop(void);
    virtual void flush(void);
    virtual YamiStatus decode(VideoDecodeBuffer*);

private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderVC1>;
    friend class VaapiDecoderVC1Test;
    YamiStatus ensureContext();

    YamiStatus decodeFrame(PicturePtr& picture,
        const uint8_t* data, uint32_t size, uint64_t pts);
    YamiStatus decodeField(const PicturePtr& picture,
        const uint8_t* data, uint32_t size);
    YamiStatus decodeSlice(const PicturePtr& picture,
        const uint8_t* data, uint32_t size);

    YamiStatus ensureSlice(const PicturePtr&, const uint8_t* data, uint32_t size,
        uint32_t mbOffset, uint32_t sliceAddr = 0);
    bool ensurePicture(PicturePtr&);
    bool makeBitPlanes(PicturePtr&, VAPictureParameterBufferVC1*);
    YamiStatus outputPicture(const PicturePtr&);
    YamiParser::VC1::Parser m_parser;

    const static uint32_t VC1_MAX_REFRENCE_SURFACE_NUMBER = 2;

    class DPB {
        typedef std::function<YamiStatus(const PicturePtr&)> OutputCallback;

    public:
        typedef VaapiDecoderVC1::PicturePtr PicturePtr;

        std::deque<PicturePtr> m_refs;
        DPB(const OutputCallback& output);
        void add(const PicturePtr&, YamiParser::VC1::FrameHdr&);
        void flush();
        bool notEnoughReference(YamiParser::VC1::FrameHdr&);

    private:
        OutputCallback m_output;
    };

    DPB m_dpb;

    /**
     * VaapiDecoderFactory registration result. This decoder is registered in
     * vaapidecoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec
#endif
