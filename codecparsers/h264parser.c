/* h264parser 
 * Copyright (C) <2011> Intel Corporation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * Some bits C-c,C-v'ed and s/4/3 from h264parse and videoparsers/h264parse.c:
 *    Copyright (C) <2010> Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 *    Copyright (C) <2010> Collabora Multimedia
 *    Copyright (C) <2010> Nokia Corporation
 *
 *    (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *    (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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
 * SECTION:h264parser
 * @short_description: Convenience library for h264 video
 * bitstream parsing.
 *
 * It offers you bitstream parsing in AVC mode or not. To identify Nals in a bitstream and
 * parse its headers, you should call:
 * <itemizedlist>
 *   <listitem>
 *      #h264_parser_identify_nalu to identify the following nalu in not AVC bitstreams
 *   </listitem>
 *   <listitem>
 *      #h264_parser_identify_nalu_avc to identify the nalu in AVC bitstreams
 *   </listitem>
 * </itemizedlist>
 *
 * Then, depending on the #H264NalUnitType of the newly parsed #H264NalUnit, you should
 * call the differents functions to parse the structure:
 * <itemizedlist>
 *   <listitem>
 *      From #H264_NAL_SLICE to #H264_NAL_SLICE_IDR: #h264_parser_parse_slice_hdr
 *   </listitem>
 *   <listitem> * #H264_NAL_SEI: #h264_parser_parse_sei
 *   </listitem>
 *   <listitem>
 *      #H264_NAL_SPS: #h264_parser_parse_sps
 *   </listitem>
 *   <listitem>
 *      #H264_NAL_PPS: #h264_parser_parse_pps
 *   </listitem>
 *   <listitem>
 *      Any other: #h264_parser_parse_nal
 *   </listitem>
 * </itemizedlist>
 *
 * Note: You should always call h264_parser_parse_nal if you don't actually need
 * #H264NalUnitType to be parsed for your personnal use, in order to guarantee that the
 * #H264NalParser is always up to date.
 *
 * For more details about the structures, look at the ITU-T H.264 and ISO/IEC 14496-10 – MPEG-4
 * Part 10 specifications, you can download them from:
 *
 * <itemizedlist>
 *   <listitem>
 *     ITU-T H.264: http://www.itu.int/rec/T-REC-H.264
 *   </listitem>
 *   <listitem>
 *     ISO/IEC 14496-10: http://www.iso.org/iso/iso_catalogue/catalogue_tc/catalogue_detail.htm?csnumber=56538
 *   </listitem>
 * </itemizedlist>
 */
#include <string.h>
#include <stdlib.h>
#include "bytereader.h"
#include "bitreader.h"
#include "nalreader.h"
#include "parserutils.h"
#include "log.h"

#include "h264parser.h"

/**** Default scaling_lists according to Table 7-2 *****/
static const uint8_t default_4x4_intra[16] = {
  6, 13, 20, 28,
  13, 20, 28, 32,
  20, 28, 32, 37,
  28, 32, 37, 42
};

static const uint8_t default_4x4_inter[16] = {
  10, 14, 20, 24,
  14, 20, 24, 27,
  20, 24, 27, 30,
  24, 27, 30, 34
};

static const uint8_t default_8x8_intra[64] = {
  6, 10, 13, 16, 18, 23, 25, 27,
  10, 11, 16, 18, 23, 25, 27, 29,
  13, 16, 18, 23, 25, 27, 29, 31,
  16, 18, 23, 25, 27, 29, 31, 33,
  18, 23, 25, 27, 29, 31, 33, 36,
  23, 25, 27, 29, 31, 33, 36, 38,
  25, 27, 29, 31, 33, 36, 38, 40,
  27, 29, 31, 33, 36, 38, 40, 42
};

static const uint8_t default_8x8_inter[64] = {
  9, 13, 15, 17, 19, 21, 22, 24,
  13, 13, 17, 19, 21, 22, 24, 25,
  15, 17, 19, 21, 22, 24, 25, 27,
  17, 19, 21, 22, 24, 25, 27, 28,
  19, 21, 22, 24, 25, 27, 28, 30,
  21, 22, 24, 25, 27, 28, 30, 32,
  22, 24, 25, 27, 28, 30, 32, 33,
  24, 25, 27, 28, 30, 32, 33, 35
};


static const uint8_t zigzag_8x8[64] = {
  0, 1, 8, 16, 9, 2, 3, 10,
  17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};

static const uint8_t zigzag_4x4[16] = {
  0, 1, 4, 8,
  5, 2, 3, 6,
  9, 12, 13, 10,
  7, 11, 14, 15,
};

typedef struct
{
  uint32_t par_n, par_d;
} PAR;

/* Table E-1 - Meaning of sample aspect ratio indicator (1..16) */
static PAR aspect_ratios[17] = {
  {0, 0},
  {1, 1},
  {12, 11},
  {10, 11},
  {16, 11},
  {40, 33},
  {24, 11},
  {20, 11},
  {32, 11},
  {80, 33},
  {18, 11},
  {15, 11},
  {64, 33},
  {160, 99},
  {4, 3},
  {3, 2},
  {2, 1}
};

/* Compute Ceil(Log2(v)) */
/* Derived from branchless code for integer log2(v) from:
   <http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog> */
static uint32_t
ceil_log2 (uint32_t v)
{
  uint32_t r, shift;

  v--;
  r = (v > 0xFFFF) << 4;
  v >>= r;
  shift = (v > 0xFF) << 3;
  v >>= shift;
  r |= shift;
  shift = (v > 0xF) << 2;
  v >>= shift;
  r |= shift;
  shift = (v > 0x3) << 1;
  v >>= shift;
  r |= shift;
  r |= (v >> 1);
  return r + 1;
}

/* nal parser wrapper macro */
#define NAL_CHECK_ALLOWED(val, min, max) { \
  if (val < min || val > max) { \
    LOG_WARNING ("value not in allowed range. value: %d, range %d-%d", \
                     val, min, max); \
    goto error; \
  } \
}

#define NAL_READ_UINT8(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint8 (nr, &val, nbits)) { \
    LOG_WARNING ("failed to read uint8_t, nbits: %d", nbits); \
    goto error; \
  } \
}

#define NAL_READ_UINT16(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint16 (nr, &val, nbits)) { \
  LOG_WARNING ("failed to read uint16_t, nbits: %d", nbits); \
    goto error; \
  } \
}

#define NAL_READ_UINT32(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint32 (nr, &val, nbits)) { \
  LOG_WARNING ("failed to read uint32_t, nbits: %d", nbits); \
    goto error; \
  } \
}

#define NAL_READ_UINT64(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint64 (nr, &val, nbits)) { \
    LOG_WARNING ("failed to read uint32_t, nbits: %d", nbits); \
    goto error; \
  } \
}

#define NAL_READ_UE(nr, val) { \
  if (!nal_reader_get_ue (nr, &val)) { \
    LOG_WARNING ("failed to read UE"); \
    goto error; \
  } \
}

#define NAL_READ_UE_ALLOWED(nr, val, min, max) { \
  uint32_t tmp; \
  NAL_READ_UE (nr, tmp); \
  NAL_CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

#define NAL_READ_SE(nr, val) { \
  if (!nal_reader_get_se (nr, &val)) { \
    LOG_WARNING ("failed to read SE"); \
    goto error; \
  } \
}

#define NAL_READ_SE_ALLOWED(nr, val, min, max) { \
  int32_t tmp; \
  NAL_READ_SE (nr, tmp); \
  NAL_CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

/*****  Utils ****/
#define EXTENDED_SAR 255

static H264SPS *
h264_parser_get_sps (H264NalParser * nalparser, uint8_t sps_id)
{
  H264SPS *sps;

  sps = &nalparser->sps[sps_id];

  if (sps->valid)
    return sps;

  return NULL;
}

static H264PPS *
h264_parser_get_pps (H264NalParser * nalparser, uint8_t pps_id)
{
  H264PPS *pps;

  pps = &nalparser->pps[pps_id];

  if (pps->valid)
    return pps;

  return NULL;
}

static bool
h264_parse_nalu_header (H264NalUnit * nalu)
{
  uint8_t *data = nalu->data + nalu->offset;
  BitReader br;

  if (nalu->size < 1)
    return false;

  nalu->type = (data[0] & 0x1f);
  nalu->ref_idc = (data[0] & 0x60) >> 5;
  nalu->idr_pic_flag = (nalu->type == 5 ? 1 : 0);
  nalu->header_bytes = 1;

  switch (nalu->type) {
    case H264_NAL_PREFIX_UNIT:
    case H264_NAL_SLICE_EXT:
      if (nalu->size < 4)
        return false;
      bit_reader_init (&br, nalu->data + nalu->offset + nalu->header_bytes,
          nalu->size - nalu->header_bytes);
      if ((data[1] & 0x80) == 0) {      /* MVC */
        H264NalUnitExtensionMVC *const mvc = &nalu->extension.mvc;

        bit_reader_skip_unchecked (&br, 1); /* skip the svc_extension_flag */

        nalu->extension_type = H264_NAL_EXTENSION_MVC;
        mvc->non_idr_flag = bit_reader_get_bits_uint8_unchecked (&br, 1);
        mvc->priority_id = bit_reader_get_bits_uint8_unchecked (&br, 6);
        mvc->view_id = bit_reader_get_bits_uint16_unchecked (&br, 10);
        mvc->temporal_id = bit_reader_get_bits_uint8_unchecked (&br, 3);
        mvc->anchor_pic_flag = bit_reader_get_bits_uint8_unchecked (&br, 1);
        mvc->inter_view_flag = bit_reader_get_bits_uint8_unchecked (&br, 1);

        if (!mvc->non_idr_flag)
          nalu->idr_pic_flag = 1;
        else
          nalu->idr_pic_flag = 0;
      }
      nalu->header_bytes += 3;
      break;
    default:
      nalu->extension_type = H264_NAL_EXTENSION_NONE;
      break;
  }

  LOG_DEBUG ("Nal type %u, ref_idc %u", nalu->type, nalu->ref_idc);
  return true;
}

static inline int32_t
scan_for_start_codes (const uint8_t * data, uint32_t size)
{
  ByteReader br;
  byte_reader_init (&br, data, size);

  /* NALU not empty, so we can at least expect 1 (even 2) bytes following sc */
  return byte_reader_masked_scan_uint32 (&br, 0xffffff00, 0x00000100,
      0, size);
}

