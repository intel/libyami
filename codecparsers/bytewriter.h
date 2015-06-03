/* reamer byte writer
 *
 * Copyright (C) 2009 Sebastian Dröge <sebastian.droege@collabora.co.uk>.
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

#ifndef __BYTE_WRITER_H__
#define __BYTE_WRITER_H__

#include "commondef.h"
#include "bytereader.h"

#include <string.h>

G_BEGIN_DECLS

#define BYTE_WRITER(writer) ((ByteWriter *) (writer))

/**
 * ByteWriter:
 * @parent: #ByteReader parent
 * @alloc_size: Allocation size of the data
 * @fixed: If %TRUE no reallocations are allowed
 * @owned: If %FALSE no reallocations are allowed and copies of data are returned
 *
 * A byte writer instance.
 */
typedef struct {
  ByteReader parent;

  uint32_t alloc_size;

  bool fixed;
  bool owned;

  /* < private > */
  void* _reserved[PADDING];
} ByteWriter;

ByteWriter * byte_writer_new             (void) G_GNUC_MALLOC;
ByteWriter * byte_writer_new_with_size   (uint32_t size, bool fixed) G_GNUC_MALLOC;
ByteWriter * byte_writer_new_with_data   (uint8_t *data, uint32_t size, bool initialized) G_GNUC_MALLOC;

void            byte_writer_init            (ByteWriter *writer);
void            byte_writer_init_with_size  (ByteWriter *writer, uint32_t size, bool fixed);
void            byte_writer_init_with_data  (ByteWriter *writer, uint8_t *data,
                                                 uint32_t size, bool initialized);

void            byte_writer_free                    (ByteWriter *writer);
uint8_t *        byte_writer_free_and_get_data       (ByteWriter *writer);
Buffer *     byte_writer_free_and_get_buffer     (ByteWriter *writer) G_GNUC_MALLOC;

void            byte_writer_reset                   (ByteWriter *writer);
uint8_t *        byte_writer_reset_and_get_data      (ByteWriter *writer);
Buffer *     byte_writer_reset_and_get_buffer    (ByteWriter *writer) G_GNUC_MALLOC;

/**
 * byte_writer_get_pos:
 * @writer: #ByteWriter instance
 *
 * Returns: The current position of the read/write cursor
 */
/**
 * byte_writer_set_pos:
 * @writer: #ByteWriter instance
 * @pos: new position
 *
 * Sets the current read/write cursor of @writer. The new position
 * can only be between 0 and the current size.
 *
 * Returns: %TRUE if the new position could be set
 */
/**
 * byte_writer_get_size:
 * @writer: #ByteWriter instance
 *
 * Returns: The current, initialized size of the data
 */
#ifdef _FOOL_GTK_DOC_
G_INLINE_FUNC uint32_t     byte_writer_get_pos  (const ByteWriter *writer);
G_INLINE_FUNC bool  byte_writer_set_pos  (ByteWriter *writer, uint32_t pos);
G_INLINE_FUNC uint32_t     byte_writer_get_size (const ByteWriter *writer);
#else
static inline uint32_t
byte_writer_get_pos (const ByteWriter *writer)
{
  return byte_reader_get_pos ((const ByteReader *) writer);
}

static inline bool
byte_writer_set_pos (ByteWriter *writer, uint32_t pos)
{
  return byte_reader_set_pos (BYTE_READER (writer), pos);
}

static inline uint32_t
byte_writer_get_size (const ByteWriter *writer)
{
  return byte_reader_get_size ((const ByteReader *) writer);
}
#endif

uint32_t           byte_writer_get_remaining     (const ByteWriter *writer);
bool        byte_writer_ensure_free_space (ByteWriter *writer, uint32_t size);

