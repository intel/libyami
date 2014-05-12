/* reamer
 * Copyright (C) <2011> Intel Corporation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * From bad/sys/vdpau/mpeg/mpegutil.c:
 *   Copyright (C) <2007> Jan Schmidt <thaytan@mad.scientist.com>
 *   Copyright (C) <2009> Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
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
 * SECTION:gstmpegvideoparser
 * @short_description: Convenience library for mpeg1 and 2 video
 * bitstream parsing.
 *
 * <refsect2>
 * <para>
 * Provides useful functions for mpeg videos bitstream parsing.
 * </para>
 * </refsect2>
 */

#include "bitreader.h"
#include "bytereader.h"
#include "parserutils.h"

#include "mpegvideoparser.h"

#define MARKER_BIT 0x1

/* default intra quant matrix, in zig-zag order */
static const uint8_t default_intra_quantizer_matrix[64] = {
  8,
  16, 16,
  19, 16, 19,
  22, 22, 22, 22,
  22, 22, 26, 24, 26,
  27, 27, 27, 26, 26, 26,
  26, 27, 27, 27, 29, 29, 29,
  34, 34, 34, 29, 29, 29, 27, 27,
  29, 29, 32, 32, 34, 34, 37,
  38, 37, 35, 35, 34, 35,
  38, 38, 40, 40, 40,
  48, 48, 46, 46,
  56, 56, 58,
  69, 69,
  83
};

static const uint8_t mpeg_zigzag_8x8[64] = {
  0, 1, 8, 16, 9, 2, 3, 10,
  17, 24, 32, 25, 18, 11, 4, 5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13, 6, 7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};

static bool initialized = false;

static inline bool
find_start_code (BitReader * b)
{
  uint32_t bits;

  /* 0 bits until byte aligned */
  while (b->bit != 0) {
    GET_BITS (b, 1, &bits);
  }

  /* 0 bytes until startcode */
  while (bit_reader_peek_bits_uint32 (b, &bits, 32)) {
    if (bits >> 8 == 0x1) {
      return true;
    } else if (bit_reader_skip (b, 8) == false)
      break;
  }

  return false;

failed:
  return false;
}

/* Set the Pixel Aspect Ratio in our hdr from a ASR code in the data */
static void
set_par_from_asr_mpeg1 (MpegVideoSequenceHdr * seqhdr, uint8_t asr_code)
{
  int32_t ratios[16][2] = {
    {0, 0},                     /* 0, Invalid */
    {1, 1},                     /* 1, 1.0 */
    {10000, 6735},              /* 2, 0.6735 */
    {64, 45},                   /* 3, 0.7031 16:9 625 line */
    {10000, 7615},              /* 4, 0.7615 */
    {10000, 8055},              /* 5, 0.8055 */
    {32, 27},                   /* 6, 0.8437 */
    {10000, 8935},              /* 7, 0.8935 */
    {10000, 9375},              /* 8, 0.9375 */
    {10000, 9815},              /* 9, 0.9815 */
    {10000, 10255},             /* 10, 1.0255 */
    {10000, 10695},             /* 11, 1.0695 */
    {8, 9},                     /* 12, 1.125 */
    {10000, 11575},             /* 13, 1.1575 */
    {10000, 12015},             /* 14, 1.2015 */
    {0, 0},                     /* 15, invalid */
  };
  asr_code &= 0xf;

  seqhdr->par_w = ratios[asr_code][0];
  seqhdr->par_h = ratios[asr_code][1];
}

static void
set_fps_from_code (MpegVideoSequenceHdr * seqhdr, uint8_t fps_code)
{
  const int32_t framerates[][2] = {
    {30, 1}, {24000, 1001}, {24, 1}, {25, 1},
    {30000, 1001}, {30, 1}, {50, 1}, {60000, 1001},
    {60, 1}, {30, 1}
  };

  if (fps_code && fps_code < 10) {
    seqhdr->fps_n = framerates[fps_code][0];
    seqhdr->fps_d = framerates[fps_code][1];
  } else {
    LOG_DEBUG ("unknown/invalid frame_rate_code %d", fps_code);
    /* Force a valid framerate */
    /* FIXME or should this be kept unknown ?? */
    seqhdr->fps_n = 30000;
    seqhdr->fps_d = 1001;
  }
}

