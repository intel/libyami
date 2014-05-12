/* 
 * Copyright (C) <2011> Intel Coperation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
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
 * SECTION:mpeg4parser
 * @short_description: Convenience library for parsing mpeg4 part 2 video
 * bitstream.
 *
 * For more details about the structures, you can refer to the
 * specifications: ISO-IEC-14496-2_2004_MPEG4_VISUAL.pdf
 */

#include "bitreader.h"
#include "bytereader.h"
#include "parserutils.h"

#include "mpeg4parser.h"

#define CHECK_MARKER(br) { \
  uint8_t marker;\
  if (!bit_reader_get_bits_uint8 (br, &marker, 1)) { \
    LOG_WARNING ("failed to read marker bit \n"); \
    goto failed; \
  } else if (!marker) {\
    LOG_WARNING ("Wrong marker bit \n"); \
    goto failed;\
  }\
}

#define MARKER_UNCHECKED(br) { \
  if (!bit_reader_get_bits_uint8_unchecked (br, 1)) { \
    LOG_WARNING ("Wrong marker bit \n"); \
    goto failed; \
  } \
}

#define CHECK_REMAINING(br, needed) { \
  if (bit_reader_get_remaining (br) < needed) \
    goto failed; \
}

static const uint8_t default_intra_quant_mat[64] = {
  8, 17, 18, 19, 21, 23, 25, 27,
  17, 18, 19, 21, 23, 25, 27, 28,
  20, 21, 22, 23, 24, 26, 28, 30,
  21, 22, 23, 24, 26, 28, 30, 32,
  22, 23, 24, 26, 28, 30, 32, 35,
  23, 24, 26, 28, 30, 32, 35, 38,
  25, 26, 28, 30, 32, 35, 38, 41,
  27, 28, 30, 32, 35, 38, 41, 45
};

static const uint8_t default_non_intra_quant_mat[64] = {
  16, 17, 18, 19, 20, 21, 22, 23,
  17, 18, 19, 20, 21, 22, 23, 24,
  18, 19, 20, 21, 22, 23, 24, 25,
  19, 20, 21, 22, 23, 24, 26, 27,
  20, 21, 22, 23, 25, 26, 27, 28,
  21, 22, 23, 24, 26, 27, 28, 30,
  22, 23, 24, 26, 27, 28, 30, 31,
  23, 24, 25, 27, 28, 30, 31, 33,
};

static const uint8_t mpeg4_zigzag_8x8[64] = {
  0, 1, 8, 16, 9, 2, 3, 10,
  17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};

static const VLCTable mpeg4_dmv_size_vlc_table[] = {
  {0x00, 2, 0},
  {0x02, 3, 1},
  {0x03, 3, 2},
  {0x04, 3, 3},
  {0x05, 3, 4},
  {0x06, 3, 5},
  {0x0e, 4, 6},
  {0x1e, 5, 7},
  {0x3e, 6, 8},
  {0x7e, 7, 9},
  {0xfe, 8, 10},
  {0x1fe, 9, 11},
  {0x3fe, 10, 12},
  {0x7fe, 11, 13},
  {0xffe, 12, 14}
};

static void
mpeg4_util_par_from_info (uint8_t aspect_ratio_info, uint8_t * par_width,
    uint8_t * par_height)
{
  switch (aspect_ratio_info) {
    case 0x02:
      *par_width = 12;
      *par_height = 11;
      break;
    case 0x03:
      *par_width = 10;
      *par_height = 11;
      break;
    case 0x04:
      *par_width = 16;
      *par_height = 11;
      break;
    case 0x05:
      *par_width = 40;
      *par_height = 33;
      break;

    case 0x01:
    default:
      *par_width = 1;
      *par_height = 1;
  }
}

static bool
parse_quant (BitReader * br, uint8_t quant_mat[64],
    const uint8_t default_quant_mat[64], uint8_t * load_quant_mat)
{
  READ_UINT8 (br, *load_quant_mat, 1);
  if (*load_quant_mat) {
    uint32_t i;
    uint8_t val;

    val = 1;
    for (i = 0; i < 64; i++) {

      if (val != 0)
        READ_UINT8 (br, val, 8);

      if (val == 0) {
        if (i == 0)
          goto invalid_quant_mat;
        quant_mat[mpeg4_zigzag_8x8[i]] = quant_mat[mpeg4_zigzag_8x8[i - 1]];
      } else
        quant_mat[mpeg4_zigzag_8x8[i]] = val;
    }
  } else
    memcpy (quant_mat, default_quant_mat, 64);

  return true;

failed:
  LOG_WARNING ("failed parsing quant matrix");
  return false;

invalid_quant_mat:
  LOG_WARNING ("the first value should be non zero");
  goto failed;
}

static bool
parse_signal_type (BitReader * br, Mpeg4VideoSignalType * signal_type)
{
  READ_UINT8 (br, signal_type->type, 1);

  if (signal_type->type) {
    READ_UINT8 (br, signal_type->format, 3);
    READ_UINT8 (br, signal_type->range, 1);
    READ_UINT8 (br, signal_type->color_description, 1);

    if (signal_type->color_description) {
      READ_UINT8 (br, signal_type->color_primaries, 8);
      READ_UINT8 (br, signal_type->transfer_characteristics, 8);
      READ_UINT8 (br, signal_type->matrix_coefficients, 8);
    }
  }

  return true;

failed:
  LOG_WARNING ("failed parsing \"Video Signal Type\"");

  return false;
}

static bool
parse_sprite_trajectory (BitReader * br,
    Mpeg4SpriteTrajectory * sprite_traj, uint32_t no_of_sprite_warping_points)
{
  uint32_t i, length;

  for (i = 0; i < no_of_sprite_warping_points; i++) {

    if (!decode_vlc (br, &length, mpeg4_dmv_size_vlc_table,
          ARRAY_N_ELEMENT(mpeg4_dmv_size_vlc_table)))
      goto failed;

    if (length)
      READ_UINT16 (br, sprite_traj->vop_ref_points[i], length);
    CHECK_MARKER (br);

    if (!decode_vlc (br, &length, mpeg4_dmv_size_vlc_table,
          ARRAY_N_ELEMENT(mpeg4_dmv_size_vlc_table)))
     goto failed;

    if (length)
      READ_UINT16 (br, sprite_traj->sprite_ref_points[i], length);
    CHECK_MARKER (br);
  }

  return true;

failed:
  LOG_WARNING ("Could not parse the sprite trajectory");
  return false;
}

