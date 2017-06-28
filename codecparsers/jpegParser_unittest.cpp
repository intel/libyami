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
#include "jpegParser.h"

// library headers
#include "common/unittest.h"

// system headers
#include <limits>

namespace YamiParser {
namespace JPEG {

const static std::array<uint8_t, 844> g_SimpleJPEG = {
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43,
    0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x0a, 0x00, 0x0a, 0x03,
    0x01, 0x11, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xc4, 0x00,
    0x1f, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x10, 0x00,
    0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05, 0x04, 0x04, 0x00,
    0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21,
    0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81,
    0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24,
    0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a,
    0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86,
    0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
    0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6,
    0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
    0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xc4, 0x00,
    0x1f, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x11, 0x00,
    0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00,
    0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31,
    0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08,
    0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15,
    0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18,
    0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55,
    0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84,
    0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
    0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4,
    0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xda, 0x00,
    0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0xfe,
    0x7b, 0xb4, 0xcd, 0x33, 0xee, 0xfc, 0xa7, 0x9c, 0x76, 0xfc, 0x7b, 0x8f,
    0xc3, 0x1f, 0xd7, 0x9a, 0xff, 0x00, 0x72, 0x7e, 0x90, 0xbf, 0x48, 0x3b,
    0xbc, 0x77, 0xfb, 0x6a, 0xff, 0x00, 0x97, 0x9b, 0x55, 0xd9, 0x59, 0xeb,
    0xf1, 0x34, 0x7f, 0x56, 0x7d, 0x1c, 0xbe, 0x8e, 0x97, 0xfa, 0x82, 0xfa,
    0x87, 0x4a, 0x7f, 0xf2, 0xeb, 0xc9, 0x2e, 0xdf, 0x77, 0xcb, 0xa6, 0xaf,
    0xad, 0x5d, 0x30, 0xe0, 0x7c, 0xa3, 0xa0, 0xee, 0x3d, 0x3e, 0xb5, 0xfe,
    0x62, 0x62, 0xbe, 0x90, 0xd1, 0xfa, 0xd6, 0x27, 0xfd, 0xb7, 0xfe, 0x62,
    0x2b, 0x7f, 0xcb, 0xc7, 0xff, 0x00, 0x3f, 0x24, 0x7f, 0xa9, 0x98, 0x5f,
    0xa3, 0xa4, 0x7e, 0xab, 0x86, 0xff, 0x00, 0x62, 0x5f, 0xee, 0xf4, 0x7f,
    0xe5, 0xd2, 0xff, 0x00, 0x9f, 0x71, 0xfe, 0xe8, 0x69, 0x8a, 0xbc, 0x70,
    0x3a, 0x0e, 0xc3, 0xd4, 0x55, 0xfd, 0x21, 0x31, 0x58, 0xaf, 0xf6, 0xef,
    0xf6, 0x9a, 0xff, 0x00, 0x15, 0x5f, 0xf9, 0x7d, 0x53, 0xfb, 0xdf, 0xde,
    0x23, 0xe8, 0xe3, 0x84, 0xc2, 0xdb, 0x2f, 0xff, 0x00, 0x66, 0xc3, 0xef,
    0x4b, 0xfe, 0x5c, 0xd3, 0xf2, 0xfe, 0xe9, 0xd5, 0xed, 0x5f, 0x41, 0xf9,
    0x0a, 0xff, 0x00, 0x31, 0x31, 0x58, 0xac, 0x4f, 0xd6, 0x71, 0x1f, 0xed,
    0x15, 0xff, 0x00, 0x8f, 0x5b, 0xfe, 0x5f, 0x54, 0xff, 0x00, 0x9f, 0x92,
    0xfe, 0xf1, 0xfe, 0xa5, 0xe1, 0x30, 0x98, 0x5f, 0xaa, 0xe1, 0xbf, 0xd9,
    0xb0, 0xff, 0x00, 0xee, 0xf4, 0x7f, 0xe5, 0xcd, 0x3f, 0xf9, 0xf7, 0x1f,
    0xee, 0x9f, 0xff, 0xd9
};

class JPEGParserTest
    : public ::testing::Test {

protected:

#define MEMBER_VARIABLE(result_type, var) \
    result_type& var(Parser& p) \
    { \
        return p.var; \
    }

    MEMBER_VARIABLE(BitReader, m_input);
    MEMBER_VARIABLE(Segment, m_current);
    MEMBER_VARIABLE(ScanHeader::Shared, m_scanHeader);
    MEMBER_VARIABLE(FrameHeader::Shared, m_frameHeader);
    MEMBER_VARIABLE(QuantTables, m_quantTables);
    MEMBER_VARIABLE(HuffTables, m_dcHuffTables);
    MEMBER_VARIABLE(HuffTables, m_acHuffTables);
    MEMBER_VARIABLE(ArithmeticTable, m_arithDCL);
    MEMBER_VARIABLE(ArithmeticTable, m_arithDCU);
    MEMBER_VARIABLE(ArithmeticTable, m_arithACK);
    MEMBER_VARIABLE(Parser::Callbacks, m_callbacks);
    MEMBER_VARIABLE(bool, m_sawSOI);
    MEMBER_VARIABLE(bool, m_sawEOI);
    MEMBER_VARIABLE(unsigned, m_restartInterval);

#undef MEMBER_VARIABLE

#define PARSE_MARKER(marker) \
    bool parse ## marker(Parser& p) \
    { \
        return p.parse ## marker(); \
    }

    PARSE_MARKER(SOI);
    PARSE_MARKER(APP);
    PARSE_MARKER(SOS);
    PARSE_MARKER(EOI);
    PARSE_MARKER(DAC);
    PARSE_MARKER(DQT);
    PARSE_MARKER(DHT);
    PARSE_MARKER(DRI);

#undef PARSE_MARKER

    bool parseSOF(Parser& p, bool isBaseline, bool isProgressive, bool isArithmetic)
    {
        return p.parseSOF(isBaseline, isProgressive, isArithmetic);
    }

    void checkSimpleJPEG(const Parser& parser)
    {
        ASSERT_TRUE(parser.m_scanHeader.get() != NULL);

        EXPECT_TRUE(parser.m_input.end());
        EXPECT_EQ(M_EOI, parser.m_current.marker);
        EXPECT_EQ(0u, parser.restartInterval());

        ASSERT_TRUE(parser.m_frameHeader.get() != NULL);
        EXPECT_EQ(10u, parser.m_frameHeader->imageWidth);
        EXPECT_EQ(10u, parser.m_frameHeader->imageHeight);
        EXPECT_EQ(8, parser.m_frameHeader->dataPrecision);
        EXPECT_TRUE(parser.m_frameHeader->isBaseline);
        EXPECT_FALSE(parser.m_frameHeader->isProgressive);
        EXPECT_FALSE(parser.m_frameHeader->isArithmetic);
        ASSERT_EQ(3u, parser.m_frameHeader->components.size());
        EXPECT_TRUE(parser.m_frameHeader->components[0].get() != NULL);
        EXPECT_TRUE(parser.m_frameHeader->components[1].get() != NULL);
        EXPECT_TRUE(parser.m_frameHeader->components[2].get() != NULL);

        EXPECT_TRUE(parser.quantTables()[0].get() != NULL);
        EXPECT_TRUE(parser.quantTables()[1].get() != NULL);
        EXPECT_TRUE(parser.quantTables()[2].get() == NULL);
        EXPECT_TRUE(parser.quantTables()[3].get() == NULL);

        EXPECT_TRUE(parser.dcHuffTables()[0].get() != NULL);
        EXPECT_TRUE(parser.dcHuffTables()[1].get() != NULL);
        EXPECT_TRUE(parser.dcHuffTables()[2].get() == NULL);
        EXPECT_TRUE(parser.dcHuffTables()[3].get() == NULL);

        EXPECT_TRUE(parser.acHuffTables()[0].get() != NULL);
        EXPECT_TRUE(parser.acHuffTables()[1].get() != NULL);
        EXPECT_TRUE(parser.acHuffTables()[2].get() == NULL);
        EXPECT_TRUE(parser.acHuffTables()[3].get() == NULL);
    }
};

#define JPEG_PARSER_TEST(name) \
    TEST_F(JPEGParserTest, name)

JPEG_PARSER_TEST(Construct_Defaults)
{
    std::array<uint8_t, 1> data = {0x00};

    Parser parser(&data[0], data.size());

    EXPECT_FALSE(m_input(parser).end());
    EXPECT_EQ(m_current(parser).marker, M_ERROR);

    EXPECT_TRUE(m_frameHeader(parser).get() == NULL);
    EXPECT_TRUE(m_scanHeader(parser).get() == NULL);

    EXPECT_EQ(m_quantTables(parser).size(), NUM_QUANT_TBLS);
    for (size_t i(0); i < m_quantTables(parser).size(); ++i)
        EXPECT_TRUE(m_quantTables(parser)[i].get() == NULL);

    EXPECT_EQ(m_dcHuffTables(parser).size(), NUM_HUFF_TBLS);
    for (size_t i(0); i < m_dcHuffTables(parser).size(); ++i)
        EXPECT_TRUE(m_dcHuffTables(parser)[i].get() == NULL);

    EXPECT_EQ(m_acHuffTables(parser).size(), NUM_HUFF_TBLS);
    for (size_t i(0); i < m_acHuffTables(parser).size(); ++i)
        EXPECT_TRUE(m_acHuffTables(parser)[i].get() == NULL);

    EXPECT_EQ(m_arithDCL(parser).size(), NUM_ARITH_TBLS);
    EXPECT_EQ(m_arithDCU(parser).size(), NUM_ARITH_TBLS);
    EXPECT_EQ(m_arithACK(parser).size(), NUM_ARITH_TBLS);

    EXPECT_EQ(m_callbacks(parser).size(), 0u);

    EXPECT_FALSE(m_sawSOI(parser));
    EXPECT_FALSE(m_sawEOI(parser));

    EXPECT_EQ(m_restartInterval(parser), 0u);
}

JPEG_PARSER_TEST(Construct_InvalidParams)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";

