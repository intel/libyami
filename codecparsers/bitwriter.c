/*
 *  bitwriter.c - bitstream writer
 *
 *  Copyright (C) 2013 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#define BIT_WRITER_DISABLE_INLINES

#include "bitwriter.h"

/**
 * bit_writer_init:
 * @bitwriter: a #BitWriter instance
 * @reserved_bits: reserved bits to allocate data
 *
 * Initializes a #BitWriter instance and allocate @reserved_bits
 * data inside.
 *
 * Cleanup function: bit_writer_clear
 */
void
bit_writer_init (BitWriter * bitwriter, uint32_t reserved_bits)
{
  bitwriter->bit_size = 0;
  bitwriter->data = NULL;
  bitwriter->bit_capacity = 0;
  bitwriter->auto_grow = true;
  if (reserved_bits)
    _bit_writer_check_space (bitwriter, reserved_bits);
}

/**
 * bit_writer_init_fill:
 * @bitwriter: a #BitWriter instance
 * @data: allocated data
  * @bits: size of allocated @data in bits
 *
 * Initializes a #BitWriter instance with alocated @data and @bit outside.
 *
 * Cleanup function: bit_writer_clear
 */
void
bit_writer_init_fill (BitWriter * bitwriter, uint8_t * data, uint32_t bits)
{
  bitwriter->bit_size = 0;
  bitwriter->data = data;
  bitwriter->bit_capacity = bits;
  bitwriter->auto_grow = false;
}

/**
 * bit_writer_clear:
 * @bitwriter: a #BitWriter instance
 * @free_data: flag to free #BitWriter allocated data
 *
 * Clear a #BitWriter instance and destroy allocated data inside
 * if @free_data is %true.
 */
void
bit_writer_clear (BitWriter * bitwriter, bool free_data)
{
  if (bitwriter->auto_grow && bitwriter->data && free_data)
    free (bitwriter->data);

  bitwriter->data = NULL;
  bitwriter->bit_size = 0;
  bitwriter->bit_capacity = 0;
}

/**
 * bit_writer_new:
 * @bitwriter: a #BitWriter instance
 * @reserved_bits: reserved bits to allocate data
 *
 * Create a #BitWriter instance and allocate @reserved_bits data inside.
 *
 * Free-function: bit_writer_free
 *
 * Returns: a new #BitWriter instance
 */
BitWriter *
bit_writer_new (uint32_t reserved_bits)
{
  BitWriter *ret = (BitWriter *) malloc (sizeof (BitWriter));

  bit_writer_init (ret, reserved_bits);

  return ret;
}

/**
 * bit_writer_new_fill:
 * @bitwriter: a #BitWriter instance
 * @data: allocated data
 * @bits: size of allocated @data in bits
 *
 * Create a #BitWriter instance with allocated @data and @bit outside.
 *
 * Free-function: bit_writer_free
 *
 * Returns: a new #BitWriter instance
 */
BitWriter *
bit_writer_new_fill (uint8_t * data, uint32_t bits)
{
  BitWriter *ret = (BitWriter *) calloc (sizeof (BitWriter), 1);

  bit_writer_init_fill (ret, data, bits);

  return ret;
}

/**
 * bit_writer_free:
 * @bitwriter: a #BitWriter instance
 * @free_data:  flag to free @data which is allocated inside
 *
 * Clear a #BitWriter instance and destroy allocated data inside if
 * @free_data is %true
 */
void
bit_writer_free (BitWriter * writer, bool free_data)
{
  if (writer == NULL)
    return;

  bit_writer_clear (writer, free_data);

  free (writer);
}

/**
 * bit_writer_get_size:
 * @bitwriter: a #BitWriter instance
 *
 * Get size of written @data
 *
 * Returns: size of bits written in @data
 */
uint
bit_writer_get_size (BitWriter * bitwriter)
{
  return _bit_writer_get_size_inline (bitwriter);
}

/**
 * bit_writer_get_data:
 * @bitwriter: a #BitWriter instance
 *
 * Get written @data pointer
 *
 * Returns: @data pointer
 */
uint8_t *
bit_writer_get_data (BitWriter * bitwriter)
{
  return _bit_writer_get_data_inline (bitwriter);
}

/**
 * bit_writer_get_data:
 * @bitwriter: a #BitWriter instance
 * @pos: new position of data end
 *
 * Set the new postion of data end which should be the new size of @data.
 *
 * Returns: %true if successful, %false otherwise
 */
bool
bit_writer_set_pos (BitWriter * bitwriter, uint32_t pos)
{
  return _bit_writer_set_pos_inline (bitwriter, pos);
}

/**
 * bit_writer_put_bits_uint8:
 * @bitwriter: a #BitWriter instance
 * @value: value of #uint8_t to write
 * @nbits: number of bits to write
 *
 * Write @nbits bits of @value to #BitWriter.
 *
 * Returns: %true if successful, %false otherwise.
 */

/**
 * bit_writer_put_bits_uint16:
 * @bitwriter: a #BitWriter instance
 * @value: value of #uint16 to write
 * @nbits: number of bits to write
 *
 * Write @nbits bits of @value to #BitWriter.
 *
 * Returns: %true if successful, %false otherwise.
 */

/**
 * bit_writer_put_bits_uint32:
 * @bitwriter: a #BitWriter instance
 * @value: value of #uint32_t to write
 * @nbits: number of bits to write
 *
 * Write @nbits bits of @value to #BitWriter.
 *
 * Returns: %true if successful, %false otherwise.
 */

/**
 * bit_writer_put_bits_uint64:
 * @bitwriter: a #BitWriter instance
 * @value: value of #uint64 to write
 * @nbits: number of bits to write
 *
 * Write @nbits bits of @value to #BitWriter.
 *
 * Returns: %true if successful, %false otherwise.
 */

#define BIT_WRITER_WRITE_BITS(bits) \
bool \
bit_writer_put_bits_uint##bits (BitWriter *bitwriter, uint##bits##_t value, uint32_t nbits) \
{ \
  return _bit_writer_put_bits_uint##bits##_inline (bitwriter, value, nbits); \
}

BIT_WRITER_WRITE_BITS (8)
BIT_WRITER_WRITE_BITS (16)
BIT_WRITER_WRITE_BITS (32)
BIT_WRITER_WRITE_BITS (64)
#undef BIT_WRITER_WRITE_BITS
/**
 * bit_writer_put_bytes:
 * @bitwriter: a #BitWriter instance
 * @data: pointer of data to write
 * @nbytes: number of bytes to write
 *
 * Write @nbytes bytes of @data to #BitWriter.
 *
 * Returns: %true if successful, %false otherwise.
 */
    bool
bit_writer_put_bytes (BitWriter * bitwriter, const uint8_t * data, uint32_t nbytes)
{
  return _bit_writer_put_bytes_inline (bitwriter, data, nbytes);
}

/**
 * bit_writer_align_bytes:
 * @bitwriter: a #BitWriter instance
 * @trailing_bit: trailing bits of last byte, 0 or 1
 *
 * Write trailing bit to align last byte of @data. @trailing_bit can
 * only be 1 or 0.
 *
 * Returns: %true if successful, %false otherwise.
 */
bool
bit_writer_align_bytes (BitWriter * bitwriter, uint8_t trailing_bit)
{
  return _bit_writer_align_bytes_inline (bitwriter, trailing_bit);
}
