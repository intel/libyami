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

// primary header
#include "mpeg2_parser.h"

// library headers
#include "common/Array.h"
#include "common/log.h"
#include "common/unittest.h"

// system headers
#include <limits>
#include <time.h> //using rand

namespace YamiParser {
namespace MPEG2 {

    const static std::array<const uint8_t, 9> SequenceHeader
        = { 0xb3, 0x20, 0x01, 0x20, 0x34, 0xff, 0xff, 0xe0, 0x18 };

    const static std::array<const uint8_t, 7> SequenceExtension
        = { 0xb5, 0x14, 0x8a, 0x00, 0x01, 0x00, 0x00 };

    const static std::array<const uint8_t, 5> GroupOfPicturesHeader
        = { 0xb8, 0x00, 0x08, 0x06, 0x00 };

    const static std::array<const uint8_t, 5> PictureHeaderArray
        = { 0x00, 0x00, 0x0f, 0xff, 0xf8 };

    const static std::array<const uint8_t, 6> PictureCodingExtensionArray
        = { 0xb5, 0x8f, 0xff, 0xf3, 0x41, 0x80 };

    const static std::array<const uint8_t, 124> SliceArray
        = { 0x01, 0x13, 0xf8, 0x7d, 0x29, 0x48, 0x8b, 0x94, 0xa5, 0x22, 0x2e,
            0x52, 0x94, 0x88, 0xb9, 0x4a, 0x52, 0x22, 0xe5, 0x29, 0x48, 0x8b,
            0x94, 0xa5, 0x22, 0x2e, 0x52, 0x94, 0x88, 0xb9, 0x4a, 0x52, 0x22,
            0xe5, 0x29, 0x48, 0x8b, 0x94, 0xa5, 0x22, 0x2e, 0x52, 0x94, 0x88,
            0xb9, 0x4a, 0x52, 0x22, 0xe5, 0x29, 0x48, 0x8b, 0x94, 0xa5, 0x22,
            0x2e, 0x52, 0x94, 0x88, 0xb9, 0x4a, 0x52, 0x22, 0xe5, 0x29, 0x48,
            0x8b, 0x94, 0xa5, 0x22, 0x2e, 0x52, 0x94, 0x88, 0xb9, 0x4a, 0x52,
            0x22, 0xe5, 0x29, 0x48, 0x8b, 0x94, 0xa5, 0x22, 0x2e, 0x52, 0x94,
            0x88, 0xb9, 0x4a, 0x52, 0x22, 0xe5, 0x29, 0x48, 0x8b, 0x94, 0xa5,
            0x22, 0x2e, 0x52, 0x94, 0x88, 0xb9, 0x4a, 0x52, 0x22, 0xe5, 0x29,
            0x48, 0x8b, 0x94, 0xa5, 0x22, 0x2e, 0x52, 0x94, 0x88, 0xb9, 0x4a,
            0x52, 0x22, 0x00 };

    class MPEG2ParserTest : public ::testing::Test {
    protected:

#define MEMBER_VARIABLE(result_type, var)                                      \
    result_type& var(Parser& p) { return p.var; }

        MEMBER_VARIABLE(BitReader*, m_bitReader);
        MEMBER_VARIABLE(SeqHeader, m_sequenceHdr);
        MEMBER_VARIABLE(SeqExtension, m_sequenceExtension);
        MEMBER_VARIABLE(GOPHeader, m_GOPHeader);
        MEMBER_VARIABLE(PictureHeader, m_pictureHeader);
        MEMBER_VARIABLE(PictureCodingExtension, m_pictureCodingExtension);
        MEMBER_VARIABLE(Slice, m_slice);

#undef MEMBER_VARIABLE

        void checkParamsSeqHeader(const Parser& parser)
        {
            EXPECT_EQ(0x200u, parser.m_sequenceHdr.horizontal_size_value);
            EXPECT_EQ(0x120u, parser.m_sequenceHdr.vertical_size_value);
            EXPECT_EQ(3u, parser.m_sequenceHdr.aspect_ratio_info);
            EXPECT_EQ(4u, parser.m_sequenceHdr.frame_rate_code);
            EXPECT_EQ(0x3ffffu, parser.m_sequenceHdr.bit_rate_value);
            EXPECT_EQ(3u, parser.m_sequenceHdr.vbv_buffer_size_value);
        }

