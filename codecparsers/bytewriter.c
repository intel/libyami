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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define BYTE_WRITER_DISABLE_INLINES
#include "bytewriter.h"

/**
 * SECTION:bytewriter
 * @short_description: Writes different integer, string and floating point
 *     types to a memory buffer and allows reading
 *
 * #ByteWriter provides a byte writer and reader that can write/read different
 * integer and floating point types to/from a memory buffer. It provides functions
 * for writing/reading signed/unsigned, little/big endian integers of 8, 16, 24,
 * 32 and 64 bits and functions for reading little/big endian floating points numbers of
 * 32 and 64 bits. It also provides functions to write/read NUL-terminated strings
 * in various character encodings.
 */

/**
 * byte_writer_new:
 *
 * Creates a new, empty #ByteWriter instance
 *
 * Free-function: byte_writer_free
 *
 * Returns: (transfer full): a new, empty #ByteWriter instance
 */
ByteWriter *
byte_writer_new (void)
{
  ByteWriter *ret = g_slice_new0 (ByteWriter);

  ret->owned = TRUE;
  return ret;
}

/**
 * byte_writer_new_with_size:
 * @size: Initial size of data
 * @fixed: If %TRUE the data can't be reallocated
 *
 * Creates a new #ByteWriter instance with the given
 * initial data size.
 *
 * Free-function: byte_writer_free
 *
 * Returns: (transfer full): a new #ByteWriter instance
 */
ByteWriter *
byte_writer_new_with_size (uint32_t size, bool fixed)
{
  ByteWriter *ret = byte_writer_new ();

  ret->alloc_size = size;
  ret->parent.data = g_malloc (ret->alloc_size);
  ret->fixed = fixed;
  ret->owned = TRUE;

  return ret;
}

/**
 * byte_writer_new_with_data:
 * @data: Memory area for writing
 * @size: Size of @data in bytes
 * @initialized: If %TRUE the complete data can be read from the beginning
 *
 * Creates a new #ByteWriter instance with the given
 * memory area. If @initialized is %TRUE it is possible to
 * read @size bytes from the #ByteWriter from the beginning.
 *
 * Free-function: byte_writer_free
 *
 * Returns: (transfer full): a new #ByteWriter instance
 */
ByteWriter *
byte_writer_new_with_data (uint8_t * data, uint32_t size, bool initialized)
{
  ByteWriter *ret = byte_writer_new ();

  ret->parent.data = data;
  ret->parent.size = (initialized) ? size : 0;
  ret->alloc_size = size;
  ret->fixed = TRUE;
  ret->owned = FALSE;

  return ret;
}

/**
 * byte_writer_init:
 * @writer: #ByteWriter instance
 *
 * Initializes @writer to an empty instance
 */
void
byte_writer_init (ByteWriter * writer)
{
  g_return_if_fail (writer != NULL);

  memset (writer, 0, sizeof (ByteWriter));

  writer->owned = TRUE;
}

/**
 * byte_writer_init_with_size:
 * @writer: #ByteWriter instance
 * @size: Initial size of data
 * @fixed: If %TRUE the data can't be reallocated
 *
 * Initializes @writer with the given initial data size.
 */
void
byte_writer_init_with_size (ByteWriter * writer, uint32_t size,
    bool fixed)
{
  g_return_if_fail (writer != NULL);

  byte_writer_init (writer);

  writer->parent.data = g_malloc (size);
  writer->alloc_size = size;
  writer->fixed = fixed;
  writer->owned = TRUE;
}

/**
 * byte_writer_init_with_data:
 * @writer: #ByteWriter instance
 * @data: (array length=size) (transfer none): Memory area for writing
 * @size: Size of @data in bytes
 * @initialized: If %TRUE the complete data can be read from the beginning
 *
 * Initializes @writer with the given
 * memory area. If @initialized is %TRUE it is possible to
 * read @size bytes from the #ByteWriter from the beginning.
 */
void
byte_writer_init_with_data (ByteWriter * writer, uint8_t * data,
    uint32_t size, bool initialized)
{
  g_return_if_fail (writer != NULL);

  byte_writer_init (writer);

  writer->parent.data = data;
  writer->parent.size = (initialized) ? size : 0;
  writer->alloc_size = size;
  writer->fixed = TRUE;
  writer->owned = FALSE;
}

/**
 * byte_writer_reset:
 * @writer: #ByteWriter instance
 *
 * Resets @writer and frees the data if it's
 * owned by @writer.
 */
void
byte_writer_reset (ByteWriter * writer)
{
  g_return_if_fail (writer != NULL);

  if (writer->owned)
    g_free ((uint8_t *) writer->parent.data);
  memset (writer, 0, sizeof (ByteWriter));
}

/**
 * byte_writer_reset_and_get_data:
 * @writer: #ByteWriter instance
 *
 * Resets @writer and returns the current data.
 *
 * Free-function: g_free
 *
 * Returns: (array) (transfer full): the current data. g_free() after
 * usage.
 */
