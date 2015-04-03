/*
 *  encodeinput.cpp - encode test input
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Guangxin Xu<Guangxin.Xu@intel.com>
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

using namespace YamiMediaCodec;

SharedPtr<VppInput> VppInput::create(const char* inputFileName, uint32_t fourcc, int width, int height)
{
    SharedPtr<VppInput> input;
    if (!inputFileName)
        return input;

    input.reset(new VppInputFile);
    if (!(input->init(inputFileName, fourcc, width, height))) {
        input.reset();
    }
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
    if (!guessFormat(inputFileName, fourcc, width, height)) {
        return false;
    }
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

SharedPtr<VppOutput> VppOutput::create(const char* outputFileName)
{
    SharedPtr<VppOutput> output;
    if (!outputFileName) {
        ERROR("invalid output file name");
        return output;
    }
    output.reset(new VppOutputEncode);
    if (output->init(outputFileName))
        return output;
    output.reset(new VppOutputFile);
    if (output->init(outputFileName))
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
}


bool VppOutputFile::init(const char* outputFileName)
{
    if (!outputFileName) {
        ERROR("output file name is null");
        return false;
    }
    if (!guessFormat(outputFileName, m_fourcc, m_width, m_height)) {
        ERROR("guess format from %s failed", outputFileName);
        return false;
    }
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
