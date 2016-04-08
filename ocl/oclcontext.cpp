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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/log.h"
#include "oclcontext.h"

#include <stdio.h>
#include <iterator>
#include <vector>

using namespace std;

namespace YamiMediaCodec{

///internal class hold device id and make sure *.cl compile in serial
class OclDevice {

public:
    static SharedPtr<OclDevice> getInstance();
    bool createKernel(cl_context context, const char* name, OclKernelMap& kernelMap);
    bool releaseKernel(OclKernelMap& kernelMap);
    YamiStatus createImageFromFdIntel(cl_context context, const cl_import_image_info_intel* info, cl_mem* mem);
    YamiStatus createBufferFromFdIntel(cl_context context, const cl_import_buffer_info_intel* info, cl_mem* mem);

private:
    typedef cl_mem (*OclCreateImageFromFdIntel)(cl_context, const cl_import_image_info_intel*, cl_int*);
    typedef cl_mem (*OclCreateBufferFromFdIntel)(cl_context, const cl_import_buffer_info_intel*, cl_int*);

    OclDevice();
    bool init();
    bool loadKernel_l(cl_context context, const char* name, OclKernelMap& kernelMap);
    bool loadFile(const char* path, vector<char>& dest);
    void* getExtensionFunctionAddress(const char* name);

    static WeakPtr<OclDevice> m_instance;
    static Lock m_lock;

    //all operations need procted by m_lock
    cl_platform_id m_platform;
    cl_device_id m_device;
    OclCreateImageFromFdIntel m_oclCreateImageFromFdIntel;
    OclCreateBufferFromFdIntel m_oclCreateBufferFromFdIntel;
    friend OclContext;

    DISALLOW_COPY_AND_ASSIGN(OclDevice)

};

Lock OclDevice::m_lock;
WeakPtr<OclDevice> OclDevice::m_instance;

SharedPtr<OclContext> OclContext::create()
{
    SharedPtr<OclContext> context(new OclContext);
    if (!context->init())
        context.reset();
    return context;
}

OclContext::OclContext()
    :m_context(0), m_queue(0)
{
}

OclContext::~OclContext()
{
    clReleaseCommandQueue(m_queue);
    clReleaseContext(m_context);
}

bool OclContext::init()
{
    SharedPtr<OclDevice> device = OclDevice::getInstance();
    if (!device)
        return false;
    m_device= device;
    cl_int status;
    AutoLock lock(device->m_lock);
    m_context = clCreateContext(NULL, 1, &m_device->m_device, NULL,NULL,&status);
    if (!checkOclStatus(status, "clCreateContext"))
        return false;

    m_queue = clCreateCommandQueue(m_context, m_device->m_device, 0, &status);
    if (!checkOclStatus(status, "clCreateContext"))
        return false;

    return true;
}

bool OclContext::createKernel(const char* name, OclKernelMap& kernelMap)
{
    return m_device->createKernel(m_context, name, kernelMap);
}

bool OclContext::releaseKernel(OclKernelMap& kernelMap)
{
    return m_device->releaseKernel(kernelMap);
}

YamiStatus
OclContext::createImageFromFdIntel(const cl_import_image_info_intel* info, cl_mem* mem)
{
    return m_device->createImageFromFdIntel(m_context, info, mem);
}

YamiStatus
OclContext::createBufferFromFdIntel(const cl_import_buffer_info_intel* info, cl_mem* mem)
{
    return m_device->createBufferFromFdIntel(m_context, info, mem);
}

OclDevice::OclDevice()
    : m_platform(0)
    , m_device(0)
    , m_oclCreateImageFromFdIntel(0)
    , m_oclCreateBufferFromFdIntel(0)
{
}

SharedPtr<OclDevice> OclDevice::getInstance()
{
    AutoLock lock(m_lock);
    SharedPtr<OclDevice> device = m_instance.lock();
    if (device)
        return device;
    device.reset(new OclDevice);
    if (!device->init()) {
        device.reset();
    }
    return device;
}

bool OclDevice::init()
{
    cl_int status;
    status = clGetPlatformIDs(1, &m_platform, NULL);
    if (!checkOclStatus(status, "clGetPlatformIDs"))
        return false;

    status = clGetDeviceIDs(m_platform, CL_DEVICE_TYPE_GPU, 1, &m_device, NULL);
    if (!checkOclStatus(status, "clGetDeviceIDs"))
        return false;

    m_oclCreateImageFromFdIntel = (OclCreateImageFromFdIntel)
        getExtensionFunctionAddress("clCreateImageFromFdINTEL");
    m_oclCreateBufferFromFdIntel = (OclCreateBufferFromFdIntel)
        getExtensionFunctionAddress("clCreateBufferFromFdINTEL");
    if (!m_oclCreateImageFromFdIntel || !m_oclCreateBufferFromFdIntel) {
        ERROR("failed to get extension function");
        return false;
    }
    return true;
}

void* OclDevice::getExtensionFunctionAddress(const char* name)
{
#ifdef CL_VERSION_1_2
    return clGetExtensionFunctionAddressForPlatform(m_platform, name);
#else
    return clGetExtensionFunctionAddress(name);
#endif
}

bool OclDevice::loadFile(const char* path, vector<char>& dest)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return false;
    fseek(fp,0L,SEEK_END);
    size_t len = ftell(fp);
    if (len > 0) {
        dest.resize(len + 1);
        fseek(fp,0L,SEEK_SET);
        if (len == fread(&dest[0], 1, len, fp)) {
            dest.back() = '\0';
        } else {
            dest.clear();
        }
    }
    fclose(fp);
    return !dest.empty();
}

