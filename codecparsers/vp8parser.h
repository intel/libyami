/*
 * vp8parser.h
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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

#ifndef __VP8_PARSER_H__
#define __VP8_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "basictype.h"

#define VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME    3   /* frame-tag */
#define VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME        10  /* frame tag + magic number + frame width/height */
#define VP8_KEY_FRAME                               0   /* section 9.1 of spec: 0 for intra frames, 1 for interframes */

typedef struct Vp8FrameHdr Vp8FrameHdr;
typedef struct Vp8Segmentation Vp8Segmentation;
typedef struct Vp8MbLfAdjustments Vp8MbLfAdjustments;
typedef struct Vp8QuantIndices Vp8QuantIndices;
typedef struct Vp8TokenProbUpdate Vp8TokenProbUpdate;
typedef struct Vp8MvProbUpdate Vp8MvProbUpdate;

/**
 * Vp8ParseResult:
 * @VP8_PARSER_OK: The parsing went well
 * @VP8_PARSER_NO_PACKET_ERROR: An error accured durint the parsing
 *
 * Result type of any parsing function.
 */
typedef enum
{
  VP8_PARSER_OK,
  VP8_PARSER_ERROR,
} Vp8ParseResult;

struct Vp8Segmentation
{
  uint8 segmentation_enabled;
  uint8 update_mb_segmentation_map;
  uint8 update_segment_feature_data;
  uint8 segment_feature_mode;
  int8 quantizer_update_value[4];
  int8 lf_update_value[4];
  int8 segment_prob[3];
};

struct Vp8MbLfAdjustments
{
  uint8 loop_filter_adj_enable;
  uint8 mode_ref_lf_delta_update;
  int8 ref_frame_delta[4];
  int8 mb_mode_delta[4];
};

struct Vp8MvProbUpdate
{
  uint8 prob[2][19];
};

struct Vp8QuantIndices
{
  int8 y_ac_qi;
  int8 y_dc_delta;
  int8 y2_dc_delta;
  int8 y2_ac_delta;
  int8 uv_dc_delta;
  int8 uv_ac_delta;
};

struct Vp8TokenProbUpdate
{
  uint8 coeff_prob[4][8][3][11];
};

/* these fileds impacts all ensuing frame, should be kept by upper level. */
typedef struct
{
  Vp8TokenProbUpdate token_prob_update;
  Vp8MvProbUpdate mv_prob_update;
  Vp8MbLfAdjustments mb_lf_adjust;
  Vp8Segmentation segmentation;
} Vp8MultiFrameData;

typedef struct
{
  uint8 range;
  int remaining_bits;
  uint8 code_word;
  const uint8 *buffer;         /* point to next byte to be read for decoding */
} Vp8RangeDecoderStatus;

struct Vp8FrameHdr
{
  uint32 key_frame;
  uint8 version;
  uint8 show_frame;
  uint32 first_part_size;
  uint32 width;
  uint32 height;
  uint32 horizontal_scale;
  uint32 vertical_scale;

  uint8 color_space;
  uint8 clamping_type;

  uint8 filter_type;
  uint8 loop_filter_level;
  uint8 sharpness_level;
  uint8 log2_nbr_of_dct_partitions;
  uint32 partition_size[8];

  Vp8QuantIndices quant_indices;
  uint8 refresh_golden_frame;
  uint8 refresh_alternate_frame;
  uint8 copy_buffer_to_golden;
  uint8 copy_buffer_to_alternate;
  uint8 sign_bias_golden;
  uint8 sign_bias_alternate;
  uint8 refresh_entropy_probs;
  uint8 refresh_last;

  uint8 mb_no_skip_coeff;
  uint8 prob_skip_false;

  uint8 prob_intra;
  uint8 prob_last;
  uint8 prob_gf;

  /* the following means to intra block in inter frame (non-key frame) */
  uint8 intra_16x16_prob_update_flag;
  uint8 intra_16x16_prob[4];
  uint8 intra_chroma_prob_update_flag;
  uint8 intra_chroma_prob[3];
  Vp8MultiFrameData *multi_frame_data;
  Vp8RangeDecoderStatus rangedecoder_state;
};

Vp8ParseResult
vp8_parse_frame_header (Vp8FrameHdr * frame_hdr, const uint8 * data,
    uint32 offset, uint32 size);

boolean
vp8_parse_init_default_multi_frame_data (Vp8MultiFrameData *
    multi_frame_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VP8_PARSER_H__ */
