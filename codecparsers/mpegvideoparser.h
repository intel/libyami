/* Gstreamer
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

#ifndef __MPEG_VIDEO_UTILS_H__
#define __MPEG_VIDEO_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>

/**
 * MpegVideoPacketTypeCode:
 * @MPEG_VIDEO_PACKET_PICTURE: Picture packet starting code
 * @MPEG_VIDEO_PACKET_SLICE_MIN: Slice min packet starting code
 * @MPEG_VIDEO_PACKET_SLICE_MAX: Slice max packet starting code
 * @MPEG_VIDEO_PACKET_USER_DATA: User data packet starting code
 * @MPEG_VIDEO_PACKET_SEQUENCE : Sequence packet starting code
 * @MPEG_VIDEO_PACKET_EXTENSION: Extension packet starting code
 * @MPEG_VIDEO_PACKET_SEQUENCE_END: Sequence end packet code
 * @MPEG_VIDEO_PACKET_GOP: Group of Picture packet starting code
 * @MPEG_VIDEO_PACKET_NONE: None packet code
 *
 * Indicates the type of MPEG packet
 */
typedef enum {
  MPEG_VIDEO_PACKET_PICTURE      = 0x00,
  MPEG_VIDEO_PACKET_SLICE_MIN    = 0x01,
  MPEG_VIDEO_PACKET_SLICE_MAX    = 0xaf,
  MPEG_VIDEO_PACKET_USER_DATA    = 0xb2,
  MPEG_VIDEO_PACKET_SEQUENCE     = 0xb3,
  MPEG_VIDEO_PACKET_EXTENSION    = 0xb5,
  MPEG_VIDEO_PACKET_SEQUENCE_END = 0xb7,
  MPEG_VIDEO_PACKET_GOP          = 0xb8,
  MPEG_VIDEO_PACKET_NONE         = 0xff
} MpegVideoPacketTypeCode;

/**
 * MPEG_VIDEO_PACKET_IS_SLICE:
 * @typecode: The MPEG video packet type code
 *
 * Checks whether a packet type code is a slice.
 *
 * Returns: %true if the packet type code corresponds to a slice,
 * else %false.
 */
#define MPEG_VIDEO_PACKET_IS_SLICE(typecode) ((typecode) >= MPEG_VIDEO_PACKET_SLICE_MIN && \
						  (typecode) <= MPEG_VIDEO_PACKET_SLICE_MAX)

/**
 * MpegVideoPacketExtensionCode:
 * @MPEG_VIDEO_PACKET_EXT_SEQUENCE: Sequence extension code
 * @MPEG_VIDEO_PACKET_EXT_SEQUENCE_DISPLAY: Sequence Display extension code
 * @MPEG_VIDEO_PACKET_EXT_QUANT_MATRIX: Quantization Matrix extension code
 * @MPEG_VIDEO_PACKET_EXT_PICTURE: Picture coding extension
 *
 * Indicates what type of packets are in this block, some are mutually
 * exclusive though - ie, sequence packs are accumulated separately. GOP &
 * Picture may occur together or separately.
 */
typedef enum {
  MPEG_VIDEO_PACKET_EXT_SEQUENCE         = 0x01,
  MPEG_VIDEO_PACKET_EXT_SEQUENCE_DISPLAY = 0x02,
  MPEG_VIDEO_PACKET_EXT_QUANT_MATRIX     = 0x03,
  MPEG_VIDEO_PACKET_EXT_PICTURE          = 0x08
} MpegVideoPacketExtensionCode;

/**
 * MpegVideoLevel:
 * @MPEG_VIDEO_LEVEL_LOW: Low level (LL)
 * @MPEG_VIDEO_LEVEL_MAIN: Main level (ML)
 * @MPEG_VIDEO_LEVEL_HIGH_1440: High 1440 level (H-14)
 * @MPEG_VIDEO_LEVEL_HIGH: High level (HL)
 *
 * Mpeg-2 Levels.
 **/
typedef enum {
 MPEG_VIDEO_LEVEL_HIGH      = 0x04,
 MPEG_VIDEO_LEVEL_HIGH_1440 = 0x06,
 MPEG_VIDEO_LEVEL_MAIN      = 0x08,
 MPEG_VIDEO_LEVEL_LOW       = 0x0a
} MpegVideoLevel;

