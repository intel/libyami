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

/* Define the maximum IDR period */
#define MAX_IDR_PERIOD 512

#define HEVC_NAL_START_CODE 0x000001

#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_NONE        0
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_LOW         1
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_MEDIUM      2
#define VAAPI_ENCODER_HEVC_NAL_REF_IDC_HIGH        3

#define  VPS_NUT    32
#define  SPS_NUT    33
#define  PPS_NUT    34

/* Get slice_type value for H.264 specification */
static uint8_t
hevc_get_slice_type (VaapiPictureType type)
{
    switch (type) {
    case VAAPI_PICTURE_TYPE_I:
        return 2;
    case VAAPI_PICTURE_TYPE_P:
        return 0;
    case VAAPI_PICTURE_TYPE_B:
        return 1;
    default:
        return -1;
    }
    return -1;
}

static uint32_t log2 (uint32_t num)
{
    uint32_t ret = 0;
    assert(num);

    while ((num >>= 1))
        ++ret;

    return ret;
}

/* Get log2_max_frame_num value for H.264 specification */
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
        /* Max layer is zero. and sub layer info is not present
        * if it can support multi layers, the sub layer info should be present.
        * TBD
        */
    }
}


class VaapiEncStreamHeaderHEVC
{
    typedef std::vector<uint8_t> Header;
public:

    VaapiEncStreamHeaderHEVC();
    VaapiEncStreamHeaderHEVC(VaapiEncoderHEVC* encoder) {m_encoder = encoder;};

    void setVPS(const VAEncSequenceParameterBufferHEVC* const sequence, VaapiProfile profile)
    {
        ASSERT(m_vps.empty());
        BitWriter bs;
        bit_writer_init (&bs, 128 * 8);
        bit_writer_write_vps (&bs, sequence, profile);
        bsToHeader(m_vps, bs);
        bit_writer_clear (&bs, TRUE);
    }

