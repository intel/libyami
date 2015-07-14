/*
 *  vaapiencoder_hevc.cpp - hevc encoder for va
 *
 *  Copyright (C) 2014-2015 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
 *    Author: Li Zhong <zhong.li@intel.com>
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
#include "vaapiencoder_hevc.h"
#include <assert.h>
#include "bitwriter.h"
#include "scopedlogger.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapicodedbuffer.h"
#include "vaapiencpicture.h"
#include "vaapiencoder_factory.h"
#include <algorithm>
#include <tr1/functional>
namespace YamiMediaCodec{
//shortcuts
typedef VaapiEncoderHEVC::PicturePtr PicturePtr;
typedef VaapiEncoderHEVC::ReferencePtr ReferencePtr;
typedef VaapiEncoderHEVC::StreamHeaderPtr StreamHeaderPtr;

using std::list;
using std::vector;
using std::deque;

/* Define the maximum IDR period */
#define MAX_IDR_PERIOD 512

#define HEVC_NAL_START_CODE 0x000001

#define HEVC_SLICE_TYPE_I            2
#define HEVC_SLICE_TYPE_P           1
#define HEVC_SLICE_TYPE_B           0

#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_NONE        0
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_LOW         1
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_MEDIUM      2
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_HIGH        3

typedef enum
{
    TRAIL_N = 0,
    TRAIL_R = 1,
    TSA_N = 2,
    TSA_R = 3 ,
    STSA_N = 4,
    STSA_R = 5,
    RADL_N = 6,
    RADL_R = 7,
    RASL_N = 8,
    RASL_R = 9,
    RSV_VCL_N10 = 10,
    RSV_VCL_N11 = 11,
    RSV_VCL_N12 = 12,
    RSV_VCL_N13 = 13,
    RSV_VCL_N14 = 14,
    RSV_VCL_N15 = 15,
    BLA_W_LP = 16,
    BLA_W_RADL = 17,
    BLA_N_LP = 18,
    IDR_W_RADL = 19,
    IDR_N_LP = 20,
    CRA_NUT = 21,
    RSV_IRAP_VCL22 = 22,
    RSV_IRAP_VCL23 = 23,
    RSV_VCL24 = 24,
    RSV_VCL25 = 25,
    RSV_VCL26 = 26,
    RSV_VCL27 = 27,
    RSV_VCL28 = 28,
    RSV_VCL29 = 29,
    RSV_VCL30 = 30,
    RSV_VCL31 = 31,
    VPS_NUT = 32,
    SPS_NUT = 33,
    PPS_NUT = 34,
    AUD_NUT = 35,
    EOS_NUT = 36,
    EOB_NUT = 37,
    FD_NUT = 38,
    PREFIX_SEI_NUT = 39,
    SUFFIX_SEI_NUT = 40
}HevcNalUnitType;

static uint8_t
hevc_get_slice_type (VaapiPictureType type)
{
    switch (type) {
    case VAAPI_PICTURE_TYPE_I:
        DEBUG("HEVC_SLICE_TYPE_I \n");
        return HEVC_SLICE_TYPE_I;
    case VAAPI_PICTURE_TYPE_P:
        DEBUG("HEVC_SLICE_TYPE_P \n");
        return HEVC_SLICE_TYPE_P;
    case VAAPI_PICTURE_TYPE_B:
        DEBUG("HEVC_SLICE_TYPE_B \n");
        return HEVC_SLICE_TYPE_B;
    default:
        return -1;
    }
    return -1;
}

static uint32_t log2 (uint32_t num)
{
    uint32_t ret = 0;
    assert(num);

    while (num > (1 << ret))
        ++ret;

    return ret;
}

static uint32_t
hevc_get_log2_max_frame_num (uint32_t num)
{
    uint32_t ret = 0;

    ret = log2(num);

    if (ret <= 4)
        ret = 4;
    else if (ret > 10)
        ret = 10;
    /* must be greater than 4 */
    return ret;
}

static uint8_t hevc_get_profile_idc (VaapiProfile profile)
{
    uint8_t idc;
    switch (profile) {
    case VAAPI_PROFILE_HEVC_MAIN:
        idc =  1;
        break;
    case VAAPI_PROFILE_HEVC_MAIN10:
        idc = 2;
    default:
        assert(0);
    }
    return idc;

}

static BOOL
bit_writer_put_ue(BitWriter *bitwriter, uint32_t value)
{
    uint32_t  size_in_bits = 0;
    uint32_t  tmp_value = ++value;

    while (tmp_value) {
        ++size_in_bits;
        tmp_value >>= 1;
    }
    if (size_in_bits > 1
        && !bit_writer_put_bits_uint32(bitwriter, 0, size_in_bits-1))
        return FALSE;
    if (!bit_writer_put_bits_uint32(bitwriter, value, size_in_bits))
        return FALSE;
    return TRUE;
}

static BOOL
bit_writer_put_se(BitWriter *bitwriter, int32_t value)
{
    uint32_t new_val;

    if (value <= 0)
        new_val = -(value<<1);
    else
        new_val = (value<<1) - 1;

    if (!bit_writer_put_ue(bitwriter, new_val))
        return FALSE;
    return TRUE;
}

static BOOL
bit_writer_write_nal_header(
    BitWriter *bitwriter,
    uint32_t nal_unit_type
)
{
    /* forbidden_zero_bit */
    bit_writer_put_bits_uint32(bitwriter, 0, 1);
    /* nal unit_type */
    bit_writer_put_bits_uint32(bitwriter, nal_unit_type, 6);
    /* layer_id */
    bit_writer_put_bits_uint32(bitwriter, 0, 6);
    /* temporal_id_plus1*/
    bit_writer_put_bits_uint32(bitwriter, 1, 3);

    return TRUE;
}

static BOOL
bit_writer_write_trailing_bits(BitWriter *bitwriter)
{
    bit_writer_put_bits_uint32(bitwriter, 1, 1);
    bit_writer_align_bytes_unchecked(bitwriter, 0);
    return TRUE;
}

static void profile_tier_level(
    BitWriter *bitwriter,
    BOOL profile_present_flag,
    uint32_t max_num_sub_layers,
    const VAEncSequenceParameterBufferHEVC* const seq
)
{
    uint32_t i;
    bool general_profile_compatibility_flag[32] = {0};

    unsigned char vps_general_level_idc = seq->general_level_idc * 3;

    general_profile_compatibility_flag[1] = general_profile_compatibility_flag[2] = 1;

    if (profile_present_flag) {
        /* general_profile_space */
        bit_writer_put_bits_uint32(bitwriter, 0, 2);
        /* general_tier_flag */
        bit_writer_put_bits_uint32(bitwriter, seq->general_tier_flag, 1);
        /* general_profile_idc */
        bit_writer_put_bits_uint32(bitwriter, seq->general_profile_idc, 5);

        /* general_profile_compatibility_flag. Only the bit corresponding to profile_idc is set */
        for (i = 0; i < N_ELEMENTS(general_profile_compatibility_flag); i++) {
            bit_writer_put_bits_uint32(bitwriter, general_profile_compatibility_flag[i], 1);
        }

        /* general_progressive_source_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);
        /* general_interlaced_source_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);
        /* general_non_packed_constraint_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);
        /* general_frame_only_constraint_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);

        /* reserved zero 44bits */
        bit_writer_put_bits_uint32(bitwriter, 0, 12);
        bit_writer_put_bits_uint32(bitwriter, 0, 16);
        bit_writer_put_bits_uint32(bitwriter, 0, 16);
    }

    /* general_level_idc */
    bit_writer_put_bits_uint32(bitwriter, vps_general_level_idc, 8);

    if (max_num_sub_layers) {
        /* TBD: can't support multi-layers now
        */
    }
}

