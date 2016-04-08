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

#ifndef __ENCODE_HELP__
#define __ENCODE_HELP__
#include "interface/VideoEncoderDefs.h"
#include <getopt.h>

static int idrInterval = 0;
static int intraPeriod = 30;
static int ipPeriod = 1;
static char *inputFileName = NULL;
static char defaultOutputFile[] = "test.264";
static char *outputFileName = defaultOutputFile;
static char *codec = NULL;
static uint32_t inputFourcc = 0;
static int videoWidth = 0, videoHeight = 0, bitRate = 0, fps = 30;
static int initQp=26;
static VideoRateControl rcMode = RATE_CONTROL_CQP;
static int frameCount = 0;
static int numRefFrames = 1;

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
    printf("   --ipperiod <0 (I frame only) | 1 (I and P frames) | N (I,P and B frames, B frame number is N-1)> optional\n");
    printf("   --intraperiod <Intra frame period (default 30)> optional\n");
    printf("   --refnum <number of referece frames(default 1)> optional\n");
    printf("   --idrinterval <AVC/HEVC IDR frame interval (default 0)> optional\n");
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
        {"ipperiod", required_argument, NULL, 0 },
        {"intraperiod", required_argument, NULL, 0 },
        {"refnum", required_argument, NULL, 0 },
        {"idrinterval", required_argument, NULL, 0 },
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
                case 3:
                    ipPeriod = atoi(optarg);
                    break;
                case 4:
                    intraPeriod = atoi(optarg);
                    break;
                case 5:
                    numRefFrames= atoi(optarg);
                    break;
                case 6:
                    idrInterval = atoi(optarg);
                    break;
            }
        }
    }

#if !ANDROID
    if (!inputFileName) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }
#endif

    if ((rcMode == RATE_CONTROL_CBR) && (bitRate <= 0)) {
        fprintf(stderr, "please make sure bitrate is positive when CBR mode\n");
        return false;
    }

    if (inputFileName && !strncmp(inputFileName, "/dev/video", strlen("/dev/video")) && !frameCount)
        frameCount = 50;

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
    encVideoParams->intraPeriod = intraPeriod;
    encVideoParams->ipPeriod = ipPeriod;
    encVideoParams->rcParams.bitRate = bitRate;
    encVideoParams->rcParams.initQP = initQp;
    encVideoParams->rcMode = rcMode;
    encVideoParams->numRefFrames = numRefFrames;
    //encVideoParams->rcParams.minQP = 1;

    //encVideoParams->profile = VAProfileH264Main;
 //   encVideoParams->profile = VAProfileVP8Version0_3;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;

}
#endif
