/* reamer
 * Copyright (C) <2011> Intel
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

#ifndef __VC1_PARSER_H__
#define __VC1_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "bitreader.h"

#define MAX_HRD_NUM_LEAKY_BUCKETS 31

/**
 * @VC1_BFRACTION_BASIS: The @bfraction variable should be divided
 * by this constant to have the actual value.
 */
#define VC1_BFRACTION_BASIS 840

#define VC1_BFRACTION_RESERVED (VC1_BFRACTION_BASIS + 1)
#define VC1_BFRACTION_PTYPE_BI (VC1_BFRACTION_BASIS + 2)

typedef enum {
  VC1_END_OF_SEQ       = 0x0A,
  VC1_SLICE            = 0x0B,
  VC1_FIELD            = 0x0C,
  VC1_FRAME            = 0x0D,
  VC1_ENTRYPOINT       = 0x0E,
  VC1_SEQUENCE         = 0x0F,
  VC1_SLICE_USER       = 0x1B,
  VC1_FIELD_USER       = 0x1C,
  VC1_FRAME_USER       = 0x1D,
  VC1_ENTRY_POINT_USER = 0x1E,
  VC1_SEQUENCE_USER    = 0x1F
} VC1StartCode;

typedef enum {
  VC1_PROFILE_SIMPLE,
  VC1_PROFILE_MAIN,
  VC1_PROFILE_RESERVED,
  VC1_PROFILE_ADVANCED
} VC1Profile;

typedef enum {
  VC1_PARSER_OK,
  VC1_PARSER_BROKEN_DATA,
  VC1_PARSER_NO_BDU,
  VC1_PARSER_NO_BDU_END,
  VC1_PARSER_ERROR,
} VC1ParserResult;

typedef enum
{
  VC1_PICTURE_TYPE_P,
  VC1_PICTURE_TYPE_B,
  VC1_PICTURE_TYPE_I,
  VC1_PICTURE_TYPE_BI,
  VC1_PICTURE_TYPE_SKIPPED
} VC1PictureType;

typedef enum
{
    VC1_LEVEL_LOW    = 0,    /* Simple/Main profile low level */
    VC1_LEVEL_MEDIUM = 1,    /* Simple/Main profile medium level */
    VC1_LEVEL_HIGH   = 2,   /* Main profile high level */

    VC1_LEVEL_L0    = 0,    /* Advanced profile level 0 */
    VC1_LEVEL_L1    = 1,    /* Advanced profile level 1 */
    VC1_LEVEL_L2    = 2,    /* Advanced profile level 2 */
    VC1_LEVEL_L3    = 3,    /* Advanced profile level 3 */
    VC1_LEVEL_L4    = 4,    /* Advanced profile level 4 */

    /* 5 to 7 reserved */
    VC1_LEVEL_UNKNOWN = 255  /* Unknown profile */
} VC1Level;

typedef enum
{
  VC1_QUANTIZER_IMPLICITLY,
  VC1_QUANTIZER_EXPLICITLY,
  VC1_QUANTIZER_NON_UNIFORM,
  VC1_QUANTIZER_UNIFORM
} VC1QuantizerSpec;

typedef enum {
  VC1_DQPROFILE_FOUR_EDGES,
  VC1_DQPROFILE_DOUBLE_EDGES,
  VC1_DQPROFILE_SINGLE_EDGE,
  VC1_DQPROFILE_ALL_MBS
} VC1DQProfile;

typedef enum {
  VC1_CONDOVER_NONE,
  VC1_CONDOVER_ALL,
  VC1_CONDOVER_SELECT
} VC1Condover;

/**
 * VC1MvMode:
 *
 */
typedef enum
{
  VC1_MVMODE_1MV_HPEL_BILINEAR,
  VC1_MVMODE_1MV,
  VC1_MVMODE_1MV_HPEL,
  VC1_MVMODE_MIXED_MV,
  VC1_MVMODE_INTENSITY_COMP
} VC1MvMode;

typedef enum
{
  VC1_FRAME_PROGRESSIVE = 0x0,
  VC1_FRAME_INTERLACE   = 0x10,
  VC1_FIELD_INTERLACE   = 0x11
} VC1FrameCodingMode;

typedef struct _VC1SeqHdr            VC1SeqHdr;
typedef struct _VC1AdvancedSeqHdr    VC1AdvancedSeqHdr;
typedef struct _VC1HrdParam          VC1HrdParam;
typedef struct _VC1EntryPointHdr     VC1EntryPointHdr;

typedef struct _VC1SeqLayer     VC1SeqLayer;

typedef struct _VC1SeqStructA   VC1SeqStructA;
typedef struct _VC1SeqStructB   VC1SeqStructB;
typedef struct _VC1SeqStructC   VC1SeqStructC;