static bool
h264_parser_more_data (NalReader * nr)
{
  NalReader nr_tmp;
  uint32_t remaining, nbits;
  uint8_t rbsp_stop_one_bit, zero_bits;

  remaining = nal_reader_get_remaining (nr);
  if (remaining == 0)
    return false;

  nr_tmp = *nr;
  nr = &nr_tmp;

  if (!nal_reader_get_bits_uint8 (nr, &rbsp_stop_one_bit, 1))
    return false;
  if (!rbsp_stop_one_bit)
    return true;

  nbits = --remaining % 8;
  while (remaining > 0) {
    if (!nal_reader_get_bits_uint8 (nr, &zero_bits, nbits))
      return false;
    if (zero_bits != 0)
      return true;
    remaining -= nbits;
    nbits = 8;
  }
  return false;
}

/****** Parsing functions *****/

static bool
h264_parse_hrd_parameters (H264HRDParams * hrd, NalReader * nr)
{
  uint32_t sched_sel_idx;

  LOG_DEBUG ("parsing \"HRD Parameters\"");

  NAL_READ_UE_ALLOWED (nr, hrd->cpb_cnt_minus1, 0, 31);
  NAL_READ_UINT8 (nr, hrd->bit_rate_scale, 4);
  NAL_READ_UINT8 (nr, hrd->cpb_size_scale, 4);

  for (sched_sel_idx = 0; sched_sel_idx <= hrd->cpb_cnt_minus1; sched_sel_idx++) {
    NAL_READ_UE (nr, hrd->bit_rate_value_minus1[sched_sel_idx]);
    NAL_READ_UE (nr, hrd->cpb_size_value_minus1[sched_sel_idx]);
    NAL_READ_UINT8 (nr, hrd->cbr_flag[sched_sel_idx], 1);
  }

  NAL_READ_UINT8 (nr, hrd->initial_cpb_removal_delay_length_minus1, 5);
  NAL_READ_UINT8 (nr, hrd->cpb_removal_delay_length_minus1, 5);
  NAL_READ_UINT8 (nr, hrd->dpb_output_delay_length_minus1, 5);
  NAL_READ_UINT8 (nr, hrd->time_offset_length, 5);

  return true;

error:
  LOG_WARNING ("error parsing \"HRD Parameters\"");
  return false;
}

static bool
h264_parse_vui_parameters (H264SPS * sps, NalReader * nr)
{
  H264VUIParams *vui = &sps->vui_parameters;

  LOG_DEBUG ("parsing \"VUI Parameters\"");

  /* set default values for fields that might not be present in the bitstream
     and have valid defaults */
  vui->aspect_ratio_idc = 0;
  vui->video_format = 5;
  vui->video_full_range_flag = 0;
  vui->colour_primaries = 2;
  vui->transfer_characteristics = 2;
  vui->matrix_coefficients = 2;
  vui->chroma_sample_loc_type_top_field = 0;
  vui->chroma_sample_loc_type_bottom_field = 0;
  vui->low_delay_hrd_flag = 0;
  vui->par_n = 0;
  vui->par_d = 0;

  NAL_READ_UINT8 (nr, vui->aspect_ratio_info_present_flag, 1);
  if (vui->aspect_ratio_info_present_flag) {
    NAL_READ_UINT8 (nr, vui->aspect_ratio_idc, 8);
    if (vui->aspect_ratio_idc == EXTENDED_SAR) {
      NAL_READ_UINT16 (nr, vui->sar_width, 16);
      NAL_READ_UINT16 (nr, vui->sar_height, 16);
      vui->par_n = vui->sar_width;
      vui->par_d = vui->sar_height;
    } else if (vui->aspect_ratio_idc <= 16) {
      vui->par_n = aspect_ratios[vui->aspect_ratio_idc].par_n;
      vui->par_d = aspect_ratios[vui->aspect_ratio_idc].par_d;
    }
  }

  NAL_READ_UINT8 (nr, vui->overscan_info_present_flag, 1);
  if (vui->overscan_info_present_flag)
    NAL_READ_UINT8 (nr, vui->overscan_appropriate_flag, 1);

  NAL_READ_UINT8 (nr, vui->video_signal_type_present_flag, 1);
  if (vui->video_signal_type_present_flag) {

    NAL_READ_UINT8 (nr, vui->video_format, 3);
    NAL_READ_UINT8 (nr, vui->video_full_range_flag, 1);
    NAL_READ_UINT8 (nr, vui->colour_description_present_flag, 1);
    if (vui->colour_description_present_flag) {
      NAL_READ_UINT8 (nr, vui->colour_primaries, 8);
      NAL_READ_UINT8 (nr, vui->transfer_characteristics, 8);
      NAL_READ_UINT8 (nr, vui->matrix_coefficients, 8);
    }
  }

  NAL_READ_UINT8 (nr, vui->chroma_loc_info_present_flag, 1);
  if (vui->chroma_loc_info_present_flag) {
    NAL_READ_UE_ALLOWED (nr, vui->chroma_sample_loc_type_top_field, 0, 5);
    NAL_READ_UE_ALLOWED (nr, vui->chroma_sample_loc_type_bottom_field, 0, 5);
  }

  NAL_READ_UINT8 (nr, vui->timing_info_present_flag, 1);
  if (vui->timing_info_present_flag) {
    NAL_READ_UINT32 (nr, vui->num_units_in_tick, 32);
    if (vui->num_units_in_tick == 0)
      LOG_WARNING ("num_units_in_tick = 0 detected in stream "
          "(incompliant to H.264 E.2.1).");

    NAL_READ_UINT32 (nr, vui->time_scale, 32);
    if (vui->time_scale == 0)
      LOG_WARNING ("time_scale = 0 detected in stream "
          "(incompliant to H.264 E.2.1).");

    NAL_READ_UINT8 (nr, vui->fixed_frame_rate_flag, 1);
  }

  NAL_READ_UINT8 (nr, vui->nal_hrd_parameters_present_flag, 1);
  if (vui->nal_hrd_parameters_present_flag) {
    if (!h264_parse_hrd_parameters (&vui->nal_hrd_parameters, nr))
      goto error;
  }

  NAL_READ_UINT8 (nr, vui->vcl_hrd_parameters_present_flag, 1);
  if (vui->vcl_hrd_parameters_present_flag) {
    if (!h264_parse_hrd_parameters (&vui->vcl_hrd_parameters, nr))
      goto error;
  }

  if (vui->nal_hrd_parameters_present_flag ||
      vui->vcl_hrd_parameters_present_flag)
    NAL_READ_UINT8 (nr, vui->low_delay_hrd_flag, 1);

  NAL_READ_UINT8 (nr, vui->pic_struct_present_flag, 1);
  NAL_READ_UINT8 (nr, vui->bitstream_restriction_flag, 1);
  if (vui->bitstream_restriction_flag) {
    NAL_READ_UINT8 (nr, vui->motion_vectors_over_pic_boundaries_flag, 1);
    NAL_READ_UE (nr, vui->max_bytes_per_pic_denom);
    NAL_READ_UE_ALLOWED (nr, vui->max_bits_per_mb_denom, 0, 16);
    NAL_READ_UE_ALLOWED (nr, vui->log2_max_mv_length_horizontal, 0, 16);
    NAL_READ_UE_ALLOWED (nr, vui->log2_max_mv_length_vertical, 0, 16);
    NAL_READ_UE (nr, vui->num_reorder_frames);
    NAL_READ_UE (nr, vui->max_dec_frame_buffering);
  }

  return true;

error:
  LOG_WARNING ("error parsing \"VUI Parameters\"");
  return false;
}

static bool
h264_parser_parse_scaling_list (NalReader * nr,
    uint8_t scaling_lists_4x4[6][16], uint8_t scaling_lists_8x8[6][64],
    const uint8_t fallback_4x4_inter[16], const uint8_t fallback_4x4_intra[16],
    const uint8_t fallback_8x8_inter[64], const uint8_t fallback_8x8_intra[64],
    uint8_t n_lists)
{
  uint32_t i;

  LOG_DEBUG ("parsing scaling lists");

  for (i = 0; i < 12; i++) {
    bool use_default = false;

    if (i < n_lists) {
      uint8_t scaling_list_present_flag;

      NAL_READ_UINT8 (nr, scaling_list_present_flag, 1);
      if (scaling_list_present_flag) {
        uint8_t *scaling_list;
        const uint8_t *scan;
        uint32_t size;
        uint32_t j;
        uint8_t last_scale, next_scale;

        if (i < 6) {
          scaling_list = scaling_lists_4x4[i];
          scan = zigzag_4x4;
          size = 16;
        } else {
          scaling_list = scaling_lists_8x8[i - 6];
          scan = zigzag_8x8;
          size = 64;
        }

        last_scale = 8;
        next_scale = 8;
        for (j = 0; j < size; j++) {
          if (next_scale != 0) {
            int32_t delta_scale;

            NAL_READ_SE (nr, delta_scale);
            next_scale = (last_scale + delta_scale) & 0xff;
          }
          if (j == 0 && next_scale == 0) {
            use_default = true;
            break;
          }
          last_scale = scaling_list[scan[j]] =
              (next_scale == 0) ? last_scale : next_scale;
        }
      } else
        use_default = true;
    } else
      use_default = true;

    if (use_default) {
      switch (i) {
        case 0:
          memcpy (scaling_lists_4x4[0], fallback_4x4_intra, 16);
          break;
        case 1:
          memcpy (scaling_lists_4x4[1], scaling_lists_4x4[0], 16);
          break;
        case 2:
          memcpy (scaling_lists_4x4[2], scaling_lists_4x4[1], 16);
          break;
        case 3:
          memcpy (scaling_lists_4x4[3], fallback_4x4_inter, 16);
          break;
        case 4:
          memcpy (scaling_lists_4x4[4], scaling_lists_4x4[3], 16);
          break;
        case 5:
          memcpy (scaling_lists_4x4[5], scaling_lists_4x4[4], 16);
          break;
        case 6:
          memcpy (scaling_lists_8x8[0], fallback_8x8_intra, 64);
          break;
        case 7:
          memcpy (scaling_lists_8x8[1], fallback_8x8_inter, 64);
          break;
        case 8:
          memcpy (scaling_lists_8x8[2], scaling_lists_8x8[0], 64);
          break;
        case 9:
          memcpy (scaling_lists_8x8[3], scaling_lists_8x8[1], 64);
          break;
        case 10:
          memcpy (scaling_lists_8x8[4], scaling_lists_8x8[2], 64);
          break;
        case 11:
          memcpy (scaling_lists_8x8[5], scaling_lists_8x8[3], 64);
          break;

        default:
          break;
      }
    }
  }

  return true;

error:
  LOG_WARNING ("error parsing scaling lists");
  return false;
}

