/*
 * Copyright (C) 2013-2016 Intel Corporation. All rights reserved.
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

#include "codecparsers/h265Parser.h"
#include "common/log.h"
#include "common/nalreader.h"

#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapidecpicture.h"

#include <algorithm>

#include "vaapidecoder_h265.h"

namespace YamiMediaCodec{
typedef VaapiDecoderH265::PicturePtr PicturePtr;
using std::bind;
using std::placeholders::_1;
using std::ref;

using namespace YamiParser::H265;

bool isIdr(const NalUnit* const nalu)
{
    return nalu->nal_unit_type == NalUnit::IDR_W_RADL
            || nalu->nal_unit_type == NalUnit::IDR_N_LP;
}

bool isBla(const NalUnit* const nalu)
{
    return nalu->nal_unit_type == NalUnit::BLA_W_LP
            || nalu->nal_unit_type == NalUnit::BLA_W_RADL
            || nalu->nal_unit_type == NalUnit::BLA_N_LP;
}

bool isIrap(const NalUnit* const nalu)
{
    return nalu->nal_unit_type >=  NalUnit::BLA_W_LP
            && nalu->nal_unit_type <= NalUnit::RSV_IRAP_VCL23;
}

bool isRasl(const NalUnit* const nalu)
{
    return nalu->nal_unit_type == NalUnit::RASL_R
            || nalu->nal_unit_type == NalUnit::RASL_N;
}

bool isRadl(const NalUnit* const nalu)
{
    return nalu->nal_unit_type == NalUnit::RADL_R
            || nalu->nal_unit_type == NalUnit::RADL_N;
}

bool isCra(const NalUnit* const nalu)
{
    return nalu->nal_unit_type == NalUnit::CRA_NUT;
}

bool isSublayerNoRef(const NalUnit* const nalu)
{
    static const uint8_t noRef[] = {
        NalUnit::TRAIL_N,
        NalUnit::TSA_N,
        NalUnit::STSA_N,
        NalUnit::RADL_N,
        NalUnit::RASL_N,
        NalUnit::RSV_VCL_N10,
        NalUnit::RSV_VCL_N12,
        NalUnit::RSV_VCL_N14
    };
    static const uint8_t* end  = noRef + N_ELEMENTS(noRef);
    return std::binary_search(noRef, end, nalu->nal_unit_type);
}

class VaapiDecPictureH265 : public VaapiDecPicture
{
public:
    VaapiDecPictureH265(const ContextPtr& context, const SurfacePtr& surface, int64_t timeStamp):
        VaapiDecPicture(context, surface, timeStamp)
    {
    }
    VaapiDecPictureH265()
    {
    }
    int32_t     m_poc;
    uint16_t    m_pocLsb;
    bool        m_noRaslOutputFlag;
    bool        m_picOutputFlag;
    uint32_t    m_picLatencyCount;

    //is unused reference picture
    bool        m_isUnusedReference;
    //is this picture ref able?
    bool        m_isReference;

};

inline bool VaapiDecoderH265::DPB::PocLess::operator()(const PicturePtr& left, const PicturePtr& right) const
{
    return left->m_poc < right->m_poc;
}

VaapiDecoderH265::DPB::DPB(OutputCallback output):
    m_output(output),
    m_dummy(new VaapiDecPictureH265)
{
}

bool VaapiDecoderH265::DPB::initShortTermRef(RefSet& ref, int32_t currPoc,
            const int32_t* delta, const uint8_t* used,  uint8_t num)
{
    if (num > 16)
        return false;
    ref.clear();
    for (uint8_t i = 0; i < num; i++) {
        int32_t poc = currPoc + delta[i];
        VaapiDecPictureH265* pic = getPic(poc);
        if (!pic) {
            ERROR("can't find short ref %d for %d", poc, currPoc);
        } else {
            if (used[i])
                ref.push_back(pic);
            else
                m_stFoll.push_back(pic);
        }
    }
    return true;
}

bool VaapiDecoderH265::DPB::initShortTermRef(const PicturePtr& picture,
                                             const SliceHeader* const slice)
{
    const PPS *const pps = slice->pps.get();
    const SPS *const sps = pps->sps.get();

    const ShortTermRefPicSet* stRef;
    if (!slice->short_term_ref_pic_set_sps_flag)
        stRef = &slice->short_term_ref_pic_sets;
    else
        stRef = &sps->short_term_ref_pic_set[slice->short_term_ref_pic_set_idx];

    //clear it here
    m_stFoll.clear();

    if (!initShortTermRef(m_stCurrBefore, picture->m_poc,
            stRef->DeltaPocS0, stRef->UsedByCurrPicS0, stRef->NumNegativePics))
        return false;
    if (!initShortTermRef(m_stCurrAfter, picture->m_poc,
            stRef->DeltaPocS1, stRef->UsedByCurrPicS1, stRef->NumPositivePics))
        return false;
    return true;
}

bool VaapiDecoderH265::DPB::initLongTermRef(const PicturePtr& picture, const SliceHeader *const slice)
{
    const PPS *const pps = slice->pps.get();
    const SPS *const sps = pps->sps.get();

    int32_t deltaPocMsbCycleLt[16];

    const int32_t maxPicOrderCntLsb =1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    uint16_t num = slice->num_long_term_sps + slice->num_long_term_pics;
    //(7-38)
    for (int i = 0; i < num; i++) {
        if( i == 0  || i == slice->num_long_term_sps)
            deltaPocMsbCycleLt[ i ] = slice->delta_poc_msb_cycle_lt[i];
        else
            deltaPocMsbCycleLt[ i ] = slice->delta_poc_msb_cycle_lt[ i ] + deltaPocMsbCycleLt[i-1];
    }
    //(8-5)
    for (int i = 0; i < num; i++) {
        int32_t poc;
        bool used;
        if (i < slice->num_long_term_sps) {
            poc = sps->lt_ref_pic_poc_lsb_sps[slice->lt_idx_sps[i]];
            used = sps->used_by_curr_pic_lt_sps_flag[slice->lt_idx_sps[i]];
        } else {
            poc = slice->poc_lsb_lt[i];
            used = slice->used_by_curr_pic_lt_flag[i];
        }
        if (slice->delta_poc_msb_present_flag[i]) {
            poc += picture->m_poc - deltaPocMsbCycleLt[i] * maxPicOrderCntLsb
                    - slice->slice_pic_order_cnt_lsb;
        }
        VaapiDecPictureH265* pic = getPic(poc, slice->delta_poc_msb_present_flag[i]);
        if (!pic) {
            ERROR("can't find long ref %d for %d", poc, picture->m_poc);
        } else {
            if (used)
                m_ltCurr.push_back(pic);
            else
                m_ltFoll.push_back(pic);
        }
    }
    return true;

}

void markUnusedReference(const PicturePtr& picture)
{
    picture->m_isUnusedReference = true;
}

void clearReference(const PicturePtr& picture)
{
    if (picture->m_isUnusedReference)
        picture->m_isReference = false;
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

void VaapiDecoderH265::DPB::removeUnused()
{
    forEach(clearReference);

    /* Remove unused pictures from DPB */
    PictureList::iterator it;
    for (it = m_pictures.begin(); it != m_pictures.end();) {
        if (isUnusedPicture(*it))
            m_pictures.erase(it++);
        else
            ++it;
    }
}

