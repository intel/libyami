/* GStreamer byte reader
 *
 * Copyright (C) 2008 Sebastian Dröge <sebastian.droege@collabora.co.uk>.
 * Copyright (C) 2009 Tim-Philipp Müller <tim centricular net>
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

#ifndef __BYTE_READER_H__
#define __BYTE_READER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include "log.h"

#define BYTE_READER(reader) ((ByteReader *) (reader))

/**
 * ByteReader:
 * @data: Data from which the bit reader will read
 * @size: Size of @data in bytes
 * @byte: Current byte position
 *
 * A byte reader instance.
 */
typedef struct {
  const uint8_t *data;
  uint32_t size;

  uint32_t byte;  /* Byte position */
} ByteReader;

ByteReader * byte_reader_new (const uint8_t *data, uint32_t size);
void byte_reader_init (ByteReader *reader, const uint8_t *data, uint32_t size);
void byte_reader_free (ByteReader *reader);

bool byte_reader_set_pos (ByteReader *reader, uint32_t pos);
uint32_t byte_reader_get_pos (const ByteReader *reader);
uint32_t byte_reader_get_remaining (const ByteReader *reader);
uint32_t byte_reader_get_size (const ByteReader *reader);
bool byte_reader_skip (ByteReader *reader, uint32_t nbytes);

bool byte_reader_get_uint8 (ByteReader *reader, uint8_t *val);
bool byte_reader_get_int8 (ByteReader *reader, int8_t *val);
bool byte_reader_get_uint16_le (ByteReader *reader, uint16_t *val);
bool byte_reader_get_int16_le (ByteReader *reader, int16_t *val);
bool byte_reader_get_uint16_be (ByteReader *reader, uint16_t *val);
bool byte_reader_get_int16_be (ByteReader *reader, int16_t *val);
bool byte_reader_get_uint24_le (ByteReader *reader, uint32_t *val);
bool byte_reader_get_int24_le (ByteReader *reader, int32_t *val);
bool byte_reader_get_uint24_be (ByteReader *reader, uint32_t *val);
bool byte_reader_get_int24_be (ByteReader *reader, int32_t *val);
bool byte_reader_get_uint32_le (ByteReader *reader, uint32_t *val);
bool byte_reader_get_int32_le (ByteReader *reader, int32_t *val);
bool byte_reader_get_uint32_be (ByteReader *reader, uint32_t *val);
bool byte_reader_get_int32_be (ByteReader *reader, int32_t *val);
bool byte_reader_get_uint64_le (ByteReader *reader, uint64_t *val);
bool byte_reader_get_int64_le (ByteReader *reader, int64_t *val);
bool byte_reader_get_uint64_be (ByteReader *reader, uint64_t *val);
bool byte_reader_get_int64_be (ByteReader *reader, int64_t *val);

bool byte_reader_peek_uint8 (const ByteReader *reader, uint8_t *val);
bool byte_reader_peek_int8 (const ByteReader *reader, int8_t *val);
bool byte_reader_peek_uint16_le (const ByteReader *reader, uint16_t *val);
bool byte_reader_peek_int16_le (const ByteReader *reader, int16_t *val);
bool byte_reader_peek_uint16_be (const ByteReader *reader, uint16_t *val);
bool byte_reader_peek_int16_be (const ByteReader *reader, int16_t *val);
bool byte_reader_peek_uint24_le (const ByteReader *reader, uint32_t *val);
bool byte_reader_peek_int24_le (const ByteReader *reader, int32_t *val);
bool byte_reader_peek_uint24_be (const ByteReader *reader, uint32_t *val);
bool byte_reader_peek_int24_be (const ByteReader *reader, int32_t *val);
bool byte_reader_peek_uint32_le (const ByteReader *reader, uint32_t *val);
bool byte_reader_peek_int32_le (const ByteReader *reader, int32_t *val);
bool byte_reader_peek_uint32_be (const ByteReader *reader, uint32_t *val);
bool byte_reader_peek_int32_be (const ByteReader *reader, int32_t *val);
bool byte_reader_peek_uint64_le (const ByteReader *reader, uint64_t *val);
bool byte_reader_peek_int64_le (const ByteReader *reader, int64_t *val);
bool byte_reader_peek_uint64_be (const ByteReader *reader, uint64_t *val);
bool byte_reader_peek_int64_be (const ByteReader *reader, int64_t *val);