static uint32_t
find_psc (ByteReader * br)
{
  uint32_t psc_pos = -1, psc;

  if (!byte_reader_peek_uint24_be (br, &psc))
    goto failed;

  /* Scan for the picture start code (22 bits - 0x0020) */
  while ((byte_reader_get_remaining (br) >= 3)) {
    if (byte_reader_peek_uint24_be (br, &psc) &&
        ((psc & 0xfffffc) == 0x000080)) {
      psc_pos = byte_reader_get_pos (br);
      break;
    } else
      byte_reader_skip_unchecked (br, 1);
  }

failed:

  return psc_pos;
}

static inline uint8_t
compute_resync_marker_size (const Mpeg4VideoObjectPlane * vop,
    uint32_t * pattern, uint32_t * mask)
{
  uint8_t off;

  /* FIXME handle the binary only shape case */
  switch (vop->coding_type) {
    case (MPEG4_I_VOP):
      off = 16;
      break;
    case (MPEG4_S_VOP):
    case (MPEG4_P_VOP):
      off = 15 + vop->fcode_forward;

      break;
    case (MPEG4_B_VOP):
      off = MAX (15 + MAX (vop->fcode_forward, vop->fcode_backward), 17);

      break;
    default:
      return -1;
  }

  if (mask && pattern) {
    switch (off) {
      case 16:
        *pattern = 0x00008000;
        *mask = 0xffff8000;
        break;
      case 17:
        *pattern = 0x00004000;
        *mask = 0xffffc000;
        break;
      case 18:
        *pattern = 0x00002000;
        *mask = 0xffffe000;
        break;
      case 19:
        *pattern = 0x00001000;
        *mask = 0xfffff000;
        break;
      case 20:
        *pattern = 0x00000800;
        *mask = 0xfffff800;
        break;
      case 21:
        *pattern = 0x00000400;
        *mask = 0xfffffc00;
        break;
      case 22:
        *pattern = 0x00000200;
        *mask = 0xfffffe00;
        break;
      case 23:
        *pattern = 0x00000100;
        *mask = 0xffffff00;
        break;
    }
  }

  return off++;                 /* Take the following 1 into account */
}

/**
 * mpeg4_next_resync:
 * @packet: The #Mpeg4Packet to fill
 * @vop: The previously parsed #Mpeg4VideoObjectPlane
 * @offset: offset from which to start the parsing
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data and fills @packet with the information of the next resync packet
 * found.
 *
 * Returns: a #Mpeg4ParseResult
 */
static Mpeg4ParseResult
mpeg4_next_resync (Mpeg4Packet * packet,
    const Mpeg4VideoObjectPlane * vop, const uint8_t * data, size_t size,
    bool first_resync_marker)
{
  uint32_t markersize = 0, off1, off2;
  uint32_t mask = 0xff, pattern = 0xff;
  ByteReader br;

  byte_reader_init (&br, data, size);

  RETURN_VAL_IF_FAIL (packet != NULL, MPEG4_PARSER_ERROR);
  RETURN_VAL_IF_FAIL (vop != NULL, MPEG4_PARSER_ERROR);

  markersize = compute_resync_marker_size (vop, &pattern, &mask);

  if (first_resync_marker) {
    off1 = 0;
  } else {
    off1 = byte_reader_masked_scan_uint32 (&br, mask, pattern, 0, size);
  }

  if (off1 == -1)
    return MPEG4_PARSER_NO_PACKET;

  LOG_DEBUG ("Resync code found at %i", off1);

  packet->offset = off1;
  packet->type = MPEG4_RESYNC;
  packet->marker_size = markersize;

  off2 = byte_reader_masked_scan_uint32 (&br, mask, pattern,
      off1 + 2, size - off1 - 2);

  if (off2 == -1)
    return MPEG4_PARSER_NO_PACKET_END;

  packet->size = off2 - off1;

  return MPEG4_PARSER_OK;
}


/********** API **********/

/**
 * mpeg4_parse:
 * @packet: The #Mpeg4Packet to fill
 * @skip_user_data: %true to skip user data packet %false otherwize
 * @vop: The last parsed #Mpeg4VideoObjectPlane or %NULL if you do
 * not need to detect the resync codes.
 * @offset: offset from which to start the parsing
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data and fills @packet with the information of the next packet
 * found.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse (Mpeg4Packet * packet, bool skip_user_data,
    Mpeg4VideoObjectPlane * vop, const uint8_t * data, uint32_t offset,
    size_t size)
{
  int32_t off1, off2;
  ByteReader br;
  Mpeg4ParseResult resync_res;
  static uint32_t first_resync_marker = true;

  byte_reader_init (&br, data, size);

  RETURN_VAL_IF_FAIL (packet != NULL, MPEG4_PARSER_ERROR);

  if (size - offset <= 4) {
    LOG_DEBUG ("Can't parse, buffer is to small size %li at offset %d", size, offset);
    return MPEG4_PARSER_ERROR;
  }

  if (vop) {
    resync_res =
        mpeg4_next_resync (packet, vop, data + offset, size - offset,
        first_resync_marker);
    first_resync_marker = false;

    /*  We found a complet slice */
    if (resync_res == MPEG4_PARSER_OK)
      return resync_res;
    else if (resync_res == MPEG4_PARSER_NO_PACKET_END) {
      /* It doesn't mean there is no standard packet end, look for it */
      off1 = packet->offset;
      goto find_end;
    } else if (resync_res == MPEG4_PARSER_NO_PACKET)
      return resync_res;
  } else {
    first_resync_marker = true;
  }

  off1 = byte_reader_masked_scan_uint32 (&br, 0xffffff00, 0x00000100,
      offset, size - offset);

  if (off1 == -1) {
    LOG_DEBUG ("No start code prefix in this buffer");
    return MPEG4_PARSER_NO_PACKET;
  }

  /* Recursively skip user data if needed */
  if (skip_user_data && data[off1 + 3] == MPEG4_USER_DATA)
    /* If we are here, we know no resync code has been found the first time, so we
     * don't look for it this time */
    return mpeg4_parse (packet, skip_user_data, NULL, data, off1 + 3, size);

  packet->offset = off1 + 3;
  packet->data = data;
  packet->type = (Mpeg4StartCode) (data[off1 + 3]);

find_end:
  off2 = byte_reader_masked_scan_uint32 (&br, 0xffffff00, 0x00000100,
      off1 + 4, size - off1 - 4);

  if (off2 == -1) {
    LOG_DEBUG ("Packet start %d, No end found", off1 + 4);

    packet->size = UINT_MAX;
    return MPEG4_PARSER_NO_PACKET_END;
  }

  if (packet->type == MPEG4_RESYNC) {
    packet->size = (size_t) off2 - off1;
  } else {
    packet->size = (size_t) off2 - off1 - 3;
  }

  LOG_DEBUG ("Complete packet of type %x found at: %d, Size: %li \n",
      packet->type, packet->offset, packet->size);
  return MPEG4_PARSER_OK;

}