void VaapiDecoderH265::DPB::clearRefSet()
{
    m_stCurrBefore.clear();
    m_stCurrAfter.clear();
    m_stFoll.clear();
    m_ltCurr.clear();
    m_ltFoll.clear();
}
/*8.3.2*/
bool VaapiDecoderH265::DPB::initReference(const PicturePtr& picture,
     const SliceHeader *const slice,  const NalUnit *const nalu, bool newStream)
{
    clearRefSet();
    if (isIdr(nalu))
        return true;
    if (!initShortTermRef(picture, slice))
        return false;
    if (!initLongTermRef(picture, slice))
        return false;
    return true;
}

bool matchPocLsb(const PicturePtr& picture, int32_t poc)
{
    return picture->m_pocLsb == poc;
}

VaapiDecPictureH265* VaapiDecoderH265::DPB::getPic(int32_t poc, bool hasMsb)
{
    PictureList::iterator it;
    if (hasMsb) {
        m_dummy->m_poc = poc;
        it = m_pictures.find(m_dummy);
    } else {
        it = find_if(m_pictures.begin(), m_pictures.end(), bind(matchPocLsb, _1, poc));
    }
    if (it != m_pictures.end()) {
        const PicturePtr& picture = *it;
        if (picture->m_isReference) {
            //use by current decode picture
            picture->m_isUnusedReference = false;
            return picture.get();
        }
    }
    return NULL;
}

void VaapiDecoderH265::DPB::forEach(ForEachFunction fn)
{
    std::for_each(m_pictures.begin(), m_pictures.end(), fn);
}

bool VaapiDecoderH265::DPB::checkReorderPics(const SPS* const sps)
{
    uint32_t num = count_if(m_pictures.begin(), m_pictures.end(), isOutputNeeded);
    return num > sps->sps_max_num_reorder_pics[sps->sps_max_sub_layers_minus1];
}

bool checkPicLatencyCount(const PicturePtr& picture, uint32_t spsMaxLatencyPictures)
{
    return isOutputNeeded(picture) && (picture->m_picLatencyCount >= spsMaxLatencyPictures);
}

bool VaapiDecoderH265::DPB::checkLatency(const SPS* const sps)
{
    uint8_t highestTid = sps->sps_max_sub_layers_minus1;
    if (!sps->sps_max_latency_increase_plus1[highestTid])
        return false;
    uint16_t spsMaxLatencyPictures = sps->sps_max_num_reorder_pics[highestTid]
        + sps->sps_max_latency_increase_plus1[highestTid] - 1;
    return find_if(m_pictures.begin(),
                   m_pictures.end(),
                   bind(checkPicLatencyCount, _1, spsMaxLatencyPictures)
                   ) != m_pictures.end();

}

bool VaapiDecoderH265::DPB::checkDpbSize(const SPS* const sps)
{
    uint8_t highestTid = sps->sps_max_sub_layers_minus1;
    return m_pictures.size() >= (size_t)(sps->sps_max_dec_pic_buffering_minus1[highestTid] + 1);
}

//C.5.2.2
bool VaapiDecoderH265::DPB::init(const PicturePtr& picture,
     const SliceHeader *const slice,  const NalUnit *const nalu, bool newStream)
{
    forEach(markUnusedReference);
    if (!initReference(picture, slice, nalu, newStream))
        return false;
    if (isIrap(nalu) && picture->m_noRaslOutputFlag && !newStream) {
        bool noOutputOfPriorPicsFlag;
        //TODO how to check C.5.2.2 item 1's second otherwise
        if (isCra(nalu))
            noOutputOfPriorPicsFlag = true;
        else
            noOutputOfPriorPicsFlag = slice->no_output_of_prior_pics_flag;
        clearRefSet();
        if (!noOutputOfPriorPicsFlag) {
            removeUnused();
            bumpAll();
        }
        m_pictures.clear();
        return true;
    }
    removeUnused();
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
    while (checkReorderPics(sps) || checkLatency(sps) || checkDpbSize(sps)) {
        if (!bump())
            return false;
    }

    return true;
}

bool VaapiDecoderH265::DPB::output(const PicturePtr& picture)
{
    picture->m_picOutputFlag = false;

    return m_output(picture) == YAMI_SUCCESS;
}

bool VaapiDecoderH265::DPB::bump()
{
    PictureList::iterator it =
        find_if(m_pictures.begin(),m_pictures.end(), isOutputNeeded);
    if (it == m_pictures.end())
        return false;
    bool success = output(*it);
    if (!isReference(*it))
        m_pictures.erase(it);
    return success;
}

