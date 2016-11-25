/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#include "bitReader.h"
#include "vp9quant.h"
#include "vp9parser.h"
#include <string.h>
#include <stdlib.h>
#include "common/log.h"

using YamiParser::BitReader;

#define MAX_LOOP_FILTER 63
#define MAX_PROB 255

typedef struct _ReferenceSize {
    uint32_t width;
    uint32_t height;
} ReferenceSize;

typedef struct _Vp9ParserPrivate {
    int8_t y_dc_delta_q;
    int8_t uv_dc_delta_q;
    int8_t uv_ac_delta_q;

    int16_t y_dequant[QINDEX_RANGE][2];
    int16_t uv_dequant[QINDEX_RANGE][2];

    /* for loop filters */
    int8_t ref_deltas[VP9_MAX_REF_LF_DELTAS];
    int8_t mode_deltas[VP9_MAX_MODE_LF_DELTAS];

    BOOL segmentation_abs_delta;
    Vp9SegmentationInfoData segmentation[VP9_MAX_SEGMENTS];

    ReferenceSize reference[VP9_REF_FRAMES];
} Vp9ParserPrivate;

void init_dequantizer(Vp9Parser* parser);

static void init_vp9_parser(Vp9Parser* parser, VP9_BIT_DEPTH bit_depth)
{
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    memset(parser, 0, sizeof(Vp9Parser));
    memset(priv, 0, sizeof(Vp9ParserPrivate));
    parser->priv = priv;
    parser->bit_depth = bit_depth;
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
    init_vp9_parser(parser, VP9_BITS_8);
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

#define vp9_read_bit(br) br->read(1)
#define vp9_read_bits(br, bits) br->read(bits)

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

static BOOL verify_sync_code(BitReader* const br)
{
#define VP9_SYNC_CODE_0 0x49
#define VP9_SYNC_CODE_1 0x83
#define VP9_SYNC_CODE_2 0x42
    return vp9_read_bits(br, 8) == VP9_SYNC_CODE_0 && vp9_read_bits(br, 8) == VP9_SYNC_CODE_1 && vp9_read_bits(br, 8) == VP9_SYNC_CODE_2;
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
    uint32_t* width, uint32_t* height)
{
    const uint32_t w = vp9_read_bits(br, 16) + 1;
    const uint32_t h = vp9_read_bits(br, 16) + 1;
    *width = w;
    *height = h;
}

static void read_display_frame_size(Vp9FrameHdr* frame_hdr, BitReader* br)
{
    frame_hdr->display_size_enabled = vp9_read_bit(br);
    if (frame_hdr->display_size_enabled) {
        read_frame_size(br, &frame_hdr->display_width, &frame_hdr->display_height);
    }
}

static void read_frame_size_from_refs(const Vp9Parser* parser, Vp9FrameHdr* frame_hdr, BitReader* br)
{
    BOOL found = FALSE;
    int i;
    const Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
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

static int8_t read_delta_q(BitReader* br)
{
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
            seg->tree_probs[i] = seg->update_tree_probs[i] ? vp9_read_bits(br, 8) : MAX_PROB;
        }
        seg->temporal_update = vp9_read_bit(br);
        if (seg->temporal_update) {
            for (i = 0; i < VP9_PREDICTION_PROBS; i++) {
                seg->update_pred_probs[i] = vp9_read_bit(br);
                seg->pred_probs[i] = seg->update_pred_probs[i] ? vp9_read_bits(br, 8) : MAX_PROB;
            }
        }
        else {
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
    uint32_t log2 = 0;
    while ((uint64_t)(MAX_TILE_WIDTH_B64 << log2) < sb_cols)
        ++log2;
    return log2;
}

/* align to 64, follow specification 6.2.6 Compute image size syntax */
#define SB_ALIGN(w) (((w) + 63) >> 6)
static BOOL read_tile_info(Vp9FrameHdr* frame_hdr, BitReader* br)
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

static void loop_filter_update(Vp9Parser* parser, const Vp9LoopFilter* lf)
{
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
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

void init_dequantizer(Vp9Parser* parser)
{
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    int q;
    for (q = 0; q < QINDEX_RANGE; q++) {
        priv->y_dequant[q][0] = vp9_dc_quant(parser->bit_depth, q, priv->y_dc_delta_q);
        priv->y_dequant[q][1] = vp9_ac_quant(parser->bit_depth, q, 0);
        priv->uv_dequant[q][0] = vp9_dc_quant(parser->bit_depth, q, priv->uv_dc_delta_q);
        priv->uv_dequant[q][1] = vp9_ac_quant(parser->bit_depth, q, priv->uv_ac_delta_q);
    }
}
static void quantization_update(Vp9Parser* parser, const Vp9FrameHdr* frame_hdr)
{
    BOOL update = FALSE;
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    update |= compare_and_set(&priv->y_dc_delta_q, frame_hdr->y_dc_delta_q);
    update |= compare_and_set(&priv->uv_dc_delta_q, frame_hdr->uv_dc_delta_q);
    update |= compare_and_set(&priv->uv_ac_delta_q, frame_hdr->uv_ac_delta_q);
    if (update) {
        init_dequantizer(parser);
    }
    parser->lossless_flag = frame_hdr->base_qindex == 0 && frame_hdr->y_dc_delta_q == 0 && frame_hdr->uv_dc_delta_q == 0 && frame_hdr->uv_ac_delta_q == 0;
}

uint8_t seg_get_base_qindex(const Vp9Parser* parser, const Vp9FrameHdr* frame_hdr, int segid)
{
    int seg_base = frame_hdr->base_qindex;
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    const Vp9SegmentationInfoData* seg = priv->segmentation + segid;
    /* DEBUG("id = %d, seg_base = %d, seg enable = %d, alt eanble = %d, abs = %d, alt= %d\n",segid,
     seg_base, frame_hdr->segmentation.enabled, seg->alternate_quantizer_enabled, priv->segmentation_abs_delta,  seg->alternate_quantizer);
 */
    if (frame_hdr->segmentation.enabled && seg->alternate_quantizer_enabled) {
        if (priv->segmentation_abs_delta)
            seg_base = seg->alternate_quantizer;
        else
            seg_base += seg->alternate_quantizer;
    }
    return clamp(seg_base, 0, MAXQ);
}

uint8_t seg_get_filter_level(const Vp9Parser* parser, const Vp9FrameHdr* frame_hdr, int segid)
{
    int seg_filter = frame_hdr->loopfilter.filter_level;
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    const Vp9SegmentationInfoData* seg = priv->segmentation + segid;

    if (frame_hdr->segmentation.enabled && seg->alternate_loop_filter_enabled) {
        if (priv->segmentation_abs_delta)
            seg_filter = seg->alternate_loop_filter;
        else
            seg_filter += seg->alternate_loop_filter;
    }
    return clamp(seg_filter, 0, MAX_LOOP_FILTER);
}

/*save segmentation info from frame header to parser*/
static void segmentation_save(Vp9Parser* parser, const Vp9FrameHdr* frame_hdr)
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

static void segmentation_update(Vp9Parser* parser, const Vp9FrameHdr* frame_hdr)
{
    int i = 0;
    const Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    const Vp9LoopFilter* lf = &frame_hdr->loopfilter;
    int default_filter = lf->filter_level;
    const int scale = 1 << (default_filter >> 5);

    segmentation_save(parser, frame_hdr);

    for (i = 0; i < VP9_MAX_SEGMENTS; i++) {
        uint8_t q = seg_get_base_qindex(parser, frame_hdr, i);

        Vp9Segmentation* seg = parser->segmentation + i;
        const Vp9SegmentationInfoData* info = priv->segmentation + i;

        seg->luma_dc_quant_scale = priv->y_dequant[q][0];
        seg->luma_ac_quant_scale = priv->y_dequant[q][1];
        seg->chroma_dc_quant_scale = priv->uv_dequant[q][0];
        seg->chroma_ac_quant_scale = priv->uv_dequant[q][1];

        if (lf->filter_level) {
            uint8_t filter = seg_get_filter_level(parser, frame_hdr, i);

            if (!lf->mode_ref_delta_enabled) {
                memset(seg->filter_level, filter, sizeof(seg->filter_level));
            }
            else {
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
        seg->reference_frame_enabled = info->reference_frame_enabled;
        ;
        seg->reference_frame = info->reference_frame;
        seg->reference_skip = info->reference_skip;
    }
}

static void reference_update(Vp9Parser* parser, const Vp9FrameHdr* const frame_hdr)
{
    uint8_t flag = 1;
    uint8_t refresh_frame_flags;
    int i;
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    ReferenceSize* reference = priv->reference;
    if (frame_hdr->frame_type == VP9_KEY_FRAME) {
        refresh_frame_flags = 0xff;
    }
    else {
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

static inline int key_or_intra_only(const Vp9FrameHdr* frame_hdr)
{
    return frame_hdr->frame_type == VP9_KEY_FRAME || frame_hdr->intra_only;
}

static void set_default_lf_deltas(Vp9Parser* parser)
{
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;
    priv->ref_deltas[VP9_INTRA_FRAME] = 1;
    priv->ref_deltas[VP9_LAST_FRAME] = 0;
    priv->ref_deltas[VP9_GOLDEN_FRAME] = -1;
    priv->ref_deltas[VP9_ALTREF_FRAME] = -1;

    priv->mode_deltas[0] = 0;
    priv->mode_deltas[1] = 0;
}

static void set_default_segmentation_info(Vp9Parser* parser)
{
    Vp9ParserPrivate* priv = (Vp9ParserPrivate*)parser->priv;

    memset(priv->segmentation, 0, sizeof(priv->segmentation));
    priv->segmentation_abs_delta = FALSE;
}

static void setup_past_independence(Vp9Parser* parser, Vp9FrameHdr* const frame_hdr)
{
    set_default_lf_deltas(parser);
    set_default_segmentation_info(parser);
    memset(frame_hdr->ref_frame_sign_bias, 0, sizeof(frame_hdr->ref_frame_sign_bias));
}

static Vp9ParseResult vp9_parser_update(Vp9Parser* parser, Vp9FrameHdr* const frame_hdr)
{
    if (frame_hdr->frame_type == VP9_KEY_FRAME) {
        init_vp9_parser(parser, frame_hdr->bit_depth);
    }
    if (key_or_intra_only(frame_hdr) || frame_hdr->error_resilient_mode) {
        setup_past_independence(parser, frame_hdr);
    }
    loop_filter_update(parser, &frame_hdr->loopfilter);
    quantization_update(parser, frame_hdr);
    segmentation_update(parser, frame_hdr);
    reference_update(parser, frame_hdr);
    return VP9_PARSER_OK;
}

static Vp9ParseResult vp9_color_config(Vp9Parser* parser, Vp9FrameHdr* frame_hdr, BitReader* br)
{
    if (frame_hdr->profile >= VP9_PROFILE_2) {
        frame_hdr->bit_depth = vp9_read_bit(br) ? VP9_BITS_12 : VP9_BITS_10;
        if (VP9_BITS_12 == frame_hdr->bit_depth) {
            ERROR("vp9 12 bits-depth is unsupported by now!");
            return VP9_PARSER_UNSUPPORTED;
        }
    }
    else {
        frame_hdr->bit_depth = VP9_BITS_8;
    }

    frame_hdr->color_space = (VP9_COLOR_SPACE)vp9_read_bits(br, 3);
    if (frame_hdr->color_space != VP9_SRGB) {
        vp9_read_bit(br); //color_range
        if (frame_hdr->profile == VP9_PROFILE_1 || frame_hdr->profile == VP9_PROFILE_3) {
            frame_hdr->subsampling_x = vp9_read_bit(br);
            frame_hdr->subsampling_y = vp9_read_bit(br);

            if (vp9_read_bit(br)) { //reserved_zero
                ERROR("reserved bit");
                return VP9_PARSER_ERROR;
            }
        }
        else {
            frame_hdr->subsampling_y = frame_hdr->subsampling_x = 1;
        }
    }
    else {
        ERROR("do not support SRGB");
        return VP9_PARSER_UNSUPPORTED;
    }

    return VP9_PARSER_OK;
}

Vp9ParseResult
vp9_parse_frame_header(Vp9Parser* parser, Vp9FrameHdr* frame_hdr, const uint8_t* data, uint32_t size)
{
#define FRAME_CONTEXTS_BITS 2
    BitReader bit_reader(data, size);
    BitReader* br = &bit_reader;
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
        if (vp9_color_config(parser, frame_hdr, br) != VP9_PARSER_OK)
            goto error;
        read_frame_size(br, &frame_hdr->width, &frame_hdr->height);
        read_display_frame_size(frame_hdr, br);
    }
    else {
        frame_hdr->intra_only = frame_hdr->show_frame ? 0 : vp9_read_bit(br);
        frame_hdr->reset_frame_context = frame_hdr->error_resilient_mode ? 0 : vp9_read_bits(br, 2);
        if (frame_hdr->intra_only) {
            if (!verify_sync_code(br))
                goto error;
            if (frame_hdr->profile > VP9_PROFILE_0) {
                if (vp9_color_config(parser, frame_hdr, br) != VP9_PARSER_OK)
                    goto error;
            }
            else {
                frame_hdr->color_space = VP9_BT_601;
                frame_hdr->subsampling_y = frame_hdr->subsampling_x = 1;
                frame_hdr->bit_depth = VP9_BITS_8;
            }

            frame_hdr->refresh_frame_flags = vp9_read_bits(br, VP9_REF_FRAMES);
            read_frame_size(br, &frame_hdr->width, &frame_hdr->height);
            read_display_frame_size(frame_hdr, br);
        }
        else {
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
    }
    else {
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
    frame_hdr->frame_header_length_in_bytes = (br->getPos() + 7) / 8;
    return vp9_parser_update(parser, frame_hdr);
error:
    return VP9_PARSER_ERROR;
}