        void checkParamsSeqExtension(const Parser& parser)
        {
            EXPECT_EQ(
                kSequence,
                parser.m_sequenceExtension.extension_start_code_identifier);
            EXPECT_EQ(0x48u,
                      parser.m_sequenceExtension.profile_and_level_indication);
            EXPECT_EQ(1, parser.m_sequenceExtension.progressive_sequence);
            EXPECT_EQ(1u, parser.m_sequenceExtension.chroma_format);
        }

        void checkParamsPictureHeader(const Parser& parser)
        {
            EXPECT_EQ(1u, parser.m_pictureHeader.picture_coding_type);
            EXPECT_EQ(0xffffu, parser.m_pictureHeader.vbv_delay);
        }

        void checkParamsPictureCodingExtension(const Parser& parser)
        {
            EXPECT_EQ(0xfu, parser.m_pictureCodingExtension.f_code[0][0]);
            EXPECT_EQ(0xfu, parser.m_pictureCodingExtension.f_code[0][1]);
            EXPECT_EQ(0xfu, parser.m_pictureCodingExtension.f_code[1][0]);
            EXPECT_EQ(0xfu, parser.m_pictureCodingExtension.f_code[1][1]);
            EXPECT_EQ(0u, parser.m_pictureCodingExtension.intra_dc_precision);
            EXPECT_EQ(3u, parser.m_pictureCodingExtension.picture_structure);
            EXPECT_EQ(1, parser.m_pictureCodingExtension.frame_pred_frame_dct);
            EXPECT_EQ(1, parser.m_pictureCodingExtension.chrome_420_type);
            EXPECT_EQ(1, parser.m_pictureCodingExtension.progressive_frame);
        }

        void checkParamsSlice(const Parser& parser)
        {
            EXPECT_EQ(14u, parser.m_slice.sliceHeaderSize);
            EXPECT_EQ(0u, parser.m_slice.macroblockRow);
            EXPECT_EQ(0u, parser.m_slice.macroblockColumn);
        }
    };

#define MPEG2_PARSER_TEST(name) TEST_F(MPEG2ParserTest, name)

    MPEG2_PARSER_TEST(ParseSequenceHeader)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &SequenceHeader[0];
        streamParser.nalSize = SequenceHeader.size();

        EXPECT_TRUE(parser.parseSequenceHeader(&streamParser));
        checkParamsSeqHeader(parser);
    }

    MPEG2_PARSER_TEST(ParseSequenceExtension)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &SequenceExtension[0];
        streamParser.nalSize = SequenceExtension.size();

        EXPECT_TRUE(parser.parseSequenceExtension(&streamParser));
        checkParamsSeqExtension(parser);
    }

    MPEG2_PARSER_TEST(ParseGOPHeader)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &GroupOfPicturesHeader[0];
        streamParser.nalSize = GroupOfPicturesHeader.size();

        EXPECT_TRUE(parser.parseGOPHeader(&streamParser));
        //all params will be zero for this unittest
    }

    MPEG2_PARSER_TEST(ParsePictureHeader)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &PictureHeaderArray[0];
        streamParser.nalSize = PictureHeaderArray.size();

        EXPECT_TRUE(parser.parsePictureHeader(&streamParser));
        checkParamsPictureHeader(parser);
    }

    MPEG2_PARSER_TEST(ParsePictureCodingExtension)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &PictureCodingExtensionArray[0];
        streamParser.nalSize = PictureCodingExtensionArray.size();

        EXPECT_TRUE(parser.parsePictureCodingExtension(&streamParser));
        checkParamsPictureCodingExtension(parser);
    }

    MPEG2_PARSER_TEST(ParseSlice)
    {
        Parser parser;
        StreamHeader streamParser;

        streamParser.nalData = &SliceArray[0];
        streamParser.nalSize = SliceArray.size();

        EXPECT_TRUE(parser.parseSlice(&streamParser));
        checkParamsSlice(parser);
    }
} // namespace MPEG2 
} // namespace YamiParser
