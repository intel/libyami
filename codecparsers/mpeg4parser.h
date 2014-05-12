/*
 * GStreamer
 * Copyright (C) 2009 Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>
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

#ifndef __MPEG4_PARSER_H__
#define __MPEG4_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "bitreader.h"
#include <stdint.h>

typedef struct _Mpeg4VisualObjectSequence    Mpeg4VisualObjectSequence;
typedef struct _Mpeg4VisualObject            Mpeg4VisualObject;
typedef struct _Mpeg4VideoObjectLayer        Mpeg4VideoObjectLayer;
typedef struct _Mpeg4GroupOfVOP              Mpeg4GroupOfVOP;
typedef struct _Mpeg4VideoObjectPlane        Mpeg4VideoObjectPlane;
typedef struct _Mpeg4VideoSignalType         Mpeg4VideoSignalType;
typedef struct _Mpeg4VideoPlaneShortHdr      Mpeg4VideoPlaneShortHdr;
typedef struct _Mpeg4VideoPacketHdr          Mpeg4VideoPacketHdr;

typedef struct _Mpeg4SpriteTrajectory        Mpeg4SpriteTrajectory;

typedef struct _Mpeg4Packet                  Mpeg4Packet;

/**
 * Mpeg4StartCode:
 *
 * Defines the different startcodes present in the bitstream as
 * defined in: Table 6-3 — Start code values
 */
typedef enum
{
  MPEG4_VIDEO_OBJ_FIRST      = 0x00,
  MPEG4_VIDEO_OBJ_LAST       = 0x1f,
  MPEG4_VIDEO_LAYER_FIRST    = 0x20,
  MPEG4_VIDEO_LAYER_LAST     = 0x2f,
  MPEG4_VISUAL_OBJ_SEQ_START = 0xb0,
  MPEG4_VISUAL_OBJ_SEQ_END   = 0xb1,
  MPEG4_USER_DATA            = 0xb2,
  MPEG4_GROUP_OF_VOP         = 0xb3,
  MPEG4_VIDEO_SESSION_ERR    = 0xb4,
  MPEG4_VISUAL_OBJ           = 0xb5,
  MPEG4_VIDEO_OBJ_PLANE      = 0xb6,
  MPEG4_FBA                  = 0xba,
  MPEG4_FBA_PLAN             = 0xbb,
  MPEG4_MESH                 = 0xbc,
  MPEG4_MESH_PLAN            = 0xbd,
  MPEG4_STILL_TEXTURE_OBJ    = 0xbe,
  MPEG4_TEXTURE_SPATIAL      = 0xbf,
  MPEG4_TEXTURE_SNR_LAYER    = 0xc0,
  MPEG4_TEXTURE_TILE         = 0xc1,
  MPEG4_SHAPE_LAYER          = 0xc2,
  MPEG4_STUFFING             = 0xc3,
  MPEG4_SYSTEM_FIRST         = 0xc6,
  MPEG4_SYSTEM_LAST          = 0xff,
  MPEG4_RESYNC               = 0xfff
} Mpeg4StartCode;

/**
 * Mpeg4VisualObjectType:
 *
 * Defines the different visual object types as
 * defined in: Table 6-5 -- Meaning of visual object type
 */
typedef enum {
  MPEG4_VIDEO_ID         = 0x01,
  MPEG4_STILL_TEXTURE_ID = 0x02,
  MPEG4_STILL_MESH_ID    = 0x03,
  MPEG4_STILL_FBA_ID     = 0x04,
  MPEG4_STILL_3D_MESH_ID = 0x05,
  /*... reserved */

} Mpeg4VisualObjectType;

/**
 * Mpeg4AspectRatioInfo:
 * @MPEG4_SQUARE: 1:1 square
 * @MPEG4_625_TYPE_4_3: 12:11 (625-type for 4:3 picture)
 * @MPEG4_525_TYPE_4_3: 10:11 (525-type for 4:3 picture)
 * @MPEG4_625_TYPE_16_9: 16:11 (625-type stretched for 16:9 picture)
 * @MPEG4_525_TYPE_16_9: 40:33 (525-type stretched for 16:9 picture)
 * @MPEG4_EXTENDED_PAR: Extended par
 *
 * Defines the different pixel aspect ratios as
 * defined in: Table 6-12 -- Meaning of pixel aspect ratio
 */