/**
 * h263_parse:
 * @packet: The #Mpeg4Packet to fill
 * @offset: offset from which to start the parsing
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data and fills @packet with the information of the next packet
 * found.
 *
 * Note that the type of the packet is meaningless in this case.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
h263_parse (Mpeg4Packet * packet,
    const uint8_t * data, uint32_t offset, size_t size)
{
  int32_t off1, off2;
  ByteReader br;

  byte_reader_init (&br, data + offset, size - offset);

  RETURN_VAL_IF_FAIL (packet != NULL, MPEG4_PARSER_ERROR);

  if (size - offset < 3) {
    LOG_DEBUG ("Can't parse, buffer is to small size %li at offset %d \n",
        size, offset);
    return MPEG4_PARSER_ERROR;
  }

  off1 = find_psc (&br);

  if (off1 == -1) {
    LOG_DEBUG ("No start code prefix in this buffer \n");
    return MPEG4_PARSER_NO_PACKET;
  }

  packet->offset = off1 + offset;
  packet->data = data;

  byte_reader_skip_unchecked (&br, 3);
  off2 = find_psc (&br);

  if (off2 == -1) {
    LOG_DEBUG ("Packet start %d, No end found \n", off1);

    packet->size = UINT_MAX;
    return MPEG4_PARSER_NO_PACKET_END;
  }

  packet->size = (size_t) off2 - off1;

  LOG_DEBUG ("Complete packet found at: %d, Size: %li \n" ,
      packet->offset, packet->size);

  return MPEG4_PARSER_OK;
}

/**
 * mpeg4_parse_visual_object_sequence:
 * @vos: The #Mpeg4VisualObjectSequence structure to fill
 * @data: The data to parse, should contain the visual_object_sequence_start_code
 * but not the start code prefix
 * @size: The size of the @data to parse
 *
 * Parses @data containing the visual object sequence packet, and fills
 * the @vos structure.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse_visual_object_sequence (Mpeg4VisualObjectSequence * vos,
    const uint8_t * data, size_t size)
{
  uint8_t vos_start_code;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (vos != NULL, MPEG4_PARSER_ERROR);

  READ_UINT8 (&br, vos_start_code, 8);
  if (vos_start_code != MPEG4_VISUAL_OBJ_SEQ_START)
    goto wrong_start_code;

  READ_UINT8 (&br, vos->profile_and_level_indication, 8);

  switch (vos->profile_and_level_indication) {
    case 0x01:
      vos->profile = MPEG4_PROFILE_SIMPLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x02:
      vos->profile = MPEG4_PROFILE_SIMPLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x03:
      vos->profile = MPEG4_PROFILE_SIMPLE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0x08:
      vos->profile = MPEG4_PROFILE_SIMPLE;
      vos->level = MPEG4_LEVEL0;
      break;
    case 0x10:
      vos->profile = MPEG4_PROFILE_SIMPLE_SCALABLE;
      vos->level = MPEG4_LEVEL0;
      break;
    case 0x11:
      vos->profile = MPEG4_PROFILE_SIMPLE_SCALABLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x12:
      vos->profile = MPEG4_PROFILE_SIMPLE_SCALABLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x21:
      vos->profile = MPEG4_PROFILE_CORE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x22:
      vos->profile = MPEG4_PROFILE_CORE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x32:
      vos->profile = MPEG4_PROFILE_MAIN;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x33:
      vos->profile = MPEG4_PROFILE_MAIN;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0x34:
      vos->profile = MPEG4_PROFILE_MAIN;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0x42:
      vos->profile = MPEG4_PROFILE_N_BIT;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x51:
      vos->profile = MPEG4_PROFILE_SCALABLE_TEXTURE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x61:
      vos->profile = MPEG4_PROFILE_SIMPLE_FACE_ANIMATION;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x62:
      vos->profile = MPEG4_PROFILE_SIMPLE_FACE_ANIMATION;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x63:
      vos->profile = MPEG4_PROFILE_SIMPLE_FBA;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x64:
      vos->profile = MPEG4_PROFILE_SIMPLE_FBA;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x71:
      vos->profile = MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x72:
      vos->profile = MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x81:
      vos->profile = MPEG4_PROFILE_HYBRID;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x82:
      vos->profile = MPEG4_PROFILE_HYBRID;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x91:
      vos->profile = MPEG4_PROFILE_ADVANCED_REALTIME_SIMPLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0x92:
      vos->profile = MPEG4_PROFILE_ADVANCED_REALTIME_SIMPLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0x93:
      vos->profile = MPEG4_PROFILE_ADVANCED_REALTIME_SIMPLE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0x94:
      vos->profile = MPEG4_PROFILE_ADVANCED_REALTIME_SIMPLE;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xa1:
      vos->profile = MPEG4_PROFILE_CORE_SCALABLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xa2:
      vos->profile = MPEG4_PROFILE_CORE_SCALABLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xa3:
      vos->profile = MPEG4_PROFILE_CORE_SCALABLE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xb1:
      vos->profile = MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xb2:
      vos->profile = MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xb3:
      vos->profile = MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xb4:
      vos->profile = MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xc1:
      vos->profile = MPEG4_PROFILE_ADVANCED_CORE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xc2:
      vos->profile = MPEG4_PROFILE_ADVANCED_CORE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xc3:
      vos->profile = MPEG4_PROFILE_ADVANCED_CORE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xd1:
      vos->profile = MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xd2:
      vos->profile = MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xd3:
      vos->profile = MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xe1:
      vos->profile = MPEG4_PROFILE_SIMPLE_STUDIO;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xe2:
      vos->profile = MPEG4_PROFILE_SIMPLE_STUDIO;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xe3:
      vos->profile = MPEG4_PROFILE_SIMPLE_STUDIO;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xe4:
      vos->profile = MPEG4_PROFILE_SIMPLE_STUDIO;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xe5:
      vos->profile = MPEG4_PROFILE_CORE_STUDIO;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xe6:
      vos->profile = MPEG4_PROFILE_CORE_STUDIO;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xe7:
      vos->profile = MPEG4_PROFILE_CORE_STUDIO;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xe8:
      vos->profile = MPEG4_PROFILE_CORE_STUDIO;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xf0:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL0;
      break;
    case 0xf1:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xf2:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xf3:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xf4:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xf5:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL5;
      break;
    case 0xf7:
      vos->profile = MPEG4_PROFILE_ADVANCED_SIMPLE;
      vos->level = MPEG4_LEVEL3b;
      break;
    case 0xf8:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL0;
      break;
    case 0xf9:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL1;
      break;
    case 0xfa:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL2;
      break;
    case 0xfb:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL3;
      break;
    case 0xfc:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL4;
      break;
    case 0xfd:
      vos->profile = MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE;
      vos->level = MPEG4_LEVEL5;
      break;
    default:
      vos->profile = MPEG4_PROFILE_RESERVED;
      vos->level = MPEG4_LEVEL_RESERVED;
      break;
  }

  return MPEG4_PARSER_OK;

wrong_start_code:
  LOG_WARNING ("got buffer with wrong start code \n");
  return MPEG4_PARSER_ERROR;

failed:
  LOG_WARNING ("failed parsing \"Visual Object\" \n");
  return MPEG4_PARSER_ERROR;
}

/**
 * mpeg4_parse_visual_object:
 * @vo: The #Mpeg4VisualObject structure to fill
 * @signal_type: The #Mpeg4VideoSignalType to fill or %NULL
 * @data: The data to parse, should contain the vo_start_code
 * but not the start code prefix
 * @size: The size of the @data to parse
 *
 * Parses @data containing the visual object packet, and fills
 * the @vo structure.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse_visual_object (Mpeg4VisualObject * vo,
    Mpeg4VideoSignalType * signal_type, const uint8_t * data, size_t size)
{
  uint8_t vo_start_code, type;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (vo != NULL, MPEG4_PARSER_ERROR);

  LOG_DEBUG ("Parsing visual object");

  READ_UINT8 (&br, vo_start_code, 8);
  if (vo_start_code != MPEG4_VISUAL_OBJ)
    goto wrong_start_code;

  /* set default values */
  vo->verid = 0x1;
  vo->priority = 1;

  READ_UINT8 (&br, vo->is_identifier, 1);
  if (vo->is_identifier) {
    READ_UINT8 (&br, vo->verid, 4);
    READ_UINT8 (&br, vo->priority, 3);
  }

  READ_UINT8 (&br, type, 4);
  vo->type = type;

  if ((type == MPEG4_VIDEO_ID ||
          type == MPEG4_STILL_TEXTURE_ID) && signal_type) {

    if (!parse_signal_type (&br, signal_type))
      goto failed;

  } else if (signal_type) {
    signal_type->type = 0;
  }

  return MPEG4_PARSER_OK;

