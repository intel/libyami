/* reamer byte reader
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __BYTE_READER_H__
#define __BYTE_READER_H__

#include "gst/gst.h"

G_BEGIN_DECLS

#define BYTE_READER(reader) ((ByteReader *) (reader))

/**
 * ByteReader:
 * @data: (array length=size): Data from which the bit reader will
 *   read
 * @size: Size of @data in bytes
 * @byte: Current byte position
 *
 * A byte reader instance.
 */
typedef struct {
  const uint8_t *data;
  uint32_t size;

  uint32_t byte;  /* Byte position */

  /* < private > */
  void* _reserved[PADDING];
} ByteReader;

ByteReader * byte_reader_new             (const uint8_t *data, uint32_t size) G_GNUC_MALLOC;
void            byte_reader_free            (ByteReader *reader);

void            byte_reader_init            (ByteReader *reader, const uint8_t *data, uint32_t size);

bool        byte_reader_peek_sub_reader (ByteReader * reader,
                                                 ByteReader * sub_reader,
                                                 uint32_t           size);

bool        byte_reader_get_sub_reader  (ByteReader * reader,
                                                 ByteReader * sub_reader,
                                                 uint32_t           size);

bool        byte_reader_set_pos         (ByteReader *reader, uint32_t pos);
uint32_t           byte_reader_get_pos         (const ByteReader *reader);

uint32_t           byte_reader_get_remaining   (const ByteReader *reader);

uint32_t           byte_reader_get_size        (const ByteReader *reader);

bool        byte_reader_skip            (ByteReader *reader, uint32_t nbytes);

bool        byte_reader_get_uint8       (ByteReader *reader, uint8_t *val);
bool        byte_reader_get_int8        (ByteReader *reader, int8_t *val);
bool        byte_reader_get_uint16_le   (ByteReader *reader, uint16_t *val);
bool        byte_reader_get_int16_le    (ByteReader *reader, int16_t *val);
bool        byte_reader_get_uint16_be   (ByteReader *reader, uint16_t *val);
bool        byte_reader_get_int16_be    (ByteReader *reader, int16_t *val);
bool        byte_reader_get_uint24_le   (ByteReader *reader, uint32_t *val);
bool        byte_reader_get_int24_le    (ByteReader *reader, int32_t *val);
bool        byte_reader_get_uint24_be   (ByteReader *reader, uint32_t *val);
bool        byte_reader_get_int24_be    (ByteReader *reader, int32_t *val);
bool        byte_reader_get_uint32_le   (ByteReader *reader, uint32_t *val);
bool        byte_reader_get_int32_le    (ByteReader *reader, int32_t *val);
bool        byte_reader_get_uint32_be   (ByteReader *reader, uint32_t *val);
bool        byte_reader_get_int32_be    (ByteReader *reader, int32_t *val);
bool        byte_reader_get_uint64_le   (ByteReader *reader, uint64_t *val);
bool        byte_reader_get_int64_le    (ByteReader *reader, int64_t *val);
bool        byte_reader_get_uint64_be   (ByteReader *reader, uint64_t *val);
bool        byte_reader_get_int64_be    (ByteReader *reader, int64_t *val);

