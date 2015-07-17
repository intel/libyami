/* vp9parser.c
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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
 * SECTION:vp9parser
 * @short_description: Convenience library for parsing vp9 video bitstream.
 *
 * For more details about the structures, you can refer to the
 * specifications:
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "bitreader.h"
#include "vp9parser.h"
#include <string.h>
#include <stdlib.h>
#include "common/log.h"

#define MAXQ 255
#define QINDEX_RANGE 256
#define QINDEX_BITS 8

#define MAX_LOOP_FILTER 63
#define MAX_PROB 255

typedef struct _ReferenceSize
{
  uint32_t width;
  uint32_t height;
}ReferenceSize;

typedef struct _Vp9ParserPrivate
{
  int8_t   y_dc_delta_q;
  int8_t   uv_dc_delta_q;
  int8_t   uv_ac_delta_q;

  int16_t   y_dequant[QINDEX_RANGE][2];
  int16_t   uv_dequant[QINDEX_RANGE][2];

  /* for loop filters */
  int8_t    ref_deltas[VP9_MAX_REF_LF_DELTAS];
  int8_t    mode_deltas[VP9_MAX_MODE_LF_DELTAS];

  BOOL segmentation_abs_delta;
  Vp9SegmentationInfoData segmentation[VP9_MAX_SEGMENTS];

  ReferenceSize reference[VP9_REF_FRAMES];
} Vp9ParserPrivate;

void init_dequantizer(Vp9Parser* parser);

static void init_vp9_parser(Vp9Parser* parser)
{
  Vp9ParserPrivate* priv = parser->priv;
  memset(parser, 0, sizeof(Vp9Parser));
  memset(priv, 0, sizeof(Vp9ParserPrivate));
  parser->priv = priv;
  init_dequantizer(parser);
}

Vp9Parser* vp9_parser_new()
{
  Vp9Parser* parser = (Vp9Parser*)malloc(sizeof(Vp9Parser));
  if (!parser)
    return NULL;
  Vp9ParserPrivate* priv = (Vp9ParserPrivate*)malloc(sizeof(Vp9ParserPrivate));
  if (!priv) {
    free(parser);
    return NULL;
  }
  parser->priv = priv;
  init_vp9_parser(parser);
  return parser;
}

void vp9_parser_free(Vp9Parser* parser)
{
  if (parser) {
    if (parser->priv)
      free(parser->priv);
    free(parser);
  }
}

#define vp9_read_bit(br) bit_reader_get_bits_uint8_unchecked(br, 1)
#define vp9_read_bits(br, bits) bit_reader_get_bits_uint32_unchecked(br, bits)

static int32_t vp9_read_signed_bits(BitReader* br, int bits)
{
  assert(bits < 32);
  const int32_t value = vp9_read_bits(br, bits);
  return vp9_read_bit(br) ? -value : value;

}

static BOOL verify_frame_marker(BitReader* br)
{
#define VP9_FRAME_MARKER 2
  uint8_t frame_marker = vp9_read_bits(br, 2);
  if (frame_marker != VP9_FRAME_MARKER)
    return FALSE;
  return TRUE;
}

static BOOL verify_sync_code(BitReader* const br) {
#define VP9_SYNC_CODE_0 0x49
#define VP9_SYNC_CODE_1 0x83
#define VP9_SYNC_CODE_2 0x42
  return vp9_read_bits(br, 8) == VP9_SYNC_CODE_0 &&
         vp9_read_bits(br, 8) == VP9_SYNC_CODE_1 &&
         vp9_read_bits(br, 8) == VP9_SYNC_CODE_2;
}

static VP9_PROFILE read_profile(BitReader* br)
{
    uint8_t profile = vp9_read_bit(br);
    profile |= vp9_read_bit(br) << 1;
    if (profile > 2)
        profile += vp9_read_bit(br);
    return (VP9_PROFILE)profile;
}