/* Pictures Structures */
typedef struct _VC1FrameLayer        VC1FrameLayer;
typedef struct _VC1FrameHdr          VC1FrameHdr;
typedef struct _VC1PicAdvanced       VC1PicAdvanced;
typedef struct _VC1PicSimpleMain     VC1PicSimpleMain;
typedef struct _VC1Picture           VC1Picture;
typedef struct _VC1SliceHdr          VC1SliceHdr;

typedef struct _VC1VopDquant         VC1VopDquant;

typedef struct _VC1BitPlanes         VC1BitPlanes;

typedef struct _VC1BDU               VC1BDU;

struct _VC1HrdParam
{
  uint8_t hrd_num_leaky_buckets;
  uint8_t bit_rate_exponent;
  uint8_t buffer_size_exponent;
  uint16_t hrd_rate[MAX_HRD_NUM_LEAKY_BUCKETS];
  uint16_t hrd_buffer[MAX_HRD_NUM_LEAKY_BUCKETS];
};

/**
 * VC1EntryPointHdr:
 *
 * Structure for entrypoint header, this will be used only in advanced profiles
 */
struct _VC1EntryPointHdr
{
  uint8_t broken_link;
  uint8_t closed_entry;
  uint8_t panscan_flag;
  uint8_t refdist_flag;
  uint8_t loopfilter;
  uint8_t fastuvmc;
  uint8_t extended_mv;
  uint8_t dquant;
  uint8_t vstransform;
  uint8_t overlap;
  uint8_t quantizer;
  uint8_t coded_size_flag;
  uint16_t coded_width;
  uint16_t coded_height;
  uint8_t extended_dmv;
  uint8_t range_mapy_flag;
  uint8_t range_mapy;
  uint8_t range_mapuv_flag;
  uint8_t range_mapuv;

  uint8_t hrd_full[MAX_HRD_NUM_LEAKY_BUCKETS];
};

/**
 * VC1AdvancedSeqHdr:
 *
 * Structure for the advanced profile sequence headers specific parameters.
 */
struct _VC1AdvancedSeqHdr
{
  VC1Level  level;

  uint8_t  frmrtq_postproc;
  uint8_t  bitrtq_postproc;
  uint8_t  postprocflag;
  uint16_t max_coded_width;
  uint16_t max_coded_height;
  uint8_t  pulldown;
  uint8_t  interlace;
  uint8_t  tfcntrflag;
  uint8_t  finterpflag;
  uint8_t  psf;
  uint8_t  display_ext;
  uint16_t disp_horiz_size;
  uint16_t disp_vert_size;
  uint8_t  aspect_ratio_flag;
  uint8_t  aspect_ratio;
  uint8_t  aspect_horiz_size;
  uint8_t  aspect_vert_size;
  uint8_t  framerate_flag;
  uint8_t  framerateind;
  uint8_t  frameratenr;
  uint8_t  frameratedr;
  uint16_t framerateexp;
  uint8_t  color_format_flag;
  uint8_t  color_prim;
  uint8_t  transfer_char;
  uint8_t  matrix_coef;
  uint8_t  hrd_param_flag;
  uint8_t  colordiff_format;

  VC1HrdParam hrd_param;

  /* computed */
  uint32_t framerate; /* Around in fps, 0 if unknown*/
  uint32_t bitrate;   /* Around in kpbs, 0 if unknown*/
  uint32_t par_n;
  uint32_t par_d;
  uint32_t fps_n;
  uint32_t fps_d;

  /* The last parsed entry point */
  VC1EntryPointHdr entrypoint;
};

struct _VC1SeqStructA
{
  uint32_t vert_size;
  uint32_t horiz_size;
};

struct _VC1SeqStructB
{
  VC1Level  level;

  uint8_t cbr;
  uint32_t framerate;

  /* In simple and main profiles only */
  uint32_t hrd_buffer;
  uint32_t hrd_rate;
};

struct _VC1SeqStructC
{
  VC1Profile profile;

  /* Only in simple and main profiles */
  uint8_t frmrtq_postproc;
  uint8_t bitrtq_postproc;
  uint8_t res_sprite;
  uint8_t loop_filter;
  uint8_t multires;
  uint8_t fastuvmc;
  uint8_t extended_mv;
  uint8_t dquant;
  uint8_t vstransform;
  uint8_t overlap;
  uint8_t syncmarker;
  uint8_t rangered;
  uint8_t maxbframes;
  uint8_t quantizer;
  uint8_t finterpflag;

  /* Computed */
  uint32_t framerate; /* Around in fps, 0 if unknown*/
  uint32_t bitrate;   /* Around in kpbs, 0 if unknown*/

