/*
 *  jpegparser.h - JPEG parser
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

#ifndef __JPEG_PARSER_H
#define __JPEG_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include "common/common_def.h"

/**
 * JPEG_MAX_FRAME_COMPONENTS:
 *
 * Maximum number of image components in a frame (Nf).
 */

#define JPEG_MAX_FRAME_COMPONENTS   256

/**
 * JPEG_MAX_SCAN_COMPONENTS:
 *
 * Maximum number of image components in a scan (Ns).
 */
#define JPEG_MAX_SCAN_COMPONENTS    4

/**
 * JPEG_MAX_QUANT_ELEMENTS:
 *
 * Number of elements in the quantization table.
 */
#define JPEG_MAX_QUANT_ELEMENTS     64

typedef struct _JpegQuantTable       JpegQuantTable;
typedef struct _JpegQuantTables      JpegQuantTables;
typedef struct _JpegHuffmanTable     JpegHuffmanTable;
typedef struct _JpegHuffmanTables    JpegHuffmanTables;
typedef struct _JpegScanComponent    JpegScanComponent;
typedef struct _JpegScanHdr          JpegScanHdr;
typedef struct _JpegFrameComponent   JpegFrameComponent;
typedef struct _JpegFrameHdr         JpegFrameHdr;
typedef struct _JpegMarkerSegment    JpegMarkerSegment;

/**
 * JpegMarkerCode:
 * @JPEG_MARKER_SOF_MIN: Start of frame min marker code
 * @JPEG_MARKER_SOF_MAX: Start of frame max marker code
 * @JPEG_MARKER_DHT: Huffman tabler marker code
 * @JPEG_MARKER_DAC: Arithmetic coding marker code
 * @JPEG_MARKER_RST_MIN: Restart interval min marker code
 * @JPEG_MARKER_RST_MAX: Restart interval max marker code
 * @JPEG_MARKER_SOI: Start of image marker code
 * @JPEG_MARKER_EOI: End of image marker code
 * @JPEG_MARKER_SOS: Start of scan marker code
 * @JPEG_MARKER_DQT: Define quantization table marker code
 * @JPEG_MARKER_DNL: Define number of lines marker code
 * @JPEG_MARKER_DRI: Define restart interval marker code
 * @JPEG_MARKER_APP_MIN: Application segment min marker code
 * @JPEG_MARKER_APP_MAX: Application segment max marker code
 * @JPEG_MARKER_COM: Comment marker code
 *
 * Indicates the type of JPEG segment.
 */
typedef enum {
  JPEG_MARKER_SOF_MIN       = 0xC0,
  JPEG_MARKER_SOF_MAX       = 0xCF,
  JPEG_MARKER_DHT           = 0xC4,
  JPEG_MARKER_DAC           = 0xCC,
  JPEG_MARKER_RST_MIN       = 0xD0,
  JPEG_MARKER_RST_MAX       = 0xD7,
  JPEG_MARKER_SOI           = 0xD8,
  JPEG_MARKER_EOI           = 0xD9,
  JPEG_MARKER_SOS           = 0xDA,
  JPEG_MARKER_DQT           = 0xDB,
  JPEG_MARKER_DNL           = 0xDC,
  JPEG_MARKER_DRI           = 0xDD,
  JPEG_MARKER_APP_MIN       = 0xE0,
  JPEG_MARKER_APP_MAX       = 0xEF,
  JPEG_MARKER_COM           = 0xFE,
} JpegMarkerCode;

/**
 * JpegProfile:
 * @JPEG_PROFILE_BASELINE: Baseline DCT
 * @JPEG_PROFILE_EXTENDED: Extended sequential DCT
 * @JPEG_PROFILE_PROGRESSIVE: Progressive DCT
 * @JPEG_PROFILE_LOSSLESS: Lossless (sequential)
 *
 * JPEG encoding processes.
 */
typedef enum {
  JPEG_PROFILE_BASELINE     = 0x00,
  JPEG_PROFILE_EXTENDED     = 0x01,
  JPEG_PROFILE_PROGRESSIVE  = 0x02,
  JPEG_PROFILE_LOSSLESS     = 0x03,
} JpegProfile;

/**
 * JpegEntropyCodingMode:
 * @JPEG_ENTROPY_CODING_HUFFMAN: Huffman coding
 * @JPEG_ENTROPY_CODING_ARITHMETIC: arithmetic coding
 *
 * JPEG entropy coding mode.
 */
typedef enum {
  JPEG_ENTROPY_CODING_HUFFMAN       = 0x00,
  JPEG_ENTROPY_CODING_ARITHMETIC    = 0x08
} JpegEntropyCodingMode;