static void read_frame_size(BitReader* br,
                     uint32_t *width, uint32_t *height) {
  const uint32_t w = vp9_read_bits(br, 16) + 1;
  const uint32_t h = vp9_read_bits(br, 16) + 1;
  *width = w;
  *height = h;
}

static void read_display_frame_size(Vp9FrameHdr* frame_hdr, BitReader* br)
{
  frame_hdr->display_width = frame_hdr->width;
  frame_hdr->display_height = frame_hdr->height;
  frame_hdr->display_size_enabled = vp9_read_bit(br);
  if (frame_hdr->display_size_enabled) {
    read_frame_size(br, &frame_hdr->display_width, &frame_hdr->display_height);
  }
}

static void read_frame_size_from_refs(const Vp9Parser* parser, Vp9FrameHdr* frame_hdr, BitReader* br)
{
  BOOL found = FALSE;
  int i;
  const Vp9ParserPrivate* priv = parser->priv;
  for (i = 0; i < VP9_REFS_PER_FRAME; i++) {
    found = vp9_read_bit(br);
    if (found) {
      uint8_t idx = frame_hdr->ref_frame_indices[i];
      frame_hdr->size_from_ref[i] = TRUE;
      frame_hdr->width = priv->reference[idx].width;
      frame_hdr->height = priv->reference[idx].height;
      break;
    }
  }
  if (!found) {
    read_frame_size(br, &frame_hdr->width, &frame_hdr->height);
  }
}

static VP9_INTERP_FILTER read_interp_filter(BitReader* br)
{
  const VP9_INTERP_FILTER filter_map[] = { VP9_EIGHTTAP_SMOOTH,
                                           VP9_EIGHTTAP,
                                           VP9_EIGHTTAP_SHARP,
                                           VP9_BILINEAR };
  return vp9_read_bit(br) ? VP9_SWITCHABLE
                             : filter_map[vp9_read_bits(br, 2)];
}

static void read_loopfilter(Vp9LoopFilter* lf, BitReader* br)
{
  lf->filter_level = vp9_read_bits(br, 6);
  lf->sharpness_level = vp9_read_bits(br, 3);

  lf->mode_ref_delta_update = 0;

  lf->mode_ref_delta_enabled = vp9_read_bit(br);
  if (lf->mode_ref_delta_enabled) {
    lf->mode_ref_delta_update = vp9_read_bit(br);
    if (lf->mode_ref_delta_update) {
      int i;
      for (i = 0; i < VP9_MAX_REF_LF_DELTAS; i++) {
        lf->update_ref_deltas[i] = vp9_read_bit(br);
        if (lf->update_ref_deltas[i])
          lf->ref_deltas[i] = vp9_read_signed_bits(br, 6);
      }

      for (i = 0; i < VP9_MAX_MODE_LF_DELTAS; i++) {
        lf->update_mode_deltas[i] = vp9_read_bit(br);
        if (lf->update_mode_deltas[i])
          lf->mode_deltas[i] = vp9_read_signed_bits(br, 6);
      }
    }
  }
}

static int8_t read_delta_q(BitReader* br) {
  return vp9_read_bit(br) ? vp9_read_signed_bits(br, 4) : 0;
}


static void read_quantization(Vp9FrameHdr* frame_hdr, BitReader* br)
{
  frame_hdr->base_qindex = vp9_read_bits(br, QINDEX_BITS);
  frame_hdr->y_dc_delta_q = read_delta_q(br);
  frame_hdr->uv_dc_delta_q = read_delta_q(br);
  frame_hdr->uv_ac_delta_q = read_delta_q(br);
}

