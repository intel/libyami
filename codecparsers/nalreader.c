/* GStreamer
 *
 * Copyright (C) 2009 Carl-Anton Ingmarsson <ca.ingmarsson@gmail.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include "nalreader.h"

static bool nal_reader_read (NalReader * reader, uint32_t nbits);

/**
 * SECTION:nalreader
 * @short_description: Bit reader which automatically skips
 * emulation_prevention bytes
 *
 * #NalReader provides a bit reader which automatically skips
 * emulation_prevention bytes. It provides functions for reading any number of bits
 * into 8, 16, 32 and 64 bit variables. It also provides functions for reading
 * Exp-Golomb values.
 */

/**
 * nal_reader_new:
 * @data: Data from which the #NalReader should read
 * @size: Size of @data in bytes
 *
 * Create a new #NalReader instance, which will read from @data.
 *
 * Returns: a new #NalReader instance
 *
 * Since: 0.10.22
 */
NalReader *
nal_reader_new (const uint8_t * data, uint32_t size)
{
  NalReader *ret = (NalReader*) malloc (sizeof(NalReader));
  if (!ret)
    return NULL;

  ret->data = data;
  ret->size = size;

  ret->first_byte = 0xff;
  ret->cache = 0xff;

  return ret;
}

/**
 * nal_reader_free:
 * @reader: a #NalReader instance
 *
 * Frees a #NalReader instance, which was previously allocated by
 * nal_reader_new() or nal_reader_new_from_buffer().
 * 
 * Since: 0.10.22
 */
void
nal_reader_free (NalReader * reader)
{
  RETURN_IF_FAIL (reader != NULL);

  free (reader);
}

/**
 * nal_reader_init:
 * @reader: a #NalReader instance
 * @data: Data from which the #NalReader should read
 * @size: Size of @data in bytes
 *
 * Initializes a #NalReader instance to read from @data. This function
 * can be called on already initialized instances.
 * 
 * Since: 0.10.22
 */
void
nal_reader_init (NalReader * reader, const uint8_t * data, uint32_t size)
{
  RETURN_IF_FAIL (reader != NULL);

  reader->data = data;
  reader->size = size;

  reader->n_epb = 0;
  reader->byte = 0;
  reader->bits_in_cache = 0;
  /* fill with something other than 0 to detect emulation prevention bytes */
  reader->first_byte = 0xff;
  reader->cache = 0xff;
}

/**
 * nal_reader_skip:
 * @reader: a #NalReader instance
 * @nbits: the number of bits to skip
 *
 * Skips @nbits bits of the #NalReader instance.
 *
 * Returns: %true if @nbits bits could be skipped, %false otherwise.
 * 
 * Since: 0.10.22
 */
bool
nal_reader_skip (NalReader * reader, uint32_t nbits)
{
  RETURN_VAL_IF_FAIL (reader != NULL, false);

  if (!nal_reader_read (reader, nbits))
    return false;

  reader->bits_in_cache -= nbits;

  return true;
}

/**
 * nal_reader_skip_to_byte:
 * @reader: a #NalReader instance
 *
 * Skips until the next byte.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */
bool
nal_reader_skip_to_byte (NalReader * reader)
{
  RETURN_VAL_IF_FAIL (reader != NULL, false);

  if (reader->bits_in_cache == 0) {
    if ((reader->size - reader->byte) > 0)
      reader->byte++;
    else
      return false;
  }

  reader->bits_in_cache = 0;

  return true;
}

/**
 * nal_reader_get_pos:
 * @reader: a #NalReader instance
 *
 * Returns the current position of a NalReader instance in bits.
 *
 * Returns: The current position in bits
 *
 */
uint32_t
nal_reader_get_pos (const NalReader * reader)
{
  return reader->byte * 8 - reader->bits_in_cache;
}

/**
 * nal_reader_get_remaining:
 * @reader: a #NalReader instance
 *
 * Returns the remaining number of bits of a NalReader instance.
 *
 * Returns: The remaining number of bits.
 *
 */
uint32_t
nal_reader_get_remaining (const NalReader * reader)
{
  return (reader->size - reader->byte) * 8 + reader->bits_in_cache;
}

/**
 * nal_reader_get_epb:
 * @reader: a #NalReader instance
 *
 * Returns the total number of emulation prevention bytes.
 *
 * Returns: The sum of epb.
 *
 */
uint32_t
nal_reader_get_epb_count (const NalReader * reader)
{
  return reader->n_epb;
}

/**
 * nal_reader_get_bits_uint8:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint328 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint16:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint16_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint32:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint64:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint64_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint8:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint328 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint16:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint16_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint32:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint64:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint64_t to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %true if successful, %false otherwise.
 * 
 * Since: 0.10.22
 */

