/*
 *  vaapidecoder_h265.cpp - h265 decoder
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "codecparsers/h265parser.h"
#include "common/log.h"
#include "common/nalreader.h"
#include "vaapidecoder_factory.h"

#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapidecpicture.h"

#include <algorithm>

#include "vaapidecoder_h265.h"




namespace YamiMediaCodec{
typedef VaapiDecoderH265::PicturePtr PicturePtr;
using std::tr1::bind;
using std::tr1::placeholders::_1;
using std::tr1::ref;

bool isIdr(const H265NalUnit* const nalu)
{
    return nalu->type == H265_NAL_SLICE_IDR_W_RADL
            || nalu->type ==H265_NAL_SLICE_IDR_N_LP;
}

bool isBla(const H265NalUnit* const nalu)
{
    return nalu->type == H265_NAL_SLICE_BLA_W_LP
            || nalu->type == H265_NAL_SLICE_BLA_W_RADL
            || nalu->type == H265_NAL_SLICE_BLA_N_LP;
}

#ifndef H265_NAL_SLICE_RSV_IRAP_VCL23
#define H265_NAL_SLICE_RSV_IRAP_VCL23 23
#endif
bool isIrap(const H265NalUnit* const nalu)
{
    return nalu->type >=  H265_NAL_SLICE_BLA_W_LP
            && nalu->type <= H265_NAL_SLICE_RSV_IRAP_VCL23;
}

bool isRasl(const H265NalUnit* const nalu)
{
    return nalu->type == H265_NAL_SLICE_RASL_R
            || nalu->type == H265_NAL_SLICE_RASL_N;
}

bool isRadl(const H265NalUnit* const nalu)
{
    return nalu->type == H265_NAL_SLICE_RADL_R
            || nalu->type == H265_NAL_SLICE_RADL_N;
}

bool isCra(const H265NalUnit* const nalu)
{
    return nalu->type == H265_NAL_SLICE_CRA_NUT;
}


#ifndef H265_NAL_SLICE_RSV_VCL_N10
#define H265_NAL_SLICE_RSV_VCL_N10 10
#endif

#ifndef H265_NAL_SLICE_RSV_VCL_N12
#define H265_NAL_SLICE_RSV_VCL_N12 12
#endif

#ifndef H265_NAL_SLICE_RSV_VCL_N14
#define H265_NAL_SLICE_RSV_VCL_N14 14
#endif

bool isSublayerNoRef(const H265NalUnit* const nalu)
{
    static const uint8_t noRef[] = {
        H265_NAL_SLICE_TRAIL_N,
        H265_NAL_SLICE_TSA_N,
        H265_NAL_SLICE_STSA_N,
        H265_NAL_SLICE_RADL_N,
        H265_NAL_SLICE_RASL_N,
        H265_NAL_SLICE_RSV_VCL_N10,
        H265_NAL_SLICE_RSV_VCL_N12,
        H265_NAL_SLICE_RSV_VCL_N14
    };
    static const uint8_t* end  = noRef + N_ELEMENTS(noRef);
    return std::binary_search(noRef, end, nalu->type);
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
                                             const H265SliceHdr* const slice)
{
    const H265PPS *const pps = slice->pps;
    const H265SPS *const sps = pps->sps;

    const H265ShortTermRefPicSet* stRef;
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

bool VaapiDecoderH265::DPB::initLongTermRef(const PicturePtr& picture, const H265SliceHdr *const slice)
{
    const H265PPS *const pps = slice->pps;
    const H265SPS *const sps = pps->sps;

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
        uint16_t poc;
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
                    - slice->pic_order_cnt_lsb;
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
    PictureList::iterator it =
            remove_if(m_pictures.begin(), m_pictures.end(), isUnusedPicture);
    m_pictures.erase(it, m_pictures.end());
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
     const H265SliceHdr *const slice,  const H265NalUnit *const nalu, bool newStream)
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

bool VaapiDecoderH265::DPB::checkReorderPics(const H265SPS* const sps)
{
    uint32_t num = count_if(m_pictures.begin(), m_pictures.end(), isOutputNeeded);
    return num > sps->max_num_reorder_pics[sps->max_sub_layers_minus1];
}

bool checkPicLatencyCount(const PicturePtr& picture, uint32_t spsMaxLatencyPictures)
{
    return isOutputNeeded(picture) && (picture->m_picLatencyCount >= spsMaxLatencyPictures);
}

bool VaapiDecoderH265::DPB::checkLatency(const H265SPS* const sps)
{
    uint8_t highestTid = sps->max_sub_layers_minus1;
    if (!sps->max_latency_increase_plus1[highestTid])
        return false;
    uint16_t spsMaxLatencyPictures = sps->max_num_reorder_pics[highestTid]
        + sps->max_latency_increase_plus1[highestTid] - 1;
    return find_if(m_pictures.begin(),
                   m_pictures.end(),
                   bind(checkPicLatencyCount, _1, spsMaxLatencyPictures)
                   ) != m_pictures.end();

}

bool VaapiDecoderH265::DPB::checkDpbSize(const H265SPS* const sps)
{
    uint8_t highestTid = sps->max_sub_layers_minus1;
    return m_pictures.size() >= (sps->max_dec_pic_buffering_minus1[highestTid] + 1);
}

//C.5.2.2
bool VaapiDecoderH265::DPB::init(const PicturePtr& picture,
     const H265SliceHdr *const slice,  const H265NalUnit *const nalu, bool newStream)
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
    const H265PPS* const pps = slice->pps;
    const H265SPS* const sps = pps->sps;
    while (checkReorderPics(sps) || checkLatency(sps) || checkDpbSize(sps))
        bump();

    return true;
}

bool VaapiDecoderH265::DPB::output(const PicturePtr& picture)
{
    picture->m_picOutputFlag = false;

    //    ERROR("DPB: output picture(Poc:%d)", picture->m_poc);
    return m_output(picture) == DECODE_SUCCESS;
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

bool VaapiDecoderH265::DPB::add(const PicturePtr& picture, const H265SliceHdr* const lastSlice)
{
    const H265PPS* const pps = lastSlice->pps;
    const H265SPS* const sps = pps->sps;

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
    m_newStream(true),
    m_dpb(bind(&VaapiDecoderH265::outputPicture, this, _1))
{
    m_parser = h265_parser_new();
    m_prevSlice = new H265SliceHdr;
    m_currSlice = new H265SliceHdr;
}

VaapiDecoderH265::~VaapiDecoderH265()
{
    stop();
    h265_parser_free(m_parser);
    delete m_prevSlice;
    delete m_currSlice;
}

Decode_Status VaapiDecoderH265::start(VideoConfigBuffer * buffer)
{
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH265::decodeParamSet(H265NalUnit *nalu)
{
    H265ParserResult result = h265_parser_parse_nal(m_parser, nalu);
    return result == H265_PARSER_OK ? DECODE_SUCCESS : DECODE_FAIL;
}

Decode_Status VaapiDecoderH265::outputPicture(const PicturePtr& picture)
{
    VaapiDecoderBase::PicturePtr base = std::tr1::static_pointer_cast<VaapiDecPicture>(picture);
    return VaapiDecoderBase::outputPicture(base);
}

Decode_Status VaapiDecoderH265::decodeCurrent()
{
    Decode_Status status = DECODE_SUCCESS;
    if (!m_current)
        return status;
    if (!m_current->decode()) {
        ERROR("decode %d failed", m_current->m_poc);
        //ignore it
        return status;
    }
    if (!m_dpb.add(m_current, m_prevSlice))
        return DECODE_FAIL;
    m_current.reset();
    m_newStream = false;
    return status;
}


#define FILL_SCALING_LIST(mxm) \
void fillScalingList##mxm(VAIQMatrixBufferHEVC* iqMatrix, const H265ScalingList* const scalingList) \
{ \
    for (int i = 0; i < N_ELEMENTS(iqMatrix->ScalingList##mxm); i++) { \
        h265_quant_matrix_##mxm##_get_raster_from_uprightdiagonal(iqMatrix->ScalingList##mxm[i], \
            scalingList->scaling_lists_##mxm[i]); \
    } \
}

FILL_SCALING_LIST(4x4)
FILL_SCALING_LIST(8x8)
FILL_SCALING_LIST(16x16)
FILL_SCALING_LIST(32x32)

#define FILL_SCALING_LIST_DC(mxm) \
void fillScalingListDc##mxm(VAIQMatrixBufferHEVC* iqMatrix, const H265ScalingList* const scalingList) \
{ \
    for (int i = 0; i < N_ELEMENTS(iqMatrix->ScalingListDC##mxm); i++) { \
        iqMatrix->ScalingListDC##mxm[i] = \
            scalingList->scaling_list_dc_coef_minus8_##mxm[i] + 8; \
    } \
}

FILL_SCALING_LIST_DC(16x16)
FILL_SCALING_LIST_DC(32x32)

bool VaapiDecoderH265::fillIqMatrix(const PicturePtr& picture, const H265SliceHdr* const slice)
{
    H265PPS* pps = slice->pps;
    H265SPS* sps = pps->sps;
    H265ScalingList* scalingList;
    if (pps->scaling_list_data_present_flag) {
        scalingList = &pps->scaling_list;
    } else if(sps->scaling_list_enabled_flag) {
        if(sps->scaling_list_data_present_flag) {
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
  //ERROR("fill ref(%x):", flags);
    for (int32_t i = 0; i < refset.size(); i++) {
        VAPictureHEVC* r = refs + n;
        const VaapiDecPictureH265* pic = refset[i];

        r->picture_id = refset[i]->getSurfaceID();
        r->pic_order_cnt = pic->m_poc;
        r->flags = flags;

        //record for late use
        m_pocToIndex[pic->m_poc] = n;
	//    ERROR("%d", pic->m_poc);

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
bool VaapiDecoderH265::fillPicture(const PicturePtr& picture, const H265SliceHdr* const slice)
{
    VAPictureParameterBufferHEVC* param;
    if (!picture->editPicture(param))
        return false;
    param->CurrPic.picture_id = picture->getSurfaceID();
    param->CurrPic.pic_order_cnt = picture->m_poc;
    fillReference(param->ReferenceFrames,  N_ELEMENTS(param->ReferenceFrames));

    H265PPS* pps = slice->pps;
    H265SPS* sps = pps->sps;
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
        = pps->loop_filter_across_slices_enabled_flag;
    FILL_PIC(pps, loop_filter_across_tiles_enabled_flag);
    FILL_PIC(sps, pcm_loop_filter_disabled_flag);
    //how to fill this?
    //NoPicReorderingFlag
    //NoBiPredFlag

    param->sps_max_dec_pic_buffering_minus1 =
        sps->max_dec_pic_buffering_minus1[0];
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
    param->pps_cb_qp_offset = pps->cb_qp_offset;
    param->pps_cr_qp_offset = pps->cr_qp_offset;
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
        pps->deblocking_filter_disabled_flag;
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
    param->pps_beta_offset_div2 = pps->beta_offset_div2;
    param->pps_tc_offset_div2 = pps->tc_offset_div2;
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
bool VaapiDecoderH265::fillReferenceIndex(VASliceParameterBufferHEVC* sliceParam, const H265SliceHdr* const slice)
{
    RefSet& before = m_dpb.m_stCurrBefore;
    RefSet& after = m_dpb.m_stCurrAfter;

    RefSet refset;
    if (!H265_IS_I_SLICE(slice)) {
        if (!getRefPicList(refset, before, after,
            slice->num_ref_idx_l0_active_minus1 + 1,
            slice->ref_pic_list_modification.ref_pic_list_modification_flag_l0,
            slice->ref_pic_list_modification.list_entry_l0)) {
            return false;
        }
    }
    fillReferenceIndexForList(sliceParam, refset, true);

    refset.clear();
    if (H265_IS_B_SLICE(slice)) {
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
        const H265SliceHdr* slice, uint8_t chromaLog2WeightDenom) \
{ \
    const H265PredWeightTable& w = slice->pred_weight_table; \
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
                        deltaOffset - (((128*chromaWeight)>>chromaLog2WeightDenom) + 128);\
\
                    sliceParam->delta_chroma_weight_l##n[i][j] = deltaWeight; \
                    sliceParam->ChromaOffsetL##n[i][j]= (int8_t)clip3(-128, 127, chromaOffset); \
            } \
        } \
    } \
}

FILL_WEIGHT_TABLE(0)
FILL_WEIGHT_TABLE(1)

bool VaapiDecoderH265::fillPredWeightTable(VASliceParameterBufferHEVC* sliceParam, const H265SliceHdr* const slice)
{
    H265PPS* pps = slice->pps;
    H265SPS* sps = pps->sps;
    const H265PredWeightTable& w = slice->pred_weight_table;
    if ((pps->weighted_pred_flag && H265_IS_P_SLICE (slice)) ||
            (pps->weighted_bipred_flag && H265_IS_B_SLICE (slice))) {
        uint8_t chromaLog2WeightDenom = w.luma_log2_weight_denom;
        sliceParam->luma_log2_weight_denom = w.luma_log2_weight_denom;
        if (sps->chroma_format_idc != 0) {
            sliceParam->delta_chroma_log2_weight_denom
                = w.delta_chroma_log2_weight_denom;
            chromaLog2WeightDenom
                += w.delta_chroma_log2_weight_denom;
        }
        fillPredWedightTableL0(sliceParam,  slice, chromaLog2WeightDenom);
        if (pps->weighted_bipred_flag && H265_IS_B_SLICE (slice))
            fillPredWedightTableL1(sliceParam,  slice, chromaLog2WeightDenom);
    }
    return true;
}

static inline uint32_t
getSliceDataByteOffset(const H265SliceHdr* const sliceHdr, uint32_t nalHeaderBytes)
{
    return nalHeaderBytes + (sliceHdr->header_size + 7) / 8
            - sliceHdr->n_emulation_prevention_bytes;
}

bool VaapiDecoderH265::fillSlice(const PicturePtr& picture,
        const H265SliceHdr* const theSlice, const H265NalUnit* const nalu)
{
    const H265SliceHdr* slice = theSlice;
    VASliceParameterBufferHEVC* sliceParam;
    if (!picture->newSlice(sliceParam, nalu->data + nalu->offset, nalu->size))
        return false;
    sliceParam->slice_data_byte_offset =
        getSliceDataByteOffset(slice, nalu->header_bytes);
    sliceParam->slice_segment_address = slice->segment_address;

#define FILL_LONG(f) sliceParam->LongSliceFlags.fields.f = slice->f
#define FILL_LONG_SLICE(f) sliceParam->LongSliceFlags.fields.slice_##f = slice->f
    //how to fill this
    //LastSliceOfPic
    FILL_LONG(dependent_slice_segment_flag);

    //follow spec
    if (slice->dependent_slice_segment_flag) {
        slice = m_prevSlice;
    }

    if (!fillReferenceIndex(sliceParam, slice))
        return false;

    FILL_LONG_SLICE(type);
    sliceParam->LongSliceFlags.fields.color_plane_id = slice->colour_plane_id;
    FILL_LONG_SLICE(sao_luma_flag);
    FILL_LONG_SLICE(sao_chroma_flag);
    FILL_LONG(mvd_l1_zero_flag);
    FILL_LONG(cabac_init_flag);
    FILL_LONG_SLICE(temporal_mvp_enabled_flag);
    FILL_LONG_SLICE(deblocking_filter_disabled_flag);
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

Decode_Status VaapiDecoderH265::ensureContext(const H265SPS* const sps)
{
    uint8_t surfaceNumber = sps->max_dec_pic_buffering_minus1[0] + 1 + H265_EXTRA_SURFACE_NUMBER;
    if (m_configBuffer.width != sps->width
        || m_configBuffer.height <  sps->height
        || m_configBuffer.surfaceNumber < surfaceNumber) {
        INFO("frame size changed, reconfig codec. orig size %d x %d, new size: %d x %d",
                m_configBuffer.width, m_configBuffer.height, sps->width, sps->height);
        Decode_Status status = VaapiDecoderBase::terminateVA();
        if (status != DECODE_SUCCESS)
            return status;
        m_configBuffer.width = sps->conformance_window_flag ? sps->crop_rect_width : sps->width;
        m_configBuffer.height = sps->conformance_window_flag ? sps->crop_rect_height : sps->height;
        m_configBuffer.surfaceWidth = sps->width;
        m_configBuffer.surfaceHeight =sps->height;
        m_configBuffer.flag |= HAS_SURFACE_NUMBER;
        m_configBuffer.profile = VAProfileHEVCMain;
        m_configBuffer.flag &= ~USE_NATIVE_GRAPHIC_BUFFER;
        m_configBuffer.surfaceNumber = surfaceNumber;
        status = VaapiDecoderBase::start(&m_configBuffer);
        if (status != DECODE_SUCCESS)
            return status;
        return DECODE_FORMAT_CHANGE;
    }
    return DECODE_SUCCESS;
}

/* 8.3.1 */
void VaapiDecoderH265::getPoc(const PicturePtr& picture,
        const H265SliceHdr* const slice,
        const H265NalUnit* const nalu)
{
    const H265PPS* const pps = slice->pps;
    const H265SPS* const sps = pps->sps;

    const uint16_t pocLsb = slice->pic_order_cnt_lsb;
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
    //ERROR("poc = %d", picture->m_poc);

    uint8_t temporalID = nalu->temporal_id_plus1 - 1;
    //fixme:sub-layer non-reference picture.
    if (!temporalID && !isRasl(nalu) &&  !isRadl(nalu) && !isSublayerNoRef(nalu)) {
        m_prevPicOrderCntMsb = picOrderCntMsb;
        m_prevPicOrderCntLsb = pocLsb;
    }
}