static void read_segmentation(Vp9SegmentationInfo* seg, BitReader* br)
{
  int i;

  seg->update_map = FALSE;
  seg->update_data = FALSE;

  seg->enabled = vp9_read_bit(br);
  if (!seg->enabled)
    return;
  seg->update_map = vp9_read_bit(br);
  if (seg->update_map) {
    for (i = 0; i < VP9_SEG_TREE_PROBS; i++) {
      seg->update_tree_probs[i] = vp9_read_bit(br);
      seg->tree_probs[i] = seg->update_tree_probs[i] ?
        vp9_read_bits(br, 8) : MAX_PROB;
    }
    seg->temporal_update = vp9_read_bit(br);
    if (seg->temporal_update) {
      for (i = 0; i < VP9_PREDICTION_PROBS; i++) {
        seg->update_pred_probs[i] = vp9_read_bit(br);
        seg->pred_probs[i] = seg->update_pred_probs[i] ?
          vp9_read_bits(br, 8) : MAX_PROB;
      }
    } else {
      for (i = 0; i < VP9_PREDICTION_PROBS; i++) {
        seg->pred_probs[i] = MAX_PROB;
      }
    }
  }

  seg->update_data = vp9_read_bit(br);

  if (seg->update_data) {
    /* clear all features */
    memset(seg->data, 0, sizeof(seg->data));

    seg->abs_delta = vp9_read_bit(br);
    for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
      Vp9SegmentationInfoData* seg_data = seg->data + i;
      uint8_t data;
      /* SEG_LVL_ALT_Q */
      seg_data->alternate_quantizer_enabled = vp9_read_bit(br);
      if (seg_data->alternate_quantizer_enabled) {
        data = vp9_read_bits(br, 8);
        seg_data->alternate_quantizer = vp9_read_bit(br) ? -data : data;
      }
      /* SEG_LVL_ALT_LF */
      seg_data->alternate_loop_filter_enabled = vp9_read_bit(br);
      if (seg_data->alternate_loop_filter_enabled) {
        data = vp9_read_bits(br, 6);
        seg_data->alternate_loop_filter = vp9_read_bit(br) ? -data : data;
      }
      /* SEG_LVL_REF_FRAME */
      seg_data->reference_frame_enabled = vp9_read_bit(br);
      if (seg_data->reference_frame_enabled) {
        seg_data->reference_frame = vp9_read_bits(br, 2);
      }
      seg_data->reference_skip = vp9_read_bit(br);
    }
  }
}

#define MIN_TILE_WIDTH_B64 4
#define MAX_TILE_WIDTH_B64 64
uint32_t get_max_lb_tile_cols(uint32_t sb_cols)
{
  int log2 = 0;
  while ((sb_cols >> log2) >= MIN_TILE_WIDTH_B64)
    ++log2;
  if (log2)
    log2--;
  return log2;
}

uint32_t get_min_lb_tile_cols(uint32_t sb_cols)
{
  int log2 = 0;
  while ((MAX_TILE_WIDTH_B64 << log2) < sb_cols)
    ++log2;
  return log2;
}

/* align to 64 */
#define SB_ALIGN(w) (((w | 0x3f) + 1) >> 6)
static BOOL read_tile_info(Vp9FrameHdr * frame_hdr, BitReader* br)
{
  const uint32_t sb_cols = SB_ALIGN(frame_hdr->width);
  uint32_t min_lb_tile_cols = get_min_lb_tile_cols(sb_cols);
  uint32_t max_lb_tile_cols = get_max_lb_tile_cols(sb_cols);
  uint32_t max_ones = max_lb_tile_cols - min_lb_tile_cols;

  /* columns */
  frame_hdr->log2_tile_columns = min_lb_tile_cols;
  while (max_ones-- && vp9_read_bit(br))
    frame_hdr->log2_tile_columns++;

  /* row */
  frame_hdr->log2_tile_rows = vp9_read_bit(br);
  if (frame_hdr->log2_tile_rows)
    frame_hdr->log2_tile_rows += vp9_read_bit(br);
  return TRUE;

}

