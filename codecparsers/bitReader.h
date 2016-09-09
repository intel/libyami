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

    /* Read specified bits(<= 8*sizeof(uint32_t)) as a uint32_t to v */
    /* if not enough data, it will return false, eat all data and keep v untouched */
    bool read(uint32_t& v, uint32_t nbits);
    /* Read specified bits(<= 8*sizeof(uint32_t)) as a uint32_t return value */
    /* will return 0 if not enough data*/
    uint32_t read(uint32_t nbits);

    /*TODO: change to this to read, and change read to readBits */
    template <class T>
    inline bool readT(T& v, uint32_t nbits);

    template <class T>
    inline bool readT(T& v);

    inline bool readT(bool& v);

    /*read the next nbits bits from the bitstream but not advance the bitstream pointer*/
    uint32_t peek(uint32_t nbits) const;

    /* this version will check read beyond boundary */
    template <class T>
    bool peek(T& v, uint32_t nbits) const;

    bool skip(uint32_t nbits);

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

    const uint8_t* m_stream; /*a pointer to source data*/
    uint32_t m_size; /*the size of source data in bytes*/
    unsigned long int m_cache; /*the buffer which load less than or equal to 8 bytes*/
    uint32_t m_loadBytes; /*the total bytes of data read from source data*/
    uint32_t m_bitsInCache; /*the remaining bits in cache*/
private:
    inline uint32_t extractBitsFromCache(uint32_t nbits);
    inline void reload();
};

template <class T>
bool BitReader::readT(T& v, uint32_t nbits)
{
    uint32_t tmp;
    if (!read(tmp, nbits))
        return false;
    v = tmp;
    return true;
}

template <class T>
bool BitReader::readT(T& v)
{
    return readT(v, sizeof(v) << 3);
}

bool BitReader::readT(bool& v)
{
    return readT(v, 1);
}

template <class T>
bool BitReader::peek(T& v, uint32_t nbits) const
{
    BitReader tmp(*this);
    return tmp.readT(v, nbits);
}

} /*namespace YamiParser*/

#endif
