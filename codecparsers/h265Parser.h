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
 * The data structures defined below follow the ITU Recommendation
 * H.265 (http://www.itu.int/rec/T-REC-H.265). The names are used
 * exactly as in H.265, so struct fields which hold syntax elements are
 * spelled in lowercase_with_underscores. Struct fields which hold variables
 * are spelled in CamelCase.
 */

#ifndef h265Parser_h
#define h265Parser_h

#include "nalReader.h"
#include "VideoCommonDefs.h"

#include <map>
#include <vector>

namespace YamiParser {
namespace H265 {

    const uint8_t MAXSUBLAYERS = 7;
    const uint8_t MAXSPSCOUNT = 15;
    const uint8_t MAXPPSCOUNT = 63;
    const uint8_t MAXSHORTTERMRPSCOUNT = 64;

    //A.4
    const uint8_t MAXDPBSIZE = 16;

    const uint8_t UpperRightDiagonal4x4[16] = {
        0, 4, 1, 8,
        5, 2, 12, 9,
        6, 3, 13, 10,
        7, 14, 11, 15
    };

    const uint8_t UpperRightDiagonal8x8[64] = {
        0, 8, 1, 16, 9, 2, 24, 17,
        10, 3, 32, 25, 18, 11, 4, 40,
        33, 26, 19, 12, 5, 48, 41, 34,
        27, 20, 13, 6, 56, 49, 42, 35,
        28, 21, 14, 7, 57, 50, 43, 36,
        29, 22, 15, 58, 51, 44, 37, 30,
        23, 59, 52, 45, 38, 31, 60, 53,
        46, 39, 61, 54, 47, 62, 55, 63
    };

#define UpperRightDiagonal16x16 UpperRightDiagonal8x8
#define UpperRightDiagonal32x32 UpperRightDiagonal8x8

    //7.3.3
    struct ProfileTierLevel {
        uint8_t general_profile_space;
        bool general_tier_flag;
        uint8_t general_profile_idc;
        bool general_profile_compatibility_flag[32];
        bool general_progressive_source_flag;
        bool general_interlaced_source_flag;
        bool general_non_packed_constraint_flag;
        bool general_frame_only_constraint_flag;
        bool general_max_12bit_constraint_flag;
        bool general_max_10bit_constraint_flag;
        bool general_max_8bit_constraint_flag;
        bool general_max_422chroma_constraint_flag;
        bool general_max_420chroma_constraint_flag;
        bool general_max_monochrome_constraint_flag;
        bool general_intra_constraint_flag;
        bool general_one_picture_only_constraint_flag;
        bool general_lower_bit_rate_constraint_flag;
        //uint64_t general_reserved_zero_34bits;
        //uint64_t general_reserved_zero_43bits;
        bool general_inbld_flag;
        //bool general_reserved_zero_bit;
        uint8_t general_level_idc;
        bool sub_layer_profile_present_flag[6];
        bool sub_layer_level_present_flag[6];
        //uint8_t reserved_zero_2bits[6];
        uint8_t sub_layer_profile_space[6];
        bool sub_layer_tier_flag[6];
        uint8_t sub_layer_profile_idc[6];
        bool sub_layer_profile_compatibility_flag[6][32];
        bool sub_layer_progressive_source_flag[6];
        bool sub_layer_interlaced_source_flag[6];
        bool sub_layer_non_packed_constraint_flag[6];
        bool sub_layer_frame_only_constraint_flag[6];
        bool sub_layer_max_12bit_constraint_flag[6];
        bool sub_layer_max_10bit_constraint_flag[6];
        bool sub_layer_max_8bit_constraint_flag[6];
        bool sub_layer_max_422chroma_constraint_flag[6];
        bool sub_layer_max_420chroma_constraint_flag[6];
        bool sub_layer_max_monochrome_constraint_flag[6];
        bool sub_layer_intra_constraint_flag[6];
        bool sub_layer_one_picture_only_constraint_flag[6];
        bool sub_layer_lower_bit_rate_constraint_flag[6];
        //uint64_t sub_layer_reserved_zero_34bits[6];
        //uint64_t sub_layer_reserved_zero_43bits[6];
        bool sub_layer_inbld_flag[6];
        //bool sub_layer_reserved_zero_bit[6];
        uint8_t sub_layer_level_idc[6];
    };

    struct SubLayerHRDParameters {
        uint32_t bit_rate_value_minus1[32];
        uint32_t cpb_size_value_minus1[32];
        uint32_t cpb_size_du_value_minus1[32];
        uint32_t bit_rate_du_value_minus1[32];
        bool cbr_flag[32];
    };

    struct HRDParameters {
        bool nal_hrd_parameters_present_flag;
        bool vcl_hrd_parameters_present_flag;
        bool sub_pic_hrd_params_present_flag;
        uint8_t tick_divisor_minus2;
        uint8_t du_cpb_removal_delay_increment_length_minus1;
        bool sub_pic_cpb_params_in_pic_timing_sei_flag;
        uint8_t dpb_output_delay_du_length_minus1;
        uint8_t bit_rate_scale;
        uint8_t cpb_size_scale;
        uint8_t cpb_size_du_scale;
        uint8_t initial_cpb_removal_delay_length_minus1;
        uint8_t au_cpb_removal_delay_length_minus1;
        uint8_t dpb_output_delay_length_minus1;
        bool fixed_pic_rate_general_flag[MAXSUBLAYERS];
        bool fixed_pic_rate_within_cvs_flag[MAXSUBLAYERS];
        uint16_t elemental_duration_in_tc_minus1[MAXSUBLAYERS]; //[0, 2047]
        bool low_delay_hrd_flag[MAXSUBLAYERS];
        uint8_t cpb_cnt_minus1[MAXSUBLAYERS]; //[0, 31]

        SubLayerHRDParameters sublayer_hrd_params[MAXSUBLAYERS];
    };

    struct ShortTermRefPicSet {
        bool inter_ref_pic_set_prediction_flag;
        uint8_t delta_idx_minus1;
        bool delta_rps_sign;
        uint16_t abs_delta_rps_minus1;
        bool used_by_curr_pic_flag[16];
        bool use_delta_flag[16];
        uint8_t num_negative_pics;
        uint8_t num_positive_pics;
        uint32_t delta_poc_s0_minus1[16];
        bool used_by_curr_pic_s0_flag[16];
        uint32_t delta_poc_s1_minus1[16];
        bool used_by_curr_pic_s1_flag[16];
        uint8_t NumDeltaPocs;
        uint8_t NumNegativePics;
        uint8_t NumPositivePics;
        uint8_t UsedByCurrPicS0[16];
        uint8_t UsedByCurrPicS1[16];
        int32_t DeltaPocS0[16];
        int32_t DeltaPocS1[16];
    };

    struct VuiParameters {
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
        uint8_t matrix_coeffs;
        bool chroma_loc_info_present_flag;
        uint8_t chroma_sample_loc_type_top_field; //[0,5]
        uint8_t chroma_sample_loc_type_bottom_field; //[0,5]
        bool neutral_chroma_indication_flag;
        bool field_seq_flag;
        bool frame_field_info_present_flag;
        bool default_display_window_flag;
        uint32_t def_disp_win_left_offset;
        uint32_t def_disp_win_right_offset;
        uint32_t def_disp_win_top_offset;
        uint32_t def_disp_win_bottom_offset;
        bool vui_timing_info_present_flag;
        uint32_t vui_num_units_in_tick;
        uint32_t vui_time_scale;
        bool vui_poc_proportional_to_timing_flag;
        uint32_t vui_num_ticks_poc_diff_one_minus1;
        bool vui_hrd_parameters_present_flag;

        HRDParameters hrd_params;

        bool bitstream_restriction_flag;
        bool tiles_fixed_structure_flag;
        bool motion_vectors_over_pic_boundaries_flag;
        bool restricted_ref_pic_lists_flag;
        uint16_t min_spatial_segmentation_idc; //[0, 4095]
        uint8_t max_bytes_per_pic_denom; //[0, 16]
        uint8_t max_bits_per_min_cu_denom; //[0, 16]
        uint8_t log2_max_mv_length_horizontal; //[0, 16]
        uint8_t log2_max_mv_length_vertical; //[0, 15]
    };

    struct ScalingList {
        uint8_t scalingList4x4[6][16];
        uint8_t scalingList8x8[6][64];
        uint8_t scalingList16x16[6][64];
        // According to spec "7.3.4 Scaling list data syntax",
        // we just use scalingList32x32[0] and scalingList32x32[3];
        // as spec "7.4.5 Scaling list data semantics",
        // matrixId as the index of scalingList32x32 can be equal to 3;
        // so we'd better define scalingList32x32[6][] other than scalingList32x32[2][].
        uint8_t scalingList32x32[6][64];
        uint8_t scalingListDC16x16[6];
        // scalingListDC32x32 is similar to scalingList32x32.
        uint8_t scalingListDC32x32[6];
    };

    struct RefPicListModification {
        bool ref_pic_list_modification_flag_l0;
        uint32_t list_entry_l0[15];
        bool ref_pic_list_modification_flag_l1;
        uint32_t list_entry_l1[15];
    };

    struct PredWeightTable {
        uint8_t luma_log2_weight_denom;
        int8_t delta_chroma_log2_weight_denom;
        bool luma_weight_l0_flag[15];
        bool chroma_weight_l0_flag[15];
        int8_t delta_luma_weight_l0[15];
        int8_t luma_offset_l0[15];
        int8_t delta_chroma_weight_l0[15][2];
        int16_t delta_chroma_offset_l0[15][2];
        bool luma_weight_l1_flag[15];
        bool chroma_weight_l1_flag[15];
        int8_t delta_luma_weight_l1[15];
        int8_t luma_offset_l1[15];
        int8_t delta_chroma_weight_l1[15][2];
        int16_t delta_chroma_offset_l1[15][2];
    };

    //video parameter set
    struct VPS {
        VPS();
        ~VPS();

        uint8_t vps_id; //vps_video_parameter_set_id
        bool vps_base_layer_internal_flag;
        bool vps_base_layer_available_flag;
        uint8_t vps_max_layers_minus1;
        uint8_t vps_max_sub_layers_minus1; //[0, 6]
        bool vps_temporal_id_nesting_flag;
        //uint16_t vps_reserved_0xffff_16bits;
        ProfileTierLevel profile_tier_level;
        bool vps_sub_layer_ordering_info_present_flag;
        uint8_t vps_max_dec_pic_buffering_minus1[MAXSUBLAYERS];
        uint8_t vps_max_num_reorder_pics[MAXSUBLAYERS];
        uint32_t vps_max_latency_increase_plus1[MAXSUBLAYERS];
        uint8_t vps_max_layer_id;
        uint16_t vps_num_layer_sets_minus1; //[0, 1023]
        //bool layer_id_included_flag[][];
        bool vps_timing_info_present_flag;
        uint32_t vps_num_units_in_tick;
        uint32_t vps_time_scale;
        bool vps_poc_proportional_to_timing_flag;
        uint32_t vps_num_ticks_poc_diff_one_minus1;
        uint16_t vps_num_hrd_parameters;
        HRDParameters hrd_parameters;
        bool vps_extension_flag;

        //all non pod type should start here
        std::vector<uint16_t> hrd_layer_set_idx;
        std::vector<uint8_t> cprms_present_flag;
    };

    //Sequence parameter set
    struct SPS {
        SPS();

        uint8_t vps_id; //sps_video_parameter_set_id
        uint8_t sps_max_sub_layers_minus1;
        bool sps_temporal_id_nesting_flag;
        ProfileTierLevel profile_tier_level;
        uint8_t sps_id; //sps_seq_parameter_set_id, [0, 15]
        uint8_t chroma_format_idc; //[0, 3]
        bool separate_colour_plane_flag;
        uint8_t chroma_array_type;
        uint16_t pic_width_in_luma_samples;
        uint16_t pic_height_in_luma_samples;
        bool conformance_window_flag;
        uint32_t conf_win_left_offset;
        uint32_t conf_win_right_offset;
        uint32_t conf_win_top_offset;
        uint32_t conf_win_bottom_offset;
        int32_t width;
        int32_t height;
        //cropped frame
        uint32_t croppedLeft;
        uint32_t croppedTop;
        uint32_t croppedWidth;
        uint32_t croppedHeight;
        uint8_t bit_depth_luma_minus8; //[0, 8]
        uint8_t bit_depth_chroma_minus8; //[0, 8]
        uint8_t log2_max_pic_order_cnt_lsb_minus4; //[0,12]
        bool sps_sub_layer_ordering_info_present_flag;
        uint8_t sps_max_dec_pic_buffering_minus1[MAXSUBLAYERS];
        uint8_t sps_max_num_reorder_pics[MAXSUBLAYERS];
        uint8_t sps_max_latency_increase_plus1[MAXSUBLAYERS];
        uint8_t log2_min_luma_coding_block_size_minus3;
        uint8_t log2_diff_max_min_luma_coding_block_size;
        uint8_t log2_min_transform_block_size_minus2;
        uint8_t log2_diff_max_min_transform_block_size;
        uint8_t max_transform_hierarchy_depth_inter;
        uint8_t max_transform_hierarchy_depth_intra;
        uint8_t scaling_list_enabled_flag;
        bool sps_scaling_list_data_present_flag;

        ScalingList scaling_list;

        bool amp_enabled_flag;
        bool sample_adaptive_offset_enabled_flag;
        bool pcm_enabled_flag;
        uint8_t pcm_sample_bit_depth_luma_minus1;
        uint8_t pcm_sample_bit_depth_chroma_minus1;
        uint8_t log2_min_pcm_luma_coding_block_size_minus3;
        uint8_t log2_diff_max_min_pcm_luma_coding_block_size;
        bool pcm_loop_filter_disabled_flag;
        uint8_t num_short_term_ref_pic_sets; //[0,64]

        ShortTermRefPicSet short_term_ref_pic_set[64];

        bool long_term_ref_pics_present_flag;
        uint8_t num_long_term_ref_pics_sps; //[0,32]
        uint16_t lt_ref_pic_poc_lsb_sps[32];
        bool used_by_curr_pic_lt_sps_flag[32];
        bool temporal_mvp_enabled_flag;
        bool strong_intra_smoothing_enabled_flag;
        bool vui_parameters_present_flag;

        VuiParameters vui_params;

        bool sps_extension_present_flag;

        //all non pod type should start here
        SharedPtr<VPS> vps;
    };

    //Picture parameter set
    struct PPS {
        PPS();

        uint8_t pps_id; //pps_pic_parameter_set_id, [0, 63]
        uint8_t sps_id; //pps_seq_parameter_set_id, [0, 15]
        bool dependent_slice_segments_enabled_flag;
        bool output_flag_present_flag;
        uint8_t num_extra_slice_header_bits;
        bool sign_data_hiding_enabled_flag;
        bool cabac_init_present_flag;
        uint8_t num_ref_idx_l0_default_active_minus1; //0, 14]
        uint8_t num_ref_idx_l1_default_active_minus1; //0, 14]
        int8_t init_qp_minus26;
        bool constrained_intra_pred_flag;
        bool transform_skip_enabled_flag;
        bool cu_qp_delta_enabled_flag;
        uint8_t diff_cu_qp_delta_depth;
        int8_t pps_cb_qp_offset; //[-12, 12]
        int8_t pps_cr_qp_offset; //[-12, 12]
        bool slice_chroma_qp_offsets_present_flag;
        bool weighted_pred_flag;
        bool weighted_bipred_flag;
        bool transquant_bypass_enabled_flag;
        bool tiles_enabled_flag;
        bool entropy_coding_sync_enabled_flag;
        uint8_t num_tile_columns_minus1;
        uint8_t num_tile_rows_minus1;
        bool uniform_spacing_flag;
        uint32_t column_width_minus1[19];
        uint32_t row_height_minus1[21];
        bool loop_filter_across_tiles_enabled_flag;
        bool pps_loop_filter_across_slices_enabled_flag;
        bool deblocking_filter_control_present_flag;
        bool deblocking_filter_override_enabled_flag;
        bool pps_deblocking_filter_disabled_flag;
        int8_t pps_beta_offset_div2; //[-6, 6]
        int8_t pps_tc_offset_div2; //[-6, 6]
        bool pps_scaling_list_data_present_flag;

        ScalingList scaling_list;

        bool lists_modification_present_flag;
        uint8_t log2_parallel_merge_level_minus2;
        bool slice_segment_header_extension_present_flag;
        bool pps_extension_present_flag;
        bool pps_range_extension_flag;
        bool pps_multilayer_extension_flag;
        bool pps_3d_extension_flag;
        uint8_t pps_extension_5bits;
        bool pps_extension_data_flag;
        //PPSRangeExtension pps_range_extension;
        //7.3.2.3.2 Picture parameter set range extension syntax
        uint32_t log2_max_transform_skip_block_size_minus2;
        bool cross_component_prediction_enabled_flag;
        bool chroma_qp_offset_list_enabled_flag;
        uint32_t diff_cu_chroma_qp_offset_depth;
        uint8_t chroma_qp_offset_list_len_minus1; //[0, 5]
        int8_t cb_qp_offset_list[6]; //[-12, 12]
        int8_t cr_qp_offset_list[6]; //[-12, 12]
        uint8_t log2_sao_offset_scale_luma;
        uint8_t log2_sao_offset_scale_chroma;
        //used for parsing other syntax elements
        uint32_t picWidthInCtbsY;
        uint32_t picHeightInCtbsY;

        //all non pod type should start here
        SharedPtr<SPS> sps;
    };

    class NalUnit {
    public:
        enum {
            NALU_HEAD_SIZE = 2,
            NALU_MIN_SIZE = 4
        };

        //Table 7-1
        enum NalUnitType {
            //Coded slice segment of a non-TSA, non-STSA trailing picture
            TRAIL_N,
            TRAIL_R,
            //Coded slice segment of a TSA picture
            TSA_N,
            TSA_R,
            //Coded slice segment of an STSA picture
            STSA_N,
            STSA_R,
            //Coded slice segment of a RADL picture
            RADL_N,
            RADL_R,
            //Coded slice segment of a RASL picture
            RASL_N,
            RASL_R,
            //10, 12, 14 Reserved non-IRAP SLNR VCL NAL unit types
            RSV_VCL_N10 = 10,
            RSV_VCL_N12 = 12,
            RSV_VCL_N14 = 14,
            //11, 13, 15 Reserved non-IRAP sub-layer reference VCL NAL unit types
            //Coded slice segment of a BLA picture
            BLA_W_LP = 16,
            BLA_W_RADL,
            BLA_N_LP,
            //Coded slice segment of an IDR picture
            IDR_W_RADL,
            IDR_N_LP,
            //Coded slice segment of a CRA picture
            CRA_NUT,
            //22 and 23 for Reserved IRAP VCL NAL unit types/
            RSV_IRAP_VCL22,
            RSV_IRAP_VCL23,
            //24 ... 31 for Reserved non-IRAP VCL NAL unit types
            VPS_NUT = 32, //Video parameter set
            SPS_NUT, //Sequence parameter set
            PPS_NUT, //Picture parameter set
            AUD_NUT, //Access unit delimiter
            EOS_NUT, //End of sequence
            EOB_NUT, //End of bitstream
            FD_NUT, //Filler data
            PREFIX_SEI_NUT, //Supplemental enhancement information
            SUFFIX_SEI_NUT
            //41 ... 47 Reserved
            //48 ... 63 Unspecified
        };

        /* nal should be a complete nal unit without start code or length bytes */
        bool parseNaluHeader(const uint8_t* nal, size_t size);

    public:
        const uint8_t* m_data;
        uint32_t m_size;

        //defined in spec, keeping their original name
        uint8_t nal_unit_type;
        uint8_t nuh_layer_id;
        uint8_t nuh_temporal_id_plus1;
    };

    struct SliceHeader {
        SliceHeader();
        ~SliceHeader();

        uint32_t getSliceDataByteOffset() const;

        bool isBSlice() const;
        bool isPSlice() const;
        bool isISlice() const;

        uint8_t pps_id; //slice_pic_parameter_set_id
        bool first_slice_segment_in_pic_flag;
        bool no_output_of_prior_pics_flag;
        bool dependent_slice_segment_flag;
        uint32_t slice_segment_address;
        uint8_t slice_type;
        bool pic_output_flag;
        uint8_t colour_plane_id;
        uint16_t slice_pic_order_cnt_lsb;
        bool short_term_ref_pic_set_sps_flag;
        uint8_t short_term_ref_pic_set_idx; //[0, 63]
        ShortTermRefPicSet short_term_ref_pic_sets;

        uint8_t num_long_term_sps; //[0, 32]
        uint8_t num_long_term_pics;
        uint8_t lt_idx_sps[16]; //[0, 31]
        uint32_t poc_lsb_lt[16];
        bool used_by_curr_pic_lt_flag[16];
        bool delta_poc_msb_present_flag[16];
        uint32_t delta_poc_msb_cycle_lt[16];
        bool temporal_mvp_enabled_flag;
        bool sao_luma_flag;
        bool sao_chroma_flag;
        bool num_ref_idx_active_override_flag;
        uint8_t num_ref_idx_l0_active_minus1; //[0, 14]
        uint8_t num_ref_idx_l1_active_minus1; //[0, 14]

        RefPicListModification ref_pic_list_modification;

        bool mvd_l1_zero_flag;
        bool cabac_init_flag;
        bool collocated_from_l0_flag;
        uint8_t collocated_ref_idx;

        PredWeightTable pred_weight_table;

        uint8_t five_minus_max_num_merge_cand;
        int8_t qp_delta;
        int8_t cb_qp_offset; //[-12, 12]
        int8_t cr_qp_offset; //[-12, 12]
        bool cu_chroma_qp_offset_enabled_flag;
        bool deblocking_filter_override_flag;
        bool deblocking_filter_disabled_flag;
        int8_t beta_offset_div2; //[-6, 6]
        int8_t tc_offset_div2; //[-6, 6]
        bool loop_filter_across_slices_enabled_flag;
        uint32_t num_entry_point_offsets;
        uint8_t offset_len_minus1; //[0, 31]
        uint16_t slice_segment_header_extension_length; //[0, 256]

        //Size of the slice_header() in bits
        uint32_t headerSize;
        //Number of emulation prevention bytes
        uint32_t emulationPreventionBytes;

        //all none pod type should start here
        SharedPtr<PPS> pps;
        std::vector<uint32_t> entry_point_offset_minus1;
    };

    class Parser {
    public:
        bool parseVps(const NalUnit* nalu);
        bool parseSps(const NalUnit* nalu);
        bool parsePps(const NalUnit* nalu);
        bool parseSlice(const NalUnit* nalu, SliceHeader* slice);

    private:
        static uint8_t EXTENDED_SAR;

        bool profileTierLevel(ProfileTierLevel* ptl, NalReader& nr, uint8_t maxNumSubLayersMinus1);
        bool subLayerHrdParameters(SubLayerHRDParameters* subParams,
            NalReader& nr, uint32_t CpbCnt, uint8_t subPicParamsPresentFlag);
        bool hrdParameters(HRDParameters* params, NalReader& nr,
            uint8_t commonInfPresentFlag, uint8_t maxNumSubLayersMinus1);
        bool vuiParameters(SPS* sps, NalReader& nr);
        bool useDefaultScalingLists(uint8_t* dest, uint8_t* dstDcList, uint8_t sizeId, uint8_t matrixId);
        bool scalingListData(ScalingList* dest_scaling_list, NalReader& nr);
        bool stRefPicSet(ShortTermRefPicSet* stRPS,
            NalReader& nr, uint8_t stRpsIdx, SPS* sps);
        bool refPicListsModification(SliceHeader* slice,
            NalReader& nr, int32_t numPicTotalCurr);
        bool predWeightTable(SliceHeader* slice, NalReader& nr);

        typedef std::map<uint8_t, SharedPtr<VPS> > VpsMap;
        typedef std::map<uint8_t, SharedPtr<SPS> > SpsMap;
        typedef std::map<uint8_t, SharedPtr<PPS> > PpsMap;

        SharedPtr<VPS> getVps(uint8_t id) const;
        SharedPtr<SPS> getSps(uint8_t id) const;
        SharedPtr<PPS> getPps(uint8_t id) const;

        VpsMap m_vps;
        SpsMap m_sps;
        PpsMap m_pps;

        friend class H265ParserTest;
    };
}
}
#endif