static void loop_filter_update(Vp9Parser* parser, const Vp9LoopFilter * lf)
{
  Vp9ParserPrivate* priv = parser->priv;
  int i;

  for (i = 0; i < VP9_MAX_REF_LF_DELTAS; i++) {
    if (lf->update_ref_deltas[i])
      priv->ref_deltas[i] = lf->ref_deltas[i];
  }

  for (i = 0; i < VP9_MAX_MODE_LF_DELTAS; i++) {
    if (lf->update_mode_deltas[i])
      priv->mode_deltas[i] = lf->mode_deltas[i];
  }
}

BOOL compare_and_set(int8_t* dest, const int8_t src)
{
  const int8_t old = *dest;
  *dest = src;
  return old != src;
}


static int clamp(int value, int low, int high) {
  return value < low ? low : (value > high ? high : value);
}

static const int16_t dc_qlookup[QINDEX_RANGE] = {
  4,       8,    8,    9,   10,   11,   12,   12,
  13,     14,   15,   16,   17,   18,   19,   19,
  20,     21,   22,   23,   24,   25,   26,   26,
  27,     28,   29,   30,   31,   32,   32,   33,
  34,     35,   36,   37,   38,   38,   39,   40,
  41,     42,   43,   43,   44,   45,   46,   47,
  48,     48,   49,   50,   51,   52,   53,   53,
  54,     55,   56,   57,   57,   58,   59,   60,
  61,     62,   62,   63,   64,   65,   66,   66,
  67,     68,   69,   70,   70,   71,   72,   73,
  74,     74,   75,   76,   77,   78,   78,   79,
  80,     81,   81,   82,   83,   84,   85,   85,
  87,     88,   90,   92,   93,   95,   96,   98,
  99,    101,  102,  104,  105,  107,  108,  110,
  111,   113,  114,  116,  117,  118,  120,  121,
  123,   125,  127,  129,  131,  134,  136,  138,
  140,   142,  144,  146,  148,  150,  152,  154,
  156,   158,  161,  164,  166,  169,  172,  174,
  177,   180,  182,  185,  187,  190,  192,  195,
  199,   202,  205,  208,  211,  214,  217,  220,
  223,   226,  230,  233,  237,  240,  243,  247,
  250,   253,  257,  261,  265,  269,  272,  276,
  280,   284,  288,  292,  296,  300,  304,  309,
  313,   317,  322,  326,  330,  335,  340,  344,
  349,   354,  359,  364,  369,  374,  379,  384,
  389,   395,  400,  406,  411,  417,  423,  429,
  435,   441,  447,  454,  461,  467,  475,  482,
  489,   497,  505,  513,  522,  530,  539,  549,
  559,   569,  579,  590,  602,  614,  626,  640,
  654,   668,  684,  700,  717,  736,  755,  775,
  796,   819,  843,  869,  896,  925,  955,  988,
  1022, 1058, 1098, 1139, 1184, 1232, 1282, 1336,
};

