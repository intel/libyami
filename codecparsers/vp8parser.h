/*
 * GStreamer
 * Copyright (C) <2013> Intel
 * Copyright (C) <2013> Halley Zhao<halley.zhao@intel.com>
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

#ifndef __VP8UTIL_H__
#define __VP8UTIL_H__

#include "basictype.h"
#include "bitreader.h"
#include "bytereader.h"

typedef struct _Vp8VideoPacketHdr        Vp8VideoPacketHdr;
typedef struct _Vp8Packet                Vp8Packet;

typedef struct _Vp8FrameHdr              Vp8FrameHdr;
typedef struct _Vp8Segmentation          Vp8Segmentation;
typedef struct _Vp8MbLfAdjustments       Vp8MbLfAdjustments;
typedef struct _Vp8QuantIndices          Vp8QuantIndices;
typedef struct _Vp8TokenProbUpdate       Vp8TokenProbUpdate;
typedef struct _Vp8MvProbUpdate          Vp8MvProbUpdate;

/**
 * Vp8ParseResult:
 * @VP8_PARSER_OK: The parsing went well
 * @VP8_PARSER_BROKEN_DATA: The bitstream was broken
 * @VP8_PARSER_NO_PACKET: There was no packet in the buffer
 * @VP8_PARSER_NO_PACKET_END: There was no packet end in the buffer
 * @VP8_PARSER_NO_PACKET_ERROR: An error accured durint the parsing
 *
 * Result type of any parsing function.
 */
typedef enum {
  VP8_PARSER_OK,
  VP8_PARSER_BROKEN_DATA,
  VP8_PARSER_NO_PACKET,
  VP8_PARSER_NO_PACKET_END,
  VP8_PARSER_ERROR,
} Vp8ParseResult;

struct _Vp8Segmentation {
    uint8  update_mb_segmentation_map;
    uint8  update_segment_feature_data;
    uint8  segment_feature_mode;
    // uint8  quantizer_update[4]; // todo, not necessary to roll up
    int8   quantizer_update_value[4];
    // uint8  loop_filter_update[4];
    int8   lf_update_value[4];
    // uint8  segment_prob_update[3];
    int8   segment_prob[3];
};

struct _Vp8MbLfAdjustments {
    uint8  loop_filter_adj_enable;
    uint8  mode_ref_lf_delta_update;   // seems to be redundant in semantic
    // uint8  ref_frame_delta_update_flag[4];
    int8   ref_frame_delta[4];
    // uint8  mb_mode_delta_update_flag[4];
    int8   mb_mode_delta[4];

};

struct _Vp8MvProbUpdate {
    uint8  has_update;
    // uint8  mv_prob_update_flag[2][19];
    uint8    prob[2][19];
};

struct _Vp8QuantIndices {
    int8   y_ac_qi;
    // uint8  y_dc_delta_present;
    int8   y_dc_delta;
    // uint8  y2_dc_delta_present;
    int8   y2_dc_delta;
    // uint8  y_ac_delta_present;
    int8   y2_ac_delta;
    // uint8  uv_dc_delta_present;
    int8   uv_dc_delta;
    // uint8  uv_ac_delta_present;
    int8   uv_ac_delta;
};

struct _Vp8TokenProbUpdate {
    // uint8  has_update;
    uint8  coeff_prob[4][8][3][11];
};

struct _Vp8FrameHdr
{
    uint32   key_frame;
    uint8    version;
    uint8    show_frame;
    uint32   first_part_size;
    uint32   width;
    uint32   height;

    uint8  color_space;
    uint8  clamping_type;

    uint8  segmentation_enabled;
    Vp8Segmentation segmentation;

    uint8  filter_type;
    uint8  loop_filter_level;
    uint8  sharpness_level;
    Vp8MbLfAdjustments mb_lf_adjust;
    uint8  log2_nbr_of_dct_partitions;
    uint32 partition_size[9];

    Vp8QuantIndices quant_indices; 
    uint8  refresh_golden_frame;
    uint8  refresh_alternate_frame;
    uint8  copy_buffer_to_golden;
    uint8  copy_buffer_to_alternate;
    uint8  sign_bias_golden;
    uint8  sign_bias_alternate;
    uint8  refresh_entropy_probs;
    uint8  refresh_last;

    Vp8TokenProbUpdate token_prob_update;
    uint8  mb_no_skip_coeff;
    uint8  prob_skip_false;

    uint8  prob_intra;
    uint8  prob_last;
    uint8  prob_gf;

    uint8  intra_16x16_prob_update_flag;
    uint8  intra_16x16_prob[4];

    uint8  intra_chroma_prob_update_flag;
    uint8  intra_chroma_prob[3];
    Vp8MvProbUpdate      mv_prob_update;
    uint8 *mb_buffer;
    uint32 mb_offset_bits;
};

Vp8ParseResult
vp8_parse_frame_header(Vp8FrameHdr *frame_hdr, const uint8 * data, 
                        uint32 offset, size_t size);

#endif /* __VP8UTIL_H__ */
