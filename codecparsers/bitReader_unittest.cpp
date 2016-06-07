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
#include "bitReader.h"

// library headers
#include "common/unittest.h"

// system libraries
#include <limits>
#include <vector>

namespace YamiParser {

class BitReaderTest
    : public ::testing::Test
{
protected:
    std::vector<uint8_t>& largeData() const
    {
        static const uint32_t nBytes =
            (std::numeric_limits<uint32_t>::max() >> 3) + 1;
        static std::vector<uint8_t> data(nBytes);

        return data;
    }
};

#define BITREADER_TEST(name) \
    TEST_F(BitReaderTest, name)

BITREADER_TEST(NoOverflow)
{
    std::vector<uint8_t>& data = largeData();
    const uint32_t nBytes = data.size();

    ASSERT_EQ(0u, nBytes % (1024 * 1024 * 512));

    BitReader reader(&data[0], nBytes);

    EXPECT_FALSE(reader.end());
    EXPECT_EQ(0u, reader.getPos());
    EXPECT_EQ(static_cast<uint64_t>(nBytes) << 3,
        reader.getRemainingBitsCount());

    const uint32_t nReads = nBytes / BitReader::CACHEBYTES;
    const uint32_t rem = nBytes % BitReader::CACHEBYTES;
    const uint32_t bitsPerRead = BitReader::CACHEBYTES << 3;
    for (uint32_t i(0); i < nReads; ++i)
        reader.read(bitsPerRead);
    reader.read(rem << 3);

    EXPECT_EQ(static_cast<uint64_t>(nBytes) << 3, reader.getPos());
    EXPECT_TRUE(reader.end());
    EXPECT_EQ(0u, reader.getRemainingBitsCount());
}

} // namespace YamiParser