wrong_start_code:
  LOG_WARNING ("got buffer with wrong start code \n");
  return MPEG4_PARSER_ERROR;

failed:
  LOG_WARNING ("failed parsing \"Visual Object\" \n");
  return MPEG4_PARSER_ERROR;
}

/**
 * mpeg4_parse_video_object_layer:
 * @vol: The #Mpeg4VideoObjectLayer structure to fill
 * @vo: The #Mpeg4VisualObject currently being parsed or %NULL
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data containing the video object layer packet, and fills
 * the @vol structure.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse_video_object_layer (Mpeg4VideoObjectLayer * vol,
    Mpeg4VisualObject * vo, const uint8_t * data, size_t size)
{
  uint8_t video_object_layer_start_code;

  /* Used for enums types */
  uint8_t tmp;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (vol != NULL, MPEG4_PARSER_ERROR);

  LOG_DEBUG ("Parsing video object layer \n");

  READ_UINT8 (&br, video_object_layer_start_code, 8);
  if (!(video_object_layer_start_code >= MPEG4_VIDEO_LAYER_FIRST &&
          video_object_layer_start_code <= MPEG4_VIDEO_LAYER_LAST))
    goto failed;

  /* set default values */
  if (vo) {
    vol->verid = vo->verid;
    vol->priority = vo->priority;
  } else {
    vol->verid = 1;
    vol->priority = 0;
  }

  vol->low_delay = false;
  vol->chroma_format = 1;
  vol->vbv_parameters = false;
  vol->quant_precision = 5;
  vol->bits_per_pixel = 8;
  vol->quarter_sample = false;
  vol->newpred_enable = false;
  vol->interlaced = 0;
  vol->width = 0;
  vol->height = 0;

  READ_UINT8 (&br, vol->random_accessible_vol, 1);
  READ_UINT8 (&br, vol->video_object_type_indication, 8);

  READ_UINT8 (&br, vol->is_object_layer_identifier, 1);
  if (vol->is_object_layer_identifier) {
    READ_UINT8 (&br, vol->verid, 4);
    READ_UINT8 (&br, vol->priority, 3);
  }

  READ_UINT8 (&br, tmp, 4);
  vol->aspect_ratio_info = tmp;
  if (vol->aspect_ratio_info != MPEG4_EXTENDED_PAR) {
    mpeg4_util_par_from_info (vol->aspect_ratio_info, &vol->par_width,
        &vol->par_height);

  } else {
    int32_t v;

    READ_UINT8 (&br, vol->par_width, 8);
    v = vol->par_width;
    CHECK_ALLOWED (v, 1, 255);

    READ_UINT8 (&br, vol->par_height, 8);
    v = vol->par_height;
    CHECK_ALLOWED (v, 1, 255);
  }
  LOG_DEBUG ("Pixel aspect ratio %d/%d", vol->par_width, vol->par_width);

  READ_UINT8 (&br, vol->control_parameters, 1);
  if (vol->control_parameters) {
    uint8_t chroma_format;

    READ_UINT8 (&br, chroma_format, 2);
    vol->chroma_format = chroma_format;
    READ_UINT8 (&br, vol->low_delay, 1);

    READ_UINT8 (&br, vol->vbv_parameters, 1);
    if (vol->vbv_parameters) {
      CHECK_REMAINING (&br, 79);

      vol->first_half_bitrate =
          bit_reader_get_bits_uint16_unchecked (&br, 15);
      MARKER_UNCHECKED (&br);

      vol->latter_half_bitrate =
          bit_reader_get_bits_uint16_unchecked (&br, 15);
      MARKER_UNCHECKED (&br);

      vol->bit_rate =
          (vol->first_half_bitrate << 15) | vol->latter_half_bitrate;

      vol->first_half_vbv_buffer_size =
          bit_reader_get_bits_uint16_unchecked (&br, 15);
      MARKER_UNCHECKED (&br);

      vol->latter_half_vbv_buffer_size =
          bit_reader_get_bits_uint8_unchecked (&br, 3);

      vol->vbv_buffer_size = (vol->first_half_vbv_buffer_size << 15) |
          vol->latter_half_vbv_buffer_size;

      vol->first_half_vbv_occupancy =
          bit_reader_get_bits_uint16_unchecked (&br, 11);
      MARKER_UNCHECKED (&br);

      vol->latter_half_vbv_occupancy =
          bit_reader_get_bits_uint16_unchecked (&br, 15);
      MARKER_UNCHECKED (&br);
    }
  }

  READ_UINT8 (&br, tmp, 2);
  vol->shape = tmp;

  if (vol->shape == MPEG4_GRAYSCALE) {
    /* TODO support grayscale shapes, for now we just pass */

    /* Something the standard starts to define... */
    LOG_WARNING ("Grayscale shaped not supported \n");
    goto failed;
  }

  if (vol->shape == MPEG4_GRAYSCALE && vol->verid != 0x01)
    READ_UINT8 (&br, vol->shape_extension, 4);

  CHECK_REMAINING (&br, 19);

  MARKER_UNCHECKED (&br);
  vol->vop_time_increment_resolution =
      bit_reader_get_bits_uint16_unchecked (&br, 16);
  if (vol->vop_time_increment_resolution < 1) {
    LOG_WARNING ("value not in allowed range. value: %d, range %d-%d \n",
        vol->vop_time_increment_resolution, 1, UINT16_MAX);
    goto failed;
  }
  vol->vop_time_increment_bits =
      BIT_STORAGE_CALCULATE (vol->vop_time_increment_resolution);

  MARKER_UNCHECKED (&br);
  vol->fixed_vop_rate = bit_reader_get_bits_uint8_unchecked (&br, 1);
  if (vol->fixed_vop_rate)
    READ_UINT16 (&br, vol->fixed_vop_time_increment,
        vol->vop_time_increment_bits);

  if (vol->shape != MPEG4_BINARY_ONLY) {
    if (vol->shape == MPEG4_RECTANGULAR) {
      CHECK_REMAINING (&br, 29);

      MARKER_UNCHECKED (&br);
      vol->width = bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);
      vol->height = bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);
    }

    READ_UINT8 (&br, vol->interlaced, 1);
    READ_UINT8 (&br, vol->obmc_disable, 1);

    if (vol->verid == 0x1) {
      READ_UINT8 (&br, tmp, 1);
    } else {
      READ_UINT8 (&br, tmp, 2);
    }
    vol->sprite_enable = tmp;

    if (vol->sprite_enable == MPEG4_SPRITE_STATIC ||
        vol->sprite_enable == MPEG4_SPRITE_GMG) {

      if (vol->sprite_enable == MPEG4_SPRITE_GMG) {
        CHECK_REMAINING (&br, 9);
      } else {
        CHECK_REMAINING (&br, 65);

        vol->sprite_width = bit_reader_get_bits_uint16_unchecked (&br, 13);
        MARKER_UNCHECKED (&br);

        vol->sprite_height = bit_reader_get_bits_uint16_unchecked (&br, 13);
        MARKER_UNCHECKED (&br);

        vol->sprite_left_coordinate =
            bit_reader_get_bits_uint16_unchecked (&br, 13);
        MARKER_UNCHECKED (&br);

        vol->sprite_top_coordinate =
            bit_reader_get_bits_uint16_unchecked (&br, 13);
        MARKER_UNCHECKED (&br);
      }
      vol->no_of_sprite_warping_points =
          bit_reader_get_bits_uint8_unchecked (&br, 6);
      vol->sprite_warping_accuracy =
          bit_reader_get_bits_uint8_unchecked (&br, 2);
      vol->sprite_brightness_change =
          bit_reader_get_bits_uint8_unchecked (&br, 1);

      if (vol->sprite_enable != MPEG4_SPRITE_GMG)
        vol->low_latency_sprite_enable =
            bit_reader_get_bits_uint8_unchecked (&br, 1);
    }

    if (vol->verid != 0x1 && vol->shape != MPEG4_RECTANGULAR)
      READ_UINT8 (&br, vol->sadct_disable, 1);

    READ_UINT8 (&br, vol->not_8_bit, 1);
    if (vol->not_8_bit) {
      READ_UINT8 (&br, vol->quant_precision, 4);
      CHECK_ALLOWED (vol->quant_precision, 3, 9);

      READ_UINT8 (&br, vol->bits_per_pixel, 4);
      CHECK_ALLOWED (vol->bits_per_pixel, 4, 12);
    }

    if (vol->shape == MPEG4_GRAYSCALE) {
      /* We don't actually support it */
      READ_UINT8 (&br, vol->no_gray_quant_update, 1);
      READ_UINT8 (&br, vol->composition_method, 1);
      READ_UINT8 (&br, vol->linear_composition, 1);
    }

    READ_UINT8 (&br, vol->quant_type, 1);
    if (vol->quant_type) {
      if (!parse_quant (&br, vol->intra_quant_mat, default_intra_quant_mat,
              &vol->load_intra_quant_mat))
        goto failed;

      if (!parse_quant (&br, vol->non_intra_quant_mat,
              default_non_intra_quant_mat, &vol->load_non_intra_quant_mat))
        goto failed;

      if (vol->shape == MPEG4_GRAYSCALE) {
        /* Something the standard starts to define... */
        LOG_WARNING ("Grayscale shaped not supported \n");
        goto failed;
      }

    } else {
      memset (&vol->intra_quant_mat, 0, 64);
      memset (&vol->non_intra_quant_mat, 0, 64);
    }

    if (vol->verid != 0x1)
      READ_UINT8 (&br, vol->quarter_sample, 1);

    READ_UINT8 (&br, vol->complexity_estimation_disable, 1);
    if (!vol->complexity_estimation_disable) {
      uint8_t estimation_method;
      uint8_t estimation_disable;

      /* skip unneeded properties */
      READ_UINT8 (&br, estimation_method, 2);
      if (estimation_method < 2) {
        READ_UINT8 (&br, estimation_disable, 1);
        if (!estimation_disable)
          SKIP (&br, 6);
        READ_UINT8 (&br, estimation_disable, 1);
        if (!estimation_disable)
          SKIP (&br, 4);
        CHECK_MARKER (&br);
        READ_UINT8 (&br, estimation_disable, 1);
        if (!estimation_disable)
          SKIP (&br, 4);
        READ_UINT8 (&br, estimation_disable, 1);
        if (!estimation_disable)
          SKIP (&br, 6);
        CHECK_MARKER (&br);

        if (estimation_method == 1) {
          READ_UINT8 (&br, estimation_disable, 1);
          if (!estimation_disable)
            SKIP (&br, 2);
        }
      }
    }

    READ_UINT8 (&br, vol->resync_marker_disable, 1);
    READ_UINT8 (&br, vol->data_partitioned, 1);

    if (vol->data_partitioned)
      READ_UINT8 (&br, vol->reversible_vlc, 1);

    if (vol->verid != 0x01) {
      READ_UINT8 (&br, vol->newpred_enable, 1);
      if (vol->newpred_enable)
        /* requested_upstream_message_type and newpred_segment_type */
        SKIP (&br, 3);

      READ_UINT8 (&br, vol->reduced_resolution_vop_enable, 1);
    }

    READ_UINT8 (&br, vol->scalability, 1);
    if (vol->scalability) {
      SKIP (&br, 26);           /* Few not needed props */
      READ_UINT8 (&br, vol->enhancement_type, 1);
    }

    /* More unused infos */
  } else if (vol->verid != 0x01) {
    LOG_WARNING ("Binary only shapes not fully supported \n");
    goto failed;
  }
  /* ... */

  return MPEG4_PARSER_OK;

