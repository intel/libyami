/*
 * Copyright 2018 Intel Corporation
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

#include "EpbReader.h"
#include <assert.h>

namespace YamiParser {

EpbReader::EpbReader(const uint8_t* pdata, uint32_t size)
    : BitReader(pdata, size)
    , m_epb(0)
{
}

void EpbReader::loadDataToCache(uint32_t nbytes)
{
    const uint8_t* pStart = m_stream + m_loadBytes;
    const uint8_t* p;
    const uint8_t* pEnd = m_stream + m_size;
    /*the numbers of emulation prevention three byte in current load block*/
    uint32_t epb = 0;

    unsigned long int tmp = 0;
    uint32_t size = 0;
    for (p = pStart; p < pEnd && size < nbytes; p++) {
        if (!isEmulationPreventionByte(p)) {
            tmp <<= 8;
            tmp |= *p;
            size++;
        } else {
            epb++;
        }
    }
    m_cache = tmp;
    m_loadBytes += p - pStart;
    m_bitsInCache = size << 3;
    m_epb += epb;
}

uint64_t EpbReader::getPos() const
{
    uint32_t count = m_bitsInCache / 8;
    const uint8_t* p = m_stream + m_loadBytes - 1;
    uint32_t epb = 0;
    while (count > 0) {
        if (isEmulationPreventionByte(p))
            epb++;
        else
            count--;
        p--;
    }
    //some epb loaded in cache, but we are not reach it yet
    //we need exclude it
    return (static_cast<uint64_t>(m_loadBytes - epb) << 3) - m_bitsInCache;
}

} /*namespace YamiParser*/
