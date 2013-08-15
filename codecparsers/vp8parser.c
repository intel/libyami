/* gstreamer
 * Copyright (C) <2013> Intel Coperation
 * Copyright (C) <2013> Halley Zhao<halley.zhao@intel.com>
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
 * SECTION:vp8parser
 * @short_description: Convenience library for parsing vp8 part 2 video
 * bitstream.
 *
 * For more details about the structures, you can refer to the
 * specifications: ISO-IEC-14496-2_2004_vp8_VISUAL.pdf
 */

#include "vp8parser.h"

#define BitDecoderType  VP56RangeCoder
#define READ_UINT32(br, val, nbits, field_name) \
    do { val = vp8_rac_get_uint(br, nbits);  } while(0)
#define READ_UINT8(br, val, nbits, field_name) \
    do { val = vp8_rac_get_uint(br, nbits);  } while(0)

typedef struct VP56RangeCoder {
    int32 high;
    int32 bits; /* stored negated (i.e. negative "bits" is a positive number of
                 bits left) in order to eliminate a negate in cache refilling */
    const uint8 *buffer;
    const uint8 *end;
    int32 code_word;
} VP56RangeCoder;

const uint8 ff_vp56_norm_shift[256]= {
 8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,
 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

static const uint8 vp8_token_update_probs[4][8][3][11] =
{
    {
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 176, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 223, 241, 252, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 249, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 244, 252, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 234, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 246, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 239, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 251, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 251, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 253, 255, 254, 255, 255, 255, 255, 255, 255 },
            { 250, 255, 254, 255, 254, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
    },
    {
        {
            { 217, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 225, 252, 241, 253, 255, 255, 254, 255, 255, 255, 255 },
            { 234, 250, 241, 250, 253, 255, 253, 254, 255, 255, 255 },
        },
        {
            { 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 223, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 238, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 249, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 247, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
    },
    {
        {
            { 186, 251, 250, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 234, 251, 244, 254, 255, 255, 255, 255, 255, 255, 255 },
            { 251, 251, 243, 253, 254, 255, 254, 255, 255, 255, 255 },
        },
        {
            { 255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 236, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 251, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
    },
    {
        {
            { 248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 250, 254, 252, 254, 255, 255, 255, 255, 255, 255, 255 },
            { 248, 254, 249, 253, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 246, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 252, 254, 251, 254, 254, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 254, 252, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 248, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 253, 255, 254, 254, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 245, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 249, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
        {
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
            { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
        },
    },
};
static const uint8 vp8_mv_update_prob[2][19] = {
    { 237,
      246,
      253, 253, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 250, 250, 252, 254, 254 },
    { 231,
      243,
      245, 253, 254, 254, 254, 254, 254,
      254, 254, 254, 254, 254, 251, 251, 254, 254, 254 }
};

static int32 vp56_rac_get_prob(VP56RangeCoder *c, uint8 prob);

uint32 bytestream_get_be24(const uint8 **pbuf)
{
    const uint8 *buf = *pbuf;
    *pbuf = *pbuf+3;
    return (buf[0]<<16) | (buf[1]<<8) | buf[2];
}

uint32 bytestream_get_be16(const uint8 **pbuf)
{
    const uint8 *buf = *pbuf;
    *pbuf = *pbuf+2;
    return (buf[0]<<8) | buf[1];
}

void 
ff_vp56_init_range_decoder(VP56RangeCoder *c, const uint8 *buf, int32 buf_size)
{
    c->high = 255;
    c->bits = -16;
    c->buffer = buf;
    c->end = buf + buf_size;
    c->code_word = bytestream_get_be24(&c->buffer);
}

// rounding is different than vp56_rac_get, is vp56_rac_get wrong?
static int32 
vp8_rac_get(VP56RangeCoder *c)
{
    return vp56_rac_get_prob(c, 128);
}

static int32
vp56_rac_renorm(VP56RangeCoder *c)
{
    int32 shift = ff_vp56_norm_shift[c->high];
    int32 bits = c->bits;
    int32 code_word = c->code_word;

    c->high   <<= shift;
    code_word <<= shift;
    bits       += shift;
    if(bits >= 0 && c->buffer < c->end) {
        code_word |= bytestream_get_be16(&c->buffer) << bits;
        bits -= 16;
    }
    c->bits = bits;
    return code_word;
}

static int32
vp56_rac_get_prob(VP56RangeCoder *c, uint8 prob)
{
    static int32 count = 0;
    int32 code_word = vp56_rac_renorm(c);
    int32 low = 1 + (((c->high - 1) * prob) >> 8);
    int32 low_shift = low << 16;
    int32 bit = code_word >= low_shift;

    c->high = bit ? c->high - low : low;
    c->code_word = bit ? code_word - low_shift : code_word;
    count++;

    return bit;
}

static int32
vp56_rac_get_prob_branchy(VP56RangeCoder *c, int32 prob)
{
    unsigned long code_word = vp56_rac_renorm(c);
    unsigned low = 1 + (((c->high - 1) * prob) >> 8);
    unsigned low_shift = low << 16;

    if (code_word >= low_shift) {
        c->high     -= low;
        c->code_word = code_word - low_shift;
        return 1;
    }

    c->high = low;
    c->code_word = code_word;
    return 0;
}

static int32
vp8_rac_get_uint(VP56RangeCoder *c, int32 bits)
{
    int32 value = 0;

    while (bits--) {
        value = (value << 1) | vp8_rac_get(c);
    }

    return value;
}

static void
vp8_rac_get_pos(VP56RangeCoder *c, uint8 **buffer, uint32 *offset_bits)
{
    uint32 remaining_bits = 8-c->bits;
    *buffer = (uint8*) c->buffer; 
    while (remaining_bits>8) {
        *buffer = *buffer - 1;
        remaining_bits -= 8;
    }

    *buffer = *buffer - 1;
    *offset_bits = 8 - remaining_bits;
}

static boolean 
update_segmentation(BitDecoderType *BitDecoder, Vp8FrameHdr *frame_hdr)
{
    Vp8Segmentation *seg = &frame_hdr->segmentation;
    int32 i;
    uint8 tmp;

    READ_UINT8(BitDecoder, seg->update_mb_segmentation_map, 1, "update_mb_segmentation_map");
    READ_UINT8(BitDecoder, seg->update_segment_feature_data, 1, "update_segment_feature_data");
    if (seg->update_segment_feature_data) {
        READ_UINT8(BitDecoder, seg->segment_feature_mode, 1, "segment_feature_mode");
        for (i = 0; i < 4; i++) {
            READ_UINT8(BitDecoder, tmp, 1, "quantizer_update");
            if (tmp) {
                READ_UINT8(BitDecoder, seg->quantizer_update_value[i], 7, "quantizer_update_value");
                READ_UINT8(BitDecoder, tmp, 1, "quantizer_update_sign");
                if (tmp) 
                   seg->quantizer_update_value[i] = -seg->quantizer_update_value[i];
            }
        }

        for (i = 0; i < 4; i++) {
            READ_UINT8(BitDecoder, tmp, 1, "loop_filter_update");
            if (tmp) {
                READ_UINT8(BitDecoder, seg->lf_update_value[i], 6, "lf_update_value");
                READ_UINT8(BitDecoder, tmp, 1, "lf_update_sign");
                if (tmp) 
                   seg->lf_update_value[i] = -seg->lf_update_value[i];
            }
        }
    }

    if (seg->update_mb_segmentation_map) {
        for (i = 0; i < 3; i++) {
            READ_UINT8(BitDecoder, tmp, 1, "segment_prob_update");
            if (tmp) {
                READ_UINT8(BitDecoder, seg->segment_prob[i], 8, "segment_prob");
            }
        }
    }
    
    return TRUE;

error:
    LOG_WARNING("failed in %s", __FUNCTION__);
    return FALSE;
}

static boolean 
mb_lf_adjustments(BitDecoderType *BitDecoder, Vp8FrameHdr *frame_hdr)
{
    Vp8MbLfAdjustments *mb_lf_adjust = &frame_hdr->mb_lf_adjust;
    int32 i;
    uint8  tmp;

    READ_UINT8(BitDecoder, mb_lf_adjust->loop_filter_adj_enable, 1, "loop_filter_adj_enable");
    if (mb_lf_adjust->loop_filter_adj_enable) {
        READ_UINT8(BitDecoder, mb_lf_adjust->mode_ref_lf_delta_update, 1, "mode_ref_lf_delta_update");
        if (mb_lf_adjust->mode_ref_lf_delta_update) {
            for (i = 0; i < 4; i++) {
                READ_UINT8(BitDecoder, tmp, 1, "ref_frame_delta_update_flag");
                if (tmp) {
                    READ_UINT8(BitDecoder, mb_lf_adjust->ref_frame_delta[i], 6, "delta_magnitude");
                    READ_UINT8(BitDecoder, tmp, 1, "delta_sign");
                    if (tmp) 
                        mb_lf_adjust->ref_frame_delta[i] = -mb_lf_adjust->ref_frame_delta[i];
                }
            }
            for (i = 0; i < 4; i++) {
                READ_UINT8(BitDecoder, tmp, 1, "mb_mode_delta_update_flag");
                if (tmp) {
                    READ_UINT8(BitDecoder, mb_lf_adjust->mb_mode_delta[i], 6, "delta_magnitude");
                    READ_UINT8(BitDecoder, tmp, 1, "delta_sign");
                    if (tmp) 
                        mb_lf_adjust->mb_mode_delta[i] = -mb_lf_adjust->mb_mode_delta[i];
                }
            }
        }
    }
    return TRUE;

error:
    LOG_WARNING("failed in %s", __FUNCTION__);
    return FALSE;
}

static boolean 
quant_indices_parse(BitDecoderType *BitDecoder, Vp8FrameHdr *frame_hdr)
{
    Vp8QuantIndices *quant_indices = &frame_hdr->quant_indices;
    int32 i;
    uint8 tmp;

    READ_UINT8(BitDecoder, quant_indices->y_ac_qi, 7, "y_ac_qi");
    READ_UINT8(BitDecoder, tmp, 1, "y_dc_delta_present");
    if (tmp) {
        READ_UINT8(BitDecoder, quant_indices->y_dc_delta, 4, "y_dc_delta_present");
        READ_UINT8(BitDecoder, tmp, 1, "y_dc_delta_sign");
        if (tmp) 
           quant_indices->y_dc_delta = -quant_indices->y_dc_delta;
    }

    READ_UINT8(BitDecoder, tmp, 1, "y2_dc_delta_present");
    if (tmp) {
        READ_UINT8(BitDecoder, quant_indices->y2_dc_delta, 4, "y2_dc_delta_magnitude");
        READ_UINT8(BitDecoder, tmp, 1, "y2_dc_delta_sign");
        if (tmp) 
           quant_indices->y2_dc_delta = -quant_indices->y2_dc_delta;
    }

    READ_UINT8(BitDecoder, tmp, 1, "y2_ac_delta_present");
    if (tmp) {
        READ_UINT8(BitDecoder, quant_indices->y2_ac_delta, 4, "y2_ac_delta_magnitude");
        READ_UINT8(BitDecoder, tmp, 1, "y2_ac_delta_sign");
        if (tmp) 
           quant_indices->y2_ac_delta= -quant_indices->y2_ac_delta;
    }

    READ_UINT8(BitDecoder, tmp, 1, "uv_dc_delta_present");
    if (tmp) {
        READ_UINT8(BitDecoder, quant_indices->uv_dc_delta, 4, "uv_dc_delta_magnitude");
        READ_UINT8(BitDecoder, tmp, 1, "uv_dc_delta_sign");
        if (tmp) 
           quant_indices->uv_dc_delta= -quant_indices->uv_dc_delta;
    }

    READ_UINT8(BitDecoder, tmp, 1, "uv_ac_delta_present");
    if (tmp) {
        READ_UINT8(BitDecoder, quant_indices->uv_ac_delta, 4, "uv_ac_delta_magnitude");
        READ_UINT8(BitDecoder, tmp, 1, "uv_ac_delta_sign");
        if (tmp) 
           quant_indices->uv_ac_delta= -quant_indices->uv_ac_delta;
    }

    return TRUE;

error:
    LOG_WARNING("failed in %s", __FUNCTION__);
    return FALSE;
}

static boolean 
token_prob_update(BitDecoderType *BitDecoder, Vp8FrameHdr *frame_hdr)
{
    Vp8TokenProbUpdate *token_prob_update = &frame_hdr->token_prob_update;
    int32 i, j, k, l;
    uint8 tmp;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            for (k = 0; k < 3; k++) {
                for (l = 0; l < 11; l++) {
                    if (vp56_rac_get_prob_branchy(BitDecoder, vp8_token_update_probs[i][j][k][l])) {
                        uint8 prob = vp8_rac_get_uint(BitDecoder, 8);
                        token_prob_update->coeff_prob[i][j][k][l] = prob;
                    }
                }
            }
        }
    }

    return TRUE;

error:
    LOG_WARNING("failed in %s", __FUNCTION__);
    return FALSE;
}

static int32
vp8_rac_get_nn(VP56RangeCoder *c)
{
    int32 v = vp8_rac_get_uint(c, 7) << 1;
    return v + !v;
}

static boolean 
mv_prob_update(BitDecoderType *BitDecoder, Vp8FrameHdr *frame_hdr)
{
    Vp8MvProbUpdate *mv_prob_update = &frame_hdr->mv_prob_update;
    int32 i, j;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 19; j++) {
           if (vp56_rac_get_prob_branchy(BitDecoder, vp8_mv_update_prob[i][j])) {
                mv_prob_update->prob[i][j] = vp8_rac_get_nn(BitDecoder);
            }
        }
    }
    return TRUE;

error:
    LOG_WARNING("failed in %s", __FUNCTION__);
    return FALSE;
}

/**
 * vp8_parse:
 * @frame_hdr: The #Vp8FrameHdr to fill
 * @offset: offset from which to start the parsing
 * @data: The data to parse
 * @size: The size of the @data to parse
 *
 * Parses @data and fills @frame_hdr with the information 
 * Returns: a #Vp8ParseResult
 */

Vp8ParseResult 
vp8_parse_frame_header(Vp8FrameHdr *frame_hdr, const uint8 * data, 
    uint32 offset, size_t size)
{
    ByteReader Br;
#if USE_BIT_READER
    BitReader  BitDecoder;
#else
    VP56RangeCoder BitDecoder;
#endif

    Vp8ParseResult parse_result = VP8_PARSER_ERROR;
    int32     frame_tag = 0;
    uint32    tmp = 0;
    uint16    tmp_16 = 0;
    int32     i, j;

    byte_reader_init(&Br, data, size);
    if (!byte_reader_get_int24_le(&Br, &frame_tag)) {
        LOG_WARNING("byte_reader_get_int24_le");
        goto error;
    }

    // VP8 reference code shows the '!' for key_frame parsing.
    frame_hdr->key_frame    = !(frame_tag & 0x01);
    frame_hdr->version      = (frame_tag >> 1) & 0x07;
    frame_hdr->show_frame   = (frame_tag >> 4) & 0x01;
    frame_hdr->first_part_size  = (frame_tag >> 5) & 0x7ffff;

    if (frame_hdr->key_frame) {
        if (!byte_reader_get_uint24_be(&Br, &tmp)) {
            LOG_WARNING("byte_reader_get_int24_be");
            goto error;
        }
        if (tmp != 0x9d012a) 
           LOG_WARNING("vp8 parser: invalid start code in frame header.");

        if (!byte_reader_get_uint16_le(&Br, &tmp_16)) {
            LOG_WARNING("byte_reader_get_int16_le");
            goto error;
        }
        frame_hdr->width = tmp_16 & 0x3ffff;
        frame_hdr->width = frame_hdr->width << (tmp_16 >> 14);

        if (!byte_reader_get_uint16_le(&Br, &tmp_16)) {
            LOG_WARNING("byte_reader_get_int16_le");
            goto error;
        }
        frame_hdr->height = tmp_16 & 0x3fff;
        frame_hdr->height = frame_hdr->height << (tmp_16 >> 14);

    }

    int32 pos = byte_reader_get_pos(&Br);

    // * Frame Header
#if USE_BIT_READER
    bit_reader_init(&BitDecoder, data+pos, size-pos);
#else
    ff_vp56_init_range_decoder(&BitDecoder, data+pos, size-pos);
#endif

    if (frame_hdr->key_frame) {
        READ_UINT32(&BitDecoder, tmp, 2, "color_space & clamping_type");
        frame_hdr->color_space      = (tmp & 0x02) ? 1:0;
        frame_hdr->clamping_type    = tmp & 0x01;
    }

    READ_UINT32(&BitDecoder, tmp, 1, "segmentation_enabled");
    frame_hdr->segmentation_enabled = tmp & 0x01;
    if (frame_hdr->segmentation_enabled) {
        if (!update_segmentation(&BitDecoder, frame_hdr)) {
            LOG_WARNING("update_segmentation");
            goto error;
        }
    }

    READ_UINT32(&BitDecoder, tmp, 1, "filter_type");
    frame_hdr->filter_type = tmp & 0x01;

    READ_UINT32(&BitDecoder, tmp, 6, "loop_filter_level");
    frame_hdr->loop_filter_level = tmp & 0x3f;

    READ_UINT32(&BitDecoder, tmp, 3, "sharpness_level");
    frame_hdr->sharpness_level = tmp & 0x07;

   if (!mb_lf_adjustments(&BitDecoder, frame_hdr)) {
       LOG_WARNING("mb_lf_adjustments");
       goto error; 
    }

    READ_UINT32(&BitDecoder, tmp, 2, "log2_nbr_of_dct_partitions");
    frame_hdr->log2_nbr_of_dct_partitions = tmp & 0x03;
    if (frame_hdr->log2_nbr_of_dct_partitions) {
        int32 par_count = 1 << frame_hdr->log2_nbr_of_dct_partitions;
        int32 i=0;
        do {
            unsigned char c[3];
            READ_UINT8(&BitDecoder, c[0], 8, "partition size c[0]");
            READ_UINT8(&BitDecoder, c[1], 8, "partition size c[1]");
            READ_UINT8(&BitDecoder, c[2], 8, "partition size c[2]");
            frame_hdr->partition_size[i] = c[0] + (c[1]<<8) + (c[2]<<16);
        } while (++i<par_count); // todo ++i<par_count? since the last partition size is not specified P9
    }

    if (!quant_indices_parse(&BitDecoder, frame_hdr)) {
        LOG_WARNING("quant_indices_parse");
        goto error; 
    }

    if (frame_hdr->key_frame) {
        READ_UINT32(&BitDecoder, tmp, 1, "refresh_entropy_probs");
        frame_hdr->refresh_entropy_probs = tmp & 0x01;
    }

    else {
        READ_UINT32(&BitDecoder, tmp, 2, "refresh_golden_frame");
        frame_hdr->refresh_golden_frame     = (tmp & 0x02) ? 1:0;
        frame_hdr->refresh_alternate_frame  = tmp & 0x01;
        if (!frame_hdr->refresh_golden_frame) {
            READ_UINT8(&BitDecoder, frame_hdr->copy_buffer_to_golden, 2, "copy_buffer_to_golden");
        }
        if (!frame_hdr->refresh_alternate_frame) {
            READ_UINT8(&BitDecoder, frame_hdr->copy_buffer_to_alternate, 2, "copy_buffer_to_alternate");
        }

        READ_UINT32(&BitDecoder, tmp, 4, "sign_bias_golden, sign_bias_alternate, refresh_entropy_probs, refresh_last");
        frame_hdr->sign_bias_golden     = (tmp & 0x08) ? 1:0;
        frame_hdr->sign_bias_alternate  = (tmp & 0x04) ? 1:0;
        frame_hdr->refresh_entropy_probs= (tmp & 0x02) ? 1:0;
        frame_hdr->refresh_last         = (tmp & 0x01) ? 1:0;
   }

    if (!token_prob_update(&BitDecoder, frame_hdr)) {
        LOG_WARNING("token_prob_update");
        goto error;
    }

    READ_UINT32(&BitDecoder, tmp, 1, "mb_no_skip_coeff");
    frame_hdr->mb_no_skip_coeff = tmp & 0x01;
    if (frame_hdr->mb_no_skip_coeff) {
        READ_UINT8(&BitDecoder, frame_hdr->prob_skip_false, 8, "prob_skip_false");
    }

    if (!frame_hdr->key_frame) {
        READ_UINT8(&BitDecoder, frame_hdr->prob_intra, 8, "prob_intra");
        READ_UINT8(&BitDecoder, frame_hdr->prob_last, 8, "prob_last");
        READ_UINT8(&BitDecoder, frame_hdr->prob_gf, 8, "prob_gf");
        READ_UINT32(&BitDecoder, tmp, 1, "intra_16x16_prob_update_flag");
        frame_hdr->intra_16x16_prob_update_flag = tmp & 0x01;

        if (frame_hdr->intra_16x16_prob_update_flag) {
            for (i = 0; i < 4; i++) {
                READ_UINT8(&BitDecoder, frame_hdr->intra_16x16_prob[i], 8, "intra_16x16_prob");
            }
        }

        READ_UINT32(&BitDecoder, tmp, 1, "intra_chroma_prob_update_flag");
        if (tmp & 0x01) {
            for (i = 0; i < 3; i++) {
                READ_UINT8(&BitDecoder, frame_hdr->intra_chroma_prob[i], 8, "intra_chroma_prob");
            }
        }

        if (!mv_prob_update(&BitDecoder, frame_hdr)) {
            LOG_WARNING("mv_prob_update");
            goto error;
        }

    }

    vp8_rac_get_pos(&BitDecoder, &frame_hdr->mb_buffer, &frame_hdr->mb_offset_bits);

#if USE_BIT_READER
    bit_reader_free(&BitDecoder);
#else
    // it seems we needn't free VP56RangeCoder
#endif
    return VP8_PARSER_OK;

error:
#if USE_BIT_READER
    bit_reader_free(&BitDecoder);
#else
    // it seems we needn't free VP56RangeCoder
#endif
    LOG_WARNING("failed in %s", __FUNCTION__);

    return VP8_PARSER_ERROR;
}
