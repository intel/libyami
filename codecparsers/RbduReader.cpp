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

#include "RbduReader.h"
#include <assert.h>

namespace YamiParser {

RbduReader::RbduReader(const uint8_t* pdata, uint32_t size)
    : EpbReader(pdata, size)
{
}

//table 255
bool RbduReader::isEmulationPreventionByte(const uint8_t* p) const
{

    if (*p != 0x03)
        return false;

    uint32_t pos = p - m_stream;

    if (pos < 2)
        return false;
    if (*(p - 1) != 0x00 || *(p - 2) != 0x00)
        return false;

    if ((pos + 1) >= m_size)
        return false;
    uint8_t next = *(p + 1);
    if (next > 0x03)
        return false;

    return true;
}

} /*namespace YamiParser*/
