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
#include "vaapiencoder_jpeg.h"

// library headers
#include "common/utils.h"

// system headers
#include <string>
#include <tr1/array>
#include <vector>

const static std::tr1::array<uint8_t, 150> g_SimpleSmallI420 = {
    0x37, 0x38, 0x39, 0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x38, 0x38,
    0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x39, 0x39, 0x3b, 0x3b,
    0x3c, 0x3e, 0x3e, 0x40, 0x41, 0x42, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x44, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x40, 0x42, 0x43, 0x44, 0x45,
    0x3c, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x47, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x46, 0x47, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x45, 0x45, 0x46, 0x48, 0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46,
    0x46, 0x47, 0x49, 0x49, 0xe9, 0xda, 0xcb, 0xbd, 0xaf, 0xda, 0xcb, 0xbd,
    0xaf, 0xa1, 0xcb, 0xbd, 0xaf, 0xa0, 0x92, 0xbd, 0xae, 0xa0, 0x92, 0x83,
    0xaf, 0xa0, 0x92, 0x83, 0x74, 0x63, 0x73, 0x83, 0x92, 0xa2, 0x73, 0x83,
    0x92, 0xa2, 0xb1, 0x82, 0x92, 0xa1, 0xb1, 0xc1, 0x92, 0xa1, 0xb0, 0xc0,
    0xcf, 0xa0, 0xb0, 0xc0, 0xcf, 0xde
};

const static std::tr1::array<uint8_t, 150> g_SimpleSmallNV12 = {
    0x37, 0x38, 0x39, 0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x38, 0x38,
    0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x39, 0x39, 0x3b, 0x3b,
    0x3c, 0x3e, 0x3e, 0x40, 0x41, 0x42, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x44, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x40, 0x42, 0x43, 0x44, 0x45,
    0x3c, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x47, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x46, 0x47, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x45, 0x45, 0x46, 0x48, 0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46,
    0x46, 0x47, 0x49, 0x49, 0xe9, 0x63, 0xda, 0x73, 0xcb, 0x83, 0xbd, 0x92,
    0xaf, 0xa2, 0xda, 0x73, 0xcb, 0x83, 0xbd, 0x92, 0xaf, 0xa2, 0xa1, 0xb1,
    0xcb, 0x82, 0xbd, 0x92, 0xaf, 0xa1, 0xa0, 0xb1, 0x92, 0xc1, 0xbd, 0x92,
    0xae, 0xa1, 0xa0, 0xb0, 0x92, 0xc0, 0x83, 0xcf, 0xaf, 0xa0, 0xa0, 0xb0,
    0x92, 0xc0, 0x83, 0xcf, 0x74, 0xde
};

const static std::tr1::array<uint8_t, 150> g_SimpleSmallYV12 = {
    0x37, 0x38, 0x39, 0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x38, 0x38,
    0x39, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x39, 0x39, 0x3b, 0x3b,
    0x3c, 0x3e, 0x3e, 0x40, 0x41, 0x42, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x44, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x40, 0x42, 0x43, 0x44, 0x45,
    0x3c, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x47, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x46, 0x47, 0x3f, 0x40, 0x41, 0x42,
    0x43, 0x45, 0x45, 0x46, 0x48, 0x48, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46,
    0x46, 0x47, 0x49, 0x49, 0x5f, 0x6f, 0x7f, 0x8e, 0x9d, 0x6f, 0x7f, 0x8e,
    0x9e, 0xad, 0x7e, 0x8e, 0x9d, 0xad, 0xbd, 0x8e, 0x9e, 0xac, 0xbc, 0xcb,
    0x9c, 0xac, 0xbc, 0xcb, 0xda, 0xec, 0xdd, 0xce, 0xc0, 0xb2, 0xde, 0xcf,
    0xc1, 0xb2, 0xa4, 0xcf, 0xc0, 0xb2, 0xa3, 0x95, 0xc1, 0xb2, 0xa3, 0x95,
    0x86, 0xb2, 0xa3, 0x95, 0x87, 0x77
};