static bool
slice_parse_ref_pic_list_modification_1 (H264SliceHdr * slice,
    NalReader * nr, uint32_t list, bool is_mvc)
{
  H264RefPicListModification *entries;
  uint8_t *ref_pic_list_modification_flag, *n_ref_pic_list_modification;
  uint32_t modification_of_pic_nums_idc;
  uint32_t i = 0;

  if (list == 0) {
    entries = slice->ref_pic_list_modification_l0;
    ref_pic_list_modification_flag = &slice->ref_pic_list_modification_flag_l0;
    n_ref_pic_list_modification = &slice->n_ref_pic_list_modification_l0;
  } else {
    entries = slice->ref_pic_list_modification_l1;
    ref_pic_list_modification_flag = &slice->ref_pic_list_modification_flag_l1;
    n_ref_pic_list_modification = &slice->n_ref_pic_list_modification_l1;
  }

  NAL_READ_UINT8 (nr, *ref_pic_list_modification_flag, 1);
  if (*ref_pic_list_modification_flag) {
    while (1) {
      NAL_READ_UE (nr, modification_of_pic_nums_idc);
      if (modification_of_pic_nums_idc == 3)
        break;
      if (modification_of_pic_nums_idc == 0 ||
          modification_of_pic_nums_idc == 1) {
        NAL_READ_UE_ALLOWED (nr, entries[i].value.abs_diff_pic_num_minus1, 0,
            slice->max_pic_num - 1);
      } else if (modification_of_pic_nums_idc == 2) {
        NAL_READ_UE (nr, entries[i].value.long_term_pic_num);
      } else if (is_mvc && (modification_of_pic_nums_idc == 4 ||
              modification_of_pic_nums_idc == 5)) {
        NAL_READ_UE (nr, entries[i].value.abs_diff_view_idx_minus1);
      } else
        continue;
      entries[i++].modification_of_pic_nums_idc = modification_of_pic_nums_idc;
    }
  }
  *n_ref_pic_list_modification = i;
  return true;

error:
  LOG_WARNING ("error parsing \"Reference picture list %u modification\"", list);
  return false;
}

static bool
slice_parse_ref_pic_list_modification (H264SliceHdr * slice, NalReader * nr,
    bool is_mvc)
{
  if (!H264_IS_I_SLICE (slice) && !H264_IS_SI_SLICE (slice)) {
    if (!slice_parse_ref_pic_list_modification_1 (slice, nr, 0, is_mvc))
      return false;
  }

  if (H264_IS_B_SLICE (slice)) {
    if (!slice_parse_ref_pic_list_modification_1 (slice, nr, 1, is_mvc))
      return false;
  }
  return true;
}

static bool
h264_slice_parse_dec_ref_pic_marking (H264SliceHdr * slice,
    H264NalUnit * nalu, NalReader * nr)
{
  H264DecRefPicMarking *dec_ref_pic_m;

  LOG_DEBUG ("parsing \"Decoded reference picture marking\"");

  dec_ref_pic_m = &slice->dec_ref_pic_marking;

  if (nalu->idr_pic_flag) {
    NAL_READ_UINT8 (nr, dec_ref_pic_m->no_output_of_prior_pics_flag, 1);
    NAL_READ_UINT8 (nr, dec_ref_pic_m->long_term_reference_flag, 1);
  } else {
    NAL_READ_UINT8 (nr, dec_ref_pic_m->adaptive_ref_pic_marking_mode_flag, 1);
    if (dec_ref_pic_m->adaptive_ref_pic_marking_mode_flag) {
      uint32_t mem_mgmt_ctrl_op;
      H264RefPicMarking *refpicmarking;

      dec_ref_pic_m->n_ref_pic_marking = 0;
      while (1) {
        refpicmarking =
            &dec_ref_pic_m->ref_pic_marking[dec_ref_pic_m->n_ref_pic_marking];

        NAL_READ_UE (nr, mem_mgmt_ctrl_op);
        if (mem_mgmt_ctrl_op == 0)
          break;

        refpicmarking->memory_management_control_operation = mem_mgmt_ctrl_op;

        if (mem_mgmt_ctrl_op == 1 || mem_mgmt_ctrl_op == 3)
          NAL_READ_UE (nr, refpicmarking->difference_of_pic_nums_minus1);

        if (mem_mgmt_ctrl_op == 2)
          NAL_READ_UE (nr, refpicmarking->long_term_pic_num);

        if (mem_mgmt_ctrl_op == 3 || mem_mgmt_ctrl_op == 6)
          NAL_READ_UE (nr, refpicmarking->long_term_frame_idx);

        if (mem_mgmt_ctrl_op == 4)
          NAL_READ_UE (nr, refpicmarking->max_long_term_frame_idx_plus1);

        dec_ref_pic_m->n_ref_pic_marking++;
      }
    }
  }

  return true;

error:
  LOG_WARNING ("error parsing \"Decoded reference picture marking\"");
  return false;
}

static bool
h264_slice_parse_pred_weight_table (H264SliceHdr * slice,
    NalReader * nr, uint8_t chroma_array_type)
{
  H264PredWeightTable *p;
  int16_t default_luma_weight, default_chroma_weight;
  int32_t i;

  LOG_DEBUG ("parsing \"Prediction weight table\"");

  p = &slice->pred_weight_table;

  NAL_READ_UE_ALLOWED (nr, p->luma_log2_weight_denom, 0, 7);
  /* set default values */
  default_luma_weight = 1 << p->luma_log2_weight_denom;
  for (i = 0; i < ARRAY_N_ELEMENT(p->luma_weight_l0); i++)
    p->luma_weight_l0[i] = default_luma_weight;
  memset (p->luma_offset_l0, 0, sizeof (p->luma_offset_l0));
  if (H264_IS_B_SLICE (slice)) {
    for (i = 0; i < sizeof (p->luma_weight_l1) / sizeof (p->luma_weight_l1[0]); i++)
      p->luma_weight_l1[i] = default_luma_weight;
    memset (p->luma_offset_l1, 0, sizeof (p->luma_offset_l1));
  }

  if (chroma_array_type != 0) {
    NAL_READ_UE_ALLOWED (nr, p->chroma_log2_weight_denom, 0, 7);
    /* set default values */
    default_chroma_weight = 1 << p->chroma_log2_weight_denom;
    for (i = 0; i < ARRAY_N_ELEMENT (p->chroma_weight_l0); i++) {
      p->chroma_weight_l0[i][0] = default_chroma_weight;
      p->chroma_weight_l0[i][1] = default_chroma_weight;
    }
    memset (p->chroma_offset_l0, 0, sizeof (p->chroma_offset_l0));
    if (H264_IS_B_SLICE (slice)) {
      for (i = 0; i < ARRAY_N_ELEMENT (p->chroma_weight_l1); i++) {
        p->chroma_weight_l1[i][0] = default_chroma_weight;
        p->chroma_weight_l1[i][1] = default_chroma_weight;
      }
      memset (p->chroma_offset_l1, 0, sizeof (p->chroma_offset_l1));
    }
  }

  for (i = 0; i <= slice->num_ref_idx_l0_active_minus1; i++) {
    uint8_t luma_weight_l0_flag;

    NAL_READ_UINT8 (nr, luma_weight_l0_flag, 1);
    if (luma_weight_l0_flag) {
      NAL_READ_SE_ALLOWED (nr, p->luma_weight_l0[i], -128, 127);
      NAL_READ_SE_ALLOWED (nr, p->luma_offset_l0[i], -128, 127);
    }
    if (chroma_array_type != 0) {
      uint8_t chroma_weight_l0_flag;
      int32_t j;

      NAL_READ_UINT8 (nr, chroma_weight_l0_flag, 1);
      if (chroma_weight_l0_flag) {
        for (j = 0; j < 2; j++) {
          NAL_READ_SE_ALLOWED (nr, p->chroma_weight_l0[i][j], -128, 127);
          NAL_READ_SE_ALLOWED (nr, p->chroma_offset_l0[i][j], -128, 127);
        }
      }
    }
  }

  if (H264_IS_B_SLICE (slice)) {
    for (i = 0; i <= slice->num_ref_idx_l1_active_minus1; i++) {
      uint8_t luma_weight_l1_flag;

      NAL_READ_UINT8 (nr, luma_weight_l1_flag, 1);
      if (luma_weight_l1_flag) {
        NAL_READ_SE_ALLOWED (nr, p->luma_weight_l1[i], -128, 127);
        NAL_READ_SE_ALLOWED (nr, p->luma_offset_l1[i], -128, 127);
      }
      if (chroma_array_type != 0) {
        uint8_t chroma_weight_l1_flag;
        int32_t j;

        NAL_READ_UINT8 (nr, chroma_weight_l1_flag, 1);
        if (chroma_weight_l1_flag) {
          for (j = 0; j < 2; j++) {
            NAL_READ_SE_ALLOWED (nr, p->chroma_weight_l1[i][j], -128, 127);
            NAL_READ_SE_ALLOWED (nr, p->chroma_offset_l1[i][j], -128, 127);
          }
        }
      }
    }
  }

  return true;

error:
  LOG_WARNING ("error parsing \"Prediction weight table\"");
  return false;
}

