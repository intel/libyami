/*
 *  utils.cpp - utils functions
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#include "utils.h"

#include "common/common_def.h"
#include "common/log.h"

#include <assert.h>
#include <string.h>
#include <va/va.h>

namespace YamiMediaCodec{

uint32_t guessFourcc(const char* fileName)
{
    static const char *possibleFourcc[] = {
            "I420", "NV12", "YV12",
            "YUY2", "UYVY",
            "RGBX", "BGRX", "XRGB", "XBGR"
        };
    const char* extension = strrchr(fileName, '.');
    if (extension) {
        extension++;
        for (int i = 0; i < N_ELEMENTS(possibleFourcc); i++) {
            const char* fourcc = possibleFourcc[i];
            if (!strcasecmp(fourcc, extension)) {
                uint32_t ret = VA_FOURCC(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
                DEBUG_FOURCC("guessed input fourcc:", ret);
                return ret;
            }
        }
    }

    return VA_FOURCC_I420;
}

bool getPlaneResolution(uint32_t fourcc, uint32_t pixelWidth, uint32_t pixelHeight, uint32_t byteWidth[3], uint32_t byteHeight[3],  uint32_t& planes)
{
    int w = pixelWidth;
    int h = pixelHeight;
    uint32_t* width = byteWidth;
    uint32_t* height = byteHeight;
    switch (fourcc) {
        case VA_FOURCC_NV12:
        case VA_FOURCC_I420:
        case VA_FOURCC_YV12:{
            width[0] = w;
            height[0] = h;
            if (fourcc == VA_FOURCC_NV12) {
                width[1]  = w + (w & 1);
                height[1] = (h + 1) >> 1;
                planes = 2;
            } else {
                width[1] = width[2] = (w + 1) >> 1;
                height[1] = height[2] = (h + 1) >> 1;
                planes = 3;
            }
            break;
        }
        case VA_FOURCC_YUY2:
        case VA_FOURCC_UYVY: {
            width[0] = w * 2;
            height[0] = h;
            planes = 1;
            break;
        }
        case VA_FOURCC_RGBX:
        case VA_FOURCC_RGBA:
        case VA_FOURCC_BGRX:
        case VA_FOURCC_BGRA: {
            width[0] = w * 4;
            height[0] = h;
            planes = 1;
            break;
        }
        default: {
            assert(0 && "do not support this format");
            planes = 0;
            return false;
        }
    }
    return true;
}

};