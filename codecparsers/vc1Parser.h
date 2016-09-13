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

#ifndef __VC1_PARSER_H__
#define __VC1_PARSER_H__

#include <stdint.h>
#include <stdlib.h>
#include "bitReader.h"
#include <vector>

namespace YamiParser {
namespace VC1 {
    /*6.1.1 Profile(PROFILE)(2 bits)*/
    enum Profile {
        PROFILE_SIMPLE = 0,
        PROFILE_MAIN = 1,
        PROFILE_ADVANCED = 3
    };

    enum FrameType {
        FRAME_I,
        FRAME_P,
        FRAME_B,
        FRAME_BI,
        FRAME_SKIPPED
    };

    enum BitPlaneMode {
        IMODE_NORM2,
        IMODE_NORM6,
        IMODE_ROWSKIP,
        IMODE_COLSKIP,
        IMODE_DIFF2,
        IMODE_DIFF6,
        IMODE_RAW,
    };

    /*Table 43: Macroblock Quantization Profile(DQPROFILE) Code Table*/
    enum DQProfile {
        DQPROFILE_ALL_FOUR_EDGES,
        DQPROFILE_DOUBLE_EDGE,
        DQPROFILE_SINGLE_EDGE,
        DQPROFILE_ALL_MACROBLOCKS
    };

    enum MvMode {
        MVMODE_1_MV,
        MVMODE_1_MV_HALF_PEL,
        MVMODE_1_MV_HALF_PEL_BILINEAR,
        MVMODE_MIXED_MV,
        MVMODE_INTENSITY_COMPENSATION
    };

    /*Table 13: Syntax elements for HRD_PARAM structure*/
    struct HrdParam {
        uint8_t hrd_num_leaky_buckets;
        uint8_t bit_rate_exponent;
        uint8_t buffer_size_exponent;
        uint16_t hrd_rate[31];
        uint16_t hrd_buffer[31];
    };

    /*Table 14: Entry-point layer bitstream for Advanced Profile*/
    struct EntryPointHdr {
        bool broken_link;
        bool closed_entry;
        bool panscan_flag;
        bool reference_distance_flag;
        bool loopfilter;
        bool fastuvmc;
        bool extended_mv;
        uint8_t dquant;
        bool variable_sized_transform_flag;
        bool overlap;
        uint8_t quantizer;
        bool coded_size_flag;
        uint16_t coded_width;
        uint16_t coded_height;
        bool extended_dmv_flag;
        bool range_mapy_flag;
        uint8_t range_mapy;
        bool range_mapuv_flag;
        uint8_t range_mapuv;
        uint8_t hrd_full[31];
    };

    /*Table 3: Sequence layer bitstream for Advanced Profile*/
    /*Table 263: Sequence Header Data Structure STRUCT_C for Simple and Main Profiles*/
    struct SeqHdr {
        uint8_t profile;
        uint16_t coded_width;
        uint16_t coded_height;
        uint8_t frmrtq_postproc;
        uint8_t bitrtq_postproc;
        bool loop_filter;
        bool multires;
        bool fastuvmc;
        bool extended_mv;
        uint8_t dquant;
        bool variable_sized_transform_flag;
        bool overlap;
        bool syncmarker;
        bool rangered;
        uint8_t max_b_frames;
        uint8_t quantizer;
        bool finterpflag;
        uint8_t level;
        uint8_t colordiff_format;
        bool postprocflag;
        bool pulldown;
        bool interlace;
        bool tfcntrflag;
        bool psf;
        bool display_ext;
        uint16_t disp_horiz_size;
        uint16_t disp_vert_size;
        bool aspect_ratio_flag;
        uint8_t aspect_ratio;
        uint8_t aspect_horiz_size;
        uint8_t aspect_vert_size;
        bool framerate_flag;
        bool framerateind;
        uint8_t frameratenr;
        uint8_t frameratedr;
        uint16_t framerateexp;
        bool color_format_flag;
        uint8_t color_prim;
        uint8_t transfer_char;
        uint8_t matrix_coef;
        bool hrd_param_flag;
        HrdParam hrd_param;
    };

    struct BitPlanes {
        std::vector<uint8_t> acpred;
        std::vector<uint8_t> fieldtx;
        std::vector<uint8_t> overflags;
        std::vector<uint8_t> mvtypemb;
        std::vector<uint8_t> skipmb;
        std::vector<uint8_t> directmb;
        std::vector<uint8_t> forwardmb;
    };

