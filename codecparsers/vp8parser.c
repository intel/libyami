/*
 * vp8parser.c - VP8 parser
 *
 * Copyright (C) 2013-2014 Intel Corporation
 *   Author: Halley Zhao <halley.zhao@intel.com>
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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

/**
 * SECTION:vp8parser
 * @short_description: Convenience library for parsing vp8 video bitstream.
 *
 * For more details about the structures, you can refer to the
 * specifications: VP8-rfc6386.pdf
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <string.h>
#include "bytereader.h"
#include "vp8parser.h"
#include "vp8rangedecoder.h"
#include "vp8utils.h"

DEBUG_CATEGORY (vp8_parser_debug);
#define CAT_DEFAULT vp8_parser_debug

#define INITIALIZE_DEBUG_CATEGORY ensure_debug_category ()
static void
ensure_debug_category (void)
{
#ifndef DISABLE_DEBUG
  static size_t is_initialized;

  if (g_once_init_enter (&is_initialized)) {
    DEBUG_CATEGORY_INIT (vp8_parser_debug, "codecparsers_vp8", 0,
        "vp8 parser library");
    g_once_init_leave (&is_initialized, TRUE);
  }
#endif
}

static Vp8MvProbs vp8_mv_update_probs;
static Vp8TokenProbs vp8_token_update_probs;

static void
ensure_prob_tables (void)
{
  static size_t is_initialized;

  if (g_once_init_enter (&is_initialized)) {
    vp8_mv_update_probs_init (&vp8_mv_update_probs);
    vp8_token_update_probs_init (&vp8_token_update_probs);
    g_once_init_leave (&is_initialized, TRUE);
  }
}

#define READ_BOOL(rd, val, field_name) \
  val = vp8_read_bool ((rd))
#define READ_UINT(rd, val, nbits, field_name) \
  val = vp8_read_uint ((rd), (nbits))
#define READ_SINT(rd, val, nbits, field_name) \
  val = vp8_read_sint ((rd), (nbits))

static inline bool
vp8_read_bool (Vp8RangeDecoder * rd)
{
  return (bool) vp8_range_decoder_read_literal (rd, 1);
}

static inline uint32_t
vp8_read_uint (Vp8RangeDecoder * rd, uint32_t nbits)
{
  return (uint32_t) vp8_range_decoder_read_literal (rd, nbits);
}

static inline int32_t
vp8_read_sint (Vp8RangeDecoder * rd, uint32_t nbits)
{
  int32_t v;

  v = vp8_range_decoder_read_literal (rd, nbits);
  if (vp8_range_decoder_read_literal (rd, 1))
    v = -v;
  return v;
}

/* Parse update_segmentation() */
static bool
parse_update_segmentation (Vp8RangeDecoder * rd, Vp8Segmentation * seg)
{
  bool update;
  int32_t i;

  seg->update_mb_segmentation_map = FALSE;
  seg->update_segment_feature_data = FALSE;

  READ_BOOL (rd, seg->segmentation_enabled, "segmentation_enabled");
  if (!seg->segmentation_enabled)
    return TRUE;

  READ_BOOL (rd, seg->update_mb_segmentation_map, "update_mb_segmentation_map");
  READ_BOOL (rd, seg->update_segment_feature_data,
      "update_segment_feature_data");

  if (seg->update_segment_feature_data) {
    READ_UINT (rd, seg->segment_feature_mode, 1, "segment_feature_mode");

    /* quantizer_update_value defaults to zero if update flag is zero
       (Section 9.3, 4.b) */
    for (i = 0; i < 4; i++) {
      READ_BOOL (rd, update, "quantizer_update");
      if (update) {
        READ_SINT (rd, seg->quantizer_update_value[i], 7,
            "quantizer_update_value");
      } else
        seg->quantizer_update_value[i] = 0;
    }

    /* lf_update_value defaults to zero if update flag is zero
       (Section 9.3, 4.b) */
    for (i = 0; i < 4; i++) {
      READ_BOOL (rd, update, "loop_filter_update");
      if (update) {
        READ_SINT (rd, seg->lf_update_value[i], 6, "lf_update_value");
      } else
        seg->lf_update_value[i] = 0;
    }
  }

  /* segment_prob defaults to 255 if update flag is zero
     (Section 9.3, 5) */
  if (seg->update_mb_segmentation_map) {
    for (i = 0; i < 3; i++) {
      READ_BOOL (rd, update, "segment_prob_update");
      if (update) {
        READ_UINT (rd, seg->segment_prob[i], 8, "segment_prob");
      } else
        seg->segment_prob[i] = 255;
    }
  }
  return TRUE;
}