void st_ref_pic_set(BitWriter *bs, int stRpsIdx, const ShortRFS& shortRFS)
{
    int i;
    if (stRpsIdx)
        bit_writer_put_bits_uint32(bs, shortRFS.inter_ref_pic_set_prediction_flag, 1);

    ASSERT(!shortRFS.inter_ref_pic_set_prediction_flag);

    bit_writer_put_ue(bs, shortRFS.num_negative_pics);
    bit_writer_put_ue(bs, shortRFS.num_positive_pics);

    for (i = 0; i < shortRFS.num_negative_pics; i++)
    {
        bit_writer_put_ue(bs, shortRFS.delta_poc_s0_minus1[i]);
        bit_writer_put_bits_uint32(bs, shortRFS.used_by_curr_pic_s0_flag[i], 1);
    }
    for (i = 0; i < shortRFS.num_positive_pics; i++)
    {
        bit_writer_put_ue(bs, shortRFS.delta_poc_s1_minus1[i]);
        bit_writer_put_bits_uint32(bs, shortRFS.used_by_curr_pic_s1_flag[i], 1);
    }

    return;
}


class VaapiEncStreamHeaderHEVC
{
    typedef std::vector<uint8_t> Header;
public:

    VaapiEncStreamHeaderHEVC();
    VaapiEncStreamHeaderHEVC(VaapiEncoderHEVC* encoder) {m_encoder = encoder;};

    void setVPS(const VAEncSequenceParameterBufferHEVC* const sequence)
    {
        ASSERT(m_vps.empty());
        BitWriter bs;
        bit_writer_init (&bs, 128 * 8);
        bit_writer_write_vps (&bs, sequence);
        bsToHeader(m_vps, bs);
        bit_writer_clear (&bs, TRUE);
    }

    void setSPS(const VAEncSequenceParameterBufferHEVC* const sequence)
    {
        ASSERT(m_vps.size() && m_sps.empty());
        BitWriter bs;
        bit_writer_init (&bs, 128 * 8);
        bit_writer_write_sps (&bs, sequence);
        bsToHeader(m_sps, bs);
        bit_writer_clear (&bs, TRUE);
    }

    void addPPS(const VAEncPictureParameterBufferHEVC* const picParam)
    {
        ASSERT(m_sps.size() && m_pps.empty());
        BitWriter bs;
        bit_writer_init (&bs, 128 * 8);
        bit_writer_write_pps (&bs, picParam);
        bsToHeader(m_pps, bs);
        bit_writer_clear (&bs, TRUE);
    }

    void generateCodecConfig()
    {
        std::vector<Header*> headers;

        ASSERT(m_vps.size() && m_sps.size() && (m_sps.size() > 4)&& m_pps.size() && m_headers.empty());

        headers.push_back(&m_vps);
        headers.push_back(&m_sps);
        headers.push_back(&m_pps);
        uint8_t sync[] = {0, 0, 0, 1};
        for (int i = 0; i < headers.size(); i++) {
            m_headers.insert(m_headers.end(), sync, sync + N_ELEMENTS(sync));
            appendHeaderWithEmulation(*headers[i]);
        }
    }

    Encode_Status getCodecConfig(VideoEncOutputBuffer *outBuffer)
    {
        ASSERT(outBuffer && (outBuffer->format == OUTPUT_CODEC_DATA || outBuffer->format == OUTPUT_EVERYTHING));
        if (outBuffer->bufferSize < m_headers.size())
            return ENCODE_BUFFER_TOO_SMALL;
        if (m_headers.empty())
            return ENCODE_NO_REQUEST_DATA;
        std::copy(m_headers.begin(), m_headers.end(), outBuffer->data);
        outBuffer->dataSize = m_headers.size();
        outBuffer->flag |= ENCODE_BUFFERFLAG_CODECCONFIG;
        return ENCODE_SUCCESS;
    }
private:
    BOOL bit_writer_write_vps (
        BitWriter *bitwriter,
        const VAEncSequenceParameterBufferHEVC* const seq
    )
    {
        BOOL vps_timing_info_present_flag = 0;

        bit_writer_write_nal_header(bitwriter, VPS_NUT);

        /* vps_video_parameter_set_id */
        bit_writer_put_bits_uint32(bitwriter, 0, 4);
        /* vps_base_layer_internal_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);
        /* vps_base_layer_available_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);
        /* vps_max_layers_minus1 */
        bit_writer_put_bits_uint32(bitwriter, 0, 6);
        /* vps_max_sub_layers_minus1 */
        bit_writer_put_bits_uint32(bitwriter, 0, 3);
        /* vps_temporal_id_nesting_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);

        /* vps_reserved_0xffff_16bits */
        bit_writer_put_bits_uint32(bitwriter, 0xffff, 16);

        profile_tier_level(bitwriter, 1, 0, seq);

        /* vps_sub_layer_ordering_info_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /*vps_max_dec_pic_buffering_minus1*/
        bit_writer_put_ue(bitwriter, 5);

        /*vps_max_num_reorder_pics*/
        bit_writer_put_ue(bitwriter, 0);

        /*vps_max_latency_increase_plus1*/
        bit_writer_put_ue(bitwriter, 0);

        /* vps_max_layer_id */
        bit_writer_put_bits_uint32(bitwriter, 0, 6);

        /* vps_num_layer_sets_minus1 */
        bit_writer_put_ue(bitwriter, 0);

        /* vps_timing_info_present_flag */
        bit_writer_put_bits_uint32(bitwriter, vps_timing_info_present_flag, 1);

        if (vps_timing_info_present_flag) {
            /* TBD: add it later for BRC */
        }

        /* vps_extension_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* rbsp_trailing_bits */
        bit_writer_write_trailing_bits(bitwriter);

        return TRUE;
    }

    BOOL bit_writer_write_sps(
        BitWriter *bitwriter,
        const VAEncSequenceParameterBufferHEVC* const seq
    )
    {
        uint32_t i = 0;

        bit_writer_write_nal_header(bitwriter, SPS_NUT);

        /* sps_video_parameter_set_id */
        bit_writer_put_bits_uint32(bitwriter, 0, 4);
        /* sps_max_sub_layers_minus1 */
        bit_writer_put_bits_uint32(bitwriter, 0, 3);
        /* sps_temporal_id_nesting_flag */
        bit_writer_put_bits_uint32(bitwriter, 1, 1);

        profile_tier_level(bitwriter, 1, 0, seq);

        /* seq_parameter_set_id */
        bit_writer_put_ue(bitwriter, 0);
        /* chroma_format_idc: only support 4:2:0 for libva */
        bit_writer_put_ue(bitwriter, seq->seq_fields.bits.chroma_format_idc);
        if (3 == seq->seq_fields.bits.chroma_format_idc) {
            bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.separate_colour_plane_flag, 1);
        }
        /* pic_width_in_luma_samples */
        bit_writer_put_ue(bitwriter, seq->pic_width_in_luma_samples);
        /* pic_height_in_luma_samples */
        bit_writer_put_ue(bitwriter, seq->pic_height_in_luma_samples);

        /* conformance_window_flag */
        bit_writer_put_bits_uint32(bitwriter, m_encoder->m_confWinFlag, 1);

        if (m_encoder->m_confWinFlag) {
            bit_writer_put_ue(bitwriter, m_encoder->m_confWinLeftOffset);
            bit_writer_put_ue(bitwriter, m_encoder->m_confWinRightOffset);
            bit_writer_put_ue(bitwriter, m_encoder->m_confWinTopOffset);
            bit_writer_put_ue(bitwriter, m_encoder->m_confWinBottomOffset);
        }

        /* bit_depth_luma_minus8 */
        bit_writer_put_ue(bitwriter, seq->seq_fields.bits.bit_depth_luma_minus8);
        /* bit_depth_chroma_minus8 */
        bit_writer_put_ue(bitwriter, seq->seq_fields.bits.bit_depth_chroma_minus8);

