/*
 *  vaapidecoder_h265.h
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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

#ifndef vaapidecoder_h265_h
#define vaapidecoder_h265_h

#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include <tr1/functional>
#include <set>

extern "C" {
typedef struct _H265Parser                   H265Parser;
typedef struct _H265NalUnit                  H265NalUnit;
typedef struct _H265VPS                      H265VPS;
typedef struct _H265SPS                      H265SPS;
typedef struct _H265PPS                      H265PPS;
typedef struct _VASliceParameterBufferHEVC VASliceParameterBufferHEVC;
}


namespace YamiMediaCodec{
enum {
    H265_EXTRA_SURFACE_NUMBER = 5,
};

class VaapiDecPictureH265;
class VaapiDecoderH265:public VaapiDecoderBase {
public:
    typedef SharedPtr<VaapiDecPictureH265> PicturePtr;
    typedef std::vector<VaapiDecPictureH265*> RefSet;
    VaapiDecoderH265();
    virtual ~VaapiDecoderH265();
    virtual Decode_Status start(VideoConfigBuffer* );
    virtual Decode_Status decode(VideoDecodeBuffer*);

private:
    class DPB {
        typedef VaapiDecoderH265::RefSet     RefSet;
        typedef std::tr1::function<Decode_Status (const PicturePtr&)> OutputCallback;
        typedef std::tr1::function<void (const PicturePtr&)> ForEachFunction;
    public:
        typedef VaapiDecoderH265::PicturePtr PicturePtr;
        DPB(OutputCallback output);
        bool init(const PicturePtr&,
                  const H265SliceHdr *const,
                  const H265NalUnit *const,
                  bool newStream);
        bool add(const PicturePtr&, const H265SliceHdr* const lastSlice);
        void flush();

        RefSet m_stCurrBefore;
        RefSet m_stCurrAfter;
        RefSet m_stFoll;
        RefSet m_ltCurr;
        RefSet m_ltFoll;
    private:
        void forEach(ForEachFunction);
        bool initReference(const PicturePtr&,
                           const H265SliceHdr *const,
                           const H265NalUnit *const,
                           bool newStream);
        bool initShortTermRef(const PicturePtr& picture,
                              const H265SliceHdr* const);
        bool initShortTermRef(RefSet& ref,int32_t currPoc,
                              const int32_t* const delta,
                              const uint8_t* const used,
                              uint8_t num);
        bool initLongTermRef(const PicturePtr&,
                             const H265SliceHdr *const);
        VaapiDecPictureH265* getPic(int32_t poc, bool hasMsb = true);

        //for C.5.2.2
        bool checkReorderPics(const H265SPS* const sps);
        bool checkDpbSize(const H265SPS* const);
        bool checkLatency(const H265SPS* const);
        void removeUnused();
        void clearRefSet();

        void bumpAll();
        bool bump();
        bool output(const PicturePtr& picture);


        struct PocLess
        {
            inline bool operator()(const PicturePtr& left, const PicturePtr& right) const;
        };
        typedef std::set<PicturePtr, PocLess> PictureList;
        PictureList     m_pictures;
        OutputCallback  m_output;
        PicturePtr      m_dummy;
    };
    Decode_Status decodeNalu(H265NalUnit*);
    Decode_Status decodeParamSet(H265NalUnit*);
    Decode_Status decodeSlice(H265NalUnit*);

    Decode_Status ensureContext(const H265SPS* const);
    bool fillPicture(const PicturePtr& , const H265SliceHdr* const );
    bool fillSlice(const PicturePtr&, const H265SliceHdr* const, const H265NalUnit* const );
    bool fillIqMatrix(const PicturePtr&, const H265SliceHdr* const);
    bool fillPredWeightTable(VASliceParameterBufferHEVC*, const H265SliceHdr* const);
    bool fillReference(const PicturePtr&,
            VASliceParameterBufferHEVC*, const H265SliceHdr* const);
    void fillReference(VAPictureHEVC* refs, int32_t size);
    void fillReference(VAPictureHEVC* refs, int32_t& n, const RefSet& refset, uint32_t flags);


    bool fillReferenceIndex(VASliceParameterBufferHEVC*, const H265SliceHdr* const);
    void fillReferenceIndexForList(VASliceParameterBufferHEVC*, const RefSet&, bool isList0);
    bool getRefPicList(RefSet& refset, const RefSet& stCurr0, const RefSet& stCurr1,
                       uint8_t numActive, bool modify, const uint32_t* modiList);
    uint8_t getIndex(int32_t poc);


    PicturePtr createPicture(const H265SliceHdr* const, const H265NalUnit* const nalu);
    void getPoc(const PicturePtr&, const H265SliceHdr* const,
            const H265NalUnit* const);
    Decode_Status decodeCurrent();
    Decode_Status outputPicture(const PicturePtr&);

    H265Parser* m_parser;
    PicturePtr  m_current;
    uint16_t    m_prevPicOrderCntMsb;
    int32_t     m_prevPicOrderCntLsb;
    bool        m_newStream;
    DPB         m_dpb;
    std::map<int32_t, uint8_t> m_pocToIndex;
    H265SliceHdr* m_prevSlice;
    H265SliceHdr* m_currSlice;


    static const bool s_registered; // VaapiDecoderFactory registration result
};

};

#endif