  /* This should be filled by user if previously known */
  uint16_t coded_width;
  /* This should be filled by user if previously known */
  uint16_t coded_height;

  /* Wmvp specific */
  uint8_t wmvp;          /* Specify if the stream is wmp or not */
  /* In the wmvp case, the framerate is not computed but in the bistream */
  uint8_t slice_code;
};

struct _VC1SeqLayer
{
  uint32_t numframes;

  VC1SeqStructA struct_a;
  VC1SeqStructB struct_b;
  VC1SeqStructC struct_c;
};

/**
 * VC1SeqHdr:
 *
 * Structure for sequence headers in any profile.
 */
struct _VC1SeqHdr
{
  VC1Profile profile;

  VC1SeqStructC struct_c;

  /*  calculated */
  uint32_t mb_height;
  uint32_t mb_width;
  uint32_t mb_stride;

  VC1AdvancedSeqHdr   advanced;

};

/**
 * VC1PicSimpleMain:
 * @bfaction: Should be divided by #VC1_BFRACTION_BASIS
 * to get the real value.
 */
struct _VC1PicSimpleMain
{
  uint8_t frmcnt;
  uint8_t mvrange;
  uint8_t rangeredfrm;

  /* I and P pic simple and main profiles only */
  uint8_t respic;

  /* I and BI pic simple and main profiles only */
  uint8_t transacfrm2;
  uint8_t bf;

  /* B and P pic simple and main profiles only */
  uint8_t mvmode;
  uint8_t mvtab;
  uint8_t ttmbf;

  /* P pic simple and main profiles only */
  uint8_t mvmode2;
  uint8_t lumscale;
  uint8_t lumshift;

  uint8_t cbptab;
  uint8_t ttfrm;

  /* B and BI picture only
   * Should be divided by #VC1_BFRACTION_BASIS
   * to get the real value. */
  uint16_t bfraction;

  /* Biplane value, those fields only mention the fact
   * that the bitplane is in raw mode or not */
  uint8_t mvtypemb;
  uint8_t skipmb;
  uint8_t directmb; /* B pic main profile only */
};

/**
 * VC1PicAdvanced:
 * @bfaction: Should be divided by #VC1_BFRACTION_BASIS
 * to get the real value.
 */
struct _VC1PicAdvanced
{
  VC1FrameCodingMode fcm;
  uint8_t  tfcntr;

  uint8_t  rptfrm;
  uint8_t  tff;
  uint8_t  rff;
  uint8_t  ps_present;
  uint32_t ps_hoffset;
  uint32_t ps_voffset;
  uint16_t ps_width;
  uint16_t ps_height;
  uint8_t  rndctrl;
  uint8_t  uvsamp;
  uint8_t  postproc;

  /*  B and P picture specific */
  uint8_t  mvrange;
  uint8_t  mvmode;
  uint8_t  mvtab;
  uint8_t  cbptab;
  uint8_t  ttmbf;
  uint8_t  ttfrm;

  /* B and BI picture only
   * Should be divided by #VC1_BFRACTION_BASIS
   * to get the real value. */
  uint16_t bfraction;

  /* ppic */
  uint8_t  mvmode2;
  uint8_t  lumscale;
  uint8_t  lumshift;

  /* bipic */
  uint8_t  bf;
  uint8_t  condover;
  uint8_t  transacfrm2;

  /* Biplane value, those fields only mention the fact
   * that the bitplane is in raw mode or not */
  uint8_t  acpred;
  uint8_t  overflags;
  uint8_t  mvtypemb;
  uint8_t  skipmb;
  uint8_t  directmb;
  uint8_t  forwardmb; /* B pic interlace field only */

  /* For interlaced pictures only */
  uint8_t  fieldtx;

  /* P and B pictures */
  uint8_t  intcomp;
  uint8_t  dmvrange;
  uint8_t  mbmodetab;
  uint8_t  imvtab;
  uint8_t  icbptab;
  uint8_t  mvbptab2;
  uint8_t  mvbptab4; /* If 4mvswitch in ppic */

  /*  P picture */
  uint8_t  mvswitch4;

  /* For interlaced fields only */
  uint16_t refdist;
  uint8_t fptype; /* Raw value */

  /* P pic */
  uint8_t  numref;
  uint8_t  reffield;
  uint8_t  lumscale2;
  uint8_t  lumshift2;
  uint8_t  intcompfield;

};

struct _VC1BitPlanes
{
  uint8_t  *acpred;
  uint8_t  *fieldtx;
  uint8_t  *overflags;
  uint8_t  *mvtypemb;
  uint8_t  *skipmb;
  uint8_t  *directmb;
  uint8_t  *forwardmb;

  uint32_t size; /* Size of the arrays */
};

