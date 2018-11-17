/*
 * Copyright 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
3B *     http://www.apache.org/licenses/LICENSE-2.0
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
#include "RbduReader.h"

// library headers
#include "common/unittest.h"

namespace YamiParser {

class RbduReaderTest
    : public ::testing::Test {

public:
    static bool isEpb(const RbduReader& r, const uint8_t* p)
    {
        return r.isEmulationPreventionByte(p);
    }
};

#define RBDUREADER_TEST(name) \
    TEST_F(RbduReaderTest, name)

#define DIM(a) (sizeof(a) / sizeof(a[0]))

RBDUREADER_TEST(GetPosForEPB)
{
    const uint8_t data[4][8] = {
        { 0x0, 0x0, 0x3, 0x0, 0x1, 0x2, 0x3, 0x4 },
        { 0x0, 0x0, 0x3, 0x1, 0x1, 0x2, 0x3, 0x4 },
        { 0x0, 0x0, 0x3, 0x2, 0x1, 0x2, 0x3, 0x4 },
        { 0x0, 0x0, 0x3, 0x3, 0x1, 0x2, 0x3, 0x4 }
    };

    for (uint32_t i = 0; i < DIM(data); i++) {
        RbduReader r(data[i], sizeof(data[i]));
        EXPECT_EQ(0u, r.getPos());

        r.skip(1);
        EXPECT_EQ(1u, r.getPos());

        r.skip(7);
        EXPECT_EQ(8u, r.getPos());

        r.skip(8);
        //we are reach the epb
        EXPECT_EQ(24u, r.getPos());

        r.skip(8);
        EXPECT_EQ(32u, r.getPos());

        r.skip(24);

        uint8_t b;
        EXPECT_TRUE(r.readT(b));
        EXPECT_EQ(4u, b);
    }
}

RBDUREADER_TEST(GetPos)
{
    const uint8_t data[4][8] = {
        { 0x0, 0x0, 0x3, 0x4, 0x1, 0x2, 0x3, 0x4 },
        { 0x0, 0x0, 0x3, 0xFF, 0x1, 0x2, 0x3, 0x4 },
        { 0x1, 0x0, 0x3, 0x2, 0x1, 0x2, 0x3, 0x4 },
        { 0x0, 0x0, 0x0, 0x3, 0x5, 0x2, 0x3, 0x4 }
    };

    for (uint32_t i = 0; i < DIM(data); i++) {
        RbduReader r(data[i], sizeof(data[i]));
        EXPECT_EQ(0u, r.getPos());

        r.skip(1);
        EXPECT_EQ(1u, r.getPos());

        r.skip(7);
        EXPECT_EQ(8u, r.getPos());

        r.skip(16);
        EXPECT_EQ(24u, r.getPos());

        r.skip(8);
        EXPECT_EQ(32u, r.getPos());

        r.skip(24);

        uint8_t b;
        EXPECT_TRUE(r.readT(b));
        EXPECT_EQ(4u, b);
    }
}

RBDUREADER_TEST(isEpb)
{
    const uint8_t data[] = {
        0x0, 0x0, 0x3, 0x0,
        0x3, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x3
    };
    RbduReader r(data, DIM(data));
    for (uint32_t i = 0; i < DIM(data); i++) {
        bool epb = RbduReaderTest::isEpb(r, &data[i]);
        if (i == 2)
            EXPECT_TRUE(epb);
        else
            EXPECT_FALSE(epb);
    }
}

} // namespace YamiParser
