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
#include "bitWriter.h"

// library headers
#include "common/unittest.h"

namespace YamiParser {

static const uint16_t writeArrary[16][2] = {
    { 1, 1 }, { 2, 2 }, { 3, 2 }, { 4, 3 },
    { 5, 3 }, { 6, 3 }, { 7, 3 }, { 8, 4 },
    { 9, 4 }, { 10, 4 }, { 11, 4 }, { 12, 4 },
    { 13, 4 }, { 14, 4 }, { 15, 4 }, { 16, 5 }
};

class BitWriterTest
    : public ::testing::Test {
};

#define BITWriter_TEST(name) \
    TEST_F(BitWriterTest, name)

BITWriter_TEST(Writer_Simple)
{
    BitWriter Writer;

    EXPECT_EQ(0u, Writer.getCodedBitsCount());
    EXPECT_EQ(NULL, Writer.getBitWriterData());

    uint32_t i, bitsSum;

    for (i = 0, bitsSum = 0; i < 16; i++) {
        EXPECT_TRUE(Writer.writeBits(writeArrary[i][0], writeArrary[i][1]));
        bitsSum += writeArrary[i][1];
    }

    EXPECT_EQ(bitsSum, Writer.getCodedBitsCount());
}

} // namespace YamiParser