PicturePtr VaapiDecoderH265::createPicture(const H265SliceHdr* const slice,
        const H265NalUnit* const nalu)
{
    PicturePtr picture;
    SurfacePtr surface = createSurface();
    if (!surface)
        return picture;
    picture.reset(new VaapiDecPictureH265(m_context, surface, m_currentPTS));

    picture->m_noRaslOutputFlag = isIdr(nalu) || isBla(nalu) || m_newStream;
    picture->m_picOutputFlag
        = (isRasl(nalu) && picture->m_noRaslOutputFlag) ? false : slice->pic_output_flag;

    getPoc(picture, slice, nalu);

    return picture;
}

Decode_Status VaapiDecoderH265::decodeSlice(H265NalUnit *nalu)
{
    H265SliceHdr* slice = m_currSlice;
    H265ParserResult result;
    Decode_Status status;

    memset(slice, 0, sizeof(H265SliceHdr));
    result = h265_parser_parse_slice_hdr(m_parser, nalu, slice);
    if (result == H265_PARSER_ERROR) {
        return DECODE_FAIL;
    }
    status = ensureContext(slice->pps->sps);
    if (status != DECODE_SUCCESS) {
        return status;
    }
    if (slice->first_slice_segment_in_pic_flag) {
        status = decodeCurrent();
        if (status != DECODE_SUCCESS)
            return status;
        m_current = createPicture(slice, nalu);
        if (!m_current || !m_dpb.init(m_current, slice, nalu, m_newStream))
            return DECODE_FAIL;
        if (!fillPicture(m_current, slice) || !fillIqMatrix(m_current, slice))
            return DECODE_FAIL;
    }
    if (!m_current)
        return DECODE_FAIL;
    status = fillSlice(m_current, slice, nalu);
    if (status == DECODE_SUCCESS && !slice->dependent_slice_segment_flag)
        std::swap(m_currSlice, m_prevSlice);
    return status;

}

