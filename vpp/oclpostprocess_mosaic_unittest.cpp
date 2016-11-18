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
#include "oclpostprocess_mosaic.h"

namespace YamiMediaCodec {

class OclPostProcessMosaicTest
  : public FactoryTest<IVideoPostProcess, OclPostProcessMosaic>
{ };

#define OCLPOSTPROCESS_MOSAIC_TEST(name) \
    TEST_F(OclPostProcessMosaicTest, name)

OCLPOSTPROCESS_MOSAIC_TEST(Factory) {
    FactoryKeys mimeTypes;
    mimeTypes.push_back(YAMI_VPP_OCL_MOSAIC);
    doFactoryTest(mimeTypes);
}

}