bool        byte_writer_put_uint8         (ByteWriter *writer, uint8_t val);
bool        byte_writer_put_int8          (ByteWriter *writer, int8_t val);
bool        byte_writer_put_uint16_be     (ByteWriter *writer, uint16_t val);
bool        byte_writer_put_uint16_le     (ByteWriter *writer, uint16_t val);
bool        byte_writer_put_int16_be      (ByteWriter *writer, int16_t val);
bool        byte_writer_put_int16_le      (ByteWriter *writer, int16_t val);
bool        byte_writer_put_uint24_be     (ByteWriter *writer, uint32_t val);
bool        byte_writer_put_uint24_le     (ByteWriter *writer, uint32_t val);
bool        byte_writer_put_int24_be      (ByteWriter *writer, int32_t val);
bool        byte_writer_put_int24_le      (ByteWriter *writer, int32_t val);
bool        byte_writer_put_uint32_be     (ByteWriter *writer, uint32_t val);
bool        byte_writer_put_uint32_le     (ByteWriter *writer, uint32_t val);
bool        byte_writer_put_int32_be      (ByteWriter *writer, int32_t val);
bool        byte_writer_put_int32_le      (ByteWriter *writer, int32_t val);
bool        byte_writer_put_uint64_be     (ByteWriter *writer, uint64_t val);
bool        byte_writer_put_uint64_le     (ByteWriter *writer, uint64_t val);
bool        byte_writer_put_int64_be      (ByteWriter *writer, int64_t val);
bool        byte_writer_put_int64_le      (ByteWriter *writer, int64_t val);

bool        byte_writer_put_float32_be    (ByteWriter *writer, float val);
bool        byte_writer_put_float32_le    (ByteWriter *writer, float val);
bool        byte_writer_put_float64_be    (ByteWriter *writer, double val);
bool        byte_writer_put_float64_le    (ByteWriter *writer, double val);

bool        byte_writer_put_data          (ByteWriter *writer, const uint8_t *data, uint32_t size);
bool        byte_writer_fill              (ByteWriter *writer, uint8_t value, uint32_t size);
bool        byte_writer_put_string_utf8   (ByteWriter *writer, const char *data);
bool        byte_writer_put_string_utf16  (ByteWriter *writer, const uint16_t *data);
bool        byte_writer_put_string_utf32  (ByteWriter *writer, const uint32_t *data);
bool        byte_writer_put_buffer        (ByteWriter *writer, Buffer * buffer, size_t offset, gssize size);

/**
 * byte_writer_put_string:
 * @writer: #ByteWriter instance
 * @data: (in) (array zero-terminated=1): Null terminated string
 *
 * Write a NUL-terminated string to @writer (including the terminator). The
 * string is assumed to be in an 8-bit encoding (e.g. ASCII,UTF-8 or
 * ISO-8859-1).
 *
 * Returns: %TRUE if the string could be written
 */
#define byte_writer_put_string(writer, data) \
  byte_writer_put_string_utf8(writer, data)

static inline uint32_t
_byte_writer_next_pow2 (uint32_t n)
{
  uint32_t ret = 16;

  /* We start with 16, smaller allocations make no sense */

  while (ret < n && ret > 0)
    ret <<= 1;

  return ret ? ret : n;
}

static inline bool
_byte_writer_ensure_free_space_inline (ByteWriter * writer, uint32_t size)
{
  void* data;

  if (G_LIKELY (size <= writer->alloc_size - writer->parent.byte))
    return TRUE;
  if (G_UNLIKELY (writer->fixed || !writer->owned))
    return FALSE;
  if (G_UNLIKELY (writer->parent.byte > G_MAXUINT - size))
    return FALSE;

  writer->alloc_size = _byte_writer_next_pow2 (writer->parent.byte + size);
  data = g_try_realloc ((uint8_t *) writer->parent.data, writer->alloc_size);
  if (G_UNLIKELY (data == NULL))
    return FALSE;

  writer->parent.data = (uint8_t *) data;

  return TRUE;
}

#define __BYTE_WRITER_CREATE_WRITE_FUNC(bits,type,name,write_func) \
static inline void \
byte_writer_put_##name##_unchecked (ByteWriter *writer, type val) \
{ \
  uint8_t *write_data; \
  \
  write_data = (uint8_t *) writer->parent.data + writer->parent.byte; \
  write_func (write_data, val); \
  writer->parent.byte += bits/8; \
  writer->parent.size = MAX (writer->parent.size, writer->parent.byte); \
} \
\
static inline bool \
_byte_writer_put_##name##_inline (ByteWriter *writer, type val) \
{ \
  g_return_val_if_fail (writer != NULL, FALSE); \
  \
  if (G_UNLIKELY (!_byte_writer_ensure_free_space_inline(writer, bits/8))) \
    return FALSE; \
  \
  byte_writer_put_##name##_unchecked (writer, val); \
  \
  return TRUE; \
}