/* Parse mb_lf_adjustments() to update loop filter delta adjustments */
static bool
parse_mb_lf_adjustments (Vp8RangeDecoder * rd, Vp8MbLfAdjustments * adj)
{
  bool update;
  int32_t i;

  adj->mode_ref_lf_delta_update = FALSE;

  READ_BOOL (rd, adj->loop_filter_adj_enable, "loop_filter_adj_enable");
  if (!adj->loop_filter_adj_enable)
    return TRUE;

  READ_BOOL (rd, adj->mode_ref_lf_delta_update, "mode_ref_lf_delta_update");
  if (!adj->mode_ref_lf_delta_update)
    return TRUE;

  for (i = 0; i < 4; i++) {
    READ_BOOL (rd, update, "ref_frame_delta_update_flag");
    if (update) {
      READ_SINT (rd, adj->ref_frame_delta[i], 6, "ref_frame_delta_magniture");
    }
  }

  for (i = 0; i < 4; i++) {
    READ_BOOL (rd, update, "mb_mode_delta_update_flag");
    if (update) {
      READ_SINT (rd, adj->mb_mode_delta[i], 6, "mb_mode_delta_magnitude");
    }
  }
  return TRUE;
}

/* Parse quant_indices() */
static bool
parse_quant_indices (Vp8RangeDecoder * rd, Vp8QuantIndices * qip)
{
  bool update;

  READ_UINT (rd, qip->y_ac_qi, 7, "y_ac_qi");

  READ_BOOL (rd, update, "y_dc_delta_present");
  if (update) {
    READ_SINT (rd, qip->y_dc_delta, 4, "y_dc_delta_magnitude");
  } else
    qip->y_dc_delta = 0;

  READ_BOOL (rd, update, "y2_dc_delta_present");
  if (update) {
    READ_SINT (rd, qip->y2_dc_delta, 4, "y2_dc_delta_magnitude");
  } else
    qip->y2_dc_delta = 0;

  READ_BOOL (rd, update, "y2_ac_delta_present");
  if (update) {
    READ_SINT (rd, qip->y2_ac_delta, 4, "y2_ac_delta_magnitude");
  } else
    qip->y2_ac_delta = 0;

  READ_BOOL (rd, update, "uv_dc_delta_present");
  if (update) {
    READ_SINT (rd, qip->uv_dc_delta, 4, "uv_dc_delta_magnitude");
  } else
    qip->uv_dc_delta = 0;

  READ_BOOL (rd, update, "uv_ac_delta_present");
  if (update) {
    READ_SINT (rd, qip->uv_ac_delta, 4, "uv_ac_delta_magnitude");
  } else
    qip->uv_ac_delta = 0;

  return TRUE;
}

/* Parse token_prob_update() to update persistent token probabilities */
static bool
parse_token_prob_update (Vp8RangeDecoder * rd, Vp8TokenProbs * probs)
{
  int32_t i, j, k, l;
  uint8_t prob;

  for (i = 0; i < 4; i++) {
    for (j = 0; j < 8; j++) {
      for (k = 0; k < 3; k++) {
        for (l = 0; l < 11; l++) {
          if (vp8_range_decoder_read (rd,
                  vp8_token_update_probs.prob[i][j][k][l])) {
            READ_UINT (rd, prob, 8, "token_prob_update");
            probs->prob[i][j][k][l] = prob;
          }
        }
      }
    }
  }
  return TRUE;
}