    struct FrameHdr {
        uint8_t picture_type;
        bool interpfrm;
        bool halfqp;
        uint8_t transacfrm;
        bool intra_transform_dc_table;
        uint8_t pqindex;
        bool pquantizer;
        uint8_t pquant;
        uint8_t pq_diff;
        uint8_t abs_pq;
        bool dq_frame;
        uint8_t dq_profile;
        uint8_t dq_sb_edge;
        uint8_t dq_db_edge;
        bool dq_binary_level;
        uint8_t alt_pic_quantizer;
        uint8_t extended_mv_range;
        bool range_reduction_frame;
        uint8_t picture_resolution_index;
        uint8_t transacfrm2;
        uint8_t mv_mode;
        uint8_t mv_table;
        bool mb_level_transform_type_flag;
        uint8_t mv_mode2;
        uint8_t lumscale;
        uint8_t lumshift;
        uint8_t cbp_table;
        uint8_t frame_level_transform_type;
        bool mv_type_mb;
        bool skip_mb;
        bool direct_mb;
        uint8_t fcm;
        uint8_t rptfrm;
        bool tff;
        bool rff;
        bool rounding_control;
        bool uvsamp;
        uint8_t post_processing;
        uint8_t condover;
        bool ac_pred;
        bool overflags;
        bool forwardmb;
        bool fieldtx;
        bool intcomp;
        uint8_t dmvrange;
        uint8_t mbmodetab;
        uint8_t imvtab;
        uint8_t icbptab;
        uint8_t mvbptab2;
        uint8_t mvbptab4;
        bool mvswitch4;
        uint8_t refdist;
        uint8_t fptype;
        bool numref;
        bool reffield;
        uint8_t lumscale2;
        uint8_t lumshift2;
        uint8_t intcompfield;
        uint16_t bfraction;
        uint32_t macroblock_offset;
    };

    struct VLCTable {
        uint16_t codeWord;
        uint16_t codeLength;
    };

    /*Table 41: Frame Coding Mode VLC */
    enum FrameCodingMode {
        PROGRESSIVE,
        FRAME_INTERLACE,
        FIELD_INTERLACE
    };

    struct SliceHdr {
        uint16_t slice_addr;
        uint32_t macroblock_offset;
    };

    class Parser {
    public:
        Parser();
        ~Parser();
        bool parseCodecData(uint8_t*, uint32_t);
        bool parseFrameHeader(uint8_t*&, uint32_t&);
        bool parseSliceHeader(uint8_t*, uint32_t);
        int32_t searchStartCode(uint8_t*, uint32_t);
        SeqHdr m_seqHdr;
        FrameHdr m_frameHdr;
        SliceHdr m_sliceHdr;
        EntryPointHdr m_entryPointHdr;
        BitPlanes m_bitPlanes;
        uint32_t m_mbWidth;
        uint32_t m_mbHeight;

    private:
        void mallocBitPlanes();
        bool getRefDist(BitReader*, uint8_t& refDist);
        int32_t getFirst01Bit(BitReader*, bool, uint32_t);
        uint8_t getMVMode(BitReader*, uint8_t, bool);
        bool decodeBFraction(BitReader*);
        bool convertToRbdu(uint8_t*&, uint32_t&);
        bool decodeVLCTable(BitReader*, uint16_t*, const VLCTable*, uint32_t);
        bool decodeRowskipMode(BitReader*, uint8_t*, uint32_t, uint32_t);
        bool decodeColskipMode(BitReader*, uint8_t*, uint32_t, uint32_t);
        bool decodeNorm2Mode(BitReader*, uint8_t*, uint32_t, uint32_t);
        bool decodeNorm6Mode(BitReader*, uint8_t*, uint32_t, uint32_t);
        bool decodeBitPlane(BitReader*, uint8_t*, bool*);
        void inverseDiff(uint8_t*, uint32_t, uint32_t, uint32_t);
        bool parseVopdquant(BitReader*, uint8_t);
        bool parseSequenceHeader(const uint8_t*, uint32_t);
        bool parseEntryPointHeader(const uint8_t*, uint32_t);
        bool parseFrameHeaderSimpleMain(BitReader*);
        bool parseFrameHeaderAdvanced(BitReader*);
        std::vector<uint8_t> m_rbdu;
    };
}
}
#endif
