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

#ifndef EpbReader_h
#define EpbReader_h

#include "bitReader.h"
#include "common/log.h"

namespace YamiParser {

//this reader will skip the emulation prevention byte(epb)
class EpbReader : public BitReader {
public:
    EpbReader(const uint8_t* data, uint32_t size);

    uint32_t getEpbCnt() { return m_epb; }

    uint64_t getPos() const;

protected:
    virtual bool isEmulationPreventionByte(const uint8_t* p) const = 0;

private:
    void loadDataToCache(uint32_t nbytes);

    uint32_t m_epb; /*the number of emulation prevention bytes*/
};

} /*namespace YamiParser*/

#endif
