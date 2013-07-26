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
  uint8 hrd_num_leaky_buckets;
  uint8 bit_rate_exponent;
  uint8 buffer_size_exponent;
  uint16 hrd_rate[MAX_HRD_NUM_LEAKY_BUCKETS];
  uint16 hrd_buffer[MAX_HRD_NUM_LEAKY_BUCKETS];
};

/**
 * VC1EntryPointHdr:
 *
 * Structure for entrypoint header, this will be used only in advanced profiles
 */
struct _VC1EntryPointHdr
{
  uint8 broken_link;
  uint8 closed_entry;
  uint8 panscan_flag;
  uint8 refdist_flag;
  uint8 loopfilter;
  uint8 fastuvmc;
  uint8 extended_mv;
  uint8 dquant;
  uint8 vstransform;
  uint8 overlap;
  uint8 quantizer;
  uint8 coded_size_flag;
  uint16 coded_width;
  uint16 coded_height;
  uint8 extended_dmv;
  uint8 range_mapy_flag;
  uint8 range_mapy;
  uint8 range_mapuv_flag;
  uint8 range_mapuv;

  uint8 hrd_full[MAX_HRD_NUM_LEAKY_BUCKETS];
};

/**
 * VC1AdvancedSeqHdr:
 *
 * Structure for the advanced profile sequence headers specific parameters.
 */
struct _VC1AdvancedSeqHdr
{
  VC1Level  level;

  uint8  frmrtq_postproc;
  uint8  bitrtq_postproc;
  uint8  postprocflag;
  uint16 max_coded_width;
  uint16 max_coded_height;
  uint8  pulldown;
  uint8  interlace;
  uint8  tfcntrflag;
  uint8  finterpflag;
  uint8  psf;
  uint8  display_ext;
  uint16 disp_horiz_size;
  uint16 disp_vert_size;
  uint8  aspect_ratio_flag;
  uint8  aspect_ratio;
  uint8  aspect_horiz_size;
  uint8  aspect_vert_size;
  uint8  framerate_flag;
  uint8  framerateind;
  uint8  frameratenr;
  uint8  frameratedr;
  uint16 framerateexp;
  uint8  color_format_flag;
  uint8  color_prim;
  uint8  transfer_char;
  uint8  matrix_coef;
  uint8  hrd_param_flag;
  uint8  colordiff_format;

  VC1HrdParam hrd_param;

  /* computed */
  uint32 framerate; /* Around in fps, 0 if unknown*/
  uint32 bitrate;   /* Around in kpbs, 0 if unknown*/
  uint32 par_n;
  uint32 par_d;
  uint32 fps_n;
  uint32 fps_d;

  /* The last parsed entry point */
  VC1EntryPointHdr entrypoint;
};

struct _VC1SeqStructA
{
  uint32 vert_size;
  uint32 horiz_size;
};

struct _VC1SeqStructB
{
  VC1Level  level;

  uint8 cbr;
  uint32 framerate;

  /* In simple and main profiles only */
  uint32 hrd_buffer;
  uint32 hrd_rate;
};

struct _VC1SeqStructC
{
  VC1Profile profile;

  /* Only in simple and main profiles */
  uint8 frmrtq_postproc;
  uint8 bitrtq_postproc;
  uint8 res_sprite;
  uint8 loop_filter;
  uint8 multires;
  uint8 fastuvmc;
  uint8 extended_mv;
  uint8 dquant;
  uint8 vstransform;
  uint8 overlap;
  uint8 syncmarker;
  uint8 rangered;
  uint8 maxbframes;
  uint8 quantizer;
  uint8 finterpflag;

  /* Computed */
  uint32 framerate; /* Around in fps, 0 if unknown*/
  uint32 bitrate;   /* Around in kpbs, 0 if unknown*/

  /* This should be filled by user if previously known */
  uint16 coded_width;
  /* This should be filled by user if previously known */
  uint16 coded_height;

  /* Wmvp specific */
  uint8 wmvp;          /* Specify if the stream is wmp or not */
  /* In the wmvp case, the framerate is not computed but in the bistream */
  uint8 slice_code;
};