static bool
h264_parser_parse_buffering_period (H264NalParser * nalparser,
    H264BufferingPeriod * per, NalReader * nr)
{
  H264SPS *sps;
  uint8_t sps_id;

  LOG_DEBUG ("parsing \"Buffering period\"");

  NAL_READ_UE_ALLOWED (nr, sps_id, 0, H264_MAX_SPS_COUNT - 1);
  sps = h264_parser_get_sps (nalparser, sps_id);
  if (!sps) {
    LOG_WARNING ("couldn't find associated sequence parameter set with id: %d",
        sps_id);
    return H264_PARSER_BROKEN_LINK;
  }
  per->sps = sps;

  if (sps->vui_parameters_present_flag) {
    H264VUIParams *vui = &sps->vui_parameters;

    if (vui->nal_hrd_parameters_present_flag) {
      H264HRDParams *hrd = &vui->nal_hrd_parameters;
      uint8_t sched_sel_idx;

      for (sched_sel_idx = 0; sched_sel_idx <= hrd->cpb_cnt_minus1;
          sched_sel_idx++) {
        NAL_READ_UINT8 (nr, per->nal_initial_cpb_removal_delay[sched_sel_idx], 5);
        NAL_READ_UINT8 (nr,
            per->nal_initial_cpb_removal_delay_offset[sched_sel_idx], 5);
      }
    }

    if (vui->vcl_hrd_parameters_present_flag) {
      H264HRDParams *hrd = &vui->vcl_hrd_parameters;
      uint8_t sched_sel_idx;

      for (sched_sel_idx = 0; sched_sel_idx <= hrd->cpb_cnt_minus1;
          sched_sel_idx++) {
        NAL_READ_UINT8 (nr, per->vcl_initial_cpb_removal_delay[sched_sel_idx], 5);
        NAL_READ_UINT8 (nr,
            per->vcl_initial_cpb_removal_delay_offset[sched_sel_idx], 5);
      }
    }
  }

  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Buffering period\"");
  return H264_PARSER_ERROR;
}

static bool
h264_parse_clock_timestamp (H264ClockTimestamp * tim,
    H264VUIParams * vui, NalReader * nr)
{
  uint8_t full_timestamp_flag;
  uint8_t time_offset_length;

  LOG_DEBUG ("parsing \"Clock timestamp\"");

  /* defalt values */
  tim->time_offset = 0;

  NAL_READ_UINT8 (nr, tim->ct_type, 2);
  NAL_READ_UINT8 (nr, tim->nuit_field_based_flag, 1);
  NAL_READ_UINT8 (nr, tim->counting_type, 5);
  NAL_READ_UINT8 (nr, full_timestamp_flag, 1);
  NAL_READ_UINT8 (nr, tim->discontinuity_flag, 1);
  NAL_READ_UINT8 (nr, tim->cnt_dropped_flag, 1);
  NAL_READ_UINT8 (nr, tim->n_frames, 8);

  if (full_timestamp_flag) {
    tim->seconds_flag = true;
    NAL_READ_UINT8 (nr, tim->seconds_value, 6);

    tim->minutes_flag = true;
    NAL_READ_UINT8 (nr, tim->minutes_value, 6);

    tim->hours_flag = true;
    NAL_READ_UINT8 (nr, tim->hours_value, 5);
  } else {
    NAL_READ_UINT8 (nr, tim->seconds_flag, 1);
    if (tim->seconds_flag) {
      NAL_READ_UINT8 (nr, tim->seconds_value, 6);
      NAL_READ_UINT8 (nr, tim->minutes_flag, 1);
      if (tim->minutes_flag) {
        NAL_READ_UINT8 (nr, tim->minutes_value, 6);
        NAL_READ_UINT8 (nr, tim->hours_flag, 1);
        if (tim->hours_flag)
          NAL_READ_UINT8 (nr, tim->hours_value, 5);
      }
    }
  }

  time_offset_length = 0;
  if (vui->nal_hrd_parameters_present_flag)
    time_offset_length = vui->nal_hrd_parameters.time_offset_length;
  else if (vui->vcl_hrd_parameters_present_flag)
    time_offset_length = vui->vcl_hrd_parameters.time_offset_length;

  if (time_offset_length > 0)
    NAL_READ_UINT32 (nr, tim->time_offset, time_offset_length);

  return true;

error:
  LOG_WARNING ("error parsing \"Clock timestamp\"");
  return false;
}

static bool
h264_parser_parse_pic_timing (H264NalParser * nalparser,
    H264PicTiming * tim, NalReader * nr)
{
  LOG_DEBUG ("parsing \"Picture timing\"");
  if (!nalparser->last_sps || !nalparser->last_sps->valid) {
    LOG_WARNING ("didn't get the associated sequence paramater set for the "
        "current access unit");
    goto error;
  }

  /* default values */
  memset (tim->clock_timestamp_flag, 0, 3);

  if (nalparser->last_sps->vui_parameters_present_flag) {
    H264VUIParams *vui = &nalparser->last_sps->vui_parameters;

    if (vui->nal_hrd_parameters_present_flag) {
      NAL_READ_UINT32 (nr, tim->cpb_removal_delay,
          vui->nal_hrd_parameters.cpb_removal_delay_length_minus1 + 1);
      NAL_READ_UINT32 (nr, tim->dpb_output_delay,
          vui->nal_hrd_parameters.dpb_output_delay_length_minus1 + 1);
    } else if (vui->nal_hrd_parameters_present_flag) {
      NAL_READ_UINT32 (nr, tim->cpb_removal_delay,
          vui->vcl_hrd_parameters.cpb_removal_delay_length_minus1 + 1);
      NAL_READ_UINT32 (nr, tim->dpb_output_delay,
          vui->vcl_hrd_parameters.dpb_output_delay_length_minus1 + 1);
    }

    if (vui->pic_struct_present_flag) {
      const uint8_t num_clock_ts_table[9] = {
        1, 1, 1, 2, 2, 3, 3, 2, 3
      };
      uint8_t num_clock_num_ts;
      uint32_t i;

      tim->pic_struct_present_flag = true;
      NAL_READ_UINT8 (nr, tim->pic_struct, 4);
      NAL_CHECK_ALLOWED ((int8_t) tim->pic_struct, 0, 8);

      num_clock_num_ts = num_clock_ts_table[tim->pic_struct];
      for (i = 0; i < num_clock_num_ts; i++) {
        NAL_READ_UINT8 (nr, tim->clock_timestamp_flag[i], 1);
        if (tim->clock_timestamp_flag[i]) {
          if (!h264_parse_clock_timestamp (&tim->clock_timestamp[i], vui,
                  nr))
            goto error;
        }
      }
    }
  }

  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Picture timing\"");
  return H264_PARSER_ERROR;
}

/******** API *************/

/**
 * h264_nal_parser_new:
 *
 * Creates a new #H264NalParser. It should be freed with
 * h264_nal_parser_free after use.
 *
 * Returns: a new #H264NalParser
 */
H264NalParser *
h264_nal_parser_new (void)
{
  H264NalParser *nalparser;

  nalparser = (H264NalParser*) malloc (sizeof(H264NalParser));

  return nalparser;
}

/**
 * h264_nal_parser_free:
 * @nalparser: the #H264NalParser to free
 *
 * Frees @nalparser and sets it to %NULL
 */
void
h264_nal_parser_free (H264NalParser * nalparser)
{
  uint32_t i;

  for (i = 0; i < H264_MAX_SPS_COUNT; i++)
    h264_sps_free_1 (&nalparser->sps[i]);

  free (nalparser);
  nalparser = NULL;
}

/**
 * h264_parser_identify_nalu_unchecked:
 * @nalparser: a #H264NalParser
 * @data: The data to parse
 * @offset: the offset from which to parse @data
 * @size: the size of @data
 * @nalu: The #H264NalUnit where to store parsed nal headers
 *
 * Parses @data and fills @nalu from the next nalu data from @data.
 *
 * This differs from @h264_parser_identify_nalu in that it doesn't
 * check whether the packet is complete or not.
 *
 * Note: Only use this function if you already know the provided @data
 * is a complete NALU, else use @h264_parser_identify_nalu.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_identify_nalu_unchecked (H264NalParser * nalparser,
    const uint8_t * data, uint32_t offset, size_t size, H264NalUnit * nalu)
{
  int32_t off1;

  if (size < offset + 4) {
    LOG_DEBUG ("Can't parse, buffer has too small size %d, offset %d", size, offset);
    return H264_PARSER_ERROR;
  }

  off1 = scan_for_start_codes (data + offset, size - offset);

  if (off1 < 0) {
    LOG_DEBUG ("No start code prefix in this buffer");
    return H264_PARSER_NO_NAL;
  }

  if (offset + off1 == size - 1) {
    LOG_DEBUG ("Missing data to identify nal unit");

    return H264_PARSER_ERROR;
  }

  nalu->sc_offset = offset + off1;

  /* sc might have 2 or 3 0-bytes */
  if (nalu->sc_offset > 0 && data[nalu->sc_offset - 1] == 00)
    nalu->sc_offset--;

  nalu->offset = offset + off1 + 3;
  nalu->data = (uint8_t *) data;
  nalu->size = size - nalu->offset;

  if (!h264_parse_nalu_header (nalu)) {
    LOG_WARNING ("error parsing \"NAL unit header\"");
    nalu->size = 0;
    return H264_PARSER_BROKEN_DATA;
  }

  nalu->valid = true;

  if (nalu->type == H264_NAL_SEQ_END ||
      nalu->type == H264_NAL_STREAM_END) {
    LOG_DEBUG ("end-of-seq or end-of-stream nal found");
    nalu->size = 0;
    return H264_PARSER_OK;
  }

  return H264_PARSER_OK;
}

