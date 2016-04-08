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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vppinputoutput.h"
#include "vppoutputencode.h"
#include "encodeinput.h"
#include "common/log.h"
#include "common/utils.h"
#include "VideoEncoderInterface.h"
#include "VideoEncoderHost.h"
#include "VideoPostProcessHost.h"
#include <stdio.h>
#include <stdlib.h>

using namespace YamiMediaCodec;

SharedPtr<VppInput> createInput(const char* filename, const SharedPtr<VADisplay>& display)
{
    SharedPtr<VppInput> input(VppInput::create(filename));
    if (!input) {
        ERROR("creat input failed");
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

SharedPtr<VppOutput> createOutput(const char* filename, const SharedPtr<VADisplay>& display)
{

    SharedPtr<VppOutput> output = VppOutput::create(filename);
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

class VppTest
{
public:
    bool init(const char* input, const char* output)
    {
        m_display = createVADisplay();
        if (!m_display) {
            printf("create display failed");
            return false;
        }
        if (!createVpp()) {
            ERROR("create vpp failed");
            return false;
        }
        m_input = createInput(input, m_display);
        m_output =  createOutput(output, m_display);
        if (!m_input || !m_output) {
            printf("create input or output failed");
            return false;
        }
        m_allocator = createAllocator(m_output, m_display);
        return m_allocator;
    }

    bool run()
    {

        SharedPtr<VideoFrame> src, dest;
        YamiStatus  status;
        int count = 0;
        while (m_input->read(src)) {
            dest = m_allocator->alloc();
            status = m_vpp->process(src, dest);
            if (status != YAMI_SUCCESS) {
                ERROR("vpp process failed, status = %d", status);
                return true;
            }
            m_output->output(dest);
            count++;
        }
        //flush output
        dest.reset();
        m_output->output(dest);

        printf("%d frame processed\n", count);
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
};

void usage()
{
    printf("a tool to do video post process, support scaling and CSC\n");
    printf("we can guess size and color format from your file name\n");
    printf("current supported format are i420, yv12, nv12\n");
    printf("usage: yamivpp input_1920x1080.i420 output_320x240.yv12\n");

}

int main(int argc, char** argv)
{
    if (argc != 3) {
        usage();
        return -1;
    }
    VppTest vpp;
    if (!vpp.init(argv[1], argv[2])) {
        ERROR("init vpp with %s, %s, failed", argv[1], argv[2]);
        return -1;
    }
    if (!vpp.run()){
        ERROR("run vpp failed");
        return -1;
    }
    printf("vpp done\n");
    return  0;

}