failed:
  LOG_WARNING ("failed parsing \"Video Object Layer\" \n");
  return MPEG4_PARSER_ERROR;

wrong_start_code:
  LOG_WARNING ("got buffer with wrong start code \n");
  goto failed;
}

/**
 * mpeg4_parse_group_of_vop:
 * @gov: The #Mpeg4GroupOfVOP structure to fill
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data containing the group of video object plane packet, and fills
 * the @gov structure.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse_group_of_vop (Mpeg4GroupOfVOP *
    gov, const uint8_t * data, size_t size)
{
  uint8_t gov_start_code;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (gov != NULL, MPEG4_PARSER_ERROR);

  READ_UINT8 (&br, gov_start_code, 8);
  if (gov_start_code != MPEG4_GROUP_OF_VOP)
    goto wrong_start_code;

  CHECK_REMAINING (&br, 65);

  gov->hours = bit_reader_get_bits_uint8_unchecked (&br, 5);
  gov->minutes = bit_reader_get_bits_uint8_unchecked (&br, 6);
  /* marker bit */
  MARKER_UNCHECKED (&br);
  gov->seconds = bit_reader_get_bits_uint8_unchecked (&br, 6);

  gov->closed = bit_reader_get_bits_uint8_unchecked (&br, 1);
  gov->broken_link = bit_reader_get_bits_uint8_unchecked (&br, 1);

  return MPEG4_PARSER_OK;