/* @size and @offset are wrt current reader position */
static inline uint32_t
scan_for_start_codes (const ByteReader * reader, uint32_t offset, uint32_t size)
{
  const uint8_t *data;
  uint32_t i = 0;

  RETURN_VAL_IF_FAIL ((uint64_t) offset + size <= reader->size - reader->byte,
      -1);

  /* we can't find the pattern with less than 4 bytes */
  if (size < 4)
    return -1;

  data = reader->data + reader->byte + offset;

  while (i <= (size - 4)) {
    if (data[i + 2] > 1) {
      i += 3;
    } else if (data[i + 1]) {
      i += 2;
    } else if (data[i] || data[i + 2] != 1) {
      i++;
    } else {
      break;
    }
  }

  if (i <= (size - 4))
    return offset + i;

  /* nothing found */
  return -1;
}

/****** API *******/

/**
 * mpeg_video_parse:
 * @data: The data to parse
 * @size: The size of @data
 * @offset: The offset from which to start parsing
 *
 * Parses the MPEG 1/2 video bitstream contained in @data , and returns the
 * detect packets as a list of #MpegVideoTypeOffsetSize.
 *
 * Returns: true if a packet start code was found
 */
bool
mpeg_video_parse (MpegVideoPacket * packet,
    const uint8_t * data, size_t size, uint32_t offset)
{
  int32_t off;
  ByteReader br;

  if (!initialized) {
    initialized = true;
  }

  if (size <= offset) {
    LOG_DEBUG ("Can't parse from offset %d, buffer is to small", offset);
    return false;
  }

  size -= offset;
  byte_reader_init (&br, &data[offset], size);

  off = scan_for_start_codes (&br, 0, size);

  if (off < 0) {
    LOG_DEBUG ("No start code prefix in this buffer");
    return false;
  }

  if (byte_reader_skip (&br, off + 3) == false)
    goto failed;

  if (byte_reader_get_uint8 (&br, &packet->type) == false)
    goto failed;

  packet->data = data;
  packet->offset = offset + off + 4;
  packet->size = -1;

  /* try to find end of packet */
  size -= off + 4;
  off = scan_for_start_codes (&br, 0, size);

  if (off > 0)
    packet->size = off;

  return true;

failed:
  {
    LOG_WARNING ("Failed to parse");
    return false;
  }
}

/**
 * mpeg_video_parse_sequence_header:
 * @seqhdr: (out): The #MpegVideoSequenceHdr structure to fill
 * @data: The data from which to parse the sequence header
 * @size: The size of @data
 * @offset: The offset in byte from which to start parsing @data
 *
 * Parses the @seqhdr Mpeg Video Sequence Header structure members from @data
 *
 * Returns: %true if the seqhdr could be parsed correctly, %false otherwize.
 */