/**
 * h264_parser_identify_nalu:
 * @nalparser: a #H264NalParser
 * @data: The data to parse
 * @offset: the offset from which to parse @data
 * @size: the size of @data
 * @nalu: The #H264NalUnit where to store parsed nal headers
 *
 * Parses @data and fills @nalu from the next nalu data from @data
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_identify_nalu (H264NalParser * nalparser,
    const uint8_t * data, uint32_t offset, size_t size, H264NalUnit * nalu)
{
  H264ParserResult res;
  int32_t off2;

  res =
      h264_parser_identify_nalu_unchecked (nalparser, data, offset, size,
      nalu);

  if (res != H264_PARSER_OK || nalu->size == 0)
    goto beach;

  off2 = scan_for_start_codes (data + nalu->offset, size - nalu->offset);
  if (off2 < 0) {
    LOG_DEBUG ("Nal start %d, No end found", nalu->offset);

    return H264_PARSER_NO_NAL_END;
  }

  if (off2 > 0 && data[nalu->offset + off2 - 1] == 00)
    off2--;

  nalu->size = off2;
  if (nalu->size < 2)
    return H264_PARSER_BROKEN_DATA;

  LOG_DEBUG ("Complete nal found. Off: %d, Size: %d", nalu->offset, nalu->size);

beach:
  return res;
}


/**
 * h264_parser_identify_nalu_avc:
 * @nalparser: a #H264NalParser
 * @data: The data to parse, must be the beging of the Nal unit
 * @offset: the offset from which to parse @data
 * @size: the size of @data
 * @nal_length_size: the size in bytes of the AVC nal length prefix.
 * @nalu: The #H264NalUnit where to store parsed nal headers
 *
 * Parses @data and sets @nalu.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_identify_nalu_avc (H264NalParser * nalparser,
    const uint8_t * data, uint32_t offset, size_t size, uint8_t nal_length_size,
    H264NalUnit * nalu)
{
  BitReader br;

  if (size < offset + nal_length_size) {
    LOG_DEBUG ("Can't parse, buffer has too small size %d,  offset %d", size, offset);
    return H264_PARSER_ERROR;
  }

  size = size - offset;
  bit_reader_init (&br, data + offset, size);

  nalu->size = bit_reader_get_bits_uint32_unchecked (&br,
      nal_length_size * 8);
  nalu->sc_offset = offset;
  nalu->offset = offset + nal_length_size;

  if (size < nalu->size + nal_length_size) {
    nalu->size = 0;

    return H264_PARSER_NO_NAL_END;
  }

  nalu->data = (uint8_t *) data;

  if (!h264_parse_nalu_header (nalu)) {
    LOG_WARNING ("error parsing \"NAL unit header\"");
    nalu->size = 0;
    return H264_PARSER_BROKEN_DATA;
  }

  nalu->valid = true;

  return H264_PARSER_OK;
}

/**
 * h264_parser_parse_nal:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264NalUnit to parse
 *
 * This function should be called in the case one doesn't need to
 * parse a specific structure. It is necessary to do so to make
 * sure @nalparser is up to date.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_nal (H264NalParser * nalparser, H264NalUnit * nalu)
{
  H264SPS sps;
  H264PPS pps;

  switch (nalu->type) {
    case H264_NAL_SPS:
      return h264_parser_parse_sps (nalparser, nalu, &sps, false);
      break;
    case H264_NAL_PPS:
      return h264_parser_parse_pps (nalparser, nalu, &pps);
  }

  return H264_PARSER_OK;
}

/**
 * h264_parser_parse_sps:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_SPS #H264NalUnit to parse
 * @sps: The #H264SPS to fill.
 * @parse_vui_params: Whether to parse the vui_params or not
 *
 * Parses @data, and fills the @sps structure.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_sps (H264NalParser * nalparser, H264NalUnit * nalu,
    H264SPS * sps, bool parse_vui_params)
{
  H264ParserResult res = h264_parse_sps (nalu, sps, parse_vui_params);

  if (res == H264_PARSER_OK) {
    LOG_DEBUG ("adding sequence parameter set with id: %d to array", sps->id);

    if (!h264_sps_copy (&nalparser->sps[sps->id], sps))
      return H264_PARSER_ERROR;
    nalparser->last_sps = &nalparser->sps[sps->id];
  }
  return res;
}

/* Parse seq_parameter_set_data() */
static bool
h264_parse_sps_data (NalReader * nr, H264SPS * sps,
    bool parse_vui_params)
{
  int32_t width, height;
  uint8_t frame_cropping_flag;
  uint32_t subwc[] = { 1, 2, 2, 1 };
  uint32_t subhc[] = { 1, 2, 1, 1 };
  H264VUIParams *vui = NULL;

  /* set default values for fields that might not be present in the bitstream
     and have valid defaults */
  sps->extension_type = H264_NAL_EXTENSION_NONE;
  sps->chroma_format_idc = 1;
  sps->separate_colour_plane_flag = 0;
  sps->bit_depth_luma_minus8 = 0;
  sps->bit_depth_chroma_minus8 = 0;
  memset (sps->scaling_lists_4x4, 16, 96);
  memset (sps->scaling_lists_8x8, 16, 384);
  memset (&sps->vui_parameters, 0, sizeof (sps->vui_parameters));
  sps->mb_adaptive_frame_field_flag = 0;
  sps->frame_crop_left_offset = 0;
  sps->frame_crop_right_offset = 0;
  sps->frame_crop_top_offset = 0;
  sps->frame_crop_bottom_offset = 0;
  sps->delta_pic_order_always_zero_flag = 0;

  NAL_READ_UINT8 (nr, sps->profile_idc, 8);
  NAL_READ_UINT8 (nr, sps->constraint_set0_flag, 1);
  NAL_READ_UINT8 (nr, sps->constraint_set1_flag, 1);
  NAL_READ_UINT8 (nr, sps->constraint_set2_flag, 1);
  NAL_READ_UINT8 (nr, sps->constraint_set3_flag, 1);
  NAL_READ_UINT8 (nr, sps->constraint_set3_flag, 1);
  NAL_READ_UINT8 (nr, sps->constraint_set4_flag, 1);

  /* skip reserved_zero_2bits */
  if (!nal_reader_skip (nr, 2))
    goto error;

  NAL_READ_UINT8 (nr, sps->level_idc, 8);

  NAL_READ_UE_ALLOWED (nr, sps->id, 0, H264_MAX_SPS_COUNT - 1);

  if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
      sps->profile_idc == 122 || sps->profile_idc == 244 ||
      sps->profile_idc == 44 || sps->profile_idc == 83 ||
      sps->profile_idc == 86 || sps->profile_idc == 118 ||
      sps->profile_idc == 128) {
    NAL_READ_UE_ALLOWED (nr, sps->chroma_format_idc, 0, 3);
    if (sps->chroma_format_idc == 3)
      NAL_READ_UINT8 (nr, sps->separate_colour_plane_flag, 1);

    NAL_READ_UE_ALLOWED (nr, sps->bit_depth_luma_minus8, 0, 6);
    NAL_READ_UE_ALLOWED (nr, sps->bit_depth_chroma_minus8, 0, 6);
    NAL_READ_UINT8 (nr, sps->qpprime_y_zero_transform_bypass_flag, 1);

    NAL_READ_UINT8 (nr, sps->scaling_matrix_present_flag, 1);
    if (sps->scaling_matrix_present_flag) {
      uint8_t n_lists;

      n_lists = (sps->chroma_format_idc != 3) ? 8 : 12;
      if (!h264_parser_parse_scaling_list (nr,
              sps->scaling_lists_4x4, sps->scaling_lists_8x8,
              default_4x4_inter, default_4x4_intra,
              default_8x8_inter, default_8x8_intra, n_lists))
        goto error;
    }
  }

  NAL_READ_UE_ALLOWED (nr, sps->log2_max_frame_num_minus4, 0, 12);

  sps->max_frame_num = 1 << (sps->log2_max_frame_num_minus4 + 4);

  NAL_READ_UE_ALLOWED (nr, sps->pic_order_cnt_type, 0, 2);
  if (sps->pic_order_cnt_type == 0) {
    NAL_READ_UE_ALLOWED (nr, sps->log2_max_pic_order_cnt_lsb_minus4, 0, 12);
  } else if (sps->pic_order_cnt_type == 1) {
    uint32_t i;

    NAL_READ_UINT8 (nr, sps->delta_pic_order_always_zero_flag, 1);
    NAL_READ_SE (nr, sps->offset_for_non_ref_pic);
    NAL_READ_SE (nr, sps->offset_for_top_to_bottom_field);
    NAL_READ_UE_ALLOWED (nr, sps->num_ref_frames_in_pic_order_cnt_cycle, 0, 255);

    for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
      NAL_READ_SE (nr, sps->offset_for_ref_frame[i]);
  }

  NAL_READ_UE (nr, sps->num_ref_frames);
  NAL_READ_UINT8 (nr, sps->gaps_in_frame_num_value_allowed_flag, 1);
  NAL_READ_UE (nr, sps->pic_width_in_mbs_minus1);
  NAL_READ_UE (nr, sps->pic_height_in_map_units_minus1);
  NAL_READ_UINT8 (nr, sps->frame_mbs_only_flag, 1);

  if (!sps->frame_mbs_only_flag)
    NAL_READ_UINT8 (nr, sps->mb_adaptive_frame_field_flag, 1);

  NAL_READ_UINT8 (nr, sps->direct_8x8_inference_flag, 1);
  NAL_READ_UINT8 (nr, frame_cropping_flag, 1);
  if (frame_cropping_flag) {
    NAL_READ_UE (nr, sps->frame_crop_left_offset);
    NAL_READ_UE (nr, sps->frame_crop_right_offset);
    NAL_READ_UE (nr, sps->frame_crop_top_offset);
    NAL_READ_UE (nr, sps->frame_crop_bottom_offset);
  }

  NAL_READ_UINT8 (nr, sps->vui_parameters_present_flag, 1);
  if (sps->vui_parameters_present_flag && parse_vui_params) {
    if (!h264_parse_vui_parameters (sps, nr))
      goto error;
    vui = &sps->vui_parameters;
  }

  /* calculate ChromaArrayType */
  if (sps->separate_colour_plane_flag)
    sps->chroma_array_type = 0;
  else
    sps->chroma_array_type = sps->chroma_format_idc;

  /* Calculate  width and height */
  width = (sps->pic_width_in_mbs_minus1 + 1);
  width *= 16;
  height = (sps->pic_height_in_map_units_minus1 + 1);
  height *= 16 * (2 - sps->frame_mbs_only_flag);
  LOG_INFO("initial width=%d, height=%d", width, height);

  width -= (sps->frame_crop_left_offset + sps->frame_crop_right_offset)
      * subwc[sps->chroma_format_idc];
  height -= (sps->frame_crop_top_offset + sps->frame_crop_bottom_offset
      * subhc[sps->chroma_format_idc] * (2 - sps->frame_mbs_only_flag));
  if (width < 0 || height < 0) {
    LOG_WARNING ("invalid width/height in SPS");
    goto error;
  }
  LOG_INFO("final width=%u, height=%u", width, height);
  sps->width = width;
  sps->height = height;

  sps->fps_num = 0;
  sps->fps_den = 1;

  if (vui && vui->timing_info_present_flag) {
    /* derive framerate */
    /* FIXME verify / also handle other cases */
    LOG_INFO ("Framerate: %u %u %u %u", parse_vui_params,
        vui->fixed_frame_rate_flag, sps->frame_mbs_only_flag,
        vui->pic_struct_present_flag);

    if (parse_vui_params && vui->fixed_frame_rate_flag &&
        sps->frame_mbs_only_flag && !vui->pic_struct_present_flag) {
      sps->fps_num = vui->time_scale;
      sps->fps_den = vui->num_units_in_tick;
      /* picture is a frame = 2 fields */
      sps->fps_den *= 2;
      LOG_INFO ("framerate %d/%d", sps->fps_num, sps->fps_den);
    }
  } else {
    LOG_INFO ("No VUI, unknown framerate");
  }
  return true;

