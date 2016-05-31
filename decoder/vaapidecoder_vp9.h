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

#ifndef vaapidecoder_vp9_h
#define vaapidecoder_vp9_h

#include "codecparsers/vp9parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va_dec_vp9.h"
#include <vector>

namespace YamiMediaCodec{
enum {
    VP9_EXTRA_SURFACE_NUMBER = 5,
};

class VaapiDecoderVP9:public VaapiDecoderBase {
  public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderVP9();
    virtual ~ VaapiDecoderVP9();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual YamiStatus reset(VideoConfigBuffer*);
    virtual void stop(void);
    virtual void flush(void);
    virtual YamiStatus decode(VideoDecodeBuffer*);

  private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderVP9>;
    friend class VaapiDecoderVP9Test;

    YamiStatus ensureContext(const Vp9FrameHdr*);
    YamiStatus decode(const uint8_t* data, uint32_t size, uint64_t timeStamp);
    YamiStatus decode(const Vp9FrameHdr* hdr, const uint8_t* data, uint32_t size, uint64_t timeStamp);
    bool ensureSlice(const PicturePtr& , const void* data, int size);
    bool ensurePicture(const PicturePtr& , const Vp9FrameHdr* );
    //reference related
    bool fillReference(VADecPictureParameterBufferVP9* , const Vp9FrameHdr*);
    void updateReference(const PicturePtr&, const Vp9FrameHdr*);

    typedef SharedPtr<Vp9Parser> ParserPtr;
    ParserPtr m_parser;
    std::vector<SurfacePtr> m_reference;

    static const bool s_registered; // VaapiDecoderFactory registration result
};

};

#endif