bool OclDevice::loadKernel_l(cl_context context, const char* name, OclKernelMap& kernelMap)
{
    bool ret = true;
    string path = KERNEL_DIR;
    path += name;
    path += ".cl";

    vector<char> text;
    if (!loadFile(path.c_str(), text)) {
        ERROR("Can't open %s", path.c_str());
        return false;
    }
    cl_int status;
    const char* source = &text[0];
    cl_program prog = clCreateProgramWithSource(context, 1, &source, NULL, &status);
    if (!checkOclStatus(status, "clCreateProgramWithSource"))
        return false;
    status = clBuildProgram(prog, 1, &m_device, NULL, NULL, NULL);
    if (!(ret = checkOclStatus(status, "clBuildProgram"))) {
        char log[1024];
        status = clGetProgramBuildInfo(prog, m_device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        if (checkOclStatus(status, "clCreateProgramWithSource")) {
            //make sure null terminate
            log[sizeof(log) - 1] = '\0';
            ERROR("build cl kernel failed: %s", log);
        }
    } else {
        size_t n, num = 0;
        char name[128];
        vector<cl_kernel> kernels;
        status = clGetProgramInfo(prog, CL_PROGRAM_NUM_KERNELS, sizeof(num), &num, NULL);
        if (!checkOclStatus(status, "clGetProgramInfo")) {
            ret = false;
            goto err;
        }
        kernels.resize(num);
        status = clCreateKernelsInProgram(prog, num, kernels.data(), NULL);
        if (!checkOclStatus(status, "clCreateKernelsInProgram")) {
            ret = false;
            goto err;
        }
        for (n = 0; n < num; n++) {
            status = clGetKernelInfo(kernels[n], CL_KERNEL_FUNCTION_NAME, sizeof(name), name, NULL);
            if (!checkOclStatus(status, "clGetKernelInfo")) {
                ret = false;
                goto err;
            }
            kernelMap[name] = kernels[n];
        }
    }
err:
    clReleaseProgram(prog);
    return ret;
}

bool OclDevice::createKernel(cl_context context, const char* name, OclKernelMap& kernelMap)
{
    AutoLock lock(m_lock);
    return loadKernel_l(context, name, kernelMap);
}

bool OclDevice::releaseKernel(OclKernelMap& kernelMap)
{
    AutoLock lock(m_lock);
    bool ret = true;
    OclKernelMap::iterator it = kernelMap.begin();
    while (it != kernelMap.end()) {
        checkOclStatus(clReleaseKernel(it->second), "ReleaseKernel");
        ++it;
    }
    return ret;
}

YamiStatus OclDevice::createImageFromFdIntel(cl_context context, const cl_import_image_info_intel* info, cl_mem* mem)
{
    cl_int status;
    *mem = m_oclCreateImageFromFdIntel(context, info, &status);
    if (checkOclStatus(status, "clCreateImageFromFdINTEL")) {
        return YAMI_SUCCESS;
    }
    return YAMI_FAIL;
}

YamiStatus OclDevice::createBufferFromFdIntel(cl_context context, const cl_import_buffer_info_intel* info, cl_mem* mem)
{
    cl_int status;
    *mem = m_oclCreateBufferFromFdIntel(context, info, &status);
    if (checkOclStatus(status, "clCreateBufferFromFdINTEL")) {
        return YAMI_SUCCESS;
    }
    return YAMI_FAIL;
}

bool checkOclStatus(cl_int status, const char* msg)
{
    /* todo add more description error here*/
    if (status != CL_SUCCESS) {
        ERROR("%s: failed, status = %d", msg, status);
        return false;
    }
    return true;
}

}