struct _VC1SeqLayer
{
  uint32 numframes;

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
  uint32 mb_height;
  uint32 mb_width;
  uint32 mb_stride;

  VC1AdvancedSeqHdr   advanced;

};

/**
 * VC1PicSimpleMain:
 * @bfaction: Should be divided by #VC1_BFRACTION_BASIS
 * to get the real value.
 */
struct _VC1PicSimpleMain
{
  uint8 frmcnt;
  uint8 mvrange;
  uint8 rangeredfrm;

  /* I and P pic simple and main profiles only */
  uint8 respic;

  /* I and BI pic simple and main profiles only */
  uint8 transacfrm2;
  uint8 bf;

  /* B and P pic simple and main profiles only */
  uint8 mvmode;
  uint8 mvtab;
  uint8 ttmbf;

  /* P pic simple and main profiles only */
  uint8 mvmode2;
  uint8 lumscale;
  uint8 lumshift;

  uint8 cbptab;
  uint8 ttfrm;

  /* B and BI picture only
   * Should be divided by #VC1_BFRACTION_BASIS
   * to get the real value. */
  uint16 bfraction;

  /* Biplane value, those fields only mention the fact
   * that the bitplane is in raw mode or not */
  uint8 mvtypemb;
  uint8 skipmb;
  uint8 directmb; /* B pic main profile only */
};

/**
 * VC1PicAdvanced:
 * @bfaction: Should be divided by #VC1_BFRACTION_BASIS
 * to get the real value.
 */
struct _VC1PicAdvanced
{
  VC1FrameCodingMode fcm;
  uint8  tfcntr;

  uint8  rptfrm;
  uint8  tff;
  uint8  rff;
  uint8  ps_present;
  uint32 ps_hoffset;
  uint32 ps_voffset;
  uint16 ps_width;
  uint16 ps_height;
  uint8  rndctrl;
  uint8  uvsamp;
  uint8  postproc;

  /*  B and P picture specific */
  uint8  mvrange;
  uint8  mvmode;
  uint8  mvtab;
  uint8  cbptab;
  uint8  ttmbf;
  uint8  ttfrm;

  /* B and BI picture only
   * Should be divided by #VC1_BFRACTION_BASIS
   * to get the real value. */
  uint16 bfraction;

  /* ppic */
  uint8  mvmode2;
  uint8  lumscale;
  uint8  lumshift;

  /* bipic */
  uint8  bf;
  uint8  condover;
  uint8  transacfrm2;

  /* Biplane value, those fields only mention the fact
   * that the bitplane is in raw mode or not */
  uint8  acpred;
  uint8  overflags;
  uint8  mvtypemb;
  uint8  skipmb;
  uint8  directmb;
  uint8  forwardmb; /* B pic interlace field only */

  /* For interlaced pictures only */
  uint8  fieldtx;

  /* P and B pictures */
  uint8  intcomp;
  uint8  dmvrange;
  uint8  mbmodetab;
  uint8  imvtab;
  uint8  icbptab;
  uint8  mvbptab2;
  uint8  mvbptab4; /* If 4mvswitch in ppic */

  /*  P picture */
  uint8  mvswitch4;

  /* For interlaced fields only */
  uint16 refdist;
  uint8 fptype; /* Raw value */

  /* P pic */
  uint8  numref;
  uint8  reffield;
  uint8  lumscale2;
  uint8  lumshift2;
  uint8  intcompfield;

};

struct _VC1BitPlanes
{
  uint8  *acpred;
  uint8  *fieldtx;
  uint8  *overflags;
  uint8  *mvtypemb;
  uint8  *skipmb;
  uint8  *directmb;
  uint8  *forwardmb;

  uint32 size; /* Size of the arrays */
};

struct _VC1VopDquant
{
  uint8 pqdiff;
  uint8 abspq;

  /* Computed */
  uint8 altpquant;

  /*  if dqant != 2*/
  uint8 dquantfrm;
  uint8 dqprofile;