/* Parse prob_update() to update probabilities used for MV decoding */
static bool
parse_mv_prob_update (Vp8RangeDecoder * rd, Vp8MvProbs * probs)
{
  int32_t i, j;
  uint8_t prob;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 19; j++) {
      if (vp8_range_decoder_read (rd, vp8_mv_update_probs.prob[i][j])) {
        READ_UINT (rd, prob, 7, "mv_prob_update");
        probs->prob[i][j] = prob ? (prob << 1) : 1;
      }
    }
  }
  return TRUE;
}

/* Calculate partition sizes */
static bool
calc_partition_sizes (Vp8FrameHdr * frame_hdr, const uint8_t * data,
    uint32_t size)
{
  const uint32_t num_partitions = 1 << frame_hdr->log2_nbr_of_dct_partitions;
  uint32_t i, ofs, part_size, part_size_ofs = frame_hdr->first_part_size;

  ofs = part_size_ofs + 3 * (num_partitions - 1);
  if (ofs > size) {
    ERROR ("not enough bytes left to parse partition sizes");
    return FALSE;
  }

  /* The size of the last partition is not specified (9.5) */
  for (i = 0; i < num_partitions - 1; i++) {
    part_size = (uint32_t) data[part_size_ofs + 0] |
        ((uint32_t) data[part_size_ofs + 1] << 8) |
        ((uint32_t) data[part_size_ofs + 2] << 16);
    part_size_ofs += 3;

    frame_hdr->partition_size[i] = part_size;
    ofs += part_size;
  }

  if (ofs > size) {
    ERROR ("not enough bytes left to determine the last partition size");
    return FALSE;
  }
  frame_hdr->partition_size[i] = size - ofs;

  while (++i < G_N_ELEMENTS (frame_hdr->partition_size))
    frame_hdr->partition_size[i] = 0;
  return TRUE;
}

/* Parse uncompressed data chunk (19.1) */
static Vp8ParserResult
parse_uncompressed_data_chunk (Vp8Parser * parser, ByteReader * br,
    Vp8FrameHdr * frame_hdr)
{
  uint32_t frame_tag, start_code;
  uint16_t size_code;

  DEBUG ("parsing \"Uncompressed Data Chunk\"");

  if (!byte_reader_get_uint24_le (br, &frame_tag))
    goto error;

  frame_hdr->key_frame = !(frame_tag & 0x01);
  frame_hdr->version = (frame_tag >> 1) & 0x07;
  frame_hdr->show_frame = (frame_tag >> 4) & 0x01;
  frame_hdr->first_part_size = (frame_tag >> 5) & 0x7ffff;

  if (frame_hdr->key_frame) {
    if (!byte_reader_get_uint24_be (br, &start_code))
      goto error;
    if (start_code != 0x9d012a)
      WARNING ("vp8 parser: invalid start code in frame header");

    if (!byte_reader_get_uint16_le (br, &size_code))
      goto error;
    frame_hdr->width = size_code & 0x3fff;
    frame_hdr->horiz_scale_code = size_code >> 14;

    if (!byte_reader_get_uint16_le (br, &size_code)) {
      goto error;
    }
    frame_hdr->height = size_code & 0x3fff;
    frame_hdr->vert_scale_code = (size_code >> 14);

    /* Reset parser state on key frames */
    vp8_parser_init (parser);
  } else {
    frame_hdr->width = 0;
    frame_hdr->height = 0;
    frame_hdr->horiz_scale_code = 0;
    frame_hdr->vert_scale_code = 0;
  }

  /* Calculated values */
  frame_hdr->data_chunk_size = byte_reader_get_pos (br);
  return VP8_PARSER_OK;

error:
  WARNING ("error parsing \"Uncompressed Data Chunk\"");
  return VP8_PARSER_ERROR;
}