typedef enum {
  MPEG4_SQUARE        = 0x01,
  MPEG4_625_TYPE_4_3  = 0x02,
  MPEG4_525_TYPE_4_3  = 0x03,
  MPEG4_625_TYPE_16_9 = 0x04,
  MPEG4_525_TYPE_16_9 = 0x05,
  MPEG4_EXTENDED_PAR  = 0x0f,
} Mpeg4AspectRatioInfo;

/**
 * Mpeg4ParseResult:
 * @MPEG4_PARSER_OK: The parsing went well
 * @MPEG4_PARSER_BROKEN_DATA: The bitstream was broken
 * @MPEG4_PARSER_NO_PACKET: There was no packet in the buffer
 * @MPEG4_PARSER_NO_PACKET_END: There was no packet end in the buffer
 * @MPEG4_PARSER_NO_PACKET_ERROR: An error accured durint the parsing
 *
 * Result type of any parsing function.
 */
typedef enum {
  MPEG4_PARSER_OK,
  MPEG4_PARSER_BROKEN_DATA,
  MPEG4_PARSER_NO_PACKET,
  MPEG4_PARSER_NO_PACKET_END,
  MPEG4_PARSER_ERROR,
} Mpeg4ParseResult;

/**
 * Mpeg4VideoObjectCodingType:
 * @MPEG4_I_VOP: intra-coded (I)
 * @MPEG4_P_VOP: predictive-coded (P)
 * @MPEG4_B_VOP: bidirectionally-predictive-coded (B)
 * @MPEG4_S_VOP: sprite (S)
 *
 * The vop coding types as defined in:
 * Table 6-20 -- Meaning of vop_coding_type
 */
typedef enum {
  MPEG4_I_VOP = 0x0,
  MPEG4_P_VOP = 0x1,
  MPEG4_B_VOP = 0x2,
  MPEG4_S_VOP = 0x3
} Mpeg4VideoObjectCodingType;

/**
 * Mpeg4ChromaFormat
 *
 * The chroma format in use as
 * defined in: Table 6-13 -- Meaning of chroma_format
 */
typedef enum {
  /* Other value are reserved */
  MPEG4_CHROMA_4_2_0 = 0x01
} Mpeg4ChromaFormat;

/**
 * Mpeg4VideoObjectLayerShape:
 *
 * The different video object layer shapes as defined in:
 * Table 6-16 — Video Object Layer shape type
 */
typedef enum {
  MPEG4_RECTANGULAR,
  MPEG4_BINARY,
  MPEG4_BINARY_ONLY,
  MPEG4_GRAYSCALE
} Mpeg4VideoObjectLayerShape;

/**
 * Mpeg4SpriteEnable:
 *
 * Indicates the usage of static sprite coding
 * or global motion compensation (GMC) as defined in:
 * Table V2 - 2 -- Meaning of sprite_enable codewords
 */
typedef enum {
  MPEG4_SPRITE_UNUSED,
  MPEG4_SPRITE_STATIC,
  MPEG4_SPRITE_GMG
} Mpeg4SpriteEnable;

/**
 * Mpeg4Profile:
 *
 * Different defined profiles as defined in:
 * 9- Profiles and levels
 *
 * It is computed using:
 * Table G.1 — FLC table for profile_and_level_indication
 */