static const int16_t ac_qlookup[QINDEX_RANGE] = {
  4,       8,    9,   10,   11,   12,   13,   14,
  15,     16,   17,   18,   19,   20,   21,   22,
  23,     24,   25,   26,   27,   28,   29,   30,
  31,     32,   33,   34,   35,   36,   37,   38,
  39,     40,   41,   42,   43,   44,   45,   46,
  47,     48,   49,   50,   51,   52,   53,   54,
  55,     56,   57,   58,   59,   60,   61,   62,
  63,     64,   65,   66,   67,   68,   69,   70,
  71,     72,   73,   74,   75,   76,   77,   78,
  79,     80,   81,   82,   83,   84,   85,   86,
  87,     88,   89,   90,   91,   92,   93,   94,
  95,     96,   97,   98,   99,  100,  101,  102,
  104,   106,  108,  110,  112,  114,  116,  118,
  120,   122,  124,  126,  128,  130,  132,  134,
  136,   138,  140,  142,  144,  146,  148,  150,
  152,   155,  158,  161,  164,  167,  170,  173,
  176,   179,  182,  185,  188,  191,  194,  197,
  200,   203,  207,  211,  215,  219,  223,  227,
  231,   235,  239,  243,  247,  251,  255,  260,
  265,   270,  275,  280,  285,  290,  295,  300,
  305,   311,  317,  323,  329,  335,  341,  347,
  353,   359,  366,  373,  380,  387,  394,  401,
  408,   416,  424,  432,  440,  448,  456,  465,
  474,   483,  492,  501,  510,  520,  530,  540,
  550,   560,  571,  582,  593,  604,  615,  627,
  639,   651,  663,  676,  689,  702,  715,  729,
  743,   757,  771,  786,  801,  816,  832,  848,
  864,   881,  898,  915,  933,  951,  969,  988,
  1007, 1026, 1046, 1066, 1087, 1108, 1129, 1151,
  1173, 1196, 1219, 1243, 1267, 1292, 1317, 1343,
  1369, 1396, 1423, 1451, 1479, 1508, 1537, 1567,
  1597, 1628, 1660, 1692, 1725, 1759, 1793, 1828,
};

int16_t vp9_dc_quant(int qindex, int delta) {
  const uint8_t q = clamp(qindex + delta, 0, MAXQ);
  return dc_qlookup[q];
}

int16_t vp9_ac_quant(int qindex, int delta) {
  const uint8_t q = clamp(qindex + delta, 0, MAXQ);
  return ac_qlookup[q];
}

void init_dequantizer(Vp9Parser* parser)
{
  Vp9ParserPrivate* priv = parser->priv;
  int q;
  for (q = 0; q < QINDEX_RANGE; q++) {
    priv->y_dequant[q][0] = vp9_dc_quant(q, priv->y_dc_delta_q);
    priv->y_dequant[q][1] = vp9_ac_quant(q, 0);
    priv->uv_dequant[q][0] = vp9_dc_quant(q, priv->uv_dc_delta_q);
    priv->uv_dequant[q][1] = vp9_ac_quant(q, priv->uv_ac_delta_q);
  }
}
static void quantization_update(Vp9Parser* parser, const Vp9FrameHdr * frame_hdr)
{
  BOOL update = FALSE;
  Vp9ParserPrivate* priv = parser->priv;
  update |= compare_and_set(&priv->y_dc_delta_q, frame_hdr->y_dc_delta_q);
  update |= compare_and_set(&priv->uv_dc_delta_q, frame_hdr->uv_dc_delta_q);
  update |= compare_and_set(&priv->uv_ac_delta_q, frame_hdr->uv_ac_delta_q);
  if (update) {
    init_dequantizer(parser);
  }
  parser->lossless_flag = frame_hdr->base_qindex == 0 &&
                         frame_hdr->y_dc_delta_q == 0 &&
                         frame_hdr->uv_dc_delta_q == 0 &&
                         frame_hdr->uv_ac_delta_q == 0;

}

uint8_t seg_get_base_qindex(const Vp9Parser* parser, const Vp9FrameHdr * frame_hdr, int segid)
{
  int seg_base = frame_hdr->base_qindex;
  Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
  const Vp9SegmentationInfoData* seg = priv->segmentation + segid;
  /* DEBUG("id = %d, seg_base = %d, seg enable = %d, alt eanble = %d, abs = %d, alt= %d\n",segid,
    seg_base, frame_hdr->segmentation.enabled, seg->alternate_quantizer_enabled, priv->segmentation_abs_delta,  seg->alternate_quantizer);
*/
  if(frame_hdr->segmentation.enabled &&
    seg->alternate_quantizer_enabled) {
      if (priv->segmentation_abs_delta)
        seg_base = seg->alternate_quantizer;
      else
        seg_base += seg->alternate_quantizer;
  }
  return clamp(seg_base, 0, MAXQ);
}

