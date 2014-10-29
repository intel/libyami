/*
 *  utils.h - util functions
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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
#ifndef utils_h
#define utils_h

#include "interface/VideoCommonDefs.h"

#include <stdint.h>


#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 VA_FOURCC('I','4','2','0')
#endif

namespace YamiMediaCodec{

uint32_t guessFourcc(const char* fileName);

bool getPlaneResolution(uint32_t fourcc, uint32_t pixelWidth, uint32_t pixelHeight, uint32_t byteWidth[3], uint32_t byteHeight[3],  uint32_t& planes);

bool fillFrameRawData(VideoFrameRawData* frame, uint32_t fourcc, uint32_t width, uint32_t height, uint8_t* data);

class CalcFps
{
  public:
    CalcFps() { m_timeStart = 0;};
    void setAnchor();
    float fps(uint32_t frameCount);

  private:
    uint64_t m_timeStart;
};

};

#endif
