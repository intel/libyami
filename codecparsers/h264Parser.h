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

/*
 * NOTES:
 *     All the following structs and classes definded base on the spec of H264,
 *     you can see H.264 specification at http://www.itu.int/rec/T-REC-H.264.
 *     And in this header file, there are two categories of code-styles for variables.
 *     Some of them looks like "NalUnit::nal_ref_idc" which separated by underline,
 *     that is to say, these variables defined in H264 spec and assigned by reading bits
 *     from video bits stream directly, on my purpose, it is helpful for you to find where
 *     they are in spec quickly. Some of variables defined use CamelCase because these
 *     variables assigned by other variables.
 */

#ifndef h264parser_h
#define h264parser_h

#include "nalReader.h"
#include "VideoCommonDefs.h"

#include <map>
#include <string.h>

namespace YamiParser {
namespace H264 {

#define MAX_SPS_ID 31
#define MAX_PPS_ID 255
#define MAX_IDR_PIC_ID 65535

//get the coding type of the slice according to Table 7-6
#define IS_P_SLICE(slice_type) ((slice_type) % 5 == 0)
#define IS_B_SLICE(slice_type) ((slice_type) % 5 == 1)
#define IS_I_SLICE(slice_type) ((slice_type) % 5 == 2)
#define IS_SP_SLICE(slice_type) ((slice_type) % 5 == 3)
#define IS_SI_SLICE(slice_type) ((slice_type) % 5 == 4)

enum SliceGroupMapType {
    SLICE_GROUP_INTERLEAVED,
    SLICE_GROUP_DISPERSED_MAPPING,
    SLIEC_GROUP_FOREGROUND_LEFTOVER,
    //3, 4, 5 specify changing slice groups. when num_slice_groups_numus
    //is not equal to 1, slice_group_map_type shall not be equal to 3, 4, or 5
    SLICE_GROUP_CHANGING3,
    SLICE_GROUP_CHANGING4,
    SLICE_GROUP_CHANGING5,
    SLICE_GROUP_ASSIGNMENT
};

enum Profile {
    PROFILE_CAVLC_444_INTRA = 44, //A.2.11
    PROFILE_BASELINE = 66, //A.2.1
    PROFILE_MAIN = 77, //A.2.2
    PROFILE_SCALABLE_BASELINE = 83, //G.10.1.1
    PROFILE_SCALABLE_HIGH = 86, //G.10.1.2
    PROFILE_EXTENDED = 88, //A.2.3
    PROFILE_HIGH = 100, //A.2.4
    PROFILE_HIGH_10 = 110, //A.2.5
    PROFILE_MULTIVIEW_HIGH = 118, //H.10.1.1
    PROFILE_HIGH_422 = 122, //A.2.6
    PROFILE_STEREO_HIGH = 128, //H.10.1.2
    PROFILE_MULTIVIEW_DEPTH_HIGH = 138, //I.10.1.1
    PROFILE_HIGH_444 = 244, //A.2.7
};

//according 8.5.6
static const uint8_t zigzag_scans_4x4[16] = {
    0, 1, 4, 8,
    5, 2, 3, 6,
    9, 12, 13, 10,
    7, 11, 14, 15
};

static const uint8_t zigzag_scans_8x8[64] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

#define transform_coefficients_for_frame_macroblocks(dest, src, len, mode) \
    {                                                                      \
        if ((dest) != (src)) {                                             \
            for (uint32_t l = 0; l < (len); l++)                           \
                (dest)[zigzag_scans_##mode[l]] = (src)[l];                 \
        }                                                                  \
    }

//according to Table 7-1
enum NalUnitType {
    NAL_UNSPECIFIED, //unspecified
    NAL_SLICE_NONIDR, //coded slice of a non-IDR picture
    NAL_SLICE_DPA, //coded slice data partiiton A
    NAL_SLICE_DPB, //coded slice data partition B
    NAL_SLICE_DPC, //coded slice data partition C
    NAL_SLICE_IDR, //coded slice of an IDR picture
    NAL_SEI, //supplemental enhancement information (SEI)
    NAL_SPS, //sequence parameter set
    NAL_PPS, //picture parameter set
    NAL_AU_DELIMITER, //access unit delimiter
    NAL_SEQ_END, //end of sequence
    NAL_STREAM_END, //end of stream
    NAL_FILLER_DATA, //filler data
    NAL_SPS_EXT, //sequence parameter set extension
    NAL_PREFIX_UNIT, //prefix NAL unit
    NAL_SUBSET_SPS, //subset sequence parameter set
    //16 -18 reserved
    NAL_SLICE_AUX = 19, //coded slice of an auxiliary coded picture without partitioning
    NAL_SLICE_EXT, //coded slice extension
    NAL_SLICE_EXT_DEPV //coded slice extension for depth view components
    //22 & 23 reserved, 24 - 31 unspecified
};

struct NaluHeadMvcExt {
    bool non_idr_flag;
    uint8_t priority_id;
    uint16_t view_id;
    uint8_t temporal_id;
    bool anchor_pic_flag;
    bool inter_view_flag;
};

struct NaluHeadSvcExt {
    bool idr_flag;
    uint8_t priority_id;
    bool no_inter_layer_pred_flag;
    uint8_t dependency_id;
    uint8_t quality_id;
    uint8_t temporal_id;
    bool use_ref_base_pic_flag;
    bool discardable_flag;
    bool output_flag;
    uint8_t reserved_three_2bits;
};

class Parser;

class NalUnit {
public:
    //the min size of a valid nal unit
    enum {
        NAL_UNIT_SEQUENCE_SIZE = 4
    };

    /* nal should be a complete nal unit buffer without start code or length bytes */
    bool parseNalUnit(const uint8_t* nal, size_t size);

public:
    const uint8_t* m_data;
    uint32_t m_size;

    uint16_t nal_ref_idc;
    uint16_t nal_unit_type;

    //calc value, used by other syntax structs
    bool m_idrPicFlag;
    uint8_t m_nalUnitHeaderBytes;

    NaluHeadMvcExt m_mvc;
    NaluHeadSvcExt m_svc;

private:
    bool parseSvcExtension(BitReader& br);
    bool parseMvcExtension(BitReader& br);
};

struct HRDParameters {
    uint8_t cpb_cnt_minus1;
    uint8_t bit_rate_scale;
    uint8_t cpb_size_scale;
    uint32_t bit_rate_value_minus1[32];
    uint32_t cpb_size_value_minus1[32];
    bool cbr_flag[32];
    uint8_t initial_cpb_removal_delay_length_minus1;
    uint8_t cpb_removal_delay_length_minus1;
    uint8_t dpb_output_delay_length_minus1;
    uint8_t time_offset_length;
};

struct VUIParameters {
    bool aspect_ratio_info_present_flag;
    uint8_t aspect_ratio_idc;
    uint16_t sar_width;
    uint16_t sar_height;
    bool overscan_info_present_flag;
    bool overscan_appropriate_flag;
    bool video_signal_type_present_flag;
    uint8_t video_format;
    bool video_full_range_flag;
    bool colour_description_present_flag;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
    bool chroma_loc_info_present_flag;
    uint8_t chroma_sample_loc_type_top_field;
    uint8_t chroma_sample_loc_type_bottom_field;
    bool timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    bool fixed_frame_rate_flag;
    bool nal_hrd_parameters_present_flag;
    HRDParameters nal_hrd_parameters;
    bool vcl_hrd_parameters_present_flag;
    HRDParameters vcl_hrd_parameters;
    bool low_delay_hrd_flag;
    bool pic_struct_present_flag;
    bool bitstream_restriction_flag;
    bool motion_vectors_over_pic_boundaries_flag;
    uint32_t max_bytes_per_pic_denom;
    uint32_t max_bits_per_mb_denom;
    uint32_t log2_max_mv_length_horizontal;
    uint32_t log2_max_mv_length_vertical;
    uint32_t max_num_reorder_frames;
    uint32_t max_dec_frame_buffering;
};

struct SPS {
    uint8_t profile_idc;
    bool constraint_set0_flag;
    bool constraint_set1_flag;
    bool constraint_set2_flag;
    bool constraint_set3_flag;
    bool constraint_set4_flag;
    bool constraint_set5_flag;
    uint8_t level_idc;
    uint32_t sps_id; //seq_parameter_set_id
    uint8_t chroma_format_idc;
    bool separate_colour_plane_flag;
    uint8_t bit_depth_luma_minus8;
    uint8_t bit_depth_chroma_minus8;
    bool qpprime_y_zero_transform_bypass_flag;
    bool seq_scaling_matrix_present_flag;
    bool seq_scaling_list_present_flag[12];
    uint8_t scaling_lists_4x4[6][16];
    uint8_t scaling_lists_8x8[6][64];
    uint8_t log2_max_frame_num_minus4;
    uint8_t pic_order_cnt_type;
    uint8_t log2_max_pic_order_cnt_lsb_minus4;
    bool delta_pic_order_always_zero_flag;
    int32_t offset_for_non_ref_pic;
    int32_t offset_for_top_to_bottom_field;
    uint8_t num_ref_frames_in_pic_order_cnt_cycle;
    int32_t offset_for_ref_frame[255];
    uint32_t num_ref_frames;
    bool gaps_in_frame_num_value_allowed_flag;
    uint32_t pic_width_in_mbs_minus1;
    uint32_t pic_height_in_map_units_minus1;
    bool frame_mbs_only_flag;
    bool mb_adaptive_frame_field_flag;
    bool direct_8x8_inference_flag;
    bool frame_cropping_flag;
    uint32_t frame_crop_left_offset;
    uint32_t frame_crop_right_offset;
    uint32_t frame_crop_top_offset;
    uint32_t frame_crop_bottom_offset;
    bool vui_parameters_present_flag;
    VUIParameters m_vui;

    //Because these variables calced from other variables instead of
    //reading from bits stream, so using different style and spec do like this
    //used to calc slice`s maxPicNum
    uint32_t m_maxFrameNum;

    uint8_t m_chromaArrayType;

    int32_t m_width;
    int32_t m_height;
    int32_t m_cropX;
    int32_t m_cropY;
    int32_t m_cropRectWidth;
    int32_t m_cropRectHeight;
};

struct PPS {
    ~PPS();

    uint32_t pps_id;
    uint32_t sps_id;

    SharedPtr<SPS> m_sps;

    bool entropy_coding_mode_flag;
    bool pic_order_present_flag;
    uint32_t num_slice_groups_minus1;
    uint8_t slice_group_map_type;
    uint32_t run_length_minus1[8];
    uint32_t top_left[8];
    uint32_t bottom_right[8];
    bool slice_group_change_direction_flag;
    uint32_t slice_group_change_rate_minus1;
    uint32_t pic_size_in_map_units_minus1;
    uint8_t* slice_group_id;
    uint8_t num_ref_idx_l0_active_minus1;
    uint8_t num_ref_idx_l1_active_minus1;
    bool weighted_pred_flag;
    uint8_t weighted_bipred_idc;
    int8_t pic_init_qp_minus26;
    int8_t pic_init_qs_minus26;
    int8_t chroma_qp_index_offset;
    bool deblocking_filter_control_present_flag;
    bool constrained_intra_pred_flag;
    bool redundant_pic_cnt_present_flag;
    bool transform_8x8_mode_flag;
    bool pic_scaling_list_present_flag[12];
    uint8_t scaling_lists_4x4[6][16];
    uint8_t scaling_lists_8x8[6][64];
    int8_t second_chroma_qp_index_offset;
};

struct RefPicListModification {
    uint8_t modification_of_pic_nums_idc;
    uint32_t abs_diff_pic_num_minus1;
    uint32_t long_term_pic_num;
    uint32_t abs_diff_view_idx_minus1;
};

struct PredWeightTable {
    uint8_t luma_log2_weight_denom;
    uint8_t chroma_log2_weight_denom;
    bool luma_weight_l0_flag;
    //32 is the max of num_ref_idx_l0_active_minus1
    int16_t luma_weight_l0[32];
    int8_t luma_offset_l0[32];
    bool chroma_weight_l0_flag;
    int16_t chroma_weight_l0[32][2];
    int8_t chroma_offset_l0[32][2];
    bool luma_weight_l1_flag;
    int16_t luma_weight_l1[32];
    int8_t luma_offset_l1[32];
    bool chroma_weight_l1_flag;
    int16_t chroma_weight_l1[32][2];
    int8_t chroma_offset_l1[32][2];
};

struct RefPicMarking {
    uint8_t memory_management_control_operation;
    uint32_t difference_of_pic_nums_minus1;
    uint32_t long_term_pic_num;
    uint32_t long_term_frame_idx;
    uint32_t max_long_term_frame_idx_plus1;
};

struct DecRefPicMarking {
    bool no_output_of_prior_pics_flag;
    bool long_term_reference_flag;
    bool adaptive_ref_pic_marking_mode_flag;
    RefPicMarking ref_pic_marking[10];
    uint8_t n_ref_pic_marking;
};

class SliceHeader {
public:
    SliceHeader() { memset(&pred_weight_table, 0, sizeof(pred_weight_table)); }
    bool parseHeader(Parser* nalparser, NalUnit* nalu);

private:
    bool refPicListModification(NalReader& nr,
        RefPicListModification* pm0, RefPicListModification* pm1, bool is_mvc);
    bool predWeightTable(NalReader& nr, uint8_t chroma_array_type);
    bool decRefPicMarking(NalUnit* nalu, NalReader& nr);

public:
    uint32_t first_mb_in_slice;
    uint32_t slice_type;
    SharedPtr<PPS> m_pps;
    uint8_t colour_plane_id;
    uint16_t frame_num;
    bool field_pic_flag;
    bool bottom_field_flag;
    uint32_t idr_pic_id;
    uint16_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt_bottom;
    int32_t delta_pic_order_cnt[2];
    uint8_t redundant_pic_cnt;
    bool direct_spatial_mv_pred_flag;
    bool num_ref_idx_active_override_flag;
    uint8_t num_ref_idx_l0_active_minus1;
    uint8_t num_ref_idx_l1_active_minus1;
    bool ref_pic_list_modification_flag_l0;
    uint8_t n_ref_pic_list_modification_l0;
    RefPicListModification ref_pic_list_modification_l0[32];
    bool ref_pic_list_modification_flag_l1;
    uint8_t n_ref_pic_list_modification_l1;
    RefPicListModification ref_pic_list_modification_l1[32];
    PredWeightTable pred_weight_table;
    DecRefPicMarking dec_ref_pic_marking;
    uint8_t cabac_init_idc;
    int8_t slice_qp_delta;
    bool sp_for_switch_flag;
    int8_t slice_qs_delta;
    uint8_t disable_deblocking_filter_idc;
    int8_t slice_alpha_c0_offset_div2;
    int8_t slice_beta_offset_div2;
    uint16_t slice_group_change_cycle;

    //the allowned max value of abs_diff_pic_num_minus1
    uint32_t m_maxPicNum;

    //the size of the slice header in bits
    uint32_t m_headerSize;

    //the number of emulation prevention bytes
    uint32_t m_emulationPreventionBytes;
};

class Parser {
public:
    enum {
        MAX_CPB_CNT_MINUS1 = 31,
        MAX_CHROMA_FORMAT_IDC = 3,
        SCALING_LIST_DEFAULT_VALUE = 16
    };

    typedef std::map<uint8_t, SharedPtr<SPS> > SpsMap;
    typedef std::map<uint8_t, SharedPtr<PPS> > PpsMap;

    bool parseSps(SharedPtr<SPS>& sps, const NalUnit* nalu);
    bool parsePps(SharedPtr<PPS>& pps, const NalUnit* nalu);

    inline SharedPtr<PPS> searchPps(uint8_t id) const;
    inline SharedPtr<SPS> searchSps(uint8_t id) const;

private:
    bool hrdParameters(HRDParameters* hrd, NalReader& nr);
    bool vuiParameters(SharedPtr<SPS>& sps, NalReader& nr);

    static const uint8_t EXTENDED_SAR;
    SpsMap m_spsMap;
    PpsMap m_ppsMap;
};

}
}

#endif
