/* reamer
 * Copyright (C) <2011> Intel Corporation
 * Copyright (C) <2011> Collabora Ltd.
 * Copyright (C) <2011> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * Some bits C-c,C-v'ed and s/4/3 from h264parse and videoparsers/h264parse.c:
 *    Copyright (C) <2010> Mark Nauwelaerts <mark.nauwelaerts@collabora.co.uk>
 *    Copyright (C) <2010> Collabora Multimedia
 *    Copyright (C) <2010> Nokia Corporation
 *
 *    (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *    (C) 2008 Wim Taymans <wim.taymans@gmail.com>
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

/**
 * Common code for NAL parsing from h264 and h265 parsers.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "bytereader.h"
#include "bitreader.h"
#include <string.h>

uint32_t ceil_log2 (uint32_t v);

typedef struct
{
  const uint8_t *data;
  uint32_t size;

  uint32_t n_epb;                  /* Number of emulation prevention bytes */
  uint32_t byte;                   /* Byte position */
  uint32_t bits_in_cache;          /* bitpos in the cache of next bit */
  uint8_t first_byte;
  uint64_t cache;                /* cached bytes */
} NalReader;

void nal_reader_init (NalReader * nr, const uint8_t * data, uint32_t size);

bool nal_reader_read (NalReader * nr, uint32_t nbits);
bool nal_reader_skip (NalReader * nr, uint32_t nbits);
bool nal_reader_skip_long (NalReader * nr, uint32_t nbits);
uint32_t nal_reader_get_pos (const NalReader * nr);
uint32_t nal_reader_get_remaining (const NalReader * nr);
uint32_t nal_reader_get_epb_count (const NalReader * nr);

bool nal_reader_is_byte_aligned (NalReader * nr);
bool nal_reader_has_more_data (NalReader * nr);

#define NAL_READER_READ_BITS_H(bits) \
bool nal_reader_get_bits_uint##bits (NalReader *nr, uint##bits##_t *val, uint32_t nbits)

NAL_READER_READ_BITS_H (8);
NAL_READER_READ_BITS_H (16);
NAL_READER_READ_BITS_H (32);

#define NAL_READER_PEEK_BITS_H(bits) \
bool nal_reader_peek_bits_uint##bits (const NalReader *nr, uint##bits##_t *val, uint32_t nbits)

NAL_READER_PEEK_BITS_H (8);

bool nal_reader_get_ue (NalReader * nr, uint32_t * val);
bool nal_reader_get_se (NalReader * nr, int32_t * val);

#define CHECK_ALLOWED_MAX(val, max) { \
  if (val > max) { \
    WARNING ("value greater than max. value: %d, max %d", \
                     val, max); \
    goto error; \
  } \
}

#define CHECK_ALLOWED(val, min, max) { \
  if (val < min || val > max) { \
    WARNING ("value not in allowed range. value: %d, range %d-%d", \
                     val, min, max); \
    goto error; \
  } \
}

#define READ_UINT8(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint8 (nr, &val, nbits)) { \
    WARNING ("failed to read uint8, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT16(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint16 (nr, &val, nbits)) { \
  WARNING ("failed to read uint16, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT32(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint32 (nr, &val, nbits)) { \
  WARNING ("failed to read uint32, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UINT64(nr, val, nbits) { \
  if (!nal_reader_get_bits_uint64 (nr, &val, nbits)) { \
    WARNING ("failed to read uint32, nbits: %d", nbits); \
    goto error; \
  } \
}

#define READ_UE(nr, val) { \
  if (!nal_reader_get_ue (nr, &val)) { \
    WARNING ("failed to read UE"); \
    goto error; \
  } \
}

#define READ_UE_ALLOWED(nr, val, min, max) { \
  uint32_t tmp; \
  READ_UE (nr, tmp); \
  CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

#define READ_UE_MAX(nr, val, max) { \
  uint32_t tmp; \
  READ_UE (nr, tmp); \
  CHECK_ALLOWED_MAX (tmp, max); \
  val = tmp; \
}

#define READ_SE(nr, val) { \
  if (!nal_reader_get_se (nr, &val)) { \
    WARNING ("failed to read SE"); \
    goto error; \
  } \
}

#define READ_SE_ALLOWED(nr, val, min, max) { \
  int32_t tmp; \
  READ_SE (nr, tmp); \
  CHECK_ALLOWED (tmp, min, max); \
  val = tmp; \
}

int32_t scan_for_start_codes (const uint8_t * data, uint32_t size);
