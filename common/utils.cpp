/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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

#include "utils.h"

#include "common/common_def.h"
#include "common/log.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>
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
        for (size_t i = 0; i < N_ELEMENTS(possibleFourcc); i++) {
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

bool guessResolution(const char* filename, int& w, int& h)
{
    enum {
        STATE_START,
        STATE_WDITH,
        STATE_X,
        STATE_HEIGHT,
        STATE_END,
    };
    int state = STATE_START;
    const char* p = filename;
    const char* tokStart = NULL;
    w = h = 0;
    while (*p != '\0') {
        switch (state) {
            case STATE_START:
            {
                if (isdigit(*p)) {
                    tokStart = p;
                    state = STATE_WDITH;
                }
                break;
            }
            case STATE_WDITH:
            {
                if (*p == 'x' || *p == 'X') {
                    state = STATE_X;
                    sscanf(tokStart, "%d", &w);
                } else if (!isdigit(*p)){
                    state = STATE_START;
                }
                break;
            }
            case STATE_X:
            {
                if (isdigit(*p)) {
                    tokStart = p;
                    state  = STATE_HEIGHT;
                } else {
                    state = STATE_START;
                }
                break;
            }
            case STATE_HEIGHT:
            {
                if (!isdigit(*p)) {
                    state = STATE_END;
                    sscanf(tokStart, "%d", &h);
                }
                break;
            }
        }
        if (state == STATE_END)
            break;
        p++;
    }
    //conner case
    if (*p == '\0' && state == STATE_HEIGHT) {
        if (!isdigit(*p)) {
            sscanf(tokStart, "%d", &h);
        }
    }
    return w && h;
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

/// return system clock in ms
uint64_t getSystemTime()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return tv.tv_usec/1000+tv.tv_sec*1000;
}

double  getFps(uint64_t current, uint64_t start, int frames)
{
    uint64_t sysTime = current - start;
    double fps = frames*1000.0/sysTime;
    return fps;
}

FpsCalc::FpsCalc():m_frames(0) {}

void FpsCalc::addFrame()
{
    if (m_frames == 0) {
        m_start = getSystemTime();
    } else if (m_frames == NET_FPS_START){
        m_netStart = getSystemTime();
    }
    m_frames++;
}
void FpsCalc::log()
{
    uint64_t current = getSystemTime();
    if (m_frames > 0) {
        printf("%d frame decoded, fps = %.2f. ", m_frames, getFps(current, m_start,m_frames));
    }
    if (m_frames > NET_FPS_START) {
        printf("fps after %d frames = %.2f.", NET_FPS_START, getFps(current, m_netStart, m_frames-NET_FPS_START));
    }
    printf("\n");
}
void CalcFps::setAnchor()
{
    m_timeStart = getSystemTime();
}

float CalcFps::fps(uint32_t frameCount)
{
    if (!m_timeStart) {
        fprintf(stderr, "anchor point isn't set, please call setAnchor first\n");
        return 0.0f;
    }

    uint64_t sysTime = getSystemTime() - m_timeStart;
    float fps = frameCount*1000.0/sysTime;
    fprintf(stdout, "rendered frame count: %d in %ld ms; fps=%.2f\n",
            frameCount, sysTime, fps);

    return fps;
}

bool fillFrameRawData(VideoFrameRawData* frame, uint32_t fourcc, uint32_t width, uint32_t height, uint8_t* data)
{
    memset(frame, 0, sizeof(*frame));
    uint32_t planes;
    uint32_t w[3], h[3];
    if (!getPlaneResolution(fourcc, width, height, w, h, planes))
        return false;
    frame->fourcc = fourcc;
    frame->width = width;
    frame->height = height;
    frame->handle = reinterpret_cast<intptr_t>(data);
    frame->memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;

    uint32_t offset = 0;
    for (uint32_t i = 0; i < planes; i++) {
        frame->pitch[i] = w[i];
        frame->offset[i] = offset;
        offset += w[i] * h[i];
    }
    return true;
}

};