        /* log2_max_pic_order_cnt_lsb_minus4 */
        assert(m_encoder->m_log2MaxPicOrderCnt >= 4);
        bit_writer_put_ue(bitwriter, m_encoder->m_log2MaxPicOrderCnt - 4);

        /* sps_sub_layer_ordering_info_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

       /* sps_max_dec_pic_buffering_minus1 */
       bit_writer_put_ue(bitwriter, 5);
       /* sps_max_num_reorder_pics */
       bit_writer_put_ue(bitwriter, 0);
       /* sps_max_latency_increase_plus1 */
       bit_writer_put_ue(bitwriter, 0);

        bit_writer_put_ue(bitwriter, seq->log2_min_luma_coding_block_size_minus3);
        bit_writer_put_ue(bitwriter, seq->log2_diff_max_min_luma_coding_block_size);
        bit_writer_put_ue(bitwriter, seq->log2_min_transform_block_size_minus2);
        bit_writer_put_ue(bitwriter, seq->log2_diff_max_min_transform_block_size);
        bit_writer_put_ue(bitwriter, seq->max_transform_hierarchy_depth_inter);
        bit_writer_put_ue(bitwriter, seq->max_transform_hierarchy_depth_intra);

        /* scaling_list_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);
        /* amp_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.amp_enabled_flag, 1);
        /* sample_adaptive_offset_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.sample_adaptive_offset_enabled_flag, 1);
        /* pcm_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.pcm_enabled_flag, 1);

        if (seq->seq_fields.bits.pcm_enabled_flag) {
            /* pcm_sample_bit_depth_luma_minus1 */
            bit_writer_put_bits_uint32(bitwriter, seq->pcm_sample_bit_depth_luma_minus1, 4);
            /* pcm_sample_bit_depth_luma_minus1 */
            bit_writer_put_bits_uint32(bitwriter, seq->pcm_sample_bit_depth_chroma_minus1, 4);
            /* log2_min_pcm_luma_coding_block_size_minus3 */
            bit_writer_put_ue(bitwriter, seq->log2_min_pcm_luma_coding_block_size_minus3);
            /* log2_diff_max_min_pcm_luma_coding_block_size */
            bit_writer_put_ue(bitwriter, seq->log2_max_pcm_luma_coding_block_size_minus3 - seq->log2_min_pcm_luma_coding_block_size_minus3);
            /* pcm_loop_filter_disabled_flag */
            bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.pcm_loop_filter_disabled_flag, 1);
        }

        bit_writer_put_ue(bitwriter, m_encoder->m_shortRFS.num_short_term_ref_pic_sets);
        for (i = 0; i < m_encoder->m_shortRFS.num_short_term_ref_pic_sets; i++)
            st_ref_pic_set(bitwriter, i, m_encoder->m_shortRFS);

        /* long_term_ref_pics_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /*sps_temporal_mvp_enabled_flag*/
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* strong_intra_smoothing_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, seq->seq_fields.bits.strong_intra_smoothing_enabled_flag, 1);

        /* vui_parameters_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* sps_extension_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* rbsp_trailing_bits */
        bit_writer_write_trailing_bits(bitwriter);
        return TRUE;
    }

    BOOL bit_writer_write_pps(
        BitWriter *bitwriter,
        const VAEncPictureParameterBufferHEVC* const pic
    )
    {
        uint32_t deblocking_filter_control_present_flag  = 0;
        uint32_t pps_slice_chroma_qp_offsets_present_flag = 1;

        bit_writer_write_nal_header(bitwriter, PPS_NUT);

        /* pps_pic_parameter_set_id */
        bit_writer_put_ue(bitwriter, 0);
        /* pps_seq_parameter_set_id */
        bit_writer_put_ue(bitwriter, 0);

        /* dependent_slice_segments_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.dependent_slice_segments_enabled_flag, 1);

        /* output_flag_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* num_extra_slice_header_bits */
        bit_writer_put_bits_uint32(bitwriter, 0, 3);

        /* sign_data_hiding_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.sign_data_hiding_enabled_flag, 1);

        /* cabac_init_present_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        bit_writer_put_ue(bitwriter, pic->num_ref_idx_l0_default_active_minus1);
        bit_writer_put_ue(bitwriter, pic->num_ref_idx_l1_default_active_minus1);

        /* init_qp_minus26 */
        bit_writer_put_se(bitwriter, pic->pic_init_qp-26);

        /* constrained_intra_pred_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.constrained_intra_pred_flag, 1);

        /* transform_skip_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.transform_skip_enabled_flag, 1);

        /* cu_qp_delta_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.cu_qp_delta_enabled_flag, 1);

        if (pic->pic_fields.bits.cu_qp_delta_enabled_flag) {
            /* diff_cu_qp_delta_depth */
            bit_writer_put_ue(bitwriter, pic->diff_cu_qp_delta_depth);
        }

        /* pps_cb_qp_offset */
        bit_writer_put_se(bitwriter, pic->pps_cb_qp_offset);

        /* pps_cr_qp_offset */
        bit_writer_put_se(bitwriter, pic->pps_cr_qp_offset);

        /* pps_slice_chroma_qp_offsets_present_flag */
        bit_writer_put_bits_uint32(bitwriter, pps_slice_chroma_qp_offsets_present_flag, 1);

        /* weighted_pred_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.weighted_pred_flag, 1);

        /* weighted_bipred_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.weighted_bipred_flag, 1);

        /* transquant_bypass_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.transquant_bypass_enabled_flag, 1);

        /* tiles_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.tiles_enabled_flag, 1);

        /* entropy_coding_sync_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.entropy_coding_sync_enabled_flag, 1);

        if (pic->pic_fields.bits.tiles_enabled_flag) {
            /*
            * TBD: Add the tile division when tiles are enabled.
            */
            assert(!pic->pic_fields.bits.tiles_enabled_flag);
        }

        /* pps_loop_filter_across_slices_enabled_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.loop_filter_across_tiles_enabled_flag, 1);

        /* deblocking_filter_control_present_flag. 1 */
        bit_writer_put_bits_uint32(bitwriter, deblocking_filter_control_present_flag, 1);
        if (deblocking_filter_control_present_flag) {
            /* deblocking_filter_override_enabled_flag */
            bit_writer_put_bits_uint32(bitwriter, 0, 1);
            /* pps_deblocking_filter_disabled_flag */
            bit_writer_put_bits_uint32(bitwriter, 0, 1);
        }

        /* scaling_list_data_present_flag */
        bit_writer_put_bits_uint32(bitwriter, pic->pic_fields.bits.scaling_list_data_present_flag, 1);

        if (pic->pic_fields.bits.scaling_list_data_present_flag) {
            /*
            * TBD: Add the scaling list data for PPS
            */
            assert(!pic->pic_fields.bits.scaling_list_data_present_flag);
        }

        /* lists_modification_present_flag 0/1 ?*/
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* log2_parallel_merge_level_minus2: 2 - 2 */
        bit_writer_put_ue(bitwriter, 0);

        /* slice_segment_header_extension_present_flag. Zero */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        /* pps_extension_flag */
        bit_writer_put_bits_uint32(bitwriter, 0, 1);

        bit_writer_write_trailing_bits(bitwriter);

        return TRUE;
    }


    void bsToHeader(Header& param, BitWriter& bs)
    {
        ASSERT(BIT_WRITER_BIT_SIZE (&bs) % 8 == 0);
        param.insert(param.end(), BIT_WRITER_DATA (&bs),  BIT_WRITER_DATA (&bs) + BIT_WRITER_BIT_SIZE (&bs)/8);
    }

    void appendHeaderWithEmulation(Header& h)
    {
        Header::iterator s = h.begin();
        Header::iterator e;
        uint8_t zeros[] = {0, 0};
        uint8_t emulation[] = {0, 0, 3};
        do {
            e = std::search(s, h.end(), zeros, zeros + N_ELEMENTS(zeros));
            m_headers.insert(m_headers.end(), s, e);
            if (e == h.end())
                break;

            s = e + N_ELEMENTS(zeros);

            /* only when bitstream contains 0x000000/0x000001/0x000002/0x000003
               need to insert emulation prevention byte 0x03 */
            if (*s <= 3)
                m_headers.insert(m_headers.end(), emulation, emulation + N_ELEMENTS(emulation));
            else
                m_headers.insert(m_headers.end(), zeros, zeros + N_ELEMENTS(zeros));
         } while (1);
    }

    Header m_vps;
    Header m_sps;
    Header m_pps;
    Header m_headers;
    VaapiEncoderHEVC* m_encoder;
};