const static std::tr1::array<uint8_t, 200> g_SimpleSmallUYVY = {
    0xec, 0x37, 0x5f, 0x38, 0xdd, 0x39, 0x6f, 0x39, 0xce, 0x3b, 0x7f, 0x3c,
    0xc0, 0x3d, 0x8e, 0x3e, 0xb2, 0x3f, 0x9d, 0x40, 0xe5, 0x38, 0x67, 0x38,
    0xd6, 0x39, 0x77, 0x3b, 0xc7, 0x3c, 0x86, 0x3d, 0xb9, 0x3e, 0x96, 0x3f,
    0xab, 0x40, 0xa6, 0x41, 0xde, 0x39, 0x6f, 0x39, 0xcf, 0x3b, 0x7f, 0x3b,
    0xc1, 0x3c, 0x8e, 0x3e, 0xb2, 0x3e, 0x9e, 0x40, 0xa4, 0x41, 0xad, 0x42,
    0xd6, 0x3a, 0x76, 0x3b, 0xc7, 0x3c, 0x86, 0x3d, 0xb9, 0x3e, 0x95, 0x3f,
    0xab, 0x40, 0xa5, 0x41, 0x9d, 0x42, 0xb5, 0x43, 0xcf, 0x3b, 0x7e, 0x3c,
    0xc0, 0x3d, 0x8e, 0x3e, 0xb2, 0x3f, 0x9d, 0x40, 0xa3, 0x41, 0xad, 0x42,
    0x95, 0x43, 0xbd, 0x44, 0xc7, 0x3c, 0x86, 0x3d, 0xb9, 0x3e, 0x95, 0x3f,
    0xab, 0x40, 0xa4, 0x40, 0x9c, 0x42, 0xb5, 0x43, 0x8e, 0x44, 0xc4, 0x45,
    0xc1, 0x3c, 0x8e, 0x3e, 0xb2, 0x3f, 0x9e, 0x40, 0xa3, 0x41, 0xac, 0x42,
    0x95, 0x43, 0xbc, 0x44, 0x86, 0x46, 0xcb, 0x47, 0xb9, 0x3e, 0x95, 0x3f,
    0xaa, 0x40, 0xa4, 0x41, 0x9c, 0x42, 0xb4, 0x43, 0x8e, 0x44, 0xc4, 0x46,
    0x7f, 0x46, 0xd2, 0x47, 0xb2, 0x3f, 0x9c, 0x40, 0xa3, 0x41, 0xac, 0x42,
    0x95, 0x43, 0xbc, 0x45, 0x87, 0x45, 0xcb, 0x46, 0x77, 0x48, 0xda, 0x48,
    0xab, 0x40, 0xa4, 0x41, 0x9c, 0x42, 0xb4, 0x43, 0x8e, 0x44, 0xc4, 0x46,
    0x7f, 0x46, 0xd3, 0x47, 0x70, 0x49, 0xe2, 0x49
};

