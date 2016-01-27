/*
 *  encodehelp.h
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
#include "vppoutputencode.h"

#include <getopt.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <map>
#include <sys/stat.h>

typedef std::map<std::string, std::string> SSMap;

static const SSMap::value_type init_suffix[] =
{
    SSMap::value_type("H264", "264"),
    SSMap::value_type("AVC", "264"),
    SSMap::value_type("H265", "265"),
    SSMap::value_type("HEVC", "265"),
    SSMap::value_type("VP8", "vp8"),
    SSMap::value_type("JPEG", "jpeg")
};

static SSMap fileSuffix(init_suffix, init_suffix + 6);

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i <source yuv filename> load YUV from a file or container\n");
    printf("   -W <width> -H <height>\n");
    printf("   -o <coded file> optional\n");
    printf("   -b <bitrate: kbps> optional\n");
    printf("   -f <frame rate> optional\n");
    printf("   -c <codec: HEVC|AVC|VP8|JPEG>\n");
    printf("   -s <fourcc: NV12(default)>\n");
    printf("   -N <number of frames to encode(camera default 50), useful for camera>\n");
    printf("   --qp <initial qp> optional\n");
    printf("   --rcmode <CBR|CQP> optional\n");
    printf("   --ipbmode <0(I frame only ) | 1 (I and P frames) | 2 (I,P,B frames)> optional\n");
    printf("   --keyperiod <key frame period(default 30)> optional\n");
    printf("   --refnum <number of referece frames(default 1)> optional\n");

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

static bool processCmdLine(int argc, char *argv[], TranscodeParams& para)
{
    char opt;
    const struct option long_opts[] = {
        {"help", no_argument, NULL, 'h' },
        {"qp", required_argument, NULL, 0 },
        {"rcmode", required_argument, NULL, 0 },
        {"ipbmode", required_argument, NULL, 0 },
        {"keyperiod", required_argument, NULL, 0 },
        {"refnum", required_argument, NULL, 0 },
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
            para.m_encParams.bitRate = atoi(optarg) * 1024;//kbps to bps
            break;
        case 'f':
            para.m_encParams.fps = atoi(optarg);
            break;
        case 'c':
            para.m_encParams.codec = optarg;
            break;
        case 's':
            para.fourcc = atoi(optarg);
            break;
        case 'N':
            para.frameCount = atoi(optarg);
            break;
        case 0:
             switch (option_index) {
                case 1:
                    para.m_encParams.initQp = atoi(optarg);
                    break;
                case 2:
                    para.m_encParams.rcMode = string_to_rc_mode(optarg);
                    break;
                case 3:
                    para.m_encParams.ipbMode = atoi(optarg);
                    break;
                case 4:
                    para.m_encParams.kIPeriod = atoi(optarg);
                    break;
                case 5:
                    para.m_encParams.numRefFrames = atoi(optarg);
            }
        }
    }

    if (para.inputFileName.empty()) {
        fprintf(stderr, "can not encode without input file\n");
        return false;
    }

    std::string suffix;
    if (!para.m_encParams.codec.empty()) {
        std::string codec = para.m_encParams.codec;
        SSMap::iterator it = fileSuffix.find(codec);
        if (it == fileSuffix.end()) {
            fprintf(stderr, "codec:%s, not supported", para.m_encParams.codec.c_str());
            return false;
        }
        suffix = it->second;
    } else {
        para.m_encParams.codec = "AVC";
        suffix = fileSuffix["AVC"];
    }

    uint32_t fourcc = guessFourcc(para.inputFileName.c_str());
    if (fourcc != para.fourcc) {
        fprintf(stderr, "Note: inconsistency between the fourcc type and the file type, use the fourcc of file.\n");
        para.fourcc = fourcc;
    }

    if (!para.oWidth || !para.oHeight)
        guessResolution(para.inputFileName.c_str(), para.oWidth, para.oHeight);

    if (para.outputFileName.empty())
        para.outputFileName = "./";
    struct stat buf;
    int r = stat(para.outputFileName.c_str(), &buf);
    if (r == 0 && buf.st_mode & S_IFDIR) {
        std::ostringstream outputfile;
        const char* fileName = para.inputFileName.c_str();
        const char* s = strrchr(para.inputFileName.c_str(), '/');
        if (s) fileName = s + 1;
        outputfile<<para.outputFileName<<"/"<<fileName;
        outputfile<<"."<<suffix;
        para.outputFileName = outputfile.str();
    }

    if ((para.m_encParams.rcMode == RATE_CONTROL_CBR) && (para.m_encParams.bitRate <= 0)) {
        fprintf(stderr, "please make sure bitrate is positive when CBR mode\n");
        return false;
    }

    if (!strncmp(para.inputFileName.c_str(), "/dev/video", strlen("/dev/video")) && !para.frameCount)
        para.frameCount = 50;

    return true;
}

#endif
