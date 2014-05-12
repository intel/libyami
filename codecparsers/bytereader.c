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


#define BYTE_READER_DISABLE_INLINES
#include <stdlib.h>
#include "bytereader.h"

/**
 * SECTION:bytereader
 * @short_description: Reads different integer, string and floating point
 *     types from a memory buffer
 *
 * #ByteReader provides a byte reader that can read different integer and
 * floating point types from a memory buffer. It provides functions for reading
 * signed/unsigned, little/big endian integers of 8, 16, 24, 32 and 64 bits
 * and functions for reading little/big endian floating points numbers of
 * 32 and 64 bits. It also provides functions to read NUL-terminated strings
 * in various character encodings.
 */

/**
 * byte_reader_new:
 * @data: (in) (transfer none) (array length=size): data from which the
 *     #ByteReader should read
 * @size: Size of @data in bytes
 *
 * Create a new #ByteReader instance, which will read from @data.
 *
 * Free-function: byte_reader_free
 *
 * Returns: (transfer full): a new #ByteReader instance
 *
 * Since: 0.10.22
 */
ByteReader *
byte_reader_new (const uint8_t * data, uint32_t size)
{
  ByteReader *ret =  (ByteReader*) malloc (sizeof(ByteReader));
  if (!ret)
    return NULL;

  ret->data = data;
  ret->size = size;

  return ret;
}

/**
 * byte_reader_free:
 * @reader: (in) (transfer full): a #ByteReader instance
 *
 * Frees a #ByteReader instance, which was previously allocated by
 * byte_reader_new() or byte_reader_new_from_buffer().
 * 
 * Since: 0.10.22
 */
void
byte_reader_free (ByteReader * reader)
{
  RETURN_IF_FAIL (reader != NULL);

  free (reader);
}

/**
 * byte_reader_init:
 * @reader: a #ByteReader instance
 * @data: (in) (transfer none) (array length=size): data from which
 *     the #ByteReader should read
 * @size: Size of @data in bytes
 *
 * Initializes a #ByteReader instance to read from @data. This function
 * can be called on already initialized instances.
 * 
 * Since: 0.10.22
 */
void
byte_reader_init (ByteReader * reader, const uint8_t * data, uint32_t size)
{
  RETURN_IF_FAIL (reader != NULL);

  reader->data = data;
  reader->size = size;
  reader->byte = 0;
}

/**
 * byte_reader_set_pos:
 * @reader: a #ByteReader instance
 * @pos: The new position in bytes
 *
 * Sets the new position of a #ByteReader instance to @pos in bytes.
 *
 * Returns: %true if the position could be set successfully, %false
 * otherwise.
 * 
 * Since: 0.10.22
 */
bool
byte_reader_set_pos (ByteReader * reader, uint32_t pos)
{
  RETURN_VAL_IF_FAIL (reader != NULL, false);

  if (pos > reader->size)
    return false;

  reader->byte = pos;

  return true;
}

/**
 * byte_reader_get_pos:
 * @reader: a #ByteReader instance
 *
 * Returns the current position of a #ByteReader instance in bytes.
 *
 * Returns: The current position of @reader in bytes.
 * 
 * Since: 0.10.22
 */
uint32_t
byte_reader_get_pos (const ByteReader * reader)
{
  return byte_reader_get_pos_inline (reader);
}

/**
 * byte_reader_get_remaining:
 * @reader: a #ByteReader instance
 *
 * Returns the remaining number of bytes of a #ByteReader instance.
 *
 * Returns: The remaining number of bytes of @reader instance.
 * 
 * Since: 0.10.22
 */
uint32_t
byte_reader_get_remaining (const ByteReader * reader)
{
  return byte_reader_get_remaining_inline (reader);
}

/**
 * byte_reader_get_size:
 * @reader: a #ByteReader instance
 *
 * Returns the total number of bytes of a #ByteReader instance.
 *
 * Returns: The total number of bytes of @reader instance.
 * 
 * Since: 0.10.26
 */
uint32_t
byte_reader_get_size (const ByteReader * reader)
{
  return byte_reader_get_size_inline (reader);
}

#define byte_reader_get_remaining byte_reader_get_remaining_inline
#define byte_reader_get_size byte_reader_get_size_inline

/**
 * byte_reader_skip:
 * @reader: a #ByteReader instance
 * @nbytes: the number of bytes to skip
 *
 * Skips @nbytes bytes of the #ByteReader instance.
 *
 * Returns: %true if @nbytes bytes could be skipped, %false otherwise.
 * 
 * Since: 0.10.22
 */
bool
byte_reader_skip (ByteReader * reader, uint32_t nbytes)
{
  return byte_reader_skip_inline (reader, nbytes);
}

