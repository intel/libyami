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

#ifndef vaapiDecoderJPEG_h
#define vaapiDecoderJPEG_h

// library headers
#include "vaapidecpicture.h"
#include "vaapidecoder_base.h"

// system headers
#include <memory>

namespace YamiMediaCodec {

class VaapiDecoderJPEG
    : public VaapiDecoderBase {
public:
    VaapiDecoderJPEG();

    virtual ~VaapiDecoderJPEG() { }

    virtual Decode_Status start(VideoConfigBuffer*);
    virtual Decode_Status reset(VideoConfigBuffer*);
    virtual Decode_Status decode(VideoDecodeBuffer*);

private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderJPEG>;
    friend class VaapiDecoderJPEGTest;
    class Impl;

    Decode_Status fillPictureParam();
    Decode_Status fillSliceParam();

    Decode_Status loadQuantizationTables();
    Decode_Status loadHuffmanTables();

    Decode_Status finish();

    std::auto_ptr<VaapiDecoderJPEG::Impl> m_impl;
    PicturePtr m_picture;

    static const bool s_registered; // VaapiDecoderFactory registration result

    DISALLOW_COPY_AND_ASSIGN(VaapiDecoderJPEG);
};

} // namespace YamiMediaCodec

#endif // vaapiDecoderJPEG_h
