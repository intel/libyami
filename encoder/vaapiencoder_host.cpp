/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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

#include "common/log.h"
#include "interface/VideoEncoderHost.h"
#include "vaapiencoder_factory.h"

using namespace YamiMediaCodec;

extern "C" {

IVideoEncoder* createVideoEncoder(const char* mimeType) {
    yamiTraceInit();

    if (!mimeType) {
        ERROR("NULL mime type.");
        return NULL;
    }

    VaapiEncoderFactory::Type enc = VaapiEncoderFactory::create(mimeType);

    if (!enc)
        ERROR("Failed to create encoder for mimeType: '%s'", mimeType);
    else
        INFO("Created encoder for mimeType: '%s'", mimeType);

    return enc;
}

void releaseVideoEncoder(IVideoEncoder* p) {
    delete p;
}

} // extern "C"