Decode_Status VaapiDecoderH265::decodeNalu(H265NalUnit *nalu)
{
    uint8_t type = nalu->type;
    Decode_Status status = DECODE_SUCCESS;

    if (H265_NAL_SLICE_TRAIL_N <= type && type <= H265_NAL_SLICE_CRA_NUT) {
        status = decodeSlice(nalu);
        if (status == DECODE_FAIL) {
            //ignore the decode slice failed
            m_current.reset();
            status = DECODE_SUCCESS;
        }
    } else {
        status = decodeCurrent();
        if (status != DECODE_SUCCESS)
            return status;
        switch (type) {
            case H265_NAL_VPS:
            case H265_NAL_SPS:
            case H265_NAL_PPS:
                status = decodeParamSet(nalu);
                break;
            case H265_NAL_AUD:
            case H265_NAL_EOS:
            case H265_NAL_EOB:
            case H265_NAL_FD:
            case H265_NAL_PREFIX_SEI:
            case H265_NAL_SUFFIX_SEI:
            default:
                break;
        }
    }

    return status;
}

Decode_Status VaapiDecoderH265::decode(VideoDecodeBuffer *buffer)
{
    if (!buffer || !buffer->data) {
        decodeCurrent();
        m_dpb.flush();
        m_prevPicOrderCntMsb = 0;
        m_prevPicOrderCntLsb = 0;
        m_newStream = true;
    }
    m_currentPTS = buffer->timeStamp;

    NalReader nr(buffer->data, buffer->size);
    const uint8_t* nal;
    int32_t size;
    Decode_Status status;
    while (nr.read(nal, size)) {
        H265NalUnit nalu;
        if (H265_PARSER_OK == h265_parser_identify_nalu_unchecked(m_parser, nal, 0, size, &nalu)) {
            status = decodeNalu(&nalu);
            if (status != DECODE_SUCCESS)
                return status;
        }
    }
    return DECODE_SUCCESS;
}

const bool VaapiDecoderH265::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderH265>(YAMI_MIME_H265);

}