uint8_t seg_get_filter_level(const Vp9Parser* parser, const Vp9FrameHdr * frame_hdr, int segid)
{
  int seg_filter = frame_hdr->loopfilter.filter_level;
  Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
  const Vp9SegmentationInfoData* seg = priv->segmentation + segid;

  if (frame_hdr->segmentation.enabled &&
      seg->alternate_loop_filter_enabled) {
    if (priv->segmentation_abs_delta)
      seg_filter = seg->alternate_loop_filter;
    else
      seg_filter += seg->alternate_loop_filter;
  }
  return clamp(seg_filter, 0, MAX_LOOP_FILTER);
}

/*save segmentation info from frame header to parser*/
static void segmentation_save(Vp9Parser* parser, const Vp9FrameHdr * frame_hdr)
{
  const Vp9SegmentationInfo* info = &frame_hdr->segmentation;
  if (!info->enabled)
    return;
  if (info->update_map) {
    ASSERT(sizeof(parser->mb_segment_tree_probs) == sizeof(info->tree_probs));
    ASSERT(sizeof(parser->segment_pred_probs) == sizeof(info->pred_probs));
    memcpy(parser->mb_segment_tree_probs, info->tree_probs, sizeof(info->tree_probs));
    memcpy(parser->segment_pred_probs, info->pred_probs, sizeof(info->pred_probs));
  }
  if (info->update_data) {
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    priv->segmentation_abs_delta = info->abs_delta;
    ASSERT(sizeof(priv->segmentation) == sizeof(info->data));
    memcpy(priv->segmentation, info->data, sizeof(info->data));
  }
}

static void segmentation_update(Vp9Parser* parser, const Vp9FrameHdr * frame_hdr)
{
  int i = 0;
  const Vp9ParserPrivate* priv = parser->priv;
  const Vp9LoopFilter* lf = &frame_hdr->loopfilter;
  int default_filter = lf->filter_level;
  const int scale = 1 << (default_filter >> 5);

  segmentation_save(parser, frame_hdr);

  for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
    uint8_t q = seg_get_base_qindex(parser, frame_hdr, i);

    Vp9Segmentation* seg = parser->segmentation+i;
    const Vp9SegmentationInfoData* info = priv->segmentation + i;

    seg->luma_dc_quant_scale = priv->y_dequant[q][0];
    seg->luma_ac_quant_scale = priv->y_dequant[q][1];
    seg->chroma_dc_quant_scale = priv->uv_dequant[q][0];
    seg->chroma_ac_quant_scale = priv->uv_dequant[q][1];

    if (lf->filter_level) {
      uint8_t filter = seg_get_filter_level(parser, frame_hdr, i);

      if (!lf->mode_ref_delta_enabled) {
        memset(seg->filter_level, filter, sizeof(seg->filter_level));
      } else {
        int ref, mode;
        const int intra_filter = filter + priv->ref_deltas[VP9_INTRA_FRAME] * scale;
        seg->filter_level[VP9_INTRA_FRAME][0] = clamp(intra_filter, 0, MAX_LOOP_FILTER);
        for (ref = VP9_LAST_FRAME; ref < VP9_MAX_REF_FRAMES; ++ref) {
          for (mode = 0; mode < VP9_MAX_MODE_LF_DELTAS; ++mode) {
            const int inter_filter = filter + priv->ref_deltas[ref] * scale
                                 + priv->mode_deltas[mode] * scale;
            seg->filter_level[ref][mode] = clamp(inter_filter, 0, MAX_LOOP_FILTER);
          }
        }
      }
    }
    seg->reference_frame_enabled = info->reference_frame_enabled;;
    seg->reference_frame = info->reference_frame;
    seg->reference_skip = info->reference_skip;
  }
}