    void setSPS(const VAEncSequenceParameterBufferHEVC* const sequence, VaapiProfile profile)
    {
        ASSERT(m_vps.size() && m_sps.empty());
        BitWriter bs;
        bit_writer_init (&bs, 128 * 8);
        bit_writer_write_sps (&bs, sequence, profile);
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
        ASSERT((outBuffer && outBuffer->format == OUTPUT_CODEC_DATA) || outBuffer->format == OUTPUT_EVERYTHING);
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
        const VAEncSequenceParameterBufferHEVC* const seq,
        VaapiProfile profile
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
        bit_writer_put_ue(bitwriter, 6);

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

    void st_ref_pic_set(BitWriter *bs, int stRpsIdx, int refIdx, const ShortRFS shortRFS)
    {
        int i;
        if (stRpsIdx)
            bit_writer_put_bits_uint32(bs, shortRFS.inter_ref_pic_set_prediction_flag, 1);

        if (shortRFS.inter_ref_pic_set_prediction_flag)
        {
#if 0
            if (stRpsIdx == ps->num_short_term_ref_pic_sets)
                bitstream_put_ue(bs, ps->delta_idx_minus1);

            bitstream_put_ui(bs, ps->delta_rps_sign, 1);
            bitstream_put_ue(bs, ps->abs_delta_rps_minus1);

            for (j = 0; j <= ps->NumDeltaPocs[RefRpsIdx]; j++)
            {
                bitstream_put_ui(bs, ps->used_by_curr_pic_flag[j], 1);

                if(!ps->used_by_curr_pic_flag[j])
                    bitstream_put_ui(bs, ps->use_delta_flag[j], 1);
            }
#endif
        } else {
            bit_writer_put_ue(bs, shortRFS.num_negative_pics);
            bit_writer_put_ue(bs, shortRFS.num_positive_pics[refIdx==0?0:1]);

            for (i = 0; i < shortRFS.num_negative_pics; i++)
            {
                bit_writer_put_ue(bs, shortRFS.delta_poc_s0_minus1[i]);
                bit_writer_put_bits_uint32(bs, shortRFS.used_by_curr_pic_s0_flag[i], 1);
            }
            for (i = 0; i < shortRFS.num_positive_pics[refIdx==0?0:1]; i++)
            {
                bit_writer_put_ue(bs, shortRFS.delta_poc_s1_minus1[i]);
                bit_writer_put_bits_uint32(bs, shortRFS.used_by_curr_pic_s1_flag[i], 1);
            }
        }

        return;
    }

    BOOL bit_writer_write_sps(
        BitWriter *bitwriter,
        const VAEncSequenceParameterBufferHEVC* const seq,
        VaapiProfile profile
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
       bit_writer_put_ue(bitwriter, 6);
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
            st_ref_pic_set(bitwriter, i, i, m_encoder->m_shortRFS);

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
        bit_writer_put_bits_uint32(bitwriter, 1, 1);

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
            bit_writer_put_bits_uint32(bitwriter, 1, 1);
            /* pps_deblocking_filter_disabled_flag */
            bit_writer_put_bits_uint32(bitwriter, 1, 1);
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
    m_ctbSize(9),
    m_cuSize(16),
    m_minTbSize(4),
    m_maxTbSize(16),
    m_streamFormat(AVC_STREAM_FORMAT_ANNEXB),
    m_reorderState(VAAPI_ENC_REORD_WAIT_FRAMES)
{
    m_videoParamCommon.profile = VAProfileHEVCMain;
    m_videoParamCommon.level = 51;
    m_videoParamCommon.rcParams.initQP = 26;
    m_videoParamCommon.rcParams.minQP = 1;

    m_videoParamAVC.idrInterval = 1;
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

    m_maxCodedbufSize = m_cuAlignedWidth * m_cuAlignedHeight * 3 / 2;

    DEBUG("m_maxCodedbufSize: %u", m_maxCodedbufSize);

    return true;
}

void VaapiEncoderHEVC::resetParams ()
{

    m_levelIdc = level();
    m_profileIdc = hevc_get_profile_idc(profile());

    //FIXME:
    m_numBFrames = 0;
    m_numSlices = 1;

    assert (width() && height());

    m_cuAlignedWidth = (width() + m_cuSize -1) / m_cuSize * m_cuSize;
    m_cuAlignedHeight = (height() + m_cuSize -1) / m_cuSize * m_cuSize;

    m_ctbWidth = (width() + m_cuSize -1) / m_cuSize;
    m_ctbHeight = (height() + m_cuSize-1) / m_cuSize;

    m_confWinLeftOffset = m_confWinTopOffset = 0;

    if (m_cuAlignedWidth != width() || m_cuAlignedHeight !=height()) {
            m_confWinFlag = true;
            m_confWinRightOffset = (m_cuAlignedWidth - width()) / 2;
            m_confWinBottomOffset = (m_cuAlignedHeight -height()) / 2;
    } else {
        m_confWinFlag = false;
        m_confWinRightOffset = 0;
        m_confWinBottomOffset = 0;
    }


    if (keyFramePeriod() < intraPeriod())
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

    setShortRFS();

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
    m_refList.clear();

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
    case VideoConfigTypeAVCStreamFormat: {
            VideoConfigAVCStreamFormat* format = (VideoConfigAVCStreamFormat*)videoEncParams;
            if (format->size == sizeof(VideoConfigAVCStreamFormat)) {
                m_streamFormat = format->streamFormat;
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

    // TODO, update video resolution basing on hw requirement
    status = VaapiEncoderBase::getParameters(type, videoEncParams);

    return status;
}

Encode_Status VaapiEncoderHEVC::reorder(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    if (!surface)
        return ENCODE_INVALID_PARAMS;

    ++m_curPresentIndex;
    PicturePtr picture(new VaapiEncPictureHEVC(m_context, surface, timeStamp));
    picture->m_poc = ((m_curPresentIndex * 2) % m_maxPicOrderCnt);

    bool isIdr = (m_frameIndex == 0 ||m_frameIndex >= keyFramePeriod() || forceKeyFrame);

    /* check key frames */
    if (isIdr || (m_frameIndex % intraPeriod() == 0)) {
        ++m_curFrameNum;
        ++m_frameIndex;
        /* b frame enabled,  check queue of reorder_frame_list */
        if (m_numBFrames
                && (m_reorderFrameList.size() > 0)) {
            assert(0);

        }
        setIntraFrame (picture, isIdr);
        m_reorderFrameList.push_back(picture);
        m_reorderState = VAAPI_ENC_REORD_DUMP_FRAMES;
        return ENCODE_SUCCESS;
    }
    /* new p/b frames coming */
    ++m_frameIndex;
    if (m_reorderFrameList.size() < m_numBFrames) {
        assert(0);
        m_reorderFrameList.push_back(picture);
        return ENCODE_SUCCESS;
    }
    ++m_curFrameNum;
    setPFrame (picture);
    m_reorderFrameList.push_front(picture);
    m_reorderState = VAAPI_ENC_REORD_DUMP_FRAMES;
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
    if (m_reorderState == VAAPI_ENC_REORD_DUMP_FRAMES) {
        if (!m_maxCodedbufSize)
            ensureCodedBufferSize();
        ASSERT(m_maxCodedbufSize);
        CodedBufferPtr codedBuffer = VaapiCodedBuffer::create(m_context, m_maxCodedbufSize);
        if (!codedBuffer)
            return ENCODE_NO_MEMORY;
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
    m_curFrameNum = 0;
    m_curPresentIndex = 0;
}

/* Marks the supplied picture as a B-frame */
void VaapiEncoderHEVC::setBFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_B;
    pic->m_frameNum = (m_curFrameNum % m_maxFrameNum);
}

/* Marks the supplied picture as a P-frame */
void VaapiEncoderHEVC::setPFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_P;
    pic->m_frameNum = (m_curFrameNum % m_maxFrameNum);
}

/* Marks the supplied picture as an I-frame */
void VaapiEncoderHEVC::setIFrame (const PicturePtr& pic)
{
    pic->m_type = VAAPI_PICTURE_TYPE_I;
    pic->m_frameNum = (m_curFrameNum % m_maxFrameNum);
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
        //+1 for next frame
        m_frameIndex++;
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
        referenceListFree();
    } else if (m_refList.size() >= m_maxRefFrames) {
        m_refList.pop_front();
    }
    ReferencePtr ref(new VaapiEncoderHEVCRef(picture, surface));
    m_refList.push_front(ref); // recent first
    assert (m_refList.size() <= m_maxRefFrames);
    return true;
}

bool  VaapiEncoderHEVC::referenceListInit (
    const PicturePtr& picture,
    vector<ReferencePtr>& refList0,
    vector<ReferencePtr>& refList1) const
{
    assert(picture->m_type == VAAPI_PICTURE_TYPE_P);
    refList0.reserve(m_refList.size());
    refList0.insert(refList0.end(), m_refList.begin(), m_refList.end());

    assert (refList0.size() + refList1.size() <= m_maxRefFrames);
    if (refList0.size() > m_maxRefList0Count)
        refList0.resize(m_maxRefList0Count);
    if (refList1.size() > m_maxRefList1Count)
        refList1.resize(m_maxRefList1Count);

    return true;
}

void VaapiEncoderHEVC::referenceListFree()
{
    m_refList.clear();
}

void VaapiEncoderHEVC::setShortRFS()
{
    int i;
    int intra_idr_period = intraPeriod();
    int ip_period = 1 + m_numBFrames;

    if (intra_idr_period > 1 && ip_period == 0)
    {
        m_shortRFS.num_negative_pics         = 1;
        m_shortRFS.num_positive_pics[0]      = 0;
    } else {
        m_shortRFS.num_negative_pics         = 1;
        m_shortRFS.num_positive_pics[0]      = 0;
        m_shortRFS.num_positive_pics[1]      = 1;
    }

#if 0
    m_shortRFS.num_short_term_ref_pic_sets = seq->ip_period + 1;
#else
m_shortRFS.num_short_term_ref_pic_sets = 0;
#endif

    m_shortRFS.inter_ref_pic_set_prediction_flag = 0;

    for (i = 0; i < m_shortRFS.num_short_term_ref_pic_sets; i++) //First position stores low-delay B
    {
        //Poc S0 is the reference distance b/t I pic and low-delay B or B pics.
        m_shortRFS.delta_poc_s0_minus1[i]                 = (i == 0) ? ip_period : (i-1);
        m_shortRFS.used_by_curr_pic_s0_flag[i]            = 1;
        //Poc S1 is the reference distance b/t P pic and low-delay B pics
        m_shortRFS.delta_poc_s1_minus1[i]                 = (i == 0) ? 0 : (ip_period-i);
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

    seqParam->pic_width_in_luma_samples = m_cuAlignedWidth;
    seqParam->pic_height_in_luma_samples = m_cuAlignedHeight;

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

    if (picture->m_type != VAAPI_PICTURE_TYPE_I) {
        list<ReferencePtr>::const_iterator it;
        for (it = m_refList.begin(); it != m_refList.end(); ++it) {
            assert(*it && (*it)->m_pic && ((*it)->m_pic->getID() != VA_INVALID_ID));
            picParam->reference_frames[i].picture_id = (*it)->m_pic->getID();
            ++i;
        }
    }
    for (; i < 16; ++i) {
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
    m_headers->setVPS(sequence, profile());
    m_headers->setSPS(sequence, profile());
    return true;
}

bool VaapiEncoderHEVC::ensurePictureHeader(const PicturePtr& picture, const VAEncPictureParameterBufferHEVC* const picParam)
{
    m_headers->addPPS(picParam);
    m_headers->generateCodecConfig();
    picture->m_headers = m_headers;
    return true;
}

static void fillReferenceList(VAEncSliceParameterBufferHEVC* slice, const vector<ReferencePtr>& refList, uint32_t index)
{
    VAPictureHEVC* picList;
    int total;
    if (!index) {
        picList = slice->ref_pic_list0;
        total = N_ELEMENTS(slice->ref_pic_list0);
    }
    else {
        picList = slice->ref_pic_list0;
        total = N_ELEMENTS(slice->ref_pic_list0);
    }
    int i = 0;
    for (; i < refList.size(); i++)
        picList[i].picture_id = refList[i]->m_pic->getID();
    for (; i <total; i++)
        picList[i].picture_id = VA_INVALID_SURFACE;
}

/* Adds slice headers to picture */
bool VaapiEncoderHEVC::addSliceHeaders (const PicturePtr& picture,
                                        const vector<ReferencePtr>& refList0,
                                        const vector<ReferencePtr>& refList1) const
{
    VAEncSliceParameterBufferHEVC *sliceParam;
    uint32_t sliceOfCtus, sliceModCtus, curSliceCtus;
    uint32_t numCtus;
    uint32_t lastCtuIndex;

    assert (picture);
    /*one reference frame supported */
    if (picture->m_type == VAAPI_PICTURE_TYPE_I) {
        assert(!refList0.size() && !refList1.size());
    }
    else {
        assert(refList0.size() == 1);
        if (picture->m_type == VAAPI_PICTURE_TYPE_B)
            assert(refList1.size() == 1);
    }

    numCtus= m_ctbWidth* m_ctbHeight;

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

        sliceParam->slice_segment_address= lastCtuIndex;
        sliceParam->num_ctu_in_slice = curSliceCtus;
        sliceParam->slice_type = hevc_get_slice_type (picture->m_type);
        assert (sliceParam->slice_type != -1);
        sliceParam->slice_pic_parameter_set_id = 0;
        if (picture->m_type != VAAPI_PICTURE_TYPE_I && refList0.size() > 0)
            sliceParam->num_ref_idx_l0_active_minus1 = refList0.size() - 1;
        if (picture->m_type == VAAPI_PICTURE_TYPE_B && refList1.size() > 0)
            sliceParam->num_ref_idx_l1_active_minus1 = refList1.size() - 1;

        fillReferenceList(sliceParam, refList0, 0);
        fillReferenceList(sliceParam, refList1, 1);

        /* luma_log2_weight_denom should be the range: [0, 7] */
        sliceParam->luma_log2_weight_denom = 0;
        /* max_num_merge_cand should be the range [1, 5 + NumExtraMergeCand] */
        sliceParam->max_num_merge_cand = 5;

        /* let slice_qp equal to init_qp*/
        sliceParam->slice_qp_delta = 0;

        /* slice_beta_offset_div2 and slice_tc_offset_div2  should be the range [-6, 6] */
        sliceParam->slice_beta_offset_div2 = 2;
        sliceParam->slice_tc_offset_div2 = 2;

        /* set calculation for next slice */
        lastCtuIndex += curSliceCtus;

        sliceParam->slice_fields.bits.slice_deblocking_filter_disabled_flag = 1;

        sliceParam->slice_fields.bits.last_slice_of_pic_flag = (lastCtuIndex == numCtus);
    }
    assert (lastCtuIndex == numCtus);
    return true;
}

bool VaapiEncoderHEVC::ensureSequence(const PicturePtr& picture)
{
    if (picture->m_type != VAAPI_PICTURE_TYPE_I) {
        return true;
    }

    VAEncSequenceParameterBufferHEVC* seqParam;

    if (!picture->editSequence(seqParam) || !fill(seqParam)) {
        ERROR("failed to create sequence parameter buffer (SPS)");
        return false;
    }

    if (!ensureSequenceHeader(picture, seqParam)) {
        ERROR ("failed to create packed sequence header buffer");
        return false;
    }
    return true;
}

bool VaapiEncoderHEVC::ensurePicture (const PicturePtr& picture, const SurfacePtr& surface)
{
    VAEncPictureParameterBufferHEVC *picParam;

    if (!picture->editPicture(picParam) || !fill(picParam, picture, surface)) {
        ERROR("failed to create picture parameter buffer (PPS)");
        return false;
    }

    if (picture->isIdr() && !ensurePictureHeader (picture, picParam)) {
            ERROR ("set picture packed header failed");
            return false;
    }
    return true;
}

bool VaapiEncoderHEVC::ensureSlices(const PicturePtr& picture)
{
    assert (picture);

    vector<ReferencePtr> refList0;
    vector<ReferencePtr> refList1;

    if (picture->m_type != VAAPI_PICTURE_TYPE_I &&
            !referenceListInit(picture, refList0, refList1)) {
        ERROR ("reference list reorder failed");
        return false;
    }
    if (!addSliceHeaders (picture, refList0, refList1))
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
