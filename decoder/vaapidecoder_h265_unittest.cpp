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

// primary header
#include "vaapidecoder_h265.h"

// library headers
#include "common/factory_unittest.h"

namespace YamiMediaCodec {

class VaapiDecoderH265Test
    : public FactoryTest<IVideoDecoder, VaapiDecoderH265>
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

#define VAAPIDECODER_H265_TEST(name) \
    TEST_F(VaapiDecoderH265Test, name)

VAAPIDECODER_H265_TEST(Factory) {
    FactoryKeys mimeTypes;
    mimeTypes.push_back(YAMI_MIME_H265);
    doFactoryTest(mimeTypes);
}

}