error:
  return false;
}

/* Parse subset_seq_parameter_set() data for MVC */
static bool
h264_parse_sps_mvc_data (NalReader * nr, H264SPS * sps,
    bool parse_vui_params)
{
  H264SPSExtMVC *const mvc = &sps->extension.mvc;
  uint8_t bit_equal_to_one;
  uint32_t i, j, k;

  NAL_READ_UINT8 (nr, bit_equal_to_one, 1);
  if (!bit_equal_to_one)
    return false;

  sps->extension_type = H264_NAL_EXTENSION_MVC;

  NAL_READ_UE_ALLOWED (nr, mvc->num_views_minus1, 0, H264_MAX_VIEW_COUNT - 1);

  mvc->view = (H264SPSExtMVCView *) malloc (sizeof (H264SPSExtMVCView) * (mvc->num_views_minus1 + 1));
  if (!mvc->view)
    goto error_allocation_failed;

  for (i = 0; i <= mvc->num_views_minus1; i++)
    NAL_READ_UE_ALLOWED (nr, mvc->view[i].view_id, 0, H264_MAX_VIEW_ID);

  for (i = 1; i <= mvc->num_views_minus1; i++) {
    /* for RefPicList0 */
    NAL_READ_UE_ALLOWED (nr, mvc->view[i].num_anchor_refs_l0, 0, 15);
    for (j = 0; j < mvc->view[i].num_anchor_refs_l0; j++) {
      NAL_READ_UE_ALLOWED (nr, mvc->view[i].anchor_ref_l0[j], 0,
          H264_MAX_VIEW_ID);
    }

    /* for RefPicList1 */
    NAL_READ_UE_ALLOWED (nr, mvc->view[i].num_anchor_refs_l1, 0, 15);
    for (j = 0; j < mvc->view[i].num_anchor_refs_l1; j++) {
      NAL_READ_UE_ALLOWED (nr, mvc->view[i].anchor_ref_l1[j], 0,
          H264_MAX_VIEW_ID);
    }
  }

  for (i = 1; i <= mvc->num_views_minus1; i++) {
    /* for RefPicList0 */
    NAL_READ_UE_ALLOWED (nr, mvc->view[i].num_non_anchor_refs_l0, 0, 15);
    for (j = 0; j < mvc->view[i].num_non_anchor_refs_l0; j++) {
      NAL_READ_UE_ALLOWED (nr, mvc->view[i].non_anchor_ref_l0[j], 0,
          H264_MAX_VIEW_ID);
    }

    /* for RefPicList1 */
    NAL_READ_UE_ALLOWED (nr, mvc->view[i].num_non_anchor_refs_l1, 0, 15);
    for (j = 0; j < mvc->view[i].num_non_anchor_refs_l1; j++) {
      NAL_READ_UE_ALLOWED (nr, mvc->view[i].non_anchor_ref_l1[j], 0,
          H264_MAX_VIEW_ID);
    }
  }

  NAL_READ_UE_ALLOWED (nr, mvc->num_level_values_signalled_minus1, 0, 63);

  mvc->level_value =
      (H264SPSExtMVCLevelValue*) malloc (sizeof (H264SPSExtMVCLevelValue) *
       (mvc->num_level_values_signalled_minus1 + 1));
  if (!mvc->level_value)
    goto error_allocation_failed;

  for (i = 0; i <= mvc->num_level_values_signalled_minus1; i++) {
    H264SPSExtMVCLevelValue *const level_value = &mvc->level_value[i];

    NAL_READ_UINT8 (nr, level_value->level_idc, 8);

    NAL_READ_UE_ALLOWED (nr, level_value->num_applicable_ops_minus1, 0, 1023);
    level_value->applicable_op =
        (H264SPSExtMVCLevelValueOp*) malloc (sizeof (H264SPSExtMVCLevelValueOp) *
         (level_value->num_applicable_ops_minus1 + 1));
    if (!level_value->applicable_op)
      goto error_allocation_failed;

    for (j = 0; j <= level_value->num_applicable_ops_minus1; j++) {
      H264SPSExtMVCLevelValueOp *const op = &level_value->applicable_op[j];

      NAL_READ_UINT8 (nr, op->temporal_id, 3);

      NAL_READ_UE_ALLOWED (nr, op->num_target_views_minus1, 0, 1023);
      op->target_view_id = (uint16_t*) malloc (sizeof (uint16_t) * (op->num_target_views_minus1 + 1));
      if (!op->target_view_id)
        goto error_allocation_failed;

      for (k = 0; k <= op->num_target_views_minus1; k++)
        NAL_READ_UE_ALLOWED (nr, op->target_view_id[k], 0, H264_MAX_VIEW_ID);
      NAL_READ_UE_ALLOWED (nr, op->num_views_minus1, 0, 1023);
    }
  }

  return true;

error_allocation_failed:
  LOG_WARNING ("failed to allocate memory");
  h264_sps_free_1 (sps);
  return false;

error:
  h264_sps_free_1 (sps);
  return false;
}

/**
 * h264_parse_sps:
 * @nalu: The #H264_NAL_SPS #H264NalUnit to parse
 * @sps: The #H264SPS to fill.
 * @parse_vui_params: Whether to parse the vui_params or not
 *
 * Parses @data, and fills the @sps structure.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parse_sps (H264NalUnit * nalu, H264SPS * sps,
    bool parse_vui_params)
{
  NalReader nr;

  LOG_DEBUG ("parsing SPS");
  nal_reader_init (&nr, nalu->data + nalu->offset + nalu->header_bytes,
      nalu->size - nalu->header_bytes);

  if (!h264_parse_sps_data (&nr, sps, parse_vui_params))
    goto error;

  sps->valid = true;

  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Sequence parameter set\"");
  sps->valid = false;
  return H264_PARSER_ERROR;
}

/**
 * h264_parser_parse_subset_sps:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_SUBSET_SPS #H264NalUnit to parse
 * @sps: The #H264SPS to fill.
 * @parse_vui_params: Whether to parse the vui_params or not
 *
 * Parses @data, and fills in the @sps structure.
 *
 * This function fully parses @data and allocates all the necessary
 * data structures needed for MVC extensions. The resulting @sps
 * structure shall be deallocated with h264_sps_free_1() when it
 * is no longer needed.
 *
 * Note: if the caller doesn't need any of the MVC-specific data, then
 * h264_parser_parse_sps() is more efficient because those extra
 * syntax elements are not parsed and no extra memory is allocated.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_subset_sps (H264NalParser * nalparser,
    H264NalUnit * nalu, H264SPS * sps, bool parse_vui_params)
{
  H264ParserResult res;

  res = h264_parse_subset_sps (nalu, sps, parse_vui_params);
  if (res == H264_PARSER_OK) {
    LOG_DEBUG ("adding sequence parameter set with id: %d to array", sps->id);

    if (!h264_sps_copy (&nalparser->sps[sps->id], sps))
      return H264_PARSER_ERROR;
    nalparser->last_sps = &nalparser->sps[sps->id];
  }
  return res;
}

/**
 * h264_parse_subset_sps:
 * @nalu: The #H264_NAL_SUBSET_SPS #H264NalUnit to parse
 * @sps: The #H264SPS to fill.
 * @parse_vui_params: Whether to parse the vui_params or not
 *
 * Parses @data, and fills in the @sps structure.
 *
 * This function fully parses @data and allocates all the necessary
 * data structures needed for MVC extensions. The resulting @sps
 * structure shall be deallocated with h264_sps_free_1() when it
 * is no longer needed.
 *
 * Note: if the caller doesn't need any of the MVC-specific data, then
 * h264_parser_parse_sps() is more efficient because those extra
 * syntax elements are not parsed and no extra memory is allocated.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parse_subset_sps (H264NalUnit * nalu, H264SPS * sps,
    bool parse_vui_params)
{
  NalReader nr;

  LOG_DEBUG ("parsing Subset SPS");
  nal_reader_init (&nr, nalu->data + nalu->offset + nalu->header_bytes,
      nalu->size - nalu->header_bytes);

  if (!h264_parse_sps_data (&nr, sps, true))
    goto error;

  if (sps->profile_idc == 118 || sps->profile_idc == 128) {
    if (!h264_parse_sps_mvc_data (&nr, sps, parse_vui_params))
      goto error;
  }

  sps->valid = true;
  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Subset sequence parameter set\"");
  sps->valid = false;
  return H264_PARSER_ERROR;
}

/**
 * h264_parse_pps:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_PPS #H264NalUnit to parse
 * @pps: The #H264PPS to fill.
 *
 * Parses @data, and fills the @pps structure.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parse_pps (H264NalParser * nalparser, H264NalUnit * nalu,
    H264PPS * pps)
{
  NalReader nr;
  H264SPS *sps;
  int32_t sps_id;
  uint8_t pic_scaling_matrix_present_flag;
  int32_t qp_bd_offset;

  LOG_DEBUG ("parsing PPS");

  nal_reader_init (&nr, nalu->data + nalu->offset + nalu->header_bytes,
      nalu->size - nalu->header_bytes);

  NAL_READ_UE_ALLOWED (&nr, pps->id, 0, H264_MAX_PPS_COUNT - 1);
  NAL_READ_UE_ALLOWED (&nr, sps_id, 0, H264_MAX_SPS_COUNT - 1);

  sps = h264_parser_get_sps (nalparser, sps_id);
  if (!sps) {
    LOG_WARNING ("couldn't find associated sequence parameter set with id: %d",
        sps_id);
    return H264_PARSER_BROKEN_LINK;
  }
  pps->sequence = sps;
  qp_bd_offset = 6 * (sps->bit_depth_luma_minus8 +
      sps->separate_colour_plane_flag);

  /* set default values for fields that might not be present in the bitstream
     and have valid defaults */
  pps->slice_group_id = NULL;
  pps->transform_8x8_mode_flag = 0;
  memcpy (&pps->scaling_lists_4x4, &sps->scaling_lists_4x4, 96);
  memcpy (&pps->scaling_lists_8x8, &sps->scaling_lists_8x8, 384);

  NAL_READ_UINT8 (&nr, pps->entropy_coding_mode_flag, 1);
  NAL_READ_UINT8 (&nr, pps->pic_order_present_flag, 1);
  NAL_READ_UE_ALLOWED (&nr, pps->num_slice_groups_minus1, 0, 7);
  if (pps->num_slice_groups_minus1 > 0) {
    NAL_READ_UE_ALLOWED (&nr, pps->slice_group_map_type, 0, 6);

    if (pps->slice_group_map_type == 0) {
      int32_t i;

      for (i = 0; i <= pps->num_slice_groups_minus1; i++)
        NAL_READ_UE (&nr, pps->run_length_minus1[i]);
    } else if (pps->slice_group_map_type == 2) {
      int32_t i;

      for (i = 0; i <= pps->num_slice_groups_minus1; i++) {
        NAL_READ_UE (&nr, pps->top_left[i]);
        NAL_READ_UE (&nr, pps->bottom_right[i]);
      }
    } else if (pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5) {
      NAL_READ_UINT8 (&nr, pps->slice_group_change_direction_flag, 1);
      NAL_READ_UE (&nr, pps->slice_group_change_rate_minus1);
    } else if (pps->slice_group_map_type == 6) {
      int32_t i;
      int32_t bits = 0;

      NAL_READ_UE (&nr, pps->pic_size_in_map_units_minus1);
      bits = BIT_STORAGE_CALCULATE(pps->num_slice_groups_minus1);

      pps->slice_group_id =
          (uint8_t*) malloc (sizeof (uint8_t) * (pps->pic_size_in_map_units_minus1 + 1));
      for (i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
        NAL_READ_UINT8 (&nr, pps->slice_group_id[i], bits);
    }
  }

  NAL_READ_UE_ALLOWED (&nr, pps->num_ref_idx_l0_active_minus1, 0, 31);
  NAL_READ_UE_ALLOWED (&nr, pps->num_ref_idx_l1_active_minus1, 0, 31);
  NAL_READ_UINT8 (&nr, pps->weighted_pred_flag, 1);
  NAL_READ_UINT8 (&nr, pps->weighted_bipred_idc, 2);
  NAL_READ_SE_ALLOWED (&nr, pps->pic_init_qp_minus26, -(26 + qp_bd_offset), 25);
  NAL_READ_SE_ALLOWED (&nr, pps->pic_init_qs_minus26, -26, 25);
  NAL_READ_SE_ALLOWED (&nr, pps->chroma_qp_index_offset, -12, 12);
  pps->second_chroma_qp_index_offset = pps->chroma_qp_index_offset;
  NAL_READ_UINT8 (&nr, pps->deblocking_filter_control_present_flag, 1);
  NAL_READ_UINT8 (&nr, pps->constrained_intra_pred_flag, 1);
  NAL_READ_UINT8 (&nr, pps->redundant_pic_cnt_present_flag, 1);

  if (!h264_parser_more_data (&nr))
    goto done;

  NAL_READ_UINT8 (&nr, pps->transform_8x8_mode_flag, 1);

  NAL_READ_UINT8 (&nr, pic_scaling_matrix_present_flag, 1);
  if (pic_scaling_matrix_present_flag) {
    uint8_t n_lists;

    n_lists = 6 + ((sps->chroma_format_idc != 3) ? 2 : 6) *
        pps->transform_8x8_mode_flag;

    if (sps->scaling_matrix_present_flag) {
      if (!h264_parser_parse_scaling_list (&nr,
              pps->scaling_lists_4x4, pps->scaling_lists_8x8,
              sps->scaling_lists_4x4[3], sps->scaling_lists_4x4[0],
              sps->scaling_lists_8x8[3], sps->scaling_lists_8x8[0], n_lists))
        goto error;
    } else {
      if (!h264_parser_parse_scaling_list (&nr,
              pps->scaling_lists_4x4, pps->scaling_lists_8x8,
              default_4x4_inter, default_4x4_intra,
              default_8x8_inter, default_8x8_intra, n_lists))
        goto error;
    }
  }

  NAL_READ_SE_ALLOWED (&nr, pps->second_chroma_qp_index_offset, -12, 12);

done:
  pps->valid = true;
  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Picture parameter set\"");
  pps->valid = false;
  return H264_PARSER_ERROR;
}