uint8_t *
byte_writer_reset_and_get_data (ByteWriter * writer)
{
  uint8_t *data;

  g_return_val_if_fail (writer != NULL, NULL);

  data = (uint8_t *) writer->parent.data;
  if (!writer->owned)
    data = g_memdup (data, writer->parent.size);
  writer->parent.data = NULL;
  byte_writer_reset (writer);

  return data;
}

/**
 * byte_writer_reset_and_get_buffer:
 * @writer: #ByteWriter instance
 *
 * Resets @writer and returns the current data as buffer.
 *
 * Free-function: buffer_unref
 *
 * Returns: (transfer full): the current data as buffer. buffer_unref()
 *     after usage.
 */
Buffer *
byte_writer_reset_and_get_buffer (ByteWriter * writer)
{
  Buffer *buffer;
  void* data;
  size_t size;

  g_return_val_if_fail (writer != NULL, NULL);

  size = writer->parent.size;
  data = byte_writer_reset_and_get_data (writer);

  buffer = buffer_new ();
  if (data != NULL) {
    buffer_append_memory (buffer,
        memory_new_wrapped (0, data, size, 0, size, data, g_free));
  }

  return buffer;
}

/**
 * byte_writer_free:
 * @writer: (in) (transfer full): #ByteWriter instance
 *
 * Frees @writer and all memory allocated by it.
 */
void
byte_writer_free (ByteWriter * writer)
{
  g_return_if_fail (writer != NULL);

  byte_writer_reset (writer);
  g_slice_free (ByteWriter, writer);
}

/**
 * byte_writer_free_and_get_data:
 * @writer: (in) (transfer full): #ByteWriter instance
 *
 * Frees @writer and all memory allocated by it except
 * the current data, which is returned.
 *
 * Free-function: g_free
 *
 * Returns: (transfer full): the current data. g_free() after usage.
 */
uint8_t *
byte_writer_free_and_get_data (ByteWriter * writer)
{
  uint8_t *data;

  g_return_val_if_fail (writer != NULL, NULL);

  data = byte_writer_reset_and_get_data (writer);
  g_slice_free (ByteWriter, writer);

  return data;
}

/**
 * byte_writer_free_and_get_buffer:
 * @writer: (in) (transfer full): #ByteWriter instance
 *
 * Frees @writer and all memory allocated by it except
 * the current data, which is returned as #Buffer.
 *
 * Free-function: buffer_unref
 *
 * Returns: (transfer full): the current data as buffer. buffer_unref()
 *     after usage.
 */
Buffer *
byte_writer_free_and_get_buffer (ByteWriter * writer)
{
  Buffer *buffer;

  g_return_val_if_fail (writer != NULL, NULL);

  buffer = byte_writer_reset_and_get_buffer (writer);
  g_slice_free (ByteWriter, writer);

  return buffer;
}

/**
 * byte_writer_get_remaining:
 * @writer: #ByteWriter instance
 *
 * Returns the remaining size of data that can still be written. If
 * -1 is returned the remaining size is only limited by system resources.
 *
 * Returns: the remaining size of data that can still be written
 */
uint32_t
byte_writer_get_remaining (const ByteWriter * writer)
{
  g_return_val_if_fail (writer != NULL, -1);

  if (!writer->fixed)
    return -1;
  else
    return writer->alloc_size - writer->parent.byte;
}

/**
 * byte_writer_ensure_free_space:
 * @writer: #ByteWriter instance
 * @size: Number of bytes that should be available
 *
 * Checks if enough free space from the current write cursor is
 * available and reallocates if necessary.
 *
 * Returns: %TRUE if at least @size bytes are still available
 */
bool
byte_writer_ensure_free_space (ByteWriter * writer, uint32_t size)
{
  return _byte_writer_ensure_free_space_inline (writer, size);
}


#define CREATE_WRITE_FUNC(bits,type,name,write_func) \
bool \
byte_writer_put_##name (ByteWriter *writer, type val) \
{ \
  return _byte_writer_put_##name##_inline (writer, val); \
}

CREATE_WRITE_FUNC (8, uint8_t, uint8, GST_WRITE_UINT8);
CREATE_WRITE_FUNC (8, int8_t, int8, GST_WRITE_UINT8);
CREATE_WRITE_FUNC (16, uint16_t, uint16_le, GST_WRITE_UINT16_LE);
CREATE_WRITE_FUNC (16, uint16_t, uint16_be, GST_WRITE_UINT16_BE);
CREATE_WRITE_FUNC (16, int16_t, int16_le, GST_WRITE_UINT16_LE);
CREATE_WRITE_FUNC (16, int16_t, int16_be, GST_WRITE_UINT16_BE);
CREATE_WRITE_FUNC (24, uint32_t, uint24_le, GST_WRITE_UINT24_LE);
CREATE_WRITE_FUNC (24, uint32_t, uint24_be, GST_WRITE_UINT24_BE);
CREATE_WRITE_FUNC (24, int32_t, int24_le, GST_WRITE_UINT24_LE);
CREATE_WRITE_FUNC (24, int32_t, int24_be, GST_WRITE_UINT24_BE);
CREATE_WRITE_FUNC (32, uint32_t, uint32_le, GST_WRITE_UINT32_LE);
CREATE_WRITE_FUNC (32, uint32_t, uint32_be, GST_WRITE_UINT32_BE);
CREATE_WRITE_FUNC (32, int32_t, int32_le, GST_WRITE_UINT32_LE);
CREATE_WRITE_FUNC (32, int32_t, int32_be, GST_WRITE_UINT32_BE);
CREATE_WRITE_FUNC (64, uint64_t, uint64_le, GST_WRITE_UINT64_LE);
CREATE_WRITE_FUNC (64, uint64_t, uint64_be, GST_WRITE_UINT64_BE);
CREATE_WRITE_FUNC (64, int64_t, int64_le, GST_WRITE_UINT64_LE);
CREATE_WRITE_FUNC (64, int64_t, int64_be, GST_WRITE_UINT64_BE);

