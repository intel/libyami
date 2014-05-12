/*
 *  jpegparser.c - JPEG parser
 *
 *  Copyright (C) 2011-2012 Intel Corporation
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

#include <string.h>
#include <stdlib.h>
#include "bytereader.h"
#include "bitreader.h"
#include "jpegparser.h"

#define READ_UINT8(reader, val) {                           \
    if (!byte_reader_get_uint8 ((reader), &(val))) {        \
      LOG_WARNING ("failed to read uint8_t");                 \
      goto failed;                                          \
    }                                                       \
  }

#define READ_UINT16(reader, val)  {                         \
    if (!byte_reader_get_uint16_be ((reader), &(val))) {    \
      LOG_WARNING ("failed to read uint16_t");                \
      goto failed;                                          \
    }                                                       \
  }

#define READ_BYTES(reader, buf, length)  {                  \
    const uint8_t *vals;                                      \
    if (!byte_reader_get_data (reader, length, &vals)) {    \
      LOG_WARNING ("failed to read bytes, size:%d", length); \
      goto failed;                                          \
    }                                                       \
    memcpy (buf, vals, length);                             \
  }

#define U_READ_UINT8(reader, val)  {                        \
    (val) = byte_reader_get_uint8_unchecked(reader);        \
  }

#define U_READ_UINT16(reader, val) {                        \
    (val) = byte_reader_get_uint16_be_unchecked(reader);    \
  }

/* Table used to address an 8x8 matrix in zig-zag order */
/* *INDENT-OFF* */
static const uint8_t zigzag_index[64] = {
  0,   1,  8, 16,  9,  2,  3, 10,
  17, 24, 32, 25, 18, 11,  4,  5,
  12, 19, 26, 33, 40, 48, 41, 34,
  27, 20, 13,  6,  7, 14, 21, 28,
  35, 42, 49, 56, 57, 50, 43, 36,
  29, 22, 15, 23, 30, 37, 44, 51,
  58, 59, 52, 45, 38, 31, 39, 46,
  53, 60, 61, 54, 47, 55, 62, 63
};
/* *INDENT-ON* */

/* Table K.1 - Luminance quantization table */
/* *INDENT-OFF* */
static const uint8_t default_luminance_quant_table[64] = {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};
/* *INDENT-ON* */

/* Table K.2 - Chrominance quantization table */
/* *INDENT-OFF* */
static const uint8_t default_chrominance_quant_table[64] = {
  17,  18,  24,  47,  99,  99,  99,  99,
  18,  21,  26,  66,  99,  99,  99,  99,
  24,  26,  56,  99,  99,  99,  99,  99,
  47,  66,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};
/* *INDENT-ON* */

typedef struct _JpegHuffmanTableEntry JpegHuffmanTableEntry;
struct _JpegHuffmanTableEntry
{
  uint8_t value;                 /* category */
  uint8_t length;                /* code length in bits */
};

/* Table K.3 - Table for luminance DC coefficient differences */
static const JpegHuffmanTableEntry default_luminance_dc_table[] = {
  {0x00, 2}, {0x01, 3}, {0x02, 3}, {0x03, 3}, {0x04, 3}, {0x05, 3},
  {0x06, 4}, {0x07, 5}, {0x08, 6}, {0x09, 7}, {0x0a, 8}, {0x0b, 9}
};

/* Table K.4 - Table for chrominance DC coefficient differences */
static const JpegHuffmanTableEntry default_chrominance_dc_table[] = {
  {0x00, 2}, {0x01, 2}, {0x02, 2}, {0x03, 3}, {0x04, 4}, {0x05, 5},
  {0x06, 6}, {0x07, 7}, {0x08, 8}, {0x09, 9}, {0x0a, 10}, {0x0b, 11}
};

