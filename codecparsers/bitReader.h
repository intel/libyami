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

#ifndef bitReader_h
#define bitReader_h

#include <stdint.h>
#include <algorithm> /*std::min*/

namespace YamiParser {

class BitReader {
public:
    static const uint32_t CACHEBYTES;
    BitReader(const uint8_t* data, uint32_t size);
    virtual ~BitReader() {}

    /* Read specified bits(<= 8*sizeof(uint32_t)) as a uint32_t return value */
    uint32_t read(uint32_t nbits);

    /*read the next nbits bits from the bitstream but not advance the bitstream pointer*/
    uint32_t peek(uint32_t nbits) const;

    /* You are allowed to skip less than BitReader::CACHEBYTES bytes
     * when call this function at a time. And if you need to skip more than
     * BitReader::CACHEBYTES bytes, you can call skip() repeatedly. */
    void skip(uint32_t nbits);

    /* Get the total bits that had been read from bitstream, and the return
     * value also is the position of the next bit to be read. */
    uint64_t getPos() const
    {
        return (static_cast<uint64_t>(m_loadBytes) << 3) - m_bitsInCache;
    }

    uint64_t getRemainingBitsCount() const
    {
        return m_bitsInCache + (static_cast<uint64_t>(m_size - m_loadBytes) << 3);
    }

    bool end() const
    {
        return (getPos() >= (static_cast<uint64_t>(m_size) << 3));
    }

protected:
    virtual void loadDataToCache(uint32_t nbytes);
    inline uint32_t extractBitsFromCache(uint32_t nbits);

    const uint8_t* m_stream; /*a pointer to source data*/
    uint32_t m_size; /*the size of source data in bytes*/
    unsigned long int m_cache; /*the buffer which load less than or equal to 8 bytes*/
    uint32_t m_loadBytes; /*the total bytes of data read from source data*/
    uint32_t m_bitsInCache; /*the remaining bits in cache*/
};

} /*namespace YamiParser*/

#endif