typedef enum {
  MPEG4_PROFILE_CORE,
  MPEG4_PROFILE_MAIN,
  MPEG4_PROFILE_N_BIT,
  MPEG4_PROFILE_SIMPLE,
  MPEG4_PROFILE_HYBRID,
  MPEG4_PROFILE_RESERVED,
  MPEG4_PROFILE_SIMPLE_FBA,
  MPEG4_PROFILE_CORE_STUDIO,
  MPEG4_PROFILE_SIMPLE_STUDIO,
  MPEG4_PROFILE_CORE_SCALABLE,
  MPEG4_PROFILE_ADVANCED_CORE,
  MPEG4_PROFILE_ADVANCED_SIMPLE,
  MPEG4_PROFILE_SIMPLE_SCALABLE,
  MPEG4_PROFILE_SCALABLE_TEXTURE,
  MPEG4_PROFILE_SIMPLE_FACE_ANIMATION,
  MPEG4_PROFILE_BASIC_ANIMATED_TEXTURE,
  MPEG4_PROFILE_ADVANCED_REALTIME_SIMPLE,
  MPEG4_PROFILE_ADVANCED_SCALABLE_TEXTURE,
  MPEG4_PROFILE_FINE_GRANULARITY_SCALABLE,
  MPEG4_PROFILE_ADVANCED_CODING_EFFICIENCY
} Mpeg4Profile;

/**
 * Mpeg4Level:
 *
 * Different levels as defined in:
 * 9- Profiles and levels
 *
 * It is computed using:
 * Table G.1 — FLC table for profile_and_level_indication
 */
typedef enum {
  MPEG4_LEVEL0,
  MPEG4_LEVEL1,
  MPEG4_LEVEL2,
  MPEG4_LEVEL3,
  MPEG4_LEVEL3b,
  MPEG4_LEVEL4,
  MPEG4_LEVEL5,
  MPEG4_LEVEL_RESERVED
} Mpeg4Level;

/**
 * Mpeg4VisualObjectSequence:
 *
 * The visual object sequence structure as defined in:
 * 6.2.2 Visual Object Sequence and Visual Object
 */
struct _Mpeg4VisualObjectSequence {
  uint8_t profile_and_level_indication;

  /* Computed according to:
   * Table G.1 — FLC table for profile_and_level_indication */
  Mpeg4Level level;
  Mpeg4Profile profile;
};

/**
 * The visual object structure as defined in:
 * 6.2.2 Visual Object Sequence and Visual Object
 */
struct _Mpeg4VisualObject {
  uint8_t is_identifier;
  /* If is_identifier */
  uint8_t verid;
  uint8_t priority;

  Mpeg4VisualObjectType type;
};

/**
 * Mpeg4VideoSignalType:
 *
 * The video signal type structure as defined in:
 * 6.2.2 Visual Object Sequence and Visual Object.
 */
struct _Mpeg4VideoSignalType {
  uint8_t type;

  uint8_t format;
  uint8_t range;
  uint8_t color_description;
  uint8_t color_primaries;
  uint8_t transfer_characteristics;
  uint8_t matrix_coefficients;
};

/**
 * Mpeg4VideoPlaneShortHdr:
 *
 * The video plane short header structure as defined in:
 * 6.2.5.2 Video Plane with Short Header
 */
struct _Mpeg4VideoPlaneShortHdr {
  uint8_t temporal_reference;
  uint8_t split_screen_indicator;
  uint8_t document_camera_indicator;
  uint8_t full_picture_freeze_release;
  uint8_t source_format;
  uint8_t picture_coding_type;
  uint8_t vop_quant;
  uint8_t pei;
  uint8_t psupp;

  /*  Gob layer specific fields */
  uint8_t gob_header_empty;
  uint8_t gob_number;
  uint8_t gob_frame_id;
  uint8_t quant_scale;

  /* Computed
   * If all the values are set to 0, then it is reserved
   * Table 6-25 -- Parameters Defined by source_format Field
   */
  uint16_t vop_width;
  uint16_t vop_height;
  uint16_t num_macroblocks_in_gob;
  uint8_t num_gobs_in_vop;

  /* The size in bits */
  uint32_t size;
};

/**
 * Mpeg4VideoObjectLayer:
 *
 * The video object layer structure as defined in:
 * 6.2.3 Video Object Layer
 */
struct _Mpeg4VideoObjectLayer {
  uint8_t random_accessible_vol;
  uint8_t video_object_type_indication;

