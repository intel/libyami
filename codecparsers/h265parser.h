/* reamer H.265 bitstream parser
 * Copyright (C) 2013 Intel Corporation
 * Copyright (C) 2013 Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *
 *  Contact: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __H265_PARSER_H__
#define __H265_PARSER_H__

#ifndef USE_UNSTABLE_API
#warning "The H.265 parsing library is unstable API and may change in future."
#warning "You can define USE_UNSTABLE_API to avoid this warning."
#endif

#include "gst/gst.h"

G_BEGIN_DECLS

#define H265_MAX_SUB_LAYERS   8
#define H265_MAX_VPS_COUNT   16
#define H265_MAX_SPS_COUNT   16
#define H265_MAX_PPS_COUNT   64

#define H265_IS_B_SLICE(slice)  ((slice)->type == H265_B_SLICE)
#define H265_IS_P_SLICE(slice)  ((slice)->type == H265_P_SLICE)
#define H265_IS_I_SLICE(slice)  ((slice)->type == H265_I_SLICE)

/**
 * H265Profile:
 * @H265_PROFILE_MAIN: Main profile (A.3.2)
 * @H265_PROFILE_MAIN_10: Main 10 profile (A.3.3)
 * @H265_PROFILE_MAIN_STILL_PICTURE: Main Still Picture profile (A.3.4)
 *
 * H.265 Profiles.
 *
 */
typedef enum {
  H265_PROFILE_MAIN                 = 1,
  H265_PROFILE_MAIN_10              = 2,
  H265_PROFILE_MAIN_STILL_PICTURE   = 3
} H265Profile;

/**
 * H265NalUnitType:
 * @H265_NAL_SLICE_TRAIL_N: Slice nal of a non-TSA, non-STSA trailing picture
 * @H265_NAL_SLICE_TRAIL_R: Slice nal of a non-TSA, non-STSA trailing picture
 * @H265_NAL_SLICE_TSA_N: Slice nal of a TSA picture
 * @H265_NAL_SLICE_TSA_R: Slice nal of a TSA picture
 * @H265_NAL_SLICE_STSA_N: Slice nal of a STSA picture
 * @H265_NAL_SLICE_STSA_R: Slice nal of a STSA picture
 * @H265_NAL_SLICE_RADL_N: Slice nal of a RADL picture
 * @H265_NAL_SLICE_RADL_R: Slice nal of a RADL piicture
 * @H265_NAL_SLICE_RASL_N: Slice nal of a RASL picture
 * @H265_NAL_SLICE_RASL_R: Slice nal of a RASL picture
 * @H265_NAL_SLICE_BLA_W_LP: Slice nal of a BLA picture
 * @H265_NAL_SLICE_BLA_W_RADL: Slice nal of a BLA picture
 * @H265_NAL_SLICE_BLA_N_LP: Slice nal of a BLA picture
 * @H265_NAL_SLICE_IDR_W_RADL: Slice nal of an IDR picture
 * @H265_NAL_SLICE_IDR_N_LP: Slice nal of an IDR picture
 * @H265_NAL_SLICE_CRA_NUT: Slice nal of a CRA picture
 * @H265_NAL_VPS: Video parameter set(VPS) nal unit
 * @H265_NAL_SPS: Sequence parameter set (SPS) nal unit
 * @H265_NAL_PPS: Picture parameter set (PPS) nal unit
 * @H265_NAL_AUD: Access unit (AU) delimiter nal unit
 * @H265_NAL_EOS: End of sequence (EOS) nal unit
 * @H265_NAL_EOB: End of bitstream (EOB) nal unit
 * @H265_NAL_FD: Filler data (FD) nal lunit
 * @H265_NAL_PREFIX_SEI: Supplemental enhancement information prefix nal unit
 * @H265_NAL_SUFFIX_SEI: Suppliemental enhancement information suffix nal unit
 *
 * Indicates the type of H265 Nal Units
 */
typedef enum
{
  H265_NAL_SLICE_TRAIL_N    = 0,
  H265_NAL_SLICE_TRAIL_R    = 1,
  H265_NAL_SLICE_TSA_N      = 2,
  H265_NAL_SLICE_TSA_R      = 3,
  H265_NAL_SLICE_STSA_N     = 4,
  H265_NAL_SLICE_STSA_R     = 5,
  H265_NAL_SLICE_RADL_N     = 6,
  H265_NAL_SLICE_RADL_R     = 7,
  H265_NAL_SLICE_RASL_N     = 8,
  H265_NAL_SLICE_RASL_R     = 9,
  H265_NAL_SLICE_BLA_W_LP   = 16,
  H265_NAL_SLICE_BLA_W_RADL = 17,
  H265_NAL_SLICE_BLA_N_LP   = 18,
  H265_NAL_SLICE_IDR_W_RADL = 19,
  H265_NAL_SLICE_IDR_N_LP   = 20,
  H265_NAL_SLICE_CRA_NUT    = 21,
  H265_NAL_VPS              = 32,
  H265_NAL_SPS              = 33,
  H265_NAL_PPS              = 34,
  H265_NAL_AUD              = 35,
  H265_NAL_EOS              = 36,
  H265_NAL_EOB              = 37,
  H265_NAL_FD               = 38,
  H265_NAL_PREFIX_SEI       = 39,
  H265_NAL_SUFFIX_SEI       = 40
} H265NalUnitType;

#define RESERVED_NON_IRAP_SUBLAYER_NAL_TYPE_MIN 10
#define RESERVED_NON_IRAP_SUBLAYER_NAL_TYPE_MAX 15

#define RESERVED_IRAP_NAL_TYPE_MIN 22
#define RESERVED_IRAP_NAL_TYPE_MAX 23

#define RESERVED_NON_IRAP_NAL_TYPE_MIN 24
#define RESERVED_NON_IRAP_NAL_TYPE_MAX 31

#define RESERVED_NON_VCL_NAL_TYPE_MIN 41
#define RESERVED_NON_VCL_NAL_TYPE_MAX 47