/**
 * JpegQuantTable:
 * @quant_precision: Quantization table element precision (Pq)
 * @quant_table: Quantization table elements (Qk)
 * @valid: If the quantization table is valid, which means it has
 *   already been parsed
 *
 * Quantization table.
 */
struct _JpegQuantTable
{
  uint8_t quant_precision;
  uint16_t quant_table[JPEG_MAX_QUANT_ELEMENTS];
  BOOL valid;
};

/**
 * JpegQuantTables:
 * @quant_tables: All quantization tables
 *
 * Helper data structure that holds all quantization tables used to
 * decode an image.
 */
struct _JpegQuantTables
{
  JpegQuantTable quant_tables[JPEG_MAX_SCAN_COMPONENTS];
};

/**
 * JpegHuffmanTable:
 * @huf_bits: Number of Huffman codes of length i + 1 (Li)
 * @huf_vales: Value associated with each Huffman code (Vij)
 * @valid: If the Huffman table is valid, which means it has already
 *   been parsed
 *
 * Huffman table.
 */
struct _JpegHuffmanTable
{
  uint8_t huf_bits[16];
  uint8_t huf_values[256];
  BOOL valid;
};

/**
 * JpegHuffmanTables:
 * @dc_tables: DC Huffman tables
 * @ac_tables: AC Huffman tables
 *
 * Helper data structure that holds all AC/DC Huffman tables used to
 * decode an image.
 */
struct _JpegHuffmanTables
{
  JpegHuffmanTable dc_tables[JPEG_MAX_SCAN_COMPONENTS];
  JpegHuffmanTable ac_tables[JPEG_MAX_SCAN_COMPONENTS];
};

/**
 * JpegScanComponent:
 * @component_selector: Scan component selector (Csj)
 * @dc_selector: DC entropy coding table destination selector (Tdj)
 * @ac_selector: AC entropy coding table destination selector (Taj)

 * Component-specification parameters.
 */
struct _JpegScanComponent
{
    uint8_t component_selector;          /* 0 .. 255     */
    uint8_t dc_selector;                 /* 0 .. 3       */
    uint8_t ac_selector;                 /* 0 .. 3       */
};

/**
 * JpegScanHdr:
 * @num_components: Number of image components in scan (Ns)
 * @components: Image components
 *
 * Scan header.
 */
struct _JpegScanHdr
{
  uint8_t num_components;                /* 1 .. 4       */
  JpegScanComponent components[JPEG_MAX_SCAN_COMPONENTS];
};

/**
 * JpegFrameComponent:
 * @identifier: Component identifier (Ci)
 * @horizontal_factor: Horizontal sampling factor (Hi)
 * @vertical_factor: Vertical sampling factor (Vi)
 * @quant_table_selector: Quantization table destination selector (Tqi)
 *
 * Component-specification parameters.
 */
struct _JpegFrameComponent
{
  uint8_t identifier;                    /* 0 .. 255     */
  uint8_t horizontal_factor;             /* 1 .. 4       */
  uint8_t vertical_factor;               /* 1 .. 4       */
  uint8_t quant_table_selector;          /* 0 .. 3       */
};

/**
 * JpegFrameHdr:
 * @sample_precision: Sample precision (P)
 * @height: Number of lines (Y)
 * @width: Number of samples per line (X)
 * @num_components: Number of image components in frame (Nf)
 * @components: Image components
 * @restart_interval: Number of MCU in the restart interval (Ri)
 *
 * Frame header.
 */
struct _JpegFrameHdr
{
  uint8_t sample_precision;              /* 2 .. 16      */
  uint16_t width;                        /* 1 .. 65535   */
  uint16_t height;                       /* 0 .. 65535   */
  uint8_t num_components;                /* 1 .. 255     */
  JpegFrameComponent components[JPEG_MAX_FRAME_COMPONENTS];
};

/**
 * JpegMarkerSegment:
 * @type: The type of the segment that starts at @offset
 * @offset: The offset to the segment start in bytes. This is the
 *   exact start of the segment, no marker code included
 * @size: The size in bytes of the segment, or -1 if the end was not
 *   found. It is the exact size of the segment, no marker code included
 *
 * A structure that contains the type of a segment, its offset and its size.
 */
struct _JpegMarkerSegment
{
  uint8_t marker;
  uint32_t offset;
  int32_t size;
};

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

/**
 * jpeg_scan_for_marker_code:
 * @data: The data to parse
 * @size: The size of @data
 * @offset: The offset from which to start parsing
 *
 * Scans the JPEG bitstream contained in @data for the next marker
 * code. If found, the function returns an offset to the marker code,
 * including the 0xff prefix code but excluding any extra fill bytes.
 *
 * Returns: offset to the marker code if found, or -1 if not found.
 */
