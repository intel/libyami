/*
 *  Copyright (C) 2011-2015 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
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

int main(int argc, const char** argv)
{
    std::cout << PACKAGE_STRING << " - " << PACKAGE_URL << std::endl;

    printFactoryInfo<VaapiDecoderFactory>("decoders");
    printFactoryInfo<VaapiEncoderFactory>("encoders");
    printFactoryInfo<VaapiPostProcessFactory>("post processors");

    return 0;
}