bool
mpeg_video_parse_sequence_header (MpegVideoSequenceHdr * seqhdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  BitReader br;
  uint8_t bits;
  uint8_t load_intra_flag, load_non_intra_flag;

  RETURN_VAL_IF_FAIL (seqhdr != NULL, false);

  size -= offset;

  if (size < 4)
    return false;

  bit_reader_init (&br, &data[offset], size);

  /* Setting the height/width codes */
  READ_UINT16 (&br, seqhdr->width, 12);
  READ_UINT16 (&br, seqhdr->height, 12);

  READ_UINT8 (&br, seqhdr->aspect_ratio_info, 4);
  /* Interpret PAR according to MPEG-1. Needs to be reinterpreted
   * later, if a sequence_display extension is seen */
  set_par_from_asr_mpeg1 (seqhdr, seqhdr->aspect_ratio_info);

  READ_UINT8 (&br, seqhdr->frame_rate_code, 4);
  set_fps_from_code (seqhdr, seqhdr->frame_rate_code);

  READ_UINT32 (&br, seqhdr->bitrate_value, 18);
  if (seqhdr->bitrate_value == 0x3ffff) {
    /* VBR stream */
    seqhdr->bitrate = 0;
  } else {
    /* Value in header is in units of 400 bps */
    seqhdr->bitrate *= 400;
  }

  READ_UINT8 (&br, bits, 1);
  if (bits != MARKER_BIT)
    goto failed;

  /* VBV buffer size */
  READ_UINT16 (&br, seqhdr->vbv_buffer_size_value, 10);

  /* constrained_parameters_flag */
  READ_UINT8 (&br, seqhdr->constrained_parameters_flag, 1);

  /* load_intra_quantiser_matrix */
  READ_UINT8 (&br, load_intra_flag, 1);
  if (load_intra_flag) {
    int32_t i;
    for (i = 0; i < 64; i++)
      READ_UINT8 (&br, seqhdr->intra_quantizer_matrix[i], 8);
  } else
    memcpy (seqhdr->intra_quantizer_matrix, default_intra_quantizer_matrix, 64);

  /* non intra quantizer matrix */
  READ_UINT8 (&br, load_non_intra_flag, 1);
  if (load_non_intra_flag) {
    int32_t i;
    for (i = 0; i < 64; i++)
      READ_UINT8 (&br, seqhdr->non_intra_quantizer_matrix[i], 8);
  } else
    memset (seqhdr->non_intra_quantizer_matrix, 16, 64);

  /* dump some info */
  LOG_INFO ("width x height: %d x %d", seqhdr->width, seqhdr->height);
  LOG_INFO ("fps: %d/%d", seqhdr->fps_n, seqhdr->fps_d);
  LOG_INFO ("par: %d/%d", seqhdr->par_w, seqhdr->par_h);
  LOG_INFO ("bitrate: %d", seqhdr->bitrate);

  return true;

  /* ERRORS */
failed:
  {
    LOG_WARNING ("Failed to parse sequence header");
    /* clear out stuff */
    memset (seqhdr, 0, sizeof (*seqhdr));
    return false;
  }
}

/**
 * mpeg_video_parse_sequence_extension:
 * @seqext: (out): The #MpegVideoSequenceExt structure to fill
 * @data: The data from which to parse the sequence extension
 * @size: The size of @data
 * @offset: The offset in byte from which to start parsing @data
 *
 * Parses the @seqext Mpeg Video Sequence Extension structure members from @data
 *
 * Returns: %true if the seqext could be parsed correctly, %false otherwize.
 */
bool
mpeg_video_parse_sequence_extension (MpegVideoSequenceExt * seqext,
    const uint8_t * data, size_t size, uint32_t offset)
{
  BitReader br;

  RETURN_VAL_IF_FAIL (seqext != NULL, false);

  size -= offset;

  if (size < 6) {
    LOG_DEBUG ("not enough bytes to parse the extension");
    return false;
  }

  bit_reader_init (&br, &data[offset], size);

  if (bit_reader_get_bits_uint8_unchecked (&br, 4) !=
      MPEG_VIDEO_PACKET_EXT_SEQUENCE) {
    LOG_DEBUG ("Not parsing a sequence extension");
    return false;
  }

  /* skip profile and level escape bit */
  bit_reader_skip_unchecked (&br, 1);

  seqext->profile = bit_reader_get_bits_uint8_unchecked (&br, 3);
  seqext->level = bit_reader_get_bits_uint8_unchecked (&br, 4);

  /* progressive */
  seqext->progressive = bit_reader_get_bits_uint8_unchecked (&br, 1);

  /* chroma format */
  seqext->chroma_format = bit_reader_get_bits_uint8_unchecked (&br, 2);

  /* resolution extension */
  seqext->horiz_size_ext = bit_reader_get_bits_uint8_unchecked (&br, 2);
  seqext->vert_size_ext = bit_reader_get_bits_uint8_unchecked (&br, 2);

  seqext->bitrate_ext = bit_reader_get_bits_uint16_unchecked (&br, 12);

  /* skip marker bits */
  bit_reader_skip_unchecked (&br, 1);

  seqext->vbv_buffer_size_extension =
      bit_reader_get_bits_uint8_unchecked (&br, 8);
  seqext->low_delay = bit_reader_get_bits_uint8_unchecked (&br, 1);

  /* framerate extension */
  seqext->fps_n_ext = bit_reader_get_bits_uint8_unchecked (&br, 2);
  seqext->fps_d_ext = bit_reader_get_bits_uint8_unchecked (&br, 2);

  return true;
}