int32_t   jpeg_scan_for_marker_code   (const uint8_t * data,
                                     size_t size,
                                     uint32_t offset);

/**
 * jpeg_parse:
 * @data: The data to parse
 * @size: The size of @data
 * @offset: The offset from which to start parsing
 *
 * Parses the JPEG bitstream contained in @data, and returns the
 * detected segment as a #GstJpegMarkerSegment.
 *
 * Returns: TRUE if a packet start code was found.
 */
BOOL    jpeg_parse (JpegMarkerSegment * seg,
                       const uint8_t * data,
                       size_t size,
                       uint32_t offset);

/**
 * jpeg_parse_frame_hdr:
 * @hdr: (out): The #GstJpegFrameHdr structure to fill in
 * @data: The data from which to parse the frame header
 * @size: The size of @data
 * @offset: The offset in bytes from which to start parsing @data
 *
 * Parses the @hdr JPEG frame header structure members from @data.
 *
 * Returns: TRUE if the frame header was correctly parsed.
 */
BOOL    jpeg_parse_frame_hdr  (JpegFrameHdr * hdr,
                                  const uint8_t * data,
                                  size_t size,
                                  uint32_t offset);

/**
 * jpeg_parse_scan_hdr:
 * @hdr: (out): The #GstJpegScanHdr structure to fill in
 * @data: The data from which to parse the scan header
 * @size: The size of @data
 * @offset: The offset in bytes from which to start parsing @data
 *
 * Parses the @hdr JPEG scan header structure members from @data.
 *
 * Returns: TRUE if the scan header was correctly parsed
 */
BOOL   jpeg_parse_scan_hdr  (JpegScanHdr * hdr,
                                const uint8_t * data,
                                size_t size,
                                uint32_t offset);

/**
 * jpeg_parse_quantization_table:
 * @quant_tables: (out): The #GstJpegQuantizationTable structure to fill in
 * @num_quant_tables: The number of allocated quantization tables in @quant_tables
 * @data: The data from which to parse the quantization table
 * @size: The size of @data
 * @offset: The offset in bytes from which to start parsing @data
 *
 * Parses the JPEG quantization table structure members from @data.
 *
 * Note: @quant_tables represents the complete set of possible
 * quantization tables. However, the parser will only write to the
 * quantization table specified by the table destination identifier
 * (Tq). While doing so, the @valid flag of the specified quantization
 * table will also be set to %TRUE.
 *
 * Returns: TRUE if the quantization table was correctly parsed.
 */
BOOL  jpeg_parse_quant_table   (JpegQuantTables *quant_tables,
                                   const uint8_t * data,
                                   size_t size,
                                   uint32_t offset);

/**
 * jpeg_parse_huffman_table:
 * @huf_tables: (out): The #GstJpegHuffmanTable structure to fill in
 * @data: The data from which to parse the Huffman table
 * @size: The size of @data
 * @offset: The offset in bytes from which to start parsing @data
 *
 * Parses the JPEG Huffman table structure members from @data.
 *
 * Note: @huf_tables represents the complete set of possible Huffman
 * tables. However, the parser will only write to the Huffman table
 * specified by the table destination identifier (Th). While doing so,
 * the @valid flag of the specified Huffman table will also be set to
 * %TRUE;
 *
 * Returns: TRUE if the Huffman table was correctly parsed.
 */
BOOL  jpeg_parse_huffman_table   (JpegHuffmanTables *huf_tables,
                                     const uint8_t * data,
                                     size_t size,
                                     uint32_t offset);

/**
 * jpeg_parse_restart_interval:
 * @interval: (out): The parsed restart interval value
 * @data: The data from which to parse the restart interval specification
 * @size: The size of @data
 * @offset: The offset in bytes from which to start parsing @data
 *
 * Returns: TRUE if the restart interval value was correctly parsed.
 */
BOOL  jpeg_parse_restart_interval (uint32_t * interval,
                                      const uint8_t * data,
                                      size_t size,
                                      uint32_t offset);

/**
 * jpeg_get_default_huffman_tables:
 * @huf_tables: (out): The default DC/AC Huffman tables to fill in
 *
 * Fills in @huf_tables with the default AC/DC Huffman tables, as
 * specified by the JPEG standard.
 */
void jpeg_get_default_huffman_tables (JpegHuffmanTables *huf_tables);

/**
 * jpeg_get_default_quantization_table:
 * @quant_tables: (out): The default luma/chroma quant-tables in zigzag mode
 *
 * Fills in @quant_tables with the default quantization tables, as
 * specified by the JPEG standard.
 */
void jpeg_get_default_quantization_tables (JpegQuantTables *quant_tables);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* JPEG_PARSER_H */
