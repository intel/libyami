/*
 * vp8rangedecoder.c - VP8 range decoder interface
 *
 * Use of this source code is governed by a BSD-style license
 * that can be found in the LICENSE file in the root of the source
 * tree. An additional intellectual property rights grant can be found
 * in the file PATENTS.  All contributing project authors may
 * be found in the AUTHORS file in the root of the source tree.
 */

#include "vp8rangedecoder.h"
#include "dboolhuff.h"

#define BOOL_DECODER_CAST(rd) \
  ((BOOL_DECODER *)(&(rd)->_reserved[0]))

bool
vp8_range_decoder_init (Vp8RangeDecoder * rd, const unsigned char * buf,
    uint32_t buf_size)
{
  BOOL_DECODER *const bd = BOOL_DECODER_CAST (rd);

  g_return_val_if_fail (sizeof (rd->_reserved) >= sizeof (*bd), FALSE);

  rd->buf = buf;
  rd->buf_size = buf_size;
  return vp8dx_start_decode (bd, buf, buf_size, NULL, NULL) == 0;
}

int32_t
vp8_range_decoder_read (Vp8RangeDecoder * rd, uint8_t prob)
{
  return vp8dx_decode_bool (BOOL_DECODER_CAST (rd), prob);
}

int32_t
vp8_range_decoder_read_literal (Vp8RangeDecoder * rd, int32_t bits)
{
  return vp8_decode_value (BOOL_DECODER_CAST (rd), bits);
}

uint32_t
vp8_range_decoder_get_pos (Vp8RangeDecoder * rd)
{
  BOOL_DECODER *const bd = BOOL_DECODER_CAST (rd);

  return (bd->user_buffer - rd->buf) * 8 - (8 + bd->count);
}

void
vp8_range_decoder_get_state (Vp8RangeDecoder * rd,
    Vp8RangeDecoderState * state)
{
  BOOL_DECODER *const bd = BOOL_DECODER_CAST (rd);

  if (bd->count < 0)
    vp8dx_bool_decoder_fill (bd);

  state->range = bd->range;
  state->value = (uint8_t) ((bd->value) >> (VP8_BD_VALUE_SIZE - 8));
  state->count = (8 + bd->count) % 8;
}