static bool
nal_reader_read (NalReader * reader, uint32_t nbits)
{
  if (reader->byte * 8 + (nbits - reader->bits_in_cache) >
          reader->size * 8)
    return false;

  while (reader->bits_in_cache < nbits) {
    uint8_t byte;
    bool check_three_byte;

    check_three_byte = true;
  next_byte:
    if (reader->byte >= reader->size)
      return false;

    byte = reader->data[reader->byte++];

    /* check if the byte is a emulation_prevention_three_byte */
    if (check_three_byte && byte == 0x03 && reader->first_byte == 0x00 &&
        ((reader->cache & 0xff) == 0)) {
      /* next byte goes unconditionally to the cache, even if it's 0x03 */
      check_three_byte = false;
      reader->n_epb++;
      goto next_byte;
    }
    reader->cache = (reader->cache << 8) | reader->first_byte;
    reader->first_byte = byte;
    reader->bits_in_cache += 8;
  }

  return true;
}

#define NAL_READER_READ_BITS(bits) \
bool \
nal_reader_get_bits_uint##bits (NalReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  uint32_t shift; \
  \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  RETURN_VAL_IF_FAIL (val != NULL, false); \
  RETURN_VAL_IF_FAIL (nbits <= bits, false); \
  \
  if (!nal_reader_read (reader, nbits)) \
    return false; \
  \
  /* bring the required bits down and truncate */ \
  shift = reader->bits_in_cache - nbits; \
  *val = reader->first_byte >> shift; \
  \
  *val |= reader->cache << (8 - shift); \
  /* mask out required bits */ \
  if (nbits < bits) \
    *val &= ((uint##bits##_t)1 << nbits) - 1; \
  \
  reader->bits_in_cache = shift; \
  \
  return true; \
} \
\
bool \
nal_reader_peek_bits_uint##bits (const NalReader *reader, uint##bits##_t *val, uint32_t nbits) \
{ \
  NalReader tmp; \
  \
  RETURN_VAL_IF_FAIL (reader != NULL, false); \
  tmp = *reader; \
  return nal_reader_get_bits_uint##bits (&tmp, val, nbits); \
}

NAL_READER_READ_BITS (8);
NAL_READER_READ_BITS (16);
NAL_READER_READ_BITS (32);
NAL_READER_READ_BITS (64);

/**
 * nal_reader_get_ue:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32_t to store the result
 *
 * Reads an unsigned Exp-Golomb value into val
 *
 * Returns: %true if successful, %false otherwise.
 */
bool
nal_reader_get_ue (NalReader * reader, uint32_t * val)
{
  uint32_t i = 0;
  uint8_t bit;
  uint32_t value;

  if (!nal_reader_get_bits_uint8 (reader, &bit, 1))
    return false;

  while (bit == 0) {
    i++;
    if ((!nal_reader_get_bits_uint8 (reader, &bit, 1)))
          return false;
  }

  RETURN_VAL_IF_FAIL (i <= 32, false);

  if (!nal_reader_get_bits_uint32 (reader, &value, i))
    return false;

  *val = (1 << i) - 1 + value;

  return true;
}

/**
 * nal_reader_peek_ue:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32_t to store the result
 *
 * Read an unsigned Exp-Golomb value into val but keep the current position
 *
 * Returns: %true if successful, %false otherwise.
 */
bool
nal_reader_peek_ue (const NalReader * reader, uint32_t * val)
{
  NalReader tmp;

  RETURN_VAL_IF_FAIL (reader != NULL, false);

  tmp = *reader;
  return nal_reader_get_ue (&tmp, val);
}

/**
 * nal_reader_get_se:
 * @reader: a #NalReader instance
 * @val: Pointer to a #int32_t to store the result
 *
 * Reads a signed Exp-Golomb value into val
 *
 * Returns: %true if successful, %false otherwise.
 */
bool
nal_reader_get_se (NalReader * reader, int32_t * val)
{
  uint32_t value;

  if (!nal_reader_get_ue (reader, &value))
    return false;

  if (value % 2)
    *val = (value / 2) + 1;
  else
    *val = -(value / 2);

  return true;
}

/**
 * nal_reader_peek_se:
 * @reader: a #NalReader instance
 * @val: Pointer to a #int32_t to store the result
 *
 * Read a signed Exp-Golomb value into val but keep the current position
 *
 * Returns: %true if successful, %false otherwise.
 */
bool
nal_reader_peek_se (const NalReader * reader, int32_t * val)
{
  NalReader tmp;

  RETURN_VAL_IF_FAIL (reader != NULL, false);

  tmp = *reader;
  return nal_reader_get_se (&tmp, val);
}