/**
 * MpegVideoProfile:
 * @MPEG_VIDEO_PROFILE_422: 4:2:2 profile (422)
 * @MPEG_VIDEO_PROFILE_HIGH: High profile (HP)
 * @MPEG_VIDEO_PROFILE_SPATIALLY_SCALABLE: Spatially Scalable profile (Spatial)
 * @MPEG_VIDEO_PROFILE_SNR_SCALABLE: SNR Scalable profile (SNR)
 * @MPEG_VIDEO_PROFILE_MAIN: Main profile (MP)
 * @MPEG_VIDEO_PROFILE_SIMPLE: Simple profile (SP)
 *
 * Mpeg-2 Profiles.
 **/
typedef enum {
  MPEG_VIDEO_PROFILE_422                 = 0x00,
  MPEG_VIDEO_PROFILE_HIGH                = 0x01,
  MPEG_VIDEO_PROFILE_SPATIALLY_SCALABLE  = 0x02,
  MPEG_VIDEO_PROFILE_SNR_SCALABLE        = 0x03,
  MPEG_VIDEO_PROFILE_MAIN                = 0x04,
  MPEG_VIDEO_PROFILE_SIMPLE              = 0x05
} MpegVideoProfile;

/**
 * MpegVideoChromaFormat:
 * @MPEG_VIDEO_CHROMA_RES: Invalid (reserved for future use)
 * @MPEG_VIDEO_CHROMA_420: 4:2:0 subsampling
 * @MPEG_VIDEO_CHROMA_422: 4:2:2 subsampling
 * @MPEG_VIDEO_CHROMA_444: 4:4:4 (non-subsampled)
 *
 * Chroma subsampling type.
 */
typedef enum {
  MPEG_VIDEO_CHROMA_RES = 0x00,
  MPEG_VIDEO_CHROMA_420 = 0x01,
  MPEG_VIDEO_CHROMA_422 = 0x02,
  MPEG_VIDEO_CHROMA_444 = 0x03,
} MpegVideoChromaFormat;

/**
 * MpegVideoPictureType:
 * @MPEG_VIDEO_PICTURE_TYPE_I: Intra-coded (I) frame
 * @MPEG_VIDEO_PICTURE_TYPE_P: Predictive-codec (P) frame
 * @MPEG_VIDEO_PICTURE_TYPE_B: Bidirectionally predictive-coded (B) frame
 * @MPEG_VIDEO_PICTURE_TYPE_D: D frame
 *
 * Picture type.
 */
typedef enum {
  MPEG_VIDEO_PICTURE_TYPE_I = 0x01,
  MPEG_VIDEO_PICTURE_TYPE_P = 0x02,
  MPEG_VIDEO_PICTURE_TYPE_B = 0x03,
  MPEG_VIDEO_PICTURE_TYPE_D = 0x04
} MpegVideoPictureType;

/**
 * MpegVideoPictureStructure:
 * @MPEG_VIDEO_PICTURE_STRUCTURE_TOP_FIELD: Top field
 * @MPEG_VIDEO_PICTURE_STRUCTURE_BOTTOM_FIELD: Bottom field
 * @MPEG_VIDEO_PICTURE_STRUCTURE_FRAME: Frame picture
 *
 * Picture structure type.
 */
typedef enum {
    MPEG_VIDEO_PICTURE_STRUCTURE_TOP_FIELD    = 0x01,
    MPEG_VIDEO_PICTURE_STRUCTURE_BOTTOM_FIELD = 0x02,
    MPEG_VIDEO_PICTURE_STRUCTURE_FRAME        = 0x03
} MpegVideoPictureStructure;

typedef struct _MpegVideoSequenceHdr     MpegVideoSequenceHdr;
typedef struct _MpegVideoSequenceExt     MpegVideoSequenceExt;
typedef struct _MpegVideoSequenceDisplayExt MpegVideoSequenceDisplayExt;
typedef struct _MpegVideoPictureHdr      MpegVideoPictureHdr;
typedef struct _MpegVideoGop             MpegVideoGop;
typedef struct _MpegVideoPictureExt      MpegVideoPictureExt;
typedef struct _MpegVideoQuantMatrixExt  MpegVideoQuantMatrixExt;
typedef struct _MpegVideoPacket          MpegVideoPacket;