bool byte_reader_get_float32_le (ByteReader *reader, float *val);
bool byte_reader_get_float32_be (ByteReader *reader, float *val);
bool byte_reader_get_float64_le (ByteReader *reader, double *val);
bool byte_reader_get_float64_be (ByteReader *reader, double *val);

bool byte_reader_peek_float32_le (const ByteReader *reader, float *val);
bool byte_reader_peek_float32_be (const ByteReader *reader, float *val);
bool byte_reader_peek_float64_le (const ByteReader *reader, double *val);
bool byte_reader_peek_float64_be (const ByteReader *reader, double *val);

bool byte_reader_get_data  (ByteReader * reader, uint32_t size, const uint8_t ** val);
bool byte_reader_peek_data (const ByteReader * reader, uint32_t size, const uint8_t ** val);

#define byte_reader_skip_string(reader) \
    byte_reader_skip_string_utf8(reader)

bool byte_reader_skip_string_utf8  (ByteReader * reader);
bool byte_reader_skip_string_utf16 (ByteReader * reader);
bool byte_reader_skip_string_utf32 (ByteReader * reader);

#define byte_reader_get_string(reader,str) \
    byte_reader_get_string_utf8(reader,str)

#define byte_reader_peek_string(reader,str) \
    byte_reader_peek_string_utf8(reader,str)

bool byte_reader_get_string_utf8   (ByteReader * reader, const char ** str);
bool byte_reader_peek_string_utf8  (const ByteReader * reader, const char ** str);

uint32_t  byte_reader_masked_scan_uint32 (const ByteReader * reader,
                                             uint32_t  mask,
                                             uint32_t  pattern,
                                             uint32_t  offset,
                                             uint32_t  size);

/**
 * BYTE_READER_INIT:
 * @data: Data from which the #ByteReader should read
 * @size: Size of @data in bytes
 *
 * A #ByteReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * byte_reader_init().
 *
 * Since: 0.10.22
 */
#define BYTE_READER_INIT(data, size) {data, size, 0}

/* unchecked variants */
static inline void
byte_reader_skip_unchecked (ByteReader * reader, uint32_t nbytes)
{
  reader->byte += nbytes;
}

/* read byte from memory address and store in big/little endian */
#define READ_DATA(__data, __idx, __size, __shift) \
    (((uint##__size##_t) (((const uint8_t *) (__data))[__idx])) << (__shift))

#define READ_UINT8_BE(data)	(READ_DATA (data, 0,  8,  0))
#define READ_UINT8_LE(data)	(READ_DATA (data, 0,  8,  0))

#define READ_UINT16_BE(data)	(READ_DATA (data, 0, 16,  8) | \
				 READ_DATA (data, 1, 16,  0))

#define READ_UINT16_LE(data)	(READ_DATA (data, 1, 16,  8) | \
				 READ_DATA (data, 0, 16,  0))

#define READ_UINT24_BE(data)	(READ_DATA (data, 0, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 2, 32,  0))

#define READ_UINT24_LE(data)	(READ_DATA (data, 2, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 0, 32,  0))

#define READ_UINT32_BE(data)	(READ_DATA (data, 0, 32, 24) | \
				 READ_DATA (data, 1, 32, 16) | \
				 READ_DATA (data, 2, 32,  8) | \
				 READ_DATA (data, 3, 32,  0))

#define READ_UINT32_LE(data)	(READ_DATA (data, 3, 32, 24) | \
				 READ_DATA (data, 2, 32, 16) | \
				 READ_DATA (data, 1, 32,  8) | \
				 READ_DATA (data, 0, 32,  0))

