/*
 *  vppinputdecodecapi.h - vpp input from decoded file for capi
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Lin Hai<hai1.lin@intel.com>
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

#ifndef vppinputdecodecapi_h
#define vppinputdecodecapi_h

#include "VideoDecoderHost.h"
#include "decodeinput.h"
#include "vppinputoutput.h"
#include "capi/VideoDecoderCapi.h"

class VppInputDecodeCapi : public VppInput {
public:
    VppInputDecodeCapi()
        : m_eos(false)
        , m_error(false)
    {
    }
    ~VppInputDecodeCapi();

    bool init(const char* inputFileName, uint32_t fourcc = 0, int width = 0, int height = 0);
    bool read(SharedPtr<VideoFrame>& frame);
    bool config(NativeDisplay& nativeDisplay);

private:
    bool m_eos;
    bool m_error;
    DecodeHandler m_decoder;
    SharedPtr<DecodeInput> m_input;
};

#endif //vppinputdecodecapi_h