/**
 * MpegVideoSequenceHdr:
 * @width: Width of each frame
 * @height: Height of each frame
 * @par_w: Calculated Pixel Aspect Ratio width
 * @par_h: Calculated Pixel Aspect Ratio height
 * @fps_n: Calculated Framrate nominator
 * @fps_d: Calculated Framerate denominator
 * @bitrate_value: Value of the bitrate as is in the stream (400bps unit)
 * @bitrate: the real bitrate of the Mpeg video stream in bits per second, 0 if VBR stream
 * @constrained_parameters_flag: %true if this stream uses contrained parameters.
 * @intra_quantizer_matrix: intra-quantization table, in zigzag scan order
 * @non_intra_quantizer_matrix: non-intra quantization table, in zigzag scan order
 *
 * The Mpeg2 Video Sequence Header structure.
 */
struct _MpegVideoSequenceHdr
{
  uint16_t width, height;
  uint8_t  aspect_ratio_info;
  uint8_t  frame_rate_code;
  uint32_t bitrate_value;
  uint16_t vbv_buffer_size_value;

  uint8_t  constrained_parameters_flag;

  uint8_t  intra_quantizer_matrix[64];
  uint8_t  non_intra_quantizer_matrix[64];

  /* Calculated values */
  uint32_t   par_w, par_h;
  uint32_t   fps_n, fps_d;
  uint32_t   bitrate;
};

/**
 * MpegVideoSequenceExt:
 * @profile: mpeg2 decoder profile
 * @level: mpeg2 decoder level
 * @progressive: %true if the frames are progressive %false otherwise
 * @chroma_format: indicates the chrominance format
 * @horiz_size_ext: Horizontal size
 * @vert_size_ext: Vertical size
 * @bitrate_ext: The bitrate
 * @vbv_buffer_size_extension: VBV vuffer size
 * @low_delay: %true if the sequence doesn't contain any B-pictures, %false
 * otherwise
 * @fps_n_ext: Framerate nominator code
 * @fps_d_ext: Framerate denominator code
 *
 * The Mpeg2 Video Sequence Extension structure.
 **/
struct _MpegVideoSequenceExt
{
  /* mpeg2 decoder profile */
  uint8_t profile;
  /* mpeg2 decoder level */
  uint8_t level;

  uint8_t progressive;
  uint8_t chroma_format;

  uint8_t horiz_size_ext, vert_size_ext;

  uint16_t bitrate_ext;
  uint8_t vbv_buffer_size_extension;
  uint8_t low_delay;
  uint8_t fps_n_ext, fps_d_ext;

};

/**
 * MpegVideoSequenceDisplayExt:
 * @profile: mpeg2 decoder profil

 */
struct _MpegVideoSequenceDisplayExt
{
  uint8_t video_format;
  uint8_t colour_description_flag;

  /* if colour_description_flag: */
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;

  uint16_t display_horizontal_size;
  uint16_t display_vertical_size;
};

/**
 * MpegVideoQuantMatrixExt:
 * @load_intra_quantiser_matrix:
 * @intra_quantiser_matrix:
 * @load_non_intra_quantiser_matrix:
 * @non_intra_quantiser_matrix:
 * @load_chroma_intra_quantiser_matrix:
 * @chroma_intra_quantiser_matrix:
 * @load_chroma_non_intra_quantiser_matrix:
 * @chroma_non_intra_quantiser_matrix:
 *
 * The Quant Matrix Extension structure that exposes quantization
 * matrices in zigzag scan order. i.e. the original encoded scan
 * order.
 */
struct _MpegVideoQuantMatrixExt
{
 uint8_t load_intra_quantiser_matrix;
 uint8_t intra_quantiser_matrix[64];
 uint8_t load_non_intra_quantiser_matrix;
 uint8_t non_intra_quantiser_matrix[64];
 uint8_t load_chroma_intra_quantiser_matrix;
 uint8_t chroma_intra_quantiser_matrix[64];
 uint8_t load_chroma_non_intra_quantiser_matrix;
 uint8_t chroma_non_intra_quantiser_matrix[64];
};

/**
 * MpegVideoPictureHdr:
 * @tsn: Temporal Sequence Number
 * @pic_type: Type of the frame
 * @full_pel_forward_vector: the full pel forward flag of
 *  the frame: 0 or 1.
 * @full_pel_backward_vector: the full pel backward flag
 *  of the frame: 0 or 1.
 * @f_code: F code
 *
 * The Mpeg2 Video Picture Header structure.
 */
