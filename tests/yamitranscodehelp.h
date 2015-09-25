/*
 *  yamiencodehelp.h - yami encode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Liu Yangbin<yangbinx.liu@intel.com>
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

#ifndef __YAMI_TRANSCODE_HELP__
#define __YAMI_TRANSCODE_HELP__
#include "interface/VideoEncoderDefs.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <string>
#include <limits.h>

using std::string;

class YamiEncodeParam {
public:
    YamiEncodeParam();

	VideoRateControl rcMode;
    uint32_t frameCount;
    int32_t initQp;
    int32_t oWidth; /*output video width*/
    int32_t oHeight; /*output vide height*/
    int32_t bitRate;
    int32_t fps;
    int32_t ipPeriod;
    int32_t ipbMode;
    int32_t kIPeriod;
    uint32_t fourcc;
    string inputFileName;
    string outputFileName;
    string codec;
};

YamiEncodeParam::YamiEncodeParam():rcMode(RATE_CONTROL_CQP),
    frameCount(UINT_MAX),initQp(26),oWidth(0),oHeight(0),bitRate(0),
    fps(30),ipPeriod(1),ipbMode(1),kIPeriod(30),fourcc(0)
{
    /*nothing to do*/
}

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

static bool processCmdLine(int argc, char *argv[], YamiEncodeParam& para)
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
            para.inputFileName = optarg;
            break;
        case 'o':
            para.outputFileName = optarg;
            break;
        case 'W':
            para.oWidth = atoi(optarg);
            break;
        case 'H':
            para.oHeight = atoi(optarg);
            break;
        case 'b':
            para.bitRate = atoi(optarg) * 1024;//kbps to bps
            break;
        case 'f':
            para.fps = atoi(optarg);
            break;
        case 'c':
            para.codec = optarg;
            break;
        case 's':
            if (strlen(optarg) == 4)
                para.fourcc = VA_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
            break;
        case 'N':
            para.frameCount = atoi(optarg);
            break;
        case 0:
             switch (option_index) {
                case 1:
                    para.initQp = atoi(optarg);
                    break;
                case 2:
                    para.rcMode = string_to_rc_mode(optarg);
                    break;
            }
        }
    }

    if (para.inputFileName.empty()) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }
    if (para.outputFileName.empty())
        para.outputFileName = "test.264";

    if ((para.rcMode == RATE_CONTROL_CBR) && (para.bitRate <= 0)) {
        fprintf(stderr, "please make sure bitrate is positive when CBR mode\n");
        return false;
    }

    if (!strncmp(para.inputFileName.c_str(), "/dev/video", strlen("/dev/video")) && !para.frameCount)
        para.frameCount = 50;

    return true;
}

void ensureInputParameters(YamiEncodeParam& cmd)
{
    if (!cmd.fps)
        cmd.fps = 30;

    if (cmd.ipbMode == 0)
        cmd.ipPeriod = 0;
    else if (cmd.ipbMode == 1)
        cmd.ipPeriod = 1;
    else if (cmd.ipbMode == 2)
        cmd.ipPeriod = 3;

    /* if (!cmd.bitRate) {
        int rawBitRate = cmd.oWidth * cmd.oHeight * cmd.fps  * 3 / 2 * 8;
        int EmpiricalVaue = 40;
        cmd.bitRate = rawBitRate / EmpiricalVaue;
    } */

}

void setEncoderParameters(VideoParamsCommon * encVideoParams, YamiEncodeParam& cmd)
{
    ensureInputParameters(cmd);
    //resolution
    encVideoParams->resolution.width = cmd.oWidth;
    encVideoParams->resolution.height = cmd.oHeight;

    //frame rate parameters.
    encVideoParams->frameRate.frameRateDenom = 1;
    encVideoParams->frameRate.frameRateNum = cmd.fps;

    //picture type and bitrate
    encVideoParams->intraPeriod = cmd.kIPeriod;
    encVideoParams->ipPeriod = cmd.ipPeriod;
    encVideoParams->rcParams.bitRate = cmd.bitRate;
    encVideoParams->rcParams.initQP = cmd.initQp;
    encVideoParams->rcMode = cmd.rcMode;
    //encVideoParams->rcParams.minQP = 1;

    //encVideoParams->profile = VAProfileH264Main;
    //encVideoParams->profile = VAProfileVP8Version0_3;
    encVideoParams->rawFormat = RAW_FORMAT_YUV420;
}
#endif