bool
mpeg_video_parse_sequence_display_extension (MpegVideoSequenceDisplayExt
    * seqdisplayext, const uint8_t * data, size_t size, uint32_t offset)
{
  BitReader br;

  RETURN_VAL_IF_FAIL (seqdisplayext != NULL, false);
  if (offset > size)
    return false;

  size -= offset;
  if (size < 5) {
    LOG_DEBUG ("not enough bytes to parse the extension");
    return false;
  }

  bit_reader_init (&br, &data[offset], size);

  if (bit_reader_get_bits_uint8_unchecked (&br, 4) !=
      MPEG_VIDEO_PACKET_EXT_SEQUENCE_DISPLAY) {
    LOG_DEBUG ("Not parsing a sequence display extension");
    return false;
  }

  seqdisplayext->video_format =
      bit_reader_get_bits_uint8_unchecked (&br, 3);
  seqdisplayext->colour_description_flag =
      bit_reader_get_bits_uint8_unchecked (&br, 1);

  if (seqdisplayext->colour_description_flag) {
    seqdisplayext->colour_primaries =
        bit_reader_get_bits_uint8_unchecked (&br, 8);
    seqdisplayext->transfer_characteristics =
        bit_reader_get_bits_uint8_unchecked (&br, 8);
    seqdisplayext->matrix_coefficients =
        bit_reader_get_bits_uint8_unchecked (&br, 8);
  }

  if (bit_reader_get_remaining (&br) < 29) {
    LOG_DEBUG ("Not enough remaining bytes to parse the extension");
    return false;
  }

  seqdisplayext->display_horizontal_size =
      bit_reader_get_bits_uint16_unchecked (&br, 14);
  /* skip marker bit */
  bit_reader_skip_unchecked (&br, 1);
  seqdisplayext->display_vertical_size =
      bit_reader_get_bits_uint16_unchecked (&br, 14);

  return true;
}

bool
mpeg_video_finalise_mpeg2_sequence_header (MpegVideoSequenceHdr * seqhdr,
    MpegVideoSequenceExt * seqext,
    MpegVideoSequenceDisplayExt * displayext)
{
  uint32_t w;
  uint32_t h;

  if (seqext) {
    seqhdr->fps_n = seqhdr->fps_n * (seqext->fps_n_ext + 1);
    seqhdr->fps_d = seqhdr->fps_d * (seqext->fps_d_ext + 1);
    /* Extend width and height to 14 bits by adding the extension bits */
    seqhdr->width |= (seqext->horiz_size_ext << 12);
    seqhdr->height |= (seqext->vert_size_ext << 12);
  }

  w = seqhdr->width;
  h = seqhdr->height;
  if (displayext) {
    /* Use the display size for calculating PAR when display ext present */
    w = displayext->display_horizontal_size;
    h = displayext->display_vertical_size;
  }

  /* Pixel_width = DAR_width * display_vertical_size */
  /* Pixel_height = DAR_height * display_horizontal_size */
  switch (seqhdr->aspect_ratio_info) {
    case 0x01:                 /* Square pixels */
      seqhdr->par_w = seqhdr->par_h = 1;
      break;
    case 0x02:                 /* 3:4 DAR = 4:3 pixels */
      seqhdr->par_w = 4 * h;
      seqhdr->par_h = 3 * w;
      break;
    case 0x03:                 /* 9:16 DAR */
      seqhdr->par_w = 16 * h;
      seqhdr->par_h = 9 * w;
      break;
    case 0x04:                 /* 1:2.21 DAR */
      seqhdr->par_w = 221 * h;
      seqhdr->par_h = 100 * w;
      break;
    default:
      LOG_DEBUG ("unknown/invalid aspect_ratio_information %d",
          seqhdr->aspect_ratio_info);
      break;
  }

  return true;
}