struct _MpegVideoPictureHdr
{
  uint16_t tsn;
  uint8_t pic_type;

  uint8_t full_pel_forward_vector, full_pel_backward_vector;

  uint8_t f_code[2][2];
};

/**
 * MpegVideoPictureExt:
 * @intra_dc_precision: Intra DC precision
 * @picture_structure: Structure of the picture
 * @top_field_first: Top field first
 * @frame_pred_frame_dct: Frame
 * @concealment_motion_vectors: Concealment Motion Vectors
 * @q_scale_type: Q Scale Type
 * @intra_vlc_format: Intra Vlc Format
 * @alternate_scan: Alternate Scan
 * @repeat_first_field: Repeat First Field
 * @chroma_420_type: Chroma 420 Type
 * @progressive_frame: %true if the frame is progressive %false otherwize
 *
 * The Mpeg2 Video Picture Extension structure.
 */
struct _MpegVideoPictureExt
{
  uint8_t f_code[2][2];

  uint8_t intra_dc_precision;
  uint8_t picture_structure;
  uint8_t top_field_first;
  uint8_t frame_pred_frame_dct;
  uint8_t concealment_motion_vectors;
  uint8_t q_scale_type;
  uint8_t intra_vlc_format;
  uint8_t alternate_scan;
  uint8_t repeat_first_field;
  uint8_t chroma_420_type;
  uint8_t progressive_frame;
  uint8_t composite_display;
  uint8_t v_axis;
  uint8_t field_sequence;
  uint8_t sub_carrier;
  uint8_t burst_amplitude;
  uint8_t sub_carrier_phase;
};

/**
 * MpegVideoGop:
 * @drop_frame_flag: Drop Frame Flag
 * @hour: Hour (0-23)
 * @minute: Minute (O-59)
 * @second: Second (0-59)
 * @frame: Frame (0-59)
 * @closed_gop: Closed Gop
 * @broken_link: Broken link
 *
 * The Mpeg Video Group of Picture structure.
 */
struct _MpegVideoGop
{
  uint8_t drop_frame_flag;

  uint8_t hour, minute, second, frame;

  uint8_t closed_gop;
  uint8_t broken_link;
};

/**
 * MpegVideoTypeOffsetSize:
 *
 * @type: the type of the packet that start at @offset
 * @data: the data containing the packet starting at @offset
 * @offset: the offset of the packet start in bytes, it is the exact, start of the packet, no sync code included
 * @size: The size in bytes of the packet or -1 if the end wasn't found. It is the exact size of the packet, no sync code included
 *
 * A structure that contains the type of a packet, its offset and its size
 */
struct _MpegVideoPacket
{
  const uint8_t *data;
  uint8_t type;
  uint32_t  offset;
  int32_t   size;
};

bool gst_mpeg_video_parse                         (MpegVideoPacket * packet,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_sequence_header         (MpegVideoSequenceHdr * params,
                                                       const uint8_t * data, size_t size, uint32_t offset);

/* seqext and displayext may be NULL if not received */
bool gst_mpeg_video_finalise_mpeg2_sequence_header (MpegVideoSequenceHdr *hdr,
   MpegVideoSequenceExt *seqext, MpegVideoSequenceDisplayExt *displayext);

bool gst_mpeg_video_parse_picture_header          (MpegVideoPictureHdr* hdr,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_picture_extension       (MpegVideoPictureExt *ext,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_gop                     (MpegVideoGop * gop,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_sequence_extension      (MpegVideoSequenceExt * seqext,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_sequence_display_extension (MpegVideoSequenceDisplayExt * seqdisplayext,
                                                       const uint8_t * data, size_t size, uint32_t offset);

bool gst_mpeg_video_parse_quant_matrix_extension  (MpegVideoQuantMatrixExt * quant,
                                                       const uint8_t * data, size_t size, uint32_t offset);

void gst_mpeg_video_quant_matrix_get_raster_from_zigzag (uint8_t out_quant[64],
                                                             const uint8_t quant[64]);

void gst_mpeg_video_quant_matrix_get_zigzag_from_raster (uint8_t out_quant[64],

#ifdef __cplusplus
}
#endif /* __cplusplus */
                                                            const uint8_t quant[64]);
#endif
