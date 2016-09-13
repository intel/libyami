/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vc1Parser.h"
#include "common/log.h"
#include "common/common_def.h"
#include <cstring>
#include <cassert>

#define READ(f)                             \
    do {                                    \
        if (!br->readT(f)) {                \
            ERROR("failed to read %s", #f); \
            return false;                   \
        }                                   \
    } while (0)

#define READ_BITS(f, bits)                                   \
    do {                                                     \
        if (!br->readT(f, bits)) {                           \
            ERROR("failed to read %d bits to %s", bits, #f); \
            return false;                                    \
        }                                                    \
    } while (0)

#define SKIP(bits)                                 \
    do {                                           \
        if (!br->skip(bits)) {                     \
            ERROR("failed to skip %d bits", bits); \
            return false;                          \
        }                                          \
    } while (0)

#define PEEK(f, bits)                                        \
    do {                                                     \
        if (!br->peek(f, bits)) {                            \
            ERROR("failed to peek %d bits to %s", bits, #f); \
            return false;                                    \
        }                                                    \
    } while (0)

namespace YamiParser {
namespace VC1 {

    /* Table 36: PQINDEX to PQUANT/Quantizer Translation*/
    static const uint8_t QuantizerTranslationTable[32] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 27, 29, 31
    };

    /* Table 69: IMODE VLC Code table */
    static const VLCTable ImodeVLCTable[7] = {
        { 2, 2 }, { 3, 2 }, { 2, 3 },
        { 3, 3 }, { 1, 3 }, { 1, 4 }, { 0, 4 }
    };

    /* Table 81: Code table for 3x2 and 2x3 tiles */
    static const VLCTable Norm6VLCTable[64] = {
        { 1, 1 },
        { 2, 4 },
        { 3, 4 },
        { 0, 8 },
        { 4, 4 },
        { 1, 8 },
        { 2, 8 },
        { (2 << 5) | 7, (5 + 5) },
        { 5, 4 },
        { 3, 8 },
        { 4, 8 },
        { (2 << 5) | 11, (5 + 5) },
        { 5, 8 },
        { (2 << 5) | 13, (5 + 5) },
        { (2 << 5) | 14, (5 + 5) },
        { (3 << 8) | 14, (5 + 8) },
        { 6, 4 },
        { 6, 8 },
        { 7, 8 },
        { (2 << 5) | 19, (5 + 5) },
        { 8, 8 },
        { (2 << 5) | 21, (5 + 5) },
        { (2 << 5) | 22, (5 + 5) },
        { (3 << 8) | 13, (5 + 8) },
        { 9, 8 },
        { (2 << 5) | 25, (5 + 5) },
        { (2 << 5) | 26, (5 + 5) },
        { (3 << 8) | 12, (5 + 8) },
        { (2 << 5) | 28, (5 + 5) },
        { (3 << 8) | 11, (5 + 8) },
        { (3 << 8) | 10, (5 + 8) },
        { (3 << 4) | 7, (5 + 4) },
        { 7, 4 },
        { 10, 8 },
        { 11, 8 },
        { (2 << 5) | 3, (5 + 5) },
        { 12, 8 },
        { (2 << 5) | 5, (5 + 5) },
        { (2 << 5) | 6, (5 + 5) },
        { (3 << 8) | 9, (5 + 8) },
        { 13, 8 },
        { (2 << 5) | 9, (5 + 5) },
        { (2 << 5) | 10, (5 + 5) },
        { (3 << 8) | 8, (5 + 8) },
        { (2 << 5) | 12, (5 + 5) },
        { (3 << 8) | 7, (5 + 8) },
        { (3 << 8) | 6, (5 + 8) },
        { (3 << 4) | 6, (5 + 4) },
        { 14, 8 },
        { (2 << 5) | 17, (5 + 5) },
        { (2 << 5) | 18, (5 + 5) },
        { (3 << 8) | 5, (5 + 8) },
        { (2 << 5) | 20, (5 + 5) },
        { (3 << 8) | 4, (5 + 8) },
        { (3 << 8) | 3, (5 + 8) },
        { (3 << 4) | 5, (5 + 4) },
        { (2 << 5) | 24, (5 + 5) },
        { (3 << 8) | 2, (5 + 8) },
        { (3 << 8) | 1, (5 + 8) },
        { (3 << 4) | 4, (5 + 4) },
        { (3 << 8) | 0, (5 + 8) },
        { (3 << 4) | 3, (5 + 4) },
        { (3 << 4) | 2, (5 + 4) },
        { (3 << 1) | 1, (5 + 1) }
    };

    static const FrameType FrameTypeTable[2][8] = {
        {
            FRAME_I, FRAME_I, FRAME_P, FRAME_P,
            FRAME_B, FRAME_B, FRAME_BI, FRAME_BI
        },
        {
            FRAME_P, FRAME_B, FRAME_I, FRAME_BI,
            FRAME_SKIPPED
        },
    };

    /* Table 40: BFRACTION VLC Table */
    static const VLCTable BFractionVLCTable[] = {
        {0x00, 3}, {0x01, 3},
        {0x02, 3}, {0x02, 3},
        {0x04, 3}, {0x05, 3},
        {0x06, 3}, {0x70, 7},
        {0x71, 7}, {0x72, 7},
        {0x73, 7}, {0x74, 7},
        {0x75, 7}, {0x76, 7},
        {0x77, 7}, {0x78, 7},
        {0x79, 7}, {0x7a, 7},
        {0x7b, 7}, {0x7c, 7},
        {0x7d, 7}, {0x7e, 7},
        {0x7f, 7}
    };

    Parser::Parser()
    {
        memset(&m_seqHdr, 0, sizeof(m_seqHdr));
        memset(&m_entryPointHdr, 0, sizeof(m_entryPointHdr));
    }

    Parser::~Parser()
    {
    }

    bool Parser::parseCodecData(uint8_t* data, uint32_t size)
    {
        int32_t offset;
        offset = searchStartCode(data, size);
        if (offset < 0) {
            if (!parseSequenceHeader(data, size))
                return false;
        }
        else {
            while (1) {
                data += (offset + 4);
                size -= (offset + 4);
                if (data[-1] == 0xF) {
                    if (!parseSequenceHeader(data, size))
                        return false;
                }
                else if (data[-1] == 0xE) {
                    if (!parseEntryPointHeader(data, size))
                        return false;
                }
                offset = searchStartCode(data, size);
                if (offset < 0)
                    break;
            }
        }
        return true;
    }

    bool Parser::parseFrameHeader(uint8_t*& data, uint32_t& size)
    {
        bool ret = false;
        m_mbWidth = (m_seqHdr.coded_width + 15) >> 4;
        m_mbHeight = (m_seqHdr.coded_height + 15) >> 4;
        mallocBitPlanes();
        memset(&m_frameHdr, 0, sizeof(m_frameHdr));
        if (!convertToRbdu(data, size))
            return false;
        BitReader bitReader(&m_rbdu[0], m_rbdu.size());
        if (m_seqHdr.profile != PROFILE_ADVANCED) {
            ret = parseFrameHeaderSimpleMain(&bitReader);
        }
        else {
            /* support advanced profile */
            ret = parseFrameHeaderAdvanced(&bitReader);
        }
        m_frameHdr.macroblock_offset = bitReader.getPos();
        return ret;
    }

    int32_t Parser::searchStartCode(uint8_t* data, uint32_t size)
    {
        uint8_t* pos;
        const uint8_t startCode[] = { 0x00, 0x00, 0x01 };
        pos = std::search(data, data + size, startCode, startCode + 3);
        return (pos == data + size) ? (-1) : (pos - data);
    }

    bool Parser::convertToRbdu(uint8_t*& data, uint32_t& size)
    {
        uint8_t* pos;
        int32_t offset, i = 0;
        const uint8_t startCode[] = { 0x00, 0x00, 0x03 };

        if (m_seqHdr.profile == PROFILE_ADVANCED) {
            while (1) {
                /*skip for ununsed bdu types*/
                /*data and size are input and output parameters*/
                offset = searchStartCode(data, size);
                if (offset >= 0) {
                    data += (offset + 4);
                    size -= (offset + 4);
                }
                if ((offset < 0) || (data[-1] == 0xD))
                    break;
            }
            if (offset < 0) {
                size = 0;
                return false;
            }
        }
        m_rbdu.clear();
        /*extraction of rbdu from ebdu*/
        while (1) {
            pos = std::search(data + i, data + size, startCode, startCode + 3);
            if (pos == data + size) {
                m_rbdu.insert(m_rbdu.end(), data + i, pos);
                break;
            }
            if (pos[3] <= 0x03) {
                m_rbdu.insert(m_rbdu.end(), data + i, pos + 2);
            }
            else {
                m_rbdu.insert(m_rbdu.end(), data + i, pos + 3);
            }
            i = pos - data + 3;
        }
        return (size > 0);
    }

    /*Table 3: Sequence layer bitstream for Advanced Profile*/
    /*Table 263: Sequence Header Data Structure STRUCT_C for Simple and Main Profiles*/
    /*Table 264: Sequence Header Data Structure STRUCT_C for Advanced Profiles*/
    bool Parser::parseSequenceHeader(const uint8_t* data, uint32_t size)
    {
        uint32_t i;
        BitReader bitReader(data, size);
        BitReader* br = &bitReader;
        READ_BITS(m_seqHdr.profile, 2);
        if (m_seqHdr.profile != PROFILE_ADVANCED) {
            /* skip reserved bits */
            SKIP(2);
            READ_BITS(m_seqHdr.frmrtq_postproc, 3);
            READ_BITS(m_seqHdr.bitrtq_postproc, 5);
            READ(m_seqHdr.loop_filter);
            /* skip reserved bits */
            SKIP(1);
            READ(m_seqHdr.multires);
            /* skip reserved bits */
            SKIP(1);
            READ(m_seqHdr.fastuvmc);
            READ(m_seqHdr.extended_mv);
            READ_BITS(m_seqHdr.dquant, 2);
            READ(m_seqHdr.variable_sized_transform_flag);
            /* skip reserved bits */
            SKIP(1);
            READ(m_seqHdr.overlap);
            READ(m_seqHdr.syncmarker);
            READ(m_seqHdr.rangered);
            READ_BITS(m_seqHdr.max_b_frames, 3);
            READ_BITS(m_seqHdr.quantizer, 2);
            READ(m_seqHdr.finterpflag);
        }
        else {
            READ_BITS(m_seqHdr.level, 3);
            READ_BITS(m_seqHdr.colordiff_format, 2);
            READ_BITS(m_seqHdr.frmrtq_postproc, 3);
            READ_BITS(m_seqHdr.bitrtq_postproc, 5);
            READ(m_seqHdr.postprocflag);
            uint16_t tmp;
            READ_BITS(tmp, 12);
            m_seqHdr.coded_width = (tmp + 1) << 1;
            READ_BITS(tmp, 12);
            m_seqHdr.coded_height = (tmp + 1) << 1;
            READ(m_seqHdr.pulldown);
            READ(m_seqHdr.interlace);
            READ(m_seqHdr.tfcntrflag);
            READ(m_seqHdr.finterpflag);
            /* skip reserved bits */
            SKIP(1);
            READ(m_seqHdr.psf);
            READ(m_seqHdr.display_ext);
            if (m_seqHdr.display_ext) {
                READ_BITS(m_seqHdr.disp_horiz_size, 14);
                m_seqHdr.disp_horiz_size++;
                READ_BITS(m_seqHdr.disp_vert_size, 14);
                m_seqHdr.disp_vert_size++;
                READ(m_seqHdr.aspect_ratio_flag);
                if (m_seqHdr.aspect_ratio_flag) {
                    READ_BITS(m_seqHdr.aspect_ratio, 4);
                    if (m_seqHdr.aspect_ratio == 15) {
                        READ(m_seqHdr.aspect_horiz_size);
                        READ(m_seqHdr.aspect_vert_size);
                    }
                }
                READ(m_seqHdr.framerate_flag);
                if (m_seqHdr.framerate_flag) {
                    READ(m_seqHdr.framerateind);
                    if (m_seqHdr.framerateind == 0) {
                        READ(m_seqHdr.frameratenr);
                        READ_BITS(m_seqHdr.frameratedr, 4);
                    }
                    else {
                        READ(m_seqHdr.framerateexp);
                    }
                }
                READ(m_seqHdr.color_format_flag);

                if (m_seqHdr.color_format_flag) {
                    READ(m_seqHdr.color_prim);
                    READ(m_seqHdr.transfer_char);
                    READ(m_seqHdr.matrix_coef);
                }
            }
            READ(m_seqHdr.hrd_param_flag);

            /*Table 13: Syntax elements for HRD_PARAM structure*/
            if (m_seqHdr.hrd_param_flag) {
                READ_BITS(m_seqHdr.hrd_param.hrd_num_leaky_buckets, 5);
                READ_BITS(m_seqHdr.hrd_param.bit_rate_exponent, 4);
                READ_BITS(m_seqHdr.hrd_param.buffer_size_exponent, 4);
                for (i = 0; i < m_seqHdr.hrd_param.hrd_num_leaky_buckets; i++) {
                    READ(m_seqHdr.hrd_param.hrd_rate[i]);
                    READ(m_seqHdr.hrd_param.hrd_buffer[i]);
                }
            }
        }
        return true;
    }

    /*Table 14: Entry-point layer bitstream for Advanced Profile*/
    bool Parser::parseEntryPointHeader(const uint8_t* data, uint32_t size)
    {
        uint8_t i;
        BitReader bitReader(data, size);
        BitReader* br = &bitReader;
        READ(m_entryPointHdr.broken_link);
        READ(m_entryPointHdr.closed_entry);
        READ(m_entryPointHdr.panscan_flag);
        READ(m_entryPointHdr.reference_distance_flag);
        READ(m_entryPointHdr.loopfilter);
        READ(m_entryPointHdr.fastuvmc);
        READ(m_entryPointHdr.extended_mv);
        READ_BITS(m_entryPointHdr.dquant, 2);
        READ(m_entryPointHdr.variable_sized_transform_flag);
        READ(m_entryPointHdr.overlap);
        READ_BITS(m_entryPointHdr.quantizer, 2);
        if (m_seqHdr.hrd_param_flag) {
            for (i = 0; i < m_seqHdr.hrd_param.hrd_num_leaky_buckets; i++)
                READ(m_entryPointHdr.hrd_full[i]);
        }

        READ(m_entryPointHdr.coded_size_flag);
        if (m_entryPointHdr.coded_size_flag) {
            READ_BITS(m_entryPointHdr.coded_width, 12);
            m_entryPointHdr.coded_width = (m_entryPointHdr.coded_width + 1) << 1;
            READ_BITS(m_entryPointHdr.coded_height, 12);
            m_entryPointHdr.coded_height = (m_entryPointHdr.coded_height + 1) << 1;
            m_seqHdr.coded_width = m_entryPointHdr.coded_width;
            m_seqHdr.coded_height = m_entryPointHdr.coded_height;
        }

        if (m_entryPointHdr.extended_mv)
            READ(m_entryPointHdr.extended_dmv_flag);

        READ(m_entryPointHdr.range_mapy_flag);
        if (m_entryPointHdr.range_mapy_flag)
            READ_BITS(m_entryPointHdr.range_mapy, 3);

        READ(m_entryPointHdr.range_mapuv_flag);
        if (m_entryPointHdr.range_mapy_flag)
            READ_BITS(m_entryPointHdr.range_mapuv, 3);
        return true;
    }

    bool Parser::parseSliceHeader(uint8_t* data, uint32_t size)
    {
        bool temp;
        BitReader* br;
        bool ret = true;
        if (m_seqHdr.profile != PROFILE_ADVANCED)
            return false;
        BitReader bitReader(data, size);
        br = &bitReader;
        READ_BITS(m_sliceHdr.slice_addr, 9);
        READ(temp);
        if (temp)
            ret = parseFrameHeaderAdvanced(&bitReader);

        m_sliceHdr.macroblock_offset = bitReader.getPos();
        return ret;
    }

    void Parser::mallocBitPlanes()
    {
        uint32_t size = m_mbHeight * m_mbWidth;
        m_bitPlanes.acpred.resize(size);
        m_bitPlanes.fieldtx.resize(size);
        m_bitPlanes.overflags.resize(size);
        m_bitPlanes.mvtypemb.resize(size);
        m_bitPlanes.skipmb.resize(size);
        m_bitPlanes.directmb.resize(size);
        m_bitPlanes.forwardmb.resize(size);
    }

    bool Parser::decodeVLCTable(BitReader* br, uint16_t* out,
        const VLCTable* table, uint32_t tableLen)
    {
        uint32_t i = 0;
        uint16_t numberOfBits = 0;
        uint16_t codeWord = 0;
        while (i < tableLen) {
            if (numberOfBits != table[i].codeLength) {
                numberOfBits = table[i].codeLength;
                PEEK(codeWord, numberOfBits);
            }
            if (codeWord == table[i].codeWord) {
                *out = i;
                SKIP(numberOfBits);
                break;
            }
            i++;
        }
        return (i < tableLen) ? true : false;
    }

    /* 8.7.3.6 Row-skip mode*/
    bool Parser::decodeRowskipMode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i, j;
        for (j = 0; j < height; j++) {
            bool rowSkip;
            READ(rowSkip);
            if (rowSkip) {
                for (i = 0; i < width; i++)
                    READ_BITS(data[i], 1);
            }
            else {
                memset(data, 0, width);
            }
            data += m_mbWidth;
        }
        return true;
    }

    /* 8.7.3.7 Column-skip mode*/
    bool Parser::decodeColskipMode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i, j;
        for (i = 0; i < width; i++) {
            bool columnSkip;
            READ(columnSkip);
            if (columnSkip) {
                for (j = 0; j < height; j++)
                    READ_BITS(data[j * m_mbWidth], 1);
            }
            else {
                for (j = 0; j < height; j++)
                    data[j * m_mbWidth] = 0;
            }
            data++;
        }
        return true;
    }

    /* Table 80: Norm-2/Diff-2 Code Table */
    bool Parser::decodeNorm2Mode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i;
        bool temp;
        uint8_t* out = data;
        if ((height * width) & 1) {
            READ_BITS(*out, 1);
            out++;
        }
        for (i = (height * width) & 1; i < height * width; i += 2) {
            READ(temp);
            if (temp == 0) {
                *out++ = 0;
                *out++ = 0;
            }
            else {
                READ(temp);
                if (temp) {
                    *out++ = 1;
                    *out++ = 1;
                }
                else {
                    READ(temp);
                    if (temp == 0) {
                        *out++ = 1;
                        *out++ = 0;
                    }
                    else {
                        *out++ = 0;
                        *out++ = 1;
                    }
                }
            }
        }
        return true;
    }

    bool Parser::decodeNorm6Mode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i = 0, j = 0;
        uint16_t temp;
        uint8_t* out = data;
        bool is2x3Tiled = (((width % 3) != 0) && ((height % 3) == 0));
        if (is2x3Tiled) {
            for (j = 0; j < height; j += 3) {
                for (i = width & 1; i < width; i += 2) {
                    if (!decodeVLCTable(br, &temp, Norm6VLCTable, 64))
                        return false;
                    out[i] = temp & 1;
                    out[i + 1] = (temp & 2) >> 1;
                    out[i + width] = (temp & 4) >> 2;
                    out[i + width + 1] = (temp & 8) >> 3;
                    out[i + width * 2] = (temp & 16) >> 4;
                    out[i + width * 2 + 1] = (temp & 32) >> 5;
                }
                out += 3 * width;
            }
            if (width & 1)
                decodeColskipMode(br, data, width & 1, height);
        }
        else {
            out += (height & 1) * width;
            for (j = height & 1; j < height; j += 2) {
                for (i = width % 3; i < width; i += 3) {
                    if (!decodeVLCTable(br, &temp, Norm6VLCTable, 64))
                        return false;
                    out[i] = temp & 1;
                    out[i + 1] = (temp & 2) >> 1;
                    out[i + 2] = (temp & 4) >> 2;
                    out[i + width] = (temp & 8) >> 3;
                    out[i + width + 1] = (temp & 16) >> 4;
                    out[i + width + 2] = (temp & 32) >> 5;
                }
                out += 2 * width;
            }
            if (width % 3)
                decodeColskipMode(br, data, width % 3, height);
            if (height & 1)
                decodeRowskipMode(br, data + (width % 3), width - (width % 3), height & 1);
        }
        return true;
    }

    /* 8.7.3.8 Diff: Inverse differential decoding */
    void Parser::inverseDiff(uint8_t* data, uint32_t width, uint32_t height, uint32_t invert)
    {
        uint32_t i, j;
        for (j = 0; j < height; j++)
            for (i = 0; i < width; i++) {
                if ((i == 0) && (j == 0)) {
                    data[j * width + i] ^= invert;
                }
                else if (i == 0) {
                    data[j * width + i] ^= data[(j - 1) * width];
                }
                else if ((j > 0) && (data[(j - 1) * width + i] != data[j * width + i - 1])) {
                    data[j * width + i] ^= invert;
                }
                else {
                    data[j * width + i] ^= data[j * width + i - 1];
                }
            }
    }

    bool Parser::decodeBitPlane(BitReader* br, uint8_t* data, bool* isRaw)
    {
        uint32_t i, invert;
        uint16_t mode;
        *isRaw = false;
        READ_BITS(invert, 1);
        if (!decodeVLCTable(br, &mode, ImodeVLCTable, 7))
            return false;
        if (mode == IMODE_RAW) {
            *isRaw = true;
            return true;
        }
        else if (mode == IMODE_NORM2) {
            decodeNorm2Mode(br, data, m_mbWidth, m_mbHeight);
        }
        else if (mode == IMODE_NORM6) {
            decodeNorm6Mode(br, data, m_mbWidth, m_mbHeight);
        }
        else if (mode == IMODE_DIFF2) {
            decodeNorm2Mode(br, data, m_mbWidth, m_mbHeight);
            inverseDiff(data, m_mbWidth, m_mbHeight, invert);
        }
        else if (mode == IMODE_DIFF6) {
            decodeNorm6Mode(br, data, m_mbWidth, m_mbHeight);
            inverseDiff(data, m_mbWidth, m_mbHeight, invert);
        }
        else if (mode == IMODE_ROWSKIP) {
            decodeRowskipMode(br, data, m_mbWidth, m_mbHeight);
        }
        else if (mode == IMODE_COLSKIP) {
            decodeColskipMode(br, data, m_mbWidth, m_mbHeight);
        }
        /*8.7.1 INVERT*/
        if ((mode != IMODE_DIFF2) && (mode != IMODE_DIFF6) && invert) {
            for (i = 0; i < m_mbWidth * m_mbHeight; i++)
                data[i] = !data[i];
        }
        return true;
    }

    /*Table 24: VOPDQUANT in picture header(Refer to 7.1.1.31)*/
    /*7.1.1.31.6 Picture Quantizer Differential(PQDIFF)(3 bits)*/
    bool Parser::parseVopdquant(BitReader* br, uint8_t dquant)
    {
        if (dquant == 2) {
            m_frameHdr.dq_frame = 0;
            READ_BITS(m_frameHdr.pq_diff, 3);
            if (m_frameHdr.pq_diff != 7) {
                m_frameHdr.alt_pic_quantizer = m_frameHdr.pquant + m_frameHdr.pq_diff + 1;
            }
            else {
                READ_BITS(m_frameHdr.abs_pq, 5);
                m_frameHdr.alt_pic_quantizer = m_frameHdr.abs_pq;
            }
        }
        else {
            READ(m_frameHdr.dq_frame);
            if (m_frameHdr.dq_frame) {
                READ_BITS(m_frameHdr.dq_profile, 2);
                if (m_frameHdr.dq_profile == DQPROFILE_SINGLE_EDGE) {
                    READ_BITS(m_frameHdr.dq_sb_edge, 2);
                }
                else if (m_frameHdr.dq_profile == DQPROFILE_DOUBLE_EDGE) {
                    READ_BITS(m_frameHdr.dq_db_edge, 2);
                }
                else if (m_frameHdr.dq_profile == DQPROFILE_ALL_MACROBLOCKS) {
                    READ(m_frameHdr.dq_binary_level);
                }
                if (!((m_frameHdr.dq_profile == DQPROFILE_ALL_MACROBLOCKS)
                        && (m_frameHdr.dq_binary_level == 0))) {
                    READ_BITS(m_frameHdr.pq_diff, 3);
                    if (m_frameHdr.pq_diff != 7) {
                        m_frameHdr.alt_pic_quantizer = m_frameHdr.pquant + m_frameHdr.pq_diff + 1;
                    }
                    else {
                        READ_BITS(m_frameHdr.abs_pq, 5);
                        m_frameHdr.alt_pic_quantizer = m_frameHdr.abs_pq;
                    }
                }
            }
        }
        return true;
    }

    int32_t Parser::getFirst01Bit(BitReader* br, bool val, uint32_t len)
    {
        uint32_t i = 0;
        while (i < len) {
            //TODO: check read beyond boundary here, it may need change so many functions
            if (br->read(1) == val)
                break;
            i++;
        }
        return i;
    }

    uint8_t Parser::getMVMode(BitReader* br, uint8_t pQuant, bool isMvMode2)
    {
        int32_t temp = 0;
        uint8_t MVMode = 0;
        temp = isMvMode2 ? getFirst01Bit(br, 1, 3) : getFirst01Bit(br, 1, 4);
        switch (temp) {
        case 0:
            MVMode = (pQuant <= 12) ? MVMODE_1_MV : MVMODE_1_MV_HALF_PEL_BILINEAR;
            break;
        case 1:
            MVMode = (pQuant <= 12) ? MVMODE_MIXED_MV : MVMODE_1_MV;
            break;
        case 2:
            MVMode = MVMODE_1_MV_HALF_PEL;
            break;
        case 3:
            if (isMvMode2) {
                MVMode = (pQuant <= 12) ? MVMODE_1_MV_HALF_PEL_BILINEAR : MVMODE_MIXED_MV;
            }
            else {
                MVMode = MVMODE_INTENSITY_COMPENSATION;
            }
            break;
        case 4:
            if (!isMvMode2)
                MVMode = (pQuant <= 12) ? MVMODE_1_MV_HALF_PEL_BILINEAR : MVMODE_MIXED_MV;
            break;
        default:
            break;
        }
        return MVMode;
    }

    bool Parser::decodeBFraction(BitReader* br)
    {
        uint16_t bfraction;
        if (!decodeVLCTable(br, &bfraction, BFractionVLCTable, N_ELEMENTS(BFractionVLCTable)))
            return false;

        m_frameHdr.bfraction = bfraction;
        if (m_frameHdr.bfraction == 22) {
            m_frameHdr.picture_type = FRAME_BI;
        }
        return true;
    }

    /*Table 16: Progressive I picture layer bitstream for Simple and Main Profile*/
    /*Table 17: Progressive BI picture layer bitstream for Main Profile*/
    /*Table 19: Progressive P picture layer bitstream for Simple and Main Profile*/
    /*Table 21: Progressive B picture layer bitstream for Main Profile*/
    bool Parser::parseFrameHeaderSimpleMain(BitReader* br)
    {
        bool temp;
        m_frameHdr.interpfrm = 0;
        if (m_seqHdr.finterpflag)
            READ(m_frameHdr.interpfrm);

        SKIP(2);
        m_frameHdr.range_reduction_frame = 0;
        if (m_seqHdr.rangered)
            READ(m_frameHdr.range_reduction_frame);

        READ(temp);
        if (m_seqHdr.max_b_frames == 0) {
            /* Table 33 Simple/Main Profile Picture Type VLC if MAXBFRAMES == 0 */
            if (!temp)
                m_frameHdr.picture_type = FRAME_I;
            else
                m_frameHdr.picture_type = FRAME_P;
        }
        else {
            /* Table 34 Main Profile Picture Type VLC if MAXBFRAMES  > 0 */
            if (!temp) {
                bool ptype;
                READ(ptype);
                if (ptype)
                    m_frameHdr.picture_type = FRAME_I;
                else
                    m_frameHdr.picture_type = FRAME_B;
            }
            else {
                m_frameHdr.picture_type = FRAME_P;
            }
        }

        if (m_frameHdr.picture_type == FRAME_B) {
            if (!decodeBFraction(br))
                return false;
        }
        if (m_frameHdr.picture_type == FRAME_I
            || m_frameHdr.picture_type == FRAME_BI)
            SKIP(7); /* skip BF*/
        READ_BITS(m_frameHdr.pqindex, 5);
        if (m_frameHdr.pqindex <= 8)
            READ(m_frameHdr.halfqp);
        if (m_seqHdr.quantizer == 0) {
            m_frameHdr.pquant = QuantizerTranslationTable[m_frameHdr.pqindex];
            m_frameHdr.pquantizer = (m_frameHdr.pqindex <= 8) ? 1 : 0;
        }
        else {
            m_frameHdr.pquant = m_frameHdr.pqindex;
            if (m_seqHdr.quantizer == 1)
                READ(m_frameHdr.pquantizer);
            else if (m_seqHdr.quantizer == 2)
                m_frameHdr.pquantizer = 0;
            else if (m_seqHdr.quantizer == 3)
                m_frameHdr.pquantizer = 1;
            else
                assert(0);
        }

        if (m_seqHdr.extended_mv)
            m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);

        if (m_seqHdr.multires
            && ((m_frameHdr.picture_type == FRAME_I)
                   || (m_frameHdr.picture_type == FRAME_P))) {
            READ_BITS(m_frameHdr.picture_resolution_index, 2);
        }

        if ((m_frameHdr.picture_type == FRAME_I)
            || (m_frameHdr.picture_type == FRAME_BI)) {
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.transacfrm2 = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);
        }
        else if (m_frameHdr.picture_type == FRAME_P) {
            m_frameHdr.mv_mode = getMVMode(br, m_frameHdr.pquant, false);
            if (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION) {
                m_frameHdr.mv_mode2 = getMVMode(br, m_frameHdr.pquant, true);
                READ_BITS(m_frameHdr.lumscale, 6);
                READ_BITS(m_frameHdr.lumshift, 6);
            }
            if (m_frameHdr.mv_mode == MVMODE_MIXED_MV
                || (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION
                       && m_frameHdr.mv_mode2 == MVMODE_MIXED_MV)) {
                if (!decodeBitPlane(br, &m_bitPlanes.mvtypemb[0], &m_frameHdr.mv_type_mb))
                    return false;
            }
            if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                return false;

            READ_BITS(m_frameHdr.mv_table, 2);
            READ_BITS(m_frameHdr.cbp_table, 2);
            if (m_seqHdr.dquant)
                parseVopdquant(br, m_seqHdr.dquant);

            if (m_seqHdr.variable_sized_transform_flag) {
                READ(m_frameHdr.mb_level_transform_type_flag);
                if (m_frameHdr.mb_level_transform_type_flag)
                    READ_BITS(m_frameHdr.frame_level_transform_type, 2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);
        }
        else if (m_frameHdr.picture_type == FRAME_B) {
            READ_BITS(m_frameHdr.mv_mode, 1);
            m_frameHdr.mv_mode = !(m_frameHdr.mv_mode);
            if (!decodeBitPlane(br, &m_bitPlanes.directmb[0], &m_frameHdr.direct_mb))
                return false;
            if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                return false;
            READ_BITS(m_frameHdr.mv_table, 2);
            READ_BITS(m_frameHdr.cbp_table, 2);
            if (m_seqHdr.dquant)
                parseVopdquant(br, m_seqHdr.dquant);
            if (m_seqHdr.variable_sized_transform_flag) {
                READ(m_frameHdr.mb_level_transform_type_flag);
                if (m_frameHdr.mb_level_transform_type_flag)
                    READ_BITS(m_frameHdr.frame_level_transform_type, 2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);
        }
        return true;
    }

    /* 9.1.1.43 P Reference Distance*/
    /* Table 106: REFDIST VLC Table*/
    bool Parser::getRefDist(BitReader* br, uint8_t& refDist)
    {
        uint32_t vlcSize;
        PEEK(vlcSize, 2);
        if (vlcSize != 3) {
            READ_BITS(refDist, 2);
        }
        else {
            //TODO: check read beyond boundary
            refDist = getFirst01Bit(br, 0, 16) + 1;
        }
        return true;
    }

    /*Table 18: Progressive I and BI picture layer bitstream for Advanced Profile*/
    /*Table 20: Progressive P picture layer bitstream for Advanced Profile*/
    /*Table 22: Progressive B picture layer bitstream for Advanced Profile*/
    /*Table 83: Interlaced Frame P picture layer bitstream for Advanced Profile*/
    /*Table 84: Interlaced Frame B picture layer bitstream for Advanced Profile*/
    /*Table 85: Picture Layer bitstream for Field 1 of Interlace Field Picture for Advanced Profile*/
    bool Parser::parseFrameHeaderAdvanced(BitReader* br)
    {
        uint32_t temp;
        if (m_seqHdr.interlace) {
            m_frameHdr.fcm = getFirst01Bit(br, 0, 2);
        }
        else {
            m_frameHdr.fcm = PROGRESSIVE;
        }
        if (m_frameHdr.fcm == FIELD_INTERLACE) {
            READ_BITS(temp, 3);
            /* 9.1.1.42 Field Picture Type(FPTYPE) (3 bits) */
            m_frameHdr.picture_type = FrameTypeTable[0][temp];
        }
        else {
            /* Table 35 Advanced Profile Picture Type VLC */
            temp = getFirst01Bit(br, 0, 4);
            m_frameHdr.picture_type = FrameTypeTable[1][temp];
        }

        /* skip tfcntr */
        if (m_seqHdr.tfcntrflag)
            SKIP(8);

        if (m_seqHdr.pulldown) {
            if (!(m_seqHdr.interlace) || m_seqHdr.psf) {
                READ_BITS(m_frameHdr.rptfrm, 2);
            }
            else {
                READ(m_frameHdr.tff);
                READ(m_frameHdr.rff);
            }
        }
        else {
            m_frameHdr.tff = 1;
        }

        if (m_frameHdr.picture_type == FRAME_SKIPPED)
            return true;

        READ(m_frameHdr.rounding_control);

        if (m_seqHdr.interlace) {
            READ(m_frameHdr.uvsamp);
            if ((m_frameHdr.fcm == FIELD_INTERLACE)
                && m_entryPointHdr.reference_distance_flag
                && (m_frameHdr.picture_type != FRAME_B)
                && (m_frameHdr.picture_type != FRAME_BI)) {
                if (!getRefDist(br, m_frameHdr.refdist))
                    return false;
            }
        }
        if (m_seqHdr.finterpflag)
            READ(m_frameHdr.interpfrm);
        if ((m_frameHdr.fcm != FIELD_INTERLACE
             && m_frameHdr.picture_type == FRAME_B)
             || (m_frameHdr.fcm == FIELD_INTERLACE
             && ((m_frameHdr.picture_type == FRAME_B)
             || (m_frameHdr.picture_type == FRAME_BI)))) {
            if (!decodeBFraction(br))
                return false;
        }
        READ_BITS(m_frameHdr.pqindex, 5);
        if (m_frameHdr.pqindex <= 8)
            READ(m_frameHdr.halfqp);
        if (m_entryPointHdr.quantizer == 0) {
            m_frameHdr.pquant = QuantizerTranslationTable[m_frameHdr.pqindex];
            m_frameHdr.pquantizer = (m_frameHdr.pqindex <= 8);
        }
        else {
            m_frameHdr.pquant = m_frameHdr.pqindex;
            if (m_entryPointHdr.quantizer == 1)
                READ(m_frameHdr.pquantizer);
            else if (m_entryPointHdr.quantizer == 2)
                m_frameHdr.pquantizer = 0;
            else if (m_entryPointHdr.quantizer == 3)
                m_frameHdr.pquantizer = 1;
            else
                assert(0);
        }

        if (m_seqHdr.postprocflag)
            READ_BITS(m_frameHdr.post_processing, 2);
        if ((m_frameHdr.picture_type == FRAME_I)
            || (m_frameHdr.picture_type == FRAME_BI)) {
            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                if (!decodeBitPlane(br, &m_bitPlanes.fieldtx[0], &m_frameHdr.fieldtx))
                    return false;
            }
            if (!decodeBitPlane(br, &m_bitPlanes.acpred[0], &m_frameHdr.ac_pred))
                return false;

            if ((m_entryPointHdr.overlap) && m_frameHdr.pquant <= 8) {
                m_frameHdr.condover = getFirst01Bit(br, 0, 2);
                if (m_frameHdr.condover == 2) {
                    if (!decodeBitPlane(br, &m_bitPlanes.overflags[0], &m_frameHdr.overflags))
                        return false;
                }
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.transacfrm2 = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);

            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);
        }
        else if (m_frameHdr.picture_type == FRAME_P) {
            if (m_frameHdr.fcm == FIELD_INTERLACE) {
                READ(m_frameHdr.numref);
                if (m_frameHdr.numref)
                    READ(m_frameHdr.reffield);
            }
            if (m_entryPointHdr.extended_mv)
                m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);
            if (m_frameHdr.fcm != PROGRESSIVE) {
                if (m_entryPointHdr.extended_dmv_flag)
                    m_frameHdr.dmvrange = getFirst01Bit(br, 0, 3);
            }

            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                READ(m_frameHdr.mvswitch4);
                READ(m_frameHdr.intcomp);
                if (m_frameHdr.intcomp) {
                    READ_BITS(m_frameHdr.lumscale, 6);
                    READ_BITS(m_frameHdr.lumshift, 6);
                }
            }
            else {
                m_frameHdr.mv_mode = getMVMode(br, m_frameHdr.pquant, false);
                if (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION) {
                    m_frameHdr.mv_mode2 = getMVMode(br, m_frameHdr.pquant, true);
                    if (m_frameHdr.fcm == FIELD_INTERLACE) {
                        temp = getFirst01Bit(br, 1, 2);
                        m_frameHdr.intcompfield = temp ? (3 - temp) : temp;
                    }
                    READ_BITS(m_frameHdr.lumscale, 6);
                    READ_BITS(m_frameHdr.lumshift, 6);
                    if ((m_frameHdr.fcm == FIELD_INTERLACE)
                        && m_frameHdr.intcompfield) {
                        READ_BITS(m_frameHdr.lumscale2, 6);
                        READ_BITS(m_frameHdr.lumshift2, 6);
                    }
                }
                if (m_frameHdr.fcm == PROGRESSIVE) {
                    if (m_frameHdr.mv_mode == MVMODE_MIXED_MV
                        || (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION
                               && m_frameHdr.mv_mode2 == MVMODE_MIXED_MV)) {
                        if (!decodeBitPlane(br, &m_bitPlanes.mvtypemb[0], &m_frameHdr.mv_type_mb))
                            return false;
                    }
                }
            }

            if (m_frameHdr.fcm != FIELD_INTERLACE) {
                if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                    return false;
            }

            if (m_frameHdr.fcm != PROGRESSIVE) {
                READ_BITS(m_frameHdr.mbmodetab, 2);
                READ_BITS(m_frameHdr.imvtab, 2);
                READ_BITS(m_frameHdr.icbptab, 3);
                if (m_frameHdr.fcm != FIELD_INTERLACE) {
                    READ_BITS(m_frameHdr.mvbptab2, 2);
                    if (m_frameHdr.mvswitch4)
                        READ_BITS(m_frameHdr.mvbptab4, 2);
                }
                else if (m_frameHdr.mv_mode == MVMODE_MIXED_MV) {
                    READ_BITS(m_frameHdr.mvbptab4, 2);
                }
            }
            else {
                READ_BITS(m_frameHdr.mv_table, 2);
                READ_BITS(m_frameHdr.cbp_table, 2);
            }
            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);

            if (m_entryPointHdr.variable_sized_transform_flag) {
                READ(m_frameHdr.mb_level_transform_type_flag);
                if (m_frameHdr.mb_level_transform_type_flag)
                    READ_BITS(m_frameHdr.frame_level_transform_type, 2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);
        }
        else if (m_frameHdr.picture_type == FRAME_B) {
            if (m_entryPointHdr.extended_mv)
                m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);
            if (m_frameHdr.fcm != PROGRESSIVE) {
                if (m_entryPointHdr.extended_dmv_flag)
                    m_frameHdr.dmvrange = getFirst01Bit(br, 0, 3);
            }
            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                READ(m_frameHdr.intcomp);
            }
            else {
                READ_BITS(m_frameHdr.mv_mode, 1);
                m_frameHdr.mv_mode = !(m_frameHdr.mv_mode);
            }
            if (m_frameHdr.fcm == FIELD_INTERLACE) {
                if (!decodeBitPlane(br, &m_bitPlanes.forwardmb[0], &m_frameHdr.forwardmb))
                    return false;
            }
            else {
                if (!decodeBitPlane(br, &m_bitPlanes.directmb[0], &m_frameHdr.direct_mb))
                    return false;
                if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                    return false;
            }
            if (m_frameHdr.fcm != PROGRESSIVE) {
                READ_BITS(m_frameHdr.mbmodetab, 2);
                READ_BITS(m_frameHdr.imvtab, 2);
                READ_BITS(m_frameHdr.icbptab, 3);

                if (m_frameHdr.fcm == FRAME_INTERLACE)
                    READ_BITS(m_frameHdr.mvbptab2, 2);

                if ((m_frameHdr.fcm == FRAME_INTERLACE)
                    || ((m_frameHdr.fcm == FIELD_INTERLACE)
                           && (m_frameHdr.mv_mode == MVMODE_MIXED_MV)))
                    READ_BITS(m_frameHdr.mvbptab4, 2);
            }
            else {
                READ_BITS(m_frameHdr.mv_table, 2);
                READ_BITS(m_frameHdr.cbp_table, 2);
            }

            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);
            if (m_entryPointHdr.variable_sized_transform_flag) {
                READ(m_frameHdr.mb_level_transform_type_flag);
                if (m_frameHdr.mb_level_transform_type_flag)
                    READ_BITS(m_frameHdr.frame_level_transform_type, 2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            READ(m_frameHdr.intra_transform_dc_table);
        }
        return true;
    }
}
}