__BYTE_WRITER_CREATE_WRITE_FUNC (8, uint8_t, uint8, GST_WRITE_UINT8)
__BYTE_WRITER_CREATE_WRITE_FUNC (8, int8_t, int8, GST_WRITE_UINT8)
__BYTE_WRITER_CREATE_WRITE_FUNC (16, uint16_t, uint16_le, GST_WRITE_UINT16_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (16, uint16_t, uint16_be, GST_WRITE_UINT16_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (16, int16_t, int16_le, GST_WRITE_UINT16_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (16, int16_t, int16_be, GST_WRITE_UINT16_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (24, uint32_t, uint24_le, GST_WRITE_UINT24_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (24, uint32_t, uint24_be, GST_WRITE_UINT24_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (24, int32_t, int24_le, GST_WRITE_UINT24_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (24, int32_t, int24_be, GST_WRITE_UINT24_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (32, uint32_t, uint32_le, GST_WRITE_UINT32_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (32, uint32_t, uint32_be, GST_WRITE_UINT32_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (32, int32_t, int32_le, GST_WRITE_UINT32_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (32, int32_t, int32_be, GST_WRITE_UINT32_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, uint64_t, uint64_le, GST_WRITE_UINT64_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, uint64_t, uint64_be, GST_WRITE_UINT64_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, int64_t, int64_le, GST_WRITE_UINT64_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, int64_t, int64_be, GST_WRITE_UINT64_BE)

__BYTE_WRITER_CREATE_WRITE_FUNC (32, float, float32_be, GST_WRITE_FLOAT_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (32, float, float32_le, GST_WRITE_FLOAT_LE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, double, float64_be, GST_WRITE_DOUBLE_BE)
__BYTE_WRITER_CREATE_WRITE_FUNC (64, double, float64_le, GST_WRITE_DOUBLE_LE)

#undef __BYTE_WRITER_CREATE_WRITE_FUNC

static inline void
byte_writer_put_data_unchecked (ByteWriter * writer, const uint8_t * data,
    uint32_t size)
{
  memcpy ((uint8_t *) & writer->parent.data[writer->parent.byte], data, size);
  writer->parent.byte += size;
  writer->parent.size = MAX (writer->parent.size, writer->parent.byte);
}

static inline bool
_byte_writer_put_data_inline (ByteWriter * writer, const uint8_t * data,
    uint32_t size)
{
  g_return_val_if_fail (writer != NULL, FALSE);

  if (G_UNLIKELY (!_byte_writer_ensure_free_space_inline (writer, size)))
    return FALSE;

  byte_writer_put_data_unchecked (writer, data, size);

  return TRUE;
}

static inline void
byte_writer_fill_unchecked (ByteWriter * writer, uint8_t value, uint32_t size)
{
  memset ((uint8_t *) & writer->parent.data[writer->parent.byte], value, size);
  writer->parent.byte += size;
  writer->parent.size = MAX (writer->parent.size, writer->parent.byte);
}

static inline bool
_byte_writer_fill_inline (ByteWriter * writer, uint8_t value, uint32_t size)
{
  g_return_val_if_fail (writer != NULL, FALSE);

  if (G_UNLIKELY (!_byte_writer_ensure_free_space_inline (writer, size)))
    return FALSE;

  byte_writer_fill_unchecked (writer, value, size);

  return TRUE;
}

static inline void
byte_writer_put_buffer_unchecked (ByteWriter * writer, Buffer * buffer,
    size_t offset, gssize size)
{
  if (size == -1) {
    size = buffer_get_size (buffer);

    if (offset >= (size_t) size)
      return;

    size -= offset;
  }

  buffer_extract (buffer, offset,
      (uint8_t *) & writer->parent.data[writer->parent.byte], size);
  writer->parent.byte += size;
  writer->parent.size = MAX (writer->parent.size, writer->parent.byte);
}

static inline bool
_byte_writer_put_buffer_inline (ByteWriter * writer, Buffer * buffer,
    size_t offset, gssize size)
{
  g_return_val_if_fail (writer != NULL, FALSE);
  g_return_val_if_fail (size >= -1, FALSE);

  if (size == -1) {
    size = buffer_get_size (buffer);

    if (offset >= (size_t) size)
      return TRUE;

    size -= offset;
  }

  if (G_UNLIKELY (!_byte_writer_ensure_free_space_inline (writer, size)))
    return FALSE;

  byte_writer_put_buffer_unchecked (writer, buffer, offset, size);

  return TRUE;
}

#ifndef BYTE_WRITER_DISABLE_INLINES

/* we use defines here so we can add the G_LIKELY() */

#define byte_writer_ensure_free_space(writer, size) \
    G_LIKELY (_byte_writer_ensure_free_space_inline (writer, size))
#define byte_writer_put_uint8(writer, val) \
    G_LIKELY (_byte_writer_put_uint8_inline (writer, val))
#define byte_writer_put_int8(writer, val) \
    G_LIKELY (_byte_writer_put_int8_inline (writer, val))
#define byte_writer_put_uint16_be(writer, val) \
    G_LIKELY (_byte_writer_put_uint16_be_inline (writer, val))
#define byte_writer_put_uint16_le(writer, val) \
    G_LIKELY (_byte_writer_put_uint16_le_inline (writer, val))
#define byte_writer_put_int16_be(writer, val) \
    G_LIKELY (_byte_writer_put_int16_be_inline (writer, val))
#define byte_writer_put_int16_le(writer, val) \
    G_LIKELY (_byte_writer_put_int16_le_inline (writer, val))
#define byte_writer_put_uint24_be(writer, val) \
    G_LIKELY (_byte_writer_put_uint24_be_inline (writer, val))
#define byte_writer_put_uint24_le(writer, val) \
    G_LIKELY (_byte_writer_put_uint24_le_inline (writer, val))
#define byte_writer_put_int24_be(writer, val) \
    G_LIKELY (_byte_writer_put_int24_be_inline (writer, val))
#define byte_writer_put_int24_le(writer, val) \
    G_LIKELY (_byte_writer_put_int24_le_inline (writer, val))
#define byte_writer_put_uint32_be(writer, val) \
    G_LIKELY (_byte_writer_put_uint32_be_inline (writer, val))
#define byte_writer_put_uint32_le(writer, val) \
    G_LIKELY (_byte_writer_put_uint32_le_inline (writer, val))
#define byte_writer_put_int32_be(writer, val) \
    G_LIKELY (_byte_writer_put_int32_be_inline (writer, val))
#define byte_writer_put_int32_le(writer, val) \
    G_LIKELY (_byte_writer_put_int32_le_inline (writer, val))
#define byte_writer_put_uint64_be(writer, val) \
    G_LIKELY (_byte_writer_put_uint64_be_inline (writer, val))
#define byte_writer_put_uint64_le(writer, val) \
    G_LIKELY (_byte_writer_put_uint64_le_inline (writer, val))
#define byte_writer_put_int64_be(writer, val) \
    G_LIKELY (_byte_writer_put_int64_be_inline (writer, val))
#define byte_writer_put_int64_le(writer, val) \
    G_LIKELY (_byte_writer_put_int64_le_inline (writer, val))

#define byte_writer_put_float32_be(writer, val) \
    G_LIKELY (_byte_writer_put_float32_be_inline (writer, val))
#define byte_writer_put_float32_le(writer, val) \
    G_LIKELY (_byte_writer_put_float32_le_inline (writer, val))
#define byte_writer_put_float64_be(writer, val) \
    G_LIKELY (_byte_writer_put_float64_be_inline (writer, val))
#define byte_writer_put_float64_le(writer, val) \
    G_LIKELY (_byte_writer_put_float64_le_inline (writer, val))

#define byte_writer_put_data(writer, data, size) \
    G_LIKELY (_byte_writer_put_data_inline (writer, data, size))
#define byte_writer_fill(writer, val, size) \
    G_LIKELY (_byte_writer_fill_inline (writer, val, size))
#define byte_writer_put_buffer(writer, buffer, offset, size) \
    G_LIKELY (_byte_writer_put_buffer_inline (writer, buffer, offset, size))

#endif

G_END_DECLS

#endif /* __BYTE_WRITER_H__ */
