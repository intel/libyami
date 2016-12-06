/*
 * Copyright 2016 Intel Corporation
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

#include "bitWriter.h"
#include "common/log.h"
#include <assert.h>

namespace YamiParser {

const uint32_t CACHEBYTES = sizeof(unsigned long int);
const uint32_t CACHEBITS = sizeof(unsigned long int) * 8;

// clip to keep lowest n bits
#define CLIPBITS(value, n) ((value) & ((1 << (n)) - 1));

BitWriter::BitWriter(uint32_t size)
    : m_cache(0)
    , m_bitsInCache(0)
{
    m_bs.reserve(size);
}

uint8_t* BitWriter::getBitWriterData()
{
    uint8_t* buf;
    flushCache();

    buf = m_bs.size() ? static_cast<uint8_t*>(&m_bs[0]) : NULL;
    return buf;
}

void BitWriter::flushCache()
{
    uint8_t tmp, i, bytesInCache;

    // make sure m_bitsInCache is byte aligned, else trailing bits should be
    // padded
    if (m_bitsInCache % 8)
        writeToBytesAligned();

    assert(!(m_bitsInCache % 8));

    bytesInCache = m_bitsInCache / 8;

    for (i = 0; i < bytesInCache; i++) {
        tmp = static_cast<uint8_t>(m_cache >> (m_bitsInCache - (i + 1) * 8));
        m_bs.push_back(tmp);
    }

    m_cache = 0;
    m_bitsInCache = 0;
}

bool BitWriter::writeBits(uint32_t value, uint32_t numBits)
{
    if (value >= (uint64_t)1 << numBits) {
        WARNING("Write Bits: value overflow");
    }

    ASSERT((m_bitsInCache <= CACHEBITS) && (numBits <= CACHEBITS));

    uint32_t bitLeft = CACHEBITS - m_bitsInCache;

    if (bitLeft > numBits) {
        m_cache = (m_cache << numBits | value);
        m_bitsInCache += numBits;
    } else {
        m_cache = ((m_cache << bitLeft) | (value >> (numBits - bitLeft)));
        m_bitsInCache = CACHEBITS;
        flushCache();
        m_cache = value;
        m_bitsInCache = numBits - bitLeft;
    }

    return true;
}

bool BitWriter::writeBytes(uint8_t* data, uint32_t numBytes)
{
    if (!data || !numBytes)
        return false;

    if ((m_bitsInCache % 8) == 0) {
        flushCache();
        m_bs.insert(m_bs.end(), data, data + numBytes);
    } else {
        for (uint32_t i = 0; i < numBytes; i++)
            writeBits(data[i], 8);
    }

    return true;
}

void BitWriter::writeToBytesAligned(bool bit)
{
    uint8_t padBits = m_bitsInCache & 0x7;
    uint32_t value;
    if (padBits) {
        if (bit)
            value = (1 << (8 - padBits)) - 1;
        else
            value = 0;
        writeBits(value, 8 - padBits);
    }
}

} /*namespace YamiParser*/
