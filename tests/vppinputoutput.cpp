/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <deque>
#include <va/va.h>
#include "common/log.h"
#include "common/utils.h"
#include "common/lock.h"
#include "vppinputoutput.h"
#include "vppoutputencode.h"
#include "vppinputdecode.h"
#ifdef __ENABLE_CAPI__
#include "vppinputdecodecapi.h"
#endif
using namespace YamiMediaCodec;

#ifndef ANDROID
SharedPtr<VADisplay> createVADisplay()
{
    SharedPtr<VADisplay> display;
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        ERROR("can't open /dev/dri/renderD128, try to /dev/dri/card0");
        fd = open("/dev/dri/card0", O_RDWR);
    }
    if (fd < 0) {
        ERROR("can't open drm device");
        return display;
    }
    VADisplay vadisplay = vaGetDisplayDRM(fd);
    int majorVersion, minorVersion;
    VAStatus vaStatus = vaInitialize(vadisplay, &majorVersion, &minorVersion);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ERROR("va init failed, status =  %d", vaStatus);
        close(fd);
        return display;
    }
    display.reset(new VADisplay(vadisplay), VADisplayDeleter(fd));
    return display;
}
#endif

SharedPtr<VppInput> VppInput::create(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    SharedPtr<VppInput> input;
    if (!inputFileName)
        return input;

#ifdef __ENABLE_CAPI__
    input.reset(new VppInputDecodeCapi);
    if (input->init(inputFileName, fourcc, width, height))
        return input;
#else
    input.reset(new VppInputDecode);
    if(input->init(inputFileName, fourcc, width, height))
        return input;
#endif
    input.reset(new VppInputFile);
    if (input->init(inputFileName, fourcc, width, height))
        return input;

    ERROR("Wrong input file format %s?", inputFileName);
    input.reset();
    return input;
}

VppInputFile::VppInputFile()
    : m_fp(NULL)
    , m_readToEOS(false)
{
}

bool VppInputFile::config(const SharedPtr<FrameAllocator>& allocator, const SharedPtr<FrameReader>& reader)
{
    if (!allocator->setFormat(m_fourcc, m_width, m_height)) {
        ERROR("set format to %x, %dx%d failed", m_fourcc, m_width, m_height);
        return false;
    }
    m_reader = reader;
    m_allocator = allocator;
    return true;
}

static bool guessFormat(const char* filename, uint32_t& fourcc, int& width, int& height)
{
    if (!width || !height) {
        if (!guessResolution(filename, width, height)) {
            ERROR("failed to guess input width and height\n");
            return false;
        }
    }
    if (!fourcc)
        fourcc = guessFourcc(filename);

    if (!fourcc)
        fourcc = VA_FOURCC('I', '4', '2', '0');
    return true;
}

bool VppInputFile::init(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    if(!fourcc || !width || !height)
        if (!guessFormat(inputFileName, fourcc, width, height))
            return false;

    m_width = width;
    m_height = height;
    m_fourcc = fourcc;

    m_fp = fopen(inputFileName, "r");
    if (!m_fp) {
        fprintf(stderr, "fail to open input file: %s", inputFileName);
        return false;
    }
    return true;
}

bool VppInputFile::read(SharedPtr<VideoFrame>& frame)
{
    if (!m_allocator || !m_reader) {
        ERROR("config VppInputFile with allocator and reader, please!");
        return false;
    }
    if (m_readToEOS)
        return false;

    frame = m_allocator->alloc();
    if (!frame) {
        ERROR("allocate frame failed");
        return false;
    }

    if (!m_reader->read(m_fp, frame))
        m_readToEOS = true;
    return !m_readToEOS;
}

VppInputFile::~VppInputFile()
{
    if(m_fp)
        fclose(m_fp);
}

VppOutput::VppOutput()
    :m_fourcc(0), m_width(0), m_height(0)
{
}

SharedPtr<VppOutput> VppOutput::create(const char* outputFileName, uint32_t fourcc, int width, int height)
{
    SharedPtr<VppOutput> output;
    if (!outputFileName) {
        ERROR("invalid output file name");
        return output;
    }
    output.reset(new VppOutputEncode);
    if (output->init(outputFileName, fourcc, width, height))
        return output;
    output.reset(new VppOutputFile);
    if (output->init(outputFileName, fourcc, width, height))
        return output;
    ERROR("can't open %s, wroing extension?",outputFileName);
    output.reset();
    return output;
}


bool VppOutput::getFormat(uint32_t& fourcc, int& width, int& height)
{
    fourcc = m_fourcc;
    width = m_width;
    height = m_height;
    return true;
}


bool VppOutputFile::init(const char* outputFileName, uint32_t fourcc, int width, int height)
{
    if (!outputFileName) {
        ERROR("output file name is null");
        return false;
    }
    m_fourcc = fourcc;
    m_width = width;
    m_height = height;
    m_fp = fopen(outputFileName, "wb");
    if (!m_fp) {
        ERROR("fail to open input file: %s", outputFileName);
        return false;
    }
    return true;
}

bool VppOutputFile::config(const SharedPtr<FrameWriter>& writer)
{
    m_writer = writer;
    return true;
}

VppOutputFile::~VppOutputFile()
{
    if (m_fp)
        fclose(m_fp);
}

bool VppOutputFile::output(const SharedPtr<VideoFrame>& frame)
{
    return write(frame);
}

bool VppOutputFile::write(const SharedPtr<VideoFrame>& frame)
{
    if (!m_writer) {
        ERROR("config with writer please!");
        return false;
    }
    if (!frame)
        return true;
    return m_writer->write(m_fp, frame);
}

VppOutputFile::VppOutputFile()
    :m_fp(NULL)
{
}