/**
 * byte_reader_get_uint8:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint8_t to store the result
 *
 * Read an unsigned 8 bit integer into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int8:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int8_t to store the result
 *
 * Read a signed 8 bit integer into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint8:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint8_t to store the result
 *
 * Read an unsigned 8 bit integer into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int8:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int8_t to store the result
 *
 * Read a signed 8 bit integer into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint16_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint16_t to store the result
 *
 * Read an unsigned 16 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int16_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int16_t to store the result
 *
 * Read a signed 16 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint16_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint16_t to store the result
 *
 * Read an unsigned 16 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int16_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int16_t to store the result
 *
 * Read a signed 16 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint16_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint16_t to store the result
 *
 * Read an unsigned 16 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int16_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int16_t to store the result
 *
 * Read a signed 16 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint16_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint16_t to store the result
 *
 * Read an unsigned 16 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int16_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int16_t to store the result
 *
 * Read a signed 16 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint24_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 24 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int24_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 24 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint24_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 24 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int24_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 24 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint24_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 24 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int24_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 24 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint24_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 24 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int24_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 24 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */


/**
 * byte_reader_get_uint32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 32 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 32 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 32 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 32 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 32 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 32 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint32_t to store the result
 *
 * Read an unsigned 32 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int32_t to store the result
 *
 * Read a signed 32 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint64_t to store the result
 *
 * Read an unsigned 64 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int64_t to store the result
 *
 * Read a signed 64 bit little endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint64_t to store the result
 *
 * Read an unsigned 64 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int64_t to store the result
 *
 * Read a signed 64 bit little endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_uint64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint64_t to store the result
 *
 * Read an unsigned 64 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_int64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int64_t to store the result
 *
 * Read a signed 64 bit big endian integer into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_uint64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #uint64_t to store the result
 *
 * Read an unsigned 64 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_int64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #int64_t to store the result
 *
 * Read a signed 64 bit big endian integer into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

#define BYTE_READER_PEEK_GET(bits,type,name) \
bool \
byte_reader_get_##name (ByteReader * reader, type * val) \
{ \
  return byte_reader_get_##name##_inline (reader, val); \
} \
\
bool \
byte_reader_peek_##name (const ByteReader * reader, type * val) \
{ \
  return byte_reader_peek_##name##_inline (reader, val); \
}
/* *INDENT-OFF* */

BYTE_READER_PEEK_GET(8,uint8_t,uint8_t)
BYTE_READER_PEEK_GET(8,int8_t,int8_t)

BYTE_READER_PEEK_GET(16,uint16_t,uint16_le)
BYTE_READER_PEEK_GET(16,uint16_t,uint16_be)
BYTE_READER_PEEK_GET(16,int16_t,int16_le)
BYTE_READER_PEEK_GET(16,int16_t,int16_be)

BYTE_READER_PEEK_GET(24,uint32_t,uint24_le)
BYTE_READER_PEEK_GET(24,uint32_t,uint24_be)
BYTE_READER_PEEK_GET(24,int32_t,int24_le)
BYTE_READER_PEEK_GET(24,int32_t,int24_be)

BYTE_READER_PEEK_GET(32,uint32_t,uint32_le)
BYTE_READER_PEEK_GET(32,uint32_t,uint32_be)
BYTE_READER_PEEK_GET(32,int32_t,int32_le)
BYTE_READER_PEEK_GET(32,int32_t,int32_be)

BYTE_READER_PEEK_GET(64,uint64_t,uint64_le)
BYTE_READER_PEEK_GET(64,uint64_t,uint64_be)
BYTE_READER_PEEK_GET(64,int64_t,int64_le)
BYTE_READER_PEEK_GET(64,int64_t,int64_be)

/**
 * byte_reader_get_float32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #float to store the result
 *
 * Read a 32 bit little endian floating point value into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_float32_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #float to store the result
 *
 * Read a 32 bit little endian floating point value into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_float32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #float to store the result
 *
 * Read a 32 bit big endian floating point value into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_float32_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #float to store the result
 *
 * Read a 32 bit big endian floating point value into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_float64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #double to store the result
 *
 * Read a 64 bit little endian floating point value into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_float64_le:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #double to store the result
 *
 * Read a 64 bit little endian floating point value into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_get_float64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #double to store the result
 *
 * Read a 64 bit big endian floating point value into @val
 * and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * byte_reader_peek_float64_be:
 * @reader: a #ByteReader instance
 * @val: (out): Pointer to a #double to store the result
 *
 * Read a 64 bit big endian floating point value into @val
 * but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

BYTE_READER_PEEK_GET(32,float,float32_le)
BYTE_READER_PEEK_GET(32,float,float32_be)
BYTE_READER_PEEK_GET(64,double,float64_le)
BYTE_READER_PEEK_GET(64,double,float64_be)

/* *INDENT-ON* */