class VaapiEncPictureHEVC:public VaapiEncPicture
{
    friend class VaapiEncoderHEVC;
    friend class VaapiEncoderHEVCRef;
    typedef std::tr1::function<Encode_Status ()> Function;

public:
    virtual ~VaapiEncPictureHEVC() {}

    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer)
    {
        ASSERT(outBuffer);
        VideoOutputFormat format = outBuffer->format;
        //make a local copy of out Buffer;
        VideoEncOutputBuffer out = *outBuffer;
        out.flag = 0;

        std::vector<Function> functions;
        if (format == OUTPUT_CODEC_DATA || ((format == OUTPUT_EVERYTHING) && isIdr()))
            functions.push_back(std::tr1::bind(&VaapiEncStreamHeaderHEVC::getCodecConfig, m_headers,&out));
        if (format == OUTPUT_EVERYTHING || format == OUTPUT_FRAME_DATA)
            functions.push_back(std::tr1::bind(getOutputHelper, this, &out));
        Encode_Status ret = getOutput(&out, functions);
        if (ret == ENCODE_SUCCESS) {
            outBuffer->dataSize = out.data - outBuffer->data;
            outBuffer->flag = out.flag;
        }
        return ret;
    }

private:
    VaapiEncPictureHEVC(const ContextPtr& context, const SurfacePtr& surface, int64_t timeStamp):
        VaapiEncPicture(context, surface, timeStamp),
        m_frameNum(0),
        m_poc(0)
    {
    }

    bool isIdr() const {
        return m_type == VAAPI_PICTURE_TYPE_I && !m_frameNum;
    }

    //getOutput is a virutal function, we need this to help bind
    static Encode_Status getOutputHelper(VaapiEncPictureHEVC* p, VideoEncOutputBuffer* out)
    {
        return p->VaapiEncPicture::getOutput(out);
    }

    Encode_Status getOutput(VideoEncOutputBuffer * outBuffer, std::vector<Function>& functions)
    {
        ASSERT(outBuffer);

        outBuffer->dataSize = 0;

        Encode_Status ret;
        for (int i = 0; i < functions.size(); i++) {
            ret = functions[i]();
            if (ret != ENCODE_SUCCESS)
                return ret;
            outBuffer->bufferSize -= outBuffer->dataSize;
            outBuffer->data += outBuffer->dataSize;
        }
        return ENCODE_SUCCESS;
    }

    uint32_t m_frameNum;
    uint32_t m_poc;
    StreamHeaderPtr m_headers;
};

class VaapiEncoderHEVCRef
{
public:
    VaapiEncoderHEVCRef(const PicturePtr& picture, const SurfacePtr& surface):
        m_frameNum(picture->m_frameNum),
        m_poc(picture->m_poc),
        m_pic(surface)
    {
    }
    uint32_t m_frameNum;
    uint32_t m_poc;
    SurfacePtr m_pic;
};

VaapiEncoderHEVC::VaapiEncoderHEVC():
    m_numBFrames(0),
    m_ctbSize(8),
    m_cuSize(32),
    m_minTbSize(4),
    m_maxTbSize(16),
    m_streamFormat(AVC_STREAM_FORMAT_ANNEXB),
    m_reorderState(VAAPI_ENC_REORD_WAIT_FRAMES)
{
    m_videoParamCommon.profile = VAProfileHEVCMain;
    m_videoParamCommon.level = 51;
    m_videoParamCommon.rcParams.initQP = 26;
    m_videoParamCommon.rcParams.minQP = 1;

    m_videoParamAVC.idrInterval = 30;
}

VaapiEncoderHEVC::~VaapiEncoderHEVC()
{
    FUNC_ENTER();
}

bool VaapiEncoderHEVC::ensureCodedBufferSize()
{
    AutoLock locker(m_paramLock);

    FUNC_ENTER();

    if (m_maxCodedbufSize)
        return true;

    m_maxCodedbufSize = m_AlignedWidth * m_AlignedHeight * 3 / 2 + 0x1000;

    DEBUG("m_maxCodedbufSize: %u", m_maxCodedbufSize);

    return true;
}

void VaapiEncoderHEVC::resetParams ()
{

    m_levelIdc = level();
    m_profileIdc = hevc_get_profile_idc(profile());

    m_numSlices = 1;

    assert (width() && height());

    /* libva driver requred pic_width_in_luma_samples 16 aligned, not ctb aligned */
    m_AlignedWidth = (width() + 15) / 16 * 16;
    m_AlignedHeight = (height() + 15) / 16 * 16;

    m_cuWidth = (width() + m_cuSize -1) / m_cuSize;
    m_cuHeight = (height() + m_cuSize-1) / m_cuSize;

    m_confWinLeftOffset = m_confWinTopOffset = 0;

    if (m_AlignedWidth != width() || m_AlignedHeight !=height()) {
            m_confWinFlag = true;
            m_confWinRightOffset = (m_AlignedWidth - width()) / 2;
            m_confWinBottomOffset = (m_AlignedHeight -height()) / 2;
    } else {
        m_confWinFlag = false;
        m_confWinRightOffset = 0;
        m_confWinBottomOffset = 0;
    }

    if (ipPeriod() == 0)
        m_videoParamCommon.intraPeriod = 1;
    else if (ipPeriod() >= 1)
        m_numBFrames = ipPeriod() - 1;

    assert(intraPeriod() > ipPeriod());

    /* we set all I frame to be IDR frame right now */
    keyFramePeriod() = intraPeriod();
    if (keyFramePeriod() > MAX_IDR_PERIOD)
        keyFramePeriod() = MAX_IDR_PERIOD;

    if (minQP() > initQP() ||
            (rateControlMode()== RATE_CONTROL_CQP && minQP() < initQP()))
        minQP() = initQP();

    if (m_numBFrames > (intraPeriod() + 1) / 2)
        m_numBFrames = (intraPeriod() + 1) / 2;

    DEBUG("resetParams, ensureCodedBufferSize");
    ensureCodedBufferSize();

    /* init m_maxFrameNum, max_poc */
    m_log2MaxFrameNum =
        hevc_get_log2_max_frame_num (keyFramePeriod());
    assert (m_log2MaxFrameNum >= 4);
    m_maxFrameNum = (1 << m_log2MaxFrameNum);
    m_log2MaxPicOrderCnt = m_log2MaxFrameNum + 1;
    m_maxPicOrderCnt = (1 << m_log2MaxPicOrderCnt);

    m_maxRefList0Count = 1;
    m_maxRefList1Count = m_numBFrames > 0;
    m_maxRefFrames = m_maxRefList0Count + m_maxRefList1Count;

    INFO("m_maxRefFrames: %d", m_maxRefFrames);

    setShortRfs();

    resetGopStart();
}

