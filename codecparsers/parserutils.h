/* Gstreamer
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

#ifndef __PARSER_UTILS__
#define __PARSER_UTILS__

#include "bitreader.h"
#include "log.h"

#define GET_BITS(b, num, bits) {        \
  if (!bit_reader_get_bits_uint32(b, bits, num)) \
    LOG_ERROR ("parsed %d bits: %d", num, *(bits));\
    goto failed;                                  \
}

#define CHECK_ALLOWED(val, min, max) { \
  if (val < min || val > max) { \
    LOG_WARNING ("value not in allowed range. value: %d, range %d-%d", \
                     val, min, max); \
    goto failed; \
  } \
}

#define READ_UINT8(reader, val, nbits) { \
  if (!bit_reader_get_bits_uint8 (reader, &val, nbits)) { \
    LOG_WARNING ("failed to read uint8_t, nbits: %d \n", nbits); \
    goto failed; \
  } \
}

#define READ_UINT16(reader, val, nbits) { \
  if (!bit_reader_get_bits_uint16 (reader, &val, nbits)) { \
    LOG_WARNING ("failed to read uint16_t, nbits: %d \n", nbits); \
    goto failed; \
  } \
}

#define READ_UINT32(reader, val, nbits) { \
  if (!bit_reader_get_bits_uint32 (reader, &val, nbits)) { \
    LOG_WARNING ("failed to read uint32_t, nbits: %d", nbits); \
    goto failed; \
  } \
}

#define READ_UINT64(reader, val, nbits) G_STMT_START { \
  if (!bit_reader_get_bits_uint64 (reader, &val, nbits)) { \
    LOG_WARNING ("failed to read uint64_t, nbits: %d", nbits); \
    goto failed; \
  } \
} 

#define U_READ_UINT8(reader, val, nbits) { \
  val = bit_reader_get_bits_uint8_unchecked (reader, nbits); \
} 

#define U_READ_UINT16(reader, val, nbits) { \
  val = bit_reader_get_bits_uint16_unchecked (reader, nbits); \
}

#define U_READ_UINT32(reader, val, nbits) { \
  val = bit_reader_get_bits_uint32_unchecked (reader, nbits); \
}

#define U_READ_UINT64(reader, val, nbits) { \
  val = bit_reader_get_bits_uint64_unchecked (reader, nbits); \
}

#define SKIP(reader, nbits) { \
  if (!bit_reader_skip (reader, nbits)) { \
    LOG_WARNING ("failed to skip nbits: %d", nbits); \
    goto failed; \
  } \
}

#define ARRAY_N_ELEMENT(array) (sizeof(array)/sizeof(array[0]))

uint32_t bit_storage_calcuate(uint32_t value);
#define BIT_STORAGE_CALCULATE(value) bit_storage_calculate(value)

typedef struct _VLCTable
{
  uint32_t value;
  uint32_t cword;
  uint32_t cbits;
} VLCTable;

bool decode_vlc (BitReader * br, uint32_t * res, 
   const VLCTable * table, uint32_t length);

#endif /* __PARSER_UTILS__ */