const static std::tr1::array<uint8_t, 200> g_SimpleSmallYUY2 = {
    0x37, 0xec, 0x38, 0x5f, 0x39, 0xdd, 0x39, 0x6f, 0x3b, 0xce, 0x3c, 0x7f,
    0x3d, 0xc0, 0x3e, 0x8e, 0x3f, 0xb2, 0x40, 0x9d, 0x38, 0xe5, 0x38, 0x67,
    0x39, 0xd6, 0x3b, 0x77, 0x3c, 0xc7, 0x3d, 0x86, 0x3e, 0xb9, 0x3f, 0x96,
    0x40, 0xab, 0x41, 0xa6, 0x39, 0xde, 0x39, 0x6f, 0x3b, 0xcf, 0x3b, 0x7f,
    0x3c, 0xc1, 0x3e, 0x8e, 0x3e, 0xb2, 0x40, 0x9e, 0x41, 0xa4, 0x42, 0xad,
    0x3a, 0xd6, 0x3b, 0x76, 0x3c, 0xc7, 0x3d, 0x86, 0x3e, 0xb9, 0x3f, 0x95,
    0x40, 0xab, 0x41, 0xa5, 0x42, 0x9d, 0x43, 0xb5, 0x3b, 0xcf, 0x3c, 0x7e,
    0x3d, 0xc0, 0x3e, 0x8e, 0x3f, 0xb2, 0x40, 0x9d, 0x41, 0xa3, 0x42, 0xad,
    0x43, 0x95, 0x44, 0xbd, 0x3c, 0xc7, 0x3d, 0x86, 0x3e, 0xb9, 0x3f, 0x95,
    0x40, 0xab, 0x40, 0xa4, 0x42, 0x9c, 0x43, 0xb5, 0x44, 0x8e, 0x45, 0xc4,
    0x3c, 0xc1, 0x3e, 0x8e, 0x3f, 0xb2, 0x40, 0x9e, 0x41, 0xa3, 0x42, 0xac,
    0x43, 0x95, 0x44, 0xbc, 0x46, 0x86, 0x47, 0xcb, 0x3e, 0xb9, 0x3f, 0x95,
    0x40, 0xaa, 0x41, 0xa4, 0x42, 0x9c, 0x43, 0xb4, 0x44, 0x8e, 0x46, 0xc4,
    0x46, 0x7f, 0x47, 0xd2, 0x3f, 0xb2, 0x40, 0x9c, 0x41, 0xa3, 0x42, 0xac,
    0x43, 0x95, 0x45, 0xbc, 0x45, 0x87, 0x46, 0xcb, 0x48, 0x77, 0x48, 0xda,
    0x40, 0xab, 0x41, 0xa4, 0x42, 0x9c, 0x43, 0xb4, 0x44, 0x8e, 0x46, 0xc4,
    0x46, 0x7f, 0x47, 0xd3, 0x49, 0x70, 0x49, 0xe2
};

