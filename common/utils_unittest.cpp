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
#include "common/common_def.h"

// system headers
#include <limits>
#include <sstream>
#include <va/va.h>

#define UTILS_TEST(name) \
    TEST(UtilsTest, name)

using namespace YamiMediaCodec;

UTILS_TEST(guessFourcc) {
    EXPECT_EQ(guessFourcc("test.i420"), (uint32_t)YAMI_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test.nv12"), (uint32_t)YAMI_FOURCC_NV12);
    EXPECT_EQ(guessFourcc("test.yv12"), (uint32_t)YAMI_FOURCC_YV12);
    EXPECT_EQ(guessFourcc("test.yuy2"), (uint32_t)YAMI_FOURCC_YUY2);
    EXPECT_EQ(guessFourcc("test.uyvy"), (uint32_t)YAMI_FOURCC_UYVY);
    EXPECT_EQ(guessFourcc("test.rgbx"), (uint32_t)YAMI_FOURCC_RGBX);
    EXPECT_EQ(guessFourcc("test.bgrx"), (uint32_t)YAMI_FOURCC_BGRX);
    EXPECT_EQ(guessFourcc("test.xrgb"), (uint32_t)YAMI_FOURCC_XRGB);
    EXPECT_EQ(guessFourcc("test.xbgr"), (uint32_t)YAMI_FOURCC_XBGR);

    EXPECT_EQ(guessFourcc("test.txt"), (uint32_t)YAMI_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test"), (uint32_t)YAMI_FOURCC_I420);

    EXPECT_EQ(guessFourcc("test.I420"), (uint32_t)YAMI_FOURCC_I420);
    EXPECT_EQ(guessFourcc("test.NV12"), (uint32_t)YAMI_FOURCC_NV12);
    EXPECT_EQ(guessFourcc("test.YV12"), (uint32_t)YAMI_FOURCC_YV12);
    EXPECT_EQ(guessFourcc("test.YUY2"), (uint32_t)YAMI_FOURCC_YUY2);
    EXPECT_EQ(guessFourcc("test.UYVY"), (uint32_t)YAMI_FOURCC_UYVY);
    EXPECT_EQ(guessFourcc("test.RGBX"), (uint32_t)YAMI_FOURCC_RGBX);
    EXPECT_EQ(guessFourcc("test.BGRX"), (uint32_t)YAMI_FOURCC_BGRX);
    EXPECT_EQ(guessFourcc("test.XRGB"), (uint32_t)YAMI_FOURCC_XRGB);
    EXPECT_EQ(guessFourcc("test.XBGR"), (uint32_t)YAMI_FOURCC_XBGR);
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

    sstream.str("");
    sstream << long(std::numeric_limits<int>::max()) * 2 + 3
            << "x"
            << long(std::numeric_limits<int>::max()) * 3 + 2;

    EXPECT_FALSE(guessResolution(sstream.str().c_str(), w, h));
    EXPECT_EQ(w, 0);
    EXPECT_EQ(h, 0);
}

struct BppEntry {
    uint32_t fourcc;
    uint32_t planes;
    float bpp;
};

const static BppEntry bppEntrys[] = {
    { YAMI_FOURCC_NV12, 2, 1.5 },
    { YAMI_FOURCC_I420, 3, 1.5 },
    { YAMI_FOURCC_YV12, 3, 1.5 },
    { YAMI_FOURCC_IMC3, 3, 1.5 },
    { YAMI_FOURCC_422H, 3, 2 },
    { YAMI_FOURCC_422V, 3, 2 },
    { YAMI_FOURCC_444P, 3, 3 },
    { YAMI_FOURCC_P010, 2, 3 },
    { YAMI_FOURCC_YUY2, 1, 2 },
    { YAMI_FOURCC_UYVY, 1, 2 },
    { YAMI_FOURCC_RGBX, 1, 4 },
    { YAMI_FOURCC_RGBA, 1, 4 },
    { YAMI_FOURCC_BGRX, 1, 4 },
    { YAMI_FOURCC_BGRA, 1, 4 },
};

//check selected unsupported format.
//we do not want check all unsupported formats, it will introduce too many dependency on libva
const static uint32_t unsupported[] = {
    0,
    YAMI_FOURCC_Y800,
    YAMI_FOURCC_411P,
    YAMI_FOURCC_ARGB,
};

UTILS_TEST(getPlaneResolution)
{

    uint32_t width = 1920;
    uint32_t height = 1080;
    uint32_t planes;
    uint32_t w[3];
    uint32_t h[3];

    for (size_t i = 0; i < N_ELEMENTS(unsupported); i++) {
        EXPECT_FALSE(getPlaneResolution(unsupported[i], 1920, 1080, w, h, planes));
        EXPECT_EQ(0u, planes);
    }
    for (size_t i = 0; i < N_ELEMENTS(bppEntrys); i++) {
        const BppEntry& e = bppEntrys[i];
        EXPECT_TRUE(getPlaneResolution(e.fourcc, width, height, w, h, planes));
        EXPECT_EQ(e.planes, planes);

        float bpp;
        uint32_t bytes = 0;
        for (uint32_t p = 0; p < planes; p++) {
            bytes += w[p] * h[p];
        }
        bpp = (float)bytes / (width * height);
        EXPECT_FLOAT_EQ(e.bpp, bpp);
    }
}
