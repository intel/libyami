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
#include "utils.h"

// library headers
#include "common/unittest.h"

// system headers
#include <limits>
#include <sstream>
#include <va/va.h>

#define UTILS_TEST(name) \
    TEST(UtilsTest, name)

using namespace YamiMediaCodec;

UTILS_TEST(guessFourcc) {
    EXPECT_EQ(guessFourcc("test.i420"), VA_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test.nv12"), VA_FOURCC_NV12);
    EXPECT_EQ(guessFourcc("test.yv12"), VA_FOURCC_YV12);
    EXPECT_EQ(guessFourcc("test.yuy2"), VA_FOURCC_YUY2);
    EXPECT_EQ(guessFourcc("test.uyvy"), VA_FOURCC_UYVY);
    EXPECT_EQ(guessFourcc("test.rgbx"), VA_FOURCC_RGBX);
    EXPECT_EQ(guessFourcc("test.bgrx"), VA_FOURCC_BGRX);
    EXPECT_EQ(guessFourcc("test.xrgb"), VA_FOURCC_XRGB);
    EXPECT_EQ(guessFourcc("test.xbgr"), VA_FOURCC_XBGR);

    EXPECT_EQ(guessFourcc("test.txt"), VA_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test"), VA_FOURCC_I420);

    EXPECT_EQ(guessFourcc("test.I420"), VA_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test.NV12"), VA_FOURCC_NV12);
    EXPECT_EQ(guessFourcc("test.YV12"), VA_FOURCC_YV12);
    EXPECT_EQ(guessFourcc("test.YUY2"), VA_FOURCC_YUY2);
    EXPECT_EQ(guessFourcc("test.UYVY"), VA_FOURCC_UYVY);
    EXPECT_EQ(guessFourcc("test.RGBX"), VA_FOURCC_RGBX);
    EXPECT_EQ(guessFourcc("test.BGRX"), VA_FOURCC_BGRX);
    EXPECT_EQ(guessFourcc("test.XRGB"), VA_FOURCC_XRGB);
    EXPECT_EQ(guessFourcc("test.XBGR"), VA_FOURCC_XBGR);
}

UTILS_TEST(guessResolutionBasic) {
    int w(-1), h(-1);
    EXPECT_TRUE(guessResolution("test1920x1080", w, h));
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 1080);

    w = -1, h = -1;
    EXPECT_TRUE(guessResolution("test1920X1080", w, h));
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 1080);

    w = -1, h = -1;
    EXPECT_TRUE(guessResolution("test123-1920x1080", w, h));
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 1080);

    w = -1, h = -1;
    EXPECT_TRUE(guessResolution("test-1920x1080-123", w, h));
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 1080);

    w = -1, h = -1;
    EXPECT_FALSE(guessResolution("test1920xx1080", w, h));
    EXPECT_EQ(w, 1920);
    EXPECT_EQ(h, 0);

    w = -1, h = -1;
    EXPECT_FALSE(guessResolution("test", w, h));
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);

    w = -1, h = -1;
    EXPECT_FALSE(guessResolution("test1080p", w, h));
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);

    w = -1, h = -1;
    EXPECT_FALSE(guessResolution("test720x", w, h));
    EXPECT_EQ(w, 720);
    EXPECT_EQ(h, 0);

    w = -1, h = -1;
    EXPECT_TRUE(guessResolution("1280x720", w, h));
    EXPECT_EQ(w, 1280);
    EXPECT_EQ(h, 720);

    w = -1, h = -1;
    EXPECT_FALSE(guessResolution("w3840h2160", w, h));
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);
}

UTILS_TEST(guessResolutionOverflow) {
    std::stringstream sstream;
    int w, h;

    w = h = std::numeric_limits<int>::max();

    // sanity check
    ASSERT_GT(std::numeric_limits<long>::max(), w);
    ASSERT_GT(std::numeric_limits<long>::max(), h);

    sstream << std::numeric_limits<long>::max()
            << "x"
            << std::numeric_limits<long>::max();

    EXPECT_FALSE(guessResolution(sstream.str().c_str(), w, h));
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);
}

UTILS_TEST(getPlaneResolutionDeath) {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    uint32_t planes;
    uint32_t w[3], h[3];

    // getPlaneResolution aborts program (via assert) if there is an invalid
    // fourcc value
    EXPECT_DEATH(
        getPlaneResolution(0, 1920, 1080, w, h, planes),
        "do not support this format");
}
