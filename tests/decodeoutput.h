/*
 *  decodeoutput.h - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *            Xu Guangxin<guangxin.xu@intel.com>
 *            Liu Yangbin<yangbinx.liu@intel.com>
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

#ifndef decodeoutput_h
#define decodeoutput_h

#include "vppinputoutput.h"
#include "VideoPostProcessHost.h"
#include "VideoCommonDefs.h"
#include "common/common_def.h"

#include <string>
#include <cassert>

class DecodeOutput
{
public:
    static DecodeOutput* create(int renderMode, uint32_t fourcc, const char* inputFile, const char* outputFile);
    virtual bool output(const SharedPtr<VideoFrame>& frame) = 0;
    SharedPtr<NativeDisplay> nativeDisplay();
    virtual ~DecodeOutput() {}
protected:
    virtual bool setVideoSize(uint32_t with, uint32_t height);

    virtual bool init();

    uint32_t m_width;
    uint32_t m_height;
    SharedPtr<VADisplay> m_vaDisplay;
    SharedPtr<NativeDisplay> m_nativeDisplay;
};

#endif //decodeoutput_h