/**
 * mpeg_video_parse_quant_matrix_extension:
 * @quant: (out): The #MpegVideoQuantMatrixExt structure to fill
 * @data: The data from which to parse the Quantization Matrix extension
 * @size: The size of @data
 * @offset: The offset in byte from which to start the parsing
 *
 * Parses the @quant Mpeg Video Quant Matrix Extension structure members from
 * @data
 *
 * Returns: %true if the quant matrix extension could be parsed correctly,
 * %false otherwize.
 */
bool
mpeg_video_parse_quant_matrix_extension (MpegVideoQuantMatrixExt * quant,
    const uint8_t * data, size_t size, uint32_t offset)
{
  uint8_t i;
  BitReader br;

  RETURN_VAL_IF_FAIL (quant != NULL, false);

  size -= offset;

  if (size < 1) {
    LOG_DEBUG ("not enough bytes to parse the extension");
    return false;
  }

  bit_reader_init (&br, &data[offset], size);

  if (bit_reader_get_bits_uint8_unchecked (&br, 4) !=
      MPEG_VIDEO_PACKET_EXT_QUANT_MATRIX) {
    LOG_DEBUG ("Not parsing a quant matrix extension");
    return false;
  }

  READ_UINT8 (&br, quant->load_intra_quantiser_matrix, 1);
  if (quant->load_intra_quantiser_matrix) {
    for (i = 0; i < 64; i++) {
      READ_UINT8 (&br, quant->intra_quantiser_matrix[i], 8);
    }
  }

  READ_UINT8 (&br, quant->load_non_intra_quantiser_matrix, 1);
  if (quant->load_non_intra_quantiser_matrix) {
    for (i = 0; i < 64; i++) {
      READ_UINT8 (&br, quant->non_intra_quantiser_matrix[i], 8);
    }
  }

  READ_UINT8 (&br, quant->load_chroma_intra_quantiser_matrix, 1);
  if (quant->load_chroma_intra_quantiser_matrix) {
    for (i = 0; i < 64; i++) {
      READ_UINT8 (&br, quant->chroma_intra_quantiser_matrix[i], 8);
    }
  }

  READ_UINT8 (&br, quant->load_chroma_non_intra_quantiser_matrix, 1);
  if (quant->load_chroma_non_intra_quantiser_matrix) {
    for (i = 0; i < 64; i++) {
      READ_UINT8 (&br, quant->chroma_non_intra_quantiser_matrix[i], 8);
    }
  }

  return true;

failed:
  LOG_WARNING ("error parsing \"Quant Matrix Extension\"");
  return false;
}

/**
 * mpeg_video_parse_picture_extension:
 * @ext: (out): The #MpegVideoPictureExt structure to fill
 * @data: The data from which to parse the picture extension
 * @size: The size of @data
 * @offset: The offset in byte from which to start the parsing
 *
 * Parse the @ext Mpeg Video Picture Extension structure members from @data
 *
 * Returns: %true if the picture extension could be parsed correctly,
 * %false otherwize.
 */