#define UNSPECIFIED_NON_VCL_NAL_TYPE_MIN 48
#define UNSPECIFIED_NON_VCL_NAL_TYPE_MAX 63

/**
 * H265ParserResult:
 * @H265_PARSER_OK: The parsing succeded
 * @H265_PARSER_BROKEN_DATA: The data to parse is broken
 * @H265_PARSER_BROKEN_LINK: The link to structure needed for the parsing couldn't be found
 * @H265_PARSER_ERROR: An error accured when parsing
 * @H265_PARSER_NO_NAL: No nal found during the parsing
 * @H265_PARSER_NO_NAL_END: Start of the nal found, but not the end.
 *
 * The result of parsing H265 data.
 */
typedef enum
{
  H265_PARSER_OK,
  H265_PARSER_BROKEN_DATA,
  H265_PARSER_BROKEN_LINK,
  H265_PARSER_ERROR,
  H265_PARSER_NO_NAL,
  H265_PARSER_NO_NAL_END
} H265ParserResult;

/**
 * H265SEIPayloadType:
 * @H265_SEI_BUF_PERIOD: Buffering Period SEI Message
 * @H265_SEI_PIC_TIMING: Picture Timing SEI Message
 * ...
 *
 * The type of SEI message.
 */
typedef enum
{
  H265_SEI_BUF_PERIOD = 0,
  H265_SEI_PIC_TIMING = 1
      /* and more...  */
} H265SEIPayloadType;

/**
 * H265SEIPicStructType:
 * @H265_SEI_PIC_STRUCT_FRAME: Picture is a frame
 * @H265_SEI_PIC_STRUCT_TOP_FIELD: Top field of frame
 * @H265_SEI_PIC_STRUCT_BOTTOM_FIELD: Botom field of frame
 * @H265_SEI_PIC_STRUCT_TOP_BOTTOM: Top bottom field of frame
 * @H265_SEI_PIC_STRUCT_BOTTOM_TOP: bottom top field of frame
 * @H265_SEI_PIC_STRUCT_TOP_BOTTOM_TOP: top bottom top field of frame
 * @H265_SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM: bottom top bottom field of frame
 * @H265_SEI_PIC_STRUCT_FRAME_DOUBLING: indicates that the frame should
 *  be displayed two times consecutively
 * @H265_SEI_PIC_STRUCT_FRAME_TRIPLING: indicates that the frame should be
 *  displayed three times consecutively
 * @H265_SEI_PIC_STRUCT_TOP_PAIRED_PREVIOUS_BOTTOM: top field paired with
 *  previous bottom field in output order
 * @H265_SEI_PIC_STRUCT_BOTTOM_PAIRED_PREVIOUS_TOP: bottom field paried with
 *  previous top field in output order
 * @H265_SEI_PIC_STRUCT_TOP_PAIRED_NEXT_BOTTOM: top field paired with next
 *  bottom field in output order
 * @H265_SEI_PIC_STRUCT_BOTTOM_PAIRED_NEXT_TOP: bottom field paired with
 *  next top field in output order
 *
 * SEI pic_struct type
 */
typedef enum
{
  H265_SEI_PIC_STRUCT_FRAME                         = 0,
  H265_SEI_PIC_STRUCT_TOP_FIELD                     = 1,
  H265_SEI_PIC_STRUCT_BOTTOM_FIELD                  = 2,
  H265_SEI_PIC_STRUCT_TOP_BOTTOM                    = 3,
  H265_SEI_PIC_STRUCT_BOTTOM_TOP                    = 4,
  H265_SEI_PIC_STRUCT_TOP_BOTTOM_TOP                = 5,
  H265_SEI_PIC_STRUCT_BOTTOM_TOP_BOTTOM             = 6,
  H265_SEI_PIC_STRUCT_FRAME_DOUBLING                = 7,
  H265_SEI_PIC_STRUCT_FRAME_TRIPLING                = 8,
  H265_SEI_PIC_STRUCT_TOP_PAIRED_PREVIOUS_BOTTOM    = 9,
  H265_SEI_PIC_STRUCT_BOTTOM_PAIRED_PREVIOUS_TOP    = 10,
  H265_SEI_PIC_STRUCT_TOP_PAIRED_NEXT_BOTTOM        = 11,
  H265_SEI_PIC_STRUCT_BOTTOM_PAIRED_NEXT_TOP        = 12
} H265SEIPicStructType;

/**
 * H265SliceType:
 *
 * Type of Picture slice
 */

typedef enum
{
  H265_B_SLICE    = 0,
  H265_P_SLICE    = 1,
  H265_I_SLICE    = 2
} H265SliceType;

typedef enum
{
  H265_QUANT_MATIX_4X4   = 0,
  H265_QUANT_MATIX_8X8   = 1,
  H265_QUANT_MATIX_16X16 = 2,
  H265_QUANT_MATIX_32X32 = 3
} H265QuantMatrixSize;

typedef struct _H265Parser                   H265Parser;

typedef struct _H265NalUnit                  H265NalUnit;

typedef struct _H265VPS                      H265VPS;
typedef struct _H265SPS                      H265SPS;
typedef struct _H265PPS                      H265PPS;
typedef struct _H265ProfileTierLevel         H265ProfileTierLevel;
typedef struct _H265SubLayerHRDParams        H265SubLayerHRDParams;
typedef struct _H265HRDParams                H265HRDParams;
typedef struct _H265VUIParams                H265VUIParams;

typedef struct _H265ScalingList              H265ScalingList;
typedef struct _H265RefPicListModification   H265RefPicListModification;
typedef struct _H265PredWeightTable          H265PredWeightTable;
typedef struct _H265ShortTermRefPicSet       H265ShortTermRefPicSet;
typedef struct _H265SliceHdr                 H265SliceHdr;

typedef struct _H265PicTiming                H265PicTiming;
typedef struct _H265BufferingPeriod          H265BufferingPeriod;
typedef struct _H265SEIMessage               H265SEIMessage;

