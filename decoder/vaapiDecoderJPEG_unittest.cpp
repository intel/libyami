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

//
// The unittest header must be included before va_x11.h (which might be included
// indirectly).  The va_x11.h includes Xlib.h and X.h.  And the X headers
// define 'Bool' and 'None' preprocessor types.  Gtest uses the same names
// to define some struct placeholders.  Thus, this creates a compile conflict
// if X defines them before gtest.  Hence, the include order requirement here
// is the only fix for this right now.
//
// See bug filed on gtest at https://github.com/google/googletest/issues/371
// for more details.
//
#include "common/factory_unittest.h"

// primary header
#include "vaapiDecoderJPEG.h"

// system headers
#include <algorithm>
#include <tr1/array>

namespace YamiMediaCodec {

const static std::tr1::array<uint8_t, 844> g_SimpleJPEG = {
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

class VaapiDecoderJPEGTest
    : public FactoryTest<IVideoDecoder, VaapiDecoderJPEG>
{
protected:
    /* invoked by gtest before the test */
    virtual void SetUp() {
        return;
    }

    /* invoked by gtest after the test */
    virtual void TearDown() {
        return;
    }
};

#define VAAPIDECODER_JPEG_TEST(name) \
    TEST_F(VaapiDecoderJPEGTest, name)

VAAPIDECODER_JPEG_TEST(Factory)
{
    FactoryKeys mimeTypes;
    mimeTypes.push_back(YAMI_MIME_JPEG);
    doFactoryTest(mimeTypes);
}

VAAPIDECODER_JPEG_TEST(Decode_Simple)
{
    VaapiDecoderJPEG decoder;
    VideoConfigBuffer config;
    VideoDecodeBuffer buffer;

    config.flag = 0;

    buffer.data = const_cast<uint8_t*>(g_SimpleJPEG.data());
    buffer.size = g_SimpleJPEG.size();
    buffer.timeStamp = 0;

    ASSERT_EQ(YAMI_SUCCESS, decoder.start(&config));
    ASSERT_EQ(YAMI_DECODE_FORMAT_CHANGE, decoder.decode(&buffer));
    ASSERT_EQ(YAMI_SUCCESS, decoder.decode(&buffer));
    ASSERT_EQ(YAMI_SUCCESS, decoder.decode(&buffer));

    EXPECT_TRUE(decoder.getOutput());
}

VAAPIDECODER_JPEG_TEST(Decode_SimpleMulti)
{
    /*
     * Test MJPEG decoding.  VaapiDecoderJPEG::decode will only accept
     * one JPEG buffer at a time (i.e. the buffer must start at one SOI marker
     * and end at one EOI marker.  Otherwise it will indicate YAMI_FAIL.  It
     * is up to the caller to ensure the MJPEG is divided on single JPEG images
     * for each call to decode.
     */
    VaapiDecoderJPEG decoder;
    VideoConfigBuffer config;
    VideoDecodeBuffer buffer;
    std::tr1::array<uint8_t, 844*2> mjpeg; // 2 images

    // First image
    std::copy(g_SimpleJPEG.begin(), g_SimpleJPEG.end(), mjpeg.begin());
    // Second image
    std::copy(g_SimpleJPEG.begin(), g_SimpleJPEG.end(), mjpeg.begin() + 844);

    config.flag = 0;

    // Set buffer at first jpeg image
    buffer.data = const_cast<uint8_t*>(mjpeg.data());
    buffer.size = 844; // Length of first jpeg image data
    buffer.timeStamp = 0;

    ASSERT_EQ(YAMI_SUCCESS, decoder.start(&config));

    // Decode returns format change after it decodes the SOF segment.
    ASSERT_EQ(YAMI_DECODE_FORMAT_CHANGE, decoder.decode(&buffer));

    // Resume decoding.  The decoder will continue where it left off (after the
    // SOF segment) when we pass the same buffer as previous call.
    ASSERT_EQ(YAMI_SUCCESS, decoder.decode(&buffer));

    EXPECT_TRUE(decoder.getOutput());

    // Set buffer at second jpeg image
    buffer.data = const_cast<uint8_t*>(mjpeg.data() + 844);
    buffer.size = 844; // Length of second jpeg image data
    buffer.timeStamp = 1;

    // Decode returns format change after it decodes the SOF segment.
    ASSERT_EQ(YAMI_DECODE_FORMAT_CHANGE, decoder.decode(&buffer));

    // Resume decoding.  The decoder will continue where it left off (after the
    // SOF segment) when we pass the same buffer as previous call.
    ASSERT_EQ(YAMI_SUCCESS, decoder.decode(&buffer));

    EXPECT_TRUE(decoder.getOutput());

    /* decoder should fail if more than one image in buffer */
    buffer.data = const_cast<uint8_t*>(mjpeg.data());
    buffer.size = mjpeg.size();  // Length of entire mjpeg
    buffer.timeStamp = 3;

    // Decode returns format change after it decodes the first SOF segment.
    ASSERT_EQ(YAMI_DECODE_FORMAT_CHANGE, decoder.decode(&buffer));

    // Resume decoding.  The decoder will continue where it left off (after the
    // SOF segment) when we pass the same buffer as previous call.  In this
    // case, the decode should fail since it encounters a second image in the
    // buffer.
    ASSERT_EQ(YAMI_FAIL, decoder.decode(&buffer));
}

VAAPIDECODER_JPEG_TEST(Decode_SimpleTruncated)
{
    const size_t size(g_SimpleJPEG.size());

    for (size_t i(1); i < size; ++i){
        VaapiDecoderJPEG decoder;
        VideoConfigBuffer config;
        VideoDecodeBuffer buffer;

        config.flag = 0;

        buffer.data = const_cast<uint8_t*>(g_SimpleJPEG.data());
        buffer.size = i;
        buffer.timeStamp = 0;

        ASSERT_EQ(YAMI_SUCCESS, decoder.start(&config));
        if (i > 176) { // Has full SOF0 segment
            ASSERT_EQ(YAMI_DECODE_FORMAT_CHANGE, decoder.decode(&buffer));
        }

        EXPECT_EQ(YAMI_FAIL, decoder.decode(&buffer));
        EXPECT_EQ(YAMI_FAIL, decoder.decode(&buffer));

        if (i < 160) { // No SOF0 segment
            EXPECT_EQ(YAMI_FAIL, decoder.start(&config));
        }
    }
}

}
