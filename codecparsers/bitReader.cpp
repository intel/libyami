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

#include <assert.h>
#include "bitReader.h"

#define LOAD8BYTESDATA_BE(x) \
    (((uint64_t)((const uint8_t*)(x))[0] << 56) | \
     ((uint64_t)((const uint8_t*)(x))[1] << 48) | \
     ((uint64_t)((const uint8_t*)(x))[2] << 40) | \
     ((uint64_t)((const uint8_t*)(x))[3] << 32) | \
     ((uint64_t)((const uint8_t*)(x))[4] << 24) | \
     ((uint64_t)((const uint8_t*)(x))[5] << 16) | \
     ((uint64_t)((const uint8_t*)(x))[6] <<  8) | \
     ((uint64_t)((const uint8_t*)(x))[7]))

namespace YamiParser {

const uint32_t BitReader::CACHEBYTES = sizeof(unsigned long int);

BitReader::BitReader(const uint8_t* pdata, uint32_t size)
    : m_stream(pdata)
    , m_size(size)
    , m_cache(0)
    , m_loadBytes(0)
    , m_bitsInCache(0)
{
    if (size)
        assert(pdata);
}

void BitReader::loadDataToCache(uint32_t nbytes)
{
    unsigned long int tmp = 0;
    const uint8_t* pStart = m_stream + m_loadBytes;

    if (nbytes == 8)
        tmp = LOAD8BYTESDATA_BE(pStart);
    else {
        for (uint32_t i = 0; i < nbytes; i++) {
            tmp <<= 8;
            tmp |= pStart[i];
        }
    }

    m_cache = tmp;
    m_loadBytes += nbytes;
    m_bitsInCache = nbytes << 3;
}

inline void BitReader::reload()
{
    assert(m_size >= m_loadBytes);
    uint32_t remainingBytes = m_size - m_loadBytes;
    if (remainingBytes > 0)
        loadDataToCache(std::min(remainingBytes, CACHEBYTES));
}

inline uint32_t BitReader::extractBitsFromCache(uint32_t nbits)
{
    if (!nbits)
        return 0;
    uint32_t tmp = 0;
    tmp = m_cache << ((CACHEBYTES << 3) - m_bitsInCache) >> ((CACHEBYTES << 3) - nbits);
    m_bitsInCache -= nbits;
    return tmp;
}

bool BitReader::read(uint32_t& v, uint32_t nbits)
{
    assert(nbits <= (CACHEBYTES << 3));

    if (nbits <= m_bitsInCache) {
        v = extractBitsFromCache(nbits);
        return true;
    }
    /*not enough bits, need to save remaining bits
      in current cache and then reload more bits*/
    uint32_t toBeReadBits = nbits - m_bitsInCache;
    uint32_t tmp = extractBitsFromCache(m_bitsInCache);
    reload();
    if (toBeReadBits > m_bitsInCache)
        return false;
    v = tmp << toBeReadBits | extractBitsFromCache(toBeReadBits);
    return true;
}

uint32_t BitReader::read(uint32_t nbits)
{
    uint32_t res;
    if (read(res, nbits))
        return res;
    return 0;
}

bool BitReader::skip(uint32_t nbits)
{
    uint32_t tmp;
    uint32_t size = sizeof(tmp) << 3;
    while (nbits > size) {
        if (!readT(tmp))
            return false;
        nbits -= size;
    }
    if (!read(tmp, nbits))
        return false;
    return true;
}

uint32_t BitReader::peek(uint32_t nbits) const
{
    BitReader tmp(*this);

    return tmp.read(nbits);
}

} /*namespace YamiParser*/