/**
 * h264_parser_parse_pps:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_PPS #H264NalUnit to parse
 * @pps: The #H264PPS to fill.
 *
 * Parses @data, and fills the @pps structure.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_pps (H264NalParser * nalparser,
    H264NalUnit * nalu, H264PPS * pps)
{
  H264ParserResult res = h264_parse_pps (nalparser, nalu, pps);

  if (res == H264_PARSER_OK) {
    LOG_DEBUG ("adding picture parameter set with id: %d to array", pps->id);

    nalparser->pps[pps->id] = *pps;
    nalparser->last_pps = &nalparser->pps[pps->id];
  }

  return res;
}

/**
 * h264_parser_parse_slice_hdr:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_SLICE #H264NalUnit to parse
 * @slice: The #H264SliceHdr to fill.
 * @parse_pred_weight_table: Whether to parse the pred_weight_table or not
 * @parse_dec_ref_pic_marking: Whether to parse the dec_ref_pic_marking or not
 *
 * Parses @data, and fills the @slice structure.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_slice_hdr (H264NalParser * nalparser,
    H264NalUnit * nalu, H264SliceHdr * slice,
    bool parse_pred_weight_table, bool parse_dec_ref_pic_marking)
{
  NalReader nr;
  int32_t pps_id;
  H264PPS *pps;
  H264SPS *sps;

  if (!nalu->size) {
    LOG_DEBUG ("Invalid Nal Unit");
    return H264_PARSER_ERROR;
  }


  nal_reader_init (&nr, nalu->data + nalu->offset + nalu->header_bytes,
      nalu->size - nalu->header_bytes);

  NAL_READ_UE (&nr, slice->first_mb_in_slice);
  NAL_READ_UE (&nr, slice->type);

  LOG_DEBUG ("parsing \"Slice header\", slice type %u", slice->type);

  NAL_READ_UE_ALLOWED (&nr, pps_id, 0, H264_MAX_PPS_COUNT - 1);
  pps = h264_parser_get_pps (nalparser, pps_id);

  if (!pps) {
    LOG_WARNING ("couldn't find associated picture parameter set with id: %d",
        pps_id);

    return H264_PARSER_BROKEN_LINK;
  }

  slice->pps = pps;
  sps = pps->sequence;
  if (!sps) {
    LOG_WARNING ("couldn't find associated sequence parameter set with id: %d",
        pps->id);
    return H264_PARSER_BROKEN_LINK;
  }

  /* set default values for fields that might not be present in the bitstream
     and have valid defaults */
  slice->field_pic_flag = 0;
  slice->bottom_field_flag = 0;
  slice->delta_pic_order_cnt_bottom = 0;
  slice->delta_pic_order_cnt[0] = 0;
  slice->delta_pic_order_cnt[1] = 0;
  slice->redundant_pic_cnt = 0;
  slice->num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_active_minus1;
  slice->num_ref_idx_l1_active_minus1 = pps->num_ref_idx_l1_active_minus1;
  slice->disable_deblocking_filter_idc = 0;
  slice->slice_alpha_c0_offset_div2 = 0;
  slice->slice_beta_offset_div2 = 0;

  if (sps->separate_colour_plane_flag)
    NAL_READ_UINT8 (&nr, slice->colour_plane_id, 2);

  NAL_READ_UINT16 (&nr, slice->frame_num, sps->log2_max_frame_num_minus4 + 4);

  if (!sps->frame_mbs_only_flag) {
    NAL_READ_UINT8 (&nr, slice->field_pic_flag, 1);
    if (slice->field_pic_flag)
      NAL_READ_UINT8 (&nr, slice->bottom_field_flag, 1);
  }

  /* calculate MaxPicNum */
  if (slice->field_pic_flag)
    slice->max_pic_num = sps->max_frame_num;
  else
    slice->max_pic_num = 2 * sps->max_frame_num;

  if (nalu->idr_pic_flag)
    NAL_READ_UE_ALLOWED (&nr, slice->idr_pic_id, 0, UINT16_MAX);

  if (sps->pic_order_cnt_type == 0) {
    NAL_READ_UINT16 (&nr, slice->pic_order_cnt_lsb,
        sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    if (pps->pic_order_present_flag && !slice->field_pic_flag)
      NAL_READ_SE (&nr, slice->delta_pic_order_cnt_bottom);
  }

  if (sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag) {
    NAL_READ_SE (&nr, slice->delta_pic_order_cnt[0]);
    if (pps->pic_order_present_flag && !slice->field_pic_flag)
      NAL_READ_SE (&nr, slice->delta_pic_order_cnt[1]);
  }

  if (pps->redundant_pic_cnt_present_flag)
    NAL_READ_UE_ALLOWED (&nr, slice->redundant_pic_cnt, 0, INT8_MAX);

  if (H264_IS_B_SLICE (slice))
    NAL_READ_UINT8 (&nr, slice->direct_spatial_mv_pred_flag, 1);

  if (H264_IS_P_SLICE (slice) || H264_IS_SP_SLICE (slice) ||
      H264_IS_B_SLICE (slice)) {
    uint8_t num_ref_idx_active_override_flag;

    NAL_READ_UINT8 (&nr, num_ref_idx_active_override_flag, 1);
    if (num_ref_idx_active_override_flag) {
      NAL_READ_UE_ALLOWED (&nr, slice->num_ref_idx_l0_active_minus1, 0, 31);

      if (H264_IS_B_SLICE (slice))
        NAL_READ_UE_ALLOWED (&nr, slice->num_ref_idx_l1_active_minus1, 0, 31);
    }
  }

  if (!slice_parse_ref_pic_list_modification (slice, &nr,
          H264_IS_MVC_NALU (nalu)))
    goto error;

  if ((pps->weighted_pred_flag && (H264_IS_P_SLICE (slice)
              || H264_IS_SP_SLICE (slice)))
      || (pps->weighted_bipred_idc == 1 && H264_IS_B_SLICE (slice))) {
    if (!h264_slice_parse_pred_weight_table (slice, &nr,
            sps->chroma_array_type))
      goto error;
  }

  if (nalu->ref_idc != 0) {
    if (!h264_slice_parse_dec_ref_pic_marking (slice, nalu, &nr))
      goto error;
  }

  if (pps->entropy_coding_mode_flag && !H264_IS_I_SLICE (slice) &&
      !H264_IS_SI_SLICE (slice))
    NAL_READ_UE_ALLOWED (&nr, slice->cabac_init_idc, 0, 2);

  NAL_READ_SE_ALLOWED (&nr, slice->slice_qp_delta, -87, 77);

  if (H264_IS_SP_SLICE (slice) || H264_IS_SI_SLICE (slice)) {
    uint8_t sp_for_switch_flag;

    if (H264_IS_SP_SLICE (slice))
      NAL_READ_UINT8 (&nr, sp_for_switch_flag, 1);
    NAL_READ_SE_ALLOWED (&nr, slice->slice_qs_delta, -51, 51);
  }

  if (pps->deblocking_filter_control_present_flag) {
    NAL_READ_UE_ALLOWED (&nr, slice->disable_deblocking_filter_idc, 0, 2);
    if (slice->disable_deblocking_filter_idc != 1) {
      NAL_READ_SE_ALLOWED (&nr, slice->slice_alpha_c0_offset_div2, -6, 6);
      NAL_READ_SE_ALLOWED (&nr, slice->slice_beta_offset_div2, -6, 6);
    }
  }

  if (pps->num_slice_groups_minus1 > 0 &&
      pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5) {
    /* Ceil(Log2(PicSizeInMapUnits / SliceGroupChangeRate + 1))  [7-33] */
    uint32_t PicWidthInMbs = sps->pic_width_in_mbs_minus1 + 1;
    uint32_t PicHeightInMapUnits = sps->pic_height_in_map_units_minus1 + 1;
    uint32_t PicSizeInMapUnits = PicWidthInMbs * PicHeightInMapUnits;
    uint32_t SliceGroupChangeRate = pps->slice_group_change_rate_minus1 + 1;
    const uint32_t n = ceil_log2 (PicSizeInMapUnits / SliceGroupChangeRate + 1);
    NAL_READ_UINT16 (&nr, slice->slice_group_change_cycle, n);
  }

  slice->header_size = nal_reader_get_pos (&nr);
  slice->n_emulation_prevention_bytes = nal_reader_get_epb_count (&nr);

  slice->nal_header_bytes = nalu->header_bytes;

  return H264_PARSER_OK;

error:
  LOG_WARNING ("error parsing \"Slice header\"");
  return H264_PARSER_ERROR;
}

