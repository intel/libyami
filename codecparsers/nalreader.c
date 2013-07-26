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

static boolean nal_reader_read (NalReader * reader, uint32 nbits);

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
nal_reader_new (const uint8 * data, uint32 size)
{
  NalReader *ret = (NalReader*) malloc (sizeof(NalReader));

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
nal_reader_init (NalReader * reader, const uint8 * data, uint32 size)
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
 * Returns: %TRUE if @nbits bits could be skipped, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */
boolean
nal_reader_skip (NalReader * reader, uint32 nbits)
{
  RETURN_VAL_IF_FAIL (reader != NULL, FALSE);

  if (!nal_reader_read (reader, nbits))
    return FALSE;

  reader->bits_in_cache -= nbits;

  return TRUE;
}

/**
 * nal_reader_skip_to_byte:
 * @reader: a #NalReader instance
 *
 * Skips until the next byte.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */
boolean
nal_reader_skip_to_byte (NalReader * reader)
{
  RETURN_VAL_IF_FAIL (reader != NULL, FALSE);

  if (reader->bits_in_cache == 0) {
    if ((reader->size - reader->byte) > 0)
      reader->byte++;
    else
      return FALSE;
  }

  reader->bits_in_cache = 0;

  return TRUE;
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
uint32
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
uint32
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
uint32
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
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint16:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint16 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint32:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_get_bits_uint64:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint64 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val and update the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
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
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint16:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint16 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint32:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

/**
 * nal_reader_peek_bits_uint64:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint64 to store the result
 * @nbits: number of bits to read
 *
 * Read @nbits bits into @val but keep the current position.
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 * 
 * Since: 0.10.22
 */

static boolean
nal_reader_read (NalReader * reader, uint32 nbits)
{
  if (reader->byte * 8 + (nbits - reader->bits_in_cache) >
          reader->size * 8)
    return FALSE;

  while (reader->bits_in_cache < nbits) {
    uint8 byte;
    boolean check_three_byte;

    check_three_byte = TRUE;
  next_byte:
    if (reader->byte >= reader->size)
      return FALSE;

    byte = reader->data[reader->byte++];

    /* check if the byte is a emulation_prevention_three_byte */
    if (check_three_byte && byte == 0x03 && reader->first_byte == 0x00 &&
        ((reader->cache & 0xff) == 0)) {
      /* next byte goes unconditionally to the cache, even if it's 0x03 */
      check_three_byte = FALSE;
      reader->n_epb++;
      goto next_byte;
    }
    reader->cache = (reader->cache << 8) | reader->first_byte;
    reader->first_byte = byte;
    reader->bits_in_cache += 8;
  }

  return TRUE;
}

#define NAL_READER_READ_BITS(bits) \
boolean \
nal_reader_get_bits_uint##bits (NalReader *reader, uint##bits *val, uint32 nbits) \
{ \
  uint32 shift; \
  \
  RETURN_VAL_IF_FAIL (reader != NULL, FALSE); \
  RETURN_VAL_IF_FAIL (val != NULL, FALSE); \
  RETURN_VAL_IF_FAIL (nbits <= bits, FALSE); \
  \
  if (!nal_reader_read (reader, nbits)) \
    return FALSE; \
  \
  /* bring the required bits down and truncate */ \
  shift = reader->bits_in_cache - nbits; \
  *val = reader->first_byte >> shift; \
  \
  *val |= reader->cache << (8 - shift); \
  /* mask out required bits */ \
  if (nbits < bits) \
    *val &= ((uint##bits)1 << nbits) - 1; \
  \
  reader->bits_in_cache = shift; \
  \
  return TRUE; \
} \
\
boolean \
nal_reader_peek_bits_uint##bits (const NalReader *reader, uint##bits *val, uint32 nbits) \
{ \
  NalReader tmp; \
  \
  RETURN_VAL_IF_FAIL (reader != NULL, FALSE); \
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
 * @val: Pointer to a #uint32 to store the result
 *
 * Reads an unsigned Exp-Golomb value into val
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
boolean
nal_reader_get_ue (NalReader * reader, uint32 * val)
{
  uint32 i = 0;
  uint8 bit;
  uint32 value;

  if (!nal_reader_get_bits_uint8 (reader, &bit, 1))
    return FALSE;

  while (bit == 0) {
    i++;
    if ((!nal_reader_get_bits_uint8 (reader, &bit, 1)))
          return FALSE;
  }

  RETURN_VAL_IF_FAIL (i <= 32, FALSE);

  if (!nal_reader_get_bits_uint32 (reader, &value, i))
    return FALSE;

  *val = (1 << i) - 1 + value;

  return TRUE;
}

/**
 * nal_reader_peek_ue:
 * @reader: a #NalReader instance
 * @val: Pointer to a #uint32 to store the result
 *
 * Read an unsigned Exp-Golomb value into val but keep the current position
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
boolean
nal_reader_peek_ue (const NalReader * reader, uint32 * val)
{
  NalReader tmp;

  RETURN_VAL_IF_FAIL (reader != NULL, FALSE);

  tmp = *reader;
  return nal_reader_get_ue (&tmp, val);
}

/**
 * nal_reader_get_se:
 * @reader: a #NalReader instance
 * @val: Pointer to a #int32 to store the result
 *
 * Reads a signed Exp-Golomb value into val
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
boolean
nal_reader_get_se (NalReader * reader, int32 * val)
{
  uint32 value;

  if (!nal_reader_get_ue (reader, &value))
    return FALSE;

  if (value % 2)
    *val = (value / 2) + 1;
  else
    *val = -(value / 2);

  return TRUE;
}

/**
 * nal_reader_peek_se:
 * @reader: a #NalReader instance
 * @val: Pointer to a #int32 to store the result
 *
 * Read a signed Exp-Golomb value into val but keep the current position
 *
 * Returns: %TRUE if successful, %FALSE otherwise.
 */
boolean
nal_reader_peek_se (const NalReader * reader, int32 * val)
{
  NalReader tmp;

  RETURN_VAL_IF_FAIL (reader != NULL, FALSE);

  tmp = *reader;
  return nal_reader_get_se (&tmp, val);
}