#define READ_UINT64_BE(data)    (READ_DATA (data, 0, 64, 56) | \
				 READ_DATA (data, 1, 64, 48) | \
				 READ_DATA (data, 2, 64, 40) | \
				 READ_DATA (data, 3, 64, 32) | \
				 READ_DATA (data, 4, 64, 24) | \
				 READ_DATA (data, 5, 64, 16) | \
				 READ_DATA (data, 6, 64,  8) | \
				 READ_DATA (data, 7, 64,  0))

#define READ_UINT64_LE(data)    (READ_DATA (data, 7, 64, 56) | \
				 READ_DATA (data, 6, 64, 48) | \
				 READ_DATA (data, 5, 64, 40) | \
				 READ_DATA (data, 4, 64, 32) | \
				 READ_DATA (data, 3, 64, 24) | \
				 READ_DATA (data, 2, 64, 16) | \
				 READ_DATA (data, 1, 64,  8) | \
				 READ_DATA (data, 0, 64,  0))

static inline float read_float_be(const uint8_t *data)
{
  union
  {
    uint32_t i;
    float f;
  } u;

  u.i = READ_UINT32_BE (data);
  return u.f;
}

static inline float read_float_le(const uint8_t *data)
{
  union
  {
    uint32_t i;
    float f;
  } u;

  u.i = READ_UINT32_LE (data);
  return u.f;
}

static inline double read_double_be(const uint8_t *data)
{
  union
  {
    uint64_t i;
    double d;
  } u;

  u.i = READ_UINT64_BE (data);
  return u.d;
}

static inline double read_double_le(const uint8_t *data)
{
  union
  {
    uint64_t i;
    double d;
  } u;

  u.i = READ_UINT64_LE (data);
  return u.d;
}

#define READ_FLOAT_BE(data)     read_float_be(data)
#define READ_FLOAT_LE(data)     read_float_le(data)
#define READ_DOUBLE_BE(data)    read_double_be(data) 
#define READ_DOUBLE_LE(data)    read_double_le(data) 

#define BYTE_READER_GET_PEEK_BITS_UNCHECKED(bits,type,lower,upper,adj) \
\
static inline type \
byte_reader_peek_##lower##_unchecked (const ByteReader * reader) \
{ \
  type val = READ_##upper(reader->data + reader->byte); \
  return val; \
} \
\
static inline type \
byte_reader_get_##lower##_unchecked (ByteReader * reader) \
{ \
  type val = byte_reader_peek_##lower##_unchecked (reader); \
  reader->byte += bits / 8; \
  return val; \
}

BYTE_READER_GET_PEEK_BITS_UNCHECKED(8,uint8_t,uint8,UINT8_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(8,int8_t,int8,UINT8_LE,/* */)

BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,uint16_t,uint16_le,UINT16_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,uint16_t,uint16_be,UINT16_BE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,int16_t,int16_le,UINT16_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,int16_t,int16_be,UINT16_BE,/* */)

BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,uint32_t,uint32_le,UINT32_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,uint32_t,uint32_be,UINT32_BE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,int32_t,int32_le,UINT32_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,int32_t,int32_be,UINT32_BE,/* */)

BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,uint32_t,uint24_le,UINT24_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,uint32_t,uint24_be,UINT24_BE,/* */)

/* fix up the sign for 24-bit signed ints stored in 32-bit signed ints */
BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,int32_t,int24_le,UINT24_LE,
    if (val & 0x00800000) val |= 0xff000000;)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,int32_t,int24_be,UINT24_BE,
    if (val & 0x00800000) val |= 0xff000000;)

BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,uint64_t,uint64_le,UINT64_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,uint64_t,uint64_be,UINT64_BE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,int64_t,int64_le,UINT64_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,int64_t,int64_be,UINT64_BE,/* */)

BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,float,float32_le,FLOAT_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,float,float32_be,FLOAT_BE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,double,float64_le,DOUBLE_LE,/* */)
BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,double,float64_be,DOUBLE_BE,/* */)

