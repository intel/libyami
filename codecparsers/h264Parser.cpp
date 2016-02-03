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

#include "h264Parser.h"

#include <math.h>

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
static void scalingList(NalReader& nr, uint8_t* sl, uint32_t size, uint32_t index)
{
    uint8_t lastScale = 8;
    uint8_t nextScale = 8;
    int32_t delta_scale;
    for (uint32_t j = 0; j < size; j++) {
        if (nextScale) {
            delta_scale = nr.readSe();
            nextScale = (lastScale + delta_scale + 256) % 256;
        }
        if (!j && !nextScale) {
            memcpy(sl, Default_Scaling_List[index], size);
            break;
        }
        sl[j] = (!nextScale) ? lastScale : nextScale;
        lastScale = sl[j];
    }
}

PPS::~PPS()
{
    free(slice_group_id);
}

//according to G.7.3.1.1
void NalUnit::parseSvcExtension(BitReader& br)
{
    m_svc.idr_flag = br.read(1);
    m_svc.priority_id = br.read(6);
    m_svc.no_inter_layer_pred_flag = br.read(1);
    m_svc.dependency_id = br.read(3);
    m_svc.quality_id = br.read(4);
    m_svc.temporal_id = br.read(3);
    m_svc.use_ref_base_pic_flag = br.read(1);
    m_svc.discardable_flag = br.read(1);
    m_svc.output_flag = br.read(1);
    m_svc.reserved_three_2bits = br.read(2);
}

//according to H.7.3.1.1
void NalUnit::parseMvcExtension(BitReader& br)
{
    m_mvc.non_idr_flag = br.read(1);
    m_mvc.priority_id = br.read(6);
    m_mvc.view_id = br.read(10);
    m_mvc.temporal_id = br.read(3);
    m_mvc.anchor_pic_flag = br.read(1);
    m_mvc.inter_view_flag = br.read(1);
}

bool NalUnit::parseNalUnit(const uint8_t* src, size_t size)
{
    //It is a invalid source data when the size less than 4 bytes
    //because each nal uint start with the subsequence of 0x000001.
    if (size < NAL_UNIT_SEQUENCE_SIZE)
        return false;

    int32_t pos;
    pos = searchStartPos(src, size);
    if (pos == -1) //no nal unit
        return false;

    m_data = src + pos + 3;
    m_size = size - pos - 3;
    BitReader br(m_data, m_size);

    br.skip(1); // forbidden_zero_bit
    nal_ref_idc = br.read(2);
    nal_unit_type = br.read(5);
    //7-1
    m_idrPicFlag = (nal_unit_type == 5 ? 1 : 0);
    m_nalUnitHeaderBytes = 1;

    uint8_t svc_extension_flag;
    if (nal_unit_type == NAL_PREFIX_UNIT
        || nal_unit_type == NAL_SLICE_EXT
        || nal_unit_type == NAL_SLICE_EXT_DEPV) {
        svc_extension_flag = br.read(1);
        if (svc_extension_flag) {
            parseSvcExtension(br);
            //G.7.4.1.1
            m_idrPicFlag = m_svc.idr_flag;
        } else {
            parseMvcExtension(br);
            //H.7.4.1.1
            m_idrPicFlag = !m_mvc.non_idr_flag;
        }
        m_nalUnitHeaderBytes += 3;
    }

    return true;
}

const uint8_t Parser::EXTENDED_SAR = 255;

bool Parser::hrdParameters(HRDParameters* hrd, NalReader& nr)
{
    hrd->cpb_cnt_minus1 = nr.readUe();
    if (hrd->cpb_cnt_minus1 > MAX_CPB_CNT_MINUS1)
        return false;
    hrd->bit_rate_scale = nr.read(4);
    hrd->cpb_size_scale = nr.read(4);

    for (uint32_t SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
        hrd->bit_rate_value_minus1[SchedSelIdx] = nr.readUe();
        hrd->cpb_size_value_minus1[SchedSelIdx] = nr.readUe();
        hrd->cbr_flag[SchedSelIdx] = nr.read(1);
    }

    hrd->initial_cpb_removal_delay_length_minus1 = nr.read(5);
    hrd->cpb_removal_delay_length_minus1 = nr.read(5);
    hrd->dpb_output_delay_length_minus1 = nr.read(5);
    hrd->time_offset_length = nr.read(5);

    return true;
}