Encode_Status VaapiEncoderHEVC::getMaxOutSize(uint32_t *maxSize)
{
    FUNC_ENTER();

    if (ensureCodedBufferSize())
        *maxSize = m_maxCodedbufSize;
    else
        *maxSize = 0;

    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderHEVC::start()
{
    FUNC_ENTER();
    resetParams();
    return VaapiEncoderBase::start();
}

void VaapiEncoderHEVC::flush()
{
    FUNC_ENTER();
    resetGopStart();
    m_reorderFrameList.clear();
    referenceListFree();
    VaapiEncoderBase::flush();
}

Encode_Status VaapiEncoderHEVC::stop()
{
    flush();
    return VaapiEncoderBase::stop();
}

Encode_Status VaapiEncoderHEVC::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    Encode_Status status = ENCODE_INVALID_PARAMS;
    AutoLock locker(m_paramLock);

    FUNC_ENTER();
    if (!videoEncParams)
        return ENCODE_INVALID_PARAMS;
    switch (type) {
    case VideoParamsTypeAVC: {
            VideoParamsAVC* avc = (VideoParamsAVC*)videoEncParams;
            if (avc->size == sizeof(VideoParamsAVC)) {
                PARAMETER_ASSIGN(*avc, m_videoParamAVC);
                status = ENCODE_SUCCESS;
            }
        }
        break;
    case VideoConfigTypeAVCIntraPeriod: {
            VideoConfigAVCIntraPeriod* intraPeriod = (VideoConfigAVCIntraPeriod*)videoEncParams;
            if (intraPeriod->size == sizeof(VideoConfigAVCIntraPeriod)) {
                m_videoParamAVC.idrInterval = intraPeriod->idrInterval;
                m_videoParamCommon.intraPeriod = intraPeriod->intraPeriod;
                status = ENCODE_SUCCESS;
            }
        }
        break;
    default:
        status = VaapiEncoderBase::setParameters(type, videoEncParams);
        break;
    }
    return status;
}

Encode_Status VaapiEncoderHEVC::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    Encode_Status status = ENCODE_INVALID_PARAMS;
    AutoLock locker(m_paramLock);

    FUNC_ENTER();
    if (!videoEncParams)
        return status;
    switch (type) {
    case VideoParamsTypeAVC: {
            VideoParamsAVC* avc = (VideoParamsAVC*)videoEncParams;
            if (avc->size == sizeof(VideoParamsAVC)) {
                PARAMETER_ASSIGN(*avc, m_videoParamAVC);
                status = ENCODE_SUCCESS;
            }
        }
        break;
    case VideoConfigTypeAVCStreamFormat: {
            VideoConfigAVCStreamFormat* format = (VideoConfigAVCStreamFormat*)videoEncParams;
            if (format->size == sizeof(VideoConfigAVCStreamFormat)) {
                format->streamFormat = m_streamFormat;
                status = ENCODE_SUCCESS;
            }
        }
        break;
    default:
        status = VaapiEncoderBase::getParameters(type, videoEncParams);
        break;
    }

    return status;
}

Encode_Status VaapiEncoderHEVC::reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    if (!surface)
        return ENCODE_INVALID_PARAMS;

    PicturePtr picture(new VaapiEncPictureHEVC(m_context, surface, timeStamp));

    bool isIdr = (m_frameIndex == 0 ||m_frameIndex >= keyFramePeriod() || forceKeyFrame);

    /* check key frames */
    if (isIdr || (m_frameIndex % intraPeriod() == 0)) {
        setIntraFrame (picture, isIdr);
        m_reorderFrameList.push_back(picture);
        m_reorderState = VAAPI_ENC_REORD_DUMP_FRAMES;
    } else if (m_frameIndex % (m_numBFrames + 1) != 0) {
        setBFrame (picture);
        m_reorderFrameList.push_back(picture);
    } else {
        setPFrame (picture);
        m_reorderFrameList.push_front(picture);
        m_reorderState = VAAPI_ENC_REORD_DUMP_FRAMES;
    }

    DEBUG("m_frameIndex is %d\n", m_frameIndex);
    picture->m_poc = ((m_frameIndex) % m_maxPicOrderCnt);
    m_frameIndex++;
    return ENCODE_SUCCESS;
}

// calls immediately after reorder,
// it makes sure I frame are encoded immediately, so P frames can be pushed to the front of the m_reorderFrameList.
// it also makes sure input thread and output thread runs in parallel
Encode_Status VaapiEncoderHEVC::doEncode(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    FUNC_ENTER();
    Encode_Status ret;
    ret = reorder(surface, timeStamp, forceKeyFrame);
    if (ret != ENCODE_SUCCESS)
        return ret;

    while (m_reorderState == VAAPI_ENC_REORD_DUMP_FRAMES) {
        if (!m_maxCodedbufSize)
            ensureCodedBufferSize();
        ASSERT(m_maxCodedbufSize);
        CodedBufferPtr codedBuffer = VaapiCodedBuffer::create(m_context, m_maxCodedbufSize);
        if (!codedBuffer)
            return ENCODE_NO_MEMORY;
        DEBUG("m_reorderFrameList size: %d\n", m_reorderFrameList.size());
        PicturePtr picture = m_reorderFrameList.front();
        m_reorderFrameList.pop_front();
        picture->m_codedBuffer = codedBuffer;

        if (m_reorderFrameList.empty())
            m_reorderState = VAAPI_ENC_REORD_WAIT_FRAMES;

        ret =  encodePicture(picture);
        if (ret != ENCODE_SUCCESS) {
            return ret;
        }
        codedBuffer->setFlag(ENCODE_BUFFERFLAG_ENDOFFRAME);
        INFO("picture->m_type: 0x%x\n", picture->m_type);
        if (picture->isIdr()) {
            codedBuffer->setFlag(ENCODE_BUFFERFLAG_SYNCFRAME);
        }

        if (!output(picture))
            return ENCODE_INVALID_PARAMS;
    }

    INFO();
    return ENCODE_SUCCESS;
}

Encode_Status VaapiEncoderHEVC::getCodecConfig(VideoEncOutputBuffer * outBuffer)
{
    ASSERT(outBuffer && ((outBuffer->flag == OUTPUT_CODEC_DATA) || outBuffer->flag == OUTPUT_EVERYTHING));
    AutoLock locker(m_paramLock);
    if (!m_headers)
        return ENCODE_NO_REQUEST_DATA;
    return m_headers->getCodecConfig(outBuffer);
}

/* Handle new GOP starts */
void VaapiEncoderHEVC::resetGopStart ()
{
    m_idrNum = 0;
    m_frameIndex = 0;
}

/* Marks the supplied picture as a B-frame */
void VaapiEncoderHEVC::setBFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_B;
    pic->m_frameNum = (m_frameIndex % m_maxFrameNum);
}

/* Marks the supplied picture as a P-frame */
void VaapiEncoderHEVC::setPFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_P;
    pic->m_frameNum = (m_frameIndex % m_maxFrameNum);
}

/* Marks the supplied picture as an I-frame */
void VaapiEncoderHEVC::setIFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_I;
    pic->m_frameNum = (m_frameIndex % m_maxFrameNum);
}

/* Marks the supplied picture as an IDR frame */
void VaapiEncoderHEVC::setIdrFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_I;
    pic->m_frameNum = 0;
    pic->m_poc = 0;
}

/* Marks the supplied picture a a key-frame */
void VaapiEncoderHEVC::setIntraFrame (const PicturePtr& picture,bool idIdr)
{
    if (idIdr) {
        resetGopStart();
        setIdrFrame(picture);
    } else
        setIFrame(picture);
}

