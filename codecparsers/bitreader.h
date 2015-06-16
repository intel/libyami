/* reamer
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __BIT_READER_H__
#define __BIT_READER_H__

#include "gst/gst.h"

/* FIXME: inline functions */

G_BEGIN_DECLS

#define BIT_READER(reader) ((BitReader *) (reader))

/**
 * BitReader:
 * @data: (array length=size): Data from which the bit reader will
 *   read
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

  /* < private > */
  void* _reserved[PADDING];
} BitReader;

BitReader *  bit_reader_new              (const uint8_t *data, uint32_t size) G_GNUC_MALLOC;
void            bit_reader_free             (BitReader *reader);

void            bit_reader_init             (BitReader *reader, const uint8_t *data, uint32_t size);

bool        bit_reader_set_pos          (BitReader *reader, uint32_t pos);
uint32_t           bit_reader_get_pos          (const BitReader *reader);

uint32_t           bit_reader_get_remaining    (const BitReader *reader);

uint32_t           bit_reader_get_size         (const BitReader *reader);

bool        bit_reader_skip             (BitReader *reader, uint32_t nbits);
bool        bit_reader_skip_to_byte     (BitReader *reader);

bool        bit_reader_get_bits_uint8   (BitReader *reader, uint8_t *val, uint32_t nbits);
bool        bit_reader_get_bits_uint16  (BitReader *reader, uint16_t *val, uint32_t nbits);
bool        bit_reader_get_bits_uint32  (BitReader *reader, uint32_t *val, uint32_t nbits);
bool        bit_reader_get_bits_uint64  (BitReader *reader, uint64_t *val, uint32_t nbits);

bool        bit_reader_peek_bits_uint8  (const BitReader *reader, uint8_t *val, uint32_t nbits);
bool        bit_reader_peek_bits_uint16 (const BitReader *reader, uint16_t *val, uint32_t nbits);
bool        bit_reader_peek_bits_uint32 (const BitReader *reader, uint32_t *val, uint32_t nbits);
bool        bit_reader_peek_bits_uint64 (const BitReader *reader, uint64_t *val, uint32_t nbits);

/**
 * BIT_READER_INIT:
 * @data: Data from which the #BitReader should read
 * @size: Size of @data in bytes
 *
 * A #BitReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * bit_reader_init().
 */
#define BIT_READER_INIT(data, size) {data, size, 0, 0}

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

#define __BIT_READER_READ_BITS_UNCHECKED(bits) \
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

__BIT_READER_READ_BITS_UNCHECKED (8)
__BIT_READER_READ_BITS_UNCHECKED (16)
__BIT_READER_READ_BITS_UNCHECKED (32)
__BIT_READER_READ_BITS_UNCHECKED (64)

#undef __BIT_READER_READ_BITS_UNCHECKED

/* unchecked variants -- do not use */

static inline uint32_t
_bit_reader_get_size_unchecked (const BitReader * reader)
{
  return reader->size * 8;
}

static inline uint32_t
_bit_reader_get_pos_unchecked (const BitReader * reader)
{
  return reader->byte * 8 + reader->bit;
}

static inline uint32_t
_bit_reader_get_remaining_unchecked (const BitReader * reader)
{
  return reader->size * 8 - (reader->byte * 8 + reader->bit);
}

/* inlined variants -- do not use directly */
static inline uint32_t
_bit_reader_get_size_inline (const BitReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _bit_reader_get_size_unchecked (reader);
}

static inline uint32_t
_bit_reader_get_pos_inline (const BitReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _bit_reader_get_pos_unchecked (reader);
}

static inline uint32_t
_bit_reader_get_remaining_inline (const BitReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _bit_reader_get_remaining_unchecked (reader);
}

static inline bool
_bit_reader_skip_inline (BitReader * reader, uint32_t nbits)
{
  g_return_val_if_fail (reader != NULL, FALSE);

  if (_bit_reader_get_remaining_unchecked (reader) < nbits)
    return FALSE;

  bit_reader_skip_unchecked (reader, nbits);

  return TRUE;
}