/**
 * H265NalUnit:
 * @type: A #H265NalUnitType
 * @layer_id: A nal unit layer id
 * @temporal_id_plus1: A nal unit temporal identifier
 * @size: The size of the nal unit starting from @offset
 * @offset: The offset of the actual start of the nal unit
 * @sc_offset:The offset of the start code of the nal unit
 * @valid: If the nal unit is valid, which mean it has
 * already been parsed
 * @data: The data from which the Nalu has been parsed
 *
 * Structure defining the Nal unit headers
 */
struct _H265NalUnit
{
  uint8_t type;
  uint8_t layer_id;
  uint8_t temporal_id_plus1;

  /* calculated values */
  uint32_t size;
  uint32_t offset;
  uint32_t sc_offset;
  bool valid;

  uint8_t *data;
  uint8_t header_bytes;
};

/**
 * H265ProfileTierLevel:
 * @profile_space: specifies the context for the interpretation of
 *   general_profile_idc and general_profile_combatibility_flag
 * @tier_flag: the tier context for the interpretation of general_level_idc
 * @profile_idc: profile id
 * @profile_compatibility_flag: compatibility flags
 * @progressive_source_flag: flag to indicate the type of stream
 * @interlaced_source_flag: flag to indicate the type of stream
 * @non_packed_constraint_flag: indicate the presence of frame packing
 *   arragement sei message
 * @frame_only_constraint_flag: recognize the field_seq_flag
 * @level idc: indicate the level which the CVS confirms
 * @sub_layer_profile_present_flag: sublayer profile presence ind
 * @sub_layer_level_present_flag:sublayer level presence indicator.
 * @sub_layer_profile_space: profile space for sublayers
 * @sub_layer_tier_flag: tier flags for sublayers.
 * @sub_layer_profile_idc: conformant profile indicator for sublayers.
 * @sub_layer_profile_compatibility_flag[6][32]: compatibility flags for sublayers
 * @sub_layer_progressive_source_flag:progressive stream indicator for sublayer
 * @sub_layer_interlaced_source_flag: interlaced stream indicator for sublayer
 * @sub_layer_non_packed_constraint_flag: indicate the presence of
 *   frame packing arrangement sei message with in sublayers
 * @sub_layer_frame_only_constraint_flag:recognize the sublayer
 *   specific field_seq_flag
 * @sub_layer_level_idc:indicate the sublayer specific level
 *
 * Define ProfileTierLevel parameters
 */
struct _H265ProfileTierLevel {
  uint8_t profile_space;
  uint8_t tier_flag;
  uint8_t profile_idc;

  uint8_t profile_compatibility_flag[32];

  uint8_t progressive_source_flag;
  uint8_t interlaced_source_flag;
  uint8_t non_packed_constraint_flag;
  uint8_t frame_only_constraint_flag;
  uint8_t level_idc;

  uint8_t sub_layer_profile_present_flag[6];
  uint8_t sub_layer_level_present_flag[6];

  uint8_t sub_layer_profile_space[6];
  uint8_t sub_layer_tier_flag[6];
  uint8_t sub_layer_profile_idc[6];
  uint8_t sub_layer_profile_compatibility_flag[6][32];
  uint8_t sub_layer_progressive_source_flag[6];
  uint8_t sub_layer_interlaced_source_flag[6];
  uint8_t sub_layer_non_packed_constraint_flag[6];
  uint8_t sub_layer_frame_only_constraint_flag[6];
  uint8_t sub_layer_level_idc[6];
};

/**
 * H265SubLayerHRDParams:
 * @bit_rate_value_minus1:togeter with bit_rate_scale, it specifies
 *   the maximum input bitrate when the CPB operates at the access
 *   unit level
 * @cpb_size_value_minus1: is used together with cpb_size_scale to
 *   specify the CPB size when the CPB operates at the access unit
 *   level
 * @cpb_size_du_value_minus1: is used together with cpb_size_du_scale
 *   to specify the CPB size when the CPB operates at sub-picture
 *   level
 * @bit_rate_du_value_minus1: together with bit_rate_scale, it
 *   specifies the maximum input bit rate when the CPB operates at the
 *   sub-picture level
 * @cbr_flag: indicate whether HSS operates in intermittent bit rate
 *   mode or constant bit rate mode.
 *
 * Defines the Sublayer HRD parameters
 */
struct _H265SubLayerHRDParams
{
  uint32_t bit_rate_value_minus1[32];
  uint32_t cpb_size_value_minus1[32];

  uint32_t cpb_size_du_value_minus1[32];
  uint32_t bit_rate_du_value_minus1[32];

  uint8_t cbr_flag[32];
};

/**
 * H265HRDParams:
 * @nal_hrd_parameters_present_flag: indicate the presence of NAL HRD parameters
 * @vcl_hrd_parameters_present_flag: indicate the presence of VCL HRD parameters
 * @sub_pic_hrd_params_present_flag: indicate the presence of sub_pic_hrd_params
 * @tick_divisor_minus2: is used to specify the clock sub-tick
 * @du_cpb_removal_delay_increment_length_minus1: specifies the length,
 *   in bits, of the nal_initial_cpb_removal_delay
 * @sub_pic_cpb_params_in_pic_timing_sei_flag: specifies the length, in bits, of
 *   the cpb_delay_offset and the au_cpb_removal_delay_minus1 syntax elements.
 * @dpb_output_delay_du_length_minu1: specifies the length, in bits, of the
 *   dpb_delay_offset and the pic_dpb_output_delay syntax elements
 * @bit_rate_scale: maximum input bitrate
 * @cpb_size_scale: CPB size when operates in access unit level
 * @cpb_size_du_scale: CPB size when operates in sub-picture level
 * @initial_cpb_removal_delay_length_minus1: specifies the length, in bits, of the
 *   nal_initial_cpb_removal_delay, nal_initial_cpb_removal_offset,
 *   vcl_initial_cpb_removal_delay and vcl_initial_cpb_removal_offset.
 * @au_cpb_removal_delay_length_minus1: specifies the length, in bits, of the
 *   cpb_delay_offset and the au_cpb_removal_delay_minus1 syntax elements
 * @dpb_output_delay_length_minus1: specifies the length, in bits, of the
 *   dpb_delay_offset and the pic_dpb_output_delay syntax elements
 * @fixed_pic_rate_general_flag: flag to indicate the presence of constraint
 *   on the temporal distance between the HRD output times of consecutive
 *   pictures in output order
 * @fixed_pic_rate_within_cvs_flag: same as fixed_pic_rate_general_flag
 * @elemental_duration_in_tc_minus1: temporal distance in clock ticks
 * @low_delay_hrd_flag: specifies the HRD operational mode
 * @cpb_cnt_minus1:specifies the number of alternative CPS specifications.
 * @sublayer_hrd_params: Sublayer HRD parameters.
 *
 * Defines the HRD parameters
 */