bool Parser::vuiParameters(SharedPtr<SPS>& sps, NalReader& nr)
{
    VUIParameters* vui = &sps->m_vui;
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
            vui->matrix_coefficients = nr.read(8);
        }
    }

    vui->chroma_loc_info_present_flag = nr.read(1);
    if (vui->chroma_loc_info_present_flag) {
        vui->chroma_sample_loc_type_top_field = nr.readUe();
        //chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field
        //specify the location of chroma samples, and it shall be in the range of 0 to 5
        if (vui->chroma_sample_loc_type_top_field > 5)
            return false;
        vui->chroma_sample_loc_type_bottom_field = nr.readUe();
        if (vui->chroma_sample_loc_type_bottom_field > 5)
            return false;
    }

    vui->timing_info_present_flag = nr.read(1);
    if (vui->timing_info_present_flag) {
        vui->num_units_in_tick = nr.read(32);
        vui->time_scale = nr.read(32);
        vui->fixed_frame_rate_flag = nr.read(1);
    }

    vui->nal_hrd_parameters_present_flag = nr.read(1);
    if (vui->nal_hrd_parameters_present_flag) {
        if (!hrdParameters(&vui->nal_hrd_parameters, nr))
            return false;
    }

    vui->vcl_hrd_parameters_present_flag = nr.read(1);
    if (vui->vcl_hrd_parameters_present_flag) {
        if (!hrdParameters(&vui->vcl_hrd_parameters, nr))
            return false;
    }

    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
        vui->low_delay_hrd_flag = nr.read(1);

    vui->pic_struct_present_flag = nr.read(1);
    vui->bitstream_restriction_flag = nr.read(1);
    if (vui->bitstream_restriction_flag) {
        vui->motion_vectors_over_pic_boundaries_flag = nr.read(1);
        vui->max_bytes_per_pic_denom = nr.readUe();
        vui->max_bits_per_mb_denom = nr.readUe();
        //max_bits_per_mb_denom in the range of 0 to 16
        if (vui->max_bits_per_mb_denom > 16)
            return false;
        //both log2_max_mv_length_horizontal and log2_max_mv_length_vertical
        //shall be in the range of 0 to 16
        vui->log2_max_mv_length_horizontal = nr.readUe();
        vui->log2_max_mv_length_vertical = nr.readUe();
        if (vui->log2_max_mv_length_horizontal > 16
            || vui->log2_max_mv_length_vertical > 16)
            return false;
        vui->max_num_reorder_frames = nr.readUe();
        vui->max_dec_frame_buffering = nr.readUe();
    }

    return true;
}

