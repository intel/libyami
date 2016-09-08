/*
 * Copyright (C) 2015 Intel Corporation. All rights reserved.
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
#ifndef oclcontext_h
#define oclcontext_h


#include "VideoCommonDefs.h"
#include "common/lock.h"
#include <CL/opencl.h>
#include <CL/cl_intel.h>
#include <map>
#include <string>

namespace YamiMediaCodec{

class OclDevice;

typedef std::map<std::string, cl_kernel> OclKernelMap;

class OclContext
{
public:
    static SharedPtr<OclContext> create();
    bool createKernel(const char* name, OclKernelMap& kernelMap);
    bool releaseKernel(OclKernelMap& kernelMap);
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
