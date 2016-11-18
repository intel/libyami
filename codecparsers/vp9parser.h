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

#ifndef __VP9_PARSER_H__
#define __VP9_PARSER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "common/common_def.h"

#define VP9_REFS_PER_FRAME 3

#define VP9_REF_FRAMES_LOG2 3
#define VP9_REF_FRAMES (1 << VP9_REF_FRAMES_LOG2)

#define VP9_MAX_LOOP_FILTER 63
#define VP9_MAX_SHARPNESS 7

#define VP9_MAX_REF_LF_DELTAS 4
#define VP9_MAX_MODE_LF_DELTAS 2

#define VP9_SEGMENT_DELTADATA 0
#define VP9_SEGMENT_ABSDATA 1

#define VP9_MAX_SEGMENTS 8
#define VP9_SEG_TREE_PROBS (VP9_MAX_SEGMENTS - 1)

#define VP9_PREDICTION_PROBS 3

typedef struct _Vp9FrameHdr Vp9FrameHdr;
typedef struct _Vp9Parser Vp9Parser;
typedef struct _Vp9LoopFilter Vp9LoopFilter;
typedef struct _Vp9Segmentation Vp9Segmentation;
typedef struct _Vp9SegmentationInfo Vp9SegmentationInfo;
typedef struct _Vp9SegmentationInfoData Vp9SegmentationInfoData;

/**
  * Vp9ParseResult:
  * @VP9_PARSER_OK: The parsing went well
  * @VP9_PARSER_NO_PACKET_ERROR: An error accured durint the parsing
  *
  * Result type of any parsing function.
  */
typedef enum {
    VP9_PARSER_OK,
    VP9_PARSER_ERROR,
    VP9_PARSER_UNSUPPORTED
} Vp9ParseResult;

typedef enum VP9_PROFILE {
    VP9_PROFILE_0,
    VP9_PROFILE_1,
    VP9_PROFILE_2,
    VP9_PROFILE_3,
    MAX_VP9_PROFILES
} VP9_PROFILE;

typedef enum VP9_BIT_DEPTH {
    VP9_BITS_8,
    VP9_BITS_10,
    VP9_BITS_12
} VP9_BIT_DEPTH;

typedef enum {
    VP9_UNKNOW_COLOR_SPACE = 0,
    VP9_BT_601 = 1,
    VP9_BT_709 = 2,
    VP9_SMPTE_170 = 3,
    VP9_SMPTE_240 = 4,
    VP9_RESERVED_1 = 5,
    VP9_RESERVED_2 = 6,
    VP9_SRGB = 7,
} VP9_COLOR_SPACE;

typedef enum {
    VP9_KEY_FRAME = 0,
    VP9_INTER_FRAME = 1,
} VP9_FRAME_TYPE;

typedef enum {
    VP9_EIGHTTAP = 0,
    VP9_EIGHTTAP_SMOOTH = 1,
    VP9_EIGHTTAP_SHARP = 2,
    VP9_BILINEAR = 3,
    VP9_SWITCHABLE = 4
} VP9_INTERP_FILTER;

typedef enum {
    VP9_INTRA_FRAME = 0,
    VP9_LAST_FRAME = 1,
    VP9_GOLDEN_FRAME = 2,
    VP9_ALTREF_FRAME = 3,
    VP9_MAX_REF_FRAMES = 4
} VP9_MV_REFERENCE_FRAME;

struct _Vp9LoopFilter {
    uint8_t filter_level;
    uint8_t sharpness_level;

    BOOL mode_ref_delta_enabled;
    BOOL mode_ref_delta_update;
    BOOL update_ref_deltas[VP9_MAX_REF_LF_DELTAS];
    int8_t ref_deltas[VP9_MAX_REF_LF_DELTAS];
    BOOL update_mode_deltas[VP9_MAX_MODE_LF_DELTAS];
    int8_t mode_deltas[VP9_MAX_MODE_LF_DELTAS];
};