/* Parse Frame Header (19.2) */
static Vp8ParserResult
parse_frame_header (Vp8Parser * parser, Vp8RangeDecoder * rd,
    Vp8FrameHdr * frame_hdr)
{
  bool update;
  uint32_t i;

  DEBUG ("parsing \"Frame Header\"");

  if (frame_hdr->key_frame) {
    READ_UINT (rd, frame_hdr->color_space, 1, "color_space");
    READ_UINT (rd, frame_hdr->clamping_type, 1, "clamping_type");
  }

  if (!parse_update_segmentation (rd, &parser->segmentation))
    goto error;

  READ_UINT (rd, frame_hdr->filter_type, 1, "filter_type");
  READ_UINT (rd, frame_hdr->loop_filter_level, 6, "loop_filter_level");
  READ_UINT (rd, frame_hdr->sharpness_level, 3, "sharpness_level");

  if (!parse_mb_lf_adjustments (rd, &parser->mb_lf_adjust))
    goto error;

  READ_UINT (rd, frame_hdr->log2_nbr_of_dct_partitions, 2,
      "log2_nbr_of_dct_partitions");

  if (!parse_quant_indices (rd, &frame_hdr->quant_indices))
    goto error;

  frame_hdr->copy_buffer_to_golden = 0;
  frame_hdr->copy_buffer_to_alternate = 0;
  if (frame_hdr->key_frame) {
    READ_BOOL (rd, frame_hdr->refresh_entropy_probs, "refresh_entropy_probs");

    frame_hdr->refresh_last = TRUE;
    frame_hdr->refresh_golden_frame = TRUE;
    frame_hdr->refresh_alternate_frame = TRUE;

    vp8_mode_probs_init_defaults (&frame_hdr->mode_probs, TRUE);
  } else {
    READ_BOOL (rd, frame_hdr->refresh_golden_frame, "refresh_golden_frame");
    READ_BOOL (rd, frame_hdr->refresh_alternate_frame,
        "refresh_alternate_frame");

    if (!frame_hdr->refresh_golden_frame) {
      READ_UINT (rd, frame_hdr->copy_buffer_to_golden, 2,
          "copy_buffer_to_golden");
    }

    if (!frame_hdr->refresh_alternate_frame) {
      READ_UINT (rd, frame_hdr->copy_buffer_to_alternate, 2,
          "copy_buffer_to_alternate");
    }

    READ_UINT (rd, frame_hdr->sign_bias_golden, 1, "sign_bias_golden");
    READ_UINT (rd, frame_hdr->sign_bias_alternate, 1, "sign_bias_alternate");
    READ_BOOL (rd, frame_hdr->refresh_entropy_probs, "refresh_entropy_probs");
    READ_BOOL (rd, frame_hdr->refresh_last, "refresh_last");

    memcpy (&frame_hdr->mode_probs, &parser->mode_probs,
        sizeof (parser->mode_probs));
  }
  memcpy (&frame_hdr->token_probs, &parser->token_probs,
      sizeof (parser->token_probs));
  memcpy (&frame_hdr->mv_probs, &parser->mv_probs, sizeof (parser->mv_probs));

  if (!parse_token_prob_update (rd, &frame_hdr->token_probs))
    goto error;

  READ_BOOL (rd, frame_hdr->mb_no_skip_coeff, "mb_no_skip_coeff");
  if (frame_hdr->mb_no_skip_coeff) {
    READ_UINT (rd, frame_hdr->prob_skip_false, 8, "prob_skip_false");
  }

  if (!frame_hdr->key_frame) {
    READ_UINT (rd, frame_hdr->prob_intra, 8, "prob_intra");
    READ_UINT (rd, frame_hdr->prob_last, 8, "prob_last");
    READ_UINT (rd, frame_hdr->prob_gf, 8, "prob_gf");

    READ_BOOL (rd, update, "intra_16x16_prob_update_flag");
    if (update) {
      for (i = 0; i < 4; i++) {
        READ_UINT (rd, frame_hdr->mode_probs.y_prob[i], 8, "intra_16x16_prob");
      }
    }

    READ_BOOL (rd, update, "intra_chroma_prob_update_flag");
    if (update) {
      for (i = 0; i < 3; i++) {
        READ_UINT (rd, frame_hdr->mode_probs.uv_prob[i], 8,
            "intra_chroma_prob");
      }
    }

    if (!parse_mv_prob_update (rd, &frame_hdr->mv_probs))
      goto error;
  }

  /* Refresh entropy probabilities */
  if (frame_hdr->refresh_entropy_probs) {
    memcpy (&parser->token_probs, &frame_hdr->token_probs,
        sizeof (frame_hdr->token_probs));
    memcpy (&parser->mv_probs, &frame_hdr->mv_probs,
        sizeof (frame_hdr->mv_probs));
    if (!frame_hdr->key_frame)
      memcpy (&parser->mode_probs, &frame_hdr->mode_probs,
          sizeof (frame_hdr->mode_probs));
  }

  /* Calculated values */
  frame_hdr->header_size = vp8_range_decoder_get_pos (rd);
  return VP8_PARSER_OK;

error:
  WARNING ("error parsing \"Frame Header\"");
  return VP8_PARSER_ERROR;
}