bool VaapiEncoderHEVC::
referenceListUpdate (const PicturePtr& picture, const SurfacePtr& surface)
{
    if (VAAPI_PICTURE_TYPE_B == picture->m_type) {
        return true;
    }

    if (picture->isIdr()) {
        m_refList.clear();
    } else if (m_refList.size() >= m_maxRefFrames) {
        m_refList.pop_back();
    }
    ReferencePtr ref(new VaapiEncoderHEVCRef(picture, surface));
    m_refList.push_front(ref); // recent first
    assert (m_refList.size() <= m_maxRefFrames);
    return true;
}

bool  VaapiEncoderHEVC::pictureReferenceListSet (
    const PicturePtr& picture)
{
    uint32_t i;

    /* reset reflist0 and reflist1 every time  */
    m_refList0.clear();
    m_refList1.clear();

    for (i = 0; i < m_refList.size(); i++) {
        assert(picture->m_poc != m_refList[i]->m_poc);
        if (picture->m_poc > m_refList[i]->m_poc) {
            /* set forward reflist */
            m_refList0.push_front(m_refList[i]);
            if (m_refList0.size() > m_maxRefList0Count)
                m_refList0.pop_back();
        } else {
            /* set backward reflist */
            m_refList1.push_front(m_refList[i]);
            if (m_refList1.size() > m_maxRefList1Count)
                m_refList1.pop_back();
        }
    }

    ShortRfsUpdate(picture);

    assert (m_refList0.size() + m_refList1.size() <= m_maxRefFrames);

    return true;
}

void VaapiEncoderHEVC::referenceListFree()
{
    m_refList.clear();
    m_refList0.clear();
    m_refList1.clear();
}

void VaapiEncoderHEVC::ShortRfsUpdate(const PicturePtr& picture)
{
    int i;
 
    memset(&m_shortRFS, 0, sizeof(m_shortRFS));

    m_shortRFS.num_short_term_ref_pic_sets = 0;
    m_shortRFS.inter_ref_pic_set_prediction_flag = 0;

    if (intraPeriod() > 1 && m_refList0.size()) {
        m_shortRFS.num_negative_pics         = 1;
        m_shortRFS.delta_poc_s0_minus1[0]  = picture->m_poc - m_refList0[0]->m_poc - 1;
        m_shortRFS.used_by_curr_pic_s0_flag[0] = 1;
        if (m_numBFrames && m_refList1.size()) {
            m_shortRFS.num_positive_pics      = 1;
            m_shortRFS.delta_poc_s1_minus1[0]  = m_refList1[0]->m_poc - picture->m_poc - 1;
            m_shortRFS.used_by_curr_pic_s1_flag[0]            = 1;
            DEBUG("m_refList1_size is %d\n", m_refList1.size());
        }
    }

    for (i = 1; i < m_shortRFS.num_negative_pics; i++)
    {
        m_shortRFS.delta_poc_s0_minus1[i]                 = 0;
        m_shortRFS.used_by_curr_pic_s0_flag[i]            = 1;
    }

    for (i = 1; i < m_shortRFS.num_positive_pics; i++) {
        m_shortRFS.delta_poc_s1_minus1[i]                 = 0;
        m_shortRFS.used_by_curr_pic_s1_flag[i]            = 1;
    }
 
}


void VaapiEncoderHEVC::setShortRfs()
{
    int i;
 
    memset(&m_shortRFS, 0, sizeof(m_shortRFS));

    if (intraPeriod() > 1) {
        m_shortRFS.num_negative_pics         = 1;
        if (m_numBFrames )
            m_shortRFS.num_positive_pics      = 1;
    }

    m_shortRFS.num_short_term_ref_pic_sets = 0;

    m_shortRFS.inter_ref_pic_set_prediction_flag = 0;

    m_shortRFS.delta_poc_s0_minus1[0]                 = 0;
    m_shortRFS.used_by_curr_pic_s0_flag[0]            = 1;

    for (i = 1; i < m_shortRFS.num_negative_pics; i++)
    {
        m_shortRFS.delta_poc_s0_minus1[i]                 = 0;
        m_shortRFS.used_by_curr_pic_s0_flag[i]            = 1;
    }

    for (i = 1; i < m_shortRFS.num_positive_pics; i++) {
        m_shortRFS.delta_poc_s1_minus1[i]                 = 0;
        m_shortRFS.used_by_curr_pic_s1_flag[i]            = 1;
    }
 
}

bool VaapiEncoderHEVC::fill(VAEncSequenceParameterBufferHEVC* seqParam) const
{

    seqParam->general_profile_idc = m_profileIdc;
    seqParam->general_level_idc = level();
    seqParam->general_tier_flag = 0;
    seqParam->intra_period = intraPeriod();
    seqParam->intra_idr_period = seqParam->intra_period;
    seqParam->ip_period = 1 + m_numBFrames;
    seqParam->bits_per_second = bitRate();

    seqParam->pic_width_in_luma_samples = m_AlignedWidth;
    seqParam->pic_height_in_luma_samples = m_AlignedHeight;

    /*Only support yuv 4:2:0 format */
    seqParam->seq_fields.bits.chroma_format_idc = 1;
    /* separate color plane_flag*/
    seqParam->seq_fields.bits.separate_colour_plane_flag = 0;
    /* bit_depth_luma_minus8. Only 0 is supported for main profile */
    seqParam->seq_fields.bits.bit_depth_luma_minus8 = 0;
    /* bit_depth_chroma_minus8. Only 0 is supported for main profile*/
    seqParam->seq_fields.bits.bit_depth_chroma_minus8 = 0;
    /* scaling_list_enabled_flag. Use the default value  */
    seqParam->seq_fields.bits.scaling_list_enabled_flag = 0;
    /* strong_intra_smoothing_enabled_flag. Not use the bi-linear interpolation */
    seqParam->seq_fields.bits.strong_intra_smoothing_enabled_flag = 0;
    /* amp_enabled_flag(nLx2N or nRx2N). This is not supported */
    seqParam->seq_fields.bits.amp_enabled_flag = 1;
    /* sample_adaptive_offset_enabled_flag. Unsupported */
    seqParam->seq_fields.bits.sample_adaptive_offset_enabled_flag = 0;
    /* pcm_enabled_flag */
    seqParam->seq_fields.bits.pcm_enabled_flag = 0;
    /* pcm_loop_filter_disabled_flag */
    seqParam->seq_fields.bits.pcm_loop_filter_disabled_flag = 1;
    /* sps_temporal_mvp_enabled_flag. Enabled */
    seqParam->seq_fields.bits.sps_temporal_mvp_enabled_flag = 0;

    seqParam->log2_min_luma_coding_block_size_minus3 = log2(m_ctbSize)  -3;
    seqParam->log2_diff_max_min_luma_coding_block_size = log2(m_cuSize) - log2(m_ctbSize);
    seqParam->log2_min_transform_block_size_minus2 = log2(m_minTbSize) - 2;
    seqParam->log2_diff_max_min_transform_block_size = log2(m_maxTbSize) - log2(m_minTbSize);

    /* max_transform_hierarchy_depth_inter */
    seqParam->max_transform_hierarchy_depth_inter = 2;

    /* max_transform_hierarchy_depth_intra */
    seqParam->max_transform_hierarchy_depth_intra = 2;

    /* The PCM fields can be ignored as PCM is disabled */
    seqParam->pcm_sample_bit_depth_luma_minus1 = 7;
    seqParam->pcm_sample_bit_depth_chroma_minus1 = 7;
    seqParam->log2_min_pcm_luma_coding_block_size_minus3 = 0;
    seqParam->log2_max_pcm_luma_coding_block_size_minus3 = 0;

    return true;
}

