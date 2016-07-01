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

#include <algorithm>
#include "nalreader.h"

namespace YamiMediaCodec{

NalReader::NalReader(const uint8_t* buf, int32_t size, uint32_t nalLengthSize, bool asWhole)
{
    m_begin = buf;
    m_next  = buf;
    m_end   = buf + size;
    m_asWhole = asWhole;

    m_nalLengthSize = nalLengthSize;
    m_size = 0;

    searchNalStart();
}

bool NalReader::read(const uint8_t*& nal, int32_t& nalSize)
{
    if (m_next == m_end)
        return false;

    nal = m_next;
    const uint8_t* nalEnd;
    if (m_asWhole) {
        nalEnd = m_end;
    } else {
        nalEnd = searchNalStart();
    }
    nalSize = nalEnd - nal;
    return true;
}

static const uint8_t START_CODE[] = { 0, 0, 1 };
static const int START_CODE_SIZE = 3;
static const uint8_t* START_CODE_END = START_CODE + START_CODE_SIZE;

const uint8_t* NalReader::searchStartCode()
{
    m_begin = std::search(m_next, m_end, START_CODE, START_CODE_END);

    if (m_begin != m_end) {
        m_next = m_begin + START_CODE_SIZE;
    } else {
        m_next = m_end;
    }
    return m_begin;
}

const uint8_t* NalReader::searchNalStart()
{
    if (!m_nalLengthSize)
        return searchStartCode();

    if (m_end > m_begin + m_size + m_nalLengthSize) {
        m_begin += m_size;
    } else {
        m_begin = m_next = m_end;
        return m_begin;
    }

    uint32_t i, size;
    m_next = m_begin + m_nalLengthSize;

    for (i = 0, size = 0; i < m_nalLengthSize; i++)
        size = (size << 8) | m_begin[i];

    m_size = m_nalLengthSize + size;

    return m_begin;
}
} //namespace YamiMediaCodec
