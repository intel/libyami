/*
 *  h264_encode.cpp - h264 encode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Cong Zhong<congx.zhong@intel.com>
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

#ifndef __ENCODE_HELP__
#define __ENCODE_HELP__
#include "interface/VideoEncoderDefs.h"

static const int kIPeriod = 30;
static char *inputFileName = NULL;
static char *outputFileName = NULL;
static char *codec = NULL;
static uint32_t inputFourcc = 0;
static int videoWidth = 0, videoHeight = 0, bitRate = 0, fps = 30;
static int frameCount = 0;
#ifdef __BUILD_GET_MV__
static FILE *MVFp;
#endif

#ifndef __cplusplus
#ifndef bool
#define bool  int
#endif

#ifndef true
#define true  1
#endif

#ifndef false
#define false 0
#endif
#endif

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i <source yuv filename> load YUV from a file\n");
    printf("   -W <width> -H <height>\n");
    printf("   -o <coded file> optional\n");
    printf("   -b <bitrate> optional\n");
    printf("   -f <frame rate> optional\n");
    printf("   -c <codec: AVC|VP8|JPEG> Note: not support now\n");
    printf("   -s <fourcc: NV12|IYUV|YV12> Note: not support now\n");
    printf("   -N <number of frames to encode(camera default 50), useful for camera>\n");
}

static bool process_cmdline(int argc, char *argv[])
{
    char opt;

    if (argc < 2) {
        fprintf(stderr, "can not encode without option, please type 'h264encode -h' to help\n");
        return false;
    }

    while ((opt = getopt(argc, argv, "W:H:b:f:c:s:i:o:N:h:")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
            inputFileName = optarg;
            break;
        case 'o':
            outputFileName = optarg;
            break;
        case 'W':
            videoWidth = atoi(optarg);
            break;
        case 'H':
            videoHeight = atoi(optarg);
            break;
        case 'b':
            bitRate = atoi(optarg);
            break;
        case 'f':
            fps = atoi(optarg);
            break;
        case 'c':
            codec = optarg;
            break;
        case 's':
            if (strlen(optarg) == 4)
                inputFourcc = VA_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
            break;
        case 'N':
            frameCount = atoi(optarg);
            break;
        }
    }

    if (!inputFileName) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }

    if (!strncmp(inputFileName, "/dev/video", strlen("/dev/video")) && !frameCount)
        frameCount = 50;

    if (!outputFileName)
        outputFileName = "test.264";

    return true;
}

void ensureInputParameters()
{
    if (!fps)
        fps = 30;

    if (!bitRate) {
        int rawBitRate = videoWidth * videoHeight * fps  * 3 / 2 * 8;
        int EmpiricalVaue = 40;
        bitRate = rawBitRate / EmpiricalVaue;
    }

}

void setEncoderParameters(VideoParamsCommon * encVideoParams)
{
    ensureInputParameters();
    //resolution
    encVideoParams->resolution.width = videoWidth;
    encVideoParams->resolution.height = videoHeight;

    //frame rate parameters.
    encVideoParams->frameRate.frameRateDenom = 1;
    encVideoParams->frameRate.frameRateNum = fps;

    //picture type and bitrate
    encVideoParams->intraPeriod = kIPeriod;
    encVideoParams->rcParams.bitRate = bitRate;
    //encVideoParams->rcParams.initQP = 26;
    //encVideoParams->rcParams.minQP = 1;

    //encVideoParams->profile = VAProfileH264Main;
 //   encVideoParams->profile = VAProfileVP8Version0_3;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;

}
#endif
