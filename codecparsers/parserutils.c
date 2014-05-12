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

#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "parserutils.h"
#include "log.h"

bool
decode_vlc (BitReader * br, uint32_t * res, const VLCTable * table,
    uint32_t length)
{
  uint8_t i;
  uint32_t cbits = 0;
  uint32_t value = 0;

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

      return true;
    }
  }

  LOG_DEBUG ("Did not find code");

failed:
  {
    LOG_WARNING ("Could not decode VLC returning");

    return false;
  }
}

inline uint32_t 
bit_storage_calculate(uint32_t value)
{
  uint32_t bits = 0;
  while (value >> bits)
     bits ++;
  
  return bits;
}