/* Table K.5 - Table for luminance AC coefficients */
/* *INDENT-OFF* */
static const JpegHuffmanTableEntry default_luminance_ac_table[] = {
  {0x00,  4}, {0x01,  2}, {0x02,  2}, {0x03,  3}, {0x04,  4}, {0x05,  5},
  {0x06,  7}, {0x07,  8}, {0x08, 10}, {0x09, 16}, {0x0a, 16}, {0x11,  4},
  {0x12,  5}, {0x13,  7}, {0x14,  9}, {0x15, 11}, {0x16, 16}, {0x17, 16},
  {0x18, 16}, {0x19, 16}, {0x1a, 16}, {0x21,  5}, {0x22,  8}, {0x23, 10},
  {0x24, 12}, {0x25, 16}, {0x26, 16}, {0x27, 16}, {0x28, 16}, {0x29, 16},
  {0x2a, 16}, {0x31,  6}, {0x32,  9}, {0x33, 12}, {0x34, 16}, {0x35, 16},
  {0x36, 16}, {0x37, 16}, {0x38, 16}, {0x39, 16}, {0x3a, 16}, {0x41,  6},
  {0x42, 10}, {0x43, 16}, {0x44, 16}, {0x45, 16}, {0x46, 16}, {0x47, 16},
  {0x48, 16}, {0x49, 16}, {0x4a, 16}, {0x51,  7}, {0x52, 11}, {0x53, 16},
  {0x54, 16}, {0x55, 16}, {0x56, 16}, {0x57, 16}, {0x58, 16}, {0x59, 16},
  {0x5a, 16}, {0x61,  7}, {0x62, 12}, {0x63, 16}, {0x64, 16}, {0x65, 16},
  {0x66, 16}, {0x67, 16}, {0x68, 16}, {0x69, 16}, {0x6a, 16}, {0x71,  8},
  {0x72, 12}, {0x73, 16}, {0x74, 16}, {0x75, 16}, {0x76, 16}, {0x77, 16},
  {0x78, 16}, {0x79, 16}, {0x7a, 16}, {0x81,  9}, {0x82, 15}, {0x83, 16},
  {0x84, 16}, {0x85, 16}, {0x86, 16}, {0x87, 16}, {0x88, 16}, {0x89, 16},
  {0x8a, 16}, {0x91,  9}, {0x92, 16}, {0x93, 16}, {0x94, 16}, {0x95, 16},
  {0x96, 16}, {0x97, 16}, {0x98, 16}, {0x99, 16}, {0x9a, 16}, {0xa1,  9},
  {0xa2, 16}, {0xa3, 16}, {0xa4, 16}, {0xa5, 16}, {0xa6, 16}, {0xa7, 16},
  {0xa8, 16}, {0xa9, 16}, {0xaa, 16}, {0xb1, 10}, {0xb2, 16}, {0xb3, 16},
  {0xb4, 16}, {0xb5, 16}, {0xb6, 16}, {0xb7, 16}, {0xb8, 16}, {0xb9, 16},
  {0xba, 16}, {0xc1, 10}, {0xc2, 16}, {0xc3, 16}, {0xc4, 16}, {0xc5, 16},
  {0xc6, 16}, {0xc7, 16}, {0xc8, 16}, {0xc9, 16}, {0xca, 16}, {0xd1, 11},
  {0xd2, 16}, {0xd3, 16}, {0xd4, 16}, {0xd5, 16}, {0xd6, 16}, {0xd7, 16},
  {0xd8, 16}, {0xd9, 16}, {0xda, 16}, {0xe1, 16}, {0xe2, 16}, {0xe3, 16},
  {0xe4, 16}, {0xe5, 16}, {0xe6, 16}, {0xe7, 16}, {0xe8, 16}, {0xe9, 16},
  {0xea, 16}, {0xf0, 11}, {0xf1, 16}, {0xf2, 16}, {0xf3, 16}, {0xf4, 16},
  {0xf5, 16}, {0xf6, 16}, {0xf7, 16}, {0xf8, 16}, {0xf9, 16}, {0xfa, 16}
};
/* *INDENT-ON* */