bool
mpeg_video_parse_picture_extension (MpegVideoPictureExt * ext,
    const uint8_t * data, size_t size, uint32_t offset)
{
  BitReader br;

  RETURN_VAL_IF_FAIL (ext != NULL, false);

  size -= offset;

  if (size < 4)
    return false;

  bit_reader_init (&br, &data[offset], size);

  if (bit_reader_get_bits_uint8_unchecked (&br, 4) !=
      MPEG_VIDEO_PACKET_EXT_PICTURE) {
    LOG_DEBUG ("Not parsing a picture extension");
    return false;
  }

  /* f_code */
  READ_UINT8 (&br, ext->f_code[0][0], 4);
  READ_UINT8 (&br, ext->f_code[0][1], 4);
  READ_UINT8 (&br, ext->f_code[1][0], 4);
  READ_UINT8 (&br, ext->f_code[1][1], 4);

  /* intra DC precision */
  READ_UINT8 (&br, ext->intra_dc_precision, 2);

  /* picture structure */
  READ_UINT8 (&br, ext->picture_structure, 2);

  /* top field first */
  READ_UINT8 (&br, ext->top_field_first, 1);

  /* frame pred frame dct */
  READ_UINT8 (&br, ext->frame_pred_frame_dct, 1);

  /* concealment motion vectors */
  READ_UINT8 (&br, ext->concealment_motion_vectors, 1);

  /* q scale type */
  READ_UINT8 (&br, ext->q_scale_type, 1);

  /* intra vlc format */
  READ_UINT8 (&br, ext->intra_vlc_format, 1);

  /* alternate scan */
  READ_UINT8 (&br, ext->alternate_scan, 1);

  /* repeat first field */
  READ_UINT8 (&br, ext->repeat_first_field, 1);

  /* chroma_420_type */
  READ_UINT8 (&br, ext->chroma_420_type, 1);

  /* progressive_frame */
  READ_UINT8 (&br, ext->progressive_frame, 1);

  /* composite display */
  READ_UINT8 (&br, ext->composite_display, 1);

  if (ext->composite_display) {

    /* v axis */
    READ_UINT8 (&br, ext->v_axis, 1);

    /* field sequence */
    READ_UINT8 (&br, ext->field_sequence, 3);

    /* sub carrier */
    READ_UINT8 (&br, ext->sub_carrier, 1);

    /* burst amplitude */
    READ_UINT8 (&br, ext->burst_amplitude, 7);

    /* sub_carrier phase */
    READ_UINT8 (&br, ext->sub_carrier_phase, 8);
  }

  return true;

failed:
  LOG_WARNING ("error parsing \"Picture Coding Extension\"");
  return false;

}

/**
 * mpeg_video_parse_picture_header:
 * @hdr: (out): The #MpegVideoPictureHdr structure to fill
 * @data: The data from which to parse the picture header
 * @size: The size of @data
 * @offset: The offset in byte from which to start the parsing
 *
 * Parsers the @hdr Mpeg Video Picture Header structure members from @data
 *
 * Returns: %true if the picture sequence could be parsed correctly, %false
 * otherwize.
 */