static inline bool
_bit_reader_skip_to_byte_inline (BitReader * reader)
{
  g_return_val_if_fail (reader != NULL, FALSE);

  if (reader->byte > reader->size)
    return FALSE;

  bit_reader_skip_to_byte_unchecked (reader);

  return TRUE;
}

#define __BIT_READER_READ_BITS_INLINE(bits) \
static inline bool \
_bit_reader_get_bits_uint##bits##_inline (BitReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  g_return_val_if_fail (reader != NULL, FALSE); \
  g_return_val_if_fail (val != NULL, FALSE); \
  g_return_val_if_fail (nbits <= bits, FALSE); \
  \
  if (_bit_reader_get_remaining_unchecked (reader) < nbits) \
    return FALSE; \
\
  *val = bit_reader_get_bits_uint##bits##_unchecked (reader, nbits); \
  return TRUE; \
} \
\
static inline bool \
_bit_reader_peek_bits_uint##bits##_inline (const BitReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  g_return_val_if_fail (reader != NULL, FALSE); \
  g_return_val_if_fail (val != NULL, FALSE); \
  g_return_val_if_fail (nbits <= bits, FALSE); \
  \
  if (_bit_reader_get_remaining_unchecked (reader) < nbits) \
    return FALSE; \
\
  *val = bit_reader_peek_bits_uint##bits##_unchecked (reader, nbits); \
  return TRUE; \
}

__BIT_READER_READ_BITS_INLINE (8)
__BIT_READER_READ_BITS_INLINE (16)
__BIT_READER_READ_BITS_INLINE (32)
__BIT_READER_READ_BITS_INLINE (64)

#undef __BIT_READER_READ_BITS_INLINE

#ifndef BIT_READER_DISABLE_INLINES

#define bit_reader_get_size(reader) \
    _bit_reader_get_size_inline (reader)
#define bit_reader_get_pos(reader) \
    _bit_reader_get_pos_inline (reader)
#define bit_reader_get_remaining(reader) \
    _bit_reader_get_remaining_inline (reader)

/* we use defines here so we can add the G_LIKELY() */

#define bit_reader_skip(reader, nbits)\
    G_LIKELY (_bit_reader_skip_inline(reader, nbits))
#define bit_reader_skip_to_byte(reader)\
    G_LIKELY (_bit_reader_skip_to_byte_inline(reader))

#define bit_reader_get_bits_uint8(reader, val, nbits) \
    G_LIKELY (_bit_reader_get_bits_uint8_inline (reader, val, nbits))
#define bit_reader_get_bits_uint16(reader, val, nbits) \
    G_LIKELY (_bit_reader_get_bits_uint16_inline (reader, val, nbits))
#define bit_reader_get_bits_uint32(reader, val, nbits) \
    G_LIKELY (_bit_reader_get_bits_uint32_inline (reader, val, nbits))
#define bit_reader_get_bits_uint64(reader, val, nbits) \
    G_LIKELY (_bit_reader_get_bits_uint64_inline (reader, val, nbits))

#define bit_reader_peek_bits_uint8(reader, val, nbits) \
    G_LIKELY (_bit_reader_peek_bits_uint8_inline (reader, val, nbits))
#define bit_reader_peek_bits_uint16(reader, val, nbits) \
    G_LIKELY (_bit_reader_peek_bits_uint16_inline (reader, val, nbits))
#define bit_reader_peek_bits_uint32(reader, val, nbits) \
    G_LIKELY (_bit_reader_peek_bits_uint32_inline (reader, val, nbits))
#define bit_reader_peek_bits_uint64(reader, val, nbits) \
    G_LIKELY (_bit_reader_peek_bits_uint64_inline (reader, val, nbits))
#endif

G_END_DECLS

#endif /* __BIT_READER_H__ */