/**
 * byte_reader_get_data:
 * @reader: a #ByteReader instance
 * @size: Size in bytes
 * @val: (out) (transfer none) (array length=size): address of a
 *     #uint8_t pointer variable in which to store the result
 *
 * Returns a constant pointer to the current data
 * position if at least @size bytes are left and
 * updates the current position.
 *
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */
bool
byte_reader_get_data (ByteReader * reader, uint32_t size,
    const uint8_t ** val)
{
  return byte_reader_get_data_inline (reader, size, val);
}

/**
 * byte_reader_peek_data:
 * @reader: a #ByteReader instance
 * @size: Size in bytes
 * @val: (out) (transfer none) (array length=size): address of a
 *     #uint8_t pointer variable in which to store the result
 *
 * Returns a constant pointer to the current data
 * position if at least @size bytes are left and
 * keeps the current position.
 *
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */
bool
byte_reader_peek_data (const ByteReader * reader, uint32_t size,
    const uint8_t ** val)
{
  return byte_reader_peek_data_inline (reader, size, val);
}

/**
 * byte_reader_masked_scan_uint32:
 * @reader: a #ByteReader
 * @mask: mask to apply to data before matching against @pattern
 * @pattern: pattern to match (after mask is applied)
 * @offset: offset from which to start scanning, relative to the current
 *     position
 * @size: number of bytes to scan from offset
 *
 * Scan for pattern @pattern with applied mask @mask in the byte reader data,
 * starting from offset @offset relative to the current position.
 *
 * The bytes in @pattern and @mask are interpreted left-to-right, regardless
 * of endianness.  All four bytes of the pattern must be present in the
 * byte reader data for it to match, even if the first or last bytes are masked
 * out.
 *
 * It is an error to call this function without making sure that there is
 * enough data (offset+size bytes) in the byte reader.
 *
 * Returns: offset of the first match, or -1 if no match was found.
 *
 * Example:
 * <programlisting>
 * // Assume the reader contains 0x00 0x01 0x02 ... 0xfe 0xff
 *
 * byte_reader_masked_scan_uint32 (reader, 0xffffffff, 0x00010203, 0, 256);
 * // -> returns 0
 * byte_reader_masked_scan_uint32 (reader, 0xffffffff, 0x00010203, 1, 255);
 * // -> returns -1
 * byte_reader_masked_scan_uint32 (reader, 0xffffffff, 0x01020304, 1, 255);
 * // -> returns 1
 * byte_reader_masked_scan_uint32 (reader, 0xffff, 0x0001, 0, 256);
 * // -> returns -1
 * byte_reader_masked_scan_uint32 (reader, 0xffff, 0x0203, 0, 256);
 * // -> returns 0
 * byte_reader_masked_scan_uint32 (reader, 0xffff0000, 0x02030000, 0, 256);
 * // -> returns 2
 * byte_reader_masked_scan_uint32 (reader, 0xffff0000, 0x02030000, 0, 4);
 * // -> returns -1
 * </programlisting>
 *
 * Since: 0.10.24
 */
uint32_t
byte_reader_masked_scan_uint32 (const ByteReader * reader, uint32_t mask,
    uint32_t pattern, uint32_t offset, uint32_t size)
{
  const uint8_t *data;
  uint32_t state;
  uint32_t i;

  RETURN_VAL_IF_FAIL (size > 0, -1);
  RETURN_VAL_IF_FAIL ((uint64_t) offset + size <= reader->size - reader->byte, -1);

  /* we can't find the pattern with less than 4 bytes */
  if (size < 4)
    return -1;

  data = reader->data + reader->byte + offset;

  /* set the state to something that does not match */
  state = ~pattern;

  /* now find data */
  for (i = 0; i < size; i++) {
    /* throw away one byte and move in the next byte */
    state = ((state << 8) | data[i]);
    if ((state & mask) == pattern) {
      /* we have a match but we need to have skipped at
       * least 4 bytes to fill the state. */
      if (i >= 3)
        return offset + i - 3;
    }
  }

  /* nothing found */
  return -1;
}

