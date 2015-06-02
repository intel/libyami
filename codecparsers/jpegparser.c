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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "bytereader.h"
#include "bitreader.h"
#include "jpegparser.h"

#define READ_UINT8(reader, val) {                           \
    if (!byte_reader_get_uint8 ((reader), &(val))) {        \
      WARNING ("failed to read uint8_t");                 \
      goto failed;                                          \
    }                                                       \
  }

#define READ_UINT16(reader, val)  {                         \
    if (!byte_reader_get_uint16_be ((reader), &(val))) {    \
      WARNING ("failed to read uint16_t");                \
      goto failed;                                          \
    }                                                       \
  }

#define READ_BYTES(reader, buf, length)  {                  \
    const uint8_t *vals;                                      \
    if (!byte_reader_get_data (reader, length, &vals)) {    \
      WARNING ("failed to read bytes, size:%d", length); \
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

static inline BOOL
jpeg_parse_to_next_marker (ByteReader * br, uint8_t * marker)
{
  int32_t ofs;

  ofs = jpeg_scan_for_marker_code (br->data, br->size, br->byte);
  if (ofs < 0)
    return FALSE;

  if (marker)
    *marker = br->data[ofs + 1];
  byte_reader_skip_unchecked (br, ofs - br->byte);
  return TRUE;
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

BOOL
jpeg_parse_frame_hdr (JpegFrameHdr * frame_hdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;
  uint8_t val;
  uint32_t i;

  RETURN_VAL_IF_FAIL (frame_hdr != NULL, FALSE);
  RETURN_VAL_IF_FAIL (data != NULL, FALSE);
  RETURN_VAL_IF_FAIL (size > offset, FALSE);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 8, FALSE);

  U_READ_UINT16 (&br, length);  /* Lf */
  RETURN_VAL_IF_FAIL (size >= length, FALSE);

  U_READ_UINT8 (&br, frame_hdr->sample_precision);
  U_READ_UINT16 (&br, frame_hdr->height);
  U_READ_UINT16 (&br, frame_hdr->width);
  U_READ_UINT8 (&br, frame_hdr->num_components);
  RETURN_VAL_IF_FAIL (frame_hdr->num_components <=
      JPEG_MAX_SCAN_COMPONENTS, FALSE);

  length -= 8;
  RETURN_VAL_IF_FAIL (length >= 3 * frame_hdr->num_components, FALSE);
  for (i = 0; i < frame_hdr->num_components; i++) {
    U_READ_UINT8 (&br, frame_hdr->components[i].identifier);
    U_READ_UINT8 (&br, val);
    frame_hdr->components[i].horizontal_factor = (val >> 4) & 0x0F;
    frame_hdr->components[i].vertical_factor = (val & 0x0F);
    U_READ_UINT8 (&br, frame_hdr->components[i].quant_table_selector);
    RETURN_VAL_IF_FAIL ((frame_hdr->components[i].horizontal_factor <= 4 &&
            frame_hdr->components[i].vertical_factor <= 4 &&
            frame_hdr->components[i].quant_table_selector < 4), FALSE);
    length -= 3;
  }

  assert (length == 0);
  return TRUE;
}

BOOL
jpeg_parse_scan_hdr (JpegScanHdr * scan_hdr,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;
  uint8_t val;
  uint32_t i;

  RETURN_VAL_IF_FAIL (scan_hdr != NULL, FALSE);
  RETURN_VAL_IF_FAIL (data != NULL, FALSE);
  RETURN_VAL_IF_FAIL (size > offset, FALSE);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 3, FALSE);

  U_READ_UINT16 (&br, length);  /* Ls */
  RETURN_VAL_IF_FAIL (size >= length, FALSE);

  U_READ_UINT8 (&br, scan_hdr->num_components);
  RETURN_VAL_IF_FAIL (scan_hdr->num_components <=
      JPEG_MAX_SCAN_COMPONENTS, FALSE);

  length -= 3;
  RETURN_VAL_IF_FAIL (length >= 2 * scan_hdr->num_components, FALSE);
  for (i = 0; i < scan_hdr->num_components; i++) {
    U_READ_UINT8 (&br, scan_hdr->components[i].component_selector);
    U_READ_UINT8 (&br, val);
    scan_hdr->components[i].dc_selector = (val >> 4) & 0x0F;
    scan_hdr->components[i].ac_selector = val & 0x0F;
    RETURN_VAL_IF_FAIL ((scan_hdr->components[i].dc_selector < 4 &&
            scan_hdr->components[i].ac_selector < 4), FALSE);
    length -= 2;
  }

  /* FIXME: Ss, Se, Ah, Al */
  assert (length == 3);
  return TRUE;
}

