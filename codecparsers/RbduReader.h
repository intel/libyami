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

#ifndef RbduReader_h
#define RbduReader_h

#include "EpbReader.h"
#include "common/log.h"

namespace YamiParser {

//RBDU reader for vc1, it will skip the emulation prevention bytes.
class RbduReader : public EpbReader {
public:
    RbduReader(const uint8_t* data, uint32_t size);

private:
    friend class RbduReaderTest;
    bool isEmulationPreventionByte(const uint8_t* p) const;
};

} /*namespace YamiParser*/

#endif