static void reference_update(Vp9Parser* parser, const Vp9FrameHdr* const frame_hdr)
{
  uint8_t flag  = 1;
  uint8_t refresh_frame_flags;
  int i;
  Vp9ParserPrivate* priv = parser->priv;
  ReferenceSize* reference = priv->reference;
  if (frame_hdr->frame_type == VP9_KEY_FRAME) {
    refresh_frame_flags = 0xff;
  } else {
    refresh_frame_flags = frame_hdr->refresh_frame_flags;
  }
  for (i = 0; i < VP9_REF_FRAMES; i++) {
    if (refresh_frame_flags & flag) {
      reference[i].width = frame_hdr->width;
      reference[i].height = frame_hdr->height;
    }
    flag <<= 1;
  }
}

static inline  int frame_is_intra_only(const Vp9FrameHdr * frame_hdr)
{
  return frame_hdr->frame_type == VP9_KEY_FRAME || frame_hdr->intra_only;
}

static void set_default_lf_deltas(Vp9Parser* parser)
{
  Vp9ParserPrivate* priv = parser->priv;
  priv->ref_deltas[VP9_INTRA_FRAME] = 1;
  priv->ref_deltas[VP9_LAST_FRAME] = 0;
  priv->ref_deltas[VP9_GOLDEN_FRAME] = -1;
  priv->ref_deltas[VP9_ALTREF_FRAME] = -1;

  priv->mode_deltas[0] = 0;
  priv->mode_deltas[1] = 0;
}

static void setup_past_independence(Vp9Parser* parser)
{
  set_default_lf_deltas(parser);
}

static Vp9ParseResult vp9_parser_update(Vp9Parser* parser, const Vp9FrameHdr* const frame_hdr)
{
  if (frame_hdr->frame_type == VP9_KEY_FRAME) {
    init_vp9_parser(parser);
  }
  if (frame_is_intra_only(frame_hdr) || frame_hdr->error_resilient_mode) {
    setup_past_independence(parser);
  }
  loop_filter_update(parser, &frame_hdr->loopfilter);
  quantization_update(parser, frame_hdr);
  segmentation_update(parser, frame_hdr);
  reference_update(parser, frame_hdr);
  return VP9_PARSER_OK;
}

static BOOL read_display_attribute(Vp9FrameHdr * frame_hdr, BitReader* br)
{
  BOOL *ss_x = &frame_hdr->subsampling_x;
  BOOL *ss_y = &frame_hdr->subsampling_y;

  if (frame_hdr->profile > VP9_PROFILE_1)
    frame_hdr->bit_depth = vp9_read_bit(br);
  frame_hdr->color_space = (VP9_COLOR_SPACE)vp9_read_bits(br, 3);

  if (frame_hdr->color_space == VP9_SRGB) {
    if (frame_hdr->profile == VP9_PROFILE_1 || frame_hdr->profile == VP9_PROFILE_3) {
      *ss_x = *ss_y = 0;
      if (vp9_read_bit(br)) {
        goto reserve_bit;
      }
    } else {
        goto profile_not_support;
    }
  } else {
    vp9_read_bit(br);
    if (frame_hdr->profile == VP9_PROFILE_1 || frame_hdr->profile == VP9_PROFILE_3) {
      *ss_x = vp9_read_bit(br);
      *ss_y = vp9_read_bit(br);
      if (*ss_x == 1 && *ss_y == 1) {
        goto profile_not_support;
      }
      if (vp9_read_bit(br)) {
        goto reserve_bit;
      }
    } else {
      *ss_x = *ss_y = 1;
    }
  }

  return TRUE;
reserve_bit:
  ERROR("reserve bit");
  return FALSE;
profile_not_support:
  ERROR("YUV420 colorspace is not supported in profile VP9_PROFILE_1 and VP9_PROFILE_3");
  return FALSE;
}