failed:
  LOG_WARNING ("failed parsing \"Group of Video Object Plane\" \n");
  return MPEG4_PARSER_ERROR;

wrong_start_code:
  LOG_WARNING ("got buffer with wrong start code \n");
  goto failed;
}

/**
 * mpeg4_parse_video_object_plane:
 * @vop: The #Mpeg4VideoObjectPlane currently being parsed
 * @sprite_trajectory: A #Mpeg4SpriteTrajectory to fill or %NULL
 * @vol: The #Mpeg4VideoObjectLayer structure to fill
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data containing the video object plane packet, and fills the @vol
 * structure.
 *
 * Returns: a #Mpeg4ParseResult
 */
Mpeg4ParseResult
mpeg4_parse_video_object_plane (Mpeg4VideoObjectPlane * vop,
    Mpeg4SpriteTrajectory * sprite_trajectory,
    Mpeg4VideoObjectLayer * vol, const uint8_t * data, size_t size)
{
  uint8_t vop_start_code, coding_type, modulo_time_base;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (vop != NULL, MPEG4_PARSER_ERROR);

  if (vol->shape == MPEG4_BINARY_ONLY) {
    /* TODO: implement binary only shapes */
    LOG_WARNING ("Binary only shapes not supported \n");
    goto failed;
  }

  READ_UINT8 (&br, vop_start_code, 8);
  if (vop_start_code != MPEG4_VIDEO_OBJ_PLANE)
    goto wrong_start_code;


  /* set default values */
  vop->modulo_time_base = 0;
  vop->rounding_type = 0;
  vop->top_field_first = 1;
  vop->alternate_vertical_scan_flag = 0;
  vop->fcode_forward = 1;
  vop->fcode_backward = 1;

  /*  Compute macroblock informations */
  if (vol->interlaced)
    vop->mb_height = (2 * (vol->height + 31) / 32);
  else
    vop->mb_height = (vol->height + 15) / 16;

  vop->mb_width = (vol->width + 15) / 16;
  vop->mb_num = vop->mb_height * vop->mb_width;

  READ_UINT8 (&br, coding_type, 2);
  vop->coding_type = coding_type;

  READ_UINT8 (&br, modulo_time_base, 1);
  while (modulo_time_base) {
    vop->modulo_time_base++;

    READ_UINT8 (&br, modulo_time_base, 1);
  }

  CHECK_REMAINING (&br, vol->vop_time_increment_bits + 3);

  MARKER_UNCHECKED (&br);
  vop->time_increment =
      bit_reader_get_bits_uint16_unchecked (&br,
      vol->vop_time_increment_bits);
  MARKER_UNCHECKED (&br);

  vop->coded = bit_reader_get_bits_uint8_unchecked (&br, 1);
  if (!vop->coded)
    return MPEG4_PARSER_OK;

  if (vol->newpred_enable) {
    uint16_t nbbits =
        vop->time_increment + 3 < 15 ? vop->time_increment + 3 : 15;

    READ_UINT16 (&br, vop->id, nbbits);
    READ_UINT8 (&br, vop->id_for_prediction_indication, 1);
    if (vop->id_for_prediction_indication) {
      /* Would be nice if the standard actually told us... */
      READ_UINT16 (&br, vop->id, nbbits);
      CHECK_MARKER (&br);
    }
  }

  if (vol->shape != MPEG4_BINARY_ONLY &&
      (vop->coding_type == MPEG4_P_VOP ||
          (vop->coding_type == MPEG4_S_VOP &&
              vol->sprite_enable == MPEG4_SPRITE_GMG)))
    READ_UINT8 (&br, vop->rounding_type, 1);

  if ((vol->reduced_resolution_vop_enable) &&
      (vol->shape == MPEG4_RECTANGULAR ||
          (vop->coding_type = MPEG4_P_VOP ||
              vop->coding_type == MPEG4_I_VOP)))
    READ_UINT8 (&br, vop->reduced_resolution, 1);

  if (vol->shape != MPEG4_RECTANGULAR) {
    if (vol->sprite_enable == MPEG4_SPRITE_STATIC &&
        vop->coding_type == MPEG4_I_VOP) {
      CHECK_REMAINING (&br, 55);

      vop->width = bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);

      vop->height = bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);

      vop->horizontal_mc_spatial_ref =
          bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);

      vop->vertical_mc_spatial_ref =
          bit_reader_get_bits_uint16_unchecked (&br, 13);
      MARKER_UNCHECKED (&br);

      /* Recompute the Macroblock informations
       * accordingly to the new values */
      if (vol->interlaced)
        vop->mb_height = (2 * (vol->height + 31) / 32);
      else
        vop->mb_height = (vol->height + 15) / 16;

      vop->mb_width = (vol->width + 15) / 16;
      vop->mb_num = vop->mb_height * vop->mb_width;
    }

    if ((vol->shape != MPEG4_BINARY_ONLY) &&
        vol->scalability && vol->enhancement_type)
      READ_UINT8 (&br, vop->background_composition, 1);

    READ_UINT8 (&br, vop->change_conv_ratio_disable, 1);

    READ_UINT8 (&br, vop->constant_alpha, 1);
    if (vop->constant_alpha)
      READ_UINT8 (&br, vop->constant_alpha_value, 1);
  }

  if (vol->shape != MPEG4_BINARY_ONLY) {
    if (!vol->complexity_estimation_disable) {
      LOG_WARNING ("Complexity estimation not supported \n");
      goto failed;
    }

    READ_UINT8 (&br, vop->intra_dc_vlc_thr, 3);

    if (vol->interlaced) {
      READ_UINT8 (&br, vop->top_field_first, 1);
      READ_UINT8 (&br, vop->alternate_vertical_scan_flag, 1);
    }
  }

  if ((vol->sprite_enable == MPEG4_SPRITE_STATIC ||
          vol->sprite_enable == MPEG4_SPRITE_GMG) &&
      vop->coding_type == MPEG4_S_VOP) {

    /* only if @sprite_trajectory is not NULL we parse it */
    if (sprite_trajectory && vol->no_of_sprite_warping_points)
      parse_sprite_trajectory (&br, sprite_trajectory,
          vol->no_of_sprite_warping_points);

    if (vol->sprite_brightness_change) {
      LOG_WARNING ("sprite_brightness_change not supported \n");
      goto failed;
    }

    if (vol->sprite_enable == MPEG4_SPRITE_STATIC) {
      LOG_WARNING ("sprite enable static not supported \n");
      goto failed;
    }
  }

  if (vol->shape != MPEG4_BINARY_ONLY) {
    READ_UINT16 (&br, vop->quant, vol->quant_precision);

    if (vol->shape == MPEG4_GRAYSCALE) {
      /* TODO implement grayscale support */
      LOG_WARNING ("Grayscale shapes no supported \n");

      /* TODO implement me */
      goto failed;
    }

    if (vop->coding_type != MPEG4_I_VOP) {
      READ_UINT8 (&br, vop->fcode_forward, 3);
      CHECK_ALLOWED (vop->fcode_forward, 1, 7);
    }

    if (vop->coding_type == MPEG4_B_VOP) {
      READ_UINT8 (&br, vop->fcode_backward, 3);
      CHECK_ALLOWED (vop->fcode_backward, 1, 7);
    }
  }

  if (!vol->scalability) {
    if (vol->shape != MPEG4_RECTANGULAR)
      READ_UINT8 (&br, vop->shape_coding_type, 1);

  } else {
    if (vol->enhancement_type) {
      READ_UINT8 (&br, vop->load_backward_shape, 1);

      if (vop->load_backward_shape) {
        LOG_WARNING ("Load backward shape not supported \n");
        goto failed;
      }

      READ_UINT8 (&br, vop->ref_select_code, 2);
    }
  }

  vop->size = bit_reader_get_pos (&br);
  /* More things to possibly parse ... */

  return MPEG4_PARSER_OK;

