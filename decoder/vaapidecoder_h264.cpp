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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vaapidecoder_h264.h"

#include "common/log.h"
#include "common/nalreader.h"

#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapidecpicture.h"

#include <algorithm>

namespace YamiMediaCodec {
typedef VaapiDecoderH264::PicturePtr PicturePtr;
using std::bind;
using std::placeholders::_1;
using std::ref;
using std::vector;
using namespace YamiParser::H264;

class VaapiDecPictureH264 : public VaapiDecPicture {
public:
    VaapiDecPictureH264(const ContextPtr& context, const SurfacePtr& surface,
                        int64_t timeStamp)
        : VaapiDecPicture(context, surface, timeStamp)
        , m_idrFlag(false)
        , m_picStructure(VAAPI_PICTURE_FRAME)
        , m_longTermRefFlag(false)
        , m_shortTermRefFlag(false)
        , m_topFieldOrderCnt(0)
        , m_bottomFieldOrderCnt(0)
        , m_pocMsb(0)
        , m_pocLsb(0)
        , m_poc(0)
        , m_frameNumOffset(0)
        , m_frameNum(0)
        , m_frameNumWrap(0)
        , m_picNum(0)
        , m_longTermFrameIdx(0)
        , m_longTermPicNum(0)
        , m_picOutputFlag(true)
        , m_isReference(false)
        , m_hasMmco5(false)
        , m_isSecondField(false)
    {
    }
    VaapiDecPictureH264() {}

    PicturePtr allocPicture()
    {
        PicturePtr picture(
            new VaapiDecPictureH264(m_context, m_surface, m_timeStamp));
        assert(picture);
        return picture;
    }

    PicturePtr allocField(bool isBottomField)
    {
        PicturePtr field = allocPicture();
        *field = *this;

        field->m_picStructure = VAAPI_PICTURE_TOP_FIELD;
        field->m_poc = this->m_topFieldOrderCnt;

        if (isBottomField) {
            field->m_picStructure = VAAPI_PICTURE_BOTTOM_FIELD;
            field->m_poc = this->m_bottomFieldOrderCnt;
        }

        return field;
    }

    PicturePtr allocDummyPicture(int32_t frameNum)
    {
        PicturePtr picture = allocPicture();
        *picture = *this;

        picture->m_picStructure = VAAPI_PICTURE_FRAME;
        picture->m_frameNum = frameNum;
        picture->m_picOutputFlag = false;
        picture->m_idrFlag = false;
        picture->m_poc = 0x7fffffff;

        picture->m_isReference = true;
        picture->m_longTermRefFlag = false;
        picture->m_shortTermRefFlag = true;

        return picture;
    }

