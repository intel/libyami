/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#include "common/log.h"
#include "interface/VideoDecoderHost.h"
#include "vaapidecoder_factory.h"

#if __BUILD_FAKE_DECODER__
#include "vaapidecoder_fake.h"
#endif

using namespace YamiMediaCodec;

extern "C" {

IVideoDecoder *createVideoDecoder(const char *mimeType)
{
    yamiTraceInit();

    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }

#if __BUILD_FAKE_DECODER__
    INFO("Created fake decoder");
    return new VaapiDecoderFake(1920, 1080);
#endif

    VaapiDecoderFactory::Type dec = VaapiDecoderFactory::create(mimeType);

    if (!dec)
        ERROR("Failed to create decoder for mimeType: '%s'", mimeType);
    else
        INFO("Created decoder for mimeType: '%s'", mimeType);

    return dec;
}

void releaseVideoDecoder(IVideoDecoder * p)
{
    delete p;
}

} // extern "C"
