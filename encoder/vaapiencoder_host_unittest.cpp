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
#include "common/unittest.h"

// primary header
#include "VideoEncoderHost.h"

// system headers
#include <algorithm>
#include <string>
#include <set>

namespace YamiMediaCodec {

std::set<std::string>& expectations()
{
    static std::set<std::string> e;

#if __BUILD_H264_ENCODER__
    e.insert(YAMI_MIME_H264);
    e.insert(YAMI_MIME_AVC);
#endif

#if __BUILD_H265_ENCODER__
    e.insert(YAMI_MIME_H265);
    e.insert(YAMI_MIME_HEVC);
#endif

#if __BUILD_VP8_ENCODER__
    e.insert(YAMI_MIME_VP8);
#endif

#if __BUILD_VP9_ENCODER__
    e.insert(YAMI_MIME_VP9);
#endif

#if __BUILD_JPEG_ENCODER__
    e.insert(YAMI_MIME_JPEG);
#endif

    return e;
}

class VaapiEncoderHostTest
    : public ::testing::TestWithParam<std::string>
{ };

TEST_P(VaapiEncoderHostTest, createVideoEncoder)
{
    std::string mime = GetParam();
    IVideoEncoder *encoder = createVideoEncoder(mime.c_str());
    bool expect = expectations().count(mime) != 0;

    EXPECT_EQ(expect, (encoder != NULL))
        << "createVideoEncoder(" << mime << "): "
        << "did not " << (expect ? "SUCCEED" : "FAIL")
        << " as it should have.";

    releaseVideoEncoder(encoder);
}

TEST_P(VaapiEncoderHostTest, getVideoEncoderMimeTypesContains)
{
    std::string mime = GetParam();
    std::vector<std::string> avail = getVideoEncoderMimeTypes();

    bool expect = expectations().count(mime) != 0;
    bool actual = std::find(avail.begin(), avail.end(), mime) != avail.end();

    EXPECT_EQ( expect, actual );
}

INSTANTIATE_TEST_CASE_P(
    MimeType, VaapiEncoderHostTest,
    ::testing::Values(
        YAMI_MIME_H264, YAMI_MIME_AVC, YAMI_MIME_H265, YAMI_MIME_HEVC,
        YAMI_MIME_MPEG2, YAMI_MIME_VC1, YAMI_MIME_VP8, YAMI_MIME_VP9,
        YAMI_MIME_JPEG));
}
