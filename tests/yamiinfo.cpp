/*
 * Copyright (C) 2011-2015 Intel Corporation. All rights reserved.
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

#include <iostream>
#include "common/YamiVersion.h"
#include "decoder/vaapidecoder_factory.h"
#include "encoder/vaapiencoder_factory.h"
#include "vpp/vaapipostprocess_factory.h"

using namespace YamiMediaCodec;

template<class Factory>
void printFactoryInfo(const char* name)
{
    std::cout << std::endl << name << ":" << std::endl;
    typename Factory::Keys keys(Factory::keys());
    for (size_t i(0); i < keys.size(); ++i) {
        std::cout << "\t" << keys[i] << std::endl;
    }
}

void printApiVersion()
{
   uint32_t api;
   yamiGetApiVersion(&api);

   //uint32 just for print format
   uint32_t major = api >> 24;
   uint32_t minor = (api >> 16) & 0xff;
   uint32_t micro = (api >> 8) & 0xff;

   std::cout << "API: " << major << "." << minor << "." << micro << std::endl;
}

int main(int argc, const char** argv)
{

    std::cout << PACKAGE_STRING << " - " << PACKAGE_URL << std::endl;

    std::cout<<std::endl;
    printApiVersion();

    printFactoryInfo<VaapiDecoderFactory>("decoders");
    printFactoryInfo<VaapiEncoderFactory>("encoders");
    printFactoryInfo<VaapiPostProcessFactory>("post processors");

    return 0;
}
