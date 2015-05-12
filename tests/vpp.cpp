/*
 *  vpp.cpp - vpp test
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
#include <fcntl.h>
#include <unistd.h>

using namespace YamiMediaCodec;

struct VADisplayDeleter
{
    VADisplayDeleter(int fd):m_fd(fd) {}
    void operator()(VADisplay* display)
    {
        vaTerminate(*display);
        delete display;
        close(m_fd);
    }
private:
    int m_fd;
};

SharedPtr<VADisplay> createVADisplay()
{
    SharedPtr<VADisplay> display;
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        ERROR("open renderD128 failed");
        return display;
    }
    VADisplay vadisplay = vaGetDisplayDRM(fd);
    int majorVersion, minorVersion;
    VAStatus vaStatus = vaInitialize(vadisplay, &majorVersion, &minorVersion);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ERROR("va init failed, status =  %d", vaStatus);
        return display;
    }
    display.reset(new VADisplay(vadisplay), VADisplayDeleter(fd));
    return display;
}

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
        int count;
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

int main(int argc, char** argv)
{
    if (argc != 3) {
        printf("usage: yamivpp input_1920x1080.i420 output_320x240.yv12\n");
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
