/*
 *  decode.cpp - decode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Lin Hai<hai1.lin@intel.com>
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
#include "decodeoutput.h"
#include "common/utils.h"
#include "decodehelp.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

SharedPtr<VppInput> createInput(DecodeParameter& para, SharedPtr<NativeDisplay>& display)
{
    SharedPtr<VppInput> input(VppInput::create(para.inputFile, para.renderFourcc, para.width, para.height));
    if (!input) {
        fprintf(stderr, "VppInput create failed.\n");
        return input;
    }
    SharedPtr<VppInputDecode> inputDecode = std::tr1::dynamic_pointer_cast<VppInputDecode>(input);
    if (!inputDecode) {
        input.reset();
        fprintf(stderr, "createInput failed, cannot convert VppInput to VppInputDecode\n");
        return input;
    }
    if (!inputDecode->config(*display)) {
        input.reset();
        fprintf(stderr, "VppInputDecode config failed.\n");
        return input;
    }
    return input;
}


class DecodeTest
{
public:
    bool init(int argc, char **argv)
    {
        if (!processCmdLine(argc, argv, &m_params)) {
            fprintf(stderr, "process arguments failed.\n");
            return false;
        }
        m_output.reset(DecodeOutput::create(m_params.renderMode, m_params.renderFourcc, m_params.inputFile, m_params.outputFile.c_str()));
        if (!m_output) {
            fprintf(stderr, "DecodeOutput::create failed.\n");
            return false;
        }
        m_nativeDisplay = m_output->nativeDisplay();
        m_vppInput = createInput(m_params, m_nativeDisplay);

        if (!m_nativeDisplay || !m_vppInput) {
            fprintf(stderr, "DecodeTest init failed.\n");
            return false;
        }
        return true;
    }
    bool run()
    {
        FpsCalc fps;
        SharedPtr<VideoFrame> src;
        uint32_t count = 0;
        while (m_vppInput->read(src)) {
            if(!m_output->output(src))
                break;
            count++;
            fps.addFrame();
            if (count == m_params.renderFrames)
                break;
        }
        fps.log();
        return true;
    }

private:
    SharedPtr<DecodeOutput>   m_output;
    SharedPtr<NativeDisplay>  m_nativeDisplay;
    SharedPtr<VppInput>       m_vppInput;
    DecodeParameter           m_params;
};


int main(int argc, char *argv[])
{
    DecodeTest decode;
    if (!decode.init(argc, argv))
        return 1;
    decode.run();
    return 0;
}
