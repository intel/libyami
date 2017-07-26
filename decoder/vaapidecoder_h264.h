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

#ifndef vaapidecoder_h264_h
#define vaapidecoder_h264_h

#include "codecparsers/h264Parser.h"
#include "common/Functional.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"

#include <set>

namespace YamiMediaCodec {

#define H264_MAX_REFRENCE_SURFACE_NUMBER 16

class VaapiDecPictureH264;
class VaapiDecoderH264 : public VaapiDecoderBase {
public:
    typedef SharedPtr<VaapiDecPictureH264> PicturePtr;
    typedef std::vector<PicturePtr> RefSet;
    typedef YamiParser::H264::SliceHeader SliceHeader;
    typedef YamiParser::H264::NalUnit NalUnit;
    typedef YamiParser::H264::SPS SPS;

    VaapiDecoderH264();
    virtual ~VaapiDecoderH264();
    virtual YamiStatus start(VideoConfigBuffer*);
    virtual YamiStatus decode(VideoDecodeBuffer*);
    virtual void flush(void);

private:
    friend class FactoryTest<IVideoDecoder, VaapiDecoderH264>;
    friend class VaapiDecoderH264Test;

    class DPB {
        typedef VaapiDecoderH264::RefSet RefSet;
        typedef std::function<YamiStatus(const PicturePtr&)>
            OutputCallback;
        typedef std::function<void(const PicturePtr&)> ForEachFunction;

    public:
        typedef VaapiDecoderH264::PicturePtr PicturePtr;
        struct PocLess {
            inline bool operator()(const PicturePtr& left,
                                   const PicturePtr& right) const;
        };
        typedef std::set<PicturePtr, PocLess> PictureList;

        DPB(OutputCallback output);
        bool init(const PicturePtr&, const PicturePtr&,
                  const SliceHeader* const, const NalUnit* const,
                  bool newStream, bool contextChanged,
                  uint32_t maxDecFrameBuffering);
        bool add(const PicturePtr&);
        bool outputReadyFrames();
        void initReference(const PicturePtr&, const SliceHeader* const);
        void flush();

        RefSet m_refList0;
        RefSet m_refList1;

        PictureList m_pictures;
        bool m_isLowLatencymode;

    private:
        void forEach(ForEachFunction);

        template <class P> void findAndMarkUnusedReference(P);

        void initPSliceRef(const PicturePtr& picture, const SliceHeader* const);

        void initBSliceRef(const PicturePtr& picture, const SliceHeader* const);

        void initReferenceList(const PicturePtr& picture,
                               const SliceHeader* const);

        bool modifyReferenceList(const PicturePtr& picture,
                                 const SliceHeader* const slice,
                                 RefSet& refList, uint8_t refIdx);

        void processFrameNumWithGaps(const PicturePtr& picture,
                                     const SliceHeader* const slice);

        void adaptiveMarkReference(const PicturePtr& picture);

        bool slidingWindowMarkReference(const PicturePtr& picture);
        bool markReference(const PicturePtr& picture);

        bool isFull();
        void removeUnused();
        void clearRefSet();

        bool calcPoc(const PicturePtr&, const SliceHeader* const);
        void calcPicNum(const PicturePtr& picture,
                        const SliceHeader* const slice);

        void bumpAll();
        bool bump();
        bool output(const PicturePtr& picture);
        void printRefList();

        RefSet m_shortTermList;
        RefSet m_shortTermList1; // used to reoder m_shortTermList, then
        // generate m_refList1
        RefSet m_longTermList;

        PicturePtr m_prevPicture;
        OutputCallback m_output;
        PicturePtr m_dummy;
        bool m_noOutputOfPriorPicsFlag;
        uint32_t m_maxFrameNum;
        uint32_t m_maxNumRefFrames;
        uint32_t m_maxDecFrameBuffering;
        YamiParser::H264::DecRefPicMarking m_decRefPicMarking;
        bool m_isOutputStarted;
        int32_t m_lastOutputPoc;
    };

    YamiStatus decodeNalu(NalUnit*);
    YamiStatus decodeSps(NalUnit*);
    YamiStatus decodePps(NalUnit*);
    YamiStatus decodeSlice(NalUnit*);

    YamiStatus ensureContext(const SharedPtr<SPS>& sps);
    bool fillPicture(const PicturePtr&, const SliceHeader* const);
    bool fillSlice(const PicturePtr&, const SliceHeader* const,
                   const NalUnit* const);
    bool fillIqMatrix(const PicturePtr&, const SliceHeader* const);
    bool fillPredWeightTable(VASliceParameterBufferH264*,
                             const SliceHeader* const);
    void fillReference(VAPictureH264* refs, size_t size);

    bool fillReferenceIndex(VASliceParameterBufferH264*,
                            const SliceHeader* const);
    void fillReferenceIndexForList(VASliceParameterBufferH264* sliceParam,
                                   const SliceHeader* const slice,
                                   RefSet& refSet, bool isList0);
    bool isDecodeContextChanged(const SharedPtr<SPS>& sps);
    bool decodeAvcRecordData(uint8_t* buf, int32_t bufSize);

    YamiStatus createPicture(const SliceHeader* const,
        const NalUnit* const nalu);
    YamiStatus decodeCurrent();
    YamiStatus outputPicture(const PicturePtr&);
    SurfacePtr createSurface(const SliceHeader* const);

    YamiParser::H264::Parser m_parser;
    PicturePtr m_currPic;
    PicturePtr m_prevPic;
    bool m_newStream;
    bool m_endOfSequence;
    bool m_endOfStream;
    DPB m_dpb;
    uint32_t m_nalLengthSize;
    SurfacePtr m_currSurface;
    bool m_contextChanged;

    /**
     * VaapiDecoderFactory registration result. This decoder is registered in
     * vaapidecoder_host.cpp
     */
    static const bool s_registered;
};
} // namespace YamiMediaCodec

#endif