bool Parser::parseSps(SharedPtr<SPS>& sps, const NalUnit* nalu)
{
    NalReader nr(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    sps->profile_idc = nr.read(8);
    sps->constraint_set0_flag = nr.read(1);
    sps->constraint_set1_flag = nr.read(1);
    sps->constraint_set2_flag = nr.read(1);
    sps->constraint_set3_flag = nr.read(1);
    sps->constraint_set4_flag = nr.read(1);
    sps->constraint_set5_flag = nr.read(1);
    nr.skip(2); //reserved_zero_2bits
    sps->level_idc = nr.read(8);
    sps->sps_id = nr.readUe();
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
        sps->chroma_format_idc = nr.readUe();
        if (sps->chroma_format_idc > MAX_CHROMA_FORMAT_IDC)
            return false;
        if (sps->chroma_format_idc == MAX_CHROMA_FORMAT_IDC)
            sps->separate_colour_plane_flag = nr.read(1);

        //both bit_depth_luma_minus8 and bit_depth_chroma_minus8 shall be
        //in the range of 0 to 6. When they are not present, its shall be inferred
        //to be equal to 0, at the begining of this function
        sps->bit_depth_luma_minus8 = nr.readUe();
        sps->bit_depth_chroma_minus8 = nr.readUe();
        if (sps->bit_depth_luma_minus8 > 6
            || sps->bit_depth_chroma_minus8 > 6)
            return false;
        sps->qpprime_y_zero_transform_bypass_flag = nr.read(1);

        sps->seq_scaling_matrix_present_flag = nr.read(1);

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
                sps->seq_scaling_list_present_flag[i] = nr.read(1);
                if (sps->seq_scaling_list_present_flag[i]) {
                    if (i < 6)
                        scalingList(nr, sps->scaling_lists_4x4[i], 16, i);
                    else
                        scalingList(nr, sps->scaling_lists_8x8[i - 6], 64, i);
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
    sps->log2_max_frame_num_minus4 = nr.readUe();
    if (sps->log2_max_frame_num_minus4 > 12)
        return false;
    sps->m_maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    sps->pic_order_cnt_type = nr.readUe();
    if (sps->pic_order_cnt_type > 2)
        return false;
    if (!sps->pic_order_cnt_type) {
        //log2_max_pic_order_cnt_lsb_minus4 specifies the value of the variable
        //MaxPicOrderCntLsb that is used in the decoing process for picture order
        //count, and shall be in the range of 0 to 12
        sps->log2_max_pic_order_cnt_lsb_minus4 = nr.readUe();
        if (sps->log2_max_pic_order_cnt_lsb_minus4 > 12)
            return false;
    } else if (sps->pic_order_cnt_type == 1) {
        sps->delta_pic_order_always_zero_flag = nr.read(1);
        sps->offset_for_non_ref_pic = nr.readSe();
        sps->offset_for_top_to_bottom_field = nr.readSe();
        sps->num_ref_frames_in_pic_order_cnt_cycle = nr.readUe();
        if (sps->num_ref_frames_in_pic_order_cnt_cycle > 255)
            return false;
        for (uint32_t i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            sps->offset_for_ref_frame[i] = nr.readSe();
    }

    sps->num_ref_frames = nr.readUe();
    sps->gaps_in_frame_num_value_allowed_flag = nr.read(1);
    sps->pic_width_in_mbs_minus1 = nr.readUe();
    sps->pic_height_in_map_units_minus1 = nr.readUe();
    sps->frame_mbs_only_flag = nr.read(1);

    if (!sps->frame_mbs_only_flag)
        sps->mb_adaptive_frame_field_flag = nr.read(1);

    sps->direct_8x8_inference_flag = nr.read(1);
    sps->frame_cropping_flag = nr.read(1);
    if (sps->frame_cropping_flag) {
        sps->frame_crop_left_offset = nr.readUe();
        sps->frame_crop_right_offset = nr.readUe();
        sps->frame_crop_top_offset = nr.readUe();
        sps->frame_crop_bottom_offset = nr.readUe();
    }

    sps->vui_parameters_present_flag = nr.read(1);
    if (sps->vui_parameters_present_flag) {
        if (!vuiParameters(sps, nr))
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
    }

    m_spsMap[sps->sps_id] = sps;

    return true;
}

bool Parser::parsePps(SharedPtr<PPS>& pps, const NalUnit* nalu)
{
    SharedPtr<SPS> sps;
    uint8_t pic_scaling_matrix_present_flag;
    int32_t qp_bd_offset;

    NalReader nr(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    pps->pps_id = nr.readUe();
    pps->sps_id = nr.readUe();
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

    pps->entropy_coding_mode_flag = nr.read(1);
    pps->pic_order_present_flag = nr.read(1);
    pps->num_slice_groups_minus1 = nr.readUe();
    //num_slice_groups_minus1 in the range of 0 to 7, according to A.2.1
    if (pps->num_slice_groups_minus1 > 7)
        return false;
    if (pps->num_slice_groups_minus1 > 0) {
        pps->slice_group_map_type = nr.readUe();
        if (pps->slice_group_map_type > SLICE_GROUP_ASSIGNMENT)
            return false;
        if (!pps->slice_group_map_type) {
            for (uint32_t iGroup = 0; iGroup <= pps->num_slice_groups_minus1; iGroup++)
                pps->run_length_minus1[iGroup] = nr.readUe();
        } else if (pps->slice_group_map_type == SLIEC_GROUP_FOREGROUND_LEFTOVER) {
            for (uint32_t iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++) {
                pps->top_left[iGroup] = nr.readUe();
                pps->bottom_right[iGroup] = nr.readUe();
            }
        } else if (pps->slice_group_map_type == SLICE_GROUP_CHANGING3
                   || pps->slice_group_map_type == SLICE_GROUP_CHANGING4
                   || pps->slice_group_map_type == SLICE_GROUP_CHANGING5) {
            pps->slice_group_change_direction_flag = nr.read(1);
            pps->slice_group_change_rate_minus1 = nr.readUe();
        } else if (pps->slice_group_map_type == SLICE_GROUP_ASSIGNMENT) {
            pps->pic_size_in_map_units_minus1 = nr.readUe();
            int32_t bits = ceil(log2(pps->num_slice_groups_minus1 + 1));
            pps->slice_group_id =
                (uint8_t*)malloc(sizeof(uint8_t)*(pps->pic_size_in_map_units_minus1 + 1));
            if (!pps->slice_group_id)
                return false;
            for (uint32_t i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
                pps->slice_group_id[i] = nr.read(bits);
        }
    }

    pps->num_ref_idx_l0_active_minus1 = nr.readUe();
    pps->num_ref_idx_l1_active_minus1 = nr.readUe();
    if (pps->num_ref_idx_l0_active_minus1 > 31
        || pps->num_ref_idx_l1_active_minus1 > 31)
        goto error;
    pps->weighted_pred_flag = nr.read(1);
    pps->weighted_bipred_idc = nr.read(2);
    pps->pic_init_qp_minus26 = nr.readSe();
    if (pps->pic_init_qp_minus26 < -(26 + qp_bd_offset)
        || pps->pic_init_qp_minus26 > 25)
        goto error;
    pps->pic_init_qs_minus26 = nr.readSe();
    if (pps->pic_init_qs_minus26 < -26
        || pps->pic_init_qs_minus26 > 25)
        goto error;
    pps->chroma_qp_index_offset = nr.readSe();
    if (pps->chroma_qp_index_offset < -12
        || pps->chroma_qp_index_offset > 12)
        goto error;
    pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
    pps->deblocking_filter_control_present_flag = nr.read(1);
    pps->constrained_intra_pred_flag = nr.read(1);
    pps->redundant_pic_cnt_present_flag = nr.read(1);

    if (nr.moreRbspData()) {
        pps->transform_8x8_mode_flag = nr.read(1);
        pic_scaling_matrix_present_flag = nr.read(1);

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
                pps->pic_scaling_list_present_flag[i] = nr.read(1);
                if (pps->pic_scaling_list_present_flag[i]) {
                    if (i < 6)
                        scalingList(nr, pps->scaling_lists_4x4[i], 16, i);
                    else
                        scalingList(nr, pps->scaling_lists_8x8[i - 6], 64, i);
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
        pps->second_chroma_qp_index_offset = nr.readSe();
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

bool SliceHeader::refPicListModification(NalReader& nr, RefPicListModification* pm0,
    RefPicListModification* pm1, bool is_mvc)
{
    uint32_t i = 0;
    if (!IS_I_SLICE(slice_type) && !IS_SI_SLICE(slice_type)) {
        ref_pic_list_modification_flag_l0 = nr.read(1);
        if (ref_pic_list_modification_flag_l0) {
            do {
                pm0[i].modification_of_pic_nums_idc = nr.readUe();
                if (pm0[i].modification_of_pic_nums_idc == 0
                    || pm0[i].modification_of_pic_nums_idc == 1) {
                    pm0[i].abs_diff_pic_num_minus1 = nr.readUe();
                    if (pm0[i].abs_diff_pic_num_minus1 > m_maxPicNum - 1)
                        return false;
                } else if (pm0[i].modification_of_pic_nums_idc == 2) {
                    pm0[i].long_term_pic_num = nr.readUe();
                } else if (is_mvc && (pm0[i].modification_of_pic_nums_idc == 4
                    || pm0[i].modification_of_pic_nums_idc == 5)) {
                    pm0[i].abs_diff_view_idx_minus1 = nr.readUe();
                }
            } while (pm0[i++].modification_of_pic_nums_idc != 3);
            n_ref_pic_list_modification_l0 = i;
        }
    }

    if (IS_B_SLICE(slice_type)) {
        i = 0;
        ref_pic_list_modification_flag_l1 = nr.read(1);
        if (ref_pic_list_modification_flag_l1) {
            do {
                pm1[i].modification_of_pic_nums_idc = nr.readUe();
                if (pm1[i].modification_of_pic_nums_idc == 0
                    || pm1[i].modification_of_pic_nums_idc == 1) {
                    pm1[i].abs_diff_pic_num_minus1 = nr.readUe();
                    if (pm1[i].abs_diff_pic_num_minus1 > m_maxPicNum - 1)
                        return false;
                } else if (pm1[i].modification_of_pic_nums_idc == 2) {
                    pm1[i].long_term_pic_num = nr.readUe();
                } else if (is_mvc && (pm1[i].modification_of_pic_nums_idc == 4
                    || pm1[i].modification_of_pic_nums_idc == 5)) {
                    pm1[i].abs_diff_view_idx_minus1 = nr.readUe();
                }
            } while (pm1[i++].modification_of_pic_nums_idc != 3);
        }
        n_ref_pic_list_modification_l1 = i;
    }

    return true;
}

bool SliceHeader::decRefPicMarking(NalUnit* nalu, NalReader& nr)
{
    if (nalu->m_idrPicFlag) {
        dec_ref_pic_marking.no_output_of_prior_pics_flag = nr.read(1);
        dec_ref_pic_marking.long_term_reference_flag = nr.read(1);
    } else {
        dec_ref_pic_marking.adaptive_ref_pic_marking_mode_flag = nr.read(1);
        if (dec_ref_pic_marking.adaptive_ref_pic_marking_mode_flag) {
            uint32_t i = 0;
            RefPicMarking* const subpm = dec_ref_pic_marking.ref_pic_marking;
            do {
                subpm[i].memory_management_control_operation = nr.readUe();
                if (subpm[i].memory_management_control_operation == 1
                    || subpm[i].memory_management_control_operation == 3)
                    subpm[i].difference_of_pic_nums_minus1 = nr.readUe();
                if (subpm[i].memory_management_control_operation == 2)
                    subpm[i].long_term_pic_num = nr.readUe();
                if (subpm[i].memory_management_control_operation == 3
                    || subpm[i].memory_management_control_operation == 6)
                    subpm[i].long_term_frame_idx = nr.readUe();
                if (subpm[i].memory_management_control_operation == 4)
                    subpm[i].max_long_term_frame_idx_plus1 = nr.readUe();
            } while (subpm[i++].memory_management_control_operation != 0);
            dec_ref_pic_marking.n_ref_pic_marking = --i;
        }
    }
    return true;
}

bool SliceHeader::predWeightTable(NalReader& nr, uint8_t chromaArrayType)
{
    PredWeightTable& preTab = pred_weight_table;
    preTab.luma_log2_weight_denom = nr.readUe();
    if (preTab.luma_log2_weight_denom > 7)
        return false;
    if (chromaArrayType)
        preTab.chroma_log2_weight_denom = nr.readUe();
    for (uint32_t i = 0; i <= num_ref_idx_l0_active_minus1; i++) {
        preTab.luma_weight_l0_flag = nr.read(1);
        if (preTab.luma_weight_l0_flag) {
            //7.4.3.2
            preTab.luma_weight_l0[i] = nr.readSe();
            preTab.luma_offset_l0[i] = nr.readSe();
            if (preTab.luma_weight_l0[i] < -128
                || preTab.luma_weight_l0[i] > 127
                || preTab.luma_offset_l0[i] < -128
                || preTab.luma_offset_l0[i] > 127)
                return false;
        } else {
            preTab.luma_weight_l0[i] = 1 << preTab.luma_log2_weight_denom;
        }
        if (chromaArrayType) {
            preTab.chroma_weight_l0_flag = nr.read(1);
            for (uint32_t j = 0; j < 2; j++) {
                if (preTab.chroma_weight_l0_flag) {
                    preTab.chroma_weight_l0[i][j] = nr.readSe();
                    preTab.chroma_offset_l0[i][j] = nr.readSe();
                } else {
                    preTab.chroma_weight_l0[i][j] = 1 << preTab.chroma_log2_weight_denom;
                }
            }
        }
    }
    if (IS_B_SLICE(slice_type))
        for (uint32_t i = 0; i <= num_ref_idx_l1_active_minus1; i++) {
            preTab.luma_weight_l1_flag = nr.read(1);
            if (preTab.luma_weight_l1_flag) {
                preTab.luma_weight_l1[i] = nr.readSe();
                preTab.luma_offset_l1[i] = nr.readSe();
            } else {
                preTab.luma_weight_l1[i] = 1 << preTab.luma_log2_weight_denom;
            }
            if (chromaArrayType) {
                preTab.chroma_weight_l1_flag = nr.read(1);
                for (uint32_t j = 0; j < 2; j++) {
                    if (preTab.chroma_weight_l1_flag) {
                        preTab.chroma_weight_l1[i][j] = nr.readSe();
                        preTab.chroma_offset_l1[i][j] = nr.readSe();
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
    int32_t pps_id;
    SharedPtr<SPS> sps;

    if (!nalu->m_size)
        return false;

    NalReader nr(nalu->m_data + nalu->m_nalUnitHeaderBytes,
        nalu->m_size - nalu->m_nalUnitHeaderBytes);

    first_mb_in_slice = nr.readUe();
    slice_type = nr.readUe();
    pps_id = nr.readUe();
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
        colour_plane_id = nr.read(2);

    frame_num = nr.read(sps->log2_max_frame_num_minus4 + 4);

    if (!sps->frame_mbs_only_flag) {
        field_pic_flag = nr.read(1);
        if (field_pic_flag)
            bottom_field_flag = nr.read(1);
    }

    //calculate MaxPicNum
    if (field_pic_flag)
        m_maxPicNum = 2 * sps->m_maxFrameNum;
    else
        m_maxPicNum = sps->m_maxFrameNum;

    if (nalu->m_idrPicFlag) {
        idr_pic_id = nr.readUe();
        if (idr_pic_id > MAX_IDR_PIC_ID)
            return false;
    }

    if (!sps->pic_order_cnt_type) {
        pic_order_cnt_lsb = nr.read(sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (m_pps->pic_order_present_flag && !field_pic_flag)
            delta_pic_order_cnt_bottom = nr.readSe();
    }

    if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
        delta_pic_order_cnt[0] = nr.readSe();
        if (m_pps->pic_order_present_flag && !field_pic_flag)
            delta_pic_order_cnt[1] = nr.readSe();
    }

    if (m_pps->redundant_pic_cnt_present_flag) {
        redundant_pic_cnt = nr.readUe();
        if (redundant_pic_cnt > 127)
            return false;
    }

    if (IS_B_SLICE(slice_type))
        direct_spatial_mv_pred_flag = nr.read(1);

    if (IS_P_SLICE(slice_type) || IS_SP_SLICE(slice_type) || IS_B_SLICE(slice_type)) {
        num_ref_idx_active_override_flag = nr.read(1);
        if (num_ref_idx_active_override_flag) {
            num_ref_idx_l0_active_minus1 = nr.readUe();
            if (num_ref_idx_l0_active_minus1 > 31)
                return false;
            if (IS_B_SLICE(slice_type)) {
                num_ref_idx_l1_active_minus1 = nr.readUe();
                if (num_ref_idx_l1_active_minus1 > 31)
                    return false;
            }
        }
    }

    if (nalu->nal_unit_type == NAL_SLICE_EXT
        || nalu->nal_unit_type == NAL_SLICE_EXT_DEPV)
        refPicListModification(nr, ref_pic_list_modification_l0,
            ref_pic_list_modification_l1, true);
    else
        refPicListModification(nr, ref_pic_list_modification_l0,
            ref_pic_list_modification_l1, false);

    if ((m_pps->weighted_pred_flag && (IS_P_SLICE(slice_type) || IS_SP_SLICE(slice_type)))
        || (m_pps->weighted_bipred_idc == 1 && IS_B_SLICE(slice_type))) {
        if (!predWeightTable(nr, sps->m_chromaArrayType))
            return false;
    }

    if (nalu->nal_ref_idc) {
        if (!decRefPicMarking(nalu, nr))
            return false;
    }

    if (m_pps->entropy_coding_mode_flag
        && !IS_I_SLICE(slice_type) && !IS_SI_SLICE(slice_type)) {
        cabac_init_idc = nr.readUe();
        if (cabac_init_idc > 2)
            return false;
    }

    slice_qp_delta = nr.readSe();
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
            sp_for_switch_flag = nr.read(1);
        slice_qs_delta = nr.readSe();
    }

    if (m_pps->deblocking_filter_control_present_flag) {
        disable_deblocking_filter_idc = nr.readUe();
        if (disable_deblocking_filter_idc > 2)
            return false;
        if (disable_deblocking_filter_idc != 1) {
            slice_alpha_c0_offset_div2 = nr.readSe();
            slice_beta_offset_div2 = nr.readSe();
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
        slice_group_change_cycle = nr.read(n);
    }

    m_headerSize = nr.getPos();
    m_emulationPreventionBytes = nr.getEpbCnt();

    return true;
}

}
}