failed:
  LOG_WARNING ("failed parsing \"Video Object Plane\" \n");
  return MPEG4_PARSER_ERROR;

wrong_start_code:
  LOG_WARNING ("got buffer with wrong start code \n");
  goto failed;
}

/**
 * mpeg4_parse_video_plane_with_short_header:
 * @shorthdr: The #Mpeg4VideoPlaneShortHdr to parse
 * @data: The data to parse
 * @size: The size of the @data to parse
 */
Mpeg4ParseResult
mpeg4_parse_video_plane_short_header (Mpeg4VideoPlaneShortHdr *
    shorthdr, const uint8_t * data, size_t size)
{
  uint8_t zero_bits;

  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (shorthdr != NULL, MPEG4_PARSER_ERROR);

  if (bit_reader_get_remaining (&br) < 48)
    goto failed;

  if (bit_reader_get_bits_uint32_unchecked (&br, 22) != 0x20)
    goto failed;

  shorthdr->temporal_reference =
      bit_reader_get_bits_uint8_unchecked (&br, 8);
  CHECK_MARKER (&br);
  zero_bits = bit_reader_get_bits_uint8_unchecked (&br, 1);
  if (zero_bits != 0x00)
    goto failed;

  shorthdr->split_screen_indicator =
      bit_reader_get_bits_uint8_unchecked (&br, 1);
  shorthdr->document_camera_indicator =
      bit_reader_get_bits_uint8_unchecked (&br, 1);
  shorthdr->full_picture_freeze_release =
      bit_reader_get_bits_uint8_unchecked (&br, 1);
  shorthdr->source_format = bit_reader_get_bits_uint8_unchecked (&br, 3);

  /* Set parameters/Table 6-25 */
  switch (shorthdr->source_format) {
    case 0x01:
      shorthdr->vop_width = 128;
      shorthdr->vop_height = 96;
      shorthdr->num_macroblocks_in_gob = 8;
      shorthdr->num_gobs_in_vop = 6;
      break;
    case 0x02:
      shorthdr->vop_width = 176;
      shorthdr->vop_height = 144;
      shorthdr->num_macroblocks_in_gob = 11;
      shorthdr->num_gobs_in_vop = 9;
      break;
    case 0x03:
      shorthdr->vop_width = 352;
      shorthdr->vop_height = 288;
      shorthdr->num_macroblocks_in_gob = 22;
      shorthdr->num_gobs_in_vop = 18;
      break;
    case 0x04:
      shorthdr->vop_width = 704;
      shorthdr->vop_height = 576;
      shorthdr->num_macroblocks_in_gob = 88;
      shorthdr->num_gobs_in_vop = 18;
      break;
    case 0x05:
      shorthdr->vop_width = 1408;
      shorthdr->vop_height = 1152;
      shorthdr->num_macroblocks_in_gob = 352;
      shorthdr->num_gobs_in_vop = 18;
      break;
    default:
      shorthdr->vop_width = 0;
      shorthdr->vop_height = 0;
      shorthdr->num_macroblocks_in_gob = 0;
      shorthdr->num_gobs_in_vop = 0;
  }

  shorthdr->picture_coding_type =
      bit_reader_get_bits_uint8_unchecked (&br, 1);
  zero_bits = bit_reader_get_bits_uint8_unchecked (&br, 4);

  if (zero_bits != 0x00)
    goto failed;

  shorthdr->vop_quant = bit_reader_get_bits_uint8_unchecked (&br, 5);
  zero_bits = bit_reader_get_bits_uint8_unchecked (&br, 1);

  if (zero_bits != 0x00)
    goto failed;

  do {
    READ_UINT8 (&br, shorthdr->pei, 1);

    if (shorthdr->pei == 1)
      READ_UINT8 (&br, shorthdr->psupp, 8);

  } while (shorthdr->pei == 1);

  shorthdr->size = bit_reader_get_pos (&br);

  return MPEG4_PARSER_OK;

failed:
  LOG_WARNING ("Could not parse the Plane short header \n");

  return MPEG4_PARSER_ERROR;
}