/* Table K.6 - Table for chrominance AC coefficients */
/* *INDENT-OFF* */
static const JpegHuffmanTableEntry default_chrominance_ac_table[] = {
  {0x00,  2}, {0x01,  2}, {0x02,  3}, {0x03,  4}, {0x04,  5}, {0x05,  5},
  {0x06,  6}, {0x07,  7}, {0x08,  9}, {0x09, 10}, {0x0a, 12}, {0x11,  4},
  {0x12,  6}, {0x13,  8}, {0x14,  9}, {0x15, 11}, {0x16, 12}, {0x17, 16},
  {0x18, 16}, {0x19, 16}, {0x1a, 16}, {0x21,  5}, {0x22,  8}, {0x23, 10},
  {0x24, 12}, {0x25, 15}, {0x26, 16}, {0x27, 16}, {0x28, 16}, {0x29, 16},
  {0x2a, 16}, {0x31,  5}, {0x32,  8}, {0x33, 10}, {0x34, 12}, {0x35, 16},
  {0x36, 16}, {0x37, 16}, {0x38, 16}, {0x39, 16}, {0x3a, 16}, {0x41,  6},
  {0x42,  9}, {0x43, 16}, {0x44, 16}, {0x45, 16}, {0x46, 16}, {0x47, 16},
  {0x48, 16}, {0x49, 16}, {0x4a, 16}, {0x51,  6}, {0x52, 10}, {0x53, 16},
  {0x54, 16}, {0x55, 16}, {0x56, 16}, {0x57, 16}, {0x58, 16}, {0x59, 16},
  {0x5a, 16}, {0x61,  7}, {0x62, 11}, {0x63, 16}, {0x64, 16}, {0x65, 16},
  {0x66, 16}, {0x67, 16}, {0x68, 16}, {0x69, 16}, {0x6a, 16}, {0x71,  7},
  {0x72, 11}, {0x73, 16}, {0x74, 16}, {0x75, 16}, {0x76, 16}, {0x77, 16},
  {0x78, 16}, {0x79, 16}, {0x7a, 16}, {0x81,  8}, {0x82, 16}, {0x83, 16},
  {0x84, 16}, {0x85, 16}, {0x86, 16}, {0x87, 16}, {0x88, 16}, {0x89, 16},
  {0x8a, 16}, {0x91,  9}, {0x92, 16}, {0x93, 16}, {0x94, 16}, {0x95, 16},
  {0x96, 16}, {0x97, 16}, {0x98, 16}, {0x99, 16}, {0x9a, 16}, {0xa1,  9},
  {0xa2, 16}, {0xa3, 16}, {0xa4, 16}, {0xa5, 16}, {0xa6, 16}, {0xa7, 16},
  {0xa8, 16}, {0xa9, 16}, {0xaa, 16}, {0xb1,  9}, {0xb2, 16}, {0xb3, 16},
  {0xb4, 16}, {0xb5, 16}, {0xb6, 16}, {0xb7, 16}, {0xb8, 16}, {0xb9, 16},
  {0xba, 16}, {0xc1,  9}, {0xc2, 16}, {0xc3, 16}, {0xc4, 16}, {0xc5, 16},
  {0xc6, 16}, {0xc7, 16}, {0xc8, 16}, {0xc9, 16}, {0xca, 16}, {0xd1, 11},
  {0xd2, 16}, {0xd3, 16}, {0xd4, 16}, {0xd5, 16}, {0xd6, 16}, {0xd7, 16},
  {0xd8, 16}, {0xd9, 16}, {0xda, 16}, {0xe1, 14}, {0xe2, 16}, {0xe3, 16},
  {0xe4, 16}, {0xe5, 16}, {0xe6, 16}, {0xe7, 16}, {0xe8, 16}, {0xe9, 16},
  {0xea, 16}, {0xf0, 10}, {0xf1, 15}, {0xf2, 16}, {0xf3, 16}, {0xf4, 16},
  {0xf5, 16}, {0xf6, 16}, {0xf7, 16}, {0xf8, 16}, {0xf9, 16}, {0xfa, 16}
};
/* *INDENT-ON* */

static inline bool
jpeg_parse_to_next_marker (ByteReader * br, uint8_t * marker)
{
  int32_t ofs;

  ofs = jpeg_scan_for_marker_code (br->data, br->size, br->byte);
  if (ofs < 0)
    return false;

  if (marker)
    *marker = br->data[ofs + 1];
  byte_reader_skip_unchecked (br, ofs - br->byte);
  return true;
}

int32_t
jpeg_scan_for_marker_code (const uint8_t * data, size_t size, uint32_t offset)
{
  uint32_t i;

  RETURN_VAL_IF_FAIL (data != NULL, -1);

  i = offset + 1;
  while (i < size) {
    const uint8_t v = data[i];
    if (v < 0xc0)
      i += 2;
    else if (v < 0xff && data[i - 1] == 0xff)
      return i - 1;
    else
      i++;
  }
  return -1;
}