bool
mpeg_video_parse_picture_header (MpegVideoPictureHdr * hdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  BitReader br;

  size = size - offset;

  if (size < 4)
    goto failed;

  bit_reader_init (&br, &data[offset], size);

  /* temperal sequence number */
  if (!bit_reader_get_bits_uint16 (&br, &hdr->tsn, 10))
    goto failed;


  /* frame type */
  if (!bit_reader_get_bits_uint8 (&br, (uint8_t *) & hdr->pic_type, 3))
    goto failed;


  if (hdr->pic_type == 0 || hdr->pic_type > 4)
    goto failed;                /* Corrupted picture packet */

  /* skip VBV delay */
  if (!bit_reader_skip (&br, 16))
    goto failed;

  if (hdr->pic_type == MPEG_VIDEO_PICTURE_TYPE_P
      || hdr->pic_type == MPEG_VIDEO_PICTURE_TYPE_B) {

    READ_UINT8 (&br, hdr->full_pel_forward_vector, 1);

    READ_UINT8 (&br, hdr->f_code[0][0], 3);
    hdr->f_code[0][1] = hdr->f_code[0][0];
  } else {
    hdr->full_pel_forward_vector = 0;
    hdr->f_code[0][0] = hdr->f_code[0][1] = 0;
  }

  if (hdr->pic_type == MPEG_VIDEO_PICTURE_TYPE_B) {
    READ_UINT8 (&br, hdr->full_pel_backward_vector, 1);

    READ_UINT8 (&br, hdr->f_code[1][0], 3);
    hdr->f_code[1][1] = hdr->f_code[1][0];
  } else {
    hdr->full_pel_backward_vector = 0;
    hdr->f_code[1][0] = hdr->f_code[1][1] = 0;
  }

  return true;

failed:
  {
    LOG_WARNING ("Failed to parse picture header");
    return false;
  }
}

/**
 * mpeg_video_parse_gop:
 * @gop: (out): The #MpegVideoGop structure to fill
 * @data: The data from which to parse the gop
 * @size: The size of @data
 * @offset: The offset in byte from which to start the parsing
 *
 * Parses the @gop Mpeg Video Group of Picture structure members from @data
 *
 * Returns: %true if the gop could be parsed correctly, %false otherwize.
 */
bool
mpeg_video_parse_gop (MpegVideoGop * gop, const uint8_t * data,
    size_t size, uint32_t offset)
{
  BitReader br;

  RETURN_VAL_IF_FAIL (gop != NULL, false);

  size -= offset;

  if (size < 4)
    return false;

  bit_reader_init (&br, &data[offset], size);

  READ_UINT8 (&br, gop->drop_frame_flag, 1);

  READ_UINT8 (&br, gop->hour, 5);

  READ_UINT8 (&br, gop->minute, 6);

  /* skip unused bit */
  if (!bit_reader_skip (&br, 1))
    return false;

  READ_UINT8 (&br, gop->second, 6);

  READ_UINT8 (&br, gop->frame, 6);

  READ_UINT8 (&br, gop->closed_gop, 1);

  READ_UINT8 (&br, gop->broken_link, 1);

  return true;

failed:
  LOG_WARNING ("error parsing \"GOP\"");
  return false;
}

/**
 * mpeg_video_quant_matrix_get_raster_from_zigzag:
 * @out_quant: (out): The resulting quantization matrix
 * @quant: The source quantization matrix
 *
 * Converts quantization matrix @quant from zigzag scan order to
 * raster scan order and store the resulting factors into @out_quant.
 *
 * Note: it is an error to pass the same table in both @quant and
 * @out_quant arguments.
 *
 * Since: 1.2
 */
void
mpeg_video_quant_matrix_get_raster_from_zigzag (uint8_t out_quant[64],
    const uint8_t quant[64])
{
  uint32_t i;

  RETURN_IF_FAIL (out_quant != quant);

  for (i = 0; i < 64; i++)
    out_quant[mpeg_zigzag_8x8[i]] = quant[i];
}

/**
 * mpeg_video_quant_matrix_get_zigzag_from_raster:
 * @out_quant: (out): The resulting quantization matrix
 * @quant: The source quantization matrix
 *
 * Converts quantization matrix @quant from raster scan order to
 * zigzag scan order and store the resulting factors into @out_quant.
 *
 * Note: it is an error to pass the same table in both @quant and
 * @out_quant arguments.
 *
 * Since: 1.2
 */
void
mpeg_video_quant_matrix_get_zigzag_from_raster (uint8_t out_quant[64],
    const uint8_t quant[64])
{
  uint32_t i;

  RETURN_IF_FAIL (out_quant != quant);

  for (i = 0; i < 64; i++)
    out_quant[i] = quant[mpeg_zigzag_8x8[i]];
}
