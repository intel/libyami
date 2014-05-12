/* GStreamer
 *
 * Copyright (C) 2008 Sebastian Dröge <sebastian.droege@collabora.co.uk>.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BIT_READER_H__
#define __BIT_READER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "log.h"

#define BIT_READER(reader) ((BitReader *) (reader))
#define MIN(a, b) ((a)>(b))?(b):(a)
#define MAX(a, b) ((a)>(b))?(a):(b)

/**
 * BitReader:
 * @data: Data from which the bit reader will read
 * @size: Size of @data in bytes
 * @byte: Current byte position
 * @bit: Bit position in the current byte
 *
 * A bit reader instance.
 */
typedef struct {
  const uint8_t *data;
  uint32_t size;
  uint32_t byte;  /* Byte position */
  uint32_t bit;   /* Bit position in the current byte */
} BitReader;

BitReader * bit_reader_new (const uint8_t *data, uint32_t size);
void bit_reader_init (BitReader *reader, const uint8_t *data, uint32_t size);
void bit_reader_free (BitReader *reader);

bool bit_reader_set_pos (BitReader *reader, uint32_t pos);
uint32_t bit_reader_get_pos (const BitReader *reader);
uint32_t bit_reader_get_remaining (const BitReader *reader);

uint32_t bit_reader_get_size (const BitReader *reader);

bool bit_reader_skip (BitReader *reader, uint32_t nbits);
bool bit_reader_skip_to_byte (BitReader *reader);

bool bit_reader_get_bits_uint8 (BitReader *reader, uint8_t *val, uint32_t nbits);
bool bit_reader_get_bits_uint16 (BitReader *reader, uint16_t *val, uint32_t nbits);
bool bit_reader_get_bits_uint32 (BitReader *reader, uint32_t *val, uint32_t nbits);
bool bit_reader_get_bits_uint64 (BitReader *reader, uint64_t *val, uint32_t nbits);

bool bit_reader_peek_bits_uint8 (const BitReader *reader, uint8_t *val, uint32_t nbits);
bool bit_reader_peek_bits_uint16 (const BitReader *reader, uint16_t *val, uint32_t nbits);
bool bit_reader_peek_bits_uint32 (const BitReader *reader, uint32_t *val, uint32_t nbits);
bool bit_reader_peek_bits_uint64 (const BitReader *reader, uint64_t *val, uint32_t nbits);

/**
 * BIT_READER_INIT:
 * @data: Data from which the #BitReader should read
 * @size: Size of @data in bytes
 *
 * A #BitReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * bit_reader_init().
 *
 * Since: 0.10.22
 */
#define BIT_READER_INIT(data, size) {data, size, 0, 0}

/**
 * BIT_READER_INIT_FROM_BUFFER:
 * @buffer: Buffer from which the #BitReader should read
 *
 * A #BitReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * bit_reader_init().
 *
 * Since: 0.10.22
 */
#define BIT_READER_INIT_FROM_BUFFER(buffer) {BUFFER_DATA (buffer), BUFFER_SIZE (buffer), 0, 0}

/* Unchecked variants */

static inline void
bit_reader_skip_unchecked (BitReader * reader, uint32_t nbits)
{
  reader->bit += nbits;
  reader->byte += reader->bit / 8;
  reader->bit = reader->bit % 8;
}

static inline void
bit_reader_skip_to_byte_unchecked (BitReader * reader)
{
  if (reader->bit) {
    reader->bit = 0;
    reader->byte++;
  }
}

#define BIT_READER_READ_BITS_UNCHECKED(bits) \
static inline uint##bits##_t \
bit_reader_peek_bits_uint##bits##_unchecked (const BitReader *reader, uint32_t nbits) \
{ \
  uint##bits##_t ret = 0; \
  const uint8_t *data; \
  uint32_t byte, bit; \
  \
  data = reader->data; \
  byte = reader->byte; \
  bit = reader->bit; \
  \
  while (nbits > 0) { \
    uint32_t toread = MIN (nbits, 8 - bit); \
    \
    ret <<= toread; \
    ret |= (data[byte] & (0xff >> bit)) >> (8 - toread - bit); \
    \
    bit += toread; \
    if (bit >= 8) { \
      byte++; \
      bit = 0; \
    } \
    nbits -= toread; \
  } \
  \
  return ret; \
} \
\
static inline uint##bits##_t \
bit_reader_get_bits_uint##bits##_unchecked (BitReader *reader, uint32_t nbits) \
{ \
  uint##bits##_t ret; \
  \
  ret = bit_reader_peek_bits_uint##bits##_unchecked (reader, nbits); \
  \
  bit_reader_skip_unchecked (reader, nbits); \
  \
  return ret; \
}

BIT_READER_READ_BITS_UNCHECKED (8)
BIT_READER_READ_BITS_UNCHECKED (16)
BIT_READER_READ_BITS_UNCHECKED (32)
BIT_READER_READ_BITS_UNCHECKED (64)

