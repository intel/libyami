/*
 *  oclcontext.h - abstract for opencl.
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: XuGuangxin<Guangxin.Xu@intel.com>
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
#ifndef oclcontext_h
#define oclcontext_h


#include "interface/VideoCommonDefs.h"
#include "common/lock.h"
#include <CL/opencl.h>
#include <CL/cl_intel.h>

namespace YamiMediaCodec{

class OclDevice;

class OclContext
{
public:
    static SharedPtr<OclContext> create();
    bool createKernel(const char* name, cl_kernel& kernel);
    YamiStatus createImageFromFdIntel(const cl_import_image_info_intel* info, cl_mem* mem);
    YamiStatus createBufferFromFdIntel(const cl_import_buffer_info_intel* info, cl_mem* mem);
    ~OclContext();

    cl_context m_context;
    cl_command_queue m_queue;
private:
    OclContext();
    bool init();
    SharedPtr<OclDevice> m_device;
    DISALLOW_COPY_AND_ASSIGN(OclContext)
};

bool checkOclStatus(cl_int status, const char* err);

}

#endif //oclcontext_h
