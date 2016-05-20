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

#include "h265Parser.h"

#include <string.h>
#include <math.h>
#include <stddef.h> //offsetof

namespace YamiParser {
namespace H265 {

#define CHECK_RANGE(var, min, max)          \
    {                                       \
        if ((var) < (min) || (var) > (max)) \
            return false;                   \
    }

//when parse to the end of syntax elements, need to check validity.
#define HAVE_MORE_BITS(rd, num)                 \
    {                                           \
        if ((rd.getRemainingBitsCount()) < num) \
            return false;                       \
    }

#define SET_DEF_FOR_ORDERING_INFO(var)                                                     \
    if (!var->var##_sub_layer_ordering_info_present_flag &&                                \
        var->var##_max_sub_layers_minus1) {                                                \
        for (uint32_t i = 0; i <= var->var##_max_sub_layers_minus1 - 1U; i++) {            \
            var->var##_max_dec_pic_buffering_minus1[i] =                                   \
                var->var##_max_dec_pic_buffering_minus1[var->var##_max_sub_layers_minus1]; \
            var->var##_max_num_reorder_pics[i] =                                           \
                var->var##_max_num_reorder_pics[var->var##_max_sub_layers_minus1];         \
            var->var##_max_latency_increase_plus1[i] =                                     \
                var->var##_max_latency_increase_plus1[var->var##_max_sub_layers_minus1];   \
        }                                                                                  \
    }

#define PARSE_SUB_LAYER_ORDERING_INFO(var, nr)                                 \
    {                                                                          \
        uint32_t start =                                                       \
            var->var##_sub_layer_ordering_info_present_flag ? 0 :              \
            var->var##_max_sub_layers_minus1;                                  \
        for (uint32_t i = start; i <= var->var##_max_sub_layers_minus1; i++) { \
            var->var##_max_dec_pic_buffering_minus1[i] = nr.readUe();          \
            if (var->var##_max_dec_pic_buffering_minus1[i] > MAXDPBSIZE - 1)   \
                return false;                                                  \
                                                                               \
            var->var##_max_num_reorder_pics[i] = nr.readUe();                  \
            if (var->var##_max_num_reorder_pics[i] >                           \
                var->var##_max_dec_pic_buffering_minus1[i])                    \
                return false;                                                  \
                                                                               \
            var->var##_max_latency_increase_plus1[i] = nr.readUe();            \
        }                                                                      \
    }

// Table 7-5
static const uint8_t DefaultScalingList0[16] = {
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16,
    16, 16, 16, 16
};

// Table 7-6
static const uint8_t DefaultScalingList1[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 17, 16, 17, 16, 17, 18,
    17, 18, 18, 17, 18, 21, 19, 20,
    21, 20, 19, 21, 24, 22, 22, 24,
    24, 22, 22, 24, 25, 25, 27, 30,
    27, 25, 25, 29, 31, 35, 35, 31,
    29, 36, 41, 44, 41, 36, 47, 54,
    54, 47, 65, 70, 65, 88, 88, 115
};

static const uint8_t DefaultScalingList2[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 17, 17, 17, 17, 17, 18,
    18, 18, 18, 18, 18, 20, 20, 20,
    20, 20, 20, 20, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25,
    25, 25, 25, 28, 28, 28, 28, 28,
    28, 33, 33, 33, 33, 33, 41, 41,
    41, 41, 54, 54, 54, 71, 71, 91
};

VPS::VPS()
{
    memset(this, 0, sizeof(*this));
}

VPS::~VPS()
{
    free(hrd_layer_set_idx);
    free(cprms_present_flag);
}

SPS::SPS()
{
    memset(this, 0, offsetof(SPS, vps));
}

PPS::PPS()
{
    memset(this, 0, offsetof(PPS, sps));
}

SliceHeader::SliceHeader()
{
    memset(this, 0, offsetof(SliceHeader, pps));
}

SliceHeader::~SliceHeader()
{
    free(entry_point_offset_minus1);
}

uint32_t SliceHeader::getSliceDataByteOffset() const
{
    return NalUnit::NALU_HEAD_SIZE + (headerSize + 7) / 8 - emulationPreventionBytes;
}

bool SliceHeader::isBSlice() const
{
    return slice_type == 0;
}

bool SliceHeader::isPSlice() const
{
    return slice_type == 1;
}

bool SliceHeader::isISlice() const
{
    return slice_type == 2;
}

// Find the first occurrence of the subsequence position in the string src.
// @src, the pointer to source data.
// @size, the lenght of source data in bytes.
static const uint8_t* searchStartPos(const uint8_t* src, uint32_t size)
{
    const uint8_t seq[] = {0x00, 0x00, 0x01};
    const uint8_t* start = NULL;
    start = std::search(src, src + size, seq, seq + 3);
    if (start == src + size)
        start = NULL;
    return start;
}

// 7.3.1 NAL unit syntax
bool NalUnit::parseNaluHeader(const uint8_t* data, size_t size)
{
    if (size < NALU_MIN_SIZE)
        return false;

    const uint8_t* pos;

    pos = searchStartPos(data, size);
    if (!pos)
        return false;

    m_data = pos + 3;
    m_size = size - (m_data - data);
    if (m_size < NALU_HEAD_SIZE)
        return false;

    BitReader br(m_data, m_size);

    // forbidden_zero_bit
    br.skip(1);

    nal_unit_type = br.read(6);
    nuh_layer_id = br.read(6);
    nuh_temporal_id_plus1 = br.read(3);

    return true;
}

uint8_t Parser::EXTENDED_SAR = 255;

SharedPtr<VPS> Parser::getVps(uint8_t id) const
{
    SharedPtr<VPS> res;
    VpsMap::const_iterator it = m_vps.find(id);
    if (it != m_vps.end())
        res = it->second;
    return res;
}

SharedPtr<SPS> Parser::getSps(uint8_t id) const
{
    SharedPtr<SPS> res;
    SpsMap::const_iterator it = m_sps.find(id);
    if (it != m_sps.end())
        res = it->second;
    return res;
}

SharedPtr<PPS> Parser::getPps(uint8_t id) const
{
    SharedPtr<PPS> res;
    PpsMap::const_iterator it = m_pps.find(id);
    if (it != m_pps.end())
        res = it->second;
    return res;
}

// 7.3.3 Profile, tier and level syntax
bool Parser::profileTierLevel(ProfileTierLevel* ptl, NalReader& nr,
    uint8_t maxNumSubLayersMinus1)
{
    ptl->general_profile_space = nr.read(2);
    ptl->general_tier_flag = nr.read(1);
    ptl->general_profile_idc = nr.read(5);
    for (uint32_t j = 0; j < 32; j++)
        ptl->general_profile_compatibility_flag[j] = nr.read(1);
    ptl->general_progressive_source_flag = nr.read(1);
    ptl->general_interlaced_source_flag = nr.read(1);
    ptl->general_non_packed_constraint_flag = nr.read(1);
    ptl->general_frame_only_constraint_flag = nr.read(1);
    if (ptl->general_profile_idc == 4
        || ptl->general_profile_compatibility_flag[4]
        || ptl->general_profile_idc == 5
        || ptl->general_profile_compatibility_flag[5]
        || ptl->general_profile_idc == 6
        || ptl->general_profile_compatibility_flag[6]
        || ptl->general_profile_idc == 7
        || ptl->general_profile_compatibility_flag[7]) {
        //The number of bits in this syntax structure is not affected by this condition
        ptl->general_max_12bit_constraint_flag = nr.read(1);
        ptl->general_max_10bit_constraint_flag = nr.read(1);
        ptl->general_max_8bit_constraint_flag = nr.read(1);
        ptl->general_max_422chroma_constraint_flag = nr.read(1);
        ptl->general_max_420chroma_constraint_flag = nr.read(1);
        ptl->general_max_monochrome_constraint_flag = nr.read(1);
        ptl->general_intra_constraint_flag = nr.read(1);
        ptl->general_one_picture_only_constraint_flag = nr.read(1);
        ptl->general_lower_bit_rate_constraint_flag = nr.read(1);
        // general_reserved_zero_34bits
        nr.skip(32);
        nr.skip(2);
    } else {
        //general_reserved_zero_43bits
        nr.skip(32);
        nr.skip(11);
    }
    if ((ptl->general_profile_idc >= 1 && ptl->general_profile_idc <= 5)
        || ptl->general_profile_compatibility_flag[1]
        || ptl->general_profile_compatibility_flag[2]
        || ptl->general_profile_compatibility_flag[3]
        || ptl->general_profile_compatibility_flag[4]
        || ptl->general_profile_compatibility_flag[5])
        //The number of bits in this syntax structure is not affected by this condition
        ptl->general_inbld_flag = nr.read(1);
    else
        nr.skip(1); // general_reserved_zero_bit
    ptl->general_level_idc = nr.read(8);
    for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
        ptl->sub_layer_profile_present_flag[i] = nr.read(1);
        ptl->sub_layer_level_present_flag[i] = nr.read(1);
    }
    if (maxNumSubLayersMinus1 > 0) {
        for (uint32_t i = maxNumSubLayersMinus1; i < 8; i++)
            nr.skip(2); // reserved_zero_2bits
    }
    for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
        if (ptl->sub_layer_profile_present_flag[i]) {
            ptl->sub_layer_profile_space[i] = nr.read(2);
            ptl->sub_layer_tier_flag[i] = nr.read(1);
            ptl->sub_layer_profile_idc[i] = nr.read(5);
            for (uint32_t j = 0; j < 32; j++)
                ptl->sub_layer_profile_compatibility_flag[i][j] = nr.read(1);
            ptl->sub_layer_progressive_source_flag[i] = nr.read(1);
            ptl->sub_layer_interlaced_source_flag[i] = nr.read(1);
            ptl->sub_layer_non_packed_constraint_flag[i] = nr.read(1);
            ptl->sub_layer_frame_only_constraint_flag[i] = nr.read(1);
            if (ptl->sub_layer_profile_idc[i] == 4
                || ptl->sub_layer_profile_compatibility_flag[i][4]
                || ptl->sub_layer_profile_idc[i] == 5
                || ptl->sub_layer_profile_compatibility_flag[i][5]
                || ptl->sub_layer_profile_idc[i] == 6
                || ptl->sub_layer_profile_compatibility_flag[i][6]
                || ptl->sub_layer_profile_idc[i] == 7
                || ptl->sub_layer_profile_compatibility_flag[i][7]) {
                //The number of bits in this syntax structure is not affected by this condition
                ptl->sub_layer_max_12bit_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_max_10bit_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_max_8bit_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_max_422chroma_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_max_420chroma_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_max_monochrome_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_intra_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_one_picture_only_constraint_flag[i] = nr.read(1);
                ptl->sub_layer_lower_bit_rate_constraint_flag[i] = nr.read(1);
                // sub_layer_reserved_zero_34bits
                nr.skip(32);
                nr.skip(2);
            } else {
                //sub_layer_reserved_zero_43bits
                nr.skip(32);
                nr.skip(11);
            }
            if ((ptl->sub_layer_profile_idc[i] >= 1 && ptl->sub_layer_profile_idc[i] <= 5)
                || ptl->sub_layer_profile_compatibility_flag[1]
                || ptl->sub_layer_profile_compatibility_flag[2]
                || ptl->sub_layer_profile_compatibility_flag[3]
                || ptl->sub_layer_profile_compatibility_flag[4]
                || ptl->sub_layer_profile_compatibility_flag[5])
                //The number of bits in this syntax structure is not affected by this condition
                    ptl->sub_layer_inbld_flag[i] = nr.read(1);
            else
                nr.skip(1); // sub_layer_reserved_zero_bit[i]
        }
        if (ptl->sub_layer_level_present_flag[i])
            ptl->sub_layer_level_idc[i] = nr.read(8);
    }

    HAVE_MORE_BITS(nr, 1);

    return true;
}

// E.2.3 Sub-layer HRD parameters syntax
bool Parser::subLayerHrdParameters(SubLayerHRDParameters* subParams,
    NalReader& nr, uint32_t cpbCnt,
    uint8_t subPicParamsPresentFlag)
{
    for (uint32_t i = 0; i <= cpbCnt; i++) {
        subParams->bit_rate_value_minus1[i] = nr.readUe();
        subParams->cpb_size_value_minus1[i] = nr.readUe();
        if (subPicParamsPresentFlag) {
            subParams->cpb_size_du_value_minus1[i] = nr.readUe();
            subParams->bit_rate_du_value_minus1[i] = nr.readUe();
        }
        subParams->cbr_flag[i] = nr.read(1);
    }
    return true;
}

// E.2.2 HRD parameters syntax
bool Parser::hrdParameters(HRDParameters* params, NalReader& nr,
    uint8_t commonInfPresentFlag,
    uint8_t maxNumSubLayersMinus1)
{
    // set default values, when these syntax elements are not present, they are
    // inferred to be euqal to 23.
    params->initial_cpb_removal_delay_length_minus1 = 23;
    params->au_cpb_removal_delay_length_minus1 = 23;
    params->dpb_output_delay_length_minus1 = 23;

    if (commonInfPresentFlag) {
        params->nal_hrd_parameters_present_flag = nr.read(1);
        params->vcl_hrd_parameters_present_flag = nr.read(1);
        if (params->nal_hrd_parameters_present_flag
            || params->vcl_hrd_parameters_present_flag) {
            params->sub_pic_hrd_params_present_flag = nr.read(1);
            if (params->sub_pic_hrd_params_present_flag) {
                params->tick_divisor_minus2 = nr.read(8);
                params->du_cpb_removal_delay_increment_length_minus1 = nr.read(5);
                params->sub_pic_cpb_params_in_pic_timing_sei_flag = nr.read(1);
                params->dpb_output_delay_du_length_minus1 = nr.read(5);
            }
            params->bit_rate_scale = nr.read(4);
            params->cpb_size_scale = nr.read(4);
            if (params->sub_pic_hrd_params_present_flag)
                params->cpb_size_du_scale = nr.read(4);
            params->initial_cpb_removal_delay_length_minus1 = nr.read(5);
            params->au_cpb_removal_delay_length_minus1 = nr.read(5);
            params->dpb_output_delay_length_minus1 = nr.read(5);
        }
    }
    for (uint32_t i = 0; i <= maxNumSubLayersMinus1; i++) {
        params->fixed_pic_rate_general_flag[i] = nr.read(1);

        if (!params->fixed_pic_rate_general_flag[i])
            params->fixed_pic_rate_within_cvs_flag[i] = nr.read(1);
        else
            params->fixed_pic_rate_within_cvs_flag[i] = 1;

        if (params->fixed_pic_rate_within_cvs_flag[i])
            params->elemental_duration_in_tc_minus1[i] = nr.readUe();
        else
            params->low_delay_hrd_flag[i] = nr.read(1);

        if (!params->low_delay_hrd_flag[i])
            params->cpb_cnt_minus1[i] = nr.readUe();

        if (params->nal_hrd_parameters_present_flag) {
            if (!subLayerHrdParameters(&params->sublayer_hrd_params[i], nr,
                    params->cpb_cnt_minus1[i],
                    params->sub_pic_hrd_params_present_flag))
                return false;
        }

        if (params->vcl_hrd_parameters_present_flag) {
            if (!subLayerHrdParameters(&params->sublayer_hrd_params[i], nr,
                    params->cpb_cnt_minus1[i],
                    params->sub_pic_hrd_params_present_flag))
                return false;
        }
    }

    HAVE_MORE_BITS(nr, 1);

    return true;
}

// E.2 VUI parameters syntax
bool Parser::vuiParameters(SPS* sps, NalReader& nr)
{
    if (!sps)
        return false;

    VuiParameters* vui = &sps->vui_params;

    // set default values
    vui->video_format = 5; // MAC
    // the chromaticity is unspecified or is determined by the application
    vui->colour_primaries = 2;
    // the transfer characteristics are unspecified or are determined by the
    // application
    vui->transfer_characteristics = 2;
    vui->matrix_coeffs = 2;
    vui->motion_vectors_over_pic_boundaries_flag = 1;
    vui->max_bytes_per_pic_denom = 2;
    vui->max_bits_per_min_cu_denom = 1;
    vui->log2_max_mv_length_horizontal = 15;
    vui->log2_max_mv_length_vertical = 15;
    if (sps->profile_tier_level.general_progressive_source_flag && sps->profile_tier_level.general_interlaced_source_flag)
        vui->frame_field_info_present_flag = 1;

    vui->aspect_ratio_info_present_flag = nr.read(1);
    if (vui->aspect_ratio_info_present_flag) {
        vui->aspect_ratio_idc = nr.read(8);
        if (vui->aspect_ratio_idc == EXTENDED_SAR) {
            vui->sar_width = nr.read(16);
            vui->sar_height = nr.read(16);
        }
    }
    vui->overscan_info_present_flag = nr.read(1);
    if (vui->overscan_info_present_flag)
        vui->overscan_appropriate_flag = nr.read(1);
    vui->video_signal_type_present_flag = nr.read(1);
    if (vui->video_signal_type_present_flag) {
        vui->video_format = nr.read(3);
        vui->video_full_range_flag = nr.read(1);
        vui->colour_description_present_flag = nr.read(1);
        if (vui->colour_description_present_flag) {
            vui->colour_primaries = nr.read(8);
            vui->transfer_characteristics = nr.read(8);
            vui->matrix_coeffs = nr.read(8);
        }
    }
    vui->chroma_loc_info_present_flag = nr.read(1);
    if (vui->chroma_loc_info_present_flag) {
        vui->chroma_sample_loc_type_top_field = nr.readUe();
        vui->chroma_sample_loc_type_bottom_field = nr.readUe();
    }
    vui->neutral_chroma_indication_flag = nr.read(1);
    vui->field_seq_flag = nr.read(1);
    vui->frame_field_info_present_flag = nr.read(1);
    vui->default_display_window_flag = nr.read(1);
    if (vui->default_display_window_flag) {
        vui->def_disp_win_left_offset = nr.readUe();
        vui->def_disp_win_right_offset = nr.readUe();
        vui->def_disp_win_top_offset = nr.readUe();
        vui->def_disp_win_bottom_offset = nr.readUe();
    }
    vui->vui_timing_info_present_flag = nr.read(1);
    if (vui->vui_timing_info_present_flag) {
        vui->vui_num_units_in_tick = nr.read(32);
        vui->vui_time_scale = nr.read(32);
        vui->vui_poc_proportional_to_timing_flag = nr.read(1);
        if (vui->vui_poc_proportional_to_timing_flag)
            vui->vui_num_ticks_poc_diff_one_minus1 = nr.readUe();

        vui->vui_hrd_parameters_present_flag = nr.read(1);
        if (vui->vui_hrd_parameters_present_flag)
            if (!hrdParameters(&vui->hrd_params, nr, 1,
                    sps->sps_max_sub_layers_minus1))
                return false;
    }
    vui->bitstream_restriction_flag = nr.read(1);
    if (vui->bitstream_restriction_flag) {
        vui->tiles_fixed_structure_flag = nr.read(1);
        vui->motion_vectors_over_pic_boundaries_flag = nr.read(1);
        vui->restricted_ref_pic_lists_flag = nr.read(1);
        vui->min_spatial_segmentation_idc = nr.readUe();
        vui->max_bytes_per_pic_denom = nr.readUe();
        vui->max_bits_per_min_cu_denom = nr.readUe();
        vui->log2_max_mv_length_horizontal = nr.readUe();
        vui->log2_max_mv_length_vertical = nr.readUe();
    }

    return true;
}

bool Parser::useDefaultScalingLists(uint8_t* dstList, uint8_t* dstDcList,
    uint8_t sizeId, uint8_t matrixId)
{
    // Table 7-3-Specification of siezId
    switch (sizeId) {
    case 0: // 4x4
        memcpy(dstList, DefaultScalingList0, 16);
        break;
    case 1: // 8x8
    case 2: // 16x16
        if (matrixId <= 2)
            memcpy(dstList, DefaultScalingList1, 64);
        else
            memcpy(dstList, DefaultScalingList2, 64);
        break;
    case 3: // 32x32
        if (!matrixId)
            memcpy(dstList, DefaultScalingList1, 64);
        else
            memcpy(dstList, DefaultScalingList2, 64);
        break;
    default:
        return false;
    }

    if (sizeId > 1)
        dstDcList[matrixId] = 8;

    return true;
}

// 7.3.4 Scaling list data syntax
bool Parser::scalingListData(ScalingList* dest_scaling_list, NalReader& nr)
{
    uint8_t* dstDcList = NULL;
    uint8_t* dstList = NULL;
    uint8_t* refList = NULL;

    size_t size = 64;
    uint8_t refMatrixId = 0;
    bool scaling_list_pred_mode_flag = false;
    uint8_t scaling_list_pred_matrix_id_delta = 0;
    uint8_t nextCoef;
    uint8_t coefNum;
    int16_t scaling_list_delta_coef;

    for (uint32_t sizeId = 0; sizeId < 4; sizeId++) {
        for (uint32_t matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6);
             matrixId++) {
            size = 64;
            // Table 7-3
            switch (sizeId) {
            case 0: // 4x4
                dstList = dest_scaling_list->scalingList4x4[matrixId];
                size = 16;
                break;
            case 1: // 8x8
                dstList = dest_scaling_list->scalingList8x8[matrixId];
                break;
            case 2: // 16x16
                dstList = dest_scaling_list->scalingList16x16[matrixId];
                dstDcList = dest_scaling_list->scalingListDC16x16;
                break;
            case 3: // 32x32
                dstList = dest_scaling_list->scalingList32x32[matrixId];
                dstDcList = dest_scaling_list->scalingListDC32x32;
            }

            scaling_list_pred_mode_flag = nr.read(1);
            if (!scaling_list_pred_mode_flag) {
                scaling_list_pred_matrix_id_delta = nr.readUe();
                if (!scaling_list_pred_matrix_id_delta) {
                    if (!useDefaultScalingLists(dstList, dstDcList, sizeId, matrixId))
                        return false;
                } else {
                    //7-40
                    refMatrixId = matrixId - scaling_list_pred_matrix_id_delta * (sizeId == 3 ? 3 : 1);
                    // get referrence list
                    switch (sizeId) {
                    case 0: // 4x4
                        refList = dest_scaling_list->scalingList4x4[refMatrixId];
                        break;
                    case 1: // 8x8
                        refList = dest_scaling_list->scalingList8x8[refMatrixId];
                        break;
                    case 2: // 16x16
                        refList = dest_scaling_list->scalingList16x16[refMatrixId];
                        break;
                    case 3: // 32x32
                        refList = dest_scaling_list->scalingList32x32[refMatrixId];
                    }

                    for (uint32_t i = 0; i < size; i++)
                        dstList[i] = refList[i];

                    if (sizeId > 1)
                        dstDcList[matrixId] = dstDcList[refMatrixId];
                }
            } else {
                nextCoef = 8;
                coefNum = std::min(64, (1 << (4 + (sizeId << 1))));

                if (sizeId > 1) {
                    dstDcList[matrixId] = nr.readSe();
                    nextCoef = dstDcList[matrixId] + 8;
                }

                for (uint32_t i = 0; i < coefNum; i++) {
                    scaling_list_delta_coef = nr.readSe();
                    CHECK_RANGE(scaling_list_delta_coef, -128, 127);
                    nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
                    dstList[i] = nextCoef;
                }
            }
        }
    }

