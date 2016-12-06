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

#ifndef bitWriter_h
#define bitWriter_h

#include <stdint.h>
#include <vector>

namespace YamiParser {

#define BIT_WRITER_DEFAULT_BUFFER_SIZE 0x1000

class BitWriter {
public:
    /* size is used to set bit stream buffer capacity,
        a default buffer size may be set to avoid memory reallocated frequently
       */
    BitWriter(uint32_t size = BIT_WRITER_DEFAULT_BUFFER_SIZE);

    /* Write a value with numBits into bitstream */
    bool writeBits(uint32_t value, uint32_t numBits);

    /* Write an array with numBytes into bitstream */
    bool writeBytes(uint8_t* data, uint32_t numBytes);

    /* Pad some zeros to make sure bitsteam byte aligned */
    void writeToBytesAligned(bool bit = false);

    /* get encoded bitstream buffer */
    uint8_t* getBitWriterData();

    /* get encoded bits count
     * it is recommended to call getCodedBitsCount prior to getBitWriterData,
     * because getBitWriterData may pad some bits to make sure byte aligned,
     * then the padded bits will be caculated as coded bits.
     * */
    uint64_t getCodedBitsCount() const
    {
        return static_cast<uint64_t>(m_bs.size() * 8) + m_bitsInCache;
    }

protected:
    void flushCache();

    std::vector<uint8_t> m_bs; /* encoded bitstream buffer */

    long int m_cache; /* a 32 or 64 bits cache buffer */
    uint32_t m_bitsInCache; /* used bits in cache*/
};

} /*namespace YamiParser*/

#endif