#define BYTE_READER_SCAN_STRING(bits) \
static uint32_t \
byte_reader_scan_string_utf##bits (const ByteReader * reader) \
{ \
  uint32_t len, off, max_len; \
  \
  max_len = (reader->size - reader->byte) / sizeof (uint##bits##_t); \
  \
  /* need at least a single NUL terminator */ \
  if (max_len < 1) \
    return 0; \
  \
  len = 0; \
  off = reader->byte; \
  /* endianness does not matter if we are looking for a NUL terminator */ \
  while (READ_UINT##bits##_LE (&reader->data[off]) != 0) { \
    ++len; \
    off += sizeof (uint##bits##_t); \
    /* have we reached the end without finding a NUL terminator? */ \
    if (len == max_len) \
      return 0; \
  } \
  /* return size in bytes including the NUL terminator (hence the +1) */ \
  return (len + 1) * sizeof (uint##bits##_t); \
}

BYTE_READER_SCAN_STRING (8);
BYTE_READER_SCAN_STRING (16);
BYTE_READER_SCAN_STRING (32);

#define BYTE_READER_SKIP_STRING(bits) \
bool \
byte_reader_skip_string_utf##bits (ByteReader * reader) \
{ \
  uint32_t size; /* size in bytes including the terminator */ \
  \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  \
  size = byte_reader_scan_string_utf##bits (reader); \
  reader->byte += size; \
  return (size > 0); \
}

/**
 * byte_reader_skip_string:
 * @reader: a #ByteReader instance
 *
 * Skips a NUL-terminated string in the #ByteReader instance, advancing
 * the current position to the byte after the string. This will work for
 * any NUL-terminated string with a character width of 8 bits, so ASCII,
 * UTF-8, ISO-8859-N etc.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
/**
 * byte_reader_skip_string_utf8:
 * @reader: a #ByteReader instance
 *
 * Skips a NUL-terminated string in the #ByteReader instance, advancing
 * the current position to the byte after the string. This will work for
 * any NUL-terminated string with a character width of 8 bits, so ASCII,
 * UTF-8, ISO-8859-N etc. No input checking for valid UTF-8 is done.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
BYTE_READER_SKIP_STRING (8);

/**
 * byte_reader_skip_string_utf16:
 * @reader: a #ByteReader instance
 *
 * Skips a NUL-terminated UTF-16 string in the #ByteReader instance,
 * advancing the current position to the byte after the string.
 *
 * No input checking for valid UTF-16 is done.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
BYTE_READER_SKIP_STRING (16);

/**
 * byte_reader_skip_string_utf32:
 * @reader: a #ByteReader instance
 *
 * Skips a NUL-terminated UTF-32 string in the #ByteReader instance,
 * advancing the current position to the byte after the string.
 *
 * No input checking for valid UTF-32 is done.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
BYTE_READER_SKIP_STRING (32);

/**
 * byte_reader_peek_string:
 * @reader: a #ByteReader instance
 * @str: (out) (transfer none) (array zero-terminated=1): address of a
 *     #char pointer varieble in which to store the result
 *
 * Returns a constant pointer to the current data position if there is
 * a NUL-terminated string in the data (this could be just a NUL terminator).
 * The current position will be maintained. This will work for any
 * NUL-terminated string with a character width of 8 bits, so ASCII,
 * UTF-8, ISO-8859-N etc.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
/**
 * byte_reader_peek_string_utf8:
 * @reader: a #ByteReader instance
 * @str: (out) (transfer none) (array zero-terminated=1): address of a
 *     #char pointer varieble in which to store the result
 *
 * Returns a constant pointer to the current data position if there is
 * a NUL-terminated string in the data (this could be just a NUL terminator).
 * The current position will be maintained. This will work for any
 * NUL-terminated string with a character width of 8 bits, so ASCII,
 * UTF-8, ISO-8859-N etc.
 *
 * No input checking for valid UTF-8 is done.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be skipped, %false otherwise.
 *
 * Since: 0.10.24
 */
bool
byte_reader_peek_string_utf8 (const ByteReader * reader,
    const char ** str)
{
  RETURN_VAL_IF_FAIL (reader != NULL, false);
  RETURN_VAL_IF_FAIL (str != NULL, false);

  if (byte_reader_scan_string_utf8 (reader) > 0) {
    *str = (const char *) (reader->data + reader->byte);
  } else {
    *str = NULL;
  }
  return (*str != NULL);
}

/**
 * byte_reader_get_string_utf8:
 * @reader: a #ByteReader instance
 * @str: (out) (transfer none) (array zero-terminated=1): address of a
 *     #char pointer varieble in which to store the result
 *
 * Returns a constant pointer to the current data position if there is
 * a NUL-terminated string in the data (this could be just a NUL terminator),
 * advancing the current position to the byte after the string. This will work
 * for any NUL-terminated string with a character width of 8 bits, so ASCII,
 * UTF-8, ISO-8859-N etc.
 *
 * No input checking for valid UTF-8 is done.
 *
 * This function will fail if no NUL-terminator was found in in the data.
 *
 * Returns: %true if a string could be found, %false otherwise.
 *
 * Since: 0.10.24
 */
bool
byte_reader_get_string_utf8 (ByteReader * reader, const char ** str)
{
  uint32_t size;                   /* size in bytes including the terminator */

  RETURN_VAL_IF_FAIL (reader != NULL, false);
  RETURN_VAL_IF_FAIL (str != NULL, false);

  size = byte_reader_scan_string_utf8 (reader);
  if (size == 0) {
    *str = NULL;
    return false;
  }

  *str = (const char *) (reader->data + reader->byte);
  reader->byte += size;
  return true;
}