#undef GET_PEEK_BITS_UNCHECKED

static inline const uint8_t *
byte_reader_peek_data_unchecked (const ByteReader * reader)
{
  return (const uint8_t *) (reader->data + reader->byte);
}

static inline const uint8_t *
byte_reader_get_data_unchecked (ByteReader * reader, uint32_t size)
{
  const uint8_t *data;

  data = byte_reader_peek_data_unchecked (reader);
  byte_reader_skip_unchecked (reader, size);
  return data;
}

/* Unchecked variants that should not be used */
static inline uint32_t
byte_reader_get_pos_unchecked (const ByteReader * reader)
{
  return reader->byte;
}

static inline uint32_t
byte_reader_get_remaining_unchecked (const ByteReader * reader)
{
  return reader->size - reader->byte;
}

static inline uint32_t
byte_reader_get_size_unchecked (const ByteReader * reader)
{
  return reader->size;
}

/* inlined variants (do not use directly) */

static inline uint32_t
byte_reader_get_remaining_inline (const ByteReader * reader)
{
  RETURN_VAL_IF_FAIL (reader != NULL, 0);

  return byte_reader_get_remaining_unchecked (reader);
}

static inline uint32_t
byte_reader_get_size_inline (const ByteReader * reader)
{
  RETURN_VAL_IF_FAIL (reader != NULL, 0);

  return byte_reader_get_size_unchecked (reader);
}

#define BYTE_READER_GET_PEEK_BITS_INLINE(bits,type,name) \
\
static inline bool \
byte_reader_peek_##name##_inline (const ByteReader * reader, type * val) \
{ \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  RETURN_VAL_IF_FAIL (val != NULL, false); \
  \
  if (byte_reader_get_remaining_unchecked (reader) < (bits / 8)) \
    return false; \
\
  *val = byte_reader_peek_##name##_unchecked (reader); \
  return true; \
} \
\
static inline bool \
byte_reader_get_##name##_inline (ByteReader * reader, type * val) \
{ \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  RETURN_VAL_IF_FAIL (val != NULL, false); \
  \
  if (byte_reader_get_remaining_unchecked (reader) < (bits / 8)) \
    return false; \
\
  *val = byte_reader_get_##name##_unchecked (reader); \
  return true; \
}

BYTE_READER_GET_PEEK_BITS_INLINE(8,uint8_t,uint8)
BYTE_READER_GET_PEEK_BITS_INLINE(8,int8_t,int8)

BYTE_READER_GET_PEEK_BITS_INLINE(16,uint16_t,uint16_le)
BYTE_READER_GET_PEEK_BITS_INLINE(16,uint16_t,uint16_be)
BYTE_READER_GET_PEEK_BITS_INLINE(16,int16_t,int16_le)
BYTE_READER_GET_PEEK_BITS_INLINE(16,int16_t,int16_be)

BYTE_READER_GET_PEEK_BITS_INLINE(32,uint32_t,uint32_le)
BYTE_READER_GET_PEEK_BITS_INLINE(32,uint32_t,uint32_be)
BYTE_READER_GET_PEEK_BITS_INLINE(32,int32_t,int32_le)
BYTE_READER_GET_PEEK_BITS_INLINE(32,int32_t,int32_be)

BYTE_READER_GET_PEEK_BITS_INLINE(24,uint32_t,uint24_le)
BYTE_READER_GET_PEEK_BITS_INLINE(24,uint32_t,uint24_be)
BYTE_READER_GET_PEEK_BITS_INLINE(24,int32_t,int24_le)
BYTE_READER_GET_PEEK_BITS_INLINE(24,int32_t,int24_be)

BYTE_READER_GET_PEEK_BITS_INLINE(64,uint64_t,uint64_le)
BYTE_READER_GET_PEEK_BITS_INLINE(64,uint64_t,uint64_be)
BYTE_READER_GET_PEEK_BITS_INLINE(64,int64_t,int64_le)
BYTE_READER_GET_PEEK_BITS_INLINE(64,int64_t,int64_be)