const static std::tr1::array<uint8_t, 400> g_SimpleSmallBGRX = {
    0xff, 0x1b, 0x00, 0xff, 0xf2, 0x19, 0x0d, 0xff, 0xe5, 0x17, 0x1a, 0xff,
    0xd5, 0x13, 0x27, 0xff, 0xc9, 0x12, 0x36, 0xff, 0xbc, 0x10, 0x43, 0xff,
    0xaf, 0x0d, 0x51, 0xff, 0xa2, 0x0b, 0x5f, 0xff, 0x95, 0x08, 0x6c, 0xff,
    0x96, 0x09, 0x6e, 0xff, 0xf2, 0x19, 0x0d, 0xff, 0xe4, 0x15, 0x19, 0xff,
    0xd5, 0x13, 0x27, 0xff, 0xc9, 0x13, 0x35, 0xff, 0xbc, 0x10, 0x43, 0xff,
    0xaf, 0x0d, 0x51, 0xff, 0xa2, 0x0b, 0x5f, 0xff, 0x95, 0x09, 0x6b, 0xff,
    0x88, 0x06, 0x79, 0xff, 0x89, 0x08, 0x7a, 0xff, 0xe5, 0x17, 0x1a, 0xff,
    0xd5, 0x13, 0x27, 0xff, 0xc9, 0x12, 0x36, 0xff, 0xbb, 0x0f, 0x42, 0xff,
    0xae, 0x0c, 0x4f, 0xff, 0xa2, 0x0b, 0x5f, 0xff, 0x94, 0x07, 0x6b, 0xff,
    0x88, 0x06, 0x79, 0xff, 0x7b, 0x04, 0x87, 0xff, 0x7c, 0x05, 0x88, 0xff,
    0xd6, 0x15, 0x27, 0xff, 0xc9, 0x13, 0x35, 0xff, 0xbc, 0x10, 0x43, 0xff,
    0xaf, 0x0d, 0x51, 0xff, 0xa2, 0x0c, 0x5d, 0xff, 0x93, 0x09, 0x6b, 0xff,
    0x86, 0x07, 0x79, 0xff, 0x79, 0x04, 0x87, 0xff, 0x6c, 0x02, 0x95, 0xff,
    0x6d, 0x03, 0x96, 0xff, 0xc9, 0x13, 0x35, 0xff, 0xbc, 0x10, 0x43, 0xff,
    0xaf, 0x0d, 0x51, 0xff, 0xa2, 0x0c, 0x5d, 0xff, 0x95, 0x09, 0x6b, 0xff,
    0x86, 0x07, 0x79, 0xff, 0x79, 0x04, 0x87, 0xff, 0x6c, 0x02, 0x95, 0xff,
    0x5f, 0x00, 0xa3, 0xff, 0x60, 0x00, 0xa4, 0xff, 0xbc, 0x10, 0x43, 0xff,
    0xad, 0x0f, 0x4f, 0xff, 0xa0, 0x0c, 0x5d, 0xff, 0x93, 0x09, 0x6b, 0xff,
    0x86, 0x08, 0x77, 0xff, 0x78, 0x04, 0x84, 0xff, 0x6c, 0x02, 0x93, 0xff,
    0x5d, 0x00, 0xa1, 0xff, 0x50, 0x00, 0xaf, 0xff, 0x51, 0x00, 0xb0, 0xff,
    0xae, 0x0c, 0x4f, 0xff, 0xa0, 0x0c, 0x5d, 0xff, 0x93, 0x09, 0x6b, 0xff,
    0x86, 0x08, 0x77, 0xff, 0x79, 0x05, 0x85, 0xff, 0x6c, 0x02, 0x93, 0xff,
    0x5f, 0x00, 0xa1, 0xff, 0x50, 0x00, 0xad, 0xff, 0x44, 0x00, 0xbc, 0xff,
    0x46, 0x00, 0xbe, 0xff, 0xa2, 0x0c, 0x5d, 0xff, 0x93, 0x0a, 0x69, 0xff,
    0x86, 0x08, 0x77, 0xff, 0x79, 0x05, 0x85, 0xff, 0x6c, 0x02, 0x93, 0xff,
    0x5d, 0x01, 0x9f, 0xff, 0x50, 0x00, 0xad, 0xff, 0x44, 0x00, 0xbc, 0xff,
    0x34, 0x00, 0xc8, 0xff, 0x35, 0x00, 0xc9, 0xff, 0x95, 0x0a, 0x69, 0xff,
    0x86, 0x08, 0x77, 0xff, 0x79, 0x05, 0x85, 0xff, 0x6c, 0x02, 0x93, 0xff,
    0x5f, 0x00, 0xa1, 0xff, 0x51, 0x00, 0xaf, 0xff, 0x43, 0x00, 0xbb, 0xff,
    0x34, 0x00, 0xc8, 0xff, 0x28, 0x00, 0xd7, 0xff, 0x28, 0x00, 0xd7, 0xff,
    0x96, 0x0b, 0x6a, 0xff, 0x87, 0x09, 0x78, 0xff, 0x7a, 0x06, 0x86, 0xff,
    0x6d, 0x04, 0x94, 0xff, 0x60, 0x01, 0xa2, 0xff, 0x53, 0x01, 0xb0, 0xff,
    0x44, 0x00, 0xbc, 0xff, 0x35, 0x00, 0xc9, 0xff, 0x2a, 0x00, 0xd8, 0xff,
    0x2a, 0x00, 0xd8, 0xff
};