#undef BIT_READER_READ_BITS_UNCHECKED

/* unchecked variants -- do not use */
static inline uint32_t
bit_reader_get_size_unchecked (const BitReader * reader)
{
  return reader->size * 8;
}

static inline uint32_t
bit_reader_get_pos_unchecked (const BitReader * reader)
{
  return reader->byte * 8 + reader->bit;
}

static inline uint32_t
bit_reader_get_remaining_unchecked (const BitReader * reader)
{
  return reader->size * 8 - (reader->byte * 8 + reader->bit);
}

/* inlined variants -- do not use directly */
static inline uint32_t
bit_reader_get_size_inline (const BitReader * reader)
{
  RETURN_VAL_IF_FAIL(reader != NULL, 0);

  return bit_reader_get_size_unchecked (reader);
}

static inline uint32_t
bit_reader_get_pos_inline (const BitReader * reader)
{
  RETURN_VAL_IF_FAIL(reader != NULL, 0);

  return bit_reader_get_pos_unchecked (reader);
}

static inline uint32_t
bit_reader_get_remaining_inline (const BitReader * reader)
{
  RETURN_VAL_IF_FAIL(reader != NULL, 0);

   return bit_reader_get_remaining_unchecked (reader);
}

static inline bool
bit_reader_skip_inline (BitReader * reader, uint32_t nbits)
{
   RETURN_VAL_IF_FAIL(reader != NULL, false);

  if (bit_reader_get_remaining_unchecked (reader) < nbits)
    return false;

  bit_reader_skip_unchecked (reader, nbits);

  return true;
}

static inline bool
bit_reader_skip_to_byte_inline (BitReader * reader)
{
  RETURN_VAL_IF_FAIL(reader != NULL, false);

  if (reader->byte > reader->size)
    return false;

  bit_reader_skip_to_byte_unchecked (reader);

  return true;
}

#define BIT_READER_READ_BITS_INLINE(bits) \
static inline bool \
bit_reader_get_bits_uint##bits##_inline (BitReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  RETURN_VAL_IF_FAIL (val != NULL, false); \
  RETURN_VAL_IF_FAIL (nbits <= bits, false); \
  \
  if (bit_reader_get_remaining_unchecked (reader) < nbits) \
    return false; \
\
  *val = bit_reader_get_bits_uint##bits##_unchecked (reader, nbits); \
  return true; \
} \
\
static inline bool \
bit_reader_peek_bits_uint##bits##_inline (const BitReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  RETURN_VAL_IF_FAIL (val != NULL, false); \
  RETURN_VAL_IF_FAIL (nbits <= bits, false); \
  \
  if (bit_reader_get_remaining_unchecked (reader) < nbits) \
    return false; \
\
  *val = bit_reader_peek_bits_uint##bits##_unchecked (reader, nbits); \
  return true; \
}

BIT_READER_READ_BITS_INLINE (8)
BIT_READER_READ_BITS_INLINE (16)
BIT_READER_READ_BITS_INLINE (32)
BIT_READER_READ_BITS_INLINE (64)

#undef BIT_READER_READ_BITS_INLINE

#ifndef BIT_READER_DISABLE_INLINES

#define bit_reader_get_size(reader) \
    bit_reader_get_size_inline (reader)
#define bit_reader_get_pos(reader) \
    bit_reader_get_pos_inline (reader)
#define bit_reader_get_remaining(reader) \
    bit_reader_get_remaining_inline (reader)

#define bit_reader_skip(reader, nbits)\
    bit_reader_skip_inline(reader, nbits)
#define bit_reader_skip_to_byte(reader)\
    bit_reader_skip_to_byte_inline(reader)

#define bit_reader_get_bits_uint8(reader, val, nbits) \
    bit_reader_get_bits_uint8_inline (reader, val, nbits)
#define bit_reader_get_bits_uint16(reader, val, nbits) \
    bit_reader_get_bits_uint16_inline (reader, val, nbits)
#define bit_reader_get_bits_uint32(reader, val, nbits) \
    bit_reader_get_bits_uint32_inline (reader, val, nbits)
#define bit_reader_get_bits_uint64(reader, val, nbits) \
    bit_reader_get_bits_uint64_inline (reader, val, nbits)

#define bit_reader_peek_bits_uint8(reader, val, nbits) \
    bit_reader_peek_bits_uint8_inline (reader, val, nbits)
#define bit_reader_peek_bits_uint16(reader, val, nbits) \
    bit_reader_peek_bits_uint16_inline (reader, val, nbits)
#define bit_reader_peek_bits_uint32(reader, val, nbits) \
    bit_reader_peek_bits_uint32_inline (reader, val, nbits)
#define bit_reader_peek_bits_uint64(reader, val, nbits) \
    bit_reader_peek_bits_uint64_inline (reader, val, nbits)
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BIT_READER_H__ */