  /* Boundary edge selection. This represents DQSBEDGE
   * if dqprofile == VC1_DQPROFILE_SINGLE_EDGE or
   * DQDBEDGE if dqprofile == VC1_DQPROFILE_DOUBLE_EDGE */
  uint8 dqbedge;

  /* FIXME: remove */
  uint8 unused;

  /* if dqprofile == VC1_DQPROFILE_ALL_MBS */
  uint8 dqbilevel;

};

struct _VC1FrameLayer
{
  uint8 key;
  uint32 framesize;

  uint32 timestamp;

  /* calculated */
  uint32 next_framelayer_offset;
  uint8 skiped_p_frame;
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
  uint8 interpfrm;
  uint8 halfqp;
  uint8 transacfrm;
  uint8 transdctab;
  uint8 pqindex;
  uint8 pquantizer;

  /* Computed */
  uint8 pquant;

  /* Convenience fields */
  uint8 profile;
  uint8 dquant;

  /*  If dquant */
  VC1VopDquant vopdquant;

  union {
    VC1PicSimpleMain simple;
    VC1PicAdvanced advanced;
  } pic;

  /* Size of the picture layer in bits */
  uint32 header_size;
};

/**
 * VC1SliceHdr:
 *
 * Structure that represents slice layer in advanced profile.
 */
struct _VC1SliceHdr
{
  uint16 slice_addr;

  /* Size of the slice layer in bits */
  uint32 header_size;
};

/**
 * VC1BDU:
 *
 * Structure that represents a Bitstream Data Unit.
 */
struct _VC1BDU
{
  VC1StartCode type;
  uint32 size;
  uint32 sc_offset;
  uint32 offset;
  uint8 * data;
};

VC1ParserResult vc1_identify_next_bdu           (const uint8 *data,
                                                 size_t size,
                                                 VC1BDU *bdu);


VC1ParserResult vc1_parse_sequence_header       (const uint8 *data,
                                                 size_t size,
                                                 VC1SeqHdr * seqhdr);

VC1ParserResult vc1_parse_entry_point_header    (const  uint8 *data,
                                                 size_t size,
                                                 VC1EntryPointHdr * entrypoint,
                                                 VC1SeqHdr *seqhdr);

VC1ParserResult vc1_parse_sequence_layer        (const uint8 *data,
                                                 size_t size,
                                                 VC1SeqLayer * seqlayer);

VC1ParserResult
vc1_parse_sequence_header_struct_a              (const uint8 *data,
                                                 size_t size,
                                                 VC1SeqStructA *structa);
VC1ParserResult
vc1_parse_sequence_header_struct_b              (const uint8 *data,
                                                 size_t size,
                                                 VC1SeqStructB *structb);

VC1ParserResult
vc1_parse_sequence_header_struct_c              (const uint8 *data,
                                                 size_t size,
                                                 VC1SeqStructC *structc);

VC1ParserResult vc1_parse_frame_layer           (const uint8 *data,
                                                 size_t size,
                                                 VC1FrameLayer * framelayer);

VC1ParserResult vc1_parse_frame_header          (const uint8 *data,
                                                 size_t size,
                                                 VC1FrameHdr * framehdr,
                                                 VC1SeqHdr *seqhdr,
                                                 VC1BitPlanes *bitplanes);

VC1ParserResult vc1_parse_field_header          (const uint8 *data,
                                                 size_t size,
                                                 VC1FrameHdr * fieldhdr,
                                                 VC1SeqHdr *seqhdr,
                                                 VC1BitPlanes *bitplanes);

VC1ParserResult vc1_parse_slice_header           (const uint8 *data,
                                                  size_t size,
                                                  VC1SliceHdr *slicehdr, 
                                                  VC1SeqHdr *seqhdr);

VC1BitPlanes *  vc1_bitplanes_new               (void);

void   vc1_bitplanes_free                       (VC1BitPlanes *bitplanes);

void   vc1_bitplanes_free_1                     (VC1BitPlanes *bitplanes);

boolean   vc1_bitplanes_ensure_size             (VC1BitPlanes *bitplanes,
                                                 VC1SeqHdr *seqhdr);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