const static std::tr1::array <uint8_t, 400> g_SimpleSmallRGBX = {
    0x00, 0x1b, 0xff, 0xff, 0x0d, 0x19, 0xf2, 0xff, 0x1a, 0x17, 0xe5, 0xff,
    0x27, 0x13, 0xd5, 0xff, 0x36, 0x12, 0xc9, 0xff, 0x43, 0x10, 0xbc, 0xff,
    0x51, 0x0d, 0xaf, 0xff, 0x5f, 0x0b, 0xa2, 0xff, 0x6c, 0x08, 0x95, 0xff,
    0x6e, 0x09, 0x96, 0xff, 0x0d, 0x19, 0xf2, 0xff, 0x19, 0x15, 0xe4, 0xff,
    0x27, 0x13, 0xd5, 0xff, 0x35, 0x13, 0xc9, 0xff, 0x43, 0x10, 0xbc, 0xff,
    0x51, 0x0d, 0xaf, 0xff, 0x5f, 0x0b, 0xa2, 0xff, 0x6b, 0x09, 0x95, 0xff,
    0x79, 0x06, 0x88, 0xff, 0x7a, 0x08, 0x89, 0xff, 0x1a, 0x17, 0xe5, 0xff,
    0x27, 0x13, 0xd5, 0xff, 0x36, 0x12, 0xc9, 0xff, 0x42, 0x0f, 0xbb, 0xff,
    0x4f, 0x0c, 0xae, 0xff, 0x5f, 0x0b, 0xa2, 0xff, 0x6b, 0x07, 0x94, 0xff,
    0x79, 0x06, 0x88, 0xff, 0x87, 0x04, 0x7b, 0xff, 0x88, 0x05, 0x7c, 0xff,
    0x27, 0x15, 0xd6, 0xff, 0x35, 0x13, 0xc9, 0xff, 0x43, 0x10, 0xbc, 0xff,
    0x51, 0x0d, 0xaf, 0xff, 0x5d, 0x0c, 0xa2, 0xff, 0x6b, 0x09, 0x93, 0xff,
    0x79, 0x07, 0x86, 0xff, 0x87, 0x04, 0x79, 0xff, 0x95, 0x02, 0x6c, 0xff,
    0x96, 0x03, 0x6d, 0xff, 0x35, 0x13, 0xc9, 0xff, 0x43, 0x10, 0xbc, 0xff,
    0x51, 0x0d, 0xaf, 0xff, 0x5d, 0x0c, 0xa2, 0xff, 0x6b, 0x09, 0x95, 0xff,
    0x79, 0x07, 0x86, 0xff, 0x87, 0x04, 0x79, 0xff, 0x95, 0x02, 0x6c, 0xff,
    0xa3, 0x00, 0x5f, 0xff, 0xa4, 0x00, 0x60, 0xff, 0x43, 0x10, 0xbc, 0xff,
    0x4f, 0x0f, 0xad, 0xff, 0x5d, 0x0c, 0xa0, 0xff, 0x6b, 0x09, 0x93, 0xff,
    0x77, 0x08, 0x86, 0xff, 0x84, 0x04, 0x78, 0xff, 0x93, 0x02, 0x6c, 0xff,
    0xa1, 0x00, 0x5d, 0xff, 0xaf, 0x00, 0x50, 0xff, 0xb0, 0x00, 0x51, 0xff,
    0x4f, 0x0c, 0xae, 0xff, 0x5d, 0x0c, 0xa0, 0xff, 0x6b, 0x09, 0x93, 0xff,
    0x77, 0x08, 0x86, 0xff, 0x85, 0x05, 0x79, 0xff, 0x93, 0x02, 0x6c, 0xff,
    0xa1, 0x00, 0x5f, 0xff, 0xad, 0x00, 0x50, 0xff, 0xbc, 0x00, 0x44, 0xff,
    0xbe, 0x00, 0x46, 0xff, 0x5d, 0x0c, 0xa2, 0xff, 0x69, 0x0a, 0x93, 0xff,
    0x77, 0x08, 0x86, 0xff, 0x85, 0x05, 0x79, 0xff, 0x93, 0x02, 0x6c, 0xff,
    0x9f, 0x01, 0x5d, 0xff, 0xad, 0x00, 0x50, 0xff, 0xbc, 0x00, 0x44, 0xff,
    0xc8, 0x00, 0x34, 0xff, 0xc9, 0x00, 0x35, 0xff, 0x69, 0x0a, 0x95, 0xff,
    0x77, 0x08, 0x86, 0xff, 0x85, 0x05, 0x79, 0xff, 0x93, 0x02, 0x6c, 0xff,
    0xa1, 0x00, 0x5f, 0xff, 0xaf, 0x00, 0x51, 0xff, 0xbb, 0x00, 0x43, 0xff,
    0xc8, 0x00, 0x34, 0xff, 0xd7, 0x00, 0x28, 0xff, 0xd7, 0x00, 0x28, 0xff,
    0x6a, 0x0b, 0x96, 0xff, 0x78, 0x09, 0x87, 0xff, 0x86, 0x06, 0x7a, 0xff,
    0x94, 0x04, 0x6d, 0xff, 0xa2, 0x01, 0x60, 0xff, 0xb0, 0x01, 0x53, 0xff,
    0xbc, 0x00, 0x44, 0xff, 0xc9, 0x00, 0x35, 0xff, 0xd8, 0x00, 0x2a, 0xff,
    0xd8, 0x00, 0x2a, 0xff
};