bool        byte_reader_peek_uint8      (const ByteReader *reader, uint8_t *val);
bool        byte_reader_peek_int8       (const ByteReader *reader, int8_t *val);
bool        byte_reader_peek_uint16_le  (const ByteReader *reader, uint16_t *val);
bool        byte_reader_peek_int16_le   (const ByteReader *reader, int16_t *val);
bool        byte_reader_peek_uint16_be  (const ByteReader *reader, uint16_t *val);
bool        byte_reader_peek_int16_be   (const ByteReader *reader, int16_t *val);
bool        byte_reader_peek_uint24_le  (const ByteReader *reader, uint32_t *val);
bool        byte_reader_peek_int24_le   (const ByteReader *reader, int32_t *val);
bool        byte_reader_peek_uint24_be  (const ByteReader *reader, uint32_t *val);
bool        byte_reader_peek_int24_be   (const ByteReader *reader, int32_t *val);
bool        byte_reader_peek_uint32_le  (const ByteReader *reader, uint32_t *val);
bool        byte_reader_peek_int32_le   (const ByteReader *reader, int32_t *val);
bool        byte_reader_peek_uint32_be  (const ByteReader *reader, uint32_t *val);
bool        byte_reader_peek_int32_be   (const ByteReader *reader, int32_t *val);
bool        byte_reader_peek_uint64_le  (const ByteReader *reader, uint64_t *val);
bool        byte_reader_peek_int64_le   (const ByteReader *reader, int64_t *val);
bool        byte_reader_peek_uint64_be  (const ByteReader *reader, uint64_t *val);
bool        byte_reader_peek_int64_be   (const ByteReader *reader, int64_t *val);

bool        byte_reader_get_float32_le  (ByteReader *reader, float *val);
bool        byte_reader_get_float32_be  (ByteReader *reader, float *val);
bool        byte_reader_get_float64_le  (ByteReader *reader, double *val);
bool        byte_reader_get_float64_be  (ByteReader *reader, double *val);

bool        byte_reader_peek_float32_le (const ByteReader *reader, float *val);
bool        byte_reader_peek_float32_be (const ByteReader *reader, float *val);
bool        byte_reader_peek_float64_le (const ByteReader *reader, double *val);
bool        byte_reader_peek_float64_be (const ByteReader *reader, double *val);

bool        byte_reader_dup_data        (ByteReader * reader, uint32_t size, uint8_t       ** val);
bool        byte_reader_get_data        (ByteReader * reader, uint32_t size, const uint8_t ** val);
bool        byte_reader_peek_data       (const ByteReader * reader, uint32_t size, const uint8_t ** val);

#define byte_reader_dup_string(reader,str) \
    byte_reader_dup_string_utf8(reader,str)

bool        byte_reader_dup_string_utf8  (ByteReader * reader, char   ** str);
bool        byte_reader_dup_string_utf16 (ByteReader * reader, uint16_t ** str);
bool        byte_reader_dup_string_utf32 (ByteReader * reader, uint32_t ** str);

#define byte_reader_skip_string(reader) \
    byte_reader_skip_string_utf8(reader)

bool        byte_reader_skip_string_utf8  (ByteReader * reader);
bool        byte_reader_skip_string_utf16 (ByteReader * reader);
bool        byte_reader_skip_string_utf32 (ByteReader * reader);

#define byte_reader_get_string(reader,str) \
    byte_reader_get_string_utf8(reader,str)

#define byte_reader_peek_string(reader,str) \
    byte_reader_peek_string_utf8(reader,str)

bool        byte_reader_get_string_utf8    (ByteReader * reader, const char ** str);
bool        byte_reader_peek_string_utf8   (const ByteReader * reader, const char ** str);

uint32_t           byte_reader_masked_scan_uint32 (const ByteReader * reader,
                                                    uint32_t               mask,
                                                    uint32_t               pattern,
                                                    uint32_t                 offset,
                                                    uint32_t                 size);
uint32_t           byte_reader_masked_scan_uint32_peek (const ByteReader * reader,
                                                         uint32_t mask,
                                                         uint32_t pattern,
                                                         uint32_t offset,
                                                         uint32_t size,
                                                         uint32_t * value);

/**
 * BYTE_READER_INIT:
 * @data: Data from which the #ByteReader should read
 * @size: Size of @data in bytes
 *
 * A #ByteReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * byte_reader_init().
 */
#define BYTE_READER_INIT(data, size) {data, size, 0}

/* unchecked variants */
static inline void
byte_reader_skip_unchecked (ByteReader * reader, uint32_t nbytes)
{
  reader->byte += nbytes;
}

