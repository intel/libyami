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

#ifndef nalReader_h
#define nalReader_h

#include "common/log.h"
#include "bitReader.h"

namespace YamiParser {

class NalReader : public BitReader
{
public:
    NalReader(const uint8_t *data, uint32_t size);

    /*parse Exp-Golomb coding*/
    uint32_t readUe();
    int32_t readSe();

    bool moreRbspData() const;
    void rbspTrailingBits();
    uint32_t getEpbCnt() { return m_epb; }
private:
    void loadDataToCache(uint32_t nbytes);
    inline bool isEmulationBytes(const uint8_t *p) const;

    uint32_t m_epb; /*the number of emulation prevention bytes*/
};

} /*namespace YamiParser*/

#endif