namespace YamiMediaCodec {

class VaapiEncoderJpegTest
    : public FactoryTest<IVideoEncoder, VaapiEncoderJpeg>
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

#define VAAPIENCODER_JPEG_TEST(name) \
    TEST_F(VaapiEncoderJpegTest, name)

VAAPIENCODER_JPEG_TEST(Factory) {
    FactoryKeys mimeTypes;
    mimeTypes.push_back(YAMI_MIME_JPEG);
    doFactoryTest(mimeTypes);
}

class SimpleDataTest
    : public VaapiEncoderJpegTest
    , public ::testing::WithParamInterface<const char*>
{
protected:
    uint8_t* getData() const
    {
        const uint8_t* result(NULL);
        switch (getFourcc()) {
        case VA_FOURCC_NV12:
            result = g_SimpleSmallNV12.data();
            break;
        case VA_FOURCC_I420:
            result = g_SimpleSmallI420.data();
            break;
        case VA_FOURCC_YV12:
            result = g_SimpleSmallYV12.data();
            break;
        case VA_FOURCC_YUY2:
            result = g_SimpleSmallYUY2.data();
            break;
        case VA_FOURCC_UYVY:
            result = g_SimpleSmallUYVY.data();
            break;
        case VA_FOURCC_RGBX:
            result = g_SimpleSmallRGBX.data();
            break;
        case VA_FOURCC_BGRX:
            result = g_SimpleSmallBGRX.data();
            break;
        default:
            ADD_FAILURE() << "Test data is undefined for " << GetParam();
        }
        return const_cast<uint8_t*>(result);
    }

    uint32_t getFourcc() const
    {
        const std::string param(GetParam());
        return VA_FOURCC(param[0], param[1], param[2], param[3]);
    }
};

#define VAAPIENCODER_JPEG_TEST_SIMPLE_DATA(name) \
    TEST_P(SimpleDataTest, name)

VAAPIENCODER_JPEG_TEST_SIMPLE_DATA(Encode)
{
    uint8_t* data = getData();
    uint32_t fourcc = getFourcc();

    ASSERT_FALSE(HasFailure());

    VaapiEncoderJpeg encoder;
    VideoParamsCommon parameters = {.size = sizeof(VideoParamsCommon)};

    encoder.getParameters(VideoParamsTypeCommon, &parameters);
    parameters.resolution.width = 10;
    parameters.resolution.height = 10;
    encoder.setParameters(VideoParamsTypeCommon, &parameters);

    ASSERT_EQ(YAMI_SUCCESS, encoder.start());

    VideoFrameRawData frame = { };

    ASSERT_TRUE(fillFrameRawData(&frame, fourcc, 10, 10, data));

    ASSERT_EQ(YAMI_SUCCESS, encoder.encode(&frame));

    uint32_t size;
    encoder.getMaxOutSize(&size);
    std::vector<uint8_t> buffer(size, 0);
    VideoEncOutputBuffer output;
    output.data = const_cast<uint8_t*>(buffer.data());
    output.bufferSize = buffer.size();
    output.format = OUTPUT_EVERYTHING;

    EXPECT_EQ(YAMI_SUCCESS, encoder.getOutput(&output, false));
}

INSTANTIATE_TEST_CASE_P(
    VaapiEncoderJpegTest, SimpleDataTest,
    ::testing::Values("NV12", "I420", "YUY2")

    // The following fourcc's cause the test program to abort
    // (see https://github.com/01org/libyami/issues/439):
    //
    // "YV12", "UYVY", "RGBX", "BGRX", "BGRA", "RGBA"
);

}