/* Fills in VA picture parameter buffer */
bool VaapiEncoderHEVC::fill(VAEncPictureParameterBufferHEVC* picParam, const PicturePtr& picture,
                            const SurfacePtr& surface) const
{
    uint32_t i = 0;

    picParam->decoded_curr_pic.picture_id = surface->getID();
    picParam->decoded_curr_pic.flags = VA_PICTURE_HEVC_RPS_LT_CURR;
    picParam->decoded_curr_pic.pic_order_cnt = picture->m_poc;

    if (picture->m_type != VAAPI_PICTURE_TYPE_I) {
        for (i = 0; i < m_refList.size(); i++) {
            picParam->reference_frames[i].picture_id = m_refList[i]->m_pic->getID();
            picParam->reference_frames[i].pic_order_cnt= m_refList[i]->m_poc;
        }
    }

    for (; i < N_ELEMENTS(picParam->reference_frames); ++i) {
        picParam->reference_frames[i].picture_id = VA_INVALID_ID;
    }

    picParam->coded_buf = picture->m_codedBuffer->getID();

    /*collocated_ref_pic_index should be 0xff  when element slice_temporal_mvp_enable_flag is 0 */
    picParam->collocated_ref_pic_index = 0xff;

    picParam->last_picture = 0;  /* means last encoding picture */

    picParam->pic_init_qp = initQP();

    picParam->diff_cu_qp_delta_depth = 0;

    picParam->pps_cb_qp_offset = 0;
    picParam->pps_cr_qp_offset = 0;

    /* currently multi-tile is disabled */
    picParam->num_tile_columns_minus1 = 0;
    picParam->num_tile_rows_minus1 = 0;

    memset(picParam->column_width_minus1, 0, sizeof(picParam->column_width_minus1));
    memset(picParam->row_height_minus1, 0, sizeof(picParam->row_height_minus1));

    picParam->log2_parallel_merge_level_minus2 = 0;

    /*no bit size limitation*/
    picParam->ctu_max_bitsize_allowed = 0;

    picParam->num_ref_idx_l0_default_active_minus1 = 0;
    picParam->num_ref_idx_l1_default_active_minus1 = 0;

    picParam->slice_pic_parameter_set_id = 0;
    picParam->nal_unit_type = PPS_NUT;

    picParam->pic_fields.value = 0;
    picParam->pic_fields.bits.idr_pic_flag = picture->isIdr();
    /*FIXME: can't support picture type B1 and B2 now */
    picParam->pic_fields.bits.coding_type = picture->m_type;
    picParam->pic_fields.bits.reference_pic_flag = (picture->m_type != VAAPI_PICTURE_TYPE_B);
    picParam->pic_fields.bits.dependent_slice_segments_enabled_flag = 0;
    picParam->pic_fields.bits.sign_data_hiding_enabled_flag = 0;
    picParam->pic_fields.bits.constrained_intra_pred_flag = 0;
    picParam->pic_fields.bits.transform_skip_enabled_flag = 0;
    picParam->pic_fields.bits.cu_qp_delta_enabled_flag = 0;
    picParam->pic_fields.bits.weighted_pred_flag = 0;
    picParam->pic_fields.bits.weighted_bipred_flag = 0;
    picParam->pic_fields.bits.transquant_bypass_enabled_flag = 0;
    picParam->pic_fields.bits.tiles_enabled_flag = 0;
    picParam->pic_fields.bits.entropy_coding_sync_enabled_flag = 0;
    picParam->pic_fields.bits.loop_filter_across_tiles_enabled_flag = 0;
    picParam->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag = 0;
    /* scaling_list_data_present_flag: use default scaling list data*/
    picParam->pic_fields.bits.scaling_list_data_present_flag = 0;
    picParam->pic_fields.bits.screen_content_flag = 1;
    picParam->pic_fields.bits.no_output_of_prior_pics_flag = 0;

    return TRUE;
}

bool VaapiEncoderHEVC::ensureSequenceHeader(const PicturePtr& picture,const VAEncSequenceParameterBufferHEVC* const sequence)
{
    m_headers.reset(new VaapiEncStreamHeaderHEVC(this));
    m_headers->setVPS(sequence);
    m_headers->setSPS(sequence);
    return true;
}

bool VaapiEncoderHEVC::ensurePictureHeader(const PicturePtr& picture, const VAEncPictureParameterBufferHEVC* const picParam)
{
    m_headers->addPPS(picParam);
    m_headers->generateCodecConfig();
    picture->m_headers = m_headers;
    return true;
}

bool VaapiEncoderHEVC:: fillReferenceList(VAEncSliceParameterBufferHEVC* slice) const
{
    uint32_t i = 0;
    for (i = 0; i < m_refList0.size(); i++) {
        assert(m_refList0[i] && m_refList0[i]->m_pic && (m_refList0[i]->m_pic->getID() != VA_INVALID_ID));
        slice->ref_pic_list0[i].picture_id = m_refList0[i]->m_pic->getID();
        slice->ref_pic_list0[i].pic_order_cnt= m_refList0[i]->m_poc;
    }
    for (; i < N_ELEMENTS(slice->ref_pic_list0); i++)
        slice->ref_pic_list0[i].picture_id = VA_INVALID_SURFACE;

    for (i = 0; i < m_refList1.size(); i++){
        assert(m_refList1[i] && m_refList1[i]->m_pic && (m_refList1[i]->m_pic->getID() != VA_INVALID_ID));
        slice->ref_pic_list1[i].picture_id = m_refList1[i]->m_pic->getID();
        slice->ref_pic_list1[i].pic_order_cnt= m_refList1[i]->m_poc;
    }
    for (; i < N_ELEMENTS(slice->ref_pic_list1); i++)
        slice->ref_pic_list1[i].picture_id = VA_INVALID_SURFACE;
    return true;
}