BYTE_READER_GET_PEEK_BITS_INLINE(32,float,float32_le)
BYTE_READER_GET_PEEK_BITS_INLINE(32,float,float32_be)
BYTE_READER_GET_PEEK_BITS_INLINE(64,double,float64_le)
BYTE_READER_GET_PEEK_BITS_INLINE(64,double,float64_be)

#undef BYTE_READER_GET_PEEK_BITS_INLINE

#ifndef BYTE_READER_DISABLE_INLINES

#define byte_reader_get_remaining(reader) \
    byte_reader_get_remaining_inline(reader)

#define byte_reader_get_size(reader) \
    byte_reader_get_size_inline(reader)

#define byte_reader_get_pos(reader) \
    byte_reader_get_pos_inline(reader)

#define byte_reader_get_uint8(reader,val) \
    byte_reader_get_uint8_inline(reader,val)
#define byte_reader_get_int8(reader,val) \
    byte_reader_get_int8_inline(reader,val)
#define byte_reader_get_uint16_le(reader,val) \
    byte_reader_get_uint16_le_inline(reader,val)
#define byte_reader_get_int16_le(reader,val) \
    byte_reader_get_int16_le_inline(reader,val)
#define byte_reader_get_uint16_be(reader,val) \
    byte_reader_get_uint16_be_inline(reader,val)
#define byte_reader_get_int16_be(reader,val) \
    byte_reader_get_int16_be_inline(reader,val)
#define byte_reader_get_uint24_le(reader,val) \
    byte_reader_get_uint24_le_inline(reader,val)
#define byte_reader_get_int24_le(reader,val) \
    byte_reader_get_int24_le_inline(reader,val)
#define byte_reader_get_uint24_be(reader,val) \
    byte_reader_get_uint24_be_inline(reader,val)
#define byte_reader_get_int24_be(reader,val) \
    byte_reader_get_int24_be_inline(reader,val)
#define byte_reader_get_uint32_le(reader,val) \
    byte_reader_get_uint32_le_inline(reader,val)
#define byte_reader_get_int32_le(reader,val) \
    byte_reader_get_int32_le_inline(reader,val)
#define byte_reader_get_uint32_be(reader,val) \
    byte_reader_get_uint32_be_inline(reader,val)
#define byte_reader_get_int32_be(reader,val) \
    byte_reader_get_int32_be_inline(reader,val)
#define byte_reader_get_uint64_le(reader,val) \
    byte_reader_get_uint64_le_inline(reader,val)
#define byte_reader_get_int64_le(reader,val) \
    byte_reader_get_int64_le_inline(reader,val)
#define byte_reader_get_uint64_be(reader,val) \
    byte_reader_get_uint64_be_inline(reader,val)
#define byte_reader_get_int64_be(reader,val) \
    byte_reader_get_int64_be_inline(reader,val)

#define byte_reader_peek_uint8(reader,val) \
    byte_reader_peek_uint8_inline(reader,val)
#define byte_reader_peek_int8(reader,val) \
    byte_reader_peek_int8_inline(reader,val)
#define byte_reader_peek_uint16_le(reader,val) \
    byte_reader_peek_uint16_le_inline(reader,val)
#define byte_reader_peek_int16_le(reader,val) \
    byte_reader_peek_int16_le_inline(reader,val)
#define byte_reader_peek_uint16_be(reader,val) \
    byte_reader_peek_uint16_be_inline(reader,val)
#define byte_reader_peek_int16_be(reader,val) \
    byte_reader_peek_int16_be_inline(reader,val)
#define byte_reader_peek_uint24_le(reader,val) \
    byte_reader_peek_uint24_le_inline(reader,val)
#define byte_reader_peek_int24_le(reader,val) \
    byte_reader_peek_int24_le_inline(reader,val)