    // invalid data pointer
    EXPECT_DEATH(Parser(NULL, 10), "");

    std::array<uint8_t, 1> data = {0x00};

    // invalid size
    Parser parser(&data[0], 0);
    EXPECT_FALSE(parser.parse());
}

JPEG_PARSER_TEST(Parse_NoSOI)
{
    std::array<uint8_t, 24> data = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    Parser parser(&data[0], data.size());

    EXPECT_FALSE(parser.parse());

    EXPECT_EQ(parser.current().marker, M_ERROR);
}

JPEG_PARSER_TEST(Parse_SOINotFirst)
{
    std::array<uint8_t, 24> data = {
        0x00, 0xFF, M_SOI, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    Parser parser(&data[0], data.size());

    EXPECT_FALSE(parser.parse());

    EXPECT_EQ(parser.current().marker, M_ERROR);
}

JPEG_PARSER_TEST(Parse_SOIInvalid)
{
    std::array<uint8_t, 24> data = {
        0x00, M_SOI, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    Parser parser(&data[0], data.size());

    EXPECT_FALSE(parser.parse());

    EXPECT_EQ(parser.current().marker, M_ERROR);
}

JPEG_PARSER_TEST(ParseSOI)
{
    std::array<uint8_t, 1> data = {0x00};

    Parser parser(&data[0], data.size());

    EXPECT_FALSE(m_sawSOI(parser));
    EXPECT_TRUE(parseSOI(parser));
    EXPECT_TRUE(m_sawSOI(parser));
    EXPECT_FALSE(parseSOI(parser));
}

JPEG_PARSER_TEST(ParseSOS_NoSOF)
{
    std::array<uint8_t, 1> data = {0x00};

    Parser parser(&data[0], data.size());

    EXPECT_TRUE(m_frameHeader(parser).get() == NULL);
    EXPECT_FALSE(parseSOS(parser)); // returns false when SOF not seen
}

JPEG_PARSER_TEST(ParseSOS_InvalidLength)
{
    std::array<uint8_t, 3> data = {0x00, 0x06, 0x00};

    Parser parser(&data[0], data.size());

    m_frameHeader(parser).reset(new FrameHeader);

    ASSERT_FALSE(m_input(parser).end());
    ASSERT_EQ(m_current(parser).length, 0u);

    EXPECT_FALSE(parseSOS(parser));
    EXPECT_TRUE(m_input(parser).end());
    EXPECT_EQ(m_current(parser).length, 6u);

    data[0] = 0x00;
    data[1] = 0x0c;
    data[2] = 0x00;
    parser = Parser(&data[0], data.size());
    m_frameHeader(parser).reset(new FrameHeader);

    ASSERT_FALSE(m_input(parser).end());
    ASSERT_EQ(m_current(parser).length, 0u);

    EXPECT_FALSE(parseSOS(parser));
    EXPECT_TRUE(m_input(parser).end());
    EXPECT_EQ(m_current(parser).length, 12u);

    int nComponents = MAX_COMPS_IN_SCAN + 1;
    unsigned length = nComponents * 2 + 6;

    ASSERT_LT(length, std::numeric_limits<uint8_t>::max());

    data[0] = 0x00;
    data[1] = uint8_t(length);
    data[2] = uint8_t(nComponents);
    parser = Parser(&data[0], data.size());
    m_frameHeader(parser).reset(new FrameHeader);

    ASSERT_FALSE(m_input(parser).end());
    ASSERT_EQ(m_current(parser).length, 0u);

    EXPECT_FALSE(parseSOS(parser));
    EXPECT_TRUE(m_input(parser).end());
    EXPECT_EQ(m_current(parser).length, length);
}

JPEG_PARSER_TEST(ParseSOS_BadComponentDescriptor)
{
    std::array<uint8_t, 4> data = {0x00, 0x0c, 0x3, 0x01};

    Parser parser(&data[0], data.size());
    m_frameHeader(parser).reset(new FrameHeader);

    ASSERT_FALSE(m_input(parser).end());
    ASSERT_TRUE(m_scanHeader(parser).get() == NULL);

    EXPECT_FALSE(parseSOS(parser));
    EXPECT_TRUE(m_input(parser).end());
    ASSERT_TRUE(m_scanHeader(parser).get() != NULL);
    EXPECT_EQ(m_scanHeader(parser)->numComponents, 3u);
}

JPEG_PARSER_TEST(ParseSOS)
{
    std::array<uint8_t, 12> data = {
        0x00, 0x0c,
        0x03, 0x01, 0x11, 0x02, 0x10, 0x03, 0x01, 0x12,
        0x34, 0x56
    };

    Parser parser(&data[0], data.size());

    m_frameHeader(parser).reset(new FrameHeader);
    m_frameHeader(parser)->components.resize(3);
    for (size_t i(0); i < 3; ++i) {
        m_frameHeader(parser)->components[i].reset(new Component);
        m_frameHeader(parser)->components[i]->index = i;
        m_frameHeader(parser)->components[i]->id = i + 1;
    }

    ASSERT_FALSE(m_input(parser).end());
    ASSERT_TRUE(m_scanHeader(parser).get() == NULL);
    ASSERT_EQ(m_current(parser).length, 0u);

    EXPECT_TRUE(parseSOS(parser));
    EXPECT_TRUE(m_input(parser).end());
    EXPECT_EQ(m_current(parser).length, 12u);

    ASSERT_TRUE(m_scanHeader(parser).get() != NULL);
    EXPECT_EQ(m_scanHeader(parser)->numComponents, 3u);
    ASSERT_TRUE(m_scanHeader(parser)->components[0].get() != NULL);
    ASSERT_TRUE(m_scanHeader(parser)->components[1].get() != NULL);
    ASSERT_TRUE(m_scanHeader(parser)->components[2].get() != NULL);
    EXPECT_TRUE(m_scanHeader(parser)->components[3].get() == NULL);

    for (size_t i(0); i < 3; ++i)
        EXPECT_EQ(m_scanHeader(parser)->components[i].get(),
                  m_frameHeader(parser)->components[i].get());

    EXPECT_EQ(m_scanHeader(parser)->components[0]->dcTableNumber, 1);
    EXPECT_EQ(m_scanHeader(parser)->components[0]->acTableNumber, 1);

    EXPECT_EQ(m_scanHeader(parser)->components[1]->dcTableNumber, 1);
    EXPECT_EQ(m_scanHeader(parser)->components[1]->acTableNumber, 0);

    EXPECT_EQ(m_scanHeader(parser)->components[2]->dcTableNumber, 0);
    EXPECT_EQ(m_scanHeader(parser)->components[2]->acTableNumber, 1);

    EXPECT_EQ(m_scanHeader(parser)->ss, 0x12);
    EXPECT_EQ(m_scanHeader(parser)->se, 0x34);
    EXPECT_EQ(m_scanHeader(parser)->ah, 0x5);
    EXPECT_EQ(m_scanHeader(parser)->al, 0x6);
}

JPEG_PARSER_TEST(Parse_Simple)
{
    Parser parser(&g_SimpleJPEG[0], g_SimpleJPEG.size());

    EXPECT_TRUE(parser.parse());

    checkSimpleJPEG(parser);
    ASSERT_FALSE(HasFailure());
}

typedef std::map<Marker, std::vector<Segment> > Results;
Parser::CallbackResult simpleCallback(Results& results, const Parser& parser)
{
    results[parser.current().marker].push_back(parser.current());
    return Parser::ParseContinue;
}

JPEG_PARSER_TEST(Parse_SimpleWithCallbacks)
{
    Parser parser(&g_SimpleJPEG[0], g_SimpleJPEG.size());
    Results results;

#define registerCallback(id) \
    do { \
        parser.registerCallback(M_ ## id, \
            std::bind(&simpleCallback, std::ref(results), std::ref(parser))); \
    } while(0)

    registerCallback(SOI);
    registerCallback(EOI);
    registerCallback(APP0);
    registerCallback(DQT);
    registerCallback(SOF0);
    registerCallback(DHT);
    registerCallback(SOS);

#undef registerCallback

    EXPECT_TRUE(parser.parse());

    ASSERT_EQ(g_SimpleJPEG[1], M_SOI);
    ASSERT_EQ(results[M_SOI].size(), 1u);
    EXPECT_EQ(results[M_SOI][0].position, 1u);
    EXPECT_EQ(results[M_SOI][0].length, 0u);

    ASSERT_EQ(g_SimpleJPEG[843], M_EOI);
    ASSERT_EQ(results[M_EOI].size(), 1u);
    EXPECT_EQ(results[M_EOI][0].position, 843u);
    EXPECT_EQ(results[M_EOI][0].length, 0u);

    ASSERT_EQ(g_SimpleJPEG[3], M_APP0);
    ASSERT_EQ(results[M_APP0].size(), 1u);
    EXPECT_EQ(results[M_APP0][0].position, 3u);
    EXPECT_EQ(results[M_APP0][0].length, 16u);

    ASSERT_EQ(g_SimpleJPEG[159], M_SOF0);
    ASSERT_EQ(results[M_SOF0].size(), 1u);
    EXPECT_EQ(results[M_SOF0][0].position, 159u);
    EXPECT_EQ(results[M_SOF0][0].length, 17u);

    ASSERT_EQ(g_SimpleJPEG[610], M_SOS);
    ASSERT_EQ(results[M_SOS].size(), 1u);
    EXPECT_EQ(results[M_SOS][0].position, 610u);
    EXPECT_EQ(results[M_SOS][0].length, 12u);

    ASSERT_EQ(g_SimpleJPEG[21], M_DQT);
    ASSERT_EQ(g_SimpleJPEG[90], M_DQT);
    ASSERT_EQ(results[M_DQT].size(), 2u);
    EXPECT_EQ(results[M_DQT][0].position, 21u);
    EXPECT_EQ(results[M_DQT][0].length, 67u);
    EXPECT_EQ(results[M_DQT][1].position, 90u);
    EXPECT_EQ(results[M_DQT][1].length, 67u);

    ASSERT_EQ(g_SimpleJPEG[178], M_DHT);
    ASSERT_EQ(g_SimpleJPEG[211], M_DHT);
    ASSERT_EQ(g_SimpleJPEG[394], M_DHT);
    ASSERT_EQ(g_SimpleJPEG[427], M_DHT);
    ASSERT_EQ(results[M_DHT].size(), 4u);
    EXPECT_EQ(results[M_DHT][0].position, 178u);
    EXPECT_EQ(results[M_DHT][0].length, 31u);
    EXPECT_EQ(results[M_DHT][1].position, 211u);
    EXPECT_EQ(results[M_DHT][1].length, 181u);
    EXPECT_EQ(results[M_DHT][2].position, 394u);
    EXPECT_EQ(results[M_DHT][2].length, 31u);
    EXPECT_EQ(results[M_DHT][3].position, 427u);
    EXPECT_EQ(results[M_DHT][3].length, 181u);

    checkSimpleJPEG(parser);
    ASSERT_FALSE(HasFailure());
}

JPEG_PARSER_TEST(Parse_SimpleTruncated)
{
    const size_t size(g_SimpleJPEG.size());

    for (size_t i(1); i < size; ++i) {
        Parser parser(&g_SimpleJPEG[0], i);
        EXPECT_FALSE(parser.parse());
    }
}

} // namespace JPEG
} // namespace YamiParser