struct _H265HRDParams
{
  uint8_t nal_hrd_parameters_present_flag;
  uint8_t vcl_hrd_parameters_present_flag;
  uint8_t sub_pic_hrd_params_present_flag;

  uint8_t tick_divisor_minus2;
  uint8_t du_cpb_removal_delay_increment_length_minus1;
  uint8_t sub_pic_cpb_params_in_pic_timing_sei_flag;
  uint8_t dpb_output_delay_du_length_minus1;

  uint8_t bit_rate_scale;
  uint8_t cpb_size_scale;
  uint8_t cpb_size_du_scale;

  uint8_t initial_cpb_removal_delay_length_minus1;
  uint8_t au_cpb_removal_delay_length_minus1;
  uint8_t dpb_output_delay_length_minus1;

  uint8_t fixed_pic_rate_general_flag [7];
  uint8_t fixed_pic_rate_within_cvs_flag [7];
  uint16_t elemental_duration_in_tc_minus1 [7];
  uint8_t low_delay_hrd_flag [7];
  uint8_t cpb_cnt_minus1[7];

  H265SubLayerHRDParams sublayer_hrd_params[7];
};

/**
 * H265VPS:
 * @id: vps id
 * @max_layers_minus1: should be zero, but can be other values in future
 * @max_sub_layers_minus1:specifies the maximum number of temporal sub-layers
 * @temporal_id_nesting_flag: specifies whether inter prediction is
 *   additionally restricted
 * @profile_tier_level: ProfileTierLevel info
 * @sub_layer_ordering_info_present_flag: indicates the presense of
 *   vps_max_dec_pic_buffering_minus1, vps_max_num_reorder_pics and
 *   vps_max_latency_increase_plus1
 * @max_dec_pic_buffering_minus1: specifies the maximum required size
 *   of the decoded picture buffer
 * @max_num_reorder_pics: indicates the maximum allowed number of
 *   pictures that can precede any picture in the CVS in decoding
 *   order
 * @max_latency_increase_plus1: is used to compute the value of
 *   VpsMaxLatencyPictures
 * @max_layer_id: specifies the maximum allowed value of nuh_layer_id
 * @num_layer_sets_minus1: specifies the number of layer sets
 * @layer_id_included_flag: specifies whether a nuh_layer_id included
 *   in the layer identifier list
 * @timing_info_present_flag: indicate the presence of
 *   num_units_in_tick, time_scale, poc_proportional_to_timing_flag
 *   and num_hrd_parameters
 * @num_units_in_tick: number of time units in a tick
 * @time_scale: number of time units that pass in one second
 * @poc_proportional_to_timing_flag: indicate whether the picture
 *   order count is proportional to output timin
 * @num_ticks_poc_diff_one_minus1: specifies the number of clock ticks
 *   corresponding to a difference of picture order count values equal
 *   to 1
 * @num_hrd_parameters: number of hrd_parameters present
 * @hrd_layer_set_idx: index to the list of layer hrd params.
 * @hrd_params: the H265HRDParams list
 *
 * Defines the VPS parameters
 */
struct _H265VPS {
  uint8_t id;

  uint8_t max_layers_minus1;
  uint8_t max_sub_layers_minus1;
  uint8_t temporal_id_nesting_flag;

  H265ProfileTierLevel profile_tier_level;

  uint8_t sub_layer_ordering_info_present_flag;
  uint8_t max_dec_pic_buffering_minus1[H265_MAX_SUB_LAYERS];
  uint8_t max_num_reorder_pics[H265_MAX_SUB_LAYERS];
  uint32_t max_latency_increase_plus1[H265_MAX_SUB_LAYERS];

  uint8_t max_layer_id;
  uint16_t num_layer_sets_minus1;

  uint8_t timing_info_present_flag;
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  uint8_t poc_proportional_to_timing_flag;
  uint32_t num_ticks_poc_diff_one_minus1;

  uint16_t num_hrd_parameters;
  uint16_t hrd_layer_set_idx;
  uint8_t cprms_present_flag;

  H265HRDParams hrd_params;

  uint8_t vps_extension;

  bool valid;
};
/**
 * H265ShortTermRefPicSet:
 * @inter_ref_pic_set_prediction_flag: %TRUE specifies that the stRpsIdx-th candidate
 *  short-term RPS is predicted from another candidate short-term RPS
 * @delta_idx_minus1: plus 1 specifies the difference between the value of source and
 *  candidate short term RPS.
 * @delta_rps_sign: delta_rps_sign and abs_delta_rps_minus1 together specify
 *  the value of the variable deltaRps.
 * @abs_delta_rps_minus1: delta_rps_sign and abs_delta_rps_minus1 together specify
 *  the value of the variable deltaRps
 *
 * Defines the #H265ShortTermRefPicSet params
 */
