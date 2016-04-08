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