/*
 *  decode.cpp - h264 decode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vppinputdecode.h"
#include "vppinputoutput.h"
#include "decodeoutput.h"
#include "common/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

struct DecodeParameter
{
    char*    outputFile;
    char*    inputFile;
    int      width;
    int      height;
    short    renderMode;
    short    waitBeforeQuit;
    bool     isConverted;      //convert NV12 to I420
    uint32_t renderFrames;
    uint32_t renderFourcc;
};

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

bool processArguments(int argc, char** argv, DecodeParameter& parameters)
{
    bool isSetFourcc = false;
    parameters.renderFrames   = UINT_MAX;
    parameters.waitBeforeQuit = 1;
    parameters.renderMode     = 1;
    parameters.inputFile      = NULL;
    parameters.outputFile     = NULL;

    char opt;
    while ((opt = getopt(argc, argv, "h:m:n:i:f:o:w:?")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
                parameters.inputFile = optarg;
            break;
        case 'w':
            if (optarg)
                parameters.waitBeforeQuit = atoi(optarg);
            break;
        case 'm':
            if (optarg)
                parameters.renderMode = atoi(optarg);
            break;
        case 'n':
            if (optarg)
                parameters.renderFrames = atoi(optarg);
            break;
        case 'f':
            if (optarg) {
                if (strlen(optarg) == 4) {
                    parameters.renderFourcc = VA_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
                    isSetFourcc = true;
                } else {
                    fprintf(stderr, "invalid fourcc: %s\n", optarg);
                    return false;
                }
            }
            break;
        case 'o':
            if (optarg)
                parameters.outputFile = strdup(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    if (!parameters.inputFile) {
        fprintf(stderr, "no input media file specified.\n");
        return false;
    }
    if (!parameters.outputFile)
        parameters.outputFile = strdup ("./");
    if (!isSetFourcc)
        parameters.renderFourcc = guessFourcc(parameters.outputFile);
    int width, height;
    if (guessResolution(parameters.inputFile, width, height)) {
        parameters.width =  width;
        parameters.height =  height;
    }
    return true;
}


SharedPtr<VppInput> createInput(DecodeParameter& para, SharedPtr<NativeDisplay> display)
{
    SharedPtr<VppInput> input(VppInput::create(para.inputFile, para.renderFourcc, para.width, para.height));
    if (!input) {
        fprintf(stderr, "VppInput create failed.\n");
        return input;
    }
    SharedPtr<VppInputDecode> inputDecode = std::tr1::dynamic_pointer_cast<VppInputDecode>(input);
    if (!inputDecode || !inputDecode->config(*display)) {
        fprintf(stderr, "VppInputDecode config failed.\n");
        input.reset();
        return input;
    }
    return input;
}


class DecodeTest
{
public:
    bool init(int argc, char **argv)
    {
        if ( !processArguments(argc, argv, m_parameter)) {
            fprintf(stderr, "process arguments failed.\n");
            return false;
        }
        m_output = DecodeOutput::create(m_parameter.renderMode, m_parameter.renderFourcc, m_parameter.inputFile, m_parameter.outputFile);
        if (!m_output) {
            fprintf(stderr, "DecodeOutput::create failed.\n");
            return false;
        }
        m_nativeDisplay = m_output->nativeDisplay();
        m_vppInput      = createInput(m_parameter, m_nativeDisplay);

        if (!m_nativeDisplay || !m_vppInput) {
            fprintf(stderr, "DecodeTest init failed.\n");
            return false;
        }
        return true;
    }
    bool run()
    {
        FpsCalc  fps;
        SharedPtr<VideoFrame> src;
        int count = 0;
        while (m_vppInput->read(src)) {
            if(!m_output->output(src))
                break;
            count++;
            fps.addFrame();
            if (count == m_parameter.renderFrames)
                break;
        }
        fps.log();
        return true;
    }

private:
    SharedPtr<DecodeOutput>   m_output;
    SharedPtr<NativeDisplay>  m_nativeDisplay;
    SharedPtr<VppInput>       m_vppInput;
    DecodeParameter           m_parameter;
};


int main(int argc, char *argv[])
{
    DecodeTest decode;
    if (!decode.init(argc, argv))
        return 1;
    decode.run();
	return 0;
}