struct _H265ShortTermRefPicSet
{
  uint8_t inter_ref_pic_set_prediction_flag;
  uint8_t delta_idx_minus1;
  uint8_t delta_rps_sign;
  uint16_t abs_delta_rps_minus1;

  /* calculated values */
  uint8_t NumDeltaPocs;
  uint8_t NumNegativePics;
  uint8_t NumPositivePics;
  uint8_t UsedByCurrPicS0[16];
  uint8_t UsedByCurrPicS1[16];
  int32_t DeltaPocS0[16];
  int32_t DeltaPocS1[16];
};

/**
 * H265VUIParams:
 * @aspect_ratio_info_present_flag: %TRUE specifies that aspect_ratio_idc is present.
 *  %FALSE specifies that aspect_ratio_idc is not present
 * @aspect_ratio_idc specifies the value of the sample aspect ratio of the luma samples
 * @sar_width indicates the horizontal size of the sample aspect ratio
 * @sar_height indicates the vertical size of the sample aspect ratio
 * @overscan_info_present_flag: %TRUE overscan_appropriate_flag is present %FALSE otherwize
 * @overscan_appropriate_flag: %TRUE indicates that the cropped decoded pictures
 *  output are suitable for display using overscan. %FALSE the cropped decoded pictures
 *  output contain visually important information
 * @video_signal_type_present_flag: %TRUE specifies that video_format, video_full_range_flag and
 *  colour_description_present_flag are present.
 * @video_format: indicates the representation of the picture
 * @video_full_range_flag: indicates the black level and range of the luma and chroma signals
 * @colour_description_present_flag: %TRUE specifies that colour_primaries,
 *  transfer_characteristics and matrix_coefficients are present
 * @colour_primaries: indicates the chromaticity coordinates of the source primaries
 * @transfer_characteristics: indicates the opto-electronic transfer characteristic
 * @matrix_coefficients: describes the matrix coefficients used in deriving luma and chroma signals
 * @chroma_loc_info_present_flag: %TRUE specifies that chroma_sample_loc_type_top_field and
 *  chroma_sample_loc_type_bottom_field are present, %FALSE otherwize
 * @chroma_sample_loc_type_top_field: specify the location of chroma for top field
 * @chroma_sample_loc_type_bottom_field specify the location of chroma for bottom field
 * @neutral_chroma_indication_flag: %TRUE indicate that the value of chroma samples is equla
 *  to 1<<(BitDepthchrom-1).
 * @field_seq_flag: %TRUE indicate field and %FALSE indicate frame
 * @frame_field_info_present_flag: %TRUE indicate picture timing SEI messages are present for every
 *   picture and include the pic_struct, source_scan_type, and duplicate_flag syntax elements.
 * @default_display_window_flag: %TRUE indicate that the default display window parameters present
 * def_disp_win_left_offset:left offset of display rect
 * def_disp_win_right_offset: right offset of display rect
 * def_disp_win_top_offset: top offset of display rect
 * def_disp_win_bottom_offset: bottom offset of display rect
 * @timing_info_present_flag: %TRUE specifies that num_units_in_tick,
 *  time_scale and fixed_frame_rate_flag are present in the bitstream
 * @num_units_in_tick: is the number of time units of a clock operating at the frequency time_scale Hz
 * @time_scale: is the number of time units that pass in one second
 * @poc_proportional_to_timing_flag: %TRUE indicates that the picture order count value for each picture
 *  in the CVS that is not the first picture in the CVS, in decoding order, is proportional to the output
 *  time of the picture relative to the output time of the first picture in the CVS.
 * @num_ticks_poc_diff_one_minus1: plus 1 specifies the number of clock ticks corresponding to a
 *  difference of picture order count values equal to 1
 * @hrd_parameters_present_flag: %TRUE if hrd parameters present in the bitstream
 * @bitstream_restriction_flag: %TRUE specifies that the following coded video sequence bitstream restriction
 * parameters are present
 * @tiles_fixed_structure_flag: %TRUE indicates that each PPS that is active in the CVS has the same value
 *   of the syntax elements num_tile_columns_minus1, num_tile_rows_minus1, uniform_spacing_flag,
 *   column_width_minus1, row_height_minus1 and loop_filter_across_tiles_enabled_flag, when present
 * @motion_vectors_over_pic_boundaries_flag: %FALSE indicates that no sample outside the
 *  picture boundaries and no sample at a fractional sample position, %TRUE indicates that one or more
 *  samples outside picture boundaries may be used in inter prediction
 * @restricted_ref_pic_list_flag: %TRUE indicates that all P and B slices (when present) that belong to
 *  the same picture have an identical reference picture list 0, and that all B slices (when present)
 *   that belong to the same picture have an identical reference picture list 1
 * @min_spatial_segmentation_idc: when not equal to 0, establishes a bound on the maximum possible size
 *  of distinct coded spatial segmentation regions in the pictures of the CVS
 * @max_bytes_per_pic_denom: indicates a number of bytes not exceeded by the sum of the sizes of
 *  the VCL NAL units associated with any coded picture in the coded video sequence.
 * @max_bits_per_min_cu_denom: indicates an upper bound for the number of coded bits of coding_unit
 *  data for any coding block in any picture of the CVS
 * @log2_max_mv_length_horizontal: indicate the maximum absolute value of a decoded horizontal
 * motion vector component
 * @log2_max_mv_length_vertical: indicate the maximum absolute value of a decoded vertical
 *  motion vector component
 *
 * The structure representing the VUI parameters.
 */
struct _H265VUIParams
{
  uint8_t aspect_ratio_info_present_flag;
  uint8_t aspect_ratio_idc;
  /* if aspect_ratio_idc == 255 */
  uint16_t sar_width;
  uint16_t sar_height;

  uint8_t overscan_info_present_flag;
  /* if overscan_info_present_flag */
  uint8_t overscan_appropriate_flag;

  uint8_t video_signal_type_present_flag;
  uint8_t video_format;
  uint8_t video_full_range_flag;
  uint8_t colour_description_present_flag;
  uint8_t colour_primaries;
  uint8_t transfer_characteristics;
  uint8_t matrix_coefficients;