CREATE_WRITE_FUNC (32, float, float32_be, GST_WRITE_FLOAT_BE);
CREATE_WRITE_FUNC (32, float, float32_le, GST_WRITE_FLOAT_LE);
CREATE_WRITE_FUNC (64, double, float64_be, GST_WRITE_DOUBLE_BE);
CREATE_WRITE_FUNC (64, double, float64_le, GST_WRITE_DOUBLE_LE);

bool
byte_writer_put_data (ByteWriter * writer, const uint8_t * data,
    uint32_t size)
{
  return _byte_writer_put_data_inline (writer, data, size);
}

bool
byte_writer_fill (ByteWriter * writer, uint8_t value, uint32_t size)
{
  return _byte_writer_fill_inline (writer, value, size);
}

#define CREATE_WRITE_STRING_FUNC(bits,type) \
bool \
byte_writer_put_string_utf##bits (ByteWriter *writer, const type * data) \
{ \
  uint32_t size = 0; \
  \
  g_return_val_if_fail (writer != NULL, FALSE); \
  \
  /* endianness does not matter if we are looking for a NUL terminator */ \
  while (data[size] != 0) { \
    /* have prevent overflow */ \
    if (G_UNLIKELY (size == G_MAXUINT)) \
      return FALSE; \
    ++size; \
  } \
  ++size; \
  \
  if (G_UNLIKELY (!_byte_writer_ensure_free_space_inline(writer, size * (bits / 8)))) \
    return FALSE; \
  \
  _byte_writer_put_data_inline (writer, (const uint8_t *) data, size * (bits / 8)); \
  \
  return TRUE; \
}

CREATE_WRITE_STRING_FUNC (8, char);
CREATE_WRITE_STRING_FUNC (16, uint16_t);
CREATE_WRITE_STRING_FUNC (32, uint32_t);
/**
 * byte_writer_put_uint8:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned 8 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint16_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned big endian 16 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint24_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned big endian 24 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint32_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned big endian 32 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint64_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned big endian 64 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint16_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned little endian 16 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint24_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned little endian 24 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint32_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned little endian 32 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_uint64_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a unsigned little endian 64 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int8:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed 8 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int16_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed big endian 16 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int24_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed big endian 24 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int32_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed big endian 32 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int64_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed big endian 64 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int16_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed little endian 16 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int24_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed little endian 24 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int32_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed little endian 32 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_int64_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a signed little endian 64 bit integer to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_float32_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a big endian 32 bit float to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_float64_be:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a big endian 64 bit float to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_float32_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a little endian 32 bit float to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_float64_le:
 * @writer: #ByteWriter instance
 * @val: Value to write
 *
 * Writes a little endian 64 bit float to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_string_utf8:
 * @writer: #ByteWriter instance
 * @data: (transfer none) (array zero-terminated=1) (type utf8): UTF8 string to
 *     write
 *
 * Writes a NUL-terminated UTF8 string to @writer (including the terminator).
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_string_utf16:
 * @writer: #ByteWriter instance
 * @data: (transfer none) (array zero-terminated=1): UTF16 string to write
 *
 * Writes a NUL-terminated UTF16 string to @writer (including the terminator).
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_string_utf32:
 * @writer: #ByteWriter instance
 * @data: (transfer none) (array zero-terminated=1): UTF32 string to write
 *
 * Writes a NUL-terminated UTF32 string to @writer (including the terminator).
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_put_data:
 * @writer: #ByteWriter instance
 * @data: (transfer none) (array length=size): Data to write
 * @size: Size of @data in bytes
 *
 * Writes @size bytes of @data to @writer.
 *
 * Returns: %TRUE if the value could be written
 */
/**
 * byte_writer_fill:
 * @writer: #ByteWriter instance
 * @value: Value to be written
 * @size: Number of bytes to be written
 *
 * Writes @size bytes containing @value to @writer.
 *
 * Returns: %TRUE if the value could be written
 */

/**
 * byte_writer_put_buffer:
 * @writer: #ByteWriter instance
 * @buffer: (transfer none): source #Buffer
 * @offset: offset to copy from
 * @size: total size to copy. If -1, all data is copied
 *
 * Writes @size bytes of @data to @writer.
 *
 * Returns: %TRUE if the data could be written
 *
 */
