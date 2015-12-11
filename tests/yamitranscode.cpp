/*
 *  yamitranscode.cpp - yamitranscode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Guangxin.Xu<Guangxin.Xu@intel.com>
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

#include "yamitranscodehelp.h"
#include "vppinputdecode.h"
#include "vppinputoutput.h"
#include "vppoutputencode.h"
#include "encodeinput.h"
#include "tests/vppinputasync.h"
#include "common/log.h"
#include "common/utils.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"
#include "VideoPostProcessHost.h"
#include <stdio.h>
#include <stdlib.h>

using namespace YamiMediaCodec;

SharedPtr<VppInput> createInput(YamiEncodeParam& para, const SharedPtr<VADisplay>& display)
{
    SharedPtr<VppInput> input(VppInput::create(para.inputFileName.c_str()));
    if (!input) {
        ERROR("creat input failed");
        return input;
    }
    SharedPtr<VppInputFile> inputFile = std::tr1::dynamic_pointer_cast<VppInputFile>(input);
    if (inputFile) {
        SharedPtr<FrameReader> reader(new VaapiFrameReader(display));
        SharedPtr<FrameAllocator> alloctor(new PooledFrameAllocator(display, 5));
        if(!inputFile->config(alloctor, reader)) {
            ERROR("config input failed");
            input.reset();
        }
        return input;
    }
    SharedPtr<VppInputDecode> inputDecode = std::tr1::dynamic_pointer_cast<VppInputDecode>(input);
    if (inputDecode) {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)*display;
        if(!inputDecode->config(nativeDisplay)) {
            ERROR("config input decode failed");
            input.reset();
        }
        return input;
    }
    return VppInputAsync::create(input, 3);
}

SharedPtr<VppOutput> createOutput(YamiEncodeParam& para, const SharedPtr<VADisplay>& display)
{

    SharedPtr<VppOutput> output = VppOutput::create(para.outputFileName.c_str(), para.fourcc, para.oWidth, para.oHeight);
    SharedPtr<VppOutputFile> outputFile = std::tr1::dynamic_pointer_cast<VppOutputFile>(output);
    if (outputFile) {
        SharedPtr<FrameWriter> writer(new VaapiFrameWriter(display));
        if (!outputFile->config(writer)) {
            ERROR("config writer failed");
            output.reset();
        }
        return output;
    }
    SharedPtr<VppOutputEncode> outputEncode = std::tr1::dynamic_pointer_cast<VppOutputEncode>(output);
    if (outputEncode) {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)*display;
        if (!outputEncode->config(nativeDisplay)) {
            ERROR("config ouput encode failed");
            output.reset();
        }
        return output;
    }
    return output;
}

SharedPtr<FrameAllocator> createAllocator(const SharedPtr<VppOutput>& output, const SharedPtr<VADisplay>& display)
{
    uint32_t fourcc;
    int width, height;
    SharedPtr<FrameAllocator> allocator(new PooledFrameAllocator(display, 5));
    if (!output->getFormat(fourcc, width, height)
        || !allocator->setFormat(fourcc, width,height)) {
        allocator.reset();
        ERROR("get Format failed");
    }
    return allocator;
}

class TranscodeTest
{
public:
    bool init(int argc, char* argv[])
    {
        if (!processCmdLine(argc, argv, m_cmdParam))
            return false;

        m_display = createVADisplay();
        if (!m_display) {
            printf("create display failed");
            return false;
        }
        if (!createVpp()) {
            ERROR("create vpp failed");
            return false;
        }
        m_input = createInput(m_cmdParam, m_display);
        m_output =  createOutput(m_cmdParam, m_display);
        if (!m_input || !m_output) {
            ERROR("create input or output failed");
            return false;
        }
        m_allocator = createAllocator(m_output, m_display);
        return m_allocator;
    }

    bool run()
    {

        SharedPtr<VideoFrame> src;
        FpsCalc fps;
        uint32_t count = 0;
        while (m_input->read(src)) {
            SharedPtr<VideoFrame> dest = m_allocator->alloc();
            if (!dest) {
                ERROR("failed to get output frame");
                break;
            }
//disable scale for performance measure
//#define DISABLE_SCALE 1
#ifdef DISABLE_SCALE
            YamiStatus status = m_vpp->process(src, dest);
            if (status != YAMI_SUCCESS) {
                ERROR("failed to scale yami return %d", status);
                break;
            }
#else
            dest = src;
#endif
            if(!m_output->output(dest))
                break;
            count++;
            if(count >= m_cmdParam.frameCount)
                break;
            fps.addFrame();
        }
        fps.log();
        return true;
    }
private:
    bool createVpp()
    {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)*m_display;
        m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
        return m_vpp->setNativeDisplay(nativeDisplay) == YAMI_SUCCESS;
    }
    SharedPtr<VADisplay> m_display;
    SharedPtr<VppInput> m_input;
    SharedPtr<VppOutput> m_output;
    SharedPtr<FrameAllocator> m_allocator;
    SharedPtr<IVideoPostProcess> m_vpp;
    YamiEncodeParam m_cmdParam;
};

int main(int argc, char** argv)
{

    TranscodeTest trans;
    if (!trans.init(argc, argv)) {
        ERROR("init transcode with command line parameters failed");
        return -1;
    }
    if (!trans.run()){
        ERROR("run transcode failed");
        return -1;
    }
    printf("transcode done\n");
    return  0;

}