  uint8_t chroma_loc_info_present_flag;
  uint8_t chroma_sample_loc_type_top_field;
  uint8_t chroma_sample_loc_type_bottom_field;

  uint8_t neutral_chroma_indication_flag;
  uint8_t field_seq_flag;
  uint8_t frame_field_info_present_flag;
  uint8_t default_display_window_flag;
  uint32_t def_disp_win_left_offset;
  uint32_t def_disp_win_right_offset;
  uint32_t def_disp_win_top_offset;
  uint32_t def_disp_win_bottom_offset;

  uint8_t timing_info_present_flag;
  /* if timing_info_present_flag */
  uint32_t num_units_in_tick;
  uint32_t time_scale;
  uint8_t poc_proportional_to_timing_flag;
  /* if poc_proportional_to_timing_flag */
  uint32_t num_ticks_poc_diff_one_minus1;
  uint8_t hrd_parameters_present_flag;
  /*if hrd_parameters_present_flat */
  H265HRDParams hrd_params;

  uint8_t bitstream_restriction_flag;
  /*  if bitstream_restriction_flag */
  uint8_t tiles_fixed_structure_flag;
  uint8_t motion_vectors_over_pic_boundaries_flag;
  uint8_t restricted_ref_pic_lists_flag;
  uint16_t min_spatial_segmentation_idc;
  uint8_t max_bytes_per_pic_denom;
  uint8_t max_bits_per_min_cu_denom;
  uint8_t log2_max_mv_length_horizontal;
  uint8_t log2_max_mv_length_vertical;

  /* calculated values */
  uint32_t par_n;
  uint32_t par_d;
};

/**
 * H265ScalingList:
 * @scaling_list_dc_coef_minus8_16x16: this plus 8 specifies the DC
 *   Coefficient values for 16x16 scaling list
 * @scaling_list_dc_coef_minus8_32x32: this plus 8 specifies the DC
 *   Coefficient values for 32x32 scaling list
 * @scaling_lists_4x4: 4x4 scaling list
 * @scaling_lists_8x8: 8x8 scaling list
 * @scaling_lists_16x16: 16x16 scaling list
 * @uint8_t scaling_lists_32x32: 32x32 scaling list
 *
 * Defines the H265ScalingList
 */
struct _H265ScalingList {

  int16_t scaling_list_dc_coef_minus8_16x16[6];
  int16_t scaling_list_dc_coef_minus8_32x32[2];

  uint8_t scaling_lists_4x4 [6][16];
  uint8_t scaling_lists_8x8 [6][64];
  uint8_t scaling_lists_16x16 [6][64];
  uint8_t scaling_lists_32x32 [2][64];
};

/**
 * H265SPS:
 * @id: The ID of the sequence parameter set
 * @profile_idc: indicate the profile to which the coded video sequence conforms
 *
 * H265 Sequence Parameter Set (SPS)
 */
struct _H265SPS
{
  uint8_t id;

  H265VPS *vps;

  uint8_t max_sub_layers_minus1;
  uint8_t temporal_id_nesting_flag;

  H265ProfileTierLevel profile_tier_level;

  uint8_t chroma_format_idc;
  uint8_t separate_colour_plane_flag;
  uint16_t pic_width_in_luma_samples;
  uint16_t pic_height_in_luma_samples;

  uint8_t conformance_window_flag;
  /* if conformance_window_flag */
  uint32_t conf_win_left_offset;
  uint32_t conf_win_right_offset;
  uint32_t conf_win_top_offset;
  uint32_t conf_win_bottom_offset;

  uint8_t bit_depth_luma_minus8;
  uint8_t bit_depth_chroma_minus8;
  uint8_t log2_max_pic_order_cnt_lsb_minus4;

  uint8_t sub_layer_ordering_info_present_flag;
  uint8_t max_dec_pic_buffering_minus1[H265_MAX_SUB_LAYERS];
  uint8_t max_num_reorder_pics[H265_MAX_SUB_LAYERS];
  uint8_t max_latency_increase_plus1[H265_MAX_SUB_LAYERS];

  uint8_t log2_min_luma_coding_block_size_minus3;
  uint8_t log2_diff_max_min_luma_coding_block_size;
  uint8_t log2_min_transform_block_size_minus2;
  uint8_t log2_diff_max_min_transform_block_size;
  uint8_t max_transform_hierarchy_depth_inter;
  uint8_t max_transform_hierarchy_depth_intra;

  uint8_t scaling_list_enabled_flag;
  /* if scaling_list_enabled_flag */
  uint8_t scaling_list_data_present_flag;

  H265ScalingList scaling_list;

  uint8_t amp_enabled_flag;
  uint8_t sample_adaptive_offset_enabled_flag;
  uint8_t pcm_enabled_flag;
  /* if pcm_enabled_flag */
  uint8_t pcm_sample_bit_depth_luma_minus1;
  uint8_t pcm_sample_bit_depth_chroma_minus1;
  uint8_t log2_min_pcm_luma_coding_block_size_minus3;
  uint8_t log2_diff_max_min_pcm_luma_coding_block_size;
  uint8_t pcm_loop_filter_disabled_flag;

  uint8_t num_short_term_ref_pic_sets;
  H265ShortTermRefPicSet short_term_ref_pic_set[65];

  uint8_t long_term_ref_pics_present_flag;
  /* if long_term_ref_pics_present_flag */
  uint8_t num_long_term_ref_pics_sps;
  uint16_t lt_ref_pic_poc_lsb_sps[32];
  uint8_t used_by_curr_pic_lt_sps_flag[32];

  uint8_t temporal_mvp_enabled_flag;
  uint8_t strong_intra_smoothing_enabled_flag;
  uint8_t vui_parameters_present_flag;

  /* if vui_parameters_present_flat */
  H265VUIParams vui_params;

  uint8_t sps_extension_flag;

