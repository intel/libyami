/* Gstreamer
 * Copyright (C) <2011> Intel Corporation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
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

#include "parserutils.h"
#include "log.h"

boolean
decode_vlc (BitReader * br, uint32 * res, const VLCTable * table,
    uint32 length)
{
  uint8 i;
  uint32 cbits = 0;
  uint32 value = 0;

  for (i = 0; i < length; i++) {
    if (cbits != table[i].cbits) {
      cbits = table[i].cbits;
      if (!bit_reader_peek_bits_uint32 (br, &value, cbits)) {
        goto failed;
      }
    }

    if (value == table[i].cword) {
      bit_reader_skip (br, cbits);
      if (res)
        *res = table[i].value;

      return TRUE;
    }
  }

  LOG_DEBUG ("Did not find code");

failed:
  {
    LOG_WARNING ("Could not decode VLC returning");

    return FALSE;
  }
}

inline uint32 
bit_storage_calculate(uint32 value)
{
  uint32 bits = 0;
  while (value >> bits)
     bits ++;
  
  return bits;
}


