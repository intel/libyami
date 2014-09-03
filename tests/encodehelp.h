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

const int kIPeriod = 30;
char *inputFileName = NULL;
char *outputFileName = NULL;
char *codec = NULL;
char *colorspace = NULL;
int videoWidth = 0, videoHeight = 0, bitRate = 0, fps = 0;

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
}

static bool process_cmdline(int argc, char *argv[])
{
    char opt;

    if (argc < 2) {
        fprintf(stderr, "can not encode without option, please type 'h264encode -h' to help\n");
        return false;
    }

    while ((opt = getopt(argc, argv, "W:H:b:f:c:s:i:o:h:")) != -1)
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
            colorspace = optarg;
            break;
        }
    }

    if (!inputFileName) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }

    if (!videoWidth || !videoHeight) {
        fprintf(stderr, "can not encode without video/height\n");
        return false;
    }

    if (!fps)
        fps = 30;

    if (!bitRate) {
        int rawBitRate = videoWidth * videoHeight * fps  * 3 / 2 * 8;
        int EmpiricalVaue = 40;
        bitRate = rawBitRate / EmpiricalVaue;
    }

    if (!outputFileName)
        outputFileName = "test.yuv";

    return true;
}

void setEncoderParameters(VideoParamsCommon * encVideoParams)
{
    //resolution
    encVideoParams->resolution.width = videoWidth;
    encVideoParams->resolution.height = videoHeight;

    //frame rate parameters.
    encVideoParams->frameRate.frameRateDenom = 1;
    encVideoParams->frameRate.frameRateNum = fps;

    //picture type and bitrate
    encVideoParams->intraPeriod = kIPeriod;
    encVideoParams->rcMode = RATE_CONTROL_CBR;
    encVideoParams->rcParams.bitRate = bitRate;
    //encVideoParams->rcParams.initQP = 26;
    //encVideoParams->rcParams.minQP = 1;

    //encVideoParams->profile = VAProfileH264Main;
 //   encVideoParams->profile = VAProfileVP8Version0_3;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;

    encVideoParams->level = 31;
}
#endif