bool
jpeg_parse_frame_hdr (JpegFrameHdr * frame_hdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;
  uint8_t val;
  uint32_t i;

  RETURN_VAL_IF_FAIL (frame_hdr != NULL, false);
  RETURN_VAL_IF_FAIL (data != NULL, false);
  RETURN_VAL_IF_FAIL (size > offset, false);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 8, false);

  U_READ_UINT16 (&br, length);  /* Lf */
  RETURN_VAL_IF_FAIL (size >= length, false);

  U_READ_UINT8 (&br, frame_hdr->sample_precision);
  U_READ_UINT16 (&br, frame_hdr->height);
  U_READ_UINT16 (&br, frame_hdr->width);
  U_READ_UINT8 (&br, frame_hdr->num_components);
  RETURN_VAL_IF_FAIL (frame_hdr->num_components <=
      JPEG_MAX_SCAN_COMPONENTS, false);

  length -= 8;
  RETURN_VAL_IF_FAIL (length >= 3 * frame_hdr->num_components, false);
  for (i = 0; i < frame_hdr->num_components; i++) {
    U_READ_UINT8 (&br, frame_hdr->components[i].identifier);
    U_READ_UINT8 (&br, val);
    frame_hdr->components[i].horizontal_factor = (val >> 4) & 0x0F;
    frame_hdr->components[i].vertical_factor = (val & 0x0F);
    U_READ_UINT8 (&br, frame_hdr->components[i].quant_table_selector);
    RETURN_VAL_IF_FAIL ((frame_hdr->components[i].horizontal_factor <= 4 &&
            frame_hdr->components[i].vertical_factor <= 4 &&
            frame_hdr->components[i].quant_table_selector < 4), false);
    length -= 3;
  }

  assert (length == 0);
  return true;
}

bool
jpeg_parse_scan_hdr (JpegScanHdr * scan_hdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;
  uint8_t val;
  uint32_t i;

  RETURN_VAL_IF_FAIL (scan_hdr != NULL, false);
  RETURN_VAL_IF_FAIL (data != NULL, false);
  RETURN_VAL_IF_FAIL (size > offset, false);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 3, false);

  U_READ_UINT16 (&br, length);  /* Ls */
  RETURN_VAL_IF_FAIL (size >= length, false);

  U_READ_UINT8 (&br, scan_hdr->num_components);
  RETURN_VAL_IF_FAIL (scan_hdr->num_components <=
      JPEG_MAX_SCAN_COMPONENTS, false);

  length -= 3;
  RETURN_VAL_IF_FAIL (length >= 2 * scan_hdr->num_components, false);
  for (i = 0; i < scan_hdr->num_components; i++) {
    U_READ_UINT8 (&br, scan_hdr->components[i].component_selector);
    U_READ_UINT8 (&br, val);
    scan_hdr->components[i].dc_selector = (val >> 4) & 0x0F;
    scan_hdr->components[i].ac_selector = val & 0x0F;
    RETURN_VAL_IF_FAIL ((scan_hdr->components[i].dc_selector < 4 &&
            scan_hdr->components[i].ac_selector < 4), false);
    length -= 2;
  }

  /* FIXME: Ss, Se, Ah, Al */
  assert (length == 3);
  return true;
}

bool
jpeg_parse_huffman_table (JpegHuffmanTables * huf_tables,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  JpegHuffmanTable *huf_table;
  uint16_t length;
  uint8_t val, table_class, table_index;
  uint32_t value_count;
  uint32_t i;

  RETURN_VAL_IF_FAIL (huf_tables != NULL, false);
  RETURN_VAL_IF_FAIL (data != NULL, false);
  RETURN_VAL_IF_FAIL (size > offset, false);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 2, false);

  U_READ_UINT16 (&br, length);  /* Lh */
  RETURN_VAL_IF_FAIL (size >= length, false);

  while (byte_reader_get_remaining (&br)) {
    U_READ_UINT8 (&br, val);
    table_class = ((val >> 4) & 0x0F);
    table_index = (val & 0x0F);
    RETURN_VAL_IF_FAIL (table_index < JPEG_MAX_SCAN_COMPONENTS, false);
    if (table_class == 0) {
      huf_table = &huf_tables->dc_tables[table_index];
    } else {
      huf_table = &huf_tables->ac_tables[table_index];
    }
    READ_BYTES (&br, huf_table->huf_bits, 16);
    value_count = 0;
    for (i = 0; i < 16; i++)
      value_count += huf_table->huf_bits[i];
    READ_BYTES (&br, huf_table->huf_values, value_count);
    huf_table->valid = true;
  }
  return true;