#define __BYTE_READER_GET_PEEK_BITS_UNCHECKED(bits,type,lower,upper,adj) \
\
static inline type \
byte_reader_peek_##lower##_unchecked (const ByteReader * reader) \
{ \
  type val = (type) GST_READ_##upper (reader->data + reader->byte); \
  adj \
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

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(8,uint8_t,uint8,UINT8,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(8,int8_t,int8,UINT8,/* */)

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,uint16_t,uint16_le,UINT16_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,uint16_t,uint16_be,UINT16_BE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,int16_t,int16_le,UINT16_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(16,int16_t,int16_be,UINT16_BE,/* */)

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,uint32_t,uint32_le,UINT32_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,uint32_t,uint32_be,UINT32_BE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,int32_t,int32_le,UINT32_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,int32_t,int32_be,UINT32_BE,/* */)

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,uint32_t,uint24_le,UINT24_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,uint32_t,uint24_be,UINT24_BE,/* */)

/* fix up the sign for 24-bit signed ints stored in 32-bit signed ints */
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,int32_t,int24_le,UINT24_LE,
    if (val & 0x00800000) val |= 0xff000000;)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(24,int32_t,int24_be,UINT24_BE,
    if (val & 0x00800000) val |= 0xff000000;)

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,uint64_t,uint64_le,UINT64_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,uint64_t,uint64_be,UINT64_BE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,int64_t,int64_le,UINT64_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,int64_t,int64_be,UINT64_BE,/* */)

__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,float,float32_le,FLOAT_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(32,float,float32_be,FLOAT_BE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,double,float64_le,DOUBLE_LE,/* */)
__BYTE_READER_GET_PEEK_BITS_UNCHECKED(64,double,float64_be,DOUBLE_BE,/* */)

#undef __GET_PEEK_BITS_UNCHECKED

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

static inline uint8_t *
byte_reader_dup_data_unchecked (ByteReader * reader, uint32_t size)
{
  const void* data = byte_reader_get_data_unchecked (reader, size);
  return (uint8_t *) g_memdup (data, size);
}

/* Unchecked variants that should not be used */
static inline uint32_t
_byte_reader_get_pos_unchecked (const ByteReader * reader)
{
  return reader->byte;
}

static inline uint32_t
_byte_reader_get_remaining_unchecked (const ByteReader * reader)
{
  return reader->size - reader->byte;
}

static inline uint32_t
_byte_reader_get_size_unchecked (const ByteReader * reader)
{
  return reader->size;
}

/* inlined variants (do not use directly) */

static inline uint32_t
_byte_reader_get_remaining_inline (const ByteReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _byte_reader_get_remaining_unchecked (reader);
}

static inline uint32_t
_byte_reader_get_size_inline (const ByteReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _byte_reader_get_size_unchecked (reader);
}

#define __BYTE_READER_GET_PEEK_BITS_INLINE(bits,type,name) \
\
static inline bool \
_byte_reader_peek_##name##_inline (const ByteReader * reader, type * val) \
{ \
  g_return_val_if_fail (reader != NULL, FALSE); \
  g_return_val_if_fail (val != NULL, FALSE); \
  \
  if (_byte_reader_get_remaining_unchecked (reader) < (bits / 8)) \
    return FALSE; \
\
  *val = byte_reader_peek_##name##_unchecked (reader); \
  return TRUE; \
} \
\
static inline bool \
_byte_reader_get_##name##_inline (ByteReader * reader, type * val) \
{ \
  g_return_val_if_fail (reader != NULL, FALSE); \
  g_return_val_if_fail (val != NULL, FALSE); \
  \
  if (_byte_reader_get_remaining_unchecked (reader) < (bits / 8)) \
    return FALSE; \
\
  *val = byte_reader_get_##name##_unchecked (reader); \
  return TRUE; \
}

__BYTE_READER_GET_PEEK_BITS_INLINE(8,uint8_t,uint8)
__BYTE_READER_GET_PEEK_BITS_INLINE(8,int8_t,int8)