bool VaapiEncoderHEVC::addPackedSliceHeader(const PicturePtr& picture,
                                        const VAEncSliceParameterBufferHEVC* const sliceParam,
                                        uint32_t sliceIndex) const
{
    BitWriter bs;
    BOOL short_term_ref_pic_set_sps_flag = !!m_shortRFS.num_short_term_ref_pic_sets;
    HevcNalUnitType nalUnitType = (picture->isIdr() ? IDR_W_RADL : TRAIL_R );
    bit_writer_init (&bs, 128 * 8);
    bit_writer_put_bits_uint32(&bs, HEVC_NAL_START_CODE, 32);
    bit_writer_write_nal_header(&bs, nalUnitType);

    /* first_slice_segment_in_pic_flag */
    bit_writer_put_bits_uint32(&bs, sliceIndex == 0, 1);

    /* no_output_of_prior_pics_flag */
    if (nalUnitType >=  BLA_W_LP && nalUnitType <= RSV_IRAP_VCL23 )
        bit_writer_put_bits_uint32(&bs, 1, 1);
        
    /* slice_pic_parameter_set_id */
    bit_writer_put_ue(&bs, 0);

    if (sliceIndex) {
        /* don't support dependent_slice_segments_enabled_flag right now*/
        ASSERT (!m_picParam->pic_fields.bits.dependent_slice_segments_enabled_flag &&
                      !sliceParam->slice_fields.bits.dependent_slice_segment_flag);

        bit_writer_put_bits_uint32(&bs, sliceParam->slice_segment_address, log2(sliceParam->num_ctu_in_slice));
    }

    if (!sliceParam->slice_fields.bits.dependent_slice_segment_flag) {
        bit_writer_put_ue(&bs, sliceParam->slice_type);

        ASSERT(!m_seqParam->seq_fields.bits.separate_colour_plane_flag);

        if (nalUnitType != IDR_W_RADL && nalUnitType != IDR_N_LP) {
            bit_writer_put_bits_uint32(&bs, m_picParam->decoded_curr_pic.pic_order_cnt , m_log2MaxPicOrderCnt);
            bit_writer_put_bits_uint32(&bs, short_term_ref_pic_set_sps_flag, 1);
            if (!short_term_ref_pic_set_sps_flag)
                st_ref_pic_set(&bs, m_shortRFS.num_short_term_ref_pic_sets, m_shortRFS);
            else if (m_shortRFS.num_short_term_ref_pic_sets > 1)
                bit_writer_put_bits_uint32(&bs, m_shortRFS.short_term_ref_pic_set_idx, log2(m_shortRFS.num_short_term_ref_pic_sets));
            /* long_term_ref_pics_present_flag is set to 0 */

            if (sliceParam->slice_type != HEVC_SLICE_TYPE_I) {
                bit_writer_put_bits_uint32(&bs, sliceParam->slice_fields.bits.num_ref_idx_active_override_flag, 1);
                if (sliceParam->slice_fields.bits.num_ref_idx_active_override_flag) {
                    bit_writer_put_ue(&bs, sliceParam->num_ref_idx_l0_active_minus1);
                    if (sliceParam->slice_type == HEVC_SLICE_TYPE_B )
                        bit_writer_put_ue(&bs, sliceParam->num_ref_idx_l1_active_minus1);
                }
                /* pps lists_modification_present_flag is set to 0 */
                if (sliceParam->slice_type == HEVC_SLICE_TYPE_B)
                    bit_writer_put_bits_uint32(&bs, sliceParam->slice_fields.bits.mvd_l1_zero_flag, 1);
                if (sliceParam->slice_fields.bits.cabac_init_flag)
                    bit_writer_put_bits_uint32(&bs, sliceParam->slice_fields.bits.cabac_init_flag, 1);

                /*  slice_temporal_mvp_enabled_flag and weighted_pred_flag are set to 0*/
                ASSERT(!sliceParam->slice_fields.bits.slice_temporal_mvp_enabled_flag &&
                             !m_picParam->pic_fields.bits.weighted_bipred_flag);
                
                ASSERT(sliceParam->max_num_merge_cand <= 5);
                bit_writer_put_ue(&bs, 5 - sliceParam->max_num_merge_cand);
            }
        }
        
        bit_writer_put_ue(&bs, sliceParam->slice_qp_delta);
        /* pps_slice_chroma_qp_offsets_present_flag is set to 1 */
        bit_writer_put_ue(&bs, sliceParam->slice_cb_qp_offset);
        bit_writer_put_ue(&bs, sliceParam->slice_cr_qp_offset);
        /* deblocking_filter_override_enabled_flag and 
          * pps_loop_filter_across_slices_enabled_flag are set to 0 */
    }
    
    bit_writer_write_trailing_bits(&bs);
     
     if(!picture->addPackedHeader(VAEncPackedHeaderSlice, bs.data, bs.bit_size))
         return false;
     
     return true;
}

/* Add slice headers to picture */
bool VaapiEncoderHEVC::addSliceHeaders (const PicturePtr& picture) const
{
    VAEncSliceParameterBufferHEVC *sliceParam;
    uint32_t sliceOfCtus, sliceModCtus, curSliceCtus;
    uint32_t numCtus;
    uint32_t lastCtuIndex;

    assert (picture);

    if (picture->m_type != VAAPI_PICTURE_TYPE_I) {
        /* have one reference frame at least */
        assert(m_refList0.size() > 0);
    }

    numCtus= m_cuWidth * m_cuHeight;

    assert (m_numSlices && m_numSlices < numCtus);
    sliceOfCtus = numCtus / m_numSlices;
    sliceModCtus = numCtus % m_numSlices;
    lastCtuIndex = 0;
    for (int i = 0; i < m_numSlices; ++i) {
        curSliceCtus = sliceOfCtus;
        if (sliceModCtus) {
            ++curSliceCtus;
            --sliceModCtus;
        }
        if (!picture->newSlice(sliceParam))
            return false;

        sliceParam->slice_segment_address = lastCtuIndex;
        sliceParam->num_ctu_in_slice = curSliceCtus;
        sliceParam->slice_type = hevc_get_slice_type (picture->m_type);
        assert (sliceParam->slice_type != -1);
        sliceParam->slice_pic_parameter_set_id = 0;
        sliceParam->slice_fields.bits.num_ref_idx_active_override_flag = 1;
        if (picture->m_type != VAAPI_PICTURE_TYPE_I && m_refList0.size() > 0)
            sliceParam->num_ref_idx_l0_active_minus1 = m_refList0.size() - 1;
        if (picture->m_type == VAAPI_PICTURE_TYPE_B && m_refList1.size() > 0)
            sliceParam->num_ref_idx_l1_active_minus1 = m_refList1.size() - 1;

        fillReferenceList(sliceParam);

        /* luma_log2_weight_denom should be the range: [0, 7] */
        sliceParam->luma_log2_weight_denom = 0;
        /* max_num_merge_cand should be the range [1, 5 + NumExtraMergeCand] */
        sliceParam->max_num_merge_cand = 5;

        /* let slice_qp equal to init_qp*/
        sliceParam->slice_qp_delta = 0;

        /* slice_beta_offset_div2 and slice_tc_offset_div2  should be the range [-6, 6] */
        sliceParam->slice_beta_offset_div2 = 0;
        sliceParam->slice_tc_offset_div2 = 0;

        /* set calculation for next slice */
        lastCtuIndex += curSliceCtus;

        sliceParam->slice_fields.bits.slice_deblocking_filter_disabled_flag = 1;

        sliceParam->slice_fields.bits.last_slice_of_pic_flag = (lastCtuIndex == numCtus);

        addPackedSliceHeader(picture, sliceParam, i);
    }
    assert (lastCtuIndex == numCtus);
    
    return true;
}

bool VaapiEncoderHEVC::ensureSequence(const PicturePtr& picture)
{
    if (picture->m_type != VAAPI_PICTURE_TYPE_I) {
        return true;
    }

    if (!picture->editSequence(m_seqParam) || !fill(m_seqParam)) {
        ERROR("failed to create sequence parameter buffer (SPS)");
        return false;
    }

    if (!ensureSequenceHeader(picture, m_seqParam)) {
        ERROR ("failed to create packed sequence header buffer");
        return false;
    }

    return true;
}

bool VaapiEncoderHEVC::ensurePicture (const PicturePtr& picture, const SurfacePtr& surface)
{

    if (picture->m_type != VAAPI_PICTURE_TYPE_I &&
            !pictureReferenceListSet(picture)) {
        ERROR ("reference list reorder failed");
        return false;
    }

    if (!picture->editPicture(m_picParam) || !fill(m_picParam, picture, surface)) {
        ERROR("failed to create picture parameter buffer (PPS)");
        return false;
    }

    if (picture->isIdr() && !ensurePictureHeader (picture, m_picParam)) {
            ERROR ("set picture packed header failed");
            return false;
    }

    return true;
}

bool VaapiEncoderHEVC::ensureSlices(const PicturePtr& picture)
{
    assert (picture);

    if (!addSliceHeaders (picture))
        return false;
    return true;
}

Encode_Status VaapiEncoderHEVC::encodePicture(const PicturePtr& picture)
{
    Encode_Status ret = ENCODE_FAIL;

    SurfacePtr reconstruct = createSurface();
    if (!reconstruct)
        return ret;
    {
        AutoLock locker(m_paramLock);

        if (!ensureSequence (picture))
            return ret;
        if (!ensureMiscParams (picture.get()))
            return ret;
        if (!ensurePicture(picture, reconstruct))
            return ret;
        if (!ensureSlices (picture))
            return ret;
    }
    if (!picture->encode())
        return ret;

    if (!referenceListUpdate (picture, reconstruct))
        return ret;

    return ENCODE_SUCCESS;
}

const bool VaapiEncoderHEVC::s_registered =
    VaapiEncoderFactory::register_<VaapiEncoderHEVC>(YAMI_MIME_HEVC)
    && VaapiEncoderFactory::register_<VaapiEncoderHEVC>(YAMI_MIME_H265);

}
