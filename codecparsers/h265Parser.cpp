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

#include "h265Parser.h"

#include <string.h>
#include <math.h>
#include <stddef.h> //offsetof

#define POWER32SUB2 0xFFFFFFFE
#define POWER15 (1 << 15)

#define CHECK_RANGE_INT32(var, min, max)                                         \
    {                                                                            \
        if ((var) < (min) || (var) > (max)) {                                    \
            ERROR("%s(%d) should be in the range[%d, %d]", #var, var, min, max); \
            return false;                                                        \
        }                                                                        \
    }

#define CHECK_RANGE_UINT32(var, min, max)                                        \
    {                                                                            \
        if ((var) < (min) || (var) > (max)) {                                    \
            ERROR("%s(%u) should be in the range[%u, %u]", #var, var, min, max); \
            return false;                                                        \
        }                                                                        \
    }

#define READ(f)                             \
    do {                                    \
        if (!br.readT(f)) {                 \
            ERROR("failed to read %s", #f); \
            return false;                   \
        }                                   \
    } while (0)

#define READ_BITS(f, bits)                              \
    do {                                                \
        if (!br.readT(f, bits)) {                       \
            ERROR("failed to read %d to %s", bits, #f); \
            return false;                               \
        }                                               \
    } while (0)

#define CHECK_READ_BITS(f, bits, min, max) \
    do {                                   \
        READ_BITS(f, bits);                \
        CHECK_RANGE_UINT32(f, min, max);   \
    } while (0)

#define READ_UE(f)                            \
    do {                                      \
        if (!br.readUe(f)) {                  \
            ERROR("failed to readUe %s", #f); \
            return false;                     \
        }                                     \
    } while (0)

#define CHECK_READ_UE(f, min, max)       \
    do {                                 \
        READ_UE(f);                      \
        CHECK_RANGE_UINT32(f, min, max); \
    } while (0)

#define READ_SE(f)                            \
    do {                                      \
        if (!br.readSe(f)) {                  \
            ERROR("failed to readSe %s", #f); \
            return false;                     \
        }                                     \
    } while (0)

#define CHECK_READ_SE(f, min, max)      \
    do {                                \
        READ_SE(f);                     \
        CHECK_RANGE_INT32(f, min, max); \
    } while (0)

#define SKIP(bits)                   \
    do {                             \
        if (!br.skip(bits)) {        \
            ERROR("failed to skip"); \
            return false;            \
        }                            \
    } while (0)

namespace YamiParser {
namespace H265 {

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

#define PARSE_SUB_LAYER_ORDERING_INFO(var, br)                                                                   \
    {                                                                                                            \
        uint32_t start = var->var##_sub_layer_ordering_info_present_flag ? 0 : var->var##_max_sub_layers_minus1; \
        for (uint32_t i = start; i <= var->var##_max_sub_layers_minus1; i++) {                                   \
            CHECK_READ_UE(var->var##_max_dec_pic_buffering_minus1[i], 0, MAXDPBSIZE - 1);                        \
                                                                                                                 \
            CHECK_READ_UE(var->var##_max_num_reorder_pics[i], 0, var->var##_max_dec_pic_buffering_minus1[i]);    \
                                                                                                                 \
            CHECK_READ_UE(var->var##_max_latency_increase_plus1[i], 0, POWER32SUB2);                             \
        }                                                                                                        \
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
    memset(this, 0, offsetof(VPS, hrd_layer_set_idx));
}

VPS::~VPS()
{
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
    if (!data || !size) {
        ERROR("data is NULL, or size is 0");
        return false;
    }

    m_data = data;
    m_size = size;
    if (m_size < NALU_HEAD_SIZE) {
        ERROR("m_size(%d) < NALU_HEAD_SIZE(%d)", m_size, NALU_HEAD_SIZE);
        return false;
    }

    BitReader br(m_data, m_size);

    // forbidden_zero_bit
    SKIP(1);

    READ_BITS(nal_unit_type, 6);
    READ_BITS(nuh_layer_id, 6);
    READ_BITS(nuh_temporal_id_plus1, 3);

    return true;
}

uint8_t Parser::EXTENDED_SAR = 255;

SharedPtr<VPS> Parser::getVps(uint8_t id) const
{
    SharedPtr<VPS> res;
    VpsMap::const_iterator it = m_vps.find(id);
    if (it != m_vps.end())
        res = it->second;
    else
        WARNING("can't get the VPS by ID(%d)", id);
    return res;
}

SharedPtr<SPS> Parser::getSps(uint8_t id) const
{
    SharedPtr<SPS> res;
    SpsMap::const_iterator it = m_sps.find(id);
    if (it != m_sps.end())
        res = it->second;
    else
        WARNING("can't get the SPS by ID(%d)", id);
    return res;
}

SharedPtr<PPS> Parser::getPps(uint8_t id) const
{
    SharedPtr<PPS> res;
    PpsMap::const_iterator it = m_pps.find(id);
    if (it != m_pps.end())
        res = it->second;
    else
        WARNING("can't get the PPS by ID(%d)", id);
    return res;
}

// 7.3.3 Profile, tier and level syntax
bool Parser::profileTierLevel(ProfileTierLevel* ptl, NalReader& br,
    uint8_t maxNumSubLayersMinus1)
{
    READ_BITS(ptl->general_profile_space, 2);
    READ(ptl->general_tier_flag);
    READ_BITS(ptl->general_profile_idc, 5);
    for (uint32_t j = 0; j < 32; j++)
        READ(ptl->general_profile_compatibility_flag[j]);
    READ(ptl->general_progressive_source_flag);
    READ(ptl->general_interlaced_source_flag);
    READ(ptl->general_non_packed_constraint_flag);
    READ(ptl->general_frame_only_constraint_flag);
    if (ptl->general_profile_idc == 4
        || ptl->general_profile_compatibility_flag[4]
        || ptl->general_profile_idc == 5
        || ptl->general_profile_compatibility_flag[5]
        || ptl->general_profile_idc == 6
        || ptl->general_profile_compatibility_flag[6]
        || ptl->general_profile_idc == 7
        || ptl->general_profile_compatibility_flag[7]) {
        //The number of bits in this syntax structure is not affected by this condition
        READ(ptl->general_max_12bit_constraint_flag);
        READ(ptl->general_max_10bit_constraint_flag);
        READ(ptl->general_max_8bit_constraint_flag);
        READ(ptl->general_max_422chroma_constraint_flag);
        READ(ptl->general_max_420chroma_constraint_flag);
        READ(ptl->general_max_monochrome_constraint_flag);
        READ(ptl->general_intra_constraint_flag);
        READ(ptl->general_one_picture_only_constraint_flag);
        READ(ptl->general_lower_bit_rate_constraint_flag);
        // general_reserved_zero_34bits
        SKIP(34);
    } else {
        //general_reserved_zero_43bits
        SKIP(43);
    }
    if ((ptl->general_profile_idc >= 1 && ptl->general_profile_idc <= 5)
        || ptl->general_profile_compatibility_flag[1]
        || ptl->general_profile_compatibility_flag[2]
        || ptl->general_profile_compatibility_flag[3]
        || ptl->general_profile_compatibility_flag[4]
        || ptl->general_profile_compatibility_flag[5])
        //The number of bits in this syntax structure is not affected by this condition
        READ(ptl->general_inbld_flag);
    else
        SKIP(1); // general_reserved_zero_bit
    READ(ptl->general_level_idc);
    for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
        READ(ptl->sub_layer_profile_present_flag[i]);
        READ(ptl->sub_layer_level_present_flag[i]);
    }
    if (maxNumSubLayersMinus1 > 0) {
        for (uint32_t i = maxNumSubLayersMinus1; i < 8; i++)
            SKIP(2); // reserved_zero_2bits
    }
    for (uint32_t i = 0; i < maxNumSubLayersMinus1; i++) {
        if (ptl->sub_layer_profile_present_flag[i]) {
            READ_BITS(ptl->sub_layer_profile_space[i], 2);
            READ(ptl->sub_layer_tier_flag[i]);
            READ_BITS(ptl->sub_layer_profile_idc[i], 5);
            for (uint32_t j = 0; j < 32; j++)
                READ(ptl->sub_layer_profile_compatibility_flag[i][j]);
            READ(ptl->sub_layer_progressive_source_flag[i]);
            READ(ptl->sub_layer_interlaced_source_flag[i]);
            READ(ptl->sub_layer_non_packed_constraint_flag[i]);
            READ(ptl->sub_layer_frame_only_constraint_flag[i]);
            if (ptl->sub_layer_profile_idc[i] == 4
                || ptl->sub_layer_profile_compatibility_flag[i][4]
                || ptl->sub_layer_profile_idc[i] == 5
                || ptl->sub_layer_profile_compatibility_flag[i][5]
                || ptl->sub_layer_profile_idc[i] == 6
                || ptl->sub_layer_profile_compatibility_flag[i][6]
                || ptl->sub_layer_profile_idc[i] == 7
                || ptl->sub_layer_profile_compatibility_flag[i][7]) {
                //The number of bits in this syntax structure is not affected by this condition
                READ(ptl->sub_layer_max_12bit_constraint_flag[i]);
                READ(ptl->sub_layer_max_10bit_constraint_flag[i]);
                READ(ptl->sub_layer_max_8bit_constraint_flag[i]);
                READ(ptl->sub_layer_max_422chroma_constraint_flag[i]);
                READ(ptl->sub_layer_max_420chroma_constraint_flag[i]);
                READ(ptl->sub_layer_max_monochrome_constraint_flag[i]);
                READ(ptl->sub_layer_intra_constraint_flag[i]);
                READ(ptl->sub_layer_one_picture_only_constraint_flag[i]);
                READ(ptl->sub_layer_lower_bit_rate_constraint_flag[i]);
                // sub_layer_reserved_zero_34bits
                SKIP(34);
            } else {
                //sub_layer_reserved_zero_43bits
                SKIP(43);
            }
            if ((ptl->sub_layer_profile_idc[i] >= 1 && ptl->sub_layer_profile_idc[i] <= 5)
                || ptl->sub_layer_profile_compatibility_flag[1]
                || ptl->sub_layer_profile_compatibility_flag[2]
                || ptl->sub_layer_profile_compatibility_flag[3]
                || ptl->sub_layer_profile_compatibility_flag[4]
                || ptl->sub_layer_profile_compatibility_flag[5])
                //The number of bits in this syntax structure is not affected by this condition
                READ(ptl->sub_layer_inbld_flag[i]);
            else
                SKIP(1); // sub_layer_reserved_zero_bit[i]
        }
        if (ptl->sub_layer_level_present_flag[i])
            READ(ptl->sub_layer_level_idc[i]);
    }

    return true;
}

// E.2.3 Sub-layer HRD parameters syntax
bool Parser::subLayerHrdParameters(SubLayerHRDParameters* subParams,
    NalReader& br, uint32_t cpbCnt,
    uint8_t subPicParamsPresentFlag)
{
    for (uint32_t i = 0; i <= cpbCnt; i++) {
        if (i == 0) {
            CHECK_READ_UE(subParams->bit_rate_value_minus1[i], 0, POWER32SUB2);
            CHECK_READ_UE(subParams->cpb_size_value_minus1[i], 0, POWER32SUB2);
        }
        else {
            CHECK_READ_UE(subParams->bit_rate_value_minus1[i], subParams->bit_rate_value_minus1[i - 1], POWER32SUB2);
            CHECK_READ_UE(subParams->cpb_size_value_minus1[i], 0, subParams->cpb_size_value_minus1[i - 1] + 1);
        }
        if (subPicParamsPresentFlag) {
            if (i == 0) {
                CHECK_READ_UE(subParams->cpb_size_du_value_minus1[i], 0, POWER32SUB2);
                CHECK_READ_UE(subParams->bit_rate_du_value_minus1[i], 0, POWER32SUB2);
            }
            else {
                CHECK_READ_UE(subParams->cpb_size_du_value_minus1[i], 0, subParams->cpb_size_du_value_minus1[i - 1] + 1);
                CHECK_READ_UE(subParams->bit_rate_du_value_minus1[i], subParams->bit_rate_du_value_minus1[i - 1], POWER32SUB2);
            }
        }
        READ(subParams->cbr_flag[i]);
    }
    return true;
}

// E.2.2 HRD parameters syntax
bool Parser::hrdParameters(HRDParameters* params, NalReader& br,
    uint8_t commonInfPresentFlag,
    uint8_t maxNumSubLayersMinus1)
{
    // set default values, when these syntax elements are not present, they are
    // inferred to be euqal to 23.
    params->initial_cpb_removal_delay_length_minus1 = 23;
    params->au_cpb_removal_delay_length_minus1 = 23;
    params->dpb_output_delay_length_minus1 = 23;

    if (commonInfPresentFlag) {
        READ(params->nal_hrd_parameters_present_flag);
        READ(params->vcl_hrd_parameters_present_flag);
        if (params->nal_hrd_parameters_present_flag
            || params->vcl_hrd_parameters_present_flag) {
            READ(params->sub_pic_hrd_params_present_flag);
            if (params->sub_pic_hrd_params_present_flag) {
                READ(params->tick_divisor_minus2);
                READ_BITS(params->du_cpb_removal_delay_increment_length_minus1, 5);
                READ(params->sub_pic_cpb_params_in_pic_timing_sei_flag);
                READ_BITS(params->dpb_output_delay_du_length_minus1, 5);
            }
            READ_BITS(params->bit_rate_scale, 4);
            READ_BITS(params->cpb_size_scale, 4);
            if (params->sub_pic_hrd_params_present_flag)
                READ_BITS(params->cpb_size_du_scale, 4);
            READ_BITS(params->initial_cpb_removal_delay_length_minus1, 5);
            READ_BITS(params->au_cpb_removal_delay_length_minus1, 5);
            READ_BITS(params->dpb_output_delay_length_minus1, 5);
        }
    }
    for (uint32_t i = 0; i <= maxNumSubLayersMinus1; i++) {
        READ(params->fixed_pic_rate_general_flag[i]);

        if (!params->fixed_pic_rate_general_flag[i])
            READ(params->fixed_pic_rate_within_cvs_flag[i]);
        else
            params->fixed_pic_rate_within_cvs_flag[i] = 1;

        if (params->fixed_pic_rate_within_cvs_flag[i])
            CHECK_READ_UE(params->elemental_duration_in_tc_minus1[i], 0, 2047);
        else
            READ(params->low_delay_hrd_flag[i]);

        if (!params->low_delay_hrd_flag[i])
            CHECK_READ_UE(params->cpb_cnt_minus1[i], 0, 31);

        if (params->nal_hrd_parameters_present_flag) {
            if (!subLayerHrdParameters(&params->sublayer_hrd_params[i], br,
                    params->cpb_cnt_minus1[i],
                    params->sub_pic_hrd_params_present_flag))
                return false;
        }

        if (params->vcl_hrd_parameters_present_flag) {
            if (!subLayerHrdParameters(&params->sublayer_hrd_params[i], br,
                    params->cpb_cnt_minus1[i],
                    params->sub_pic_hrd_params_present_flag))
                return false;
        }
    }

    return true;
}

// E.2 VUI parameters syntax
bool Parser::vuiParameters(SPS* sps, NalReader& br)
{
    if (!sps) {
        ERROR("SPS is NULL");
        return false;
    }

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

    READ(vui->aspect_ratio_info_present_flag);
    if (vui->aspect_ratio_info_present_flag) {
        READ(vui->aspect_ratio_idc);
        if (vui->aspect_ratio_idc == EXTENDED_SAR) {
            READ(vui->sar_width);
            READ(vui->sar_height);
        }
    }
    READ(vui->overscan_info_present_flag);
    if (vui->overscan_info_present_flag)
        READ(vui->overscan_appropriate_flag);
    READ(vui->video_signal_type_present_flag);
    if (vui->video_signal_type_present_flag) {
        READ_BITS(vui->video_format, 3);
        READ(vui->video_full_range_flag);
        READ(vui->colour_description_present_flag);
        if (vui->colour_description_present_flag) {
            READ(vui->colour_primaries);
            READ(vui->transfer_characteristics);
            READ(vui->matrix_coeffs);
        }
    }
    READ(vui->chroma_loc_info_present_flag);
    if (vui->chroma_loc_info_present_flag) {
        CHECK_READ_UE(vui->chroma_sample_loc_type_top_field, 0, 5);
        CHECK_READ_UE(vui->chroma_sample_loc_type_bottom_field, 0, 5);
    }
    READ(vui->neutral_chroma_indication_flag);
    READ(vui->field_seq_flag);
    READ(vui->frame_field_info_present_flag);
    READ(vui->default_display_window_flag);
    if (vui->default_display_window_flag) {
        READ_UE(vui->def_disp_win_left_offset);
        READ_UE(vui->def_disp_win_right_offset);
        READ_UE(vui->def_disp_win_top_offset);
        READ_UE(vui->def_disp_win_bottom_offset);
    }
    READ(vui->vui_timing_info_present_flag);
    if (vui->vui_timing_info_present_flag) {
        READ(vui->vui_num_units_in_tick);
        READ(vui->vui_time_scale);
        READ(vui->vui_poc_proportional_to_timing_flag);
        if (vui->vui_poc_proportional_to_timing_flag)
            CHECK_READ_UE(vui->vui_num_ticks_poc_diff_one_minus1, 0, POWER32SUB2);

        READ(vui->vui_hrd_parameters_present_flag);
        if (vui->vui_hrd_parameters_present_flag)
            if (!hrdParameters(&vui->hrd_params, br, 1,
                    sps->sps_max_sub_layers_minus1))
                return false;
    }
    READ(vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        READ(vui->tiles_fixed_structure_flag);
        READ(vui->motion_vectors_over_pic_boundaries_flag);
        READ(vui->restricted_ref_pic_lists_flag);
        CHECK_READ_UE(vui->min_spatial_segmentation_idc, 0, 4095);
        CHECK_READ_UE(vui->max_bytes_per_pic_denom, 0, 16);
        CHECK_READ_UE(vui->max_bits_per_min_cu_denom, 0, 16);
        CHECK_READ_UE(vui->log2_max_mv_length_horizontal, 0, 16);
        CHECK_READ_UE(vui->log2_max_mv_length_vertical, 0, 15);
    }
    else {
        vui->motion_vectors_over_pic_boundaries_flag = 1;
        vui->log2_max_mv_length_horizontal = 15;
        vui->log2_max_mv_length_vertical = 15;
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
        ERROR("Can't get the scaling list by sizeId(%d)", sizeId);
        return false;
    }

    if (sizeId > 1)
        dstDcList[matrixId] = 16;

    return true;
}

// 7.3.4 Scaling list data syntax
bool Parser::scalingListData(ScalingList* dest_scaling_list, NalReader& br)
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
        for (uint32_t matrixId = 0; matrixId < 6;
             matrixId += (sizeId == 3) ? 3 : 1) {
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

            READ_BITS(scaling_list_pred_mode_flag, 1);
            if (!scaling_list_pred_mode_flag) {
                if (sizeId < 3) {
                    CHECK_READ_UE(scaling_list_pred_matrix_id_delta, 0, matrixId);
                }
                else if (3 == sizeId) {
                    // as spec "7.4.5 Scaling list data semantics",
                    // matrixId should be equal to 3 when scaling_list_pred_matrix_id_delta
                    // is greater than 0.
                    CHECK_READ_UE(scaling_list_pred_matrix_id_delta, 0, matrixId / 3);
                }
                else {
                    ERROR("sizeId(%u) should be in the range of[0, 3].", sizeId);
                    return false;
                }
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
                    int32_t scaling_list_dc_coef_minus8;
                    CHECK_READ_SE(scaling_list_dc_coef_minus8, -7, 247);
                    dstDcList[matrixId] = scaling_list_dc_coef_minus8 + 8;
                    nextCoef = dstDcList[matrixId];
                }

                for (uint32_t i = 0; i < coefNum; i++) {
                    CHECK_READ_SE(scaling_list_delta_coef, -128, 127);
                    nextCoef = (nextCoef + scaling_list_delta_coef + 256) % 256;
                    dstList[i] = nextCoef;
                }
            }
        }
    }

    return true;
}

// 7.3.7 Short-term reference picture set syntax
bool Parser::stRefPicSet(ShortTermRefPicSet* stRef, NalReader& br,
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
        READ(stRef->inter_ref_pic_set_prediction_flag);
    if (stRef->inter_ref_pic_set_prediction_flag) {
        if (stRpsIdx == sps->num_short_term_ref_pic_sets)
            CHECK_READ_UE(stRef->delta_idx_minus1, 0, stRpsIdx);
        // 7-57
        refRpsIdx = stRpsIdx - (stRef->delta_idx_minus1 + 1);
        READ(stRef->delta_rps_sign);
        CHECK_READ_UE(stRef->abs_delta_rps_minus1, 0, POWER15 - 1);
        // 7-58
        deltaRps = (1 - 2 * stRef->delta_rps_sign) * (stRef->abs_delta_rps_minus1 + 1);

        refPic = &sps->short_term_ref_pic_set[refRpsIdx];
        for (j = 0; j <= refPic->NumDeltaPocs; j++) {
            READ(stRef->used_by_curr_pic_flag[j]);
            if (!stRef->used_by_curr_pic_flag[j])
                READ(stRef->use_delta_flag[j]);
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
        uint8_t maxDecPicBufferingMinus1 = sps->sps_max_dec_pic_buffering_minus1[sps->sps_max_sub_layers_minus1];

        CHECK_READ_UE(stRef->num_negative_pics, 0, maxDecPicBufferingMinus1);

        CHECK_READ_UE(stRef->num_positive_pics, 0, maxDecPicBufferingMinus1 - stRef->num_negative_pics);

        // 7-61 & 7-62
        stRef->NumNegativePics = stRef->num_negative_pics;
        stRef->NumPositivePics = stRef->num_positive_pics;

        for (i = 0; i < stRef->num_negative_pics; i++) {
            //delta_poc_s0_minus1 is equal to -3 in some clips.
            READ_UE(stRef->delta_poc_s0_minus1[i]);
            if (i == 0) // 7-65
                stRef->DeltaPocS0[i] = -(stRef->delta_poc_s0_minus1[i] + 1);
            else // 7-67
                stRef->DeltaPocS0[i] = stRef->DeltaPocS0[i - 1] - (stRef->delta_poc_s0_minus1[i] + 1);

            READ(stRef->used_by_curr_pic_s0_flag[i]);
            // 7-63
            stRef->UsedByCurrPicS0[i] = stRef->used_by_curr_pic_s0_flag[i];
        }
        for (i = 0; i < stRef->num_positive_pics; i++) {
            READ_UE(stRef->delta_poc_s1_minus1[i]);
            if (i == 0)
                stRef->DeltaPocS1[i] = stRef->delta_poc_s1_minus1[i] + 1;
            else
                stRef->DeltaPocS1[i] = stRef->DeltaPocS1[i - 1] + (stRef->delta_poc_s1_minus1[i] + 1);

            READ(stRef->used_by_curr_pic_s1_flag[i]);
            // 7-64
            stRef->UsedByCurrPicS1[i] = stRef->used_by_curr_pic_s1_flag[i];
        }
    }

    stRef->NumDeltaPocs = stRef->NumPositivePics + stRef->NumNegativePics;

    return true;
}

// 7.3.6.2 Reference picture list modification syntax
bool Parser::refPicListsModification(SliceHeader* slice, NalReader& br,
    int32_t numPicTotalCurr)
{
    uint32_t nbits = ceil(log2(numPicTotalCurr));
    RefPicListModification* rplm = &slice->ref_pic_list_modification;
    memset(rplm, 0, sizeof(RefPicListModification));
    READ(rplm->ref_pic_list_modification_flag_l0);
    if (rplm->ref_pic_list_modification_flag_l0) {
        for (uint32_t i = 0; i <= slice->num_ref_idx_l0_active_minus1; i++) {
            READ_BITS(rplm->list_entry_l0[i], nbits);
        }
    }
    if (slice->isBSlice()) {
        READ(rplm->ref_pic_list_modification_flag_l1);
        if (rplm->ref_pic_list_modification_flag_l1)
            for (uint32_t i = 0; i <= slice->num_ref_idx_l1_active_minus1; i++) {
                READ_BITS(rplm->list_entry_l1[i], nbits);
            }
    }

    return true;
}

//merge parse l0 and l1 for function predWeightTable
#define SUB_PRED_WEIGHT_TABLE(slice, pwt, br, sps, mode)                                 \
    {                                                                                    \
        uint32_t i, j;                                                                   \
        for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++)                 \
            READ_BITS(pwt->luma_weight_##mode##_flag[i], 1);                             \
        if (sps->chroma_format_idc) {                                                    \
            for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++)             \
                READ_BITS(pwt->chroma_weight_##mode##_flag[i], 1);                       \
        }                                                                                \
        for (i = 0; i <= slice->num_ref_idx_##mode##_active_minus1; i++) {               \
            if (pwt->luma_weight_##mode##_flag[i]) {                                     \
                READ_SE(pwt->delta_luma_weight_##mode[i]);                               \
                CHECK_RANGE_INT32(pwt->delta_luma_weight_##mode[i], -128, 127);          \
                READ_SE(pwt->luma_offset_##mode[i]);                                     \
            }                                                                            \
            if (pwt->chroma_weight_##mode##_flag[i]) {                                   \
                for (j = 0; j < 2; j++) {                                                \
                    READ_SE(pwt->delta_chroma_weight_##mode[i][j]);                      \
                    CHECK_RANGE_INT32(pwt->delta_chroma_weight_##mode[i][j], -128, 127); \
                    READ_SE(pwt->delta_chroma_offset_##mode[i][j]);                      \
                }                                                                        \
            }                                                                            \
        }                                                                                \
    }

// 7.3.6.3 Weighted prediction parameters syntax
bool Parser::predWeightTable(SliceHeader* slice, NalReader& br)
{
    const PPS* const pps = slice->pps.get();
    const SPS* const sps = pps->sps.get();
    PredWeightTable* const pwt = &slice->pred_weight_table;

    // shall be in the range of 0 to 7, inclusive.
    CHECK_READ_UE(pwt->luma_log2_weight_denom, 0, 7);

    if (sps->chroma_format_idc)
        READ_SE(pwt->delta_chroma_log2_weight_denom);

    SUB_PRED_WEIGHT_TABLE(slice, pwt, br, sps, l0);

    if (slice->isBSlice())
        SUB_PRED_WEIGHT_TABLE(slice, pwt, br, sps, l1);

    return true;
}

// 7.3.2.1 Video parameter set RBSP syntax
bool Parser::parseVps(const NalUnit* nalu)
{
    SharedPtr<VPS> vps(new VPS());

    NalReader br(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    READ_BITS(vps->vps_id, 4);
    READ(vps->vps_base_layer_internal_flag);
    READ(vps->vps_base_layer_available_flag);
    READ_BITS(vps->vps_max_layers_minus1, 6);
    CHECK_READ_BITS(vps->vps_max_sub_layers_minus1, 3, 0, MAXSUBLAYERS - 1);
    READ(vps->vps_temporal_id_nesting_flag);
    SKIP(16); // vps_reserved_0xffff_16bits
    if (!profileTierLevel(&vps->profile_tier_level, br,
            vps->vps_max_sub_layers_minus1))
        return false;
    READ(vps->vps_sub_layer_ordering_info_present_flag);

    //parse
    PARSE_SUB_LAYER_ORDERING_INFO(vps, br);

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

    READ_BITS(vps->vps_max_layer_id, 6);
    // shall be in the range of 0 to 1023, inclusive.
    CHECK_READ_UE(vps->vps_num_layer_sets_minus1, 0, 1023);

    for (uint32_t i = 1; i <= vps->vps_num_layer_sets_minus1; i++) {
        for (uint32_t j = 0; j <= vps->vps_max_layer_id; j++)
            SKIP(1); // layer_id_included_flag
    }
    READ(vps->vps_timing_info_present_flag);
    if (vps->vps_timing_info_present_flag) {
        READ(vps->vps_num_units_in_tick);
        READ(vps->vps_time_scale);
        READ(vps->vps_poc_proportional_to_timing_flag);
        if (vps->vps_poc_proportional_to_timing_flag)
            CHECK_READ_UE(vps->vps_num_ticks_poc_diff_one_minus1, 0, POWER32SUB2);

        // vps_num_hrd_parameters shall be in the range of [0,
        // vps_num_layer_sets_minus1 + 1],
        // and vps_num_layer_sets_minus shall be in the range [0, 1023]
        CHECK_READ_UE(vps->vps_num_hrd_parameters, 0, vps->vps_num_layer_sets_minus1 + 1);
        vps->hrd_layer_set_idx.reserve(vps->vps_num_hrd_parameters);
        vps->cprms_present_flag.resize(vps->vps_num_hrd_parameters, 0);
        uint32_t idxMin = vps->vps_base_layer_internal_flag ? 0 : 1;
        for (uint32_t i = 0; i < vps->vps_num_hrd_parameters; i++) {
            uint32_t hrd_layer_set_idx;
            CHECK_READ_UE(hrd_layer_set_idx, idxMin, vps->vps_num_layer_sets_minus1);
            vps->hrd_layer_set_idx.push_back(hrd_layer_set_idx);
            if (i > 0)
                READ_BITS(vps->cprms_present_flag[i], 1);
            hrdParameters(&vps->hrd_parameters, br, vps->cprms_present_flag[i],
                vps->vps_max_sub_layers_minus1);
        }
    }

    READ(vps->vps_extension_flag);
    if (vps->vps_extension_flag) {
        while (br.moreRbspData())
            SKIP(1); // vps_extension_data_flag
    }
    br.rbspTrailingBits();
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

    NalReader br(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    READ_BITS(sps->vps_id, 4);
    vps = getVps(sps->vps_id);
    if (!vps)
        return false;
    sps->vps = vps;

    CHECK_READ_BITS(sps->sps_max_sub_layers_minus1, 3, 0, MAXSUBLAYERS - 1);

    READ(sps->sps_temporal_id_nesting_flag);

    if (!profileTierLevel(&sps->profile_tier_level, br,
            sps->sps_max_sub_layers_minus1))
        return false;

    CHECK_READ_UE(sps->sps_id, 0, MAXSPSCOUNT);

    // shall be in the range of 0 to 3, inclusive.
    CHECK_READ_UE(sps->chroma_format_idc, 0, 3);
    if (sps->chroma_format_idc == 3)
        READ(sps->separate_colour_plane_flag);

    // used for parsing slice
    if (sps->separate_colour_plane_flag)
        sps->chroma_array_type = 0;
    else
        sps->chroma_array_type = sps->chroma_format_idc;

    READ_UE(sps->pic_width_in_luma_samples);
    READ_UE(sps->pic_height_in_luma_samples);
    READ(sps->conformance_window_flag);
    if (sps->conformance_window_flag) {
        READ_UE(sps->conf_win_left_offset);
        READ_UE(sps->conf_win_right_offset);
        READ_UE(sps->conf_win_top_offset);
        READ_UE(sps->conf_win_bottom_offset);
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

        sps->croppedLeft = subWidthC[sps->chroma_format_idc] * sps->conf_win_left_offset;
        sps->croppedTop = subHeightC[sps->chroma_format_idc] * sps->conf_win_top_offset;
    }

    CHECK_READ_UE(sps->bit_depth_luma_minus8, 0, 8);
    CHECK_READ_UE(sps->bit_depth_chroma_minus8, 0, 8);

    CHECK_READ_UE(sps->log2_max_pic_order_cnt_lsb_minus4, 0, 12);

    READ(sps->sps_sub_layer_ordering_info_present_flag);

    //parse
    PARSE_SUB_LAYER_ORDERING_INFO(sps, br);

    // set default values
    // When sps_max_dec_pic_buffering_minus1[ i ] is not present for i
    // in the range of0 to sps_max_sub_layers_minus1 - 1, inclusive,
    // due to sps_sub_layer_ordering_info_present_flag being equal to 0,
    // it is inferred to be equal to sps_max_dec_pic_buffering_minus1[
    // sps_max_sub_layers_minus1 ].
    // And also apply for sps_max_num_reorder_pics[i] and
    // sps_max_latency_increase_plus1[i].
    SET_DEF_FOR_ORDERING_INFO(sps);

    READ_UE(sps->log2_min_luma_coding_block_size_minus3);
    READ_UE(sps->log2_diff_max_min_luma_coding_block_size);
    READ_UE(sps->log2_min_transform_block_size_minus2);
    READ_UE(sps->log2_diff_max_min_transform_block_size);
    READ_UE(sps->max_transform_hierarchy_depth_inter);
    READ_UE(sps->max_transform_hierarchy_depth_intra);
    READ_BITS(sps->scaling_list_enabled_flag, 1);
    if (sps->scaling_list_enabled_flag) {
        READ(sps->sps_scaling_list_data_present_flag);
        if (sps->sps_scaling_list_data_present_flag) {
            if (!scalingListData(&sps->scaling_list, br))
                return false;
        }
    }
    READ(sps->amp_enabled_flag);
    READ(sps->sample_adaptive_offset_enabled_flag);
    READ(sps->pcm_enabled_flag);
    if (sps->pcm_enabled_flag) {
        READ_BITS(sps->pcm_sample_bit_depth_luma_minus1, 4);
        READ_BITS(sps->pcm_sample_bit_depth_chroma_minus1, 4);
        READ_UE(sps->log2_min_pcm_luma_coding_block_size_minus3);
        READ_UE(sps->log2_diff_max_min_pcm_luma_coding_block_size);
        READ(sps->pcm_loop_filter_disabled_flag);
    }
    CHECK_READ_UE(sps->num_short_term_ref_pic_sets, 0, MAXSHORTTERMRPSCOUNT);

    for (uint32_t i = 0; i < sps->num_short_term_ref_pic_sets; i++) {
        if (!stRefPicSet(&sps->short_term_ref_pic_set[i], br, i, sps.get()))
            return false;
    }

    READ(sps->long_term_ref_pics_present_flag);
    if (sps->long_term_ref_pics_present_flag) {
        CHECK_READ_UE(sps->num_long_term_ref_pics_sps, 0, 32);
        uint32_t nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
        for (uint32_t i = 0; i < sps->num_long_term_ref_pics_sps; i++) {
            READ_BITS(sps->lt_ref_pic_poc_lsb_sps[i], nbits);
            READ(sps->used_by_curr_pic_lt_sps_flag[i]);
        }
    }

    READ(sps->temporal_mvp_enabled_flag);
    READ(sps->strong_intra_smoothing_enabled_flag);
    READ(sps->vui_parameters_present_flag);
    if (sps->vui_parameters_present_flag) {
        if (!vuiParameters(sps.get(), br))
            return false;
    }

    READ(sps->sps_extension_present_flag);

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

    NalReader br(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    // set default values
    pps->uniform_spacing_flag = 1;
    pps->loop_filter_across_tiles_enabled_flag = 1;

    CHECK_READ_UE(pps->pps_id, 0, MAXPPSCOUNT);
    CHECK_READ_UE(pps->sps_id, 0, MAXSPSCOUNT);

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

    READ(pps->dependent_slice_segments_enabled_flag);
    READ(pps->output_flag_present_flag);
    READ_BITS(pps->num_extra_slice_header_bits, 3);
    READ(pps->sign_data_hiding_enabled_flag);
    READ(pps->cabac_init_present_flag);
    CHECK_READ_UE(pps->num_ref_idx_l0_default_active_minus1, 0, 14);
    CHECK_READ_UE(pps->num_ref_idx_l1_default_active_minus1, 0, 14);
    READ_SE(pps->init_qp_minus26);
    READ(pps->constrained_intra_pred_flag);
    READ(pps->transform_skip_enabled_flag);
    READ(pps->cu_qp_delta_enabled_flag);
    if (pps->cu_qp_delta_enabled_flag)
        READ_UE(pps->diff_cu_qp_delta_depth);
    CHECK_READ_SE(pps->pps_cb_qp_offset, -12, 12);
    CHECK_READ_SE(pps->pps_cr_qp_offset, -12, 12);
    READ(pps->slice_chroma_qp_offsets_present_flag);
    READ(pps->weighted_pred_flag);
    READ(pps->weighted_bipred_flag);
    READ(pps->transquant_bypass_enabled_flag);
    READ(pps->tiles_enabled_flag);
    READ(pps->entropy_coding_sync_enabled_flag);
    if (pps->tiles_enabled_flag) {
        CHECK_READ_UE(pps->num_tile_columns_minus1, 0, pps->picWidthInCtbsY - 1);
        CHECK_READ_UE(pps->num_tile_rows_minus1, 0, pps->picHeightInCtbsY - 1);
        READ(pps->uniform_spacing_flag);
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
                READ_UE(pps->column_width_minus1[i]);
                pps->column_width_minus1[pps->num_tile_columns_minus1] -= (pps->column_width_minus1[i] + 1);
            }

            pps->row_height_minus1[pps->num_tile_rows_minus1] = pps->picHeightInCtbsY - 1;
            for (uint32_t i = 0; i < pps->num_tile_rows_minus1; i++) {
                READ_UE(pps->row_height_minus1[i]);
                pps->row_height_minus1[pps->num_tile_rows_minus1] -= (pps->row_height_minus1[i] + 1);
            }
        }
        READ(pps->loop_filter_across_tiles_enabled_flag);
    }
    READ(pps->pps_loop_filter_across_slices_enabled_flag);
    READ(pps->deblocking_filter_control_present_flag);
    if (pps->deblocking_filter_control_present_flag) {
        READ(pps->deblocking_filter_override_enabled_flag);
        READ(pps->pps_deblocking_filter_disabled_flag);
        if (!pps->pps_deblocking_filter_disabled_flag) {
            CHECK_READ_SE(pps->pps_beta_offset_div2, -6, 6);
            CHECK_READ_SE(pps->pps_tc_offset_div2, -6, 6);
        }
    }
    READ(pps->pps_scaling_list_data_present_flag);
    if (pps->pps_scaling_list_data_present_flag) {
        if (!scalingListData(&pps->scaling_list, br))
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
            for (uint32_t matrixId = 0; matrixId < 6;
                 matrixId += (sizeId == 3) ? 3 : 1) {
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

    READ(pps->lists_modification_present_flag);
    READ_UE(pps->log2_parallel_merge_level_minus2);
    READ(pps->slice_segment_header_extension_present_flag);

    READ(pps->pps_extension_present_flag);
    if (pps->pps_extension_present_flag) {
        READ(pps->pps_range_extension_flag);
        READ(pps->pps_multilayer_extension_flag);
        READ(pps->pps_3d_extension_flag);
        READ_BITS(pps->pps_extension_5bits, 5);
    }

    // 7.3.2.3.2 Picture parameter set range extension syntax
    if (pps->pps_range_extension_flag) {
        if (pps->transform_skip_enabled_flag)
            READ_UE(pps->log2_max_transform_skip_block_size_minus2);
        READ(pps->cross_component_prediction_enabled_flag);
        READ(pps->chroma_qp_offset_list_enabled_flag);
        if (pps->chroma_qp_offset_list_enabled_flag) {
            CHECK_READ_UE(pps->diff_cu_chroma_qp_offset_depth, 0, sps->log2_diff_max_min_luma_coding_block_size);
            CHECK_READ_UE(pps->chroma_qp_offset_list_len_minus1, 0, 5);
            for (uint32_t i = 0; i <= pps->chroma_qp_offset_list_len_minus1; i++) {
                CHECK_READ_SE(pps->cb_qp_offset_list[i], -12, 12);
                CHECK_READ_SE(pps->cr_qp_offset_list[i], -12, 12);
            }
        }

        // 7-4 & 7-6
        int32_t bitDepthY = 8 + sps->bit_depth_luma_minus8;
        int32_t bitDepthc = 8 + sps->bit_depth_chroma_minus8;

        int32_t maxValue = std::max(0, bitDepthY - 10);
        CHECK_READ_UE(pps->log2_sao_offset_scale_luma, 0, maxValue);
        maxValue = std::max(0, bitDepthc - 10);
        CHECK_READ_UE(pps->log2_sao_offset_scale_chroma, 0, maxValue);
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

    if (!nalu || !slice) {
        ERROR("nalu or slice is NULL");
        return false;
    }

    NalReader br(nalu->m_data + NalUnit::NALU_HEAD_SIZE,
        nalu->m_size - NalUnit::NALU_HEAD_SIZE);

    READ(slice->first_slice_segment_in_pic_flag);
    if (nalu->nal_unit_type >= NalUnit::BLA_W_LP && nalu->nal_unit_type <= NalUnit::RSV_IRAP_VCL23)
        READ(slice->no_output_of_prior_pics_flag);

    CHECK_READ_UE(slice->pps_id, 0, MAXPPSCOUNT);

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
            READ(slice->dependent_slice_segment_flag);
        READ_BITS(slice->slice_segment_address, nbits);
    }

    if (!slice->dependent_slice_segment_flag) {
        for (uint32_t i = 0; i < pps->num_extra_slice_header_bits; i++)
            SKIP(1); // slice_reserved_flag

        // 0 for B slice, 1 for P slice and 2 for I slice
        CHECK_READ_UE(slice->slice_type, 0, 2);

        if (pps->output_flag_present_flag)
            READ(slice->pic_output_flag);
        if (sps->separate_colour_plane_flag == 1)
            READ_BITS(slice->colour_plane_id, 2);
        if ((nalu->nal_unit_type != NalUnit::IDR_W_RADL) && (nalu->nal_unit_type != NalUnit::IDR_N_LP)) {
            nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
            READ_BITS(slice->slice_pic_order_cnt_lsb, nbits);
            READ(slice->short_term_ref_pic_set_sps_flag);
            if (!slice->short_term_ref_pic_set_sps_flag) {
                if (!stRefPicSet(&slice->short_term_ref_pic_sets, br,
                        sps->num_short_term_ref_pic_sets, sps.get()))
                    return false;
            }
            else if (sps->num_short_term_ref_pic_sets > 1) {
                nbits = ceil(log2(sps->num_short_term_ref_pic_sets));
                READ_BITS(slice->short_term_ref_pic_set_idx, nbits);
            }
            if (sps->long_term_ref_pics_present_flag) {
                if (sps->num_long_term_ref_pics_sps > 0) {
                    CHECK_READ_UE(slice->num_long_term_sps, 0, sps->num_long_term_ref_pics_sps);
                }
                READ_UE(slice->num_long_term_pics);
                for (uint32_t i = 0;
                     i < slice->num_long_term_sps + slice->num_long_term_pics; i++) {
                    if (i < slice->num_long_term_sps) {
                        if (sps->num_long_term_ref_pics_sps > 1) {
                            nbits = ceil(log2(sps->num_long_term_ref_pics_sps));
                            CHECK_READ_BITS(slice->lt_idx_sps[i], nbits, 0, sps->num_long_term_ref_pics_sps - 1);
                        }
                    } else {
                        nbits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
                        READ_BITS(slice->poc_lsb_lt[i], nbits);
                        READ(slice->used_by_curr_pic_lt_flag[i]);
                    }

                    if (i < slice->num_long_term_sps)
                        UsedByCurrPicLt[i] = sps->used_by_curr_pic_lt_sps_flag[slice->lt_idx_sps[i]];
                    else
                        UsedByCurrPicLt[i] = slice->used_by_curr_pic_lt_flag[i];

                    READ(slice->delta_poc_msb_present_flag[i]);
                    if (slice->delta_poc_msb_present_flag[i])
                        READ_UE(slice->delta_poc_msb_cycle_lt[i]);
                }
            }
            if (sps->temporal_mvp_enabled_flag)
                READ(slice->temporal_mvp_enabled_flag);
        }
        if (sps->sample_adaptive_offset_enabled_flag) {
            READ(slice->sao_luma_flag);
            if (sps->chroma_array_type)
                READ(slice->sao_chroma_flag);
        }
        if (slice->isPSlice() || slice->isBSlice()) {
            READ(slice->num_ref_idx_active_override_flag);
            if (slice->num_ref_idx_active_override_flag) {
                // shall be in the range of 0 to 14
                CHECK_READ_UE(slice->num_ref_idx_l0_active_minus1, 0, 14);
                if (slice->isBSlice()) {
                    // shall be in the range of 0 to 14
                    CHECK_READ_UE(slice->num_ref_idx_l1_active_minus1, 0, 14);
                }
            } else {
                //When the current slice is a P or B slice and num_ref_idx_l0_active_minus1
                //is not present, num_ref_idx_l0_active_minus1 is inferred to be equal to
                //num_ref_idx_l0_default_active_minus1, and num_ref_idx_l1_active_minus1
                //is inferred to be equal to num_ref_idx_l1_default_active_minus1.
                slice->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
                slice->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
            }

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
                if (!refPicListsModification(slice, br, numPicTotalCurr))
                    return false;
            }

            if (slice->isBSlice())
                READ(slice->mvd_l1_zero_flag);
            if (pps->cabac_init_present_flag)
                READ(slice->cabac_init_flag);
            if (slice->temporal_mvp_enabled_flag) {
                if (slice->isBSlice())
                    READ(slice->collocated_from_l0_flag);

                if ((slice->collocated_from_l0_flag
                        && slice->num_ref_idx_l0_active_minus1 > 0)
                    || (!slice->collocated_from_l0_flag
                        && slice->num_ref_idx_l1_active_minus1 > 0)) {
                    READ_UE(slice->collocated_ref_idx);
                }
            }
            if ((pps->weighted_pred_flag && slice->isPSlice()) || (pps->weighted_bipred_flag && slice->isBSlice())) {
                if (!predWeightTable(slice, br))
                    return false;
            }
            CHECK_READ_UE(slice->five_minus_max_num_merge_cand, 0, 4);
        }
        READ_SE(slice->qp_delta);
        if (pps->slice_chroma_qp_offsets_present_flag) {
            CHECK_READ_SE(slice->cb_qp_offset, -12, 12);
            CHECK_READ_SE(slice->cr_qp_offset, -12, 12);
        }
        if (pps->chroma_qp_offset_list_enabled_flag)
            READ(slice->cu_chroma_qp_offset_enabled_flag);
        if (pps->deblocking_filter_override_enabled_flag)
            READ(slice->deblocking_filter_override_flag);
        if (slice->deblocking_filter_override_flag) {
            READ(slice->deblocking_filter_disabled_flag);
            if (!slice->deblocking_filter_disabled_flag) {
                CHECK_READ_SE(slice->beta_offset_div2, -6, 6);
                CHECK_READ_SE(slice->tc_offset_div2, -6, 6);
            }
        }
        else {
            slice->deblocking_filter_disabled_flag = pps->pps_deblocking_filter_disabled_flag;
            if (!slice->deblocking_filter_disabled_flag) {
                slice->beta_offset_div2 = pps->pps_beta_offset_div2;
                slice->tc_offset_div2 = pps->pps_tc_offset_div2;
                CHECK_RANGE_INT32(slice->beta_offset_div2, -6, 6);
                CHECK_RANGE_INT32(slice->tc_offset_div2, -6, 6);
            }
        }

        if (pps->pps_loop_filter_across_slices_enabled_flag
            && (slice->sao_luma_flag
                || slice->sao_chroma_flag
                || !slice->deblocking_filter_disabled_flag)) {
            READ(slice->loop_filter_across_slices_enabled_flag);
        }
    }

    if (pps->tiles_enabled_flag || pps->entropy_coding_sync_enabled_flag) {
        READ_UE(slice->num_entry_point_offsets);
        if (slice->num_entry_point_offsets > 0) {
            // the value of offset_len_minus1 shall be in the range of [0, 31]
            CHECK_READ_UE(slice->offset_len_minus1, 0, 31);
            nbits = slice->offset_len_minus1 + 1;
            slice->entry_point_offset_minus1.reserve(slice->num_entry_point_offsets);
            for (uint32_t i = 0; i < slice->num_entry_point_offsets; i++) {
                uint32_t tmp;
                READ_BITS(tmp, nbits);
                slice->entry_point_offset_minus1.push_back(tmp);
            }
        }
    }
    if (pps->slice_segment_header_extension_present_flag) {
        CHECK_READ_UE(slice->slice_segment_header_extension_length, 0, 256);
        for (uint32_t i = 0; i < slice->slice_segment_header_extension_length; i++)
            SKIP(8); // slice_segment_header_extension_data_byte
    }

    // byte_alignment()
    SKIP(1); // alignment_bit_equal_to_one
    while (br.getPos() & 7)
        SKIP(1);

    slice->headerSize = br.getPos();
    slice->emulationPreventionBytes = br.getEpbCnt();

    return true;
}

}
}