struct _Vp9SegmentationInfoData {
    /* SEG_LVL_ALT_Q */
    BOOL alternate_quantizer_enabled;
    int16_t alternate_quantizer;

    /* SEG_LVL_ALT_LF */
    BOOL alternate_loop_filter_enabled;
    int8_t alternate_loop_filter;

    /* SEG_LVL_REF_FRAME */
    BOOL reference_frame_enabled;
    uint8_t reference_frame;

    BOOL reference_skip;
};

struct _Vp9SegmentationInfo {
    /* segmetation */
    /* enable in setup_segmentation*/
    BOOL enabled;
    /* update_map in setup_segmentation*/
    BOOL update_map;
    /* tree_probs exist or not*/
    BOOL update_tree_probs[VP9_SEG_TREE_PROBS];
    uint8_t tree_probs[VP9_SEG_TREE_PROBS];
    /* pred_probs exist or not*/
    BOOL update_pred_probs[VP9_PREDICTION_PROBS];
    uint8_t pred_probs[VP9_PREDICTION_PROBS];

    /* abs_delta in setup_segmentation */
    BOOL abs_delta;
    /* temporal_update in setup_segmentation */
    BOOL temporal_update;

    /* update_data in setup_segmentation*/
    BOOL update_data;
    Vp9SegmentationInfoData data[VP9_MAX_SEGMENTS];
};

struct _Vp9FrameHdr {
    VP9_PROFILE profile;
    BOOL show_existing_frame;
    uint8_t frame_to_show;
    VP9_FRAME_TYPE frame_type;
    BOOL show_frame;
    BOOL error_resilient_mode;
    BOOL subsampling_x;
    BOOL subsampling_y;
    uint32_t width;
    uint32_t height;
    BOOL display_size_enabled;
    uint32_t display_width;
    uint32_t display_height;
    uint8_t frame_context_idx;

    VP9_BIT_DEPTH bit_depth;
    VP9_COLOR_SPACE color_space;

    BOOL intra_only;
    uint8_t reset_frame_context;
    uint8_t refresh_frame_flags;

    uint8_t ref_frame_indices[VP9_REFS_PER_FRAME];
    BOOL ref_frame_sign_bias[VP9_REFS_PER_FRAME];
    BOOL size_from_ref[VP9_REFS_PER_FRAME];
    BOOL allow_high_precision_mv;
    VP9_INTERP_FILTER mcomp_filter_type;

    BOOL refresh_frame_context;
    /* frame_parallel_decoding_mode in vp9 code*/
    BOOL frame_parallel_decoding_mode;

    //quant
    uint8_t base_qindex;
    int8_t y_dc_delta_q;
    int8_t uv_dc_delta_q;
    int8_t uv_ac_delta_q;

    Vp9LoopFilter loopfilter;
    Vp9SegmentationInfo segmentation;

    uint8_t log2_tile_rows;
    uint8_t log2_tile_columns;

    uint32_t first_partition_size;
    uint32_t frame_header_length_in_bytes;
};

struct _Vp9Segmentation {
    uint8_t filter_level[4][2];
    int16_t luma_ac_quant_scale;
    int16_t luma_dc_quant_scale;
    int16_t chroma_ac_quant_scale;
    int16_t chroma_dc_quant_scale;

    BOOL reference_frame_enabled;
    uint8_t reference_frame;

    BOOL reference_skip;
};

struct _Vp9Parser {
    BOOL lossless_flag;
    VP9_BIT_DEPTH bit_depth;

    uint8_t mb_segment_tree_probs[VP9_SEG_TREE_PROBS];
    uint8_t segment_pred_probs[VP9_PREDICTION_PROBS];
    Vp9Segmentation segmentation[VP9_MAX_SEGMENTS];

    /* private data */
    void* priv;
};

Vp9ParseResult
vp9_parse_frame_header(Vp9Parser* parser, Vp9FrameHdr* frame_hdr, const uint8_t* data, uint32_t size);

Vp9Parser* vp9_parser_new();

void vp9_parser_free(Vp9Parser* parser);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VP9_PARSER_H__ */