failed:
  return false;
}

bool
jpeg_parse_quant_table (JpegQuantTables * quant_tables,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  JpegQuantTable *quant_table;
  uint16_t length;
  uint8_t val, table_index;
  uint32_t i;

  RETURN_VAL_IF_FAIL (quant_tables != NULL, false);
  RETURN_VAL_IF_FAIL (data != NULL, false);
  RETURN_VAL_IF_FAIL (size > offset, false);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 2, false);

  U_READ_UINT16 (&br, length);  /* Lq */
  RETURN_VAL_IF_FAIL (size >= length, false);

  while (byte_reader_get_remaining (&br)) {
    U_READ_UINT8 (&br, val);
    table_index = (val & 0x0f);
    RETURN_VAL_IF_FAIL (table_index < JPEG_MAX_SCAN_COMPONENTS, false);
    quant_table = &quant_tables->quant_tables[table_index];
    quant_table->quant_precision = ((val >> 4) & 0x0f);

    RETURN_VAL_IF_FAIL (byte_reader_get_remaining (&br) >=
        JPEG_MAX_QUANT_ELEMENTS * (1 + ! !quant_table->quant_precision),
        false);
    for (i = 0; i < JPEG_MAX_QUANT_ELEMENTS; i++) {
      if (!quant_table->quant_precision) {      /* 8-bit values */
        U_READ_UINT8 (&br, val);
        quant_table->quant_table[i] = val;
      } else {                  /* 16-bit values */
        U_READ_UINT16 (&br, quant_table->quant_table[i]);
      }
    }
    quant_table->valid = true;
  }
  return true;
}

bool
jpeg_parse_restart_interval (uint32_t * interval,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length, val;

  RETURN_VAL_IF_FAIL (interval != NULL, false);
  RETURN_VAL_IF_FAIL (data != NULL, false);
  RETURN_VAL_IF_FAIL (size > offset, false);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 4, false);

  U_READ_UINT16 (&br, length);  /* Lr */
  RETURN_VAL_IF_FAIL (size >= length, false);

  U_READ_UINT16 (&br, val);
  *interval = val;
  return true;
}

static int
compare_huffman_table_entry (const void *a, const void *b)
{
  const JpegHuffmanTableEntry *const e1 = *(JpegHuffmanTableEntry **) a;
  const JpegHuffmanTableEntry *const e2 = *(JpegHuffmanTableEntry **) b;

  if (e1->length == e2->length)
    return (int32_t) e1->value - (int32_t) e2->value;
  return (int32_t) e1->length - (int32_t) e2->length;
}

static void
build_huffman_table (JpegHuffmanTable * huf_table,
    const JpegHuffmanTableEntry * entries, uint32_t num_entries)
{
  const JpegHuffmanTableEntry *sorted_entries[256];
  uint32_t i, j, n;

  assert (num_entries <= N_ELEMENTS (sorted_entries));

  for (i = 0; i < num_entries; i++)
    sorted_entries[i] = &entries[i];
  qsort (sorted_entries, num_entries, sizeof (sorted_entries[0]),
      compare_huffman_table_entry);

  for (i = 0, j = 1, n = 0; i < num_entries; i++) {
    const JpegHuffmanTableEntry *const e = sorted_entries[i];
    if (e->length != j) {
      huf_table->huf_bits[j++ - 1] = n;
      for (; j < e->length; j++)
        huf_table->huf_bits[j - 1] = 0;
      n = 0;
    }
    huf_table->huf_values[i] = e->value;
    n++;
  }
  huf_table->huf_bits[j - 1] = n;

  for (; j < N_ELEMENTS (huf_table->huf_bits); j++)
    huf_table->huf_bits[j] = 0;
  for (; i < N_ELEMENTS (huf_table->huf_values); i++)
    huf_table->huf_values[i] = 0;
  huf_table->valid = true;
}

