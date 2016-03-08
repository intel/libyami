/*
 *  decodehelp.h - decode test help
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *            Xin Tang<xin.t.tang@intel.com>
 *            Lin Hai<hai1.lin@intel.com>
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
#ifndef decodehelp_h
#define decodehelp_h

#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include "common/utils.h"

using namespace YamiMediaCodec;

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i media file to decode\n");
    printf("   -w wait before quit: 0:no-wait, 1:auto(jpeg wait), 2:wait\n");
    printf("   -f dumped fourcc [*]\n");
    printf("   -o dumped output dir\n");
    printf("   -n specifiy how many frames to be decoded\n");
    printf("   -m <render mode>\n");
    printf("     -2: print MD5 by per frame and the whole decoded file MD5\n");
    printf("     -1: skip video rendering [*]\n");
    printf("      0: dump video frame to file\n");
    printf("      1: render to X window [*]\n");
    printf("      2: texture: render to Pixmap + texture from Pixmap [*]\n");
    printf("      3: texture: export video frame as drm name (RGBX) + texture from drm name\n");
    printf("      4: texture: export video frame as dma_buf(RGBX) + texutre from dma_buf\n");
    printf("      5: texture: export video frame as dma_buf(NV12) + texture from dma_buf. not implement yet\n");
    printf(" [*] v4l2decode doesn't support the option\n");
}

typedef struct DecodeParameter
{
    char*    inputFile;
    int      width;
    int      height;
    short    renderMode;
    short    waitBeforeQuit;
    uint32_t renderFrames;
    uint32_t renderFourcc;
    std::string outputFile;
} DecodeParameter;

bool processCmdLine(int argc, char** argv, DecodeParameter* parameters)
{
    bool isSetFourcc = false;
    char* outputFile = NULL;
    parameters->renderFrames   = UINT_MAX;
    parameters->waitBeforeQuit = 1;
    parameters->renderMode     = 1;
    parameters->inputFile      = NULL;

    char opt;
    while ((opt = getopt(argc, argv, "h:m:n:i:f:o:w:?")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
                parameters->inputFile = optarg;
            break;
        case 'w':
            if (optarg)
                parameters->waitBeforeQuit = atoi(optarg);
            break;
        case 'm':
            if (optarg)
                parameters->renderMode = atoi(optarg);
            break;
        case 'n':
            if (optarg)
                parameters->renderFrames = atoi(optarg);
            break;
        case 'f':
            if (optarg) {
                if (strlen(optarg) == 4) {
                    parameters->renderFourcc = VA_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
                    isSetFourcc = true;
                } else {
                    fprintf(stderr, "invalid fourcc: %s\n", optarg);
                    return false;
                }
            }
            break;
        case 'o':
            if (optarg)
                outputFile = strdup(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    if (!parameters->inputFile) {
        fprintf(stderr, "no input media file specified.\n");
        return false;
    }
    if (!outputFile)
        outputFile = strdup ("./");
    parameters->outputFile = outputFile;
    free(outputFile);
    if (!isSetFourcc)
        parameters->renderFourcc = guessFourcc(parameters->outputFile.c_str());
    int width, height;
    if (guessResolution(parameters->inputFile, width, height)) {
        parameters->width =  width;
        parameters->height =  height;
    }
    return true;
}

static bool possibleWait(const char* mimeType, short wait, short mode)
{
    // waitBeforeQuit 0:no-wait, 1:auto(jpeg wait), 2:wait
    switch(wait) {
    case 0:
        break;
    case 1:
        if (mode == 0 || strcmp(mimeType, YAMI_MIME_JPEG))
            break;
    case 2:
        fprintf(stdout, "press any key to continue ...");
        getchar();
        break;
    default:
        break;
    }

    return true;
}

#endif //decodehelp_h
