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
#include "VideoPostProcessHost.h"

// system headers
#include <algorithm>
#include <string>
#include <set>

namespace YamiMediaCodec {

std::set<std::string>& expectations()
{
    static std::set<std::string> e;

#if __BUILD_OCL_FILTERS__
    e.insert(YAMI_VPP_OCL_BLENDER);
    e.insert(YAMI_VPP_OCL_OSD);
    e.insert(YAMI_VPP_OCL_TRANSFORM);
    e.insert(YAMI_VPP_OCL_MOSAIC);
    e.insert(YAMI_VPP_OCL_WIREFRAME);
#endif

    e.insert(YAMI_VPP_SCALER);

    return e;
}

class VaapiPostProcessHostTest
    : public ::testing::TestWithParam<std::string>
{ };

TEST_P(VaapiPostProcessHostTest, createVideoPostProcess)
{
    std::string mime = GetParam();
    IVideoPostProcess *vpp = createVideoPostProcess(mime.c_str());
    bool expect = expectations().count(mime) != 0;

    EXPECT_EQ(expect, (vpp != NULL))
        << "createVideoPostProcess(" << mime << "): "
        << "did not " << (expect ? "SUCCEED" : "FAIL")
        << " as it should have.";

    releaseVideoPostProcess(vpp);
}

TEST_P(VaapiPostProcessHostTest, getVideoPostProcessMimeTypesContains)
{
    std::string mime = GetParam();
    std::vector<std::string> avail = getVideoPostProcessMimeTypes();

    bool expect = expectations().count(mime) != 0;
    bool actual = std::find(avail.begin(), avail.end(), mime) != avail.end();

    EXPECT_EQ( expect, actual );
}

INSTANTIATE_TEST_CASE_P(
    MimeType, VaapiPostProcessHostTest,
    ::testing::Values(
        YAMI_VPP_SCALER, YAMI_VPP_OCL_BLENDER, YAMI_VPP_OCL_OSD,
        YAMI_VPP_OCL_TRANSFORM, YAMI_VPP_OCL_MOSAIC, YAMI_VPP_OCL_WIREFRAME));
}