BOOL
jpeg_parse_huffman_table (JpegHuffmanTables * huf_tables,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  JpegHuffmanTable *huf_table;
  uint16_t length;
  uint8_t val, table_class, table_index;
  uint32_t value_count;
  uint32_t i;

  RETURN_VAL_IF_FAIL (huf_tables != NULL, FALSE);
  RETURN_VAL_IF_FAIL (data != NULL, FALSE);
  RETURN_VAL_IF_FAIL (size > offset, FALSE);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 2, FALSE);

  U_READ_UINT16 (&br, length);  /* Lh */
  RETURN_VAL_IF_FAIL (size >= length, FALSE);

  while (byte_reader_get_remaining (&br)) {
    U_READ_UINT8 (&br, val);
    table_class = ((val >> 4) & 0x0F);
    table_index = (val & 0x0F);
    RETURN_VAL_IF_FAIL (table_index < JPEG_MAX_SCAN_COMPONENTS, FALSE);
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
    huf_table->valid = TRUE;
  }
  return TRUE;

failed:
  return FALSE;
}

BOOL
jpeg_parse_quant_table (JpegQuantTables * quant_tables,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  JpegQuantTable *quant_table;
  uint16_t length;
  uint8_t val, table_index;
  uint32_t i;

  RETURN_VAL_IF_FAIL (quant_tables != NULL, FALSE);
  RETURN_VAL_IF_FAIL (data != NULL, FALSE);
  RETURN_VAL_IF_FAIL (size > offset, FALSE);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 2, FALSE);

  U_READ_UINT16 (&br, length);  /* Lq */
  RETURN_VAL_IF_FAIL (size >= length, FALSE);

  while (byte_reader_get_remaining (&br)) {
    U_READ_UINT8 (&br, val);
    table_index = (val & 0x0f);
    RETURN_VAL_IF_FAIL (table_index < JPEG_MAX_SCAN_COMPONENTS, FALSE);
    quant_table = &quant_tables->quant_tables[table_index];
    quant_table->quant_precision = ((val >> 4) & 0x0f);

    RETURN_VAL_IF_FAIL (byte_reader_get_remaining (&br) >=
        JPEG_MAX_QUANT_ELEMENTS * (1 + ! !quant_table->quant_precision),
        FALSE);
    for (i = 0; i < JPEG_MAX_QUANT_ELEMENTS; i++) {
      if (!quant_table->quant_precision) {      /* 8-bit values */
        U_READ_UINT8 (&br, val);
        quant_table->quant_table[i] = val;
      } else {                  /* 16-bit values */
        U_READ_UINT16 (&br, quant_table->quant_table[i]);
      }
    }
    quant_table->valid = TRUE;
  }
  return TRUE;
}

BOOL
jpeg_parse_restart_interval (uint32_t * interval,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length, val;

  RETURN_VAL_IF_FAIL (interval != NULL, FALSE);
  RETURN_VAL_IF_FAIL (data != NULL, FALSE);
  RETURN_VAL_IF_FAIL (size > offset, FALSE);

  size -= offset;
  byte_reader_init (&br, &data[offset], size);
  RETURN_VAL_IF_FAIL (size >= 4, FALSE);

  U_READ_UINT16 (&br, length);  /* Lr */
  RETURN_VAL_IF_FAIL (size >= length, FALSE);

  U_READ_UINT16 (&br, val);
  *interval = val;
  return TRUE;
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
  huf_table->valid = TRUE;
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
  quant_table->valid = TRUE;
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

BOOL
jpeg_parse (JpegMarkerSegment * seg,
    const uint8_t * data, size_t size, uint32_t offset)
{
  ByteReader br;
  uint16_t length;

  RETURN_VAL_IF_FAIL (seg != NULL, FALSE);

  if (size <= offset) {
    DEBUG ("failed to parse from offset %u, buffer is too small", offset);
    return FALSE;
  }

  size -= offset;
  byte_reader_init (&br, &data[offset], size);

  if (!jpeg_parse_to_next_marker (&br, &seg->marker)) {
    DEBUG ("failed to find marker code");
    return FALSE;
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
      /* Lf */
      if (seg->marker >= JPEG_MARKER_SOF_MIN &&
          seg->marker <= JPEG_MARKER_SOF_MIN + 11)
        goto variable_size_segment;

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
  return TRUE;

failed:
  return FALSE;
}