  uint8_t is_object_layer_identifier;
  /* if is_object_layer_identifier */
  uint8_t verid;
  uint8_t priority;

  Mpeg4AspectRatioInfo aspect_ratio_info;
  uint8_t par_width;
  uint8_t par_height;

  uint8_t control_parameters;
  /* if control_parameters */
  Mpeg4ChromaFormat chroma_format;
  uint8_t low_delay;
  uint8_t vbv_parameters;
  /* if vbv_parameters */
  uint16_t first_half_bitrate;
  uint16_t latter_half_bitrate;
  uint16_t first_half_vbv_buffer_size;
  uint16_t latter_half_vbv_buffer_size;
  uint16_t first_half_vbv_occupancy;
  uint16_t latter_half_vbv_occupancy;

  /* Computed values */
  uint32_t bit_rate;
  uint32_t vbv_buffer_size;

  Mpeg4VideoObjectLayerShape shape;
  /* if shape == MPEG4_GRAYSCALE && verid =! 1 */
  uint8_t shape_extension;

  uint16_t vop_time_increment_resolution;
  uint8_t vop_time_increment_bits;
  uint8_t fixed_vop_rate;
  /* if fixed_vop_rate */
  uint16_t fixed_vop_time_increment;

  uint16_t width;
  uint16_t height;
  uint8_t interlaced;
  uint8_t obmc_disable;

  Mpeg4SpriteEnable sprite_enable;
  /* if vol->sprite_enable == SPRITE_GMG or SPRITE_STATIC*/
  /* if vol->sprite_enable != MPEG4_SPRITE_GMG */
  uint16_t sprite_width;
  uint16_t sprite_height;
  uint16_t sprite_left_coordinate;
  uint16_t sprite_top_coordinate;

  uint8_t no_of_sprite_warping_points;
  uint8_t sprite_warping_accuracy;
  uint8_t sprite_brightness_change;
  /* if vol->sprite_enable != MPEG4_SPRITE_GMG */
  uint8_t low_latency_sprite_enable;

  /* if shape != MPEG4_RECTANGULAR */
  uint8_t sadct_disable;

  uint8_t not_8_bit;

  /* if no_8_bit */
  uint8_t quant_precision;
  uint8_t bits_per_pixel;

  /* if shape == GRAYSCALE */
  uint8_t no_gray_quant_update;
  uint8_t composition_method;
  uint8_t linear_composition;

  uint8_t quant_type;
  /* if quant_type */
  uint8_t load_intra_quant_mat;
  uint8_t intra_quant_mat[64];
  uint8_t load_non_intra_quant_mat;
  uint8_t non_intra_quant_mat[64];

  uint8_t quarter_sample;
  uint8_t complexity_estimation_disable;
  uint8_t resync_marker_disable;
  uint8_t data_partitioned;
  uint8_t reversible_vlc;
  uint8_t newpred_enable;
  uint8_t reduced_resolution_vop_enable;
  uint8_t scalability;
  uint8_t enhancement_type;

  Mpeg4VideoPlaneShortHdr short_hdr;
};

/**
 * Mpeg4SpriteTrajectory:
 *
 * The sprite trajectory structure as defined in:
 * 7.8.4 Sprite reference point decoding and
 * 6.2.5.4 Sprite coding
 */
struct _Mpeg4SpriteTrajectory {
  uint16_t vop_ref_points[63]; /* Defined as "du" in 6.2.5.4 */
  uint16_t sprite_ref_points[63]; /* Defined as "dv" in 6.2.5.4 */
};

/**
 * Mpeg4GroupOfVOP:
 *
 * The group of video object plane structure as defined in:
 * 6.2.4 Group of Video Object Plane
 */
struct _Mpeg4GroupOfVOP {
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  uint8_t closed;
  uint8_t broken_link;
};

/**
 * Mpeg4VideoObjectPlane:
 *
 * The Video object plane structure as defined in:
 * 6.2.5 Video Object Plane and Video Plane with Short Header
 */
struct _Mpeg4VideoObjectPlane {
  Mpeg4VideoObjectCodingType coding_type;