  /* calculated values */
  uint8_t chroma_array_type;
  int32_t width, height;
  int32_t crop_rect_width, crop_rect_height;
  int32_t crop_rect_x, crop_rect_y;
  int32_t fps_num, fps_den;
  bool valid;
};

/**
 * H265PPS:
 *
 * H265 Picture Parameter Set
 */
struct _H265PPS
{
  uint32_t id;

  H265SPS *sps;

  uint8_t dependent_slice_segments_enabled_flag;
  uint8_t output_flag_present_flag;
  uint8_t num_extra_slice_header_bits;
  uint8_t sign_data_hiding_enabled_flag;
  uint8_t cabac_init_present_flag;
  uint8_t num_ref_idx_l0_default_active_minus1;
  uint8_t num_ref_idx_l1_default_active_minus1;
  int8_t init_qp_minus26;
  uint8_t constrained_intra_pred_flag;
  uint8_t transform_skip_enabled_flag;
  uint8_t cu_qp_delta_enabled_flag;
  /*if cu_qp_delta_enabled_flag */
  uint8_t diff_cu_qp_delta_depth;

  int8_t cb_qp_offset;
  int8_t cr_qp_offset;
  uint8_t slice_chroma_qp_offsets_present_flag;
  uint8_t weighted_pred_flag;
  uint8_t weighted_bipred_flag;
  uint8_t transquant_bypass_enabled_flag;
  uint8_t tiles_enabled_flag;
  uint8_t entropy_coding_sync_enabled_flag;

  uint8_t num_tile_columns_minus1;
  uint8_t num_tile_rows_minus1;
  uint8_t uniform_spacing_flag;
  uint32_t column_width_minus1[19];
  uint32_t row_height_minus1[21];
  uint8_t loop_filter_across_tiles_enabled_flag;

  uint8_t loop_filter_across_slices_enabled_flag;
  uint8_t deblocking_filter_control_present_flag;
  uint8_t deblocking_filter_override_enabled_flag;
  uint8_t deblocking_filter_disabled_flag;
  int8_t beta_offset_div2;
  int8_t tc_offset_div2;

  uint8_t scaling_list_data_present_flag;

  H265ScalingList scaling_list;

  uint8_t lists_modification_present_flag;
  uint8_t log2_parallel_merge_level_minus2;
  uint8_t slice_segment_header_extension_present_flag;

  uint8_t pps_extension_flag;

  bool valid;
};

struct _H265RefPicListModification
{
  uint8_t ref_pic_list_modification_flag_l0;
  uint32_t list_entry_l0[15];
  uint8_t ref_pic_list_modification_flag_l1;
  uint32_t list_entry_l1[15];
};

struct _H265PredWeightTable
{
  uint8_t luma_log2_weight_denom;
  int8_t delta_chroma_log2_weight_denom;

  uint8_t luma_weight_l0_flag[15];
  uint8_t  chroma_weight_l0_flag[15];
  int8_t delta_luma_weight_l0[15];
  int8_t luma_offset_l0[15];
  int8_t delta_chroma_weight_l0 [15][2];
  int16_t delta_chroma_offset_l0 [15][2];

  uint8_t luma_weight_l1_flag[15];
  uint8_t chroma_weight_l1_flag[15];
  int8_t delta_luma_weight_l1[15];
  int8_t luma_offset_l1[15];
  int8_t delta_chroma_weight_l1[15][2];
  int16_t delta_chroma_offset_l1[15][2];
};

struct _H265SliceHdr
{
  uint8_t first_slice_segment_in_pic_flag;
  uint8_t no_output_of_prior_pics_flag;

  H265PPS *pps;

  uint8_t dependent_slice_segment_flag;
  uint32_t segment_address;

  uint8_t type;

  uint8_t pic_output_flag;
  uint8_t colour_plane_id;
  uint16_t pic_order_cnt_lsb;

  uint8_t  short_term_ref_pic_set_sps_flag;
  H265ShortTermRefPicSet short_term_ref_pic_sets;
  uint8_t short_term_ref_pic_set_idx;

  uint8_t num_long_term_sps;
  uint8_t num_long_term_pics;
  uint8_t lt_idx_sps[16];
  uint32_t poc_lsb_lt[16];
  uint8_t used_by_curr_pic_lt_flag[16];
  uint8_t delta_poc_msb_present_flag[16];
  uint32_t delta_poc_msb_cycle_lt[16];

  uint8_t temporal_mvp_enabled_flag;
  uint8_t sao_luma_flag;
  uint8_t sao_chroma_flag;
  uint8_t num_ref_idx_active_override_flag;
  uint8_t num_ref_idx_l0_active_minus1;
  uint8_t num_ref_idx_l1_active_minus1;

  H265RefPicListModification ref_pic_list_modification;

  uint8_t mvd_l1_zero_flag;
  uint8_t cabac_init_flag;
  uint8_t collocated_from_l0_flag;
  uint8_t collocated_ref_idx;

  H265PredWeightTable pred_weight_table;

  uint8_t five_minus_max_num_merge_cand;

  int8_t qp_delta;
  int8_t cb_qp_offset;
  int8_t cr_qp_offset;

  uint8_t deblocking_filter_override_flag;
  uint8_t deblocking_filter_disabled_flag;
  int8_t beta_offset_div2;
  int8_t tc_offset_div2;

  uint8_t loop_filter_across_slices_enabled_flag;

  uint32_t num_entry_point_offsets;
  uint8_t offset_len_minus1;
  uint32_t *entry_point_offset_minus1;

  /* calculated values */

  int32_t NumPocTotalCurr;
  /* Size of the slice_header() in bits */
  uint32_t header_size;
  /* Number of emulation prevention bytes (EPB) in this slice_header() */
  uint32_t n_emulation_prevention_bytes;
};

struct _H265PicTiming
{
  uint8_t pic_struct;
  uint8_t source_scan_type;
  uint8_t duplicate_flag;