__BYTE_READER_GET_PEEK_BITS_INLINE(16,uint16_t,uint16_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(16,uint16_t,uint16_be)
__BYTE_READER_GET_PEEK_BITS_INLINE(16,int16_t,int16_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(16,int16_t,int16_be)

__BYTE_READER_GET_PEEK_BITS_INLINE(32,uint32_t,uint32_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(32,uint32_t,uint32_be)
__BYTE_READER_GET_PEEK_BITS_INLINE(32,int32_t,int32_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(32,int32_t,int32_be)

__BYTE_READER_GET_PEEK_BITS_INLINE(24,uint32_t,uint24_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(24,uint32_t,uint24_be)
__BYTE_READER_GET_PEEK_BITS_INLINE(24,int32_t,int24_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(24,int32_t,int24_be)

__BYTE_READER_GET_PEEK_BITS_INLINE(64,uint64_t,uint64_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(64,uint64_t,uint64_be)
__BYTE_READER_GET_PEEK_BITS_INLINE(64,int64_t,int64_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(64,int64_t,int64_be)

__BYTE_READER_GET_PEEK_BITS_INLINE(32,float,float32_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(32,float,float32_be)
__BYTE_READER_GET_PEEK_BITS_INLINE(64,double,float64_le)
__BYTE_READER_GET_PEEK_BITS_INLINE(64,double,float64_be)

#undef __BYTE_READER_GET_PEEK_BITS_INLINE

#ifndef BYTE_READER_DISABLE_INLINES

#define byte_reader_init(reader,data,size) \
    _byte_reader_init_inline(reader,data,size)

#define byte_reader_get_remaining(reader) \
    _byte_reader_get_remaining_inline(reader)

#define byte_reader_get_size(reader) \
    _byte_reader_get_size_inline(reader)

#define byte_reader_get_pos(reader) \
    _byte_reader_get_pos_inline(reader)

/* we use defines here so we can add the G_LIKELY() */
#define byte_reader_get_uint8(reader,val) \
    G_LIKELY(_byte_reader_get_uint8_inline(reader,val))
#define byte_reader_get_int8(reader,val) \
    G_LIKELY(_byte_reader_get_int8_inline(reader,val))
#define byte_reader_get_uint16_le(reader,val) \
    G_LIKELY(_byte_reader_get_uint16_le_inline(reader,val))
#define byte_reader_get_int16_le(reader,val) \
    G_LIKELY(_byte_reader_get_int16_le_inline(reader,val))
#define byte_reader_get_uint16_be(reader,val) \
    G_LIKELY(_byte_reader_get_uint16_be_inline(reader,val))
#define byte_reader_get_int16_be(reader,val) \
    G_LIKELY(_byte_reader_get_int16_be_inline(reader,val))
#define byte_reader_get_uint24_le(reader,val) \
    G_LIKELY(_byte_reader_get_uint24_le_inline(reader,val))
#define byte_reader_get_int24_le(reader,val) \
    G_LIKELY(_byte_reader_get_int24_le_inline(reader,val))
#define byte_reader_get_uint24_be(reader,val) \
    G_LIKELY(_byte_reader_get_uint24_be_inline(reader,val))
#define byte_reader_get_int24_be(reader,val) \
    G_LIKELY(_byte_reader_get_int24_be_inline(reader,val))
#define byte_reader_get_uint32_le(reader,val) \
    G_LIKELY(_byte_reader_get_uint32_le_inline(reader,val))
#define byte_reader_get_int32_le(reader,val) \
    G_LIKELY(_byte_reader_get_int32_le_inline(reader,val))
#define byte_reader_get_uint32_be(reader,val) \
    G_LIKELY(_byte_reader_get_uint32_be_inline(reader,val))
#define byte_reader_get_int32_be(reader,val) \
    G_LIKELY(_byte_reader_get_int32_be_inline(reader,val))
#define byte_reader_get_uint64_le(reader,val) \
    G_LIKELY(_byte_reader_get_uint64_le_inline(reader,val))
#define byte_reader_get_int64_le(reader,val) \
    G_LIKELY(_byte_reader_get_int64_le_inline(reader,val))
#define byte_reader_get_uint64_be(reader,val) \
    G_LIKELY(_byte_reader_get_uint64_be_inline(reader,val))
#define byte_reader_get_int64_be(reader,val) \
    G_LIKELY(_byte_reader_get_int64_be_inline(reader,val))

#define byte_reader_peek_uint8(reader,val) \
    G_LIKELY(_byte_reader_peek_uint8_inline(reader,val))
#define byte_reader_peek_int8(reader,val) \
    G_LIKELY(_byte_reader_peek_int8_inline(reader,val))
#define byte_reader_peek_uint16_le(reader,val) \
    G_LIKELY(_byte_reader_peek_uint16_le_inline(reader,val))
#define byte_reader_peek_int16_le(reader,val) \
    G_LIKELY(_byte_reader_peek_int16_le_inline(reader,val))
#define byte_reader_peek_uint16_be(reader,val) \
    G_LIKELY(_byte_reader_peek_uint16_be_inline(reader,val))
#define byte_reader_peek_int16_be(reader,val) \
    G_LIKELY(_byte_reader_peek_int16_be_inline(reader,val))
#define byte_reader_peek_uint24_le(reader,val) \
    G_LIKELY(_byte_reader_peek_uint24_le_inline(reader,val))
#define byte_reader_peek_int24_le(reader,val) \
    G_LIKELY(_byte_reader_peek_int24_le_inline(reader,val))
#define byte_reader_peek_uint24_be(reader,val) \
    G_LIKELY(_byte_reader_peek_uint24_be_inline(reader,val))
#define byte_reader_peek_int24_be(reader,val) \
    G_LIKELY(_byte_reader_peek_int24_be_inline(reader,val))
#define byte_reader_peek_uint32_le(reader,val) \
    G_LIKELY(_byte_reader_peek_uint32_le_inline(reader,val))
#define byte_reader_peek_int32_le(reader,val) \
    G_LIKELY(_byte_reader_peek_int32_le_inline(reader,val))
#define byte_reader_peek_uint32_be(reader,val) \
    G_LIKELY(_byte_reader_peek_uint32_be_inline(reader,val))
#define byte_reader_peek_int32_be(reader,val) \
    G_LIKELY(_byte_reader_peek_int32_be_inline(reader,val))
#define byte_reader_peek_uint64_le(reader,val) \
    G_LIKELY(_byte_reader_peek_uint64_le_inline(reader,val))
#define byte_reader_peek_int64_le(reader,val) \
    G_LIKELY(_byte_reader_peek_int64_le_inline(reader,val))
#define byte_reader_peek_uint64_be(reader,val) \
    G_LIKELY(_byte_reader_peek_uint64_be_inline(reader,val))
#define byte_reader_peek_int64_be(reader,val) \
    G_LIKELY(_byte_reader_peek_int64_be_inline(reader,val))

#define byte_reader_get_float32_le(reader,val) \
    G_LIKELY(_byte_reader_get_float32_le_inline(reader,val))
#define byte_reader_get_float32_be(reader,val) \
    G_LIKELY(_byte_reader_get_float32_be_inline(reader,val))
#define byte_reader_get_float64_le(reader,val) \
    G_LIKELY(_byte_reader_get_float64_le_inline(reader,val))
#define byte_reader_get_float64_be(reader,val) \
    G_LIKELY(_byte_reader_get_float64_be_inline(reader,val))
#define byte_reader_peek_float32_le(reader,val) \
    G_LIKELY(_byte_reader_peek_float32_le_inline(reader,val))
#define byte_reader_peek_float32_be(reader,val) \
    G_LIKELY(_byte_reader_peek_float32_be_inline(reader,val))
#define byte_reader_peek_float64_le(reader,val) \
    G_LIKELY(_byte_reader_peek_float64_le_inline(reader,val))
#define byte_reader_peek_float64_be(reader,val) \
    G_LIKELY(_byte_reader_peek_float64_be_inline(reader,val))

#endif /* BYTE_READER_DISABLE_INLINES */

static inline void
_byte_reader_init_inline (ByteReader * reader, const uint8_t * data, uint32_t size)
{
  g_return_if_fail (reader != NULL);

  reader->data = data;
  reader->size = size;
  reader->byte = 0;
}

static inline bool
_byte_reader_peek_sub_reader_inline (ByteReader * reader,
    ByteReader * sub_reader, uint32_t size)
{
  g_return_val_if_fail (reader != NULL, FALSE);
  g_return_val_if_fail (sub_reader != NULL, FALSE);

  if (_byte_reader_get_remaining_unchecked (reader) < size)
    return FALSE;

  sub_reader->data = reader->data + reader->byte;
  sub_reader->byte = 0;
  sub_reader->size = size;
  return TRUE;
}

static inline bool
_byte_reader_get_sub_reader_inline (ByteReader * reader,
    ByteReader * sub_reader, uint32_t size)
{
  if (!_byte_reader_peek_sub_reader_inline (reader, sub_reader, size))
    return FALSE;
  byte_reader_skip_unchecked (reader, size);
  return TRUE;
}

static inline bool
_byte_reader_dup_data_inline (ByteReader * reader, uint32_t size, uint8_t ** val)
{
  g_return_val_if_fail (reader != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  if (G_UNLIKELY (size > reader->size || _byte_reader_get_remaining_unchecked (reader) < size))
    return FALSE;

  *val = byte_reader_dup_data_unchecked (reader, size);
  return TRUE;
}

static inline bool
_byte_reader_get_data_inline (ByteReader * reader, uint32_t size, const uint8_t ** val)
{
  g_return_val_if_fail (reader != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  if (G_UNLIKELY (size > reader->size || _byte_reader_get_remaining_unchecked (reader) < size))
    return FALSE;

  *val = byte_reader_get_data_unchecked (reader, size);
  return TRUE;
}

static inline bool
_byte_reader_peek_data_inline (const ByteReader * reader, uint32_t size, const uint8_t ** val)
{
  g_return_val_if_fail (reader != NULL, FALSE);
  g_return_val_if_fail (val != NULL, FALSE);

  if (G_UNLIKELY (size > reader->size || _byte_reader_get_remaining_unchecked (reader) < size))
    return FALSE;

  *val = byte_reader_peek_data_unchecked (reader);
  return TRUE;
}

static inline uint32_t
_byte_reader_get_pos_inline (const ByteReader * reader)
{
  g_return_val_if_fail (reader != NULL, 0);

  return _byte_reader_get_pos_unchecked (reader);
}

static inline bool
_byte_reader_skip_inline (ByteReader * reader, uint32_t nbytes)
{
  g_return_val_if_fail (reader != NULL, FALSE);

  if (G_UNLIKELY (_byte_reader_get_remaining_unchecked (reader) < nbytes))
    return FALSE;

  reader->byte += nbytes;
  return TRUE;
}

#ifndef BYTE_READER_DISABLE_INLINES

#define byte_reader_dup_data(reader,size,val) \
    G_LIKELY(_byte_reader_dup_data_inline(reader,size,val))
#define byte_reader_get_data(reader,size,val) \
    G_LIKELY(_byte_reader_get_data_inline(reader,size,val))
#define byte_reader_peek_data(reader,size,val) \
    G_LIKELY(_byte_reader_peek_data_inline(reader,size,val))
#define byte_reader_skip(reader,nbytes) \
    G_LIKELY(_byte_reader_skip_inline(reader,nbytes))

#endif /* BYTE_READER_DISABLE_INLINES */

G_END_DECLS

#endif /* __BYTE_READER_H__ */
