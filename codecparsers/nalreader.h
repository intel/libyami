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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __NAL_READER_H__
#define __NAL_READER_H__

#include "basictype.h"
#include "log.h"

typedef struct _NalReader
{
  const uint8 *data;
  uint32 size;
 
  uint32 n_epb;              /* number of emulatio preventio bytes */
  uint32 byte;               /* Byte position */
  uint32 bits_in_cache;      /* bitpos in the cache of next bit */
  uint8  first_byte;
  uint64 cache;              /* cached bytes */
} NalReader;

NalReader *nal_reader_new (const uint8 *data, uint32 size);
void nal_reader_init (NalReader * reader, const uint8 * data, uint32 size);
void nal_reader_free (NalReader * reader);

boolean nal_reader_skip (NalReader *reader, uint32 nbits);
boolean nal_reader_skip_to_byte (NalReader *reader);

uint32 nal_reader_get_pos (const NalReader * reader);
uint32 nal_reader_get_remaining (const NalReader * reader);
uint32 nal_reader_get_epb_count (const NalReader * reader);

boolean nal_reader_get_bits_uint8 (NalReader *reader, uint8 *val, uint32 nbits);
boolean nal_reader_get_bits_uint16 (NalReader *reader, uint16 *val, uint32 nbits);
boolean nal_reader_get_bits_uint32 (NalReader *reader, uint32 *val, uint32 nbits);
boolean nal_reader_get_bits_uint64 (NalReader *reader, uint64 *val, uint32 nbits);

boolean nal_reader_peek_bits_uint8 (const NalReader *reader, uint8 *val, uint32 nbits);
boolean nal_reader_peek_bits_uint16 (const NalReader *reader, uint16 *val, uint32 nbits);
boolean nal_reader_peek_bits_uint32 (const NalReader *reader, uint32 *val, uint32 nbits);
boolean nal_reader_peek_bits_uint64 (const NalReader *reader, uint64 *val, uint32 nbits);

boolean nal_reader_get_ue (NalReader *reader, uint32 *val);
boolean nal_reader_peek_ue (const NalReader *reader, uint32 *val);

boolean nal_reader_get_se (NalReader *reader, int32 *val);
boolean nal_reader_peek_se (const NalReader *reader, int32 *val);

/**
 * NAL_READER_INIT:
 * @data: Data from which the #NalReader should read
 * @size: Size of @data in bytes
 *
 * A #NalReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * bit_reader_init().
 *
 * Since: 0.10.22
 */
#define NAL_READER_INIT(data, size) {data, size, 0, 0, 0xff, 0xff}

/**
 * NAL_READER_INIT_FROM_BUFFER:
 * @buffer: Buffer from which the #NalReader should read
 *
 * A #NalReader must be initialized with this macro, before it can be
 * used. This macro can used be to initialize a variable, but it cannot
 * be assigned to a variable. In that case you have to use
 * bit_reader_init().
 *
 * Since: 0.10.22
 */
#define NAL_READER_INIT_FROM_BUFFER(buffer) {BUFFER_DATA (buffer), BUFFER_SIZE (buffer), 0, 0, 0xff, 0xff}

#endif /* __NAL_READER_H__ */
