/*
 *  NalReader.cpp - split buffer to nal
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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
#include <algorithm>
#include "nalreader.h"

namespace YamiMediaCodec{

NalReader::NalReader(const uint8_t* buf, int32_t size, bool asWhole)
{
    m_begin = buf;
    m_next  = buf;
    m_end   = buf + size;
    m_asWhole = asWhole;
    if (!asWhole) {
        m_begin = searchStartCode();
    }
}

bool NalReader::read(const uint8_t*& nal, int32_t& size)
{
    if (m_next == m_end)
        return false;
    const uint8_t* nalEnd;
    if (m_asWhole) {
        nalEnd = m_end;
    } else {
        nalEnd = searchStartCode();
    }
    nal = m_begin;
    size = nalEnd - m_begin;
    m_begin = nalEnd;
    return true;
}

const uint8_t START_CODE[] = {0, 0, 1};
const int START_CODE_SIZE = 3;
const uint8_t* START_CODE_END = START_CODE + START_CODE_SIZE;

const uint8_t* NalReader::searchStartCode()
{
    const uint8_t* start = std::search(m_next, m_end, START_CODE,  START_CODE_END);
    if (start != m_end) {
        m_next = start + START_CODE_SIZE;
    } else {
        m_next = m_end;
    }
    return start;
}

}//namespace YamiMediaCodec