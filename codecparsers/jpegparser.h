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
#include <limits.h>
#include <stdbool.h>
#include <string.h>

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
  bool valid;
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
  bool valid;
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
 * Returns: true if a packet start code was found.
 */
bool    jpeg_parse (JpegMarkerSegment * seg,
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
 * Returns: true if the frame header was correctly parsed.
 */
bool    jpeg_parse_frame_hdr  (JpegFrameHdr * hdr,
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
 * Returns: true if the scan header was correctly parsed
 */
bool   jpeg_parse_scan_hdr  (JpegScanHdr * hdr,
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
 * table will also be set to %true.
 *
 * Returns: true if the quantization table was correctly parsed.
 */
bool  jpeg_parse_quant_table   (JpegQuantTables *quant_tables,
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
 * %true;
 *
 * Returns: true if the Huffman table was correctly parsed.
 */
bool  jpeg_parse_huffman_table   (JpegHuffmanTables *huf_tables,
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
 * Returns: true if the restart interval value was correctly parsed.
 */
bool  jpeg_parse_restart_interval (uint32_t * interval,
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
