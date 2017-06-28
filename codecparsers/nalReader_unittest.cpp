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
#include "nalReader.h"

// library headers
#include "common/unittest.h"

namespace YamiParser {

class NalReaderTest
    : public ::testing::Test {
};

#define NALREADER_TEST(name) \
    TEST_F(NalReaderTest, name)

NALREADER_TEST(ReadBeyondBoundary)
{
    uint8_t data = 0x55;
    uint32_t u;
    int32_t s;
    NalReader reader(&data, 1);
    EXPECT_EQ(0u, reader.read(1));
    EXPECT_EQ(1u, reader.read(1));
    EXPECT_TRUE(reader.readUe(u));
    EXPECT_EQ(1u, u);
    EXPECT_TRUE(reader.readSe(s));
    EXPECT_EQ(0, s);

    EXPECT_FALSE(reader.end());
    EXPECT_FALSE(reader.read(u, 8));
    EXPECT_TRUE(reader.end());
    EXPECT_FALSE(reader.readUe(u));
    EXPECT_EQ(0u, reader.readUe());
    EXPECT_FALSE(reader.readSe(s));
    EXPECT_EQ(0, reader.readSe());
}

void checkBitreadEmpty(NalReader& reader)
{
    EXPECT_TRUE(reader.end());
    EXPECT_EQ(0u, reader.getPos());
    EXPECT_EQ(0u,
        reader.getRemainingBitsCount());

    uint32_t u;
    int32_t s;
    EXPECT_FALSE(reader.readUe(u));
    EXPECT_FALSE(reader.readSe(s));

    EXPECT_TRUE(reader.end());
}

NALREADER_TEST(NullInit)
{
    uint8_t data = 0;
    NalReader r1(&data, 0);
    checkBitreadEmpty(r1);

    NalReader r2(NULL, 0);
    checkBitreadEmpty(r2);

    EXPECT_DEATH(NalReader r3(NULL, 1), "");
}

} // namespace YamiParser