Vp9ParseResult
vp9_parse_frame_header (Vp9Parser* parser, Vp9FrameHdr * frame_hdr, const uint8_t * data, uint32_t size)
{
#define FRAME_CONTEXTS_BITS 2
  BitReader bit_reader;
  BitReader* br = &bit_reader;
  bit_reader_init(br, data, size);
  memset(frame_hdr, 0, sizeof(*frame_hdr));
  /* Uncompressed Data Chunk */
  if (!verify_frame_marker(br))
    goto error;
  frame_hdr->profile = read_profile(br);
  if (frame_hdr->profile > MAX_VP9_PROFILES)
    goto error;
  frame_hdr->show_existing_frame = vp9_read_bit(br);
  if (frame_hdr->show_existing_frame) {
    frame_hdr->frame_to_show = vp9_read_bits(br, VP9_REF_FRAMES_LOG2);
    return VP9_PARSER_OK;
  }
  frame_hdr->frame_type = (VP9_FRAME_TYPE)vp9_read_bit(br);
  frame_hdr->show_frame = vp9_read_bit(br);
  frame_hdr->error_resilient_mode = vp9_read_bit(br);
  if (frame_hdr->frame_type == VP9_KEY_FRAME) {
    if (!verify_sync_code(br))
      goto error;
    if (!read_display_attribute(frame_hdr, br))
      goto error;
    read_frame_size(br, &frame_hdr->width, &frame_hdr->height);
    read_display_frame_size(frame_hdr, br);
  } else {
    frame_hdr->intra_only = frame_hdr->show_frame ? 0 : vp9_read_bit(br);
    frame_hdr->reset_frame_context = frame_hdr->error_resilient_mode ?
      0 : vp9_read_bits(br, 2);
    if (frame_hdr->intra_only) {
      if (!verify_sync_code(br))
        goto error;

      if (frame_hdr->profile > VP9_PROFILE_0) {
        if (!read_display_attribute(frame_hdr, br))
          goto error;
      } else {
        frame_hdr->subsampling_x = frame_hdr->subsampling_y = 1;
        frame_hdr->color_space = VP9_BT_601;
      }

      frame_hdr->refresh_frame_flags = vp9_read_bits(br, VP9_REF_FRAMES);
      read_frame_size(br, &frame_hdr->width, &frame_hdr->height);
      read_display_frame_size(frame_hdr, br);
    } else {
      int i;
      frame_hdr->refresh_frame_flags = vp9_read_bits(br, VP9_REF_FRAMES);
      for (i = 0; i < VP9_REFS_PER_FRAME; i++) {
        frame_hdr->ref_frame_indices[i] = vp9_read_bits(br, VP9_REF_FRAMES_LOG2);
        frame_hdr->ref_frame_sign_bias[i] = vp9_read_bit(br);
      }
      read_frame_size_from_refs(parser, frame_hdr, br);
      read_display_frame_size(frame_hdr, br);
      frame_hdr->allow_high_precision_mv = vp9_read_bit(br);
      frame_hdr->mcomp_filter_type = read_interp_filter(br);
    }
  }
  if (!frame_hdr->error_resilient_mode) {
    frame_hdr->refresh_frame_context = vp9_read_bit(br);
    frame_hdr->frame_parallel_decoding_mode = vp9_read_bit(br);
  } else {
    frame_hdr->frame_parallel_decoding_mode = TRUE;
  }
  frame_hdr->frame_context_idx = vp9_read_bits(br, FRAME_CONTEXTS_BITS);
  read_loopfilter(&frame_hdr->loopfilter, br);
  read_quantization(frame_hdr, br);
  read_segmentation(&frame_hdr->segmentation, br);
  if (!read_tile_info(frame_hdr, br))
    goto error;
  frame_hdr->first_partition_size = vp9_read_bits(br, 16);
  if (!frame_hdr->first_partition_size)
    goto error;
  frame_hdr->frame_header_length_in_bytes = (bit_reader_get_pos(br) + 7) / 8;
  return vp9_parser_update(parser, frame_hdr);
error:
  return VP9_PARSER_ERROR;
}
