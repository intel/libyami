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
#ifndef utils_h
#define utils_h

#include "interface/VideoCommonDefs.h"

#include <stdint.h>


#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 VA_FOURCC('I','4','2','0')
#endif

namespace YamiMediaCodec{

uint32_t guessFourcc(const char* fileName);

bool guessResolution(const char* filename, int& w, int& h);

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

//slim version of CalcFps, hide all fps all fps detials.
class FpsCalc
{
public:
    FpsCalc();
    void addFrame();
    void log();
private:
    int m_frames;
    uint64_t m_start;
    uint64_t m_netStart;
    static const int NET_FPS_START = 5;
};
};

#endif