void
jpeg_get_default_huffman_tables (JpegHuffmanTables * huf_tables)
{
  assert (huf_tables);

  /* Build DC tables */
  build_huffman_table (&huf_tables->dc_tables[0], default_luminance_dc_table,
      N_ELEMENTS (default_luminance_dc_table));
  build_huffman_table (&huf_tables->dc_tables[1], default_chrominance_dc_table,
      N_ELEMENTS (default_chrominance_dc_table));
  memcpy (&huf_tables->dc_tables[2], &huf_tables->dc_tables[1],
      sizeof (huf_tables->dc_tables[2]));

  /* Build AC tables */
  build_huffman_table (&huf_tables->ac_tables[0], default_luminance_ac_table,
      N_ELEMENTS (default_luminance_ac_table));
  build_huffman_table (&huf_tables->ac_tables[1], default_chrominance_ac_table,
      N_ELEMENTS (default_chrominance_ac_table));
  memcpy (&huf_tables->ac_tables[2], &huf_tables->ac_tables[1],
      sizeof (huf_tables->ac_tables[2]));
}

static void
build_quant_table (JpegQuantTable * quant_table, const uint8_t values[64])
{
  uint32_t i;

  for (i = 0; i < 64; i++)
    quant_table->quant_table[i] = values[zigzag_index[i]];
  quant_table->quant_precision = 0;     /* Pq = 0 (8-bit precision) */
  quant_table->valid = true;
}

void
jpeg_get_default_quantization_tables (JpegQuantTables * quant_tables)
{
  assert (quant_tables);

  build_quant_table (&quant_tables->quant_tables[0],
      default_luminance_quant_table);
  build_quant_table (&quant_tables->quant_tables[1],
      default_chrominance_quant_table);
  build_quant_table (&quant_tables->quant_tables[2],
      default_chrominance_quant_table);
}

bool
jpeg_parse (JpegMarkerSegment * seg,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;

  RETURN_VAL_IF_FAIL (seg != NULL, false);

  if (size <= offset) {
    LOG_DEBUG ("failed to parse from offset %u, buffer is too small", offset);
    return false;
  }

  size -= offset;
  byte_reader_init (&br, &data[offset], size);

  if (!jpeg_parse_to_next_marker (&br, &seg->marker)) {
    LOG_DEBUG ("failed to find marker code");
    return false;
  }

  byte_reader_skip_unchecked (&br, 2);
  seg->offset = offset + byte_reader_get_pos (&br);
  seg->size = -1;

  /* Try to find end of segment */
  switch (seg->marker) {
    case JPEG_MARKER_SOI:
    case JPEG_MARKER_EOI:
    fixed_size_segment:
      seg->size = 0;
      break;

    case (JPEG_MARKER_SOF_MIN + 0):        /* Lf */
    case (JPEG_MARKER_SOF_MIN + 1):        /* Lf */
    case (JPEG_MARKER_SOF_MIN + 2):        /* Lf */
    case (JPEG_MARKER_SOF_MIN + 3):        /* Lf */
    case (JPEG_MARKER_SOF_MIN + 9):        /* Lf */
    case (JPEG_MARKER_SOF_MIN + 10):       /* Lf */
    case (JPEG_MARKER_SOF_MIN + 11):       /* Lf */
    case JPEG_MARKER_SOS:  /* Ls */
    case JPEG_MARKER_DQT:  /* Lq */
    case JPEG_MARKER_DHT:  /* Lh */
    case JPEG_MARKER_DAC:  /* La */
    case JPEG_MARKER_DRI:  /* Lr */
    case JPEG_MARKER_COM:  /* Lc */
    case JPEG_MARKER_DNL:  /* Ld */
    variable_size_segment:
      READ_UINT16 (&br, length);
      seg->size = length;
      break;

    default:
      /* Application data segment length (Lp) */
      if (seg->marker >= JPEG_MARKER_APP_MIN &&
          seg->marker <= JPEG_MARKER_APP_MAX)
        goto variable_size_segment;

      /* Restart markers (fixed size, two bytes only) */
      if (seg->marker >= JPEG_MARKER_RST_MIN &&
          seg->marker <= JPEG_MARKER_RST_MAX)
        goto fixed_size_segment;

      /* Fallback: scan for next marker */
      if (!jpeg_parse_to_next_marker (&br, NULL))
        goto failed;
      seg->size = byte_reader_get_pos (&br) - seg->offset;
      break;
  }
  return true;

failed:
  return false;
}
