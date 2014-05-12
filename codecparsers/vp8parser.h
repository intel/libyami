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

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

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
  uint8_t segmentation_enabled;
  uint8_t update_mb_segmentation_map;
  uint8_t update_segment_feature_data;
  uint8_t segment_feature_mode;
  int8_t quantizer_update_value[4];
  int8_t lf_update_value[4];
  int8_t segment_prob[3];
};

struct Vp8MbLfAdjustments
{
  uint8_t loop_filter_adj_enable;
  uint8_t mode_ref_lf_delta_update;
  int8_t ref_frame_delta[4];
  int8_t mb_mode_delta[4];
};

struct Vp8MvProbUpdate
{
  uint8_t prob[2][19];
};

struct Vp8QuantIndices
{
  int8_t y_ac_qi;
  int8_t y_dc_delta;
  int8_t y2_dc_delta;
  int8_t y2_ac_delta;
  int8_t uv_dc_delta;
  int8_t uv_ac_delta;
};

struct Vp8TokenProbUpdate
{
  uint8_t coeff_prob[4][8][3][11];
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
  uint8_t range;
  int remaining_bits;
  uint8_t code_word;
  const uint8_t *buffer;         /* point to next byte to be read for decoding */
} Vp8RangeDecoderStatus;

struct Vp8FrameHdr
{
  uint32_t key_frame;
  uint8_t version;
  uint8_t show_frame;
  uint32_t first_part_size;
  uint32_t width;
  uint32_t height;
  uint32_t horizontal_scale;
  uint32_t vertical_scale;

  uint8_t color_space;
  uint8_t clamping_type;

  uint8_t filter_type;
  uint8_t loop_filter_level;
  uint8_t sharpness_level;
  uint8_t log2_nbr_of_dct_partitions;
  uint32_t partition_size[8];

  Vp8QuantIndices quant_indices;
  uint8_t refresh_golden_frame;
  uint8_t refresh_alternate_frame;
  uint8_t copy_buffer_to_golden;
  uint8_t copy_buffer_to_alternate;
  uint8_t sign_bias_golden;
  uint8_t sign_bias_alternate;
  uint8_t refresh_entropy_probs;
  uint8_t refresh_last;

  uint8_t mb_no_skip_coeff;
  uint8_t prob_skip_false;

  uint8_t prob_intra;
  uint8_t prob_last;
  uint8_t prob_gf;

  /* the following means to intra block in inter frame (non-key frame) */
  uint8_t intra_16x16_prob_update_flag;
  uint8_t intra_16x16_prob[4];
  uint8_t intra_chroma_prob_update_flag;
  uint8_t intra_chroma_prob[3];
  Vp8MultiFrameData *multi_frame_data;
  Vp8RangeDecoderStatus rangedecoder_state;
};

Vp8ParseResult
vp8_parse_frame_header (Vp8FrameHdr * frame_hdr, const uint8_t * data,
    uint32_t offset, uint32_t size);

bool
vp8_parse_init_default_multi_frame_data (Vp8MultiFrameData *
    multi_frame_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VP8_PARSER_H__ */
