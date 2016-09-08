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
#include "nalreader.h"

// library headers
#include "common/Array.h"
#include "common/unittest.h"

namespace YamiMediaCodec {

#define NALREADER_TEST(name) \
    TEST(NalReaderTest, name)

const std::array<uint8_t, 39> g_data = {
    0x02, // junk

    0x00, 0x00, 0x01, // start code prefix
    0xff, 0xfe, 0xfd,
    0xfc, 0xfb, 0xfa,

    0x00, 0x00, 0x01, // start code prefix
    0xaa,

    0x00, 0x00, 0x01, // start code prefix
    0xfd, 0xaf, 0x24,

    0x00, 0x00, 0x01, // start code prefix
    0x01, 0x00, 0x00,
    0x1a, 0x01, 0x01,
    0x00, 0x01, 0xbd,

    0x00, 0x00, 0x01, // start code prefix
    0x56, 0x01, 0x84,
    0x12
};

const std::array<int32_t, 5> g_nsizes = {9, 4, 6, 12, 7};

NALREADER_TEST(ReadAsUnits) {
    const uint8_t* nal;
    int32_t offset(1), size;

    NalReader reader(&g_data[0], g_data.size(), false);

    for (unsigned n(0); n < g_nsizes.size(); ++n) {
        EXPECT_TRUE(reader.read(nal, size));
        EXPECT_EQ(size, g_nsizes[n]);
        for (int32_t i(0); i < size; ++i)
            EXPECT_EQ(nal[i], g_data[i+offset]);
        offset += size;
    }

    EXPECT_FALSE(reader.read(nal, size));
}

NALREADER_TEST(ReadAsWhole) {
    const uint8_t* nal;
    int32_t size;

    NalReader reader(&g_data[0], g_data.size(), true);

    EXPECT_TRUE(reader.read(nal, size));
    EXPECT_EQ((size_t)size, g_data.size());

    for (int32_t i(0); i < size; ++i)
        EXPECT_EQ(nal[i], g_data[i]);

    EXPECT_FALSE(reader.read(nal, size));
}

NALREADER_TEST(ReadEmptyBuf) {
    const uint8_t data[] = {};
    const uint8_t* nal;
    int32_t size;

    NalReader reader(data, 0, false);
    EXPECT_FALSE(reader.read(nal, size));

    reader = NalReader(data, 0, true);
    EXPECT_FALSE(reader.read(nal, size));
}

NALREADER_TEST(ReadNoPrefix) {
    const std::array<uint8_t, 9> data = {
        0x00, 0x01, 0x00,
        0x10, 0x01, 0xfe,
        0x01, 0x00, 0x00
    };
    const uint8_t* nal;
    int32_t size;

    NalReader reader(&data[0], data.size(), false);
    EXPECT_FALSE(reader.read(nal, size));
}

}
