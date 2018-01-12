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

#include "h264Parser.h"

#include <math.h>

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

#define READ_UE(f)                            \
    do {                                      \
        if (!br.readUe(f)) {                  \
            ERROR("failed to readUe %s", #f); \
            return false;                     \
        }                                     \
    } while (0)

#define READ_SE(f)                            \
    do {                                      \
        if (!br.readSe(f)) {                  \
            ERROR("failed to readSe %s", #f); \
            return false;                     \
        }                                     \
    } while (0)

namespace YamiParser {
namespace H264 {

//according to Table 7-3
static const uint8_t Default_4x4_Intra[16] = {
    6, 13, 13, 20,
    20, 20, 28, 28,
    28, 28, 32, 32,
    32, 37, 37, 42
};
static const uint8_t Default_4x4_Inter[16] = {
    10, 14, 14, 20,
    20, 20, 24, 24,
    24, 24, 27, 27,
    27, 30, 30, 34
};

//according to Table 7-4
static const uint8_t Default_8x8_Intra[64] = {
    6, 10, 10, 13, 11, 13, 16, 16,
    16, 16, 18, 18, 18, 18, 18, 23,
    23, 23, 23, 23, 23, 25, 25, 25,
    25, 25, 25, 25, 27, 27, 27, 27,
    27, 27, 27, 27, 29, 29, 29, 29,
    29, 29, 29, 31, 31, 31, 31, 31,
    31, 33, 33, 33, 33, 33, 36, 36,
    36, 36, 38, 38, 38, 40, 40, 42
};
static const uint8_t Default_8x8_Inter[64] = {
    9, 13, 13, 15, 13, 15, 17, 17,
    17, 17, 19, 19, 19, 19, 19, 21,
    21, 21, 21, 21, 21, 22, 22, 22,
    22, 22, 22, 22, 24, 24, 24, 24,
    24, 24, 24, 24, 25, 25, 25, 25,
    25, 25, 25, 27, 27, 27, 27, 27,
    27, 28, 28, 28, 28, 28, 30, 30,
    30, 30, 32, 32, 32, 33, 33, 35
};

//definded according to Table 7-2
const uint8_t* Default_Scaling_List[12] = {
    Default_4x4_Intra, Default_4x4_Intra, Default_4x4_Intra,
    Default_4x4_Inter, Default_4x4_Inter, Default_4x4_Inter,
    Default_8x8_Intra, Default_8x8_Inter, Default_8x8_Intra,
    Default_8x8_Inter, Default_8x8_Intra, Default_8x8_Inter
};

//Find the first occurrence of the subsequence position in the string src.
//@src, the pointer to source data.
//@size, the lenght of source data in bytes.
static int32_t searchStartPos(const uint8_t* src, uint32_t size)
{
    //Loop size -2 times at most because the subsequence contains 3 characters.
    for (uint32_t i = 0; i < size - 2; i++)
        if ((src[i] == 0x00) && (src[i + 1] == 0x00) && (src[i + 2] == 0x01))
            return i;
    return -1;
}

//7.3.2.1.1.1 Scaling list syntax
static bool scalingList(NalReader& br, uint8_t* sl, uint32_t size, uint32_t index)
{
    uint8_t lastScale = 8;
    uint8_t nextScale = 8;
    int32_t delta_scale;
    for (uint32_t j = 0; j < size; j++) {
        if (nextScale) {
            READ_SE(delta_scale);
            nextScale = (lastScale + delta_scale + 256) % 256;
        }
        if (!j && !nextScale) {
            memcpy(sl, Default_Scaling_List[index], size);
            break;
        }
        sl[j] = (!nextScale) ? lastScale : nextScale;
        lastScale = sl[j];
    }
    return true;
}

PPS::~PPS()
{
    free(slice_group_id);
}

//according to G.7.3.1.1
bool NalUnit::parseSvcExtension(BitReader& br)
{
    READ(m_svc.idr_flag);
    READ_BITS(m_svc.priority_id, 6);
    READ(m_svc.no_inter_layer_pred_flag);
    READ_BITS(m_svc.dependency_id, 3);
    READ_BITS(m_svc.quality_id, 4);
    READ_BITS(m_svc.temporal_id, 3);
    READ(m_svc.use_ref_base_pic_flag);
    READ(m_svc.discardable_flag);
    READ(m_svc.output_flag);
    READ_BITS(m_svc.reserved_three_2bits, 2);
    return true;
}

//according to H.7.3.1.1
bool NalUnit::parseMvcExtension(BitReader& br)
{
    READ(m_mvc.non_idr_flag);
    READ_BITS(m_mvc.priority_id, 6);
    READ_BITS(m_mvc.view_id, 10);
    READ_BITS(m_mvc.temporal_id, 3);
    READ(m_mvc.anchor_pic_flag);
    READ(m_mvc.inter_view_flag);
    return true;
}

bool NalUnit::parseNalUnit(const uint8_t* src, size_t size)
{
    if (!src || !size)
        return false;

    m_data = src;
    m_size = size;
    BitReader br(m_data, m_size);

    br.skip(1); // forbidden_zero_bit
    READ_BITS(nal_ref_idc, 2);
    READ_BITS(nal_unit_type, 5);
    //7-1
    m_idrPicFlag = (nal_unit_type == 5 ? 1 : 0);
    m_nalUnitHeaderBytes = 1;

    bool svc_extension_flag;
    if (nal_unit_type == NAL_PREFIX_UNIT
        || nal_unit_type == NAL_SLICE_EXT
        || nal_unit_type == NAL_SLICE_EXT_DEPV) {
        READ(svc_extension_flag);
        if (svc_extension_flag) {
            if (!parseSvcExtension(br))
                return false;
            //G.7.4.1.1
            m_idrPicFlag = m_svc.idr_flag;
        } else {
            if (!parseMvcExtension(br))
                return false;
            //H.7.4.1.1
            m_idrPicFlag = !m_mvc.non_idr_flag;
        }
        m_nalUnitHeaderBytes += 3;
    }

    return true;
}

const uint8_t Parser::EXTENDED_SAR = 255;

bool Parser::hrdParameters(HRDParameters* hrd, NalReader& br)
{
    READ_UE(hrd->cpb_cnt_minus1);
    if (hrd->cpb_cnt_minus1 > MAX_CPB_CNT_MINUS1)
        return false;
    READ_BITS(hrd->bit_rate_scale, 4);
    READ_BITS(hrd->cpb_size_scale, 4);

    for (uint32_t SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
        READ_UE(hrd->bit_rate_value_minus1[SchedSelIdx]);
        READ_UE(hrd->cpb_size_value_minus1[SchedSelIdx]);
        READ(hrd->cbr_flag[SchedSelIdx]);
    }

    READ_BITS(hrd->initial_cpb_removal_delay_length_minus1, 5);
    READ_BITS(hrd->cpb_removal_delay_length_minus1, 5);
    READ_BITS(hrd->dpb_output_delay_length_minus1, 5);
    READ_BITS(hrd->time_offset_length, 5);

    return true;
}

bool Parser::vuiParameters(SharedPtr<SPS>& sps, NalReader& br)
{
    VUIParameters* vui = &sps->m_vui;
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
            READ(vui->matrix_coefficients);
        }
    }

    READ(vui->chroma_loc_info_present_flag);
    if (vui->chroma_loc_info_present_flag) {
        READ_UE(vui->chroma_sample_loc_type_top_field);
        //chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field
        //specify the location of chroma samples, and it shall be in the range of 0 to 5
        if (vui->chroma_sample_loc_type_top_field > 5)
            return false;
        READ_UE(vui->chroma_sample_loc_type_bottom_field);
        if (vui->chroma_sample_loc_type_bottom_field > 5)
            return false;
    }

    READ(vui->timing_info_present_flag);
    if (vui->timing_info_present_flag) {
        READ(vui->num_units_in_tick);
        READ(vui->time_scale);
        READ(vui->fixed_frame_rate_flag);
    }

    READ(vui->nal_hrd_parameters_present_flag);
    if (vui->nal_hrd_parameters_present_flag) {
        if (!hrdParameters(&vui->nal_hrd_parameters, br))
            return false;
    }

    READ(vui->vcl_hrd_parameters_present_flag);
    if (vui->vcl_hrd_parameters_present_flag) {
        if (!hrdParameters(&vui->vcl_hrd_parameters, br))
            return false;
    }

    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
        READ(vui->low_delay_hrd_flag);

    READ(vui->pic_struct_present_flag);
    READ(vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        READ(vui->motion_vectors_over_pic_boundaries_flag);
        READ_UE(vui->max_bytes_per_pic_denom);
        READ_UE(vui->max_bits_per_mb_denom);
        //max_bits_per_mb_denom in the range of 0 to 16
        if (vui->max_bits_per_mb_denom > 16)
            return false;
        //both log2_max_mv_length_horizontal and log2_max_mv_length_vertical
        //shall be in the range of 0 to 16
        READ_UE(vui->log2_max_mv_length_horizontal);
        READ_UE(vui->log2_max_mv_length_vertical);
        if (vui->log2_max_mv_length_horizontal > 16
            || vui->log2_max_mv_length_vertical > 16)
            return false;
        READ_UE(vui->max_num_reorder_frames);
        READ_UE(vui->max_dec_frame_buffering);
    }

    return true;
}

bool Parser::parseSps(SharedPtr<SPS>& sps, const NalUnit* nalu)
{
    NalReader br(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    READ(sps->profile_idc);
    READ(sps->constraint_set0_flag);
    READ(sps->constraint_set1_flag);
    READ(sps->constraint_set2_flag);
    READ(sps->constraint_set3_flag);
    READ(sps->constraint_set4_flag);
    READ(sps->constraint_set5_flag);
    br.skip(2); //reserved_zero_2bits
    READ(sps->level_idc);
    READ_UE(sps->sps_id);
    if (sps->sps_id > MAX_SPS_ID)
        return false;

    //when chroma_format_idc is not present,
    //it shall be inferred to be equal to 1 (4:2:0 chroma format)
    sps->chroma_format_idc = 1;
    sps->bit_depth_luma_minus8 = 0;
    sps->bit_depth_chroma_minus8 = 0;

    //when seq_scaling_matrix_present_flag is not present,
    //it shall be inferred to be euqal to 0, and the elements of
    //scaling lists specified by Flat_4x4_16 and Flat_8x8_16 which
    //include the default values euqal to 16
    memset(sps->scaling_lists_4x4, SCALING_LIST_DEFAULT_VALUE,
        sizeof(sps->scaling_lists_4x4));
    memset(sps->scaling_lists_8x8, SCALING_LIST_DEFAULT_VALUE,
        sizeof(sps->scaling_lists_8x8));

    if (sps->profile_idc == PROFILE_HIGH
        || sps->profile_idc == PROFILE_HIGH_10
        || sps->profile_idc == PROFILE_HIGH_422
        || sps->profile_idc == PROFILE_HIGH_444
        || sps->profile_idc == PROFILE_CAVLC_444_INTRA
        || sps->profile_idc == PROFILE_SCALABLE_BASELINE
        || sps->profile_idc == PROFILE_SCALABLE_HIGH
        || sps->profile_idc == PROFILE_MULTIVIEW_HIGH
        || sps->profile_idc == PROFILE_STEREO_HIGH
        || sps->profile_idc == PROFILE_MULTIVIEW_DEPTH_HIGH) {
        READ_UE(sps->chroma_format_idc);
        if (sps->chroma_format_idc > MAX_CHROMA_FORMAT_IDC)
            return false;
        if (sps->chroma_format_idc == MAX_CHROMA_FORMAT_IDC)
            READ(sps->separate_colour_plane_flag);

        //both bit_depth_luma_minus8 and bit_depth_chroma_minus8 shall be
        //in the range of 0 to 6. When they are not present, its shall be inferred
        //to be equal to 0, at the begining of this function
        READ_UE(sps->bit_depth_luma_minus8);
        READ_UE(sps->bit_depth_chroma_minus8);
        if (sps->bit_depth_luma_minus8 > 6
            || sps->bit_depth_chroma_minus8 > 6)
            return false;
        READ(sps->qpprime_y_zero_transform_bypass_flag);

        READ(sps->seq_scaling_matrix_present_flag);

        //according to Table 7-2, Scaling list fall-back rule set A
        const uint8_t* const Fall_Back_Scaling_List[12] = {
            Default_4x4_Intra, sps->scaling_lists_4x4[0], sps->scaling_lists_4x4[1],
            Default_4x4_Inter, sps->scaling_lists_4x4[3], sps->scaling_lists_4x4[4],
            Default_8x8_Intra, Default_8x8_Inter, sps->scaling_lists_8x8[0],
            sps->scaling_lists_8x8[1], sps->scaling_lists_8x8[2], sps->scaling_lists_8x8[3]
        };

        if (sps->seq_scaling_matrix_present_flag) {
            uint32_t loop = (sps->chroma_format_idc != 3) ? 8 : 12;
            for (uint32_t i = 0; i < loop; i++) {
                READ(sps->seq_scaling_list_present_flag[i]);
                if (sps->seq_scaling_list_present_flag[i]) {
                    if (i < 6) {
                        if (!scalingList(br, sps->scaling_lists_4x4[i], 16, i))
                            return false;
                    }
                    else {
                        if (!scalingList(br, sps->scaling_lists_8x8[i - 6], 64, i))
                            return false;
                    }
                } else {
                    if(i < 6)
                        memcpy(sps->scaling_lists_4x4[i], Fall_Back_Scaling_List[i], 16);
                    else
                        memcpy(sps->scaling_lists_8x8[i - 6], Fall_Back_Scaling_List[i], 64);
                }
            }
        }
    }

    //log2_max_frame_num_minus4 specifies the value of the variable MaxFrameNum that
    //is used in frame_num related, and shall be in the range of 0 to 12
    READ_UE(sps->log2_max_frame_num_minus4);
    if (sps->log2_max_frame_num_minus4 > 12)
        return false;
    sps->m_maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    READ_UE(sps->pic_order_cnt_type);
    if (sps->pic_order_cnt_type > 2)
        return false;
    if (!sps->pic_order_cnt_type) {
        //log2_max_pic_order_cnt_lsb_minus4 specifies the value of the variable
        //MaxPicOrderCntLsb that is used in the decoing process for picture order
        //count, and shall be in the range of 0 to 12
        READ_UE(sps->log2_max_pic_order_cnt_lsb_minus4);
        if (sps->log2_max_pic_order_cnt_lsb_minus4 > 12)
            return false;
    } else if (sps->pic_order_cnt_type == 1) {
        READ(sps->delta_pic_order_always_zero_flag);
        READ_SE(sps->offset_for_non_ref_pic);
        READ_SE(sps->offset_for_top_to_bottom_field);
        READ_UE(sps->num_ref_frames_in_pic_order_cnt_cycle);
        if (sps->num_ref_frames_in_pic_order_cnt_cycle > 255)
            return false;
        for (uint32_t i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            READ_SE(sps->offset_for_ref_frame[i]);
    }

    READ_UE(sps->num_ref_frames);
    ASSERT(sps->num_ref_frames <= H264_MAX_REFRENCE_SURFACE_NUMBER);
    READ(sps->gaps_in_frame_num_value_allowed_flag);
    READ_UE(sps->pic_width_in_mbs_minus1);
    READ_UE(sps->pic_height_in_map_units_minus1);
    READ(sps->frame_mbs_only_flag);

    if (!sps->frame_mbs_only_flag)
        READ(sps->mb_adaptive_frame_field_flag);

    READ(sps->direct_8x8_inference_flag);
    READ(sps->frame_cropping_flag);
    if (sps->frame_cropping_flag) {
        READ_UE(sps->frame_crop_left_offset);
        READ_UE(sps->frame_crop_right_offset);
        READ_UE(sps->frame_crop_top_offset);
        READ_UE(sps->frame_crop_bottom_offset);
    }

    READ(sps->vui_parameters_present_flag);
    if (sps->vui_parameters_present_flag) {
        if (!vuiParameters(sps, br))
            return false;
    }

    //according to 7.4.2.1.1
    if (sps->separate_colour_plane_flag)
        sps->m_chromaArrayType = 0;
    else
        sps->m_chromaArrayType = sps->chroma_format_idc;

    //Calculate  width and height
    uint32_t width, height;
    uint32_t picHeightInMapUnits;
    uint32_t picWidthInMbs, frameHeightInMbs;
    picWidthInMbs = sps->pic_width_in_mbs_minus1 + 1; //7-12
    width = picWidthInMbs << 4;
    //7-15
    picHeightInMapUnits = sps->pic_height_in_map_units_minus1 + 1;
    //7-17
    frameHeightInMbs = (2 - sps->frame_mbs_only_flag) * picHeightInMapUnits;
    height = frameHeightInMbs << 4;

    sps->m_width = width;
    sps->m_height = height;

    if (sps->frame_cropping_flag) {
        //according Table 6-1, used to calc sps->width
        //when frame_cropping_flag is equal to 1
        uint32_t subWidthC[] = { 1, 2, 2, 1, 1 };
        uint32_t subHeightC[] = { 1, 2, 1, 1, 1 };
        uint32_t cropUnitX, cropUnitY;
        if (!sps->m_chromaArrayType) {
            cropUnitX = 1;
            cropUnitY = 2 - sps->frame_mbs_only_flag;
        } else {
            cropUnitX = subWidthC[sps->chroma_format_idc];
            cropUnitY = subHeightC[sps->chroma_format_idc] * (2 - sps->frame_mbs_only_flag);
        }

        width -= cropUnitX * (sps->frame_crop_left_offset + sps->frame_crop_right_offset);
        height -= cropUnitY * (sps->frame_crop_top_offset + sps->frame_crop_bottom_offset);

        sps->m_cropRectWidth = width;
        sps->m_cropRectHeight = height;
        sps->m_cropX = cropUnitX * sps->frame_crop_left_offset;
        sps->m_cropY = cropUnitY * sps->frame_crop_top_offset;
    }

    m_spsMap[sps->sps_id] = sps;

    return true;
}

bool Parser::parsePps(SharedPtr<PPS>& pps, const NalUnit* nalu)
{
    SharedPtr<SPS> sps;
    bool pic_scaling_matrix_present_flag;
    int32_t qp_bd_offset;

    NalReader br(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    READ_UE(pps->pps_id);
    READ_UE(pps->sps_id);
    if (pps->pps_id > MAX_PPS_ID || pps->sps_id > MAX_SPS_ID)
        return false;

    sps = searchSps(pps->sps_id);
    if (!sps)
        return false;

    pps->m_sps = sps;
    qp_bd_offset = 6 * (sps->bit_depth_luma_minus8 + sps->separate_colour_plane_flag);

    //the default value of  pic_scaling_matrix_present_flag is zero,
    //when pic_scaling_matrix_present_flag is equal to 0, pps scaling
    //lists shall be inferred to be equal to those specified by the sps
    memcpy(&pps->scaling_lists_4x4, &sps->scaling_lists_4x4,
        sizeof(sps->scaling_lists_4x4));
    memcpy(&pps->scaling_lists_8x8, &sps->scaling_lists_8x8,
        sizeof(sps->scaling_lists_8x8));

    READ(pps->entropy_coding_mode_flag);
    READ(pps->pic_order_present_flag);
    READ_UE(pps->num_slice_groups_minus1);
    //num_slice_groups_minus1 in the range of 0 to 7, according to A.2.1
    if (pps->num_slice_groups_minus1 > 7)
        return false;
    if (pps->num_slice_groups_minus1 > 0) {
        READ_UE(pps->slice_group_map_type);
        if (pps->slice_group_map_type > SLICE_GROUP_ASSIGNMENT)
            return false;
        if (!pps->slice_group_map_type) {
            for (uint32_t iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++)
                READ_UE(pps->run_length_minus1[iGroup]);
        } else if (pps->slice_group_map_type == SLIEC_GROUP_FOREGROUND_LEFTOVER) {
            for (uint32_t iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++) {
                READ_UE(pps->top_left[iGroup]);
                READ_UE(pps->bottom_right[iGroup]);
            }
        } else if (pps->slice_group_map_type == SLICE_GROUP_CHANGING3
                   || pps->slice_group_map_type == SLICE_GROUP_CHANGING4
                   || pps->slice_group_map_type == SLICE_GROUP_CHANGING5) {
            READ(pps->slice_group_change_direction_flag);
            READ_UE(pps->slice_group_change_rate_minus1);
        } else if (pps->slice_group_map_type == SLICE_GROUP_ASSIGNMENT) {
            READ_UE(pps->pic_size_in_map_units_minus1);
            int32_t bits = ceil(log2(pps->num_slice_groups_minus1 + 1));
            pps->slice_group_id =
                (uint8_t*)malloc(sizeof(uint8_t)*(pps->pic_size_in_map_units_minus1 + 1));
            if (!pps->slice_group_id)
                return false;
            for (uint32_t i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
                READ_BITS(pps->slice_group_id[i], bits);
        }
    }

    READ_UE(pps->num_ref_idx_l0_active_minus1);
    READ_UE(pps->num_ref_idx_l1_active_minus1);
    if (pps->num_ref_idx_l0_active_minus1 > 31
        || pps->num_ref_idx_l1_active_minus1 > 31)
        goto error;
    READ(pps->weighted_pred_flag);
    READ_BITS(pps->weighted_bipred_idc, 2);
    READ_SE(pps->pic_init_qp_minus26);
    if (pps->pic_init_qp_minus26 < -(26 + qp_bd_offset)
        || pps->pic_init_qp_minus26 > 25)
        goto error;
    READ_SE(pps->pic_init_qs_minus26);
    if (pps->pic_init_qs_minus26 < -26
        || pps->pic_init_qs_minus26 > 25)
        goto error;
    READ_SE(pps->chroma_qp_index_offset);
    if (pps->chroma_qp_index_offset < -12
        || pps->chroma_qp_index_offset > 12)
        goto error;
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
    READ(pps->deblocking_filter_control_present_flag);
    READ(pps->constrained_intra_pred_flag);
    READ(pps->redundant_pic_cnt_present_flag);

    if (br.moreRbspData()) {
        READ(pps->transform_8x8_mode_flag);
        READ(pic_scaling_matrix_present_flag);

        //according to Table 7-2, Scaling list fall-back rule set A
        const uint8_t* const Fall_Back_A_Scaling_List[12] = {
            Default_4x4_Intra, pps->scaling_lists_4x4[0], pps->scaling_lists_4x4[1],
            Default_4x4_Inter, pps->scaling_lists_4x4[3], pps->scaling_lists_4x4[4],
            Default_8x8_Intra, Default_8x8_Inter, pps->scaling_lists_8x8[0],
            pps->scaling_lists_8x8[1], pps->scaling_lists_8x8[2], pps->scaling_lists_8x8[3]
        };
        //according to Table 7-2, Scaling list fall-back rule set B
        const uint8_t* const Fall_Back_B_Scaling_List[12] = {
            sps->scaling_lists_4x4[0], pps->scaling_lists_4x4[0], pps->scaling_lists_4x4[1],
            sps->scaling_lists_4x4[3], pps->scaling_lists_4x4[3], pps->scaling_lists_4x4[4],
            sps->scaling_lists_8x8[0], sps->scaling_lists_8x8[1], pps->scaling_lists_8x8[0],
            pps->scaling_lists_8x8[1], pps->scaling_lists_8x8[2], pps->scaling_lists_8x8[3]
        };
        if (pic_scaling_matrix_present_flag) {
            uint32_t loop = 6
                + ((sps->chroma_format_idc != 3) ? 2 : 6) * pps->transform_8x8_mode_flag;
            for (uint32_t i = 0; i < loop; i++) {
                READ(pps->pic_scaling_list_present_flag[i]);
                if (pps->pic_scaling_list_present_flag[i]) {
                    if (i < 6) {
                        if (!scalingList(br, pps->scaling_lists_4x4[i], 16, i))
                            return false;
                    }
                    else {
                        if (!scalingList(br, pps->scaling_lists_8x8[i - 6], 64, i))
                            return false;
                    }
                } else if (!sps->seq_scaling_matrix_present_flag) {
                    //fall-back rule set A be used
                    if (i < 6)
                        memcpy(pps->scaling_lists_4x4[i], Fall_Back_A_Scaling_List[i], 16);
                    else
                        memcpy(pps->scaling_lists_8x8[i - 6], Fall_Back_A_Scaling_List[i], 64);
                } else {
                    //fall-back rule set B be used
                    if (i < 6)
                        memcpy(pps->scaling_lists_4x4[i], Fall_Back_B_Scaling_List[i], 16);
                    else
                        memcpy(pps->scaling_lists_8x8[i - 6], Fall_Back_B_Scaling_List[i], 64);
                }
            }
        }
        READ_SE(pps->second_chroma_qp_index_offset);
        if (pps->second_chroma_qp_index_offset < -12
            || pps->second_chroma_qp_index_offset > 12)
            goto error;
    }

    m_ppsMap[pps->pps_id] = pps;

    return true;
error:
    free(pps->slice_group_id);
    return false;
}

inline SharedPtr<PPS>
Parser::searchPps(uint8_t id) const
{
    SharedPtr<PPS> res;
    PpsMap::const_iterator it = m_ppsMap.find(id);
    if (it != m_ppsMap.end())
        res = it->second;
    return res;
}

inline SharedPtr<SPS>
Parser::searchSps(uint8_t id) const
{
    SharedPtr<SPS> res;
    SpsMap::const_iterator it = m_spsMap.find(id);
    if (it != m_spsMap.end())
        res = it->second;
    return res;
}

bool SliceHeader::refPicListModification(NalReader& br, RefPicListModification* pm0,
    RefPicListModification* pm1, bool is_mvc)
{
    uint32_t i = 0;
    if (!IS_I_SLICE(slice_type) && !IS_SI_SLICE(slice_type)) {
        READ(ref_pic_list_modification_flag_l0);
        if (ref_pic_list_modification_flag_l0) {
            do {
                READ_UE(pm0[i].modification_of_pic_nums_idc);
                if (pm0[i].modification_of_pic_nums_idc == 0
                    || pm0[i].modification_of_pic_nums_idc == 1) {
                    READ_UE(pm0[i].abs_diff_pic_num_minus1);
                    if (pm0[i].abs_diff_pic_num_minus1 > m_maxPicNum - 1)
                        return false;
                } else if (pm0[i].modification_of_pic_nums_idc == 2) {
                    READ_UE(pm0[i].long_term_pic_num);
                } else if (is_mvc && (pm0[i].modification_of_pic_nums_idc == 4
                    || pm0[i].modification_of_pic_nums_idc == 5)) {
                    READ_UE(pm0[i].abs_diff_view_idx_minus1);
                }
            } while (pm0[i++].modification_of_pic_nums_idc != 3);
            n_ref_pic_list_modification_l0 = i;
        }
    }

    if (IS_B_SLICE(slice_type)) {
        i = 0;
        READ(ref_pic_list_modification_flag_l1);
        if (ref_pic_list_modification_flag_l1) {
            do {
                READ_UE(pm1[i].modification_of_pic_nums_idc);
                if (pm1[i].modification_of_pic_nums_idc == 0
                    || pm1[i].modification_of_pic_nums_idc == 1) {
                    READ_UE(pm1[i].abs_diff_pic_num_minus1);
                    if (pm1[i].abs_diff_pic_num_minus1 > m_maxPicNum - 1)
                        return false;
                } else if (pm1[i].modification_of_pic_nums_idc == 2) {
                    READ_UE(pm1[i].long_term_pic_num);
                } else if (is_mvc && (pm1[i].modification_of_pic_nums_idc == 4
                    || pm1[i].modification_of_pic_nums_idc == 5)) {
                    READ_UE(pm1[i].abs_diff_view_idx_minus1);
                }
            } while (pm1[i++].modification_of_pic_nums_idc != 3);
        }
        n_ref_pic_list_modification_l1 = i;
    }

    return true;
}

bool SliceHeader::decRefPicMarking(NalUnit* nalu, NalReader& br)
{
    if (nalu->m_idrPicFlag) {
        READ(dec_ref_pic_marking.no_output_of_prior_pics_flag);
        READ(dec_ref_pic_marking.long_term_reference_flag);
    } else {
        READ(dec_ref_pic_marking.adaptive_ref_pic_marking_mode_flag);
        if (dec_ref_pic_marking.adaptive_ref_pic_marking_mode_flag) {
            uint32_t i = 0;
            RefPicMarking* const subpm = dec_ref_pic_marking.ref_pic_marking;
            do {
                READ_UE(subpm[i].memory_management_control_operation);
                if (subpm[i].memory_management_control_operation == 1
                    || subpm[i].memory_management_control_operation == 3)
                    READ_UE(subpm[i].difference_of_pic_nums_minus1);
                if (subpm[i].memory_management_control_operation == 2)
                    READ_UE(subpm[i].long_term_pic_num);
                if (subpm[i].memory_management_control_operation == 3
                    || subpm[i].memory_management_control_operation == 6)
                    READ_UE(subpm[i].long_term_frame_idx);
                if (subpm[i].memory_management_control_operation == 4)
                    READ_UE(subpm[i].max_long_term_frame_idx_plus1);
            } while (subpm[i++].memory_management_control_operation != 0);
            dec_ref_pic_marking.n_ref_pic_marking = --i;
        }
    }
    return true;
}

bool SliceHeader::predWeightTable(NalReader& br, uint8_t chromaArrayType)
{
    PredWeightTable& preTab = pred_weight_table;
    READ_UE(preTab.luma_log2_weight_denom);
    if (preTab.luma_log2_weight_denom > 7)
        return false;
    if (chromaArrayType)
        READ_UE(preTab.chroma_log2_weight_denom);
    for (uint32_t i = 0; i <= num_ref_idx_l0_active_minus1; i++) {
        READ(preTab.luma_weight_l0_flag);
        if (preTab.luma_weight_l0_flag) {
            //7.4.3.2
            READ_SE(preTab.luma_weight_l0[i]);
            READ_SE(preTab.luma_offset_l0[i]);
            if (preTab.luma_weight_l0[i] < -128
                || preTab.luma_weight_l0[i] > 127
                || preTab.luma_offset_l0[i] < -128
                || preTab.luma_offset_l0[i] > 127)
                return false;
        } else {
            preTab.luma_weight_l0[i] = 1 << preTab.luma_log2_weight_denom;
        }
        if (chromaArrayType) {
            READ(preTab.chroma_weight_l0_flag);
            for (uint32_t j = 0; j < 2; j++) {
                if (preTab.chroma_weight_l0_flag) {
                    READ_SE(preTab.chroma_weight_l0[i][j]);
                    READ_SE(preTab.chroma_offset_l0[i][j]);
                } else {
                    preTab.chroma_weight_l0[i][j] = 1 << preTab.chroma_log2_weight_denom;
                }
            }
        }
    }
    if (IS_B_SLICE(slice_type))
        for (uint32_t i = 0; i <= num_ref_idx_l1_active_minus1; i++) {
            READ(preTab.luma_weight_l1_flag);
            if (preTab.luma_weight_l1_flag) {
                READ_SE(preTab.luma_weight_l1[i]);
                READ_SE(preTab.luma_offset_l1[i]);
            } else {
                preTab.luma_weight_l1[i] = 1 << preTab.luma_log2_weight_denom;
            }
            if (chromaArrayType) {
                READ(preTab.chroma_weight_l1_flag);
                for (uint32_t j = 0; j < 2; j++) {
                    if (preTab.chroma_weight_l1_flag) {
                        READ_SE(preTab.chroma_weight_l1[i][j]);
                        READ_SE(preTab.chroma_offset_l1[i][j]);
                    } else {
                        preTab.chroma_weight_l1[i][j] = 1 << preTab.chroma_log2_weight_denom;
                    }
                }
            }
        }
    return true;
}

bool SliceHeader::parseHeader(Parser* nalparser, NalUnit* nalu)
{
    uint32_t pps_id;
    SharedPtr<SPS> sps;

    if (!nalu->m_size)
        return false;

    NalReader br(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    READ_UE(first_mb_in_slice);
    READ_UE(slice_type);
    READ_UE(pps_id);
    if (pps_id > MAX_PPS_ID)
        return false;

    m_pps = nalparser->searchPps(pps_id);
    if (!m_pps)
        return false;

    sps = m_pps->m_sps;

    //set default values for fields that might not be present in the bitstream
    //and have valid defaults
    num_ref_idx_l0_active_minus1 = m_pps->num_ref_idx_l0_active_minus1;
    num_ref_idx_l1_active_minus1 = m_pps->num_ref_idx_l1_active_minus1;

    if (sps->separate_colour_plane_flag == 1)
        READ_BITS(colour_plane_id, 2);

    READ_BITS(frame_num, sps->log2_max_frame_num_minus4 + 4);

    if (!sps->frame_mbs_only_flag) {
        READ(field_pic_flag);
        if (field_pic_flag)
            READ(bottom_field_flag);
    }

    //calculate MaxPicNum
    if (field_pic_flag)
        m_maxPicNum = 2 * sps->m_maxFrameNum;
    else
        m_maxPicNum = sps->m_maxFrameNum;

    if (nalu->m_idrPicFlag) {
        READ_UE(idr_pic_id);
        if (idr_pic_id > MAX_IDR_PIC_ID)
            return false;
    }

    if (!sps->pic_order_cnt_type) {
        READ_BITS(pic_order_cnt_lsb, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (m_pps->pic_order_present_flag && !field_pic_flag)
            READ_SE(delta_pic_order_cnt_bottom);
    }

    if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        READ_SE(delta_pic_order_cnt[0]);
        if (m_pps->pic_order_present_flag && !field_pic_flag)
            READ_SE(delta_pic_order_cnt[1]);
    }

    if (m_pps->redundant_pic_cnt_present_flag) {
        READ_UE(redundant_pic_cnt);
        if (redundant_pic_cnt > 127)
            return false;
    }

    if (IS_B_SLICE(slice_type))
        READ(direct_spatial_mv_pred_flag);

    if (IS_P_SLICE(slice_type) || IS_SP_SLICE(slice_type) || IS_B_SLICE(slice_type)) {
        READ(num_ref_idx_active_override_flag);
        if (num_ref_idx_active_override_flag) {
            READ_UE(num_ref_idx_l0_active_minus1);
            if (num_ref_idx_l0_active_minus1 > 31)
                return false;
            if (IS_B_SLICE(slice_type)) {
                READ_UE(num_ref_idx_l1_active_minus1);
                if (num_ref_idx_l1_active_minus1 > 31)
                    return false;
            }
        }
    }

    if (nalu->nal_unit_type == NAL_SLICE_EXT
        || nalu->nal_unit_type == NAL_SLICE_EXT_DEPV)
        refPicListModification(br, ref_pic_list_modification_l0,
            ref_pic_list_modification_l1, true);
    else
        refPicListModification(br, ref_pic_list_modification_l0,
            ref_pic_list_modification_l1, false);

    if ((m_pps->weighted_pred_flag && (IS_P_SLICE(slice_type) || IS_SP_SLICE(slice_type)))
        || (m_pps->weighted_bipred_idc == 1 && IS_B_SLICE(slice_type))) {
        if (!predWeightTable(br, sps->m_chromaArrayType))
            return false;
    }

    if (nalu->nal_ref_idc) {
        if (!decRefPicMarking(nalu, br))
            return false;
    }

    if (m_pps->entropy_coding_mode_flag
        && !IS_I_SLICE(slice_type) && !IS_SI_SLICE(slice_type)) {
        READ_UE(cabac_init_idc);
        if (cabac_init_idc > 2)
            return false;
    }

    READ_SE(slice_qp_delta);
    //according to 7-29, the value of slice_qp_delta shall be limited
    //so that sliceQPY is in the range of -QpBdOffsetY to +51.
    //sliceQPY = 26 + pic_init_qp_minus26 + slice_qp_delta
    //QpBdOffsetY = 6 * bit_depth_luma_minus8.
    int32_t QpBdOffsetY = -6 * sps->bit_depth_luma_minus8;
    int32_t sliceQPY = 26 + m_pps->pic_init_qp_minus26 + slice_qp_delta;
    if (sliceQPY < QpBdOffsetY || sliceQPY > 51)
        return false;

    if (IS_SP_SLICE(slice_type) || IS_SI_SLICE(slice_type)) {
        if (IS_SP_SLICE(slice_type))
            READ(sp_for_switch_flag);
        READ_SE(slice_qs_delta);
    }

    if (m_pps->deblocking_filter_control_present_flag) {
        READ_UE(disable_deblocking_filter_idc);
        if (disable_deblocking_filter_idc > 2)
            return false;
        if (disable_deblocking_filter_idc != 1) {
            READ_SE(slice_alpha_c0_offset_div2);
            READ_SE(slice_beta_offset_div2);
        }
    }

    // The value of slice_group_map_type in the range [0, 6], when it is equal to
    //3, 4 and 5 specify changing slice groups
    if (m_pps->num_slice_groups_minus1 > 0
        && m_pps->slice_group_map_type >= 3
        && m_pps->slice_group_map_type <= 5) {
        uint32_t picWidthInMbs = sps->pic_width_in_mbs_minus1 + 1;
        uint32_t picHeightInMapUnits = sps->pic_height_in_map_units_minus1 + 1;
        uint32_t picSizeInMapUnits = picWidthInMbs * picHeightInMapUnits;
        uint32_t sliceGroupChangeRate = m_pps->slice_group_change_rate_minus1 + 1;
        const uint32_t n = ceil(log2(picSizeInMapUnits / sliceGroupChangeRate + 1));
        READ_BITS(slice_group_change_cycle, n);
    }

    m_headerSize = br.getPos();
    m_emulationPreventionBytes = br.getEpbCnt();

    return true;
}

}
}
