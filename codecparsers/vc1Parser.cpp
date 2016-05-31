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

#include "vc1Parser.h"
#include <cstring>
#include <cassert>

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
        m_seqHdr.profile = bitReader.read(2);
        if (m_seqHdr.profile != PROFILE_ADVANCED) {
            /* skip reserved bits */
            bitReader.skip(2);
            m_seqHdr.frmrtq_postproc = bitReader.read(3);
            m_seqHdr.bitrtq_postproc = bitReader.read(5);
            m_seqHdr.loop_filter = bitReader.read(1);
            /* skip reserved bits */
            bitReader.skip(1);
            m_seqHdr.multires = bitReader.read(1);
            /* skip reserved bits */
            bitReader.skip(1);
            m_seqHdr.fastuvmc = bitReader.read(1);
            m_seqHdr.extended_mv = bitReader.read(1);
            m_seqHdr.dquant = bitReader.read(2);
            m_seqHdr.variable_sized_transform_flag = bitReader.read(1);
            /* skip reserved bits */
            bitReader.skip(1);
            m_seqHdr.overlap = bitReader.read(1);
            m_seqHdr.syncmarker = bitReader.read(1);
            m_seqHdr.rangered = bitReader.read(1);
            m_seqHdr.max_b_frames = bitReader.read(3);
            m_seqHdr.quantizer = bitReader.read(2);
            m_seqHdr.finterpflag = bitReader.read(1);
        }
        else {
            m_seqHdr.level = bitReader.read(3);
            m_seqHdr.colordiff_format = bitReader.read(2);
            m_seqHdr.frmrtq_postproc = bitReader.read(3);
            m_seqHdr.bitrtq_postproc = bitReader.read(5);
            m_seqHdr.postprocflag = bitReader.read(1);
            m_seqHdr.coded_width = (bitReader.read(12) + 1) << 1;
            m_seqHdr.coded_height = (bitReader.read(12) + 1) << 1;
            m_seqHdr.pulldown = bitReader.read(1);
            m_seqHdr.interlace = bitReader.read(1);
            m_seqHdr.tfcntrflag = bitReader.read(1);
            m_seqHdr.finterpflag = bitReader.read(1);
            /* skip reserved bits */
            bitReader.skip(1);
            m_seqHdr.psf = bitReader.read(1);
            m_seqHdr.display_ext = bitReader.read(1);
            if (m_seqHdr.display_ext == 1) {
                m_seqHdr.disp_horiz_size = bitReader.read(14) + 1;
                m_seqHdr.disp_vert_size = bitReader.read(14) + 1;
                m_seqHdr.aspect_ratio_flag = bitReader.read(1);
                if (m_seqHdr.aspect_ratio_flag == 1) {
                    m_seqHdr.aspect_ratio = bitReader.read(4);
                    if (m_seqHdr.aspect_ratio == 15) {
                        m_seqHdr.aspect_horiz_size = bitReader.read(8);
                        m_seqHdr.aspect_vert_size = bitReader.read(8);
                    }
                }
                m_seqHdr.framerate_flag = bitReader.read(1);
                if (m_seqHdr.framerate_flag == 1) {
                    m_seqHdr.framerateind = bitReader.read(1);
                    if (m_seqHdr.framerateind == 0) {
                        m_seqHdr.frameratenr = bitReader.read(8);
                        m_seqHdr.frameratedr = bitReader.read(4);
                    }
                    else {
                        m_seqHdr.framerateexp = bitReader.read(16);
                    }
                }
                m_seqHdr.color_format_flag = bitReader.read(1);

                if (m_seqHdr.color_format_flag == 1) {
                    m_seqHdr.color_prim = bitReader.read(8);
                    m_seqHdr.transfer_char = bitReader.read(8);
                    m_seqHdr.matrix_coef = bitReader.read(8);
                }
            }
            m_seqHdr.hrd_param_flag = bitReader.read(1);

            /*Table 13: Syntax elements for HRD_PARAM structure*/
            if (m_seqHdr.hrd_param_flag == 1) {
                m_seqHdr.hrd_param.hrd_num_leaky_buckets = bitReader.read(5);
                m_seqHdr.hrd_param.bit_rate_exponent = bitReader.read(4);
                m_seqHdr.hrd_param.buffer_size_exponent = bitReader.read(4);
                for (i = 0; i < m_seqHdr.hrd_param.hrd_num_leaky_buckets; i++) {
                    m_seqHdr.hrd_param.hrd_rate[i] = bitReader.read(16);
                    m_seqHdr.hrd_param.hrd_buffer[i] = bitReader.read(16);
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
        m_entryPointHdr.broken_link = bitReader.read(1);
        m_entryPointHdr.closed_entry = bitReader.read(1);
        m_entryPointHdr.panscan_flag = bitReader.read(1);
        m_entryPointHdr.reference_distance_flag = bitReader.read(1);
        m_entryPointHdr.loopfilter = bitReader.read(1);
        m_entryPointHdr.fastuvmc = bitReader.read(1);
        m_entryPointHdr.extended_mv = bitReader.read(1);
        m_entryPointHdr.dquant = bitReader.read(2);
        m_entryPointHdr.variable_sized_transform_flag = bitReader.read(1);
        m_entryPointHdr.overlap = bitReader.read(1);
        m_entryPointHdr.quantizer = bitReader.read(2);

        if (m_seqHdr.hrd_param_flag) {
            for (i = 0; i < m_seqHdr.hrd_param.hrd_num_leaky_buckets; i++)
                m_entryPointHdr.hrd_full[i] = bitReader.read(8);
        }

        m_entryPointHdr.coded_size_flag = bitReader.read(1);
        if (m_entryPointHdr.coded_size_flag) {
            m_entryPointHdr.coded_width = (bitReader.read(12) + 1) << 1;
            m_entryPointHdr.coded_height = (bitReader.read(12) + 1) << 1;
            m_seqHdr.coded_width = m_entryPointHdr.coded_width;
            m_seqHdr.coded_height = m_entryPointHdr.coded_height;
        }

        if (m_entryPointHdr.extended_mv)
            m_entryPointHdr.extended_dmv_flag = bitReader.read(1);

        m_entryPointHdr.range_mapy_flag = bitReader.read(1);
        if (m_entryPointHdr.range_mapy_flag)
            m_entryPointHdr.range_mapy = bitReader.read(3);

        m_entryPointHdr.range_mapuv_flag = bitReader.read(1);
        if (m_entryPointHdr.range_mapy_flag)
            m_entryPointHdr.range_mapuv = bitReader.read(3);
        return true;
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
                codeWord = br->peek(numberOfBits);
            }
            if (codeWord == table[i].codeWord) {
                *out = i;
                br->skip(numberOfBits);
                break;
            }
            i++;
        }
        return (i < tableLen) ? true : false;
    }

    /* 8.7.3.6 Row-skip mode*/
    void Parser::decodeRowskipMode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i, j;
        for (j = 0; j < height; j++) {
            if (br->read(1)) {
                for (i = 0; i < width; i++)
                    data[i] = br->read(1);
            }
            else {
                memset(data, 0, width);
            }
            data += m_mbWidth;
        }
    }

    /* 8.7.3.7 Column-skip mode*/
    void Parser::decodeColskipMode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i, j;
        for (i = 0; i < width; i++) {
            if (br->read(1)) {
                for (j = 0; j < height; j++)
                    data[j * m_mbWidth] = br->read(1);
            }
            else {
                for (j = 0; j < height; j++)
                    data[j * m_mbWidth] = 0;
            }
            data++;
        }
    }

    /* Table 80: Norm-2/Diff-2 Code Table */
    bool Parser::decodeNorm2Mode(BitReader* br, uint8_t* data, uint32_t width, uint32_t height)
    {
        uint32_t i;
        uint16_t temp;
        uint8_t* out = data;
        if ((height * width) & 1)
            *out++ = br->read(1);
        for (i = (height * width) & 1; i < height * width; i += 2) {
            temp = br->read(1);
            if (temp == 0) {
                *out++ = 0;
                *out++ = 0;
            }
            else {
                temp = br->read(1);
                if (temp == 1) {
                    *out++ = 1;
                    *out++ = 1;
                }
                else {
                    temp = br->read(1);
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
        invert = br->read(1);
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
            m_frameHdr.pq_diff = br->read(3);
            if (m_frameHdr.pq_diff != 7) {
                m_frameHdr.alt_pic_quantizer = m_frameHdr.pquant + m_frameHdr.pq_diff + 1;
            }
            else {
                m_frameHdr.abs_pq = br->read(5);
                m_frameHdr.alt_pic_quantizer = m_frameHdr.abs_pq;
            }
        }
        else {
            m_frameHdr.dq_frame = br->read(1);
            if (m_frameHdr.dq_frame == 1) {
                m_frameHdr.dq_profile = br->read(2);
                if (m_frameHdr.dq_profile == DQPROFILE_SINGLE_EDGE) {
                    m_frameHdr.dq_sb_edge = br->read(2);
                }
                else if (m_frameHdr.dq_profile == DQPROFILE_DOUBLE_EDGE) {
                    m_frameHdr.dq_db_edge = br->read(2);
                }
                else if (m_frameHdr.dq_profile == DQPROFILE_ALL_MACROBLOCKS) {
                    m_frameHdr.dq_binary_level = br->read(1);
                }
                if (!((m_frameHdr.dq_profile == DQPROFILE_ALL_MACROBLOCKS)
                        && (m_frameHdr.dq_binary_level == 0))) {
                    m_frameHdr.pq_diff = br->read(3);
                    if (m_frameHdr.pq_diff != 7) {
                        m_frameHdr.alt_pic_quantizer = m_frameHdr.pquant + m_frameHdr.pq_diff + 1;
                    }
                    else {
                        m_frameHdr.abs_pq = br->read(5);
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

    /*Table 16: Progressive I picture layer bitstream for Simple and Main Profile*/
    /*Table 17: Progressive BI picture layer bitstream for Main Profile*/
    /*Table 19: Progressive P picture layer bitstream for Simple and Main Profile*/
    /*Table 21: Progressive B picture layer bitstream for Main Profile*/
    bool Parser::parseFrameHeaderSimpleMain(BitReader* br)
    {
        uint32_t temp;
        m_frameHdr.interpfrm = 0;
        if (m_seqHdr.finterpflag)
            m_frameHdr.interpfrm = br->read(1);

        br->skip(2);
        m_frameHdr.range_reduction_frame = 0;
        if (m_seqHdr.rangered)
            m_frameHdr.range_reduction_frame = br->read(1);

        temp = br->read(1);
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
                if (br->read(1))
                    m_frameHdr.picture_type = FRAME_I;
                else
                    m_frameHdr.picture_type = FRAME_B;
            }
            else {
                m_frameHdr.picture_type = FRAME_P;
            }
        }

        if (m_frameHdr.picture_type == FRAME_I
            || m_frameHdr.picture_type == FRAME_BI)
            br->skip(7); /* skip BF*/
        m_frameHdr.pqindex = br->read(5);
        if (m_frameHdr.pqindex <= 8)
            m_frameHdr.halfqp = br->read(1);
        m_frameHdr.pquantizer = 1;
        if (m_seqHdr.quantizer == 0) {
            m_frameHdr.pquant = QuantizerTranslationTable[m_frameHdr.pqindex];
            m_frameHdr.pquantizer = (m_frameHdr.pqindex <= 8) ? 1 : 0;
        }
        else {
            m_frameHdr.pquant = m_frameHdr.pqindex;
            if (m_seqHdr.quantizer == 1)
                m_frameHdr.pquantizer = br->read(1);
        }

        if (m_seqHdr.extended_mv == 1)
            m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);

        if (m_seqHdr.multires
            && ((m_frameHdr.picture_type == FRAME_I)
                   || (m_frameHdr.picture_type == FRAME_P))) {
            m_frameHdr.picture_resolution_index = br->read(2);
        }

        if ((m_frameHdr.picture_type == FRAME_I)
            || (m_frameHdr.picture_type == FRAME_BI)) {
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.transacfrm2 = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);
        }
        else if (m_frameHdr.picture_type == FRAME_P) {
            m_frameHdr.mv_mode = getMVMode(br, m_frameHdr.pquant, false);
            if (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION) {
                m_frameHdr.mv_mode2 = getMVMode(br, m_frameHdr.pquant, true);
                m_frameHdr.lumscale = br->read(6);
                m_frameHdr.lumshift = br->read(6);
            }
            if (m_frameHdr.mv_mode == MVMODE_MIXED_MV
                || (m_frameHdr.mv_mode == MVMODE_INTENSITY_COMPENSATION
                       && m_frameHdr.mv_mode2 == MVMODE_MIXED_MV)) {
                if (!decodeBitPlane(br, &m_bitPlanes.mvtypemb[0], &m_frameHdr.mv_type_mb))
                    return false;
            }
            if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                return false;

            m_frameHdr.mv_table = br->read(2);
            m_frameHdr.cbp_table = br->read(2);
            if (m_seqHdr.dquant)
                parseVopdquant(br, m_seqHdr.dquant);

            if (m_seqHdr.variable_sized_transform_flag == 1) {
                m_frameHdr.mb_level_transform_type_flag = br->read(1);
                if (m_frameHdr.mb_level_transform_type_flag == 1)
                    m_frameHdr.frame_level_transform_type = br->read(2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);
        }
        else if (m_frameHdr.picture_type == FRAME_B) {
            m_frameHdr.mv_mode = br->read(1);
            if (!decodeBitPlane(br, &m_bitPlanes.directmb[0], &m_frameHdr.direct_mb))
                return false;
            if (!decodeBitPlane(br, &m_bitPlanes.skipmb[0], &m_frameHdr.skip_mb))
                return false;
            m_frameHdr.mv_table = br->read(2);
            m_frameHdr.cbp_table = br->read(2);
            if (m_seqHdr.dquant)
                parseVopdquant(br, m_seqHdr.dquant);
            if (m_seqHdr.variable_sized_transform_flag == 1) {
                m_frameHdr.mb_level_transform_type_flag = br->read(1);
                if (m_frameHdr.mb_level_transform_type_flag == 1)
                    m_frameHdr.frame_level_transform_type = br->read(2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);
        }
        return true;
    }

    /* 9.1.1.43 P Reference Distance*/
    /* Table 106: REFDIST VLC Table*/
    uint8_t Parser::getRefDist(BitReader* br)
    {
        if (br->peek(2) != 3) {
            return br->read(2);
        }
        else {
            return getFirst01Bit(br, 0, 16) + 1;
        }
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
            temp = br->read(3);
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
            br->skip(8);

        if (m_seqHdr.pulldown) {
            if (!(m_seqHdr.interlace) || m_seqHdr.psf) {
                m_frameHdr.rptfrm = br->read(2);
            }
            else {
                m_frameHdr.tff = br->read(1);
                m_frameHdr.rff = br->read(1);
            }
        }
        else {
            m_frameHdr.tff = 1;
        }

        if (m_frameHdr.picture_type == FRAME_SKIPPED)
            return true;

        m_frameHdr.rounding_control = br->read(1);

        if (m_seqHdr.interlace) {
            m_frameHdr.uvsamp = br->read(1);
            if ((m_frameHdr.fcm == FIELD_INTERLACE)
                && m_entryPointHdr.reference_distance_flag
                && (m_frameHdr.picture_type != FRAME_B)
                && (m_frameHdr.picture_type != FRAME_BI)) {
                m_frameHdr.refdist = getRefDist(br);
            }
        }
        if (m_seqHdr.finterpflag)
            m_frameHdr.interpfrm = br->read(1);
        m_frameHdr.pqindex = br->read(5);
        if (m_frameHdr.pqindex <= 8)
            m_frameHdr.halfqp = br->read(1);
        m_frameHdr.pquantizer = 1;
        if (m_entryPointHdr.quantizer == 0) {
            m_frameHdr.pquant = QuantizerTranslationTable[m_frameHdr.pqindex];
            m_frameHdr.pquantizer = (m_frameHdr.pqindex <= 8);
        }
        else {
            m_frameHdr.pquant = m_frameHdr.pqindex;
            if (m_entryPointHdr.quantizer == 1)
                m_frameHdr.pquantizer = br->read(1);
        }

        if (m_seqHdr.postprocflag == 1)
            m_frameHdr.post_processing = br->read(2);
        if ((m_frameHdr.picture_type == FRAME_I)
            || (m_frameHdr.picture_type == FRAME_BI)) {
            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                if (!decodeBitPlane(br, &m_bitPlanes.fieldtx[0], &m_frameHdr.fieldtx))
                    return false;
            }
            if (!decodeBitPlane(br, &m_bitPlanes.acpred[0], &m_frameHdr.ac_pred))
                return false;

            if ((m_entryPointHdr.overlap == 1) && m_frameHdr.pquant <= 8) {
                m_frameHdr.condover = getFirst01Bit(br, 0, 2);
                if (m_frameHdr.condover == 2) {
                    if (!decodeBitPlane(br, &m_bitPlanes.overflags[0], &m_frameHdr.overflags))
                        return false;
                }
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.transacfrm2 = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);

            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);
        }
        else if (m_frameHdr.picture_type == FRAME_P) {
            if (m_frameHdr.fcm == FIELD_INTERLACE) {
                m_frameHdr.numref = br->read(1);
                if (m_frameHdr.numref)
                    m_frameHdr.reffield = br->read(1);
            }
            if (m_entryPointHdr.extended_mv)
                m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);
            if (m_frameHdr.fcm != PROGRESSIVE) {
                if (m_entryPointHdr.extended_dmv_flag)
                    m_frameHdr.dmvrange = getFirst01Bit(br, 0, 3);
            }

            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                m_frameHdr.mvswitch4 = br->read(1);
                m_frameHdr.intcomp = br->read(1);
                if (m_frameHdr.intcomp) {
                    m_frameHdr.lumscale = br->read(6);
                    m_frameHdr.lumshift = br->read(6);
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
                    m_frameHdr.lumscale = br->read(6);
                    m_frameHdr.lumshift = br->read(6);
                    if ((m_frameHdr.fcm == FIELD_INTERLACE)
                        && m_frameHdr.intcompfield) {
                        m_frameHdr.lumscale2 = br->read(6);
                        m_frameHdr.lumshift2 = br->read(6);
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
                m_frameHdr.mbmodetab = br->read(2);
                m_frameHdr.imvtab = br->read(2);
                m_frameHdr.icbptab = br->read(3);
                if (m_frameHdr.fcm != FIELD_INTERLACE) {
                    m_frameHdr.mvbptab2 = br->read(2);
                    if (m_frameHdr.mvswitch4)
                        m_frameHdr.mvbptab4 = br->read(2);
                }
                else if (m_frameHdr.mv_mode == MVMODE_MIXED_MV) {
                    m_frameHdr.mvbptab4 = br->read(2);
                }
            }
            else {
                m_frameHdr.mv_table = br->read(2);
                m_frameHdr.cbp_table = br->read(2);
            }
            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);

            if (m_entryPointHdr.variable_sized_transform_flag) {
                m_frameHdr.mb_level_transform_type_flag = br->read(1);
                if (m_frameHdr.mb_level_transform_type_flag)
                    m_frameHdr.frame_level_transform_type = br->read(2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);
        }
        else if (m_frameHdr.picture_type == FRAME_B) {
            if (m_entryPointHdr.extended_mv)
                m_frameHdr.extended_mv_range = getFirst01Bit(br, 0, 3);
            if (m_frameHdr.fcm != PROGRESSIVE) {
                if (m_entryPointHdr.extended_dmv_flag)
                    m_frameHdr.dmvrange = getFirst01Bit(br, 0, 3);
            }
            if (m_frameHdr.fcm == FRAME_INTERLACE) {
                m_frameHdr.intcomp = br->read(1);
            }
            else {
                m_frameHdr.mv_mode = br->read(1);
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
                m_frameHdr.mbmodetab = br->read(2);
                m_frameHdr.imvtab = br->read(2);
                m_frameHdr.icbptab = br->read(3);

                if (m_frameHdr.fcm == FRAME_INTERLACE)
                    m_frameHdr.mvbptab2 = br->read(2);

                if ((m_frameHdr.fcm == FRAME_INTERLACE)
                    || ((m_frameHdr.fcm == FIELD_INTERLACE)
                           && (m_frameHdr.mv_mode == MVMODE_MIXED_MV)))
                    m_frameHdr.mvbptab4 = br->read(2);
            }
            else {
                m_frameHdr.mv_table = br->read(2);
                m_frameHdr.cbp_table = br->read(2);
            }

            if (m_entryPointHdr.dquant)
                parseVopdquant(br, m_entryPointHdr.dquant);
            if (m_entryPointHdr.variable_sized_transform_flag) {
                m_frameHdr.mb_level_transform_type_flag = br->read(1);
                if (m_frameHdr.mb_level_transform_type_flag)
                    m_frameHdr.frame_level_transform_type = br->read(2);
            }
            m_frameHdr.transacfrm = getFirst01Bit(br, 0, 2);
            m_frameHdr.intra_transform_dc_table = br->read(1);
        }
        return true;
    }
}
}