#define byte_reader_peek_uint24_be(reader,val) \
    byte_reader_peek_uint24_be_inline(reader,val)
#define byte_reader_peek_int24_be(reader,val) \
    byte_reader_peek_int24_be_inline(reader,val)
#define byte_reader_peek_uint32_le(reader,val) \
    byte_reader_peek_uint32_le_inline(reader,val)
#define byte_reader_peek_int32_le(reader,val) \
    byte_reader_peek_int32_le_inline(reader,val)
#define byte_reader_peek_uint32_be(reader,val) \
    byte_reader_peek_uint32_be_inline(reader,val)
#define byte_reader_peek_int32_be(reader,val) \
    byte_reader_peek_int32_be_inline(reader,val)
#define byte_reader_peek_uint64_le(reader,val) \
    byte_reader_peek_uint64_le_inline(reader,val)
#define byte_reader_peek_int64_le(reader,val) \
    byte_reader_peek_int64_le_inline(reader,val)
#define byte_reader_peek_uint64_be(reader,val) \
    byte_reader_peek_uint64_be_inline(reader,val)
#define byte_reader_peek_int64_be(reader,val) \
    byte_reader_peek_int64_be_inline(reader,val)

#define byte_reader_get_float32_le(reader,val) \
    byte_reader_get_float32_le_inline(reader,val)
#define byte_reader_get_float32_be(reader,val) \
    byte_reader_get_float32_be_inline(reader,val)
#define byte_reader_get_float64_le(reader,val) \
    byte_reader_get_float64_le_inline(reader,val)
#define byte_reader_get_float64_be(reader,val) \
    byte_reader_get_float64_be_inline(reader,val)
#define byte_reader_peek_float32_le(reader,val) \
    byte_reader_peek_float32_le_inline(reader,val)
#define byte_reader_peek_float32_be(reader,val) \
    byte_reader_peek_float32_be_inline(reader,val)
#define byte_reader_peek_float64_le(reader,val) \
    byte_reader_peek_float64_le_inline(reader,val)
#define byte_reader_peek_float64_be(reader,val) \
    byte_reader_peek_float64_be_inline(reader,val)

#endif /* BYTE_READER_DISABLE_INLINES */

static inline bool
byte_reader_get_data_inline (ByteReader * reader, uint32_t size, const uint8_t ** val)
{
  RETURN_VAL_IF_FAIL (reader != NULL && val != NULL, false);

  if (size > reader->size || byte_reader_get_remaining_inline (reader) < size)
    return false;

  *val = byte_reader_get_data_unchecked (reader, size);
  return true;
}

static inline bool
byte_reader_peek_data_inline (const ByteReader * reader, uint32_t size, const uint8_t ** val)
{
  RETURN_VAL_IF_FAIL (reader != NULL && val != NULL, false);

  if (size > reader->size || byte_reader_get_remaining_inline (reader) < size)
    return false;

  *val = byte_reader_peek_data_unchecked (reader);
  return true;
}

static inline uint32_t
byte_reader_get_pos_inline (const ByteReader * reader)
{
  RETURN_VAL_IF_FAIL (reader != NULL, 0);

  return byte_reader_get_pos_unchecked (reader);
}

static inline bool
byte_reader_skip_inline (ByteReader * reader, uint32_t nbytes)
{
  RETURN_VAL_IF_FAIL (reader != NULL, false);

  if (byte_reader_get_remaining_unchecked (reader) < nbytes)
    return false;

  reader->byte += nbytes;
  return true;
}

#ifndef BYTE_READER_DISABLE_INLINES

#define byte_reader_get_data(reader,size,val) \
    byte_reader_get_data_inline(reader,size,val)
#define byte_reader_peek_data(reader,size,val) \
    byte_reader_peek_data_inline(reader,size,val)
#define byte_reader_skip(reader,nbytes) \
    byte_reader_skip_inline(reader,nbytes)

#endif /* BYTE_READER_DISABLE_INLINES */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BYTE_READER_H__ */