  uint8_t  modulo_time_base;
  uint16_t time_increment;

  uint8_t  coded;
  /* if newpred_enable */
  uint16_t id;
  uint8_t  id_for_prediction_indication;
  uint16_t id_for_prediction;

  uint16_t width;
  uint16_t height;
  uint16_t horizontal_mc_spatial_ref;
  uint16_t vertical_mc_spatial_ref;

  uint8_t  rounding_type;
  /*if vol->shape != MPEG4_RECTANGULAR */
  uint8_t  background_composition;
  uint8_t  change_conv_ratio_disable;
  uint8_t  constant_alpha;
  uint8_t  constant_alpha_value;
  uint8_t  reduced_resolution;

  uint8_t  intra_dc_vlc_thr;


  uint8_t  top_field_first;
  uint8_t  alternate_vertical_scan_flag;

  uint16_t quant;

  uint8_t  fcode_forward;
  uint8_t  fcode_backward;

  uint8_t  shape_coding_type;
  uint8_t  load_backward_shape;
  uint8_t  ref_select_code;

  /* Computed macroblock informations */
  uint16_t mb_height;
  uint16_t mb_width;
  uint32_t mb_num;

  /* The size of the header */
  uint32_t    size;
};

/**
 * Mpeg4VideoPacketHdr:
 * @size: Size of the header in bit.
 *
 * The video packet header structure as defined in:
 * 6.2.5.2 Video Plane with Short Header
 */
struct _Mpeg4VideoPacketHdr {
  uint8_t  header_extension_code;
  uint16_t macroblock_number;
  uint16_t quant_scale;
  uint32_t   size;
};

/**
 * Mpeg4Packet:
 * @type: the type of the packet that start at @offset
 * @data: the data containing packet starting at @offset
 * @offset: offset of the start of the packet (without the 3 bytes startcode), but
 * including the #Mpeg4StartCode byte.
 * @size: The size in bytes of the packet or %G_MAXUINT if the end wasn't found.
 * @marker_size: The size in bit of the resync marker.
 *
 * A structure that contains the type of a packet, its offset and its size
 */
struct _Mpeg4Packet
{
  const uint8_t     *data;
  uint32_t             offset;
  size_t             size;
  uint32_t             marker_size;

  Mpeg4StartCode type;
};

Mpeg4ParseResult gst_h263_parse       (Mpeg4Packet * packet,
                                       const uint8_t * data, uint32_t offset,
                                       size_t size);


Mpeg4ParseResult gst_mpeg4_parse      (Mpeg4Packet * packet,
                                       bool skip_user_data,
                                       Mpeg4VideoObjectPlane *vop,
                                       const uint8_t * data, uint32_t offset,
                                       size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_video_object_plane       (Mpeg4VideoObjectPlane *vop,
                                          Mpeg4SpriteTrajectory *sprite_trajectory,
                                          Mpeg4VideoObjectLayer *vol,
                                          const uint8_t * data,
                                          size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_group_of_vop             (Mpeg4GroupOfVOP *gov,
                                          const uint8_t * data, size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_video_object_layer       (Mpeg4VideoObjectLayer *vol,
                                          Mpeg4VisualObject *vo,
                                          const uint8_t * data, size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_visual_object            (Mpeg4VisualObject *vo,
                                          Mpeg4VideoSignalType *signal_type,
                                          const uint8_t * data, size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_visual_object_sequence   (Mpeg4VisualObjectSequence *vos,
                                          const uint8_t * data, size_t size);
Mpeg4ParseResult
gst_mpeg4_parse_video_plane_short_header (Mpeg4VideoPlaneShortHdr * shorthdr,
                                          const uint8_t * data, size_t size);

Mpeg4ParseResult
gst_mpeg4_parse_video_packet_header      (Mpeg4VideoPacketHdr * videopackethdr,
                                          Mpeg4VideoObjectLayer * vol,
                                          Mpeg4VideoObjectPlane * vop,
                                          Mpeg4SpriteTrajectory * sprite_trajectory,
                                          const uint8_t * data, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MPEG4UTIL_H__ */
