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

#ifndef VaapiUtils_h
#define VaapiUtils_h

#include "common/log.h"
#include <va/va.h>

namespace YamiMediaCodec {

uint8_t* mapSurfaceToImage(VADisplay display, intptr_t surface, VAImage& image);

void unmapImage(VADisplay display, const VAImage& image);

#define checkVaapiStatus(status, prompt)                     \
    (                                                        \
        {                                                    \
            bool ret;                                        \
            ret = (status == VA_STATUS_SUCCESS);             \
            if (!ret)                                        \
                ERROR("%s: %s", prompt, vaErrorStr(status)); \
            ret;                                             \
        })
}

#endif //VaapiUtils_h
