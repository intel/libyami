/*
 * vp8rangedecoder.h - VP8 range decoder interface
 *
 * Copyright (C) 2014 Intel Corporation
 *   Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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

#ifndef VP8_RANGE_DECODER_H
#define VP8_RANGE_DECODER_H

#include "gst/gst.h"

typedef struct _Vp8RangeDecoder      Vp8RangeDecoder;
typedef struct _Vp8RangeDecoderState Vp8RangeDecoderState;

/**
* Vp8RangeDecoder:
* @buf: the original bitstream buffer start
* @buf_size: the original bitstream buffer size
*
* Range decoder.
*/
struct _Vp8RangeDecoder {
  const unsigned char *buf;
  uint32_t buf_size;

  /*< private >*/
  void* _reserved[PADDING_LARGE];
};

/**
* Vp8RangeDecoderState:
* @range: current "Range" value
* @value: current "Value" value
* @count: number of bits left in "Value" for decoding
*
* Range decoder state.
*/
struct _Vp8RangeDecoderState {
  uint8_t range;
  uint8_t value;
  uint8_t count;
};

bool
vp8_range_decoder_init (Vp8RangeDecoder * rd, const unsigned char * buf,
   uint32_t buf_size);

int32_t
vp8_range_decoder_read (Vp8RangeDecoder * rd, uint8_t prob);

int32_t
vp8_range_decoder_read_literal (Vp8RangeDecoder * rd, int32_t bits);

uint32_t
vp8_range_decoder_get_pos (Vp8RangeDecoder * rd);

void
vp8_range_decoder_get_state (Vp8RangeDecoder * rd,
   Vp8RangeDecoderState * state);

#endif /* VP8_RANGE_DECODER_H */