  uint8_t au_cpb_removal_delay_minus1;
  uint8_t pic_dpb_output_delay;
  uint8_t pic_dpb_output_du_delay;
  uint32_t num_decoding_units_minus1;
  uint8_t du_common_cpb_removal_delay_flag;
  uint8_t du_common_cpb_removal_delay_increment_minus1;
  uint32_t *num_nalus_in_du_minus1;
  uint8_t *du_cpb_removal_delay_increment_minus1;
};

struct _H265BufferingPeriod
{
  H265SPS *sps;

  uint8_t irap_cpb_params_present_flag;
  uint8_t cpb_delay_offset;
  uint8_t dpb_delay_offset;
  uint8_t concatenation_flag;
  uint8_t au_cpb_removal_delay_delta_minus1;

  /* seq->vui_parameters->nal_hrd_parameters_present_flag */
  uint8_t nal_initial_cpb_removal_delay[32];
  uint8_t nal_initial_cpb_removal_offset[32];
  uint8_t nal_initial_alt_cpb_removal_delay[32];
  uint8_t nal_initial_alt_cpb_removal_offset [32];

  /* seq->vui_parameters->vcl_hrd_parameters_present_flag */
  uint8_t vcl_initial_cpb_removal_delay[32];
  uint8_t vcl_initial_cpb_removal_offset[32];
  uint8_t vcl_initial_alt_cpb_removal_delay[32];
  uint8_t vcl_initial_alt_cpb_removal_offset[32];
};

struct _H265SEIMessage
{
  H265SEIPayloadType payloadType;

  union {
    H265BufferingPeriod buffering_period;
    H265PicTiming pic_timing;
    /* ... could implement more */
  } payload;
};

/**
 * H265Parser:
 *
 * H265 NAL Parser (opaque structure).
 */
struct _H265Parser
{
  /*< private >*/
  H265VPS vps[H265_MAX_VPS_COUNT];
  H265SPS sps[H265_MAX_SPS_COUNT];
  H265PPS pps[H265_MAX_PPS_COUNT];
  H265VPS *last_vps;
  H265SPS *last_sps;
  H265PPS *last_pps;
};

H265Parser *     h265_parser_new               (void);

H265ParserResult h265_parser_identify_nalu      (H265Parser  * parser,
                                                        const uint8_t   * data,
                                                        uint32_t            offset,
                                                        size_t            size,
                                                        H265NalUnit * nalu);

H265ParserResult h265_parser_identify_nalu_unchecked (H265Parser * parser,
                                                        const uint8_t   * data,
                                                        uint32_t            offset,
                                                        size_t            size,
                                                        H265NalUnit * nalu);

H265ParserResult h265_parser_identify_nalu_hevc (H265Parser  * parser,
                                                        const uint8_t   * data,
                                                        uint32_t            offset,
                                                        size_t            size,
                                                        uint8_t           nal_length_size,
                                                        H265NalUnit * nalu);

H265ParserResult h265_parser_parse_nal       (H265Parser   * parser,
                                                     H265NalUnit  * nalu);

H265ParserResult h265_parser_parse_slice_hdr (H265Parser   * parser,
                                                     H265NalUnit  * nalu,
                                                     H265SliceHdr * slice);

H265ParserResult h265_parser_parse_vps       (H265Parser   * parser,
                                                     H265NalUnit  * nalu,
                                                     H265VPS      * vps);

H265ParserResult h265_parser_parse_sps       (H265Parser   * parser,
                                                     H265NalUnit  * nalu,
                                                     H265SPS      * sps,
                                                     bool          parse_vui_params);

H265ParserResult h265_parser_parse_pps       (H265Parser   * parser,
                                                     H265NalUnit  * nalu,
                                                     H265PPS      * pps);

H265ParserResult h265_parser_parse_sei       (H265Parser   * parser,
                                                     H265NalUnit  * nalu,
                                                     GArray **messages);

void                h265_parser_free            (H265Parser  * parser);

H265ParserResult h265_parse_vps              (H265NalUnit * nalu,
                                                     H265VPS     * vps);

H265ParserResult h265_parse_sps              (H265Parser  * parser,
                                                     H265NalUnit * nalu,
                                                     H265SPS     * sps,
                                                     bool         parse_vui_params);

H265ParserResult h265_parse_pps              (H265Parser  * parser,
                                                     H265NalUnit * nalu,
                                                     H265PPS     * pps);

bool            h265_slice_hdr_copy (H265SliceHdr       * dst_slice,
                                             const H265SliceHdr * src_slice);

void                h265_slice_hdr_free (H265SliceHdr * slice_hdr);

bool            h265_sei_copy       (H265SEIMessage       * dest_sei,
                                             const H265SEIMessage * src_sei);

void                h265_sei_free       (H265SEIMessage * sei);

void    h265_quant_matrix_4x4_get_zigzag_from_raster (uint8_t out_quant[16],
                                                          const uint8_t quant[16]);

void    h265_quant_matrix_4x4_get_raster_from_zigzag (uint8_t out_quant[16],
                                                          const uint8_t quant[16]);

void    h265_quant_matrix_8x8_get_zigzag_from_raster (uint8_t out_quant[64],
                                                          const uint8_t quant[64]);

void    h265_quant_matrix_8x8_get_raster_from_zigzag (uint8_t out_quant[64],
                                                          const uint8_t quant[64]);

#define h265_quant_matrix_16x16_get_zigzag_from_raster \
        h265_quant_matrix_8x8_get_zigzag_from_raster
#define h265_quant_matrix_16x16_get_raster_from_zigzag \
        h265_quant_matrix_8x8_get_raster_from_zigzag
#define h265_quant_matrix_32x32_get_zigzag_from_raster \
        h265_quant_matrix_8x8_get_zigzag_from_raster
#define h265_quant_matrix_32x32_get_raster_from_zigzag \
        h265_quant_matrix_8x8_get_raster_from_zigzag

G_END_DECLS
#endif