    return true;
}

// 7.3.7 Short-term reference picture set syntax
bool Parser::stRefPicSet(ShortTermRefPicSet* stRef, NalReader& nr,
    uint8_t stRpsIdx, SPS* sps)
{
    int32_t i, j;
    uint8_t refRpsIdx = 0;
    int32_t deltaRps = 0;
    int32_t dPoc;
    ShortTermRefPicSet* refPic;

    // When use_delta_flag[ j ] is not present, its value is inferred to be equal
    // to 1.
    for (j = 0; j < 16; j++)
        stRef->use_delta_flag[j] = 1;

    if (stRpsIdx != 0)
        stRef->inter_ref_pic_set_prediction_flag = nr.read(1);
    if (stRef->inter_ref_pic_set_prediction_flag) {
        if (stRpsIdx == sps->num_short_term_ref_pic_sets)
            stRef->delta_idx_minus1 = nr.readUe();
        // 7-57
        refRpsIdx = stRpsIdx - (stRef->delta_idx_minus1 + 1);
        stRef->delta_rps_sign = nr.read(1);
        stRef->abs_delta_rps_minus1 = nr.readUe();
        // 7-58
        deltaRps = (1 - 2 * stRef->delta_rps_sign) * (stRef->abs_delta_rps_minus1 + 1);

        refPic = &sps->short_term_ref_pic_set[refRpsIdx];
        for (j = 0; j <= refPic->NumDeltaPocs; j++) {
            stRef->used_by_curr_pic_flag[j] = nr.read(1);
            if (!stRef->used_by_curr_pic_flag[j])
                stRef->use_delta_flag[j] = nr.read(1);
        }

        // 7-59
        i = 0;
        for (j = refPic->NumPositivePics - 1; j >= 0; j--) {
            dPoc = refPic->DeltaPocS1[j] + deltaRps;
            if (dPoc < 0 && stRef->use_delta_flag[refPic->NumNegativePics + j]) {
                stRef->DeltaPocS0[i] = dPoc;
                stRef->UsedByCurrPicS0[i++] = stRef->used_by_curr_pic_flag[refPic->NumNegativePics + j];
            }
        }
        if (deltaRps < 0 && stRef->use_delta_flag[refPic->NumDeltaPocs]) {
            stRef->DeltaPocS0[i] = deltaRps;
            stRef->UsedByCurrPicS0[i++] = stRef->used_by_curr_pic_flag[refPic->NumDeltaPocs];
        }
        for (j = 0; j < refPic->NumNegativePics; j++) {
            dPoc = refPic->DeltaPocS0[j] + deltaRps;
            if (dPoc < 0 && stRef->use_delta_flag[j]) {
                stRef->DeltaPocS0[i] = dPoc;
                stRef->UsedByCurrPicS0[i++] = stRef->used_by_curr_pic_flag[j];
            }
        }
        stRef->NumNegativePics = i;

        // 7-60
        i = 0;
        for (j = refPic->NumNegativePics - 1; j >= 0; j--) {
            dPoc = refPic->DeltaPocS0[j] + deltaRps;
            if (dPoc > 0 && stRef->use_delta_flag[j]) {
                stRef->DeltaPocS1[i] = dPoc;
                stRef->UsedByCurrPicS1[i++] = stRef->used_by_curr_pic_flag[j];
            }
        }
        if (deltaRps > 0 && stRef->use_delta_flag[refPic->NumDeltaPocs]) {
            stRef->DeltaPocS1[i] = deltaRps;
            stRef->UsedByCurrPicS1[i++] = stRef->used_by_curr_pic_flag[refPic->NumDeltaPocs];
        }
        for (j = 0; j < refPic->NumPositivePics; j++) {
            dPoc = refPic->DeltaPocS1[j] + deltaRps;
            if (dPoc > 0 && stRef->use_delta_flag[refPic->NumNegativePics + j]) {
                stRef->DeltaPocS1[i] = dPoc;
                stRef->UsedByCurrPicS1[i++] = stRef->used_by_curr_pic_flag[refPic->NumNegativePics + j];
            }
        }
        stRef->NumPositivePics = i;
    }
    else {
        stRef->num_negative_pics = nr.readUe();
        stRef->num_positive_pics = nr.readUe();
        // 7-61 & 7-62
        stRef->NumNegativePics = stRef->num_negative_pics;
        stRef->NumPositivePics = stRef->num_positive_pics;

        for (i = 0; i < stRef->num_negative_pics; i++) {
            stRef->delta_poc_s0_minus1[i] = nr.readUe();
            if (i == 0) // 7-65
                stRef->DeltaPocS0[i] = -(stRef->delta_poc_s0_minus1[i] + 1);
            else // 7-67
                stRef->DeltaPocS0[i] = stRef->DeltaPocS0[i - 1] - (stRef->delta_poc_s0_minus1[i] + 1);

            stRef->used_by_curr_pic_s0_flag[i] = nr.read(1);
            // 7-63
            stRef->UsedByCurrPicS0[i] = stRef->used_by_curr_pic_s0_flag[i];
        }
        for (i = 0; i < stRef->num_positive_pics; i++) {
            stRef->delta_poc_s1_minus1[i] = nr.readUe();
            if (i == 0)
                stRef->DeltaPocS1[i] = stRef->delta_poc_s1_minus1[i] + 1;
            else
                stRef->DeltaPocS1[i] = stRef->DeltaPocS1[i - 1] + (stRef->delta_poc_s1_minus1[i] + 1);

            stRef->used_by_curr_pic_s1_flag[i] = nr.read(1);
            // 7-64
            stRef->UsedByCurrPicS1[i] = stRef->used_by_curr_pic_s1_flag[i];
        }
    }

    stRef->NumDeltaPocs = stRef->NumPositivePics + stRef->NumNegativePics;

    return true;
}

// 7.3.6.2 Reference picture list modification syntax
bool Parser::refPicListsModification(SliceHeader* slice, NalReader& nr,
    int32_t numPicTotalCurr)
{
    uint32_t nbits = ceil(log2(numPicTotalCurr));
    RefPicListModification* rplm = &slice->ref_pic_list_modification;
    memset(rplm, 0, sizeof(RefPicListModification));
    rplm->ref_pic_list_modification_flag_l0 = nr.read(1);
    if (rplm->ref_pic_list_modification_flag_l0) {
        for (uint32_t i = 0; i <= slice->num_ref_idx_l0_active_minus1; i++) {
            rplm->list_entry_l0[i] = nr.read(nbits);
        }
    }
    if (slice->isBSlice()) {
        rplm->ref_pic_list_modification_flag_l1 = nr.read(1);
        if (rplm->ref_pic_list_modification_flag_l1)
            for (uint32_t i = 0; i <= slice->num_ref_idx_l1_active_minus1; i++) {
                rplm->list_entry_l1[i] = nr.read(nbits);
            }
    }

    return true;
}

//merge parse l0 and l1 for function predWeightTable
#define SUB_PRED_WEIGHT_TABLE(slice, pwt, nr, sps, mode)                           \
    {                                                                              \
        uint32_t i, j;                                                             \
        for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++)           \
            pwt->luma_weight_##mode##_flag[i] = nr.read(1);                        \
        if (sps->chroma_format_idc) {                                              \
            for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++)       \
                pwt->chroma_weight_##mode##_flag[i] = nr.read(1);                  \
        }                                                                          \
        for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++) {         \
            if (pwt->luma_weight_##mode##_flag[i]) {                               \
                pwt->delta_luma_weight_##mode[i] = nr.readSe();                    \
                CHECK_RANGE(pwt->delta_luma_weight_##mode[i], -128, 127);          \
                pwt->luma_offset_##mode[i] = nr.readSe();                          \
            }                                                                      \
            if (pwt->chroma_weight_##mode##_flag[i]) {                             \
                for (j = 0; j < 2; j++) {                                          \
                    pwt->delta_chroma_weight_##mode[i][j] = nr.readSe();           \
                    CHECK_RANGE(pwt->delta_chroma_weight_##mode[i][j], -128, 127); \
                    pwt->delta_chroma_offset_##mode[i][j] = nr.readSe();           \
                }                                                                  \
            }                                                                      \
        }                                                                          \
    }

// 7.3.6.3 Weighted prediction parameters syntax
bool Parser::predWeightTable(SliceHeader* slice, NalReader& nr)
{
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
    PredWeightTable* const pwt = &slice->pred_weight_table;

    pwt->luma_log2_weight_denom = nr.readUe();
    if (pwt->luma_log2_weight_denom > 7)
        return false; // shall be in the range of 0 to 7, inclusive.
    if (sps->chroma_format_idc)
        pwt->delta_chroma_log2_weight_denom = nr.readSe();

    SUB_PRED_WEIGHT_TABLE(slice, pwt, nr, sps, l0);

    if (slice->isBSlice())
        SUB_PRED_WEIGHT_TABLE(slice, pwt, nr, sps, l1);

    return true;
}

// 7.3.2.1 Video parameter set RBSP syntax
bool Parser::parseVps(const NalUnit* nalu)
{
    SharedPtr<VPS> vps(new VPS());

    NalReader nr(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    vps->vps_id = nr.read(4);
    vps->vps_base_layer_internal_flag = nr.read(1);
    vps->vps_base_layer_available_flag = nr.read(1);
    vps->vps_max_layers_minus1 = nr.read(6);
    vps->vps_max_sub_layers_minus1 = nr.read(3);
    if (vps->vps_max_sub_layers_minus1 >= MAXSUBLAYERS)
        return false;
    vps->vps_temporal_id_nesting_flag = nr.read(1);
    nr.skip(16); // vps_reserved_0xffff_16bits
    if (!profileTierLevel(&vps->profile_tier_level, nr,
            vps->vps_max_sub_layers_minus1))
        return false;
    vps->vps_sub_layer_ordering_info_present_flag = nr.read(1);

    //parse
    PARSE_SUB_LAYER_ORDERING_INFO(vps, nr);

    // set default values
    // 1) When vps_max_dec_pic_buffering_minus1[ i ] is not present for i in the
    //    range of 0 to vps_max_sub_layers_minus1 -1, inclusive, due to
    //    vps_sub_layer_ordering_info_present_flag being equal to 0, it is
    //    inferred
    //    to be equal to vps_max_dec_pic_buffering_minus1[
    //    vps_max_sub_layers_minus1 ].
    // 2) When vps_base_layer_internal_flag is equal to 0,
    // vps_max_dec_pic_buffering_minus1[ i ]
    //    shall be equal to 0 and decoders shall ignore the value of
    //    vps_max_dec_pic_buffering_minus1[ i ].
    SET_DEF_FOR_ORDERING_INFO(vps);

    vps->vps_max_layer_id = nr.read(6);
    // shall be in the range of 0 to 1023, inclusive.
    vps->vps_num_layer_sets_minus1 = nr.readUe();
    if (vps->vps_num_layer_sets_minus1 > 1023)
        return false;

    for (uint32_t i = 1; i <= vps->vps_num_layer_sets_minus1; i++) {
        for (uint32_t j = 0; j <= vps->vps_max_layer_id; j++)
            nr.skip(1); // layer_id_included_flag
    }
    vps->vps_timing_info_present_flag = nr.read(1);
    if (vps->vps_timing_info_present_flag) {
        vps->vps_num_units_in_tick = nr.read(32);
        vps->vps_time_scale = nr.read(32);
        vps->vps_poc_proportional_to_timing_flag = nr.read(1);
        if (vps->vps_poc_proportional_to_timing_flag)
            vps->vps_num_ticks_poc_diff_one_minus1 = nr.readUe();
        vps->vps_num_hrd_parameters = nr.readUe();
        // vps_num_hrd_parameters shall be in the range of [0,
        // vps_num_layer_sets_minus1 + 1],
        // and vps_num_layer_sets_minus shall be in the range [0, 1023]
        if (vps->vps_num_hrd_parameters > vps->vps_num_layer_sets_minus1 + 1)
            return false;
        vps->hrd_layer_set_idx = (uint16_t*)calloc(vps->vps_num_hrd_parameters, sizeof(uint16_t));
        vps->cprms_present_flag = (uint8_t*)calloc(vps->vps_num_hrd_parameters, sizeof(uint8_t));
        if (!vps->hrd_layer_set_idx || !vps->cprms_present_flag)
            return false; // calloc error
        for (uint32_t i = 0; i < vps->vps_num_hrd_parameters; i++) {
            vps->hrd_layer_set_idx[i] = nr.readUe();
            if (i > 0)
                vps->cprms_present_flag[i] = nr.read(1);
            hrdParameters(&vps->hrd_parameters, nr, vps->cprms_present_flag[i],
                vps->vps_max_sub_layers_minus1);
        }
    }

    HAVE_MORE_BITS(nr, 1);

    vps->vps_extension_flag = nr.read(1);
    if (vps->vps_extension_flag) {
        while (nr.moreRbspData())
            nr.skip(1); // vps_extension_data_flag
    }
    nr.rbspTrailingBits();
    m_vps[vps->vps_id] = vps;

    return true;
}

// 7.3.2.2 Sequence parameter set RBSP syntax
bool Parser::parseSps(const NalUnit* nalu)
{
    SharedPtr<SPS> sps(new SPS());
    SharedPtr<VPS> vps;
    // Table 6-1
    uint8_t subWidthC[5] = { 1, 2, 2, 1, 1 };
    uint8_t subHeightC[5] = { 1, 2, 1, 1, 1 };

    NalReader nr(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    sps->vps_id = nr.read(4);
    vps = getVps(sps->vps_id);
    if (!vps)
        return false;
    sps->vps = vps;

    sps->sps_max_sub_layers_minus1 = nr.read(3);
    if (sps->sps_max_sub_layers_minus1 >= MAXSUBLAYERS)
        return false;

    sps->sps_temporal_id_nesting_flag = nr.read(1);

    if (!profileTierLevel(&sps->profile_tier_level, nr,
            sps->sps_max_sub_layers_minus1))
        return false;

    sps->sps_id = nr.readUe();
    if (sps->sps_id > MAXSPSCOUNT)
        return false;

    sps->chroma_format_idc = nr.readUe();
    // shall be in the range of 0 to 3, inclusive.
    if (sps->chroma_format_idc > 3)
        return false;
    if (sps->chroma_format_idc == 3)
        sps->separate_colour_plane_flag = nr.read(1);

    // used for parsing slice
    if (sps->separate_colour_plane_flag)
        sps->chroma_array_type = 0;
    else
        sps->chroma_array_type = sps->chroma_format_idc;

    sps->pic_width_in_luma_samples = nr.readUe();
    sps->pic_height_in_luma_samples = nr.readUe();
    sps->conformance_window_flag = nr.read(1);
    if (sps->conformance_window_flag) {
        sps->conf_win_left_offset = nr.readUe();
        sps->conf_win_right_offset = nr.readUe();
        sps->conf_win_top_offset = nr.readUe();
        sps->conf_win_bottom_offset = nr.readUe();
    }
    sps->width = sps->pic_width_in_luma_samples;
    sps->height = sps->pic_height_in_luma_samples;
    if (sps->conformance_window_flag) {
        //D-28
        sps->croppedWidth = sps->pic_width_in_luma_samples -
                             subWidthC[sps->chroma_format_idc] *
                             (sps->conf_win_left_offset + sps->conf_win_right_offset);
        //D-29
        sps->croppedHeight = sps->pic_height_in_luma_samples -
                             subHeightC[sps->chroma_format_idc] *
                             (sps->conf_win_top_offset + sps->conf_win_bottom_offset);
    }

    sps->bit_depth_luma_minus8 = nr.readUe();
    sps->bit_depth_chroma_minus8 = nr.readUe();
    if (sps->bit_depth_luma_minus8 > 8 || sps->bit_depth_chroma_minus8 > 8)
        return false;

    sps->log2_max_pic_order_cnt_lsb_minus4 = nr.readUe();
    if (sps->log2_max_pic_order_cnt_lsb_minus4 > 12)
        return false;

    sps->sps_sub_layer_ordering_info_present_flag = nr.read(1);

    //parse
    PARSE_SUB_LAYER_ORDERING_INFO(sps, nr);

    // set default values
    // When sps_max_dec_pic_buffering_minus1[ i ] is not present for i
    // in the range of0 to sps_max_sub_layers_minus1 - 1, inclusive,
    // due to sps_sub_layer_ordering_info_present_flag being equal to 0,
    // it is inferred to be equal to sps_max_dec_pic_buffering_minus1[
    // sps_max_sub_layers_minus1 ].
    // And also apply for sps_max_num_reorder_pics[i] and
    // sps_max_latency_increase_plus1[i].
    SET_DEF_FOR_ORDERING_INFO(sps);

    sps->log2_min_luma_coding_block_size_minus3 = nr.readUe();
    sps->log2_diff_max_min_luma_coding_block_size = nr.readUe();
    sps->log2_min_transform_block_size_minus2 = nr.readUe();
    sps->log2_diff_max_min_transform_block_size = nr.readUe();
    sps->max_transform_hierarchy_depth_inter = nr.readUe();
    sps->max_transform_hierarchy_depth_intra = nr.readUe();
    sps->scaling_list_enabled_flag = nr.read(1);
    if (sps->scaling_list_enabled_flag) {
        sps->sps_scaling_list_data_present_flag = nr.read(1);
        if (sps->sps_scaling_list_data_present_flag) {
            if (!scalingListData(&sps->scaling_list, nr))
                return false;
        }
    }
    sps->amp_enabled_flag = nr.read(1);
    sps->sample_adaptive_offset_enabled_flag = nr.read(1);
    sps->pcm_enabled_flag = nr.read(1);
    if (sps->pcm_enabled_flag) {
        sps->pcm_sample_bit_depth_luma_minus1 = nr.read(4);
        sps->pcm_sample_bit_depth_chroma_minus1 = nr.read(4);
        sps->log2_min_pcm_luma_coding_block_size_minus3 = nr.readUe();
        sps->log2_diff_max_min_pcm_luma_coding_block_size = nr.readUe();
        sps->pcm_loop_filter_disabled_flag = nr.read(1);
    }
    sps->num_short_term_ref_pic_sets = nr.readUe();
    if (sps->num_short_term_ref_pic_sets > MAXSHORTTERMRPSCOUNT)
        return false;

    for (uint32_t i = 0; i < sps->num_short_term_ref_pic_sets; i++) {
        if (!stRefPicSet(&sps->short_term_ref_pic_set[i], nr, i, sps.get()))
            return false;
    }

    sps->long_term_ref_pics_present_flag = nr.read(1);
    if (sps->long_term_ref_pics_present_flag) {
        sps->num_long_term_ref_pics_sps = nr.readUe();
        uint32_t nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
        for (uint32_t i = 0; i < sps->num_long_term_ref_pics_sps; i++) {
            sps->lt_ref_pic_poc_lsb_sps[i] = nr.read(nbits);
            sps->used_by_curr_pic_lt_sps_flag[i] = nr.read(1);
        }
    }

    sps->temporal_mvp_enabled_flag = nr.read(1);
    sps->strong_intra_smoothing_enabled_flag = nr.read(1);
    sps->vui_parameters_present_flag = nr.read(1);
    if (sps->vui_parameters_present_flag) {
        if (!vuiParameters(sps.get(), nr))
            return false;
    }

    HAVE_MORE_BITS(nr, 1);

    sps->sps_extension_present_flag = nr.read(1);

    // remaining some extension elements, it is not necessary for me, so ignore.
    // maybe add in the future.

    m_sps[sps->sps_id] = sps;

    return true;
}

// 7.3.2.3 Picture parameter set RBSP syntax
bool Parser::parsePps(const NalUnit* nalu)
{
    SharedPtr<PPS> pps(new PPS());
    SharedPtr<SPS> sps;

    uint32_t minCbLog2SizeY;
    uint32_t ctbLog2SizeY;
    uint32_t ctbSizeY;

    NalReader nr(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    // set default values
    pps->uniform_spacing_flag = 1;
    pps->loop_filter_across_tiles_enabled_flag = 1;

    pps->pps_id = nr.readUe();
    if (pps->pps_id > MAXPPSCOUNT)
        return false;

    pps->sps_id = nr.readUe();
    if (pps->sps_id > MAXSPSCOUNT)
        return false;

    sps = getSps(pps->sps_id);
    if (!sps)
        return false;
    pps->sps = sps;

    // 7-10
    minCbLog2SizeY = sps->log2_min_luma_coding_block_size_minus3 + 3;
    // 7-11
    ctbLog2SizeY = minCbLog2SizeY + sps->log2_diff_max_min_luma_coding_block_size;
    // 7-13
    ctbSizeY = 1 << ctbLog2SizeY;
    // 7-15
    pps->picWidthInCtbsY = ceil((double)sps->pic_width_in_luma_samples / (double)ctbSizeY);
    // 7-17
    pps->picHeightInCtbsY = ceil((double)sps->pic_height_in_luma_samples / (double)ctbSizeY);

    pps->dependent_slice_segments_enabled_flag = nr.read(1);
    pps->output_flag_present_flag = nr.read(1);
    pps->num_extra_slice_header_bits = nr.read(3);
    pps->sign_data_hiding_enabled_flag = nr.read(1);
    pps->cabac_init_present_flag = nr.read(1);
    pps->num_ref_idx_l0_default_active_minus1 = nr.readUe();
    pps->num_ref_idx_l1_default_active_minus1 = nr.readUe();
    pps->init_qp_minus26 = nr.readSe();
    pps->constrained_intra_pred_flag = nr.read(1);
    pps->transform_skip_enabled_flag = nr.read(1);
    pps->cu_qp_delta_enabled_flag = nr.read(1);
    if (pps->cu_qp_delta_enabled_flag)
        pps->diff_cu_qp_delta_depth = nr.readUe();
    pps->pps_cb_qp_offset = nr.readSe();
    pps->pps_cr_qp_offset = nr.readSe();
    pps->slice_chroma_qp_offsets_present_flag = nr.read(1);
    pps->weighted_pred_flag = nr.read(1);
    pps->weighted_bipred_flag = nr.read(1);
    pps->transquant_bypass_enabled_flag = nr.read(1);
    pps->tiles_enabled_flag = nr.read(1);
    pps->entropy_coding_sync_enabled_flag = nr.read(1);
    if (pps->tiles_enabled_flag) {
        pps->num_tile_columns_minus1 = nr.readUe();
        pps->num_tile_rows_minus1 = nr.readUe();
        pps->uniform_spacing_flag = nr.read(1);
        if (pps->uniform_spacing_flag) {
            uint8_t numCol = pps->num_tile_columns_minus1 + 1;
            uint8_t numRow = pps->num_tile_rows_minus1 + 1;
            for (uint32_t i = 0; i < numCol; i++) {
                pps->column_width_minus1[i] = (i + 1) * pps->picWidthInCtbsY / numCol - i * pps->picWidthInCtbsY / numCol - 1;
            }
            for (uint32_t i = 0; i < numRow; i++) {
                pps->row_height_minus1[i] = (i + 1) * pps->picHeightInCtbsY / numRow - i * pps->picHeightInCtbsY / numRow - 1;
            }
        } else {
            pps->column_width_minus1[pps->num_tile_columns_minus1] = pps->picWidthInCtbsY - 1;
            for (uint32_t i = 0; i < pps->num_tile_columns_minus1; i++) {
                pps->column_width_minus1[i] = nr.readUe();
                pps->column_width_minus1[pps->num_tile_columns_minus1] -= (pps->column_width_minus1[i] + 1);
            }

            pps->row_height_minus1[pps->num_tile_rows_minus1] = pps->picHeightInCtbsY - 1;
            for (uint32_t i = 0; i < pps->num_tile_rows_minus1; i++) {
                pps->row_height_minus1[i] = nr.readUe();
                pps->row_height_minus1[pps->num_tile_rows_minus1] -= (pps->row_height_minus1[i] + 1);
            }
        }
        pps->loop_filter_across_tiles_enabled_flag = nr.read(1);
    }
    pps->pps_loop_filter_across_slices_enabled_flag = nr.read(1);
    pps->deblocking_filter_control_present_flag = nr.read(1);
    if (pps->deblocking_filter_control_present_flag) {
        pps->deblocking_filter_override_enabled_flag = nr.read(1);
        pps->pps_deblocking_filter_disabled_flag = nr.read(1);
        if (!pps->pps_deblocking_filter_disabled_flag) {
            pps->pps_beta_offset_div2 = nr.readSe();
            pps->pps_tc_offset_div2 = nr.readSe();
        }
    }
    pps->pps_scaling_list_data_present_flag = nr.read(1);
    if (pps->pps_scaling_list_data_present_flag) {
        if (!scalingListData(&pps->scaling_list, nr))
            return false;
    }
    // When scaling_list_enabled_flag is equal to 1,
    // sps_scaling_list_data_present_flag
    // is equal to 0 and pps_scaling_list_data_present_flag is equal to 0, the
    // default
    // scaling list data are used to derive the array ScalingFactor as described
    // in the
    // scaling list data semantics as specified in clause 7.4.5.
    if (sps->scaling_list_enabled_flag && !sps->sps_scaling_list_data_present_flag && !pps->pps_scaling_list_data_present_flag) {
        uint8_t* dstList = NULL;
        uint8_t* dstDcList = NULL;
        for (uint32_t sizeId = 0; sizeId < 4; sizeId++) {
            for (uint32_t matrixId = 0; matrixId < ((sizeId == 3) ? 2 : 6);
                 matrixId++) {
                switch (sizeId) {
                case 0: // 4x4
                    dstList = pps->scaling_list.scalingList4x4[matrixId];
                    break;
                case 1: // 8x8
                    dstList = pps->scaling_list.scalingList8x8[matrixId];
                    break;
                case 2: // 16x16
                    dstList = pps->scaling_list.scalingList16x16[matrixId];
                    dstDcList = pps->scaling_list.scalingListDC16x16;
                    break;
                case 3: // 32x32
                    dstList = pps->scaling_list.scalingList32x32[matrixId];
                    dstDcList = pps->scaling_list.scalingListDC32x32;
                }
                useDefaultScalingLists(dstList, dstDcList, sizeId, matrixId);
            }
        }
    }

    pps->lists_modification_present_flag = nr.read(1);
    pps->log2_parallel_merge_level_minus2 = nr.readUe();
    pps->slice_segment_header_extension_present_flag = nr.read(1);

    HAVE_MORE_BITS(nr, 1);

    pps->pps_extension_present_flag = nr.read(1);
    if (pps->pps_extension_present_flag) {
        HAVE_MORE_BITS(nr, 8);
        pps->pps_range_extension_flag = nr.read(1);
        pps->pps_multilayer_extension_flag = nr.read(1);
        pps->pps_3d_extension_flag = nr.read(1);
        HAVE_MORE_BITS(nr, 5);
        pps->pps_extension_5bits = nr.read(5);
    }

    // 7.3.2.3.2 Picture parameter set range extension syntax
    if (pps->pps_range_extension_flag) {
        if (pps->transform_skip_enabled_flag)
            pps->log2_max_transform_skip_block_size_minus2 = nr.readUe();
        pps->cross_component_prediction_enabled_flag = nr.read(1);
        pps->chroma_qp_offset_list_enabled_flag = nr.read(1);
        if (pps->chroma_qp_offset_list_enabled_flag) {
            pps->diff_cu_chroma_qp_offset_depth = nr.readUe();
            pps->chroma_qp_offset_list_len_minus1 = nr.readUe();
            for (uint32_t i = 0; i <= pps->chroma_qp_offset_list_len_minus1; i++) {
                pps->cb_qp_offset_list[i] = nr.readSe();
                pps->cr_qp_offset_list[i] = nr.readSe();
                CHECK_RANGE(pps->cb_qp_offset_list[i], -12, 12);
                CHECK_RANGE(pps->cr_qp_offset_list[i], -12, 12);
            }
        }
        pps->log2_sao_offset_scale_luma = nr.readUe();
        HAVE_MORE_BITS(nr, 1);
        pps->log2_sao_offset_scale_chroma = nr.readUe();

        // 7-4 & 7-6
        int32_t bitDepthY = 8 + sps->bit_depth_luma_minus8;
        int32_t bitDepthc = 8 + sps->bit_depth_chroma_minus8;

        int32_t maxValue = std::max(0, bitDepthY - 10);
        if (pps->log2_sao_offset_scale_luma > maxValue)
            return false;

        maxValue = std::max(0, bitDepthc - 10);
        if (pps->log2_sao_offset_scale_chroma > maxValue)
            return false;
    }

    m_pps[pps->pps_id] = pps;

    return true;
}

// 7.3.6 Slice segment header syntax
bool Parser::parseSlice(const NalUnit* nalu, SliceHeader* slice)
{
    uint32_t nbits;
    SharedPtr<PPS> pps;
    SharedPtr<SPS> sps;
    ShortTermRefPicSet* stRPS = NULL;
    uint32_t UsedByCurrPicLt[16] = {0};
    int32_t numPicTotalCurr = 0;

    if (!nalu || !slice)
        return false;

    NalReader nr(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    slice->first_slice_segment_in_pic_flag = nr.read(1);
    if (nalu->nal_unit_type >= NalUnit::BLA_W_LP && nalu->nal_unit_type <= NalUnit::RSV_IRAP_VCL23)
        slice->no_output_of_prior_pics_flag = nr.read(1);

    slice->pps_id = nr.readUe();
    if (slice->pps_id > MAXPPSCOUNT)
        return false;

    pps = getPps(slice->pps_id);
    if (!pps)
        return false;
    slice->pps = pps;

    sps = pps->sps;
    if (!sps)
        return false;

    // set default values
    slice->pic_output_flag = 1;
    slice->collocated_from_l0_flag = 1;
    slice->beta_offset_div2 = pps->pps_beta_offset_div2;
    slice->tc_offset_div2 = pps->pps_tc_offset_div2;
    slice->loop_filter_across_slices_enabled_flag = pps->pps_loop_filter_across_slices_enabled_flag;

    if (!slice->first_slice_segment_in_pic_flag) {
        nbits = ceil(log2(pps->picWidthInCtbsY * pps->picHeightInCtbsY));
        if (pps->dependent_slice_segments_enabled_flag)
            slice->dependent_slice_segment_flag = nr.read(1);
        slice->slice_segment_address = nr.read(nbits);
    }

    if (!slice->dependent_slice_segment_flag) {
        for (uint32_t i = 0; i < pps->num_extra_slice_header_bits; i++)
            nr.skip(1); // slice_reserved_flag

        // 0 for B slice, 1 for P slice and 2 for I slice
        slice->slice_type = nr.readUe();
        if (slice->slice_type > 2)
            return false;

        if (pps->output_flag_present_flag)
            slice->pic_output_flag = nr.read(1);
        if (sps->separate_colour_plane_flag == 1)
            slice->colour_plane_id = nr.read(2);
        if ((nalu->nal_unit_type != NalUnit::IDR_W_RADL) && (nalu->nal_unit_type != NalUnit::IDR_N_LP)) {
            nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
            slice->slice_pic_order_cnt_lsb = nr.read(nbits);
            slice->short_term_ref_pic_set_sps_flag = nr.read(1);
            if (!slice->short_term_ref_pic_set_sps_flag) {
                if (!stRefPicSet(&slice->short_term_ref_pic_sets, nr,
                        sps->num_short_term_ref_pic_sets, sps.get()))
                    return false;
            }
            else if (sps->num_short_term_ref_pic_sets > 1) {
                nbits = ceil(log2(sps->num_short_term_ref_pic_sets));
                slice->short_term_ref_pic_set_idx = nr.read(nbits);
            }
            if (sps->long_term_ref_pics_present_flag) {
                if (sps->num_long_term_ref_pics_sps > 0) {
                    slice->num_long_term_sps = nr.readUe();
                    if (slice->num_long_term_sps > sps->num_long_term_ref_pics_sps)
                        return false;
                }
                slice->num_long_term_pics = nr.readUe();
                for (uint32_t i = 0;
                     i < slice->num_long_term_sps + slice->num_long_term_pics; i++) {
                    if (i < slice->num_long_term_sps) {
                        if (sps->num_long_term_ref_pics_sps > 1) {
                            nbits = ceil(log2(sps->num_long_term_ref_pics_sps));
                            slice->lt_idx_sps[i] = nr.read(nbits);
                            if (slice->lt_idx_sps[i] > sps->num_long_term_ref_pics_sps - 1)
                                return false;
                        }
                    } else {
                        nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
                        slice->poc_lsb_lt[i] = nr.read(nbits);
                        slice->used_by_curr_pic_lt_flag[i] = nr.read(1);
                    }

                    if (i < slice->num_long_term_sps)
                        UsedByCurrPicLt[i] = sps->used_by_curr_pic_lt_sps_flag[slice->lt_idx_sps[i]];
                    else
                        UsedByCurrPicLt[i] = slice->used_by_curr_pic_lt_flag[i];

                    slice->delta_poc_msb_present_flag[i] = nr.read(1);
                    if (slice->delta_poc_msb_present_flag[i])
                        slice->delta_poc_msb_cycle_lt[i] = nr.readUe();
                }
            }
            if (sps->temporal_mvp_enabled_flag)
                slice->temporal_mvp_enabled_flag = nr.read(1);
        }
        if (sps->sample_adaptive_offset_enabled_flag) {
            slice->sao_luma_flag = nr.read(1);
            if (sps->chroma_array_type)
                slice->sao_chroma_flag = nr.read(1);
        }
        if (slice->isPSlice() || slice->isBSlice()) {
            slice->num_ref_idx_active_override_flag = nr.read(1);
            if (slice->num_ref_idx_active_override_flag) {
                slice->num_ref_idx_l0_active_minus1 = nr.readUe();
                if (slice->isBSlice())
                    slice->num_ref_idx_l1_active_minus1 = nr.readUe();
            } else {
                //When the current slice is a P or B slice and num_ref_idx_l0_active_minus1
                //is not present, num_ref_idx_l0_active_minus1 is inferred to be equal to
                //num_ref_idx_l0_default_active_minus1, and num_ref_idx_l1_active_minus1
                //is inferred to be equal to num_ref_idx_l1_default_active_minus1.
                slice->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
                slice->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
            }

            // shall be in the range of 0 to 14
            if (slice->num_ref_idx_l0_active_minus1 > 14 || slice->num_ref_idx_l1_active_minus1 > 14)
                return false;

            // according to 7-55
            if (!slice->short_term_ref_pic_set_sps_flag)
                stRPS = &slice->short_term_ref_pic_sets;
            else
                stRPS = &sps->short_term_ref_pic_set[slice->short_term_ref_pic_set_idx];
            for (uint32_t i = 0; i < stRPS->NumNegativePics; i++) {
                if (stRPS->UsedByCurrPicS0[i])
                    numPicTotalCurr++;
            }
            for (uint32_t i = 0; i < stRPS->NumPositivePics; i++) {
                if (stRPS->UsedByCurrPicS1[i])
                    numPicTotalCurr++;
            }
            for (uint32_t i = 0;
                 i < (slice->num_long_term_sps + slice->num_long_term_pics); i++) {
                if (UsedByCurrPicLt[i])
                    numPicTotalCurr++;
            }

            if (pps->lists_modification_present_flag && numPicTotalCurr > 1) {
                if (!refPicListsModification(slice, nr, numPicTotalCurr))
                    return false;
            }

            if (slice->isBSlice())
                slice->mvd_l1_zero_flag = nr.read(1);
            if (pps->cabac_init_present_flag)
                slice->cabac_init_flag = nr.read(1);
            if (slice->temporal_mvp_enabled_flag) {
                if (slice->isBSlice())
                    slice->collocated_from_l0_flag = nr.read(1);

                if ((slice->collocated_from_l0_flag
                        && slice->num_ref_idx_l0_active_minus1 > 0)
                    || (!slice->collocated_from_l0_flag
                        && slice->num_ref_idx_l1_active_minus1 > 0)) {
                    slice->collocated_ref_idx = nr.readUe();
                }
            }
            if ((pps->weighted_pred_flag && slice->isPSlice()) || (pps->weighted_bipred_flag && slice->isBSlice())) {
                if (!predWeightTable(slice, nr))
                    return false;
            }
            slice->five_minus_max_num_merge_cand = nr.readUe();
            if (slice->five_minus_max_num_merge_cand > 4)
                return false;
        }
        slice->qp_delta = nr.readSe();
        if (pps->slice_chroma_qp_offsets_present_flag) {
            slice->cb_qp_offset = nr.readSe();
            slice->cr_qp_offset = nr.readSe();
            CHECK_RANGE(slice->cb_qp_offset, -12, 12);
            CHECK_RANGE(slice->cr_qp_offset, -12, 12);
        }
        if (pps->chroma_qp_offset_list_enabled_flag)
            slice->cu_chroma_qp_offset_enabled_flag = nr.read(1);
        if (pps->deblocking_filter_override_enabled_flag)
            slice->deblocking_filter_override_flag = nr.read(1);
        if (slice->deblocking_filter_override_flag) {
            slice->deblocking_filter_disabled_flag = nr.read(1);
            if (!slice->deblocking_filter_disabled_flag) {
                slice->beta_offset_div2 = nr.readSe();
                slice->tc_offset_div2 = nr.readSe();
                CHECK_RANGE(slice->beta_offset_div2, -6, 6);
                CHECK_RANGE(slice->tc_offset_div2, -6, 6);
            }
        }

        if (pps->pps_loop_filter_across_slices_enabled_flag
            && (slice->sao_luma_flag
                || slice->sao_chroma_flag
                || !slice->deblocking_filter_disabled_flag)) {
            slice->loop_filter_across_slices_enabled_flag = nr.read(1);
        }
    }

    if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag) {
        slice->num_entry_point_offsets = nr.readUe();
        if (slice->num_entry_point_offsets > 0) {
            slice->offset_len_minus1 = nr.readUe();
            // the value of offset_len_minus1 shall be in the range of [0, 31]
            if (slice->offset_len_minus1 > 31)
                return false;
            nbits = slice->offset_len_minus1 + 1;
            slice->entry_point_offset_minus1 = static_cast<uint32_t*>(
                calloc(slice->num_entry_point_offsets, sizeof(uint32_t)));
            if (!slice->entry_point_offset_minus1)
                return false; // calloc error
            for (uint32_t i = 0; i < slice->num_entry_point_offsets; i++)
                slice->entry_point_offset_minus1[i] = nr.read(nbits);
        }
    }
    if (pps->slice_segment_header_extension_present_flag) {
        slice->slice_segment_header_extension_length = nr.readUe();
        for (uint32_t i = 0; i < slice->slice_segment_header_extension_length; i++)
            nr.skip(8); // slice_segment_header_extension_data_byte
    }

    // byte_alignment()
    nr.skip(1); // alignment_bit_equal_to_one
    while (nr.getPos() & 7)
        nr.skip(1);

    slice->headerSize = nr.getPos();
    slice->emulationPreventionBytes = nr.getEpbCnt();

    HAVE_MORE_BITS(nr, 1);

    return true;
}

}
}