struct _VC1VopDquant
{
  uint8_t pqdiff;
  uint8_t abspq;

  /* Computed */
  uint8_t altpquant;

  /*  if dqant != 2*/
  uint8_t dquantfrm;
  uint8_t dqprofile;

  /* Boundary edge selection. This represents DQSBEDGE
   * if dqprofile == VC1_DQPROFILE_SINGLE_EDGE or
   * DQDBEDGE if dqprofile == VC1_DQPROFILE_DOUBLE_EDGE */
  uint8_t dqbedge;

  /* FIXME: remove */
  uint8_t unused;

  /* if dqprofile == VC1_DQPROFILE_ALL_MBS */
  uint8_t dqbilevel;

};

struct _VC1FrameLayer
{
  uint8_t key;
  uint32_t framesize;

  uint32_t timestamp;

  /* calculated */
  uint32_t next_framelayer_offset;
  uint8_t skiped_p_frame;
};

/**
 * VC1FrameHdr:
 *
 * Structure that represent picture in any profile or mode.
 * You should look at @ptype and @profile to know what is currently
 * in use.
 */
struct _VC1FrameHdr
{
  /* common fields */
  VC1PictureType ptype;
  uint8_t interpfrm;
  uint8_t halfqp;
  uint8_t transacfrm;
  uint8_t transdctab;
  uint8_t pqindex;
  uint8_t pquantizer;

  /* Computed */
  uint8_t pquant;

  /* Convenience fields */
  uint8_t profile;
  uint8_t dquant;

  /*  If dquant */
  VC1VopDquant vopdquant;

  union {
    VC1PicSimpleMain simple;
    VC1PicAdvanced advanced;
  } pic;

  /* Size of the picture layer in bits */
  uint32_t header_size;
};

/**
 * VC1SliceHdr:
 *
 * Structure that represents slice layer in advanced profile.
 */
struct _VC1SliceHdr
{
  uint16_t slice_addr;

  /* Size of the slice layer in bits */
  uint32_t header_size;
};

/**
 * VC1BDU:
 *
 * Structure that represents a Bitstream Data Unit.
 */
struct _VC1BDU
{
  VC1StartCode type;
  uint32_t size;
  uint32_t sc_offset;
  uint32_t offset;
  uint8_t * data;
};

VC1ParserResult vc1_identify_next_bdu           (const uint8_t *data,
                                                 size_t size,
                                                 VC1BDU *bdu);


VC1ParserResult vc1_parse_sequence_header       (const uint8_t *data,
                                                 size_t size,
                                                 VC1SeqHdr * seqhdr);

VC1ParserResult vc1_parse_entry_point_header    (const  uint8_t *data,
                                                 size_t size,
                                                 VC1EntryPointHdr * entrypoint,
                                                 VC1SeqHdr *seqhdr);

VC1ParserResult vc1_parse_sequence_layer        (const uint8_t *data,
                                                 size_t size,
                                                 VC1SeqLayer * seqlayer);

VC1ParserResult
vc1_parse_sequence_header_struct_a              (const uint8_t *data,
                                                 size_t size,
                                                 VC1SeqStructA *structa);
VC1ParserResult
vc1_parse_sequence_header_struct_b              (const uint8_t *data,
                                                 size_t size,
                                                 VC1SeqStructB *structb);

VC1ParserResult
vc1_parse_sequence_header_struct_c              (const uint8_t *data,
                                                 size_t size,
                                                 VC1SeqStructC *structc);

VC1ParserResult vc1_parse_frame_layer           (const uint8_t *data,
                                                 size_t size,
                                                 VC1FrameLayer * framelayer);

VC1ParserResult vc1_parse_frame_header          (const uint8_t *data,
                                                 size_t size,
                                                 VC1FrameHdr * framehdr,
                                                 VC1SeqHdr *seqhdr,
                                                 VC1BitPlanes *bitplanes);

VC1ParserResult vc1_parse_field_header          (const uint8_t *data,
                                                 size_t size,
                                                 VC1FrameHdr * fieldhdr,
                                                 VC1SeqHdr *seqhdr,
                                                 VC1BitPlanes *bitplanes);

VC1ParserResult vc1_parse_slice_header           (const uint8_t *data,
                                                  size_t size,
                                                  VC1SliceHdr *slicehdr, 
                                                  VC1SeqHdr *seqhdr);

VC1BitPlanes *  vc1_bitplanes_new               (void);

void   vc1_bitplanes_free                       (VC1BitPlanes *bitplanes);

void   vc1_bitplanes_free_1                     (VC1BitPlanes *bitplanes);

bool   vc1_bitplanes_ensure_size             (VC1BitPlanes *bitplanes,
                                                 VC1SeqHdr *seqhdr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