/**** API ****/
/**
 * vp8_parser_init:
 * @parser: The #Vp8Parser to initialize
 *
 * Initializes the supplied @parser structure with its default values.
 *
 * Since: 1.4
 */
void
vp8_parser_init (Vp8Parser * parser)
{
  g_return_if_fail (parser != NULL);

  memset (&parser->segmentation, 0, sizeof (parser->segmentation));
  memset (&parser->mb_lf_adjust, 0, sizeof (parser->mb_lf_adjust));
  vp8_token_probs_init_defaults (&parser->token_probs);
  vp8_mv_probs_init_defaults (&parser->mv_probs);
  vp8_mode_probs_init_defaults (&parser->mode_probs, FALSE);
}

/**
 * vp8_parser_parse_frame_header:
 * @parser: The #Vp8Parser
 * @frame_hdr: The #Vp8FrameHdr to fill
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses the VP8 bitstream contained in @data, and fills in @frame_hdr
 * with the information. The supplied @data shall point to a complete
 * frame since there is no sync code specified for VP8 bitstreams. Thus,
 * the @size argument shall represent the whole frame size.
 *
 * Returns: a #Vp8ParserResult
 *
 * Since: 1.4
 */
Vp8ParserResult
vp8_parser_parse_frame_header (Vp8Parser * parser,
    Vp8FrameHdr * frame_hdr, const uint8_t * data, size_t size)
{
  ByteReader br;
  Vp8RangeDecoder rd;
  Vp8RangeDecoderState rd_state;
  Vp8ParserResult result;

  ensure_debug_category ();
  ensure_prob_tables ();

  g_return_val_if_fail (frame_hdr != NULL, VP8_PARSER_ERROR);
  g_return_val_if_fail (parser != NULL, VP8_PARSER_ERROR);

  /* Uncompressed Data Chunk */
  byte_reader_init (&br, data, size);

  result = parse_uncompressed_data_chunk (parser, &br, frame_hdr);
  if (result != VP8_PARSER_OK)
    return result;

  /* Frame Header */
  if (frame_hdr->data_chunk_size + frame_hdr->first_part_size > size)
    return VP8_PARSER_BROKEN_DATA;

  data += frame_hdr->data_chunk_size;
  size -= frame_hdr->data_chunk_size;
  if (!vp8_range_decoder_init (&rd, data, size))
    return VP8_PARSER_BROKEN_DATA;

  result = parse_frame_header (parser, &rd, frame_hdr);
  if (result != VP8_PARSER_OK)
    return result;

  /* Calculate partition sizes */
  if (!calc_partition_sizes (frame_hdr, data, size))
    return VP8_PARSER_BROKEN_DATA;

  /* Sync range decoder state */
  vp8_range_decoder_get_state (&rd, &rd_state);
  frame_hdr->rd_range = rd_state.range;
  frame_hdr->rd_value = rd_state.value;
  frame_hdr->rd_count = rd_state.count;
  return VP8_PARSER_OK;
}
