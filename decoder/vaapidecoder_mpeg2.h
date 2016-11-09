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
    typedef SharedPtr<VaapiDecPictureMpeg2> PicturePtr;
    VaapiDecoderMPEG2();
    virtual ~VaapiDecoderMPEG2();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual YamiStatus reset(VideoConfigBuffer*);
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
            : m_numberSurfaces(kMaxRefPictures)
            , m_outputPicture(callback)
        {
        }

        void flush();
        bool isEmpty() { return m_referencePictures.empty(); }
        YamiStatus insertPicture(const PicturePtr& picture);
        YamiStatus insertPictureToReferences(const PicturePtr& picture);

        YamiStatus getReferencePictures(const PicturePtr& current_picture,
            PicturePtr& previous_picture,
            PicturePtr& next_picture);
        YamiStatus callOutputPicture(const PicturePtr& picture)
        {
            return m_outputPicture(picture);
        }

        YamiStatus outputPreviousPictures(const PicturePtr& picture,
            bool empty = false);

    private:
        // set to minimum number of surfaces required to operate
        uint32_t m_numberSurfaces;
        std::list<PicturePtr> m_referencePictures;
        OutputCallback m_outputPicture;
    };

    // mpeg2 DPB class

    typedef SharedPtr<YamiParser::MPEG2::Parser> ParserPtr;
    typedef SharedPtr<YamiParser::MPEG2::StreamHeader> StreamHdrPtr;

    friend class FactoryTest<IVideoDecoder, VaapiDecoderMPEG2>;
    friend class VaapiDecoderMPEG2Test;

    void fillSliceParams(VASliceParameterBufferMPEG2* slice_param,
                         const YamiParser::MPEG2::Slice* slice);
    void fillPictureParams(VAPictureParameterBufferMPEG2* param,
                           const PicturePtr& picture);

    YamiStatus fillConfigBuffer();
    YamiStatus
    convertToVAProfile(const YamiParser::MPEG2::ProfileType& profile);
    YamiStatus checkLevel(const YamiParser::MPEG2::LevelType& level);
    bool isSliceCode(YamiParser::MPEG2::StartCodeType next_code);

    YamiStatus processConfigBuffer();
    YamiStatus processDecodeBuffer();

    YamiStatus preDecode(StreamHdrPtr shdr);
    YamiStatus processSlice();
    YamiStatus decodeGOP();
    YamiStatus assignSurface();
    YamiStatus assignPicture();
    YamiStatus createPicture();
    YamiStatus loadIQMatrix();
    bool updateIQMatrix(const YamiParser::MPEG2::QuantMatrices* refIQMatrix,
                        bool reset = false);
    YamiStatus decodePicture();
    YamiStatus outputPicture(const PicturePtr& picture);
    YamiStatus findReusePicture(std::list<PicturePtr>& list, bool& reuse);

    ParserPtr m_parser;
    StreamHdrPtr m_stream;

    const YamiParser::MPEG2::SeqHeader* m_sequenceHeader;
    const YamiParser::MPEG2::SeqExtension* m_sequenceExtension;
    const YamiParser::MPEG2::GOPHeader* m_GOPHeader; //check what's the use here
    const YamiParser::MPEG2::PictureHeader* m_pictureHeader;
    const YamiParser::MPEG2::PictureCodingExtension* m_pictureCodingExtension;
    const YamiParser::MPEG2::QuantMatrixExtension* m_quantMatrixExtension;
    DPB m_DPB;
    VASliceParameterBufferMPEG2* mpeg2SliceParams;

    bool m_VAStart;
    bool m_isParsingSlices;
    bool m_loadNewIQMatrix;
    bool m_canCreatePicture;
    PicturePtr m_currentPicture;
    YamiParser::MPEG2::StartCodeType m_previousStartCode;
    YamiParser::MPEG2::StartCodeType m_nextStartCode;
    IQMatricesRefs m_IQMatrices;
    VAProfile m_VAProfile;
    uint64_t m_currentPTS;

    std::list<PicturePtr> m_topFieldPictures;
    std::list<PicturePtr> m_bottomFieldPictures;

    /**
     * VaapiDecoderFactory registration result. This decoder is registered in
     * vaapidecoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec

#endif