    bool m_idrFlag;
    VaapiPictureType m_picStructure;
    bool m_longTermRefFlag;
    bool m_shortTermRefFlag;
    int32_t m_topFieldOrderCnt;
    int32_t m_bottomFieldOrderCnt;
    int32_t m_pocMsb;
    uint16_t m_pocLsb;
    int32_t m_poc;
    int32_t m_frameNumOffset;
    int32_t m_frameNum;
    int32_t m_frameNumWrap;
    int32_t m_picNum;
    int32_t m_longTermFrameIdx;
    int32_t m_longTermPicNum;
    bool m_picOutputFlag;
    bool m_isReference;
    bool m_hasMmco5;
    bool m_isSecondField;
    PicturePtr m_complementField;
};

static bool isISlice(uint32_t sliceType)
{
    return (IS_I_SLICE(sliceType) || IS_SI_SLICE(sliceType));
}

static bool isPSlice(uint32_t sliceType)
{
    return (IS_P_SLICE(sliceType) || IS_SP_SLICE(sliceType));
}

static bool isBSlice(uint32_t sliceType) { return (IS_B_SLICE(sliceType)); }

static bool isIdr(const PicturePtr& picture) { return picture->m_idrFlag; }

static bool isFrame(const PicturePtr& picture)
{
    return picture->m_picStructure == VAAPI_PICTURE_FRAME;
}

static bool isField(const PicturePtr& picture)
{
    return picture->m_picStructure != VAAPI_PICTURE_FRAME;
}

static bool isTopField(const PicturePtr& picture)
{
    return picture->m_picStructure == VAAPI_PICTURE_TOP_FIELD;
}

static bool isBottomField(const PicturePtr& picture)
{
    return picture->m_picStructure == VAAPI_PICTURE_BOTTOM_FIELD;
}

static bool isSecondField(const PicturePtr& picture)
{
    return picture->m_isSecondField;
}

inline bool isOutputNeeded(const PicturePtr& picture)
{
    return picture->m_picOutputFlag;
}

inline bool isReference(const PicturePtr& picture)
{
    return picture->m_isReference;
}

inline bool isUnusedPicture(const PicturePtr& picture)
{
    return !isReference(picture) && !isOutputNeeded(picture);
}

inline bool isShortTermReference(const PicturePtr& picture)
{
    return picture->m_shortTermRefFlag && picture->m_isReference;
}

inline bool isLongTermReference(const PicturePtr& picture)
{
    return picture->m_longTermRefFlag && picture->m_isReference;
}

void markUnusedReference(const PicturePtr& picture)
{
    picture->m_isReference = false;
    DEBUG("mark pic(poc: %d frameNumWrap: %d) as unused ref", picture->m_poc,
          picture->m_frameNumWrap);
}

void markLongTermReference(const PicturePtr& picture)
{
    picture->m_isReference = true;
    picture->m_longTermRefFlag = true;
    picture->m_shortTermRefFlag = false;

    DEBUG("mark pic(poc: %d frameNumWrap: %d) as LongTerm ref", picture->m_poc,
          picture->m_frameNumWrap);
}

void markUnusedLongTermRefWithMaxIndex(const PicturePtr& picture,
                                       int32_t maxLongTermFrameIdx)
{
    if (isLongTermReference(picture)
        && (picture->m_longTermFrameIdx > maxLongTermFrameIdx))
        markUnusedReference(picture);
}

inline bool findComplementaryField(const PicturePtr& picture, int32_t frameNum,
                                   VaapiPictureType picStructure)
{
    return ((picture->m_frameNum == frameNum)
            && (picture->m_picStructure + picStructure
                == VAAPI_PICTURE_FRAME));
}

inline bool matchPocLsb(const PicturePtr& picture, int32_t poc)
{
    return picture->m_pocLsb == poc;
}

inline bool matchPicNum(const PicturePtr& picture, int32_t picNum)
{
    return picture->m_picNum == picNum;
}

inline bool matchFrameNumInShortTermList(const PicturePtr& picture,
                                         int32_t frameNum)
{
    return (picture->m_frameNum == frameNum && picture->m_shortTermRefFlag
            && picture->m_isReference);
}

inline bool matchShortTermFrameNumWrap(const PicturePtr& picture,
                                       int32_t frameNumWrap)
{
    return (isShortTermReference(picture)
            && picture->m_frameNumWrap == frameNumWrap);
}

inline bool matchShortTermPicNum(const PicturePtr& picture, int32_t picNum)
{
    return (isShortTermReference(picture) && picture->m_picNum == picNum);
}

inline bool matchLongTermPicNum(const PicturePtr& picture,
                                int32_t longTermPicNum)
{
    return (isLongTermReference(picture)
            && picture->m_longTermPicNum == longTermPicNum);
}

inline bool matchPicStructure(const PicturePtr& picture1,
                              const PicturePtr& picture2)
{
    return picture1->m_picStructure == picture2->m_picStructure;
}

inline bool ascCompareFrameNumWrap(const PicturePtr& picture1,
                                   const PicturePtr& picture2)
{
    return picture1->m_frameNumWrap < picture2->m_frameNumWrap;
}

inline bool decCompareFrameNumWrap(const PicturePtr& picture1,
                                   const PicturePtr& picture2)
{
    return picture1->m_frameNumWrap > picture2->m_frameNumWrap;
}

inline bool decCompareStPicNum(const PicturePtr& picture1,
                               const PicturePtr& picture2)
{
    return picture1->m_picNum > picture2->m_picNum;
}

inline bool ascCompareLtPicNum(const PicturePtr& picture1,
                               const PicturePtr& picture2)
{
    return picture1->m_longTermPicNum < picture2->m_longTermPicNum;
}

inline bool ascCompareLtFrameIndex(const PicturePtr& picture1,
                                   const PicturePtr& picture2)
{
    return picture1->m_longTermFrameIdx < picture2->m_longTermFrameIdx;
}

inline bool decComparePoc(const PicturePtr& picture1,
                          const PicturePtr& picture2)
{
    return picture1->m_poc > picture2->m_poc;
}

inline bool ascComparePoc(const PicturePtr& picture1,
                          const PicturePtr& picture2)
{
    return picture1->m_poc < picture2->m_poc;
}

inline bool VaapiDecoderH264::DPB::PocLess::
operator()(const PicturePtr& left, const PicturePtr& right) const
{
    return left->m_poc <= right->m_poc;
}

bool checkMMCO5(DecRefPicMarking decRefPicMarking)
{
    for (uint32_t i = 0; i < decRefPicMarking.n_ref_pic_marking; i++) {
        if (5
            == decRefPicMarking.ref_pic_marking[i]
                   .memory_management_control_operation)
            return true;
    }

    return false;
}

VaapiDecoderH264::DPB::DPB(OutputCallback output)
    : m_output(output)
    , m_dummy(new VaapiDecPictureH264)
    , m_noOutputOfPriorPicsFlag(false)
    , m_maxFrameNum(0)
    , m_maxNumRefFrames(0)
    , m_maxDecFrameBuffering(H264_MAX_REFRENCE_SURFACE_NUMBER)
{
}

void VaapiDecoderH264::DPB::removeUnused()
{
    /* Remove unused pictures from DPB */
    PictureList::iterator it;
    for (it = m_pictures.begin(); it != m_pictures.end();) {
        if (isUnusedPicture(*it))
            m_pictures.erase(it++);
        else
            ++it;
    }
}

void VaapiDecoderH264::DPB::clearRefSet()
{
    m_shortTermList.clear();
    m_shortTermList1.clear();
    m_longTermList.clear();
    m_refList0.clear();
    m_refList1.clear();
}

void VaapiDecoderH264::DPB::printRefList()
{
    uint32_t i;

    for (i = 0; i < m_shortTermList.size(); i++) {
        DEBUG("m_shortTermList(Index %d: Poc %d, frameNumWrap: %d)", i,
              m_shortTermList[i]->m_poc, m_shortTermList[i]->m_frameNumWrap);
    }

    for (i = 0; i < m_shortTermList1.size(); i++) {
        DEBUG("m_shortTermList1(Index %d: Poc %d, frameNumWrap: %d)", i,
              m_shortTermList1[i]->m_poc, m_shortTermList1[i]->m_frameNumWrap);
    }

    for (i = 0; i < m_longTermList.size(); i++) {
        DEBUG("m_longTermList(Index %d: Poc %d, longTermFrameIdx %d)", i,
              m_longTermList[i]->m_poc, m_longTermList[i]->m_longTermFrameIdx);
    }

    for (i = 0; i < m_refList0.size(); i++) {
        DEBUG("m_refList0(Index %d: Poc %d)", i, m_refList0[i]->m_poc);
    }

    for (i = 0; i < m_refList1.size(); i++) {
        DEBUG("m_refList1(Index %d: Poc %d)", i, m_refList1[i]->m_poc);
    }

    PictureList::iterator it;

    for (i = 0, it = m_pictures.begin(); it != m_pictures.end(); it++, i++) {
        DEBUG("m_pictures(Index %d: Poc %d, frameNumWrap: %d, "
              "shortTermRefFlag:%d, longTermRefFlag:%d, isRefer: %d, "
              "outPutFlag:%d)",
              i, (*it)->m_poc, (*it)->m_frameNumWrap, (*it)->m_shortTermRefFlag,
              (*it)->m_longTermRefFlag, (*it)->m_isReference,
              (*it)->m_picOutputFlag);
    }
}

static void calcShortTermPicNum(vector<PicturePtr>& shortRefSet,
                                const PicturePtr& picture,
                                PicturePtr& refPicture, uint32_t maxFrameNum)
{
    if (refPicture->m_frameNum > picture->m_frameNum)
        refPicture->m_frameNumWrap = refPicture->m_frameNum - maxFrameNum;
    else
        refPicture->m_frameNumWrap = refPicture->m_frameNum;

    if (isFrame(refPicture))
        refPicture->m_picNum = refPicture->m_frameNumWrap;
    else if (matchPicStructure(picture, refPicture))
        refPicture->m_picNum = 2 * refPicture->m_frameNumWrap + 1;
    else
        refPicture->m_picNum = 2 * refPicture->m_frameNumWrap;

    if (isField(picture) && isFrame(refPicture)) {
        shortRefSet.push_back(refPicture->allocField(false));
        shortRefSet.push_back(refPicture->allocField(true));
    } else
        shortRefSet.push_back(refPicture);
}

static void calcLongTermPicNum(vector<PicturePtr>& longRefSet,
                               const PicturePtr& picture,
                               PicturePtr& refPicture)
{
    if (isFrame(refPicture))
        refPicture->m_longTermPicNum = refPicture->m_longTermFrameIdx;
    else if (matchPicStructure(picture, refPicture))
        refPicture->m_longTermPicNum = 2 * refPicture->m_longTermFrameIdx + 1;
    else
        refPicture->m_longTermPicNum = 2 * refPicture->m_longTermFrameIdx;

    if (isField(picture) && isFrame(refPicture)) {
        longRefSet.push_back(refPicture->allocField(false));
        longRefSet.push_back(refPicture->allocField(true));
    } else
        longRefSet.push_back(refPicture);
}

void VaapiDecoderH264::DPB::calcPicNum(const PicturePtr& picture,
                                       const SliceHeader* const slice)
{
    PictureList::iterator it;
    PicturePtr refPicture;

    m_shortTermList.clear();
    m_longTermList.clear();

    picture->m_picNum = isFrame(picture) ? picture->m_frameNum
                                         : 2 * picture->m_frameNum + 1;

    for (it = m_pictures.begin(); it != m_pictures.end(); it++) {
        refPicture = *it;
        if (isShortTermReference(refPicture)) {
            calcShortTermPicNum(m_shortTermList, picture, refPicture,
                                m_maxFrameNum);
        } else if (isLongTermReference(refPicture))
            calcLongTermPicNum(m_longTermList, picture, refPicture);
    }
}

void partitionAndInterleave(const PicturePtr& picture,
                            vector<PicturePtr>& refSet)
{
    vector<PicturePtr> refSet1, refSet2;
    vector<PicturePtr>::iterator it;
    uint32_t i, n;

    // refset has been sorted, use stable_partition to keep the sequence.
    it = std::stable_partition(refSet.begin(), refSet.end(),
                               bind(matchPicStructure, _1, picture));
    refSet1.insert(refSet1.end(), refSet.begin(), it);
    refSet2.insert(refSet2.end(), it, refSet.end());
    refSet.clear();

    for (i = 0; i < refSet1.size(); i++) {
        DEBUG("refSet1(Index %d: Poc %d, frameNumWrap: %d)", i,
              refSet1[i]->m_poc, refSet1[i]->m_frameNumWrap);
    }

    for (i = 0; i < refSet2.size(); i++) {
        DEBUG("refSet2(Index %d: Poc %d, frameNumWrap: %d)", i,
              refSet2[i]->m_poc, refSet2[i]->m_frameNumWrap);
    }

    n = MIN(refSet1.size(), refSet2.size());

    for (i = 0; i < n; i++) {
        refSet.push_back(refSet1[i]);
        refSet.push_back(refSet2[i]);
    }

    if (n < refSet1.size())
        refSet.insert(refSet.end(), refSet1.begin() + n, refSet1.end());
    else if (n < refSet2.size())
        refSet.insert(refSet.end(), refSet2.begin() + n, refSet2.end());
}

void VaapiDecoderH264::DPB::initReferenceList(const PicturePtr& picture,
                                              const SliceHeader* const slice)
{

    if (isISlice(slice->slice_type))
        return;

    if (isField(picture)) {
        partitionAndInterleave(picture, m_shortTermList);
        partitionAndInterleave(picture, m_longTermList);
    }
    m_refList0.insert(m_refList0.end(), m_shortTermList.begin(),
                      m_shortTermList.end());
    m_refList0.insert(m_refList0.end(), m_longTermList.begin(),
                      m_longTermList.end());

    if (isBSlice(slice->slice_type)) {
        if (isField(picture))
            partitionAndInterleave(picture, m_shortTermList1);

        m_refList1.insert(m_refList1.end(), m_shortTermList1.begin(),
                          m_shortTermList1.end());
        m_refList1.insert(m_refList1.end(), m_longTermList.begin(),
                          m_longTermList.end());
    }
}

void VaapiDecoderH264::DPB::initPSliceRef(const PicturePtr& picture,
                                          const SliceHeader* const slice)
{
    std::sort(m_shortTermList.begin(), m_shortTermList.end(),
              isFrame(picture) ? decCompareStPicNum : decCompareFrameNumWrap);
    std::sort(m_longTermList.begin(), m_longTermList.end(),
              isFrame(picture) ? ascCompareLtPicNum : ascCompareLtFrameIndex);

    initReferenceList(picture, slice);
}

void VaapiDecoderH264::DPB::initBSliceRef(const PicturePtr& picture,
                                          const SliceHeader* const slice)
{
    RefSet::iterator it;

    // Reflist0 init
    // For short term reflist:  descending sort for all m_poc less than curPic,
    // ascending sort for the rest.
    // For long term reflist: ascending sort as m_longTermPicNum or
    // m_longTermFrameIdx;
    std::sort(m_shortTermList.begin(), m_shortTermList.end(), ascComparePoc);
    it = std::partition(m_shortTermList.begin(), m_shortTermList.end(),
                        bind(ascComparePoc, _1, picture));
    std::sort(m_shortTermList.begin(), it, decComparePoc);
    std::sort(m_longTermList.begin(), m_longTermList.end(),
              isFrame(picture) ? ascCompareLtPicNum : ascCompareLtFrameIndex);

    // Reflist1 init
    // For short term reflist: swap to reflist0. For long term reflist: some as
    // reflist0
    m_shortTermList1.insert(m_shortTermList1.end(), it, m_shortTermList.end());
    m_shortTermList1.insert(m_shortTermList1.end(), m_shortTermList.begin(),
                            it);

    initReferenceList(picture, slice);

    if ((m_refList1.size() > 1) && (m_refList0 == m_refList1))
        std::swap(m_refList1[0], m_refList1[1]);
}

void VaapiDecoderH264::DPB::initReference(const PicturePtr& picture,
                                          const SliceHeader* const slice)
{
    clearRefSet();
    if (isIdr(picture))
        return;

    m_decRefPicMarking = slice->dec_ref_pic_marking;

    calcPicNum(picture, slice);

    if (isPSlice(slice->slice_type))
        initPSliceRef(picture, slice);
    else if (isBSlice(slice->slice_type))
        initBSliceRef(picture, slice);

    if (!isISlice(slice->slice_type))
        modifyReferenceList(picture, slice, m_refList0, 0);

    if (isBSlice(slice->slice_type))
        modifyReferenceList(picture, slice, m_refList1, 1);

    if (m_refList0.size() > (uint32_t)(slice->num_ref_idx_l0_active_minus1 + 1))
        m_refList0.resize(slice->num_ref_idx_l0_active_minus1 + 1);
    if (m_refList1.size() > (uint32_t)(slice->num_ref_idx_l1_active_minus1 + 1))
        m_refList1.resize(slice->num_ref_idx_l1_active_minus1 + 1);
}

bool VaapiDecoderH264::DPB::modifyReferenceList(const PicturePtr& picture,
                                                const SliceHeader* const slice,
                                                RefSet& refList, uint8_t refIdx)
{
    uint32_t refPicListModifyFlag, refPicListModifyNum;
    const RefPicListModification* refPicListModify;

    if (refIdx == 0) {
        refPicListModifyFlag = slice->ref_pic_list_modification_flag_l0;
        refPicListModifyNum = slice->n_ref_pic_list_modification_l0;
        refPicListModify = slice->ref_pic_list_modification_l0;
    } else if (refIdx == 1) {
        refPicListModifyFlag = slice->ref_pic_list_modification_flag_l1;
        refPicListModifyNum = slice->n_ref_pic_list_modification_l1;
        refPicListModify = slice->ref_pic_list_modification_l1;
    } else {
        assert(0);
    }

    if (!refPicListModifyFlag)
        return true;

    DEBUG("modifyReferenceList, refIdx(%d), refPicListModifyNum(%d)", refIdx,
          refPicListModifyNum);

    int32_t maxPicNum = isFrame(picture) ? m_maxFrameNum : 2 * m_maxFrameNum;
    int32_t picNumLxPred, picNumLxNoWrap, picNumLx, picNumF, absDiffPicNum;
    uint32_t refIdxLx = 0, nIdx, cIdx;
    RefSet::iterator it;

    picNumLxPred = picture->m_picNum;

    for (uint32_t i = 0; i < refPicListModifyNum; i++) {
        switch (refPicListModify[i].modification_of_pic_nums_idc) {
        case 0:
        case 1:
            // reorder short term reflist
            absDiffPicNum = refPicListModify[i].abs_diff_pic_num_minus1 + 1;
            //(8-34) and (8-35)
            if (refPicListModify[i].modification_of_pic_nums_idc == 0) {
                if (picNumLxPred - absDiffPicNum < 0)
                    picNumLxNoWrap = picNumLxPred - absDiffPicNum + maxPicNum;
                else
                    picNumLxNoWrap = picNumLxPred - absDiffPicNum;
            } else {
                if (picNumLxPred + absDiffPicNum >= maxPicNum)
                    picNumLxNoWrap = picNumLxPred + absDiffPicNum - maxPicNum;
                else
                    picNumLxNoWrap = picNumLxPred + absDiffPicNum;
            }
            picNumLxPred = picNumLxNoWrap;
            //(8-36)
            if (picNumLxNoWrap > picture->m_picNum)
                picNumLx = picNumLxNoWrap - maxPicNum;
            else
                picNumLx = picNumLxNoWrap;

            it = find_if(m_shortTermList.begin(), m_shortTermList.end(),
                         bind(matchPicNum, _1, picNumLx));

            if (it != m_shortTermList.end()) {
                refList.insert(refList.begin() + refIdxLx, *it);
                DEBUG(" Insert refList( Poc %d, PicNum %d)", (*it)->m_poc,
                      (*it)->m_picNum);
            } else
                WARNING("can't find this picture");

            nIdx = ++refIdxLx;
            for (cIdx = refIdxLx; cIdx < refList.size(); cIdx++) {
                picNumF = isShortTermReference(refList[cIdx])
                              ? refList[cIdx]->m_picNum
                              : maxPicNum;
                if (picNumF != picNumLx)
                    refList[nIdx++] = refList[cIdx];
            }
            break;

        case 2:
            // reoder long term reflist
            picNumLx = refPicListModify[i].long_term_pic_num;
            it = find_if(m_longTermList.begin(), m_longTermList.end(),
                         bind(matchLongTermPicNum, _1, picNumLx));

            if (it != m_longTermList.end())
                refList.insert(refList.begin() + refIdxLx, *it);
            else
                WARNING("can't find this picture");

            nIdx = ++refIdxLx;
            for (cIdx = refIdxLx; cIdx < refList.size(); cIdx++) {
                if (!isLongTermReference(refList[cIdx])
                    || (refList[cIdx]->m_longTermPicNum) != picNumLx)
                    refList[nIdx++] = refList[cIdx];
            }
            break;
        default:
            break;
        }
    }

    return true;
}

void VaapiDecoderH264::DPB::adaptiveMarkReference(const PicturePtr& picture)
{
    uint32_t i, mmco, picNumX;
    int32_t maxLongTermFrameIdx;
    PictureList::iterator it;
    for (i = 0; i < m_decRefPicMarking.n_ref_pic_marking; i++) {
        RefPicMarking refPicMarking = m_decRefPicMarking.ref_pic_marking[i];
        mmco = refPicMarking.memory_management_control_operation;
        picNumX = picture->m_picNum
                  - (refPicMarking.difference_of_pic_nums_minus1 + 1); // (8-39)
        DEBUG("mmco type: %d", mmco);
        switch (mmco) {
        case 1:
            findAndMarkUnusedReference(bind(matchShortTermPicNum, _1, picNumX));
            break;
        case 2:
            findAndMarkUnusedReference(
                bind(matchLongTermPicNum, _1, refPicMarking.long_term_pic_num));
            break;
        case 3:
            findAndMarkUnusedReference(bind(matchLongTermPicNum, _1,
                                            refPicMarking.long_term_frame_idx));
            it = find_if(m_pictures.begin(), m_pictures.end(),
                         bind(matchShortTermPicNum, _1, picNumX));
            if (it != m_pictures.end()) {
                markLongTermReference(*it);
                (*it)->m_longTermFrameIdx = refPicMarking.long_term_frame_idx;
            }
            break;
        case 4:
            maxLongTermFrameIdx = refPicMarking.max_long_term_frame_idx_plus1
                                  - 1;
            forEach(bind(markUnusedLongTermRefWithMaxIndex, _1,
                         maxLongTermFrameIdx));
            break;
        case 5:
            forEach(markUnusedReference);
            break;
        case 6:
            findAndMarkUnusedReference(bind(matchLongTermPicNum, _1,
                                            refPicMarking.long_term_frame_idx));
            markLongTermReference(picture);
            picture->m_longTermFrameIdx = refPicMarking.long_term_frame_idx;
            break;
        default:
            DEBUG("mmco type undefined!");
            break;
        }
    }
}

bool
VaapiDecoderH264::DPB::slidingWindowMarkReference(const PicturePtr& picture)
{
    uint32_t numShortTerm, numLongTerm;

    if (isSecondField(picture))
        return true;

    numShortTerm = m_shortTermList.size();
    numLongTerm = m_longTermList.size();

    if (numShortTerm && (numShortTerm + numLongTerm >= m_maxNumRefFrames)) {
        RefSet::iterator it1
            = std::min_element(m_shortTermList.begin(), m_shortTermList.end(),
                               ascCompareFrameNumWrap);
        PicturePtr unUsedRefPic = *it1;
        DEBUG("Find unUsedRefPic (poc %d, FrameNumWrap: %d, isReference: %d)",
              unUsedRefPic->m_poc, unUsedRefPic->m_frameNumWrap,
              unUsedRefPic->m_isReference);
        /*for field case, m_shortTermList comes from m_fields, set unUsedRefPic
          won't effect m_pictures.
          So we need one more step to mark unUsedRefPic in m_pictures */
        if (isField(picture)) {
            PictureList::iterator it2 = find_if(
                m_pictures.begin(), m_pictures.end(),
                bind(matchShortTermFrameNumWrap, _1, (*it1)->m_frameNumWrap));
            if (it2 == m_pictures.end()) {
                ERROR("can't find picture");
                return false;
            }
            unUsedRefPic = *it2;
        }
        markUnusedReference(unUsedRefPic);
    }

    return true;
}

bool VaapiDecoderH264::DPB::markReference(const PicturePtr& picture)
{
    if (!picture->m_isReference)
        return false;

    if (m_decRefPicMarking.adaptive_ref_pic_marking_mode_flag) {
        DEBUG("Adaptive ref pic marking right now");
        adaptiveMarkReference(picture);
    } else {
        slidingWindowMarkReference(picture);
    }

    return true;
}

void VaapiDecoderH264::DPB::forEach(ForEachFunction fn)
{
    std::for_each(m_pictures.begin(), m_pictures.end(), fn);
}

template <class P>
void VaapiDecoderH264::DPB::findAndMarkUnusedReference(P pred)
{
    PictureList::iterator it
        = find_if(m_pictures.begin(), m_pictures.end(), pred);

    if (it != m_pictures.end())
        markUnusedReference(*it);
}

bool VaapiDecoderH264::DPB::isFull()
{
    DEBUG("m_pictures size: %zu", m_pictures.size());
    return m_pictures.size() >= m_maxDecFrameBuffering;
}

/* 8.3.1 */
bool VaapiDecoderH264::DPB::calcPoc(const PicturePtr& picture,
                                    const SliceHeader* const slice)
{
    const SharedPtr<PPS> pps = slice->m_pps;
    const SharedPtr<SPS> sps = pps->m_sps;

    const int32_t maxPicOrderCntLsb
        = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    picture->m_frameNumOffset
        = m_prevPicture->m_frameNumOffset
          + (picture->m_frameNum < m_prevPicture->m_frameNum ? m_maxFrameNum
                                                             : 0);

    if (sps->pic_order_cnt_type == 0) {
        if (picture->m_pocLsb < m_prevPicture->m_pocLsb
            && m_prevPicture->m_pocLsb - picture->m_pocLsb
               >= (maxPicOrderCntLsb / 2))
            picture->m_pocMsb = m_prevPicture->m_pocMsb + maxPicOrderCntLsb;
        else if (picture->m_pocLsb > m_prevPicture->m_pocLsb
                 && picture->m_pocLsb - m_prevPicture->m_pocLsb
                    > (maxPicOrderCntLsb / 2))
            picture->m_pocMsb = m_prevPicture->m_pocMsb - maxPicOrderCntLsb;
        else
            picture->m_pocMsb = m_prevPicture->m_pocMsb;

        picture->m_topFieldOrderCnt = picture->m_pocMsb + picture->m_pocLsb;
        picture->m_bottomFieldOrderCnt
            = picture->m_topFieldOrderCnt
              + (isFrame(picture) ? slice->delta_pic_order_cnt_bottom : 0);
    } else if (sps->pic_order_cnt_type == 1) {
        uint32_t absFrameNum = 0, expectedPicOrderCnt = 0,
                 expectedDeltaPerPicOrderCntCycle = 0;
        uint32_t i, picOrderCntCycleCnt, frameNumInPicOrderCntCycle;

        /*(8-6) ~ (8-10) */
        if (sps->num_ref_frames_in_pic_order_cnt_cycle)
            absFrameNum = picture->m_frameNumOffset + picture->m_frameNum;
        if (!picture->m_isReference && absFrameNum)
            absFrameNum--;

        if (absFrameNum) {
            picOrderCntCycleCnt = (absFrameNum - 1)
                                  / sps->num_ref_frames_in_pic_order_cnt_cycle;
            frameNumInPicOrderCntCycle
                = (absFrameNum - 1)
                  % sps->num_ref_frames_in_pic_order_cnt_cycle;

            for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
                expectedDeltaPerPicOrderCntCycle
                    += sps->offset_for_ref_frame[i]; //(7-11)

            expectedPicOrderCnt = picOrderCntCycleCnt
                                  * expectedDeltaPerPicOrderCntCycle;

            for (i = 0; i <= frameNumInPicOrderCntCycle; i++)
                expectedPicOrderCnt += sps->offset_for_ref_frame[i];
        } else
            expectedPicOrderCnt = 0;

        if (!picture->m_isReference)
            expectedPicOrderCnt += sps->offset_for_non_ref_pic;

        picture->m_topFieldOrderCnt = expectedPicOrderCnt
                                      + slice->delta_pic_order_cnt[0];
        picture->m_bottomFieldOrderCnt
            = picture->m_topFieldOrderCnt + sps->offset_for_top_to_bottom_field
              + (isFrame(picture) ? slice->delta_pic_order_cnt[1] : 0);
    } else if (sps->pic_order_cnt_type == 2) {
        uint32_t tempPicOrderCnt;
        //(8-12)
        if (isIdr(picture))
            tempPicOrderCnt = 0;
        else if (!isReference(picture))
            tempPicOrderCnt
                = 2 * (picture->m_frameNumOffset + picture->m_frameNum) - 1;
        else
            tempPicOrderCnt
                = 2 * (picture->m_frameNumOffset + picture->m_frameNum);

        picture->m_topFieldOrderCnt = picture->m_bottomFieldOrderCnt
            = tempPicOrderCnt;
    } else {
        ERROR("incorrect poc type!");
        return false;
    }

    if (isBottomField(picture))
        picture->m_poc = picture->m_bottomFieldOrderCnt;
    else
        picture->m_poc = picture->m_topFieldOrderCnt;

    DEBUG("slice_type: %d, poc_type: %d, m_poc:%d, pocLsb:%d, pocMsb:%d, "
          "isbottomfield: %d ",
          slice->slice_type % 5, sps->pic_order_cnt_type, picture->m_poc,
          picture->m_pocLsb, picture->m_pocMsb, slice->bottom_field_flag);

    return true;
}

void
VaapiDecoderH264::DPB::processFrameNumWithGaps(const PicturePtr& picture,
                                               const SliceHeader* const slice)
{
    const SharedPtr<SPS> sps = slice->m_pps->m_sps;

    /*Inserted dummy picture won't be referred when pic_order_cnt_type is 0 ?*/
    if (!sps->pic_order_cnt_type) {
        WARNING("Process Gaps In FrameNum: pic_order_cnt_type is zero");
    }

    uint32_t unusedShortTermFrameNum, prevFrameNum, frameNum;

    SliceHeader tmpSlice = *slice;
    tmpSlice.delta_pic_order_cnt[0] = tmpSlice.delta_pic_order_cnt[1] = 0;
    /* alway use sliding window mark for dummy picture*/
    tmpSlice.dec_ref_pic_marking.adaptive_ref_pic_marking_mode_flag = 0;
    m_decRefPicMarking = tmpSlice.dec_ref_pic_marking;

    prevFrameNum = m_prevPicture->m_frameNum;
    frameNum = picture->m_frameNum;
    unusedShortTermFrameNum = (prevFrameNum + 1) % m_maxFrameNum; /*(7-23) */
    while (unusedShortTermFrameNum != frameNum) {
        printRefList();
        DEBUG("Alloc dummy picture, unusedShortTermFrameNum: %d",
              unusedShortTermFrameNum);
        PicturePtr dummyPic
            = m_prevPicture->allocDummyPicture(unusedShortTermFrameNum);
        calcPoc(dummyPic, &tmpSlice);
        calcPicNum(dummyPic, &tmpSlice);
        PictureList::iterator it = find_if(
            m_pictures.begin(), m_pictures.end(),
            bind(matchFrameNumInShortTermList, _1, unusedShortTermFrameNum));
        if (it == m_pictures.end()) {
            DEBUG("Add dummy picture");
            add(dummyPic);
        }

        unusedShortTermFrameNum = (unusedShortTermFrameNum + 1) % m_maxFrameNum;
        m_prevPicture = dummyPic;
    }
}

// C.5.2.2
bool VaapiDecoderH264::DPB::init(const PicturePtr& picture,
                                 const PicturePtr& prevPicture,
                                 const SliceHeader* const slice,
                                 const NalUnit* const nalu, bool newStream,
                                 bool contextChanged,
                                 uint32_t maxDecFrameBuffering)
{
    const SharedPtr<PPS> pps = slice->m_pps;
    const SharedPtr<SPS> sps = pps->m_sps;

    m_prevPicture = prevPicture;
    m_maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    m_decRefPicMarking = slice->dec_ref_pic_marking;
    m_maxNumRefFrames = MAX(sps->num_ref_frames, 1);
    m_maxDecFrameBuffering = maxDecFrameBuffering;
    if (isField(picture))
        m_maxNumRefFrames *= 2;

    // must double check mmco5 cases
    if (picture->m_hasMmco5)
        WARNING("MMCO5");

    if (isIdr(picture)) {
        if (!newStream && contextChanged)
            m_noOutputOfPriorPicsFlag = true;
        else
            m_noOutputOfPriorPicsFlag
                = m_decRefPicMarking.no_output_of_prior_pics_flag;
    }

    /*8.2.5.2  Decoding process for gaps in frame_num */
    if (sps->gaps_in_frame_num_value_allowed_flag
        && picture->m_frameNum != m_prevPicture->m_frameNum
        && picture->m_frameNum
           != (int32_t)((m_prevPicture->m_frameNum + 1) % m_maxFrameNum)) {
        DEBUG("processForGapsInFrameNum, m_frameNum %d, prevFrameNum: %d",
              picture->m_frameNum, m_prevPicture->m_frameNum);
        processFrameNumWithGaps(picture, slice);
    }

    if (!calcPoc(picture, slice))
        return false;

    return true;
}

bool VaapiDecoderH264::DPB::output(const PicturePtr& picture)
{
    picture->m_picOutputFlag = false;

    DEBUG("DPB: output picture(Poc:%d)", picture->m_poc);
    return m_output(picture) == YAMI_SUCCESS;
}

bool VaapiDecoderH264::DPB::bump()
{
    PictureList::iterator it
        = find_if(m_pictures.begin(), m_pictures.end(), isOutputNeeded);
    if (it == m_pictures.end())
        return false;
    bool success = output(*it);
    if (!isReference(*it))
        m_pictures.erase(it);
    return success;
}

void VaapiDecoderH264::DPB::bumpAll()
{
    while (bump()) {
        /* nothing */;
    }
}

void resetPictureHasMmco5(const PicturePtr& picture)
{
    picture->m_topFieldOrderCnt -= picture->m_poc;
    picture->m_bottomFieldOrderCnt -= picture->m_poc;
    picture->m_poc = 0;
    picture->m_frameNum = 0;
    picture->m_frameNumOffset = 0;
    picture->m_pocMsb = 0;
    picture->m_pocLsb = isBottomField(picture) ? 0 : picture->m_pocLsb;
}

bool VaapiDecoderH264::DPB::add(const PicturePtr& picture)
{
    PictureList::iterator it = m_pictures.begin(); // picture with minimum poc

    /*(8.2.1)*/
    if (picture->m_hasMmco5)
        resetPictureHasMmco5(picture);

    // C4.4: removal of pictures from the DPB before possible insertion of the
    // current picture
    if (isIdr(picture)) {
        forEach(markUnusedReference);

        if (m_noOutputOfPriorPicsFlag)
            m_pictures.clear();
    } else
        markReference(picture);

    removeUnused();
    printRefList();

    if (picture->m_hasMmco5 || (isIdr(picture) && !m_noOutputOfPriorPicsFlag)) {
        DEBUG("noOutputOfPriorPicsFlag: %d", m_noOutputOfPriorPicsFlag);
        bumpAll();
        m_pictures.clear();
    }

    if (!picture->m_isReference && isFull() && picture->m_poc < (*it)->m_poc) {
        DEBUG("Derectly output picture(Poc:%d)", picture->m_poc);
        return output(picture);
    }

    while (isFull()) {
        if (!bump())
            return false;
    }

    if (!isSecondField(picture))
        m_pictures.insert(picture);
    else {
        // since the second field use same surface as the first field, no need
        // to add second filed into DPB buffer.
        PicturePtr compPicture = picture->m_complementField;
        if (isTopField(compPicture))
            compPicture->m_bottomFieldOrderCnt = picture->m_bottomFieldOrderCnt;
        else
            compPicture->m_topFieldOrderCnt = picture->m_topFieldOrderCnt;
        compPicture->m_picStructure = VAAPI_PICTURE_FRAME;
    }

    return true;
}

void VaapiDecoderH264::DPB::flush()
{
    bumpAll();
    clearRefSet();
    m_pictures.clear();
    m_prevPicture.reset();
}

VaapiDecoderH264::VaapiDecoderH264()
    : m_newStream(true)
    , m_endOfSequence(false)
    , m_endOfStream(false)
    , m_dpb(bind(&VaapiDecoderH264::outputPicture, this, _1))
    , m_nalLengthSize(0)
    , m_contextChanged(false)
{
}

VaapiDecoderH264::~VaapiDecoderH264() { stop(); }

bool VaapiDecoderH264::decodeAvcRecordData(uint8_t* buf, int32_t bufSize)
{
    if (buf == NULL || bufSize == 0) {
        ERROR("invalid record data");
        return false;
    }

    if (buf[0] != 1) {
        VideoDecodeBuffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.data = buf;
        buffer.size = bufSize;
        //we ignore no faltal error
        return (decode(&buffer) >= YAMI_SUCCESS);
    }

    if (bufSize < 7) {
        ERROR("invalid avcc record data");
        return false;
    }

    const uint8_t spsNalLengthSize = 2;
    const uint8_t ppsNalLengthSize = 2;
    NalUnit nalu;
    const uint8_t* nalBuf = NULL;
    int32_t i = 0, numOfSps, numOfPps, nalBufSize = 0;

    numOfSps = buf[5] & 0x1f;
    for (NalReader nr(&buf[6], bufSize - 6, spsNalLengthSize); i < numOfSps;
            i++) {
        if (!nr.read(nalBuf, nalBufSize))
            return false;

        if (!nalu.parseNalUnit(nalBuf, nalBufSize))
            return false;

        if (decodeSps(&nalu) != YAMI_SUCCESS)
            return false;
    }
    nalBuf += nalBufSize;
    numOfPps = nalBuf[0] & 0x1f;
    i = 0;
    for (NalReader nr(&nalBuf[1], bufSize - (nalBuf - buf + 1),
                ppsNalLengthSize);
            i < numOfPps; i++) {
        if (!nr.read(nalBuf, nalBufSize))
            return false;

        if (!nalu.parseNalUnit(nalBuf, nalBufSize))
            return false;

        if (decodePps(&nalu) != YAMI_SUCCESS)
            return false;
    }

    m_nalLengthSize = 1 + (buf[4] & 0x3);

    return true;
}

YamiStatus VaapiDecoderH264::start(VideoConfigBuffer* buffer)
{
    if (buffer->data && buffer->size > 0) {
        if (!decodeAvcRecordData(buffer->data, buffer->size)) {
            ERROR("decode record data failed");
            return DECODE_FAIL;
        }
    }


    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderH264::decodeSps(NalUnit* nalu)
{
    SharedPtr<SPS> sps(new SPS());

    memset(sps.get(), 0, sizeof(SPS));
    if (!m_parser.parseSps(sps, nalu)) {
        return YAMI_DECODE_INVALID_DATA;
    }

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderH264::decodePps(NalUnit* nalu)
{
    SharedPtr<PPS> pps(new PPS());

    memset(pps.get(), 0, sizeof(PPS));
    if (!m_parser.parsePps(pps, nalu)) {
        return YAMI_DECODE_INVALID_DATA;
    }

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderH264::outputPicture(const PicturePtr& picture)
{
    VaapiDecoderBase::PicturePtr base
        = std::static_pointer_cast<VaapiDecPicture>(picture);
    return VaapiDecoderBase::outputPicture(base);
}

#define FILL_SCALING_LIST(mxm, n)                                              \
    void fillScalingList##mxm(VAIQMatrixBufferH264* iqMatrix,                  \
                              const SharedPtr<PPS> pps)                        \
    {                                                                          \
        for (uint32_t i = 0; i < N_ELEMENTS(iqMatrix->ScalingList##mxm); i++) {     \
            transform_coefficients_for_frame_macroblocks(                      \
                iqMatrix->ScalingList##mxm[i], pps->scaling_lists_##mxm[i], n, \
                mxm);                                                          \
        }                                                                      \
    }

FILL_SCALING_LIST(4x4, 16)
FILL_SCALING_LIST(8x8, 64)

bool VaapiDecoderH264::fillIqMatrix(const PicturePtr& picture,
                                    const SliceHeader* const slice)
{
    const SharedPtr<PPS> pps = slice->m_pps;

    VAIQMatrixBufferH264* iqMatrix;
    if (!picture->editIqMatrix(iqMatrix))
        return false;

    fillScalingList4x4(iqMatrix, pps);
    fillScalingList8x8(iqMatrix, pps);

    return true;
}

void fillVAPictureH264(VAPictureH264* vaPicH264, const PicturePtr& picture)
{

    vaPicH264->picture_id = picture->getSurfaceID();
    vaPicH264->TopFieldOrderCnt = picture->m_topFieldOrderCnt;
    vaPicH264->BottomFieldOrderCnt = picture->m_bottomFieldOrderCnt;
    vaPicH264->frame_idx = picture->m_frameNum;

    if (picture->m_picStructure == VAAPI_PICTURE_TOP_FIELD) {
        vaPicH264->flags |= VA_PICTURE_H264_TOP_FIELD;
        vaPicH264->BottomFieldOrderCnt = 0;
    }

    if (picture->m_picStructure == VAAPI_PICTURE_BOTTOM_FIELD) {
        vaPicH264->flags |= VA_PICTURE_H264_BOTTOM_FIELD;
        vaPicH264->TopFieldOrderCnt = 0;
    }

    if (picture->m_shortTermRefFlag)
        vaPicH264->flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;

    if (picture->m_longTermRefFlag) {
        vaPicH264->flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;
        vaPicH264->frame_idx = picture->m_longTermFrameIdx;
    }
}

void VaapiDecoderH264::fillReference(VAPictureH264* refs, size_t size)
{
    size_t i = 0;
    DPB::PictureList::iterator it = m_dpb.m_pictures.begin();

    for (; it != m_dpb.m_pictures.end(); it++) {
        if (!isReference(*it))
            continue;
        fillVAPictureH264(&refs[i++], *it);
        DEBUG("id %d, poc %d, isShortRef %d, isRef %d", (*it)->getSurfaceID(),
              (*it)->m_poc, (*it)->m_shortTermRefFlag, (*it)->m_isReference);
    }

    for (; i < size; i++) {
        refs[i].picture_id = VA_INVALID_SURFACE;
        refs[i].TopFieldOrderCnt = 0;
        refs[i].BottomFieldOrderCnt = 0;
        refs[i].flags = VA_PICTURE_H264_INVALID;
        refs[i].frame_idx = 0;
    }
}

bool VaapiDecoderH264::fillPicture(const PicturePtr& picture,
                                   const SliceHeader* const slice)
{
    VAPictureParameterBufferH264* picParam;
    const SharedPtr<PPS> pps = slice->m_pps;
    const SharedPtr<SPS> sps = pps->m_sps;
    if (!picture->editPicture(picParam))
        return false;

    fillVAPictureH264(&picParam->CurrPic, picture);
    fillReference(picParam->ReferenceFrames,
                  N_ELEMENTS(picParam->ReferenceFrames));

    picParam->picture_width_in_mbs_minus1 = (sps->m_width + 15) / 16 - 1;
    picParam->picture_height_in_mbs_minus1 = (sps->m_height + 15) / 16 - 1;
    picParam->bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
    picParam->bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
    picParam->num_ref_frames = sps->num_ref_frames;

    picParam->seq_fields.bits.chroma_format_idc = sps->chroma_format_idc;
    picParam->seq_fields.bits.residual_colour_transform_flag = 0;
    picParam->seq_fields.bits.gaps_in_frame_num_value_allowed_flag
        = sps->gaps_in_frame_num_value_allowed_flag;
    picParam->seq_fields.bits.frame_mbs_only_flag = sps->frame_mbs_only_flag;
    picParam->seq_fields.bits.mb_adaptive_frame_field_flag
        = sps->mb_adaptive_frame_field_flag;
    picParam->seq_fields.bits.direct_8x8_inference_flag
        = sps->direct_8x8_inference_flag;
    picParam->seq_fields.bits.MinLumaBiPredSize8x8
        = (sps->profile_idc == PROFILE_MAIN || sps->profile_idc == PROFILE_HIGH)
          && (sps->level_idc >= 31);
    picParam->seq_fields.bits.log2_max_frame_num_minus4
        = sps->log2_max_frame_num_minus4;
    picParam->seq_fields.bits.pic_order_cnt_type = sps->pic_order_cnt_type;
    picParam->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4
        = sps->log2_max_pic_order_cnt_lsb_minus4;
    picParam->seq_fields.bits.delta_pic_order_always_zero_flag
        = sps->delta_pic_order_always_zero_flag;

    picParam->num_slice_groups_minus1 = pps->num_slice_groups_minus1;
    picParam->slice_group_map_type = pps->slice_group_map_type;
    picParam->slice_group_change_rate_minus1
        = pps->slice_group_change_rate_minus1;

    picParam->pic_init_qp_minus26 = pps->pic_init_qp_minus26;
    picParam->pic_init_qs_minus26 = pps->pic_init_qs_minus26;
    picParam->chroma_qp_index_offset = pps->chroma_qp_index_offset;
    picParam->second_chroma_qp_index_offset
        = pps->second_chroma_qp_index_offset;

    picParam->pic_fields.bits.entropy_coding_mode_flag
        = pps->entropy_coding_mode_flag;
    picParam->pic_fields.bits.weighted_pred_flag = pps->weighted_pred_flag;
    picParam->pic_fields.bits.weighted_bipred_idc = pps->weighted_bipred_idc;
    picParam->pic_fields.bits.transform_8x8_mode_flag
        = pps->transform_8x8_mode_flag;
    picParam->pic_fields.bits.field_pic_flag = slice->field_pic_flag;
    picParam->pic_fields.bits.constrained_intra_pred_flag
        = pps->constrained_intra_pred_flag;
    picParam->pic_fields.bits.pic_order_present_flag
        = pps->pic_order_present_flag;
    picParam->pic_fields.bits.deblocking_filter_control_present_flag
        = pps->deblocking_filter_control_present_flag;
    picParam->pic_fields.bits.redundant_pic_cnt_present_flag
        = pps->redundant_pic_cnt_present_flag;
    picParam->pic_fields.bits.reference_pic_flag = picture->m_isReference;

    picParam->frame_num = slice->frame_num;

    return true;
}

void VaapiDecoderH264::fillReferenceIndexForList(
    VASliceParameterBufferH264* sliceParam, const SliceHeader* const slice,
    RefSet& refSet, bool isList0)
{
    uint32_t i = 0;
    VAPictureH264* refPicture
        = (isList0 ? sliceParam->RefPicList0 : sliceParam->RefPicList1);
    RefSet::iterator it = refSet.begin();

    if (isList0)
        sliceParam->num_ref_idx_l0_active_minus1
            = slice->num_ref_idx_l0_active_minus1;
    else
        sliceParam->num_ref_idx_l1_active_minus1
            = slice->num_ref_idx_l1_active_minus1;

    for (; it != refSet.end(); i++, it++) {
        fillVAPictureH264(&refPicture[i], *it);
    }
    for (; i < N_ELEMENTS(sliceParam->RefPicList0); i++) {
        refPicture[i].picture_id = VA_INVALID_SURFACE;
        refPicture[i].TopFieldOrderCnt = 0;
        refPicture[i].BottomFieldOrderCnt = 0;
        refPicture[i].flags = VA_PICTURE_H264_INVALID;
        refPicture[i].frame_idx = 0;
    }
}

bool
VaapiDecoderH264::fillReferenceIndex(VASliceParameterBufferH264* sliceParam,
                                     const SliceHeader* const slice)
{

    if (!isISlice(slice->slice_type))
        fillReferenceIndexForList(sliceParam, slice, m_dpb.m_refList0, true);

    if (isBSlice(slice->slice_type))
        fillReferenceIndexForList(sliceParam, slice, m_dpb.m_refList1, false);

    return true;
}

#define FILL_WEIGHT_TABLE(n)                                                   \
    void fillPredWedightTableL##n(VASliceParameterBufferH264* sliceParam,      \
                                  const SliceHeader* slice,                    \
                                  uint8_t chromaArrayType)                     \
    {                                                                          \
        const PredWeightTable& w = slice->pred_weight_table;                   \
        sliceParam->luma_weight_l##n##_flag = 1;                               \
        sliceParam->chroma_weight_l##n##_flag = !!chromaArrayType;             \
        for (int i = 0; i <= sliceParam->num_ref_idx_l##n##_active_minus1;     \
             i++) {                                                            \
            sliceParam->luma_weight_l##n[i] = w.luma_weight_l##n[i];           \
            sliceParam->luma_offset_l##n[i] = w.luma_offset_l##n[i];           \
            if (sliceParam->chroma_weight_l##n##_flag) {                       \
                for (int j = 0; j < 2; j++) {                                  \
                    sliceParam->chroma_weight_l##n[i][j]                       \
                        = w.chroma_weight_l##n[i][j];                          \
                    sliceParam->chroma_offset_l##n[i][j]                       \
                        = w.chroma_offset_l##n[i][j];                          \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }

FILL_WEIGHT_TABLE(0)
FILL_WEIGHT_TABLE(1)

bool
VaapiDecoderH264::fillPredWeightTable(VASliceParameterBufferH264* sliceParam,
                                      const SliceHeader* const slice)
{
    const SharedPtr<PPS> pps = slice->m_pps;
    const SharedPtr<SPS> sps = pps->m_sps;
    const PredWeightTable& w = slice->pred_weight_table;

    sliceParam->luma_log2_weight_denom = w.luma_log2_weight_denom;
    sliceParam->chroma_log2_weight_denom = w.chroma_log2_weight_denom;

    if (pps->weighted_pred_flag && isPSlice(slice->slice_type))
        fillPredWedightTableL0(sliceParam, slice, sps->m_chromaArrayType);

    if (pps->weighted_bipred_idc && isBSlice(slice->slice_type)) {
        fillPredWedightTableL0(sliceParam, slice, sps->m_chromaArrayType);
        fillPredWedightTableL1(sliceParam, slice, sps->m_chromaArrayType);
    }

    return true;
}

bool VaapiDecoderH264::fillSlice(const PicturePtr& picture,
                                 const SliceHeader* const slice,
                                 const NalUnit* const nalu)
{
    VASliceParameterBufferH264* sliceParam;
    if (!picture->newSlice(sliceParam, nalu->m_data, nalu->m_size))
        return false;

    sliceParam->slice_data_bit_offset
        = slice->m_headerSize
          + ((nalu->m_nalUnitHeaderBytes - slice->m_emulationPreventionBytes)
             << 3);
    sliceParam->first_mb_in_slice = slice->first_mb_in_slice;
    sliceParam->slice_type = slice->slice_type % 5;
    sliceParam->direct_spatial_mv_pred_flag
        = slice->direct_spatial_mv_pred_flag;
    sliceParam->cabac_init_idc = slice->cabac_init_idc;
    sliceParam->slice_qp_delta = slice->slice_qp_delta;
    sliceParam->disable_deblocking_filter_idc
        = slice->disable_deblocking_filter_idc;
    sliceParam->slice_alpha_c0_offset_div2 = slice->slice_alpha_c0_offset_div2;
    sliceParam->slice_beta_offset_div2 = slice->slice_beta_offset_div2;

    if (!fillReferenceIndex(sliceParam, slice))
        return false;

    if (!fillPredWeightTable(sliceParam, slice))
        return false;

    return true;
}

YamiStatus VaapiDecoderH264::decodeCurrent()
{
    YamiStatus status = YAMI_SUCCESS;
    if (!m_currPic)
        return status;

    if (!m_currPic->decode()) {
        ERROR("decode %d failed", m_currPic->m_poc);
        // ignore it to let application continue to decode the next frame
        return YAMI_DECODE_INVALID_DATA;
    } else
        DEBUG("decode %d done", m_currPic->m_poc);

    if (!m_dpb.add(m_currPic))
        return YAMI_DECODE_INVALID_DATA;

    m_prevPic = m_currPic;
    m_currPic.reset();
    m_newStream = false;
    return status;
}

uint32_t calcMaxDecFrameBufferingNum(const SharedPtr<SPS>& sps)
{

    if (sps->vui_parameters_present_flag
        && sps->m_vui.bitstream_restriction_flag) {
        return sps->m_vui.max_dec_frame_buffering;
    }

    uint32_t maxDpbMbs;

    /*get MaxDpbMbs as Table A-1*/
    switch (sps->level_idc) {
    case 9:
    case 10:
        maxDpbMbs = 396;
        break;
    case 11:
        maxDpbMbs = 900;
        break;
    case 12:
    case 13:
    case 20:
        maxDpbMbs = 2376;
        break;
    case 21:
        maxDpbMbs = 4752;
        break;
    case 22:
    case 30:
        maxDpbMbs = 8100;
        break;
    case 31:
        maxDpbMbs = 18000;
        break;
    case 32:
        maxDpbMbs = 20480;
        break;
    case 40:
    case 41:
        maxDpbMbs = 32768;
        break;
    case 42:
        maxDpbMbs = 34816;
        break;
    case 50:
        maxDpbMbs = 110400;
        break;
    case 51:
    case 52:
        maxDpbMbs = 184320;
        break;
    default:
        ERROR("undefined level_idc");
        maxDpbMbs = 184320;
        break;
    }

    uint32_t picWidthInMbs, frameHeightInMbs, maxDpbFrames;

    picWidthInMbs = sps->pic_width_in_mbs_minus1 + 1; //(7-12)
    frameHeightInMbs = (2 - sps->frame_mbs_only_flag)
                       * (sps->pic_height_in_map_units_minus1 + 1); //(7-17)

    maxDpbFrames = maxDpbMbs / (picWidthInMbs * frameHeightInMbs);

    return maxDpbFrames;
}

bool VaapiDecoderH264::isDecodeContextChanged(const SharedPtr<SPS>& sps)
{
    uint32_t maxDecFrameBuffering;

    maxDecFrameBuffering = calcMaxDecFrameBufferingNum(sps);

    if (maxDecFrameBuffering > H264_MAX_REFRENCE_SURFACE_NUMBER)
        maxDecFrameBuffering = H264_MAX_REFRENCE_SURFACE_NUMBER;
    else if (maxDecFrameBuffering < sps->num_ref_frames)
        maxDecFrameBuffering = sps->num_ref_frames;

    uint32_t width = sps->frame_cropping_flag ? sps->m_cropRectWidth
                                              : sps->m_width;
    uint32_t height = sps->frame_cropping_flag ? sps->m_cropRectHeight
                                               : sps->m_height;
    if (setFormat(width, height, sps->m_width, sps->m_height, maxDecFrameBuffering + 1)) {
        if (isSurfaceGeometryChanged()) {
            decodeCurrent();
            m_dpb.flush();
            m_contextChanged = true;
        }
        return true;
    }
    return false;
}

YamiStatus VaapiDecoderH264::ensureContext(const SharedPtr<SPS>& sps)
{
    if (isDecodeContextChanged(sps))
        return YAMI_DECODE_FORMAT_CHANGE;
    return ensureProfile(VAProfileH264High); // FIXME: set different profile later
}

SurfacePtr VaapiDecoderH264::createSurface(const SliceHeader* const slice)
{
    SurfacePtr s = VaapiDecoderBase::createSurface();
    if (!s)
        return s;
    SharedPtr<SPS>& sps = slice->m_pps->m_sps;

    if (sps->frame_cropping_flag)
        s->setCrop(sps->m_cropX, sps->m_cropY, sps->m_cropRectWidth, sps->m_cropRectHeight);
    else
        s->setCrop(0, 0, sps->m_width, sps->m_height);
    return s;
}

YamiStatus VaapiDecoderH264::createPicture(const SliceHeader* const slice,
    const NalUnit* const nalu)
{
    YamiStatus status = YAMI_SUCCESS;
    VaapiPictureType picStructure;
    bool isSecondField = false;

    /* skip all non-idr slices if m_prevPic is a NULL */
    if(!m_prevPic && !nalu->m_idrPicFlag)
        return YAMI_DECODE_INVALID_DATA;

    if (slice->field_pic_flag) {
        if (slice->bottom_field_flag)
            picStructure = VAAPI_PICTURE_BOTTOM_FIELD;
        else
            picStructure = VAAPI_PICTURE_TOP_FIELD;
    } else
        picStructure = VAAPI_PICTURE_FRAME;

    /*The sencond field should use the same surface of the first filed*/
    if (slice->field_pic_flag) {
        DPB::PictureList::iterator it = find_if(
            m_dpb.m_pictures.begin(), m_dpb.m_pictures.end(),
            bind(findComplementaryField, _1, slice->frame_num, picStructure));
        if (it != m_dpb.m_pictures.end()) {
            m_currPic = (*it)->allocPicture();
            m_currPic->m_isSecondField = isSecondField = true;
            m_currPic->m_complementField = *it;
        }
    }

    if (!slice->field_pic_flag || !isSecondField) {
        m_currSurface = createSurface(slice);
        if (!m_currSurface)
            return YAMI_DECODE_NO_SURFACE;
        m_currPic.reset(
            new VaapiDecPictureH264(m_context, m_currSurface, m_currentPTS));
    }

    m_currPic->m_picOutputFlag = true;
    m_currPic->m_idrFlag = nalu->m_idrPicFlag;
    m_currPic->m_frameNum = slice->frame_num;
    m_currPic->m_pocLsb = slice->pic_order_cnt_lsb;
    m_currPic->m_hasMmco5 = checkMMCO5(slice->dec_ref_pic_marking);
    m_currPic->m_picStructure = picStructure;

    if (isIdr(m_currPic)) {
        m_prevPic.reset(
            new VaapiDecPictureH264(m_context, m_currSurface, m_currentPTS));
    }

    if (nalu->nal_ref_idc) {
        m_currPic->m_isReference = true;
        m_currPic->m_longTermRefFlag
            = (isIdr(m_currPic)
               && slice->dec_ref_pic_marking.long_term_reference_flag);
        m_currPic->m_shortTermRefFlag = !m_currPic->m_longTermRefFlag;
    } else
        m_currPic->m_isReference = m_currPic->m_longTermRefFlag
            = m_currPic->m_shortTermRefFlag = false;

    return status;
}

YamiStatus VaapiDecoderH264::decodeSlice(NalUnit* nalu)
{
    SharedPtr<SliceHeader> currSlice(new SliceHeader);
    SliceHeader* slice = currSlice.get();
    YamiStatus status;

    memset(slice, 0, sizeof(SliceHeader));

    if (!slice->parseHeader(&m_parser, nalu))
        return YAMI_DECODE_INVALID_DATA;

    status = ensureContext(slice->m_pps->m_sps);
    if (status != YAMI_SUCCESS) {
        return status;
    }

    /* first slice when first_mb_in_slice is zero */
    if (!slice->first_mb_in_slice) {
        status = decodeCurrent();
        if (status != YAMI_SUCCESS)
            return status;
        status = createPicture(slice, nalu);
        if (status != YAMI_SUCCESS)
            return status;
        if (!m_currPic
            || !m_dpb.init(m_currPic, m_prevPic, slice, nalu, m_newStream,
                   m_contextChanged, m_videoFormatInfo.surfaceNumber))
            return YAMI_DECODE_INVALID_DATA;
        m_contextChanged = false;
        if (!fillPicture(m_currPic, slice) || !fillIqMatrix(m_currPic, slice))
            return YAMI_FAIL;
    }

    if (!m_currPic)
        return YAMI_DECODE_INVALID_DATA;

    /* should init reference for every slice */
    m_dpb.initReference(m_currPic, slice);

    if (!fillSlice(m_currPic, slice, nalu))
        return YAMI_FAIL;

    return status;
}

YamiStatus VaapiDecoderH264::decodeNalu(NalUnit* nalu)
{
    uint8_t type = nalu->nal_unit_type;
    YamiStatus status = YAMI_SUCCESS;

    if (NAL_SLICE_NONIDR <= type && type <= NAL_SLICE_IDR) {
        status = decodeSlice(nalu);
    } else {
        status = decodeCurrent();
        if (status != YAMI_SUCCESS)
            return status;
        switch (type) {
        case NAL_SPS:
            status = decodeSps(nalu);
            break;
        case NAL_PPS:
            status = decodePps(nalu);
            break;
        case NAL_STREAM_END:
            m_endOfStream = true;
            break;
        case NAL_SEQ_END:
            m_endOfSequence = true;
            break;
        default:
            break;
        }
    }

    return status;
}

void VaapiDecoderH264::flush(void)
{
    decodeCurrent();
    m_dpb.flush();
    m_newStream = true;
    m_endOfStream = false;
    m_endOfSequence = false;
    m_currPic.reset();
    m_prevPic.reset();
    m_currSurface.reset();
    m_contextChanged = false;
    VaapiDecoderBase::flush();
}

YamiStatus VaapiDecoderH264::decode(VideoDecodeBuffer* buffer)
{
    if (!buffer || !buffer->data) {
        decodeCurrent();
        m_dpb.flush();
        m_newStream = true;
        m_endOfStream = false;
        m_endOfSequence = false;
        m_currPic.reset();
        m_prevPic.reset();
        m_currSurface.reset();
        m_contextChanged = false;
        return YAMI_SUCCESS;
    }
    m_currentPTS = buffer->timeStamp;

    int32_t size;
    NalUnit nalu;
    YamiStatus lastError = YAMI_SUCCESS;
    YamiStatus status = YAMI_SUCCESS;
    const uint8_t* nal;
    NalReader nr(buffer->data, buffer->size, m_nalLengthSize);

    while (nr.read(nal, size)) {
        if (nalu.parseNalUnit(nal, size))
            status = decodeNalu(&nalu);
        if (status != YAMI_SUCCESS) {
            //we will continue decode if decodeNalu return YAMI_DECODE_INVALID_DATA
            //but we will return the error at end of fucntion
            lastError = status;
            if (status != YAMI_DECODE_INVALID_DATA)
                return status;
        }
    }
    return lastError;
}

}