/**
 * mpeg4_parse_video_packet_header:
 * @videopackethdr: The #Mpeg4VideoPacketHdr structure to fill
 * @vol: The last parsed #Mpeg4VideoObjectLayer, will be updated
 * with the informations found during the parsing
 * @vop: The last parsed #Mpeg4VideoObjectPlane, will be updated
 * with the informations found during the parsing
 * @sprite_trajectory: A #Mpeg4SpriteTrajectory to fill or %NULL
 * with the informations found during the parsing
 * @data: The data to parse, should be set after the resync marker.
 * @size: The size of the data to parse
 *
 * Parsers @data containing the video packet header
 * and fills the @videopackethdr structure
 */
Mpeg4ParseResult
mpeg4_parse_video_packet_header (Mpeg4VideoPacketHdr * videopackethdr,
    Mpeg4VideoObjectLayer * vol, Mpeg4VideoObjectPlane * vop,
    Mpeg4SpriteTrajectory * sprite_trajectory, const uint8_t * data,
    size_t size)
{
  uint8_t markersize;
  BitReader br = BIT_READER_INIT (data, size);

  RETURN_VAL_IF_FAIL (videopackethdr != NULL, MPEG4_PARSER_ERROR);
  RETURN_VAL_IF_FAIL (vol != NULL, MPEG4_PARSER_ERROR);

  markersize = compute_resync_marker_size (vop, NULL, NULL);

  CHECK_REMAINING (&br, markersize);

  if (bit_reader_get_bits_uint32_unchecked (&br, markersize + 1) != 0x01)
    goto failed;

  if (vol->shape != MPEG4_RECTANGULAR) {
    READ_UINT8 (&br, videopackethdr->header_extension_code, 1);
    if (vol->sprite_enable == MPEG4_SPRITE_STATIC &&
        vop->coding_type == MPEG4_I_VOP) {

      CHECK_REMAINING (&br, 56);

      U_READ_UINT16 (&br, vop->width, 13);
      CHECK_MARKER (&br);
      U_READ_UINT16 (&br, vop->height, 13);
      CHECK_MARKER (&br);
      U_READ_UINT16 (&br, vop->horizontal_mc_spatial_ref, 13);
      CHECK_MARKER (&br);
      U_READ_UINT16 (&br, vop->vertical_mc_spatial_ref, 13);
      CHECK_MARKER (&br);

      /* Update macroblock infirmations */
      vop->mb_height = (vop->height + 15) / 16;
      vop->mb_width = (vop->width + 15) / 16;
      vop->mb_num = vop->mb_height * vop->mb_width;
    }
  }

  READ_UINT16 (&br, videopackethdr->macroblock_number,
      BIT_STORAGE_CALCULATE (vop->mb_num - 1));

  if (vol->shape != MPEG4_BINARY_ONLY)
    READ_UINT16 (&br, videopackethdr->quant_scale, vol->quant_precision);

  if (vol->shape == MPEG4_RECTANGULAR)
    READ_UINT8 (&br, videopackethdr->header_extension_code, 1);

  if (videopackethdr->header_extension_code) {
    uint32_t timeincr = 0;
    uint8_t bit = 0, coding_type;

    do {
      READ_UINT8 (&br, bit, 1);
      timeincr++;
    } while (bit);

    vol->vop_time_increment_bits = timeincr;

    CHECK_MARKER (&br);
    READ_UINT16 (&br, vop->time_increment, timeincr);
    CHECK_MARKER (&br);
    READ_UINT8 (&br, coding_type, 2);
    vop->coding_type = coding_type;

    if (vol->shape != MPEG4_RECTANGULAR) {
      READ_UINT8 (&br, vop->change_conv_ratio_disable, 1);
      if (vop->coding_type != MPEG4_I_VOP)
        READ_UINT8 (&br, vop->shape_coding_type, 1);
    }

    if (vol->shape != MPEG4_BINARY_ONLY) {
      READ_UINT8 (&br, vop->intra_dc_vlc_thr, 3);

      if (sprite_trajectory && vol->sprite_enable == MPEG4_SPRITE_GMG &&
          vop->coding_type == MPEG4_S_VOP &&
          vol->no_of_sprite_warping_points > 0) {

        parse_sprite_trajectory (&br, sprite_trajectory,
            vol->no_of_sprite_warping_points);
      }

      if (vol->reduced_resolution_vop_enable &&
          vol->shape == MPEG4_RECTANGULAR &&
          (vop->coding_type == MPEG4_P_VOP ||
              vop->coding_type == MPEG4_I_VOP))
        READ_UINT8 (&br, vop->reduced_resolution, 1);

      if (vop->coding_type != MPEG4_I_VOP) {
        READ_UINT8 (&br, vop->fcode_forward, 3);
        CHECK_ALLOWED (vop->fcode_forward, 1, 7);
      }

      if (vop->coding_type == MPEG4_B_VOP) {
        READ_UINT8 (&br, vop->fcode_backward, 3);
        CHECK_ALLOWED (vop->fcode_backward, 1, 7);
      }
    }
  }

  if (vol->newpred_enable) {
    uint16_t nbbits =
        vol->vop_time_increment_bits + 3 < 15 ? vop->time_increment + 3 : 15;

    READ_UINT16 (&br, vop->id, nbbits);
    READ_UINT8 (&br, vop->id_for_prediction_indication, 1);
    if (vop->id_for_prediction_indication) {
      /* Would be nice if the standard actually told us... */
      READ_UINT16 (&br, vop->id, nbbits);
      CHECK_MARKER (&br);
    }
  }

  videopackethdr->size = bit_reader_get_pos (&br);

failed:
  LOG_DEBUG ("Failed to parse video packet header \n");

  return MPEG4_PARSER_NO_PACKET;
}
