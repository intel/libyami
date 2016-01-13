/*
 *  encode.cpp - encode test
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

#include "vppinputoutput.h"
#include "vppoutputencode.h"
#include "encodehelp.h"
#include "common/utils.h"

#include <cstdio>

using namespace YamiMediaCodec;

SharedPtr<VppInput> createInput(const TranscodeParams& param, const SharedPtr<VADisplay>& display)
{
    SharedPtr<VppInput> input(VppInput::create(param.inputFileName.c_str()));
    if (!input) {
		fprintf(stderr, "creat input failed\n");
		return input;
    }
    SharedPtr<VppInputFile> inputFile = std::tr1::dynamic_pointer_cast<VppInputFile>(input);
    if (inputFile) {
		SharedPtr<FrameReader> reader(new VaapiFrameReader(display));
		SharedPtr<FrameAllocator> alloctor(new PooledFrameAllocator(display, 5));
		inputFile->config(alloctor, reader);
    }
    return inputFile;
}

SharedPtr<VppOutput> createOutput(const TranscodeParams& param, const SharedPtr<VADisplay>& display)
{
    SharedPtr<VppOutput> output = VppOutput::create(param.outputFileName.c_str());
    SharedPtr<VppOutputEncode> outputEncode = std::tr1::dynamic_pointer_cast<VppOutputEncode>(output);
    if (outputEncode) {
		NativeDisplay nativeDisplay;
		nativeDisplay.type = NATIVE_DISPLAY_VA;
		nativeDisplay.handle = (intptr_t)*display;
		if (!outputEncode->config(nativeDisplay, &param.m_encParams)) {
			fprintf(stderr, "config ouput encode failed\n");
			output.reset();
		}
		return output;
    }
    output.reset();
    return output;
}

class EncoderTest
{
public:
    bool init(int argc, char *argv[])
    {
		if (!processCmdLine(argc, argv, m_para))
			return false;
		m_display = createVADisplay();
		if (!m_display) {
			printf("create display failed\n");
			return false;
		}
		m_input = createInput(m_para, m_display);
		m_output =  createOutput(m_para, m_display);
		if (!m_input || !m_output) {
			printf("create input or output failed\n");
			return false;
		}
		return true;
    }

    bool run()
    {
		SharedPtr<VideoFrame> src;
		int count = 0;
		FpsCalc fps;
		while (m_input->read(src)) {
			if (!m_output->output(src)) {
				fprintf(stderr, "EncoderTest VppOutput output failed\n");
				return false;
			}
			fps.addFrame();
			if ( ++count == m_para.frameCount)
				break;
		}
		fps.log();
		return true;
	}
private:
    TranscodeParams    m_para;
    SharedPtr<VADisplay> m_display;
    SharedPtr<VppInput> m_input;
    SharedPtr<VppOutput> m_output;
};

int main(int argc, char** argv)
{
    EncoderTest encoder;
    if (!encoder.init(argc, argv)) {
		fprintf(stderr, "\nEncoder init failed\n");
        return -1;
    }
    if (!encoder.run()){
		fprintf(stderr, "run encoder failed");
        return -1;
    }
    printf("encoder done\n");
    return  0;
}
