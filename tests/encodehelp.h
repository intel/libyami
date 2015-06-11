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
#include <getopt.h>

static const int kIPeriod = 30;
static char *inputFileName = NULL;
static char *outputFileName = NULL;
static char *codec = NULL;
static uint32_t inputFourcc = 0;
static int videoWidth = 0, videoHeight = 0, bitRate = 0, fps = 30;
static int initQp=26;
static VideoRateControl rcMode = RATE_CONTROL_CQP;
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
    printf("   -b <bitrate: kbps> optional\n");
    printf("   -f <frame rate> optional\n");
    printf("   -c <codec: AVC|VP8|JPEG> Note: not support now\n");
    printf("   -s <fourcc: NV12|IYUV|YV12> Note: not support now\n");
    printf("   -N <number of frames to encode(camera default 50), useful for camera>\n");
    printf("   --qp <initial qp> optional\n");
    printf("   --rcmode <CBR|CQP> optional\n");
}

static VideoRateControl string_to_rc_mode(char *str)
{
    VideoRateControl rcMode;

    if (!strcasecmp (str, "CBR"))
        rcMode = RATE_CONTROL_CBR;
    else if (!strcasecmp (str, "CQP"))
        rcMode = RATE_CONTROL_CQP;
    else {
        printf("Unsupport  RC mode\n");
        rcMode = RATE_CONTROL_NONE;
    }
    return rcMode;
}

static bool process_cmdline(int argc, char *argv[])
{
    char opt;
    const struct option long_opts[] = {
        {"help", no_argument, NULL, 'h' },
        {"qp", required_argument, NULL, 0 },
        {"rcmode", required_argument, NULL, 0 },
        {NULL, no_argument, NULL, 0 }};
    int option_index;

    if (argc < 2) {
        fprintf(stderr, "can not encode without option, please type 'yamiencode -h' to help\n");
        return false;
    }

    while ((opt = getopt_long_only(argc, argv, "W:H:b:f:c:s:i:o:N:h:", long_opts,&option_index)) != -1)
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
            bitRate = atoi(optarg) * 1024;//kbps to bps
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
        case 0:
             switch (option_index) {
                case 1:
                    initQp = atoi(optarg);
                    break;
                case 2:
                    rcMode = string_to_rc_mode(optarg);
                    break;
            }
        }
    }

    if (!inputFileName) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }

    if ((rcMode == RATE_CONTROL_CBR) && (bitRate <= 0)) {
        fprintf(stderr, "please make sure bitrate is positive when CBR mode\n");
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

    /* if (!bitRate) {
        int rawBitRate = videoWidth * videoHeight * fps  * 3 / 2 * 8;
        int EmpiricalVaue = 40;
        bitRate = rawBitRate / EmpiricalVaue;
    } */

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
    encVideoParams->rcParams.initQP = initQp;
    encVideoParams->rcMode = rcMode;
    //encVideoParams->rcParams.minQP = 1;

    //encVideoParams->profile = VAProfileH264Main;
 //   encVideoParams->profile = VAProfileVP8Version0_3;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;

}
#endif
