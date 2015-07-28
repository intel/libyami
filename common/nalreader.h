/*
 *  NalReader.h - split buffer to nal
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */
#include <stdint.h>

namespace YamiMediaCodec{

class NalReader
{
public:
    NalReader(const uint8_t* buf, int32_t size, bool asWhole = false);
    bool read(const uint8_t*& buf, int32_t& size);
private:
    const uint8_t* searchStartCode();
    const uint8_t* m_begin;
    const uint8_t* m_next;
    const uint8_t* m_end;
    bool  m_asWhole;
};

} //namespace YamiMediaCodec