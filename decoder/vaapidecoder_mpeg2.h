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

#ifndef vaapidecoder_mpeg2_h
#define vaapidecoder_mpeg2_h

#include "codecparsers/mpeg2_parser.h"
#include "common/Functional.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"

#include <deque>
#include <list>
#include <va/va.h>
#include <vector>

namespace YamiMediaCodec {

enum Mpeg2PictureStructureType {
    kTopField = 1,
    kBottomField,
    kFramePicture,
};

enum {
    kMaxRefPictures = 2,
    kMinSurfaces = 8,
};

struct IQMatricesRefs {
    IQMatricesRefs();

    const uint8_t* intra_quantiser_matrix;
    const uint8_t* non_intra_quantiser_matrix;
    const uint8_t* chroma_intra_quantiser_matrix;
    const uint8_t* chroma_non_intra_quantiser_matrix;
};

class VaapiDecPictureMpeg2;

class VaapiDecoderMPEG2 : public VaapiDecoderBase {
public:
    typedef SharedPtr<VaapiDecPicture> PicturePtr;
    VaapiDecoderMPEG2();
    virtual ~VaapiDecoderMPEG2();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual void stop(void);
    virtual void flush(void);
    virtual YamiStatus decode(VideoDecodeBuffer*);

private:
    // mpeg2 DPB class
    class DPB {
        typedef std::function<YamiStatus(const PicturePtr&)>
            OutputCallback;

    public:
        DPB(OutputCallback callback)
            : m_output(callback)
        {
        }
        void add(const PicturePtr& frame);
        bool getReferences(
            const PicturePtr& current,
            PicturePtr& forward, PicturePtr& backward);

        void flush();

        PicturePtr m_firstField;

    private:
        void addNewFrame(const PicturePtr& frame);
        OutputCallback m_output;
        std::deque<PicturePtr> m_refs;
    };

    // mpeg2 DPB class

    typedef SharedPtr<YamiParser::MPEG2::Parser> ParserPtr;

    friend class FactoryTest<IVideoDecoder, VaapiDecoderMPEG2>;
    friend class VaapiDecoderMPEG2Test;

    typedef YamiParser::MPEG2::Slice Slice;
    typedef YamiParser::MPEG2::DecodeUnit DecodeUnit;

    VAProfile
    convertToVAProfile(YamiParser::MPEG2::ProfileType profile);
    YamiStatus checkLevel(YamiParser::MPEG2::LevelType level);

    SurfacePtr createSurface(VaapiPictureType type);
    YamiStatus createPicture();

    YamiStatus ensurePicture();
    YamiStatus ensureSlice(const Slice& slice);
    YamiStatus ensureProfileAndLevel();
    bool ensureMatrices();

    YamiStatus decodeSlice(const YamiParser::MPEG2::DecodeUnit& du);
    YamiStatus decodeCurrent();
    YamiStatus decodeSequnce(const DecodeUnit& du);
    YamiStatus decodeGroup(const DecodeUnit& du);
    YamiStatus decodeExtension(const DecodeUnit& du);
    YamiStatus decodePicture(const DecodeUnit& du);
    YamiStatus decodeNalUnit(const uint8_t* nalData, int32_t nalSize);

    YamiStatus outputPicture(const PicturePtr& picture);

    ParserPtr m_parser;
    SharedPtr<YamiParser::MPEG2::QuantMatrices> m_matrices;

    DPB m_dpb;

    PicturePtr m_current;
    uint64_t m_currentPTS;
    /**
     * VaapiDecoderFactory registration result. This decoder is registered in
     * vaapidecoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec

#endif