void VaapiDecoderH265::DPB::bumpAll()
{
     while (bump())
        /* nothing */;
}

void addLatency(const PicturePtr& picture)
{
    if (picture->m_picOutputFlag)
        picture->m_picLatencyCount++;
}

bool VaapiDecoderH265::DPB::add(const PicturePtr& picture, const SliceHeader* const lastSlice)
{
    const PPS* const pps = lastSlice->pps.get();
    const SPS* const sps = pps->sps.get();

    forEach(addLatency);
    picture->m_picLatencyCount = 0;
    picture->m_isReference = true;
    m_pictures.insert(picture);
    while (checkReorderPics(sps) || checkLatency(sps))
        bump();
    return true;
}


void VaapiDecoderH265::DPB::flush()
{
    bumpAll();
    clearRefSet();
    m_pictures.clear();
}

VaapiDecoderH265::VaapiDecoderH265():
    m_prevPicOrderCntMsb(0),
    m_prevPicOrderCntLsb(0),
    m_nalLengthSize(0),
    m_newStream(true),
    m_endOfSequence(false),
    m_dpb(bind(&VaapiDecoderH265::outputPicture, this, _1))
{
    m_parser.reset(new Parser());
    m_prevSlice.reset(new SliceHeader());
}

VaapiDecoderH265::~VaapiDecoderH265()
{
    stop();
}