/**
 * h264_parser_parse_sei:
 * @nalparser: a #H264NalParser
 * @nalu: The #H264_NAL_SEI #H264NalUnit to parse
 * @sei: The #H264SEIMessage to fill.
 *
 * Parses @data, and fills the @sei structures.
 *
 * Returns: a #H264ParserResult
 */
H264ParserResult
h264_parser_parse_sei (H264NalParser * nalparser, H264NalUnit * nalu,
    H264SEIMessage * sei)
{
  NalReader nr;

  uint32_t payloadSize;
  uint8_t payload_type_byte, payload_size_byte;
  uint32_t remaining, payload_size;
  bool res;

  LOG_DEBUG ("parsing \"Sei message\"");

  nal_reader_init (&nr, nalu->data + nalu->offset + nalu->header_bytes,
      nalu->size - nalu->header_bytes);

  /* init */
  memset (sei, 0, sizeof (*sei));

  sei->payloadType = 0;
  do {
    NAL_READ_UINT8 (&nr, payload_type_byte, 8);
    sei->payloadType += payload_type_byte;
  } while (payload_type_byte == 0xff);

  payloadSize = 0;
  do {
    NAL_READ_UINT8 (&nr, payload_size_byte, 8);
    payloadSize += payload_size_byte;
  }
  while (payload_size_byte == 0xff);

  remaining = nal_reader_get_remaining (&nr) * 8;
  payload_size = payloadSize < remaining ? payloadSize : remaining;

  LOG_DEBUG ("SEI message received: payloadType  %u, payloadSize = %u bytes",
      sei->payloadType, payload_size);

  if (sei->payloadType == H264_SEI_BUF_PERIOD) {
    /* size not set; might depend on emulation_prevention_three_byte */
    res = h264_parser_parse_buffering_period (nalparser,
        &sei->payload.buffering_period, &nr);
  } else if (sei->payloadType == H264_SEI_PIC_TIMING) {
    /* size not set; might depend on emulation_prevention_three_byte */
    res = h264_parser_parse_pic_timing (nalparser,
        &sei->payload.pic_timing, &nr);
  } else
    res = H264_PARSER_OK;

  return res;

error:
  LOG_WARNING ("error parsing \"Sei message\"");
  return H264_PARSER_ERROR;
}

/* Copy MVC-specific data for subset SPS header */
static bool
h264_sps_mvc_copy (H264SPS * dst_sps, const H264SPS * src_sps)
{
  H264SPSExtMVC *const dst_mvc = &dst_sps->extension.mvc;
  const H264SPSExtMVC *const src_mvc = &src_sps->extension.mvc;
  uint32_t i, j, k;

  assert (dst_sps->extension_type == H264_NAL_EXTENSION_MVC);

  dst_mvc->num_views_minus1 = src_mvc->num_views_minus1;
  dst_mvc->view = (H264SPSExtMVCView*) malloc (sizeof (H264SPSExtMVCView) * (dst_mvc->num_views_minus1 + 1));
  if (!dst_mvc->view)
    return false;

  dst_mvc->view[0].view_id = src_mvc->view[0].view_id;

  for (i = 1; i <= dst_mvc->num_views_minus1; i++) {
    H264SPSExtMVCView *const dst_view = &dst_mvc->view[i];
    const H264SPSExtMVCView *const src_view = &src_mvc->view[i];

    dst_view->view_id = src_view->view_id;

    dst_view->num_anchor_refs_l0 = src_view->num_anchor_refs_l1;
    for (j = 0; j < dst_view->num_anchor_refs_l0; j++)
      dst_view->anchor_ref_l0[j] = src_view->anchor_ref_l0[j];

    dst_view->num_anchor_refs_l1 = src_view->num_anchor_refs_l1;
    for (j = 0; j < dst_view->num_anchor_refs_l1; j++)
      dst_view->anchor_ref_l1[j] = src_view->anchor_ref_l1[j];

    dst_view->num_non_anchor_refs_l0 = src_view->num_non_anchor_refs_l1;
    for (j = 0; j < dst_view->num_non_anchor_refs_l0; j++)
      dst_view->non_anchor_ref_l0[j] = src_view->non_anchor_ref_l0[j];

    dst_view->num_non_anchor_refs_l1 = src_view->num_non_anchor_refs_l1;
    for (j = 0; j < dst_view->num_non_anchor_refs_l1; j++)
      dst_view->non_anchor_ref_l1[j] = src_view->non_anchor_ref_l1[j];
  }

  dst_mvc->num_level_values_signalled_minus1 =
      src_mvc->num_level_values_signalled_minus1;
  dst_mvc->level_value = (H264SPSExtMVCLevelValue *) malloc (sizeof (H264SPSExtMVCLevelValue) *
      (dst_mvc->num_level_values_signalled_minus1 + 1));
  if (!dst_mvc->level_value)
    return false;

  for (i = 0; i <= dst_mvc->num_level_values_signalled_minus1; i++) {
    H264SPSExtMVCLevelValue *const dst_value = &dst_mvc->level_value[i];
    const H264SPSExtMVCLevelValue *const src_value =
        &src_mvc->level_value[i];

    dst_value->level_idc = src_value->level_idc;

    dst_value->num_applicable_ops_minus1 = src_value->num_applicable_ops_minus1;
    dst_value->applicable_op = (H264SPSExtMVCLevelValueOp*) malloc (sizeof (H264SPSExtMVCLevelValueOp) *
        (dst_value->num_applicable_ops_minus1 + 1));
    if (!dst_value->applicable_op)
      return false;

    for (j = 0; j <= dst_value->num_applicable_ops_minus1; j++) {
      H264SPSExtMVCLevelValueOp *const dst_op = &dst_value->applicable_op[j];
      const H264SPSExtMVCLevelValueOp *const src_op =
          &src_value->applicable_op[j];

      dst_op->temporal_id = src_op->temporal_id;
      dst_op->num_target_views_minus1 = src_op->num_target_views_minus1;
      dst_op->target_view_id = (uint16_t*) malloc (sizeof (uint16_t) *
          (dst_op->num_target_views_minus1 + 1));

      if (!dst_op->target_view_id)
        return false;

      for (k = 0; k <= dst_op->num_target_views_minus1; k++)
        dst_op->target_view_id[k] = src_op->target_view_id[k];
      dst_op->num_views_minus1 = src_op->num_views_minus1;
    }
  }
  return true;
}

/**
 * h264_sps_copy:
 * @dst_sps: The destination #H264SPS to copy into
 * @src_sps: The source #H264SPS to copy from
 *
 * Copies @src_sps into @dst_sps.
 *
 * Returns: %true if everything went fine, %false otherwise
 */
bool
h264_sps_copy (H264SPS * dst_sps, const H264SPS * src_sps)
{
  if (!dst_sps || !src_sps)
     return false;

  h264_sps_free_1 (dst_sps);

  *dst_sps = *src_sps;

  if (dst_sps->extension_type == H264_NAL_EXTENSION_MVC) {
    if (!h264_sps_mvc_copy (dst_sps, src_sps))
      return false;
  }

  return true;
}

/* Free MVC-specific data from subset SPS header */
static void
h264_sps_mvc_free_1 (H264SPS * sps)
{
  H264SPSExtMVC *const mvc = &sps->extension.mvc;
  uint32_t i, j;

  assert (sps->extension_type == H264_NAL_EXTENSION_MVC);

  free (mvc->view);
  mvc->view = NULL;

  for (i = 0; i <= mvc->num_level_values_signalled_minus1; i++) {
    H264SPSExtMVCLevelValue *const level_value = &mvc->level_value[i];

    for (j = 0; j <= level_value->num_applicable_ops_minus1; j++) {
      free (level_value->applicable_op[j].target_view_id);
      level_value->applicable_op[j].target_view_id = NULL;
    }
    free (level_value->applicable_op);
    level_value->applicable_op = NULL;
  }
  free (mvc->level_value);
  mvc->level_value = NULL;

  /* All meaningful MVC info are now gone, just pretend to be a
   * standard AVC struct now */
  sps->extension_type = H264_NAL_EXTENSION_NONE;
}

/**
 * h264_sps_free_1:
 * @sps: The #H264SPS to free
 *
 * Frees @sps fields.
 */
void
h264_sps_free_1 (H264SPS * sps)
{
  if (!sps)
    return;

  if (sps->extension_type == H264_NAL_EXTENSION_MVC)
    h264_sps_mvc_free_1 (sps);
}