YamiStatus VaapiDecoderH265::start(VideoConfigBuffer* buffer)
{
    if (buffer->data && buffer->size > 0) {
        if (!decodeHevcRecordData(buffer->data, buffer->size)) {
            ERROR("decode record data failed");
            return DECODE_FAIL;
        }
    }

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderH265::decodeParamSet(NalUnit* nalu)
{
    bool res = true;

    switch (nalu->nal_unit_type) {
    case NalUnit::VPS_NUT:
        res = m_parser->parseVps(nalu);
        break;
    case NalUnit::SPS_NUT:
        res = m_parser->parseSps(nalu);
        break;
    case NalUnit::PPS_NUT:
        res = m_parser->parsePps(nalu);
    }

    return res ? YAMI_SUCCESS : YAMI_DECODE_INVALID_DATA;
}

YamiStatus VaapiDecoderH265::outputPicture(const PicturePtr& picture)
{
    VaapiDecoderBase::PicturePtr base = std::static_pointer_cast<VaapiDecPicture>(picture);
    return VaapiDecoderBase::outputPicture(base);
}

YamiStatus VaapiDecoderH265::decodeCurrent()
{
    YamiStatus status = YAMI_SUCCESS;
    if (!m_current)
        return status;
    if (!m_current->decode()) {
        ERROR("decode %d failed", m_current->m_poc);
        //ignore it
        return status;
    }
    if (!m_dpb.add(m_current, m_prevSlice.get()))
        return YAMI_DECODE_INVALID_DATA;
    m_current.reset();
    m_newStream = false;
    return status;
}

#define FILL_SCALING_LIST(mxm) \
void fillScalingList##mxm(VAIQMatrixBufferHEVC* iqMatrix, const ScalingList* const scalingList) \
{ \
    for (size_t i = 0; i < N_ELEMENTS(iqMatrix->ScalingList##mxm); i++) { \
        for (size_t j = 0; j < N_ELEMENTS(UpperRightDiagonal##mxm); j++) { \
            iqMatrix->ScalingList##mxm[i][UpperRightDiagonal##mxm[j]] = scalingList->scalingList##mxm[i][j]; \
        } \
    } \
}

FILL_SCALING_LIST(4x4)
FILL_SCALING_LIST(8x8)
FILL_SCALING_LIST(16x16)
void fillScalingList32x32(VAIQMatrixBufferHEVC* iqMatrix, const ScalingList* const scalingList)
{
    for (size_t i = 0; i < N_ELEMENTS(iqMatrix->ScalingList32x32); i++) {
        for (size_t j = 0; j < N_ELEMENTS(UpperRightDiagonal32x32); j++) {
            // According to spec "7.3.4 Scaling list data syntax",
            // just use scalingList32x32[0] and scalingList32x32[3].
            iqMatrix->ScalingList32x32[i][UpperRightDiagonal32x32[j]] = scalingList->scalingList32x32[i * 3][j];
        }
    }
}

#define FILL_SCALING_LIST_DC(mxm)                                                                     \
    void fillScalingListDc##mxm(VAIQMatrixBufferHEVC* iqMatrix, const ScalingList* const scalingList) \
    {                                                                                                 \
        for (size_t i = 0; i < N_ELEMENTS(iqMatrix->ScalingListDC##mxm); i++) {                       \
            iqMatrix->ScalingListDC##mxm[i] = scalingList->scalingListDC##mxm[i];                     \
        }                                                                                             \
    }

FILL_SCALING_LIST_DC(16x16)
void fillScalingListDc32x32(VAIQMatrixBufferHEVC* iqMatrix, const ScalingList* const scalingList)
{
    for (size_t i = 0; i < N_ELEMENTS(iqMatrix->ScalingListDC32x32); i++) {
        // similar to scalingList32x32.
        iqMatrix->ScalingListDC32x32[i] = scalingList->scalingListDC32x32[i * 3];
    }
}

bool VaapiDecoderH265::fillIqMatrix(const PicturePtr& picture, const SliceHeader* const slice)
{
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
    const ScalingList* scalingList;
    if (pps->pps_scaling_list_data_present_flag) {
        scalingList = &pps->scaling_list;
    } else if(sps->scaling_list_enabled_flag) {
        if(sps->sps_scaling_list_data_present_flag) {
            scalingList = &sps->scaling_list;
        } else {
            scalingList = &pps->scaling_list;
        }
    } else {
        //default scaling list
        return true;
    }
    VAIQMatrixBufferHEVC* iqMatrix;
    if (!picture->editIqMatrix(iqMatrix))
        return false;
    fillScalingList4x4(iqMatrix, scalingList);
    fillScalingList8x8(iqMatrix, scalingList);
    fillScalingList16x16(iqMatrix, scalingList);
    fillScalingList32x32(iqMatrix, scalingList);
    fillScalingListDc16x16(iqMatrix, scalingList);
    fillScalingListDc32x32(iqMatrix, scalingList);
    return true;
}

void VaapiDecoderH265::fillReference(VAPictureHEVC* refs, int32_t& n,
    const RefSet& refset, uint32_t flags)
{
    for (size_t i = 0; i < refset.size(); i++) {
        VAPictureHEVC* r = refs + n;
        const VaapiDecPictureH265* pic = refset[i];

        r->picture_id = refset[i]->getSurfaceID();
        r->pic_order_cnt = pic->m_poc;
        r->flags = flags;

        //record for late use
        m_pocToIndex[pic->m_poc] = n;

        n++;

    }
}

void VaapiDecoderH265::fillReference(VAPictureHEVC* refs, int32_t size)
{
    int32_t n = 0;
    //clear index map
    m_pocToIndex.clear();

    fillReference(refs, n, m_dpb.m_stCurrBefore, VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE);
    fillReference(refs, n, m_dpb.m_stCurrAfter, VA_PICTURE_HEVC_RPS_ST_CURR_AFTER);
    fillReference(refs, n, m_dpb.m_stFoll, 0);
    fillReference(refs, n, m_dpb.m_ltCurr, VA_PICTURE_HEVC_LONG_TERM_REFERENCE | VA_PICTURE_HEVC_RPS_LT_CURR);
    fillReference(refs, n, m_dpb.m_ltFoll, VA_PICTURE_HEVC_LONG_TERM_REFERENCE);
    for (int i = n; i < size; i++) {
        VAPictureHEVC* ref = refs + i;
        ref->picture_id = VA_INVALID_SURFACE;
        ref->pic_order_cnt = 0;
        ref->flags = VA_PICTURE_HEVC_INVALID;
    }


}
bool VaapiDecoderH265::fillPicture(const PicturePtr& picture, const SliceHeader* const slice)
{
    VAPictureParameterBufferHEVC* param;
    if (!picture->editPicture(param))
        return false;
    param->CurrPic.picture_id = picture->getSurfaceID();
    param->CurrPic.pic_order_cnt = picture->m_poc;
    fillReference(param->ReferenceFrames,  N_ELEMENTS(param->ReferenceFrames));

    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
#define FILL(h, f) param->f = h->f
    FILL(sps, pic_width_in_luma_samples);
    FILL(sps, pic_height_in_luma_samples);
#define FILL_PIC(h, f) param->pic_fields.bits.f  = h->f
    FILL_PIC(sps, chroma_format_idc);
    FILL_PIC(sps, separate_colour_plane_flag);
    FILL_PIC(sps, pcm_enabled_flag);
    FILL_PIC(sps, scaling_list_enabled_flag);
    FILL_PIC(pps, transform_skip_enabled_flag);
    FILL_PIC(sps, amp_enabled_flag);
    FILL_PIC(sps, strong_intra_smoothing_enabled_flag);
    FILL_PIC(pps, sign_data_hiding_enabled_flag);
    FILL_PIC(pps, constrained_intra_pred_flag);
    FILL_PIC(pps, cu_qp_delta_enabled_flag);
    FILL_PIC(pps, weighted_pred_flag);
    FILL_PIC(pps, weighted_bipred_flag);
    FILL_PIC(pps, transquant_bypass_enabled_flag);
    FILL_PIC(pps, tiles_enabled_flag);
    FILL_PIC(pps, entropy_coding_sync_enabled_flag);
    param->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag
        = pps->pps_loop_filter_across_slices_enabled_flag;
    FILL_PIC(pps, loop_filter_across_tiles_enabled_flag);
    FILL_PIC(sps, pcm_loop_filter_disabled_flag);
    //how to fill this?
    //NoPicReorderingFlag
    //NoBiPredFlag

    param->sps_max_dec_pic_buffering_minus1 =
        sps->sps_max_dec_pic_buffering_minus1[0];
    FILL(sps, bit_depth_luma_minus8);
    FILL(sps, bit_depth_chroma_minus8);
    FILL(sps, pcm_sample_bit_depth_luma_minus1);
    FILL(sps, pcm_sample_bit_depth_chroma_minus1);
    FILL(sps, log2_min_luma_coding_block_size_minus3);
    FILL(sps, log2_diff_max_min_luma_coding_block_size);
    FILL(sps, log2_min_transform_block_size_minus2);
    FILL(sps, log2_diff_max_min_transform_block_size);
    FILL(sps, log2_min_pcm_luma_coding_block_size_minus3);
    FILL(sps, log2_diff_max_min_pcm_luma_coding_block_size);
    FILL(sps, max_transform_hierarchy_depth_intra);
    FILL(sps, max_transform_hierarchy_depth_inter);
    FILL(pps, init_qp_minus26);
    FILL(pps, diff_cu_qp_delta_depth);
    param->pps_cb_qp_offset = pps->pps_cb_qp_offset;
    param->pps_cr_qp_offset = pps->pps_cr_qp_offset;
    FILL(pps, log2_parallel_merge_level_minus2);
    FILL(pps, num_tile_columns_minus1);
    FILL(pps, num_tile_rows_minus1);
    for (int i = 0; i <= pps->num_tile_columns_minus1; i++) {
        param->column_width_minus1[i] = pps->column_width_minus1[i];
    }
    for (int i = 0; i <= pps->num_tile_rows_minus1; i++) {
        param->row_height_minus1[i] = pps->row_height_minus1[i];
    }


#define FILL_SLICE(h, f)    param->slice_parsing_fields.bits.f = h->f
#define FILL_SLICE_1(h, f) param->slice_parsing_fields.bits.h##_##f = h->f

    FILL_SLICE(pps, lists_modification_present_flag);
    FILL_SLICE(sps, long_term_ref_pics_present_flag);
    FILL_SLICE_1(sps, temporal_mvp_enabled_flag);

    FILL_SLICE(pps, cabac_init_present_flag);
    FILL_SLICE(pps, output_flag_present_flag);
    FILL_SLICE(pps, dependent_slice_segments_enabled_flag);
    FILL_SLICE_1(pps, slice_chroma_qp_offsets_present_flag);
    FILL_SLICE(sps, sample_adaptive_offset_enabled_flag);
    FILL_SLICE(pps, deblocking_filter_override_enabled_flag);
    param->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag =
        pps->pps_deblocking_filter_disabled_flag;
    FILL_SLICE(pps, slice_segment_header_extension_present_flag);

    /* how to fill following fields
    RapPicFlag
    IdrPicFlag
    IntraPicFlag  */

    FILL(sps, log2_max_pic_order_cnt_lsb_minus4);
    FILL(sps, num_short_term_ref_pic_sets);
    param->num_long_term_ref_pic_sps = sps->num_long_term_ref_pics_sps;
    FILL(pps, num_ref_idx_l0_default_active_minus1);
    FILL(pps, num_ref_idx_l1_default_active_minus1);
    param->pps_beta_offset_div2 = pps->pps_beta_offset_div2;
    param->pps_tc_offset_div2 = pps->pps_tc_offset_div2;
    FILL(pps, num_extra_slice_header_bits);

    /* how to fill this
     st_rps_bits*/

#undef FILL
#undef FILL_PIC
#undef FILL_SLICE
#undef FILL_SLICE_1

    return true;
}

bool VaapiDecoderH265::getRefPicList(RefSet& refset, const RefSet& stCurr0, const RefSet& stCurr1,
    uint8_t numActive, bool modify, const uint32_t* modiList)
{
    if (numActive > 15) {
        ERROR("bug: reference picutre can't large than 15");
        return false;
    }
    const RefSet& ltCurr = m_dpb.m_ltCurr;
    uint8_t numPocTotalCurr = stCurr0.size() + stCurr1.size() + ltCurr.size();
    if (numActive && !numPocTotalCurr) {
        ERROR("active refs is %d, but num numPocTotalCurr is %d", numActive, numPocTotalCurr);
        return false;
    }
    uint8_t numRpsCurrTempList = std::max(numPocTotalCurr, numActive);
    RefSet temp;
    temp.reserve(numRpsCurrTempList);
    uint32_t rIdx = 0;
    //(8-8) and (8-10)
    while (rIdx < numRpsCurrTempList) {
        for(uint32_t i = 0; i < stCurr0.size() && rIdx < numRpsCurrTempList; rIdx++, i++ )
            temp.push_back(stCurr0[i]);
        for(uint32_t i = 0; i < stCurr1.size() && rIdx < numRpsCurrTempList; rIdx++, i++ )
            temp.push_back(stCurr1[i]);
        for(uint32_t i = 0; i < ltCurr.size() && rIdx < numRpsCurrTempList; rIdx++, i++ )
            temp.push_back(ltCurr[i]);
    }
    refset.clear();
    refset.reserve(numActive);
    //(8-9) and (8-11)
    for( rIdx = 0; rIdx < numActive; rIdx++) {
        uint8_t idx = modify ? modiList[rIdx] : rIdx;
        if (idx < temp.size()) {
            refset.push_back(temp[idx]);
        } else {
            ERROR("can't get idx from temp ref, modify = %d, idx = %d, iIdx = %d", modify, idx, rIdx);
        }
    }
    return true;
}

uint8_t  VaapiDecoderH265::getIndex(int32_t poc)
{
    return m_pocToIndex[poc];
}

void VaapiDecoderH265::fillReferenceIndexForList(VASliceParameterBufferHEVC* sliceParam,
    const RefSet& refset, bool isList0)
{
    int n = isList0?0:1;
    uint32_t i;
    for (i = 0; i < refset.size(); i++) {
        sliceParam->RefPicList[n][i] = getIndex(refset[i]->m_poc);
    }
    for ( ; i < N_ELEMENTS(sliceParam->RefPicList[n]); i++) {
        sliceParam->RefPicList[n][i] = 0xFF;
    }

}

// 8.3.4
bool VaapiDecoderH265::fillReferenceIndex(VASliceParameterBufferHEVC* sliceParam, const SliceHeader* const slice)
{
    RefSet& before = m_dpb.m_stCurrBefore;
    RefSet& after = m_dpb.m_stCurrAfter;

    RefSet refset;
    if (!slice->isISlice()) {
        if (!getRefPicList(refset, before, after,
            slice->num_ref_idx_l0_active_minus1 + 1,
            slice->ref_pic_list_modification.ref_pic_list_modification_flag_l0,
            slice->ref_pic_list_modification.list_entry_l0)) {
            return false;
        }
    }
    fillReferenceIndexForList(sliceParam, refset, true);

    refset.clear();
    if (slice->isBSlice()) {
        if (!getRefPicList(refset, after, before,
            slice->num_ref_idx_l1_active_minus1 + 1,
            slice->ref_pic_list_modification.ref_pic_list_modification_flag_l1,
            slice->ref_pic_list_modification.list_entry_l1)) {
            return false;
        }
    }
    fillReferenceIndexForList(sliceParam, refset, false);

    sliceParam->num_ref_idx_l0_active_minus1 = slice->num_ref_idx_l0_active_minus1;
    sliceParam->num_ref_idx_l1_active_minus1 = slice->num_ref_idx_l1_active_minus1;

    return true;

}

inline int32_t clip3(int32_t x, int32_t y, int32_t z)
{
    if (z < x)
        return x;
    if (z > y)
        return y;
    return z;
}

#define FILL_WEIGHT_TABLE(n) \
void fillPredWedightTableL##n(VASliceParameterBufferHEVC* sliceParam, \
        const SliceHeader* slice, uint8_t chromaLog2WeightDenom) \
{ \
    const PredWeightTable& w = slice->pred_weight_table; \
    for (int i = 0; i <= sliceParam->num_ref_idx_l##n##_active_minus1; i++) { \
        if (w.luma_weight_l##n##_flag[i]) { \
                sliceParam->delta_luma_weight_l##n[i] = w.delta_luma_weight_l##n[i]; \
                sliceParam->luma_offset_l##n[i] = w.luma_offset_l##n[i];\
            } \
            if (w.chroma_weight_l##n##_flag[i]) { \
                for (int j = 0; j < 2; j++) { \
                    int8_t deltaWeight = w.delta_chroma_weight_l##n[i][j]; \
                    int32_t chromaWeight = (1 << chromaLog2WeightDenom) + deltaWeight; \
                    int16_t deltaOffset = w.delta_chroma_offset_l##n[i][j]; \
                    int32_t chromaOffset = \
                        128 + deltaOffset - ((128*chromaWeight)>>chromaLog2WeightDenom);\
\
                    sliceParam->delta_chroma_weight_l##n[i][j] = deltaWeight; \
                    sliceParam->ChromaOffsetL##n[i][j]= (int8_t)clip3(-128, 127, chromaOffset); \
            } \
        } \
    } \
}

FILL_WEIGHT_TABLE(0)
FILL_WEIGHT_TABLE(1)

bool VaapiDecoderH265::fillPredWeightTable(VASliceParameterBufferHEVC* sliceParam, const SliceHeader* const slice)
{
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
    const PredWeightTable& w = slice->pred_weight_table;
    if ((pps->weighted_pred_flag && slice->isPSlice()) ||
            (pps->weighted_bipred_flag && slice->isBSlice())) {
        uint8_t chromaLog2WeightDenom = w.luma_log2_weight_denom;
        sliceParam->luma_log2_weight_denom = w.luma_log2_weight_denom;
        if (sps->chroma_format_idc != 0) {
            sliceParam->delta_chroma_log2_weight_denom
                = w.delta_chroma_log2_weight_denom;
            chromaLog2WeightDenom
                += w.delta_chroma_log2_weight_denom;
        }
        fillPredWedightTableL0(sliceParam,  slice, chromaLog2WeightDenom);
        if (pps->weighted_bipred_flag && slice->isBSlice())
            fillPredWedightTableL1(sliceParam,  slice, chromaLog2WeightDenom);
    }
    return true;
}

bool VaapiDecoderH265::fillSlice(const PicturePtr& picture,
        const SliceHeader* const theSlice, const NalUnit* const nalu)
{
    const SliceHeader* slice = theSlice;
    VASliceParameterBufferHEVC* sliceParam;
    if (!picture->newSlice(sliceParam, nalu->m_data, nalu->m_size))
        return false;
    sliceParam->slice_data_byte_offset =
        slice->getSliceDataByteOffset();
    sliceParam->slice_segment_address = slice->slice_segment_address;

#define FILL_LONG(f) sliceParam->LongSliceFlags.fields.f = slice->f
#define FILL_LONG_SLICE(f) sliceParam->LongSliceFlags.fields.slice_##f = slice->f
    //how to fill this
    //LastSliceOfPic
    FILL_LONG(dependent_slice_segment_flag);

    //follow spec
    if (slice->dependent_slice_segment_flag) {
        slice = m_prevSlice.get();
    }

    if (!fillReferenceIndex(sliceParam, slice))
        return false;

    sliceParam->LongSliceFlags.fields.slice_type = slice->slice_type;
    sliceParam->LongSliceFlags.fields.color_plane_id = slice->colour_plane_id;
    FILL_LONG_SLICE(sao_luma_flag);
    FILL_LONG_SLICE(sao_chroma_flag);
    FILL_LONG(mvd_l1_zero_flag);
    FILL_LONG(cabac_init_flag);
    FILL_LONG_SLICE(temporal_mvp_enabled_flag);

    if (slice->deblocking_filter_override_flag)
        FILL_LONG_SLICE(deblocking_filter_disabled_flag);
    else
        sliceParam->LongSliceFlags.fields.slice_deblocking_filter_disabled_flag=
          slice->pps->pps_deblocking_filter_disabled_flag;
    FILL_LONG(collocated_from_l0_flag);
    FILL_LONG_SLICE(loop_filter_across_slices_enabled_flag);

#define FILL(f) sliceParam->f = slice->f
#define FILL_SLICE(f) sliceParam->slice_##f = slice->f
    FILL(collocated_ref_idx);
    /* following fields fill in fillReference
       num_ref_idx_l0_active_minus1
       num_ref_idx_l1_active_minus1*/

    FILL_SLICE(qp_delta);
    FILL_SLICE(cb_qp_offset);
    FILL_SLICE(cr_qp_offset);
    FILL_SLICE(beta_offset_div2);
    FILL_SLICE(tc_offset_div2);
    if (!fillPredWeightTable(sliceParam, slice))
        return false;
    FILL(five_minus_max_num_merge_cand);
    return true;
}

#define CHECK(v, expect)                                                      \
    do {                                                                      \
        if (v != expect) {                                                    \
            ERROR("the value of %s is %d,  not equals to %d", #v, v, expect); \
            return VAProfileNone;                                             \
        }                                                                     \
    } while (0)

#define CHECK_RANGE(v, min, max)                                \
    do {                                                        \
        if (v < min || v > max) {                               \
            ERROR("%s is %d, not in [%d,%d]", #v, v, min, max); \
            return VAProfileNone;                               \
        }                                                       \
    } while (0)

VAProfile VaapiDecoderH265::getVaProfile(const SPS* const sps)
{
    if (sps->profile_tier_level.general_profile_idc == 0
        || sps->profile_tier_level.general_profile_compatibility_flag[0] == 1) {
        //general_profile_idc = 0 is not defined in spec, but some stream have this
        //we treat it as Main profile
        CHECK(sps->chroma_format_idc, 1);
        CHECK(sps->bit_depth_luma_minus8, 0);
        CHECK(sps->bit_depth_chroma_minus8, 0);
        return VAProfileHEVCMain;
    }

    if (sps->profile_tier_level.general_profile_idc == 1
        || sps->profile_tier_level.general_profile_compatibility_flag[1] == 1) {
        //A.3.2, but we only check some important values
        CHECK(sps->chroma_format_idc, 1);
        CHECK(sps->bit_depth_luma_minus8, 0);
        CHECK(sps->bit_depth_chroma_minus8, 0);
        return VAProfileHEVCMain;
    }
    if (sps->profile_tier_level.general_profile_idc == 2
        || sps->profile_tier_level.general_profile_compatibility_flag[2] == 1) {
        //A.3.3, but we only check some important values
        CHECK(sps->chroma_format_idc, 1);
        CHECK_RANGE(sps->bit_depth_luma_minus8, 0, 2);
        CHECK_RANGE(sps->bit_depth_chroma_minus8, 0, 2);
        return VAProfileHEVCMain10;
    }
    ERROR("unsupported profile %d", sps->profile_tier_level.general_profile_idc);
    return VAProfileNone;
}
#undef CHECK
#undef CHECK_RANGE

YamiStatus VaapiDecoderH265::ensureContext(const SPS* const sps)
{
    uint32_t surfaceNumber = sps->sps_max_dec_pic_buffering_minus1[0] + 1;
    uint32_t width = sps->conformance_window_flag ? sps->croppedWidth : sps->width;
    uint32_t height = sps->conformance_window_flag ? sps->croppedHeight : sps->height;
    uint32_t surfaceWidth = sps->width;
    uint32_t surfaceHeight =sps->height;
    VAProfile profile = getVaProfile(sps);
    uint32_t fourcc = (profile == VAProfileHEVCMain10) ? YAMI_FOURCC_P010 : YAMI_FOURCC_NV12;

    if (setFormat(width, height, surfaceWidth, surfaceHeight, surfaceNumber, fourcc)) {
        decodeCurrent();
        return YAMI_DECODE_FORMAT_CHANGE;
    }

    if (profile == VAProfileNone)
        return YAMI_UNSUPPORTED;
    return ensureProfile(profile);
}

/* 8.3.1 */
void VaapiDecoderH265::getPoc(const PicturePtr& picture,
        const SliceHeader* const slice,
        const NalUnit* const nalu)
{
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();

    const uint16_t pocLsb = slice->slice_pic_order_cnt_lsb;
    const int32_t MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    int32_t picOrderCntMsb;
    if (isIrap(nalu) && picture->m_noRaslOutputFlag) {
        picOrderCntMsb = 0;
    } else {
        if((pocLsb < m_prevPicOrderCntLsb)
                && ((m_prevPicOrderCntLsb - pocLsb) >= (MaxPicOrderCntLsb / 2))) {
            picOrderCntMsb = m_prevPicOrderCntMsb + MaxPicOrderCntLsb;
        } else if((pocLsb > m_prevPicOrderCntLsb)
                && ((pocLsb - m_prevPicOrderCntLsb) > (MaxPicOrderCntLsb / 2))) {
            picOrderCntMsb = m_prevPicOrderCntMsb - MaxPicOrderCntLsb;
        } else {
            picOrderCntMsb =  m_prevPicOrderCntMsb;
        }
    }
    picture->m_poc = picOrderCntMsb + pocLsb;
    picture->m_pocLsb = pocLsb;

    uint8_t temporalID = nalu->nuh_temporal_id_plus1 - 1;
    //fixme:sub-layer non-reference picture.
    if (!temporalID && !isRasl(nalu) &&  !isRadl(nalu) && !isSublayerNoRef(nalu)) {
        m_prevPicOrderCntMsb = picOrderCntMsb;
        m_prevPicOrderCntLsb = pocLsb;
    }
}

SurfacePtr VaapiDecoderH265::createSurface(const SliceHeader* const slice)
{
    SurfacePtr s = VaapiDecoderBase::createSurface();
    if (!s)
        return s;
    SharedPtr<SPS>& sps = slice->pps->sps;

    if (sps->conformance_window_flag)
        s->setCrop(sps->croppedLeft, sps->croppedTop, sps->croppedWidth, sps->croppedHeight);
    else
        s->setCrop(0, 0, sps->width, sps->height);
    return s;
}

YamiStatus VaapiDecoderH265::createPicture(PicturePtr& picture, const SliceHeader* const slice,
    const NalUnit* const nalu)
{

    SurfacePtr surface = createSurface(slice);
    if (!surface)
        return YAMI_DECODE_NO_SURFACE;
    picture.reset(new VaapiDecPictureH265(m_context, surface, m_currentPTS));

    picture->m_noRaslOutputFlag = isIdr(nalu) || isBla(nalu) ||
                                  m_newStream || m_endOfSequence;
    m_noRaslOutputFlag = picture->m_noRaslOutputFlag;
    if (isIrap(nalu))
        m_associatedIrapNoRaslOutputFlag = picture->m_noRaslOutputFlag;
    picture->m_picOutputFlag
        = (isRasl(nalu) && m_associatedIrapNoRaslOutputFlag) ? false : slice->pic_output_flag;

    getPoc(picture, slice, nalu);

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderH265::decodeSlice(NalUnit* nalu)
{
    SharedPtr<SliceHeader> currSlice(new SliceHeader());
    SliceHeader* slice = currSlice.get();
    YamiStatus status;

    if (!m_parser->parseSlice(nalu, slice))
        return YAMI_DECODE_INVALID_DATA;

    status = ensureContext(slice->pps->sps.get());
    if (status != YAMI_SUCCESS) {
        return status;
    }
    if (slice->first_slice_segment_in_pic_flag) {
        status = decodeCurrent();
        if (status != YAMI_SUCCESS)
            return status;
        status = createPicture(m_current, slice, nalu);
        if (status != YAMI_SUCCESS)
            return status;
        if (m_noRaslOutputFlag && isRasl(nalu))
            return YAMI_SUCCESS;
        if (!m_current || !m_dpb.init(m_current, slice, nalu, m_newStream))
            return YAMI_DECODE_INVALID_DATA;
        if (!fillPicture(m_current, slice) || !fillIqMatrix(m_current, slice))
            return YAMI_FAIL;
    }
    if (!m_current)
        return YAMI_FAIL;
    if (!fillSlice(m_current, slice, nalu))
        return YAMI_FAIL;
    if (!slice->dependent_slice_segment_flag)
        std::swap(currSlice, m_prevSlice);
    return status;

}

YamiStatus VaapiDecoderH265::decodeNalu(NalUnit* nalu)
{
    uint8_t type = nalu->nal_unit_type;
    YamiStatus status = YAMI_SUCCESS;

    if (NalUnit::TRAIL_N <= type && type <= NalUnit::CRA_NUT) {
        status = decodeSlice(nalu);
    }
    else if (NalUnit::PREFIX_SEI_NUT == type
        || NalUnit::SUFFIX_SEI_NUT == type) {
        //In some bitsstreams, SEI NAL units are inserted between picture NAL
        //units which belong to the same picture. If decode the current
        //picture when meeting a SEI NAL unit, the picture units after the SEI
        //will not be decoded.
    }
    else {
        status = decodeCurrent();
        if (status != YAMI_SUCCESS)
            return status;
        switch (type) {
            case NalUnit::VPS_NUT:
            case NalUnit::SPS_NUT:
            case NalUnit::PPS_NUT:
                status = decodeParamSet(nalu);
                break;
            case NalUnit::EOB_NUT:
                m_newStream = true;
                break;
            case NalUnit::EOS_NUT:
                m_endOfSequence = true;
                break;
            case NalUnit::AUD_NUT:
            case NalUnit::FD_NUT:
            default:
                break;
        }
    }

    return status;
}

void VaapiDecoderH265::flush(bool discardOutput)
{
    decodeCurrent();
    m_dpb.flush();
    m_prevPicOrderCntMsb = 0;
    m_prevPicOrderCntLsb = 0;
    m_newStream = true;
    m_endOfSequence = false;
    m_prevSlice.reset(new SliceHeader());
    if (discardOutput)
        VaapiDecoderBase::flush();
}

void VaapiDecoderH265::flush(void)
{
    flush(true);
}

YamiStatus VaapiDecoderH265::decode(VideoDecodeBuffer* buffer)
{
    if (!buffer || !buffer->data) {
        flush(false);
        return YAMI_SUCCESS;
    }
    m_currentPTS = buffer->timeStamp;

    NalReader nr(buffer->data, buffer->size, m_nalLengthSize);
    const uint8_t* nal;
    int32_t size;
    YamiStatus lastError = YAMI_SUCCESS;
    YamiStatus status;
    while (nr.read(nal, size)) {
        NalUnit nalu;
        if (nalu.parseNaluHeader(nal, size)) {
            status = decodeNalu(&nalu);
            if (status != YAMI_SUCCESS) {
                //we will continue decode if decodeNalu return YAMI_DECODE_INVALID_DATA
                //but we will return the error at end of fucntion
                lastError = status;
                if (status != YAMI_DECODE_INVALID_DATA)
                    return status;
            }
        }
    }
    return lastError;
}

bool VaapiDecoderH265::decodeHevcRecordData(uint8_t* buf, int32_t bufSize)
{
    if (buf == NULL || bufSize == 0) {
        ERROR("invalid record data");
        return false;
    }
    /*
     * Some hvcC format don't used buf[0]==1 as flag, so now used the
     * (buf[0] || buf[1] || buf[2] > 1) as hvcC condition.
     */
    if (!(buf[0] || buf[1] || buf[2] > 1)) { //annexb format
        VideoDecodeBuffer buffer;
        memset(&buffer, 0, sizeof(buffer));
        buffer.data = buf;
        buffer.size = bufSize;
        //we ignore no fatal error
        return (decode(&buffer) >= YAMI_SUCCESS);
    }
    if (bufSize < 24) {
        ERROR("invalid avcc record data");
        return false;
    }
    NalUnit nalu;
    const uint8_t* nalBuf;
    int32_t i = 0, j, numNalu, nalBufSize;
    nalBuf = &buf[21];
    int32_t nalLengthSize = (*nalBuf & 0x03) + 1;
    nalBuf++;
    numNalu = *nalBuf & 0x1f;
    nalBuf++;
    for (i = 0; i < numNalu; i++) {
        nalBuf++;
        int cnt = *(nalBuf + 1) & 0xf;
        nalBuf += 2;
        for (j = 0; j < cnt; j++) {
            int nalsize = *(nalBuf + 1) + 2;
            if (buf + bufSize - nalBuf < nalsize)
               return false;
            NalReader nr(nalBuf, nalsize, 2);
            /*sps/pps/vps nal_length_size alway is 2 in hvcC format*/
            if (!nr.read(nalBuf, nalBufSize))
                return false;
            if (!nalu.parseNaluHeader(nalBuf, nalBufSize))
                return false;
            if (decodeNalu(&nalu) != YAMI_SUCCESS)
                return false;
            nalBuf += nalBufSize;
        }
    }
    m_nalLengthSize = nalLengthSize;
    return true;
}

}

