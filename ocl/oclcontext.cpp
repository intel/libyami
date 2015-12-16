/*
 *  oclcontext.cpp - abstract for opencl.
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
    bool createKernel(cl_context context, const char* name, cl_kernel& kernel);
    YamiStatus createImageFromFdIntel(cl_context context, const cl_import_image_info_intel* info, cl_mem* mem);

private:
    typedef cl_mem (*OclCreateImageFromFdIntel)(cl_context, const cl_import_image_info_intel*, cl_int*);

    OclDevice();
    bool init();
    bool loadKernel_l(cl_context context, const char* name, cl_kernel& kernel);
    bool loadFile(const char* path, vector<char>& dest);

    static WeakPtr<OclDevice> m_instance;
    static Lock m_lock;

    //all operations need procted by m_lock
    cl_platform_id m_platform;
    cl_device_id m_device;
    OclCreateImageFromFdIntel m_oclCreateImageFromFdIntel;
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

bool OclContext::createKernel(const char* name, cl_kernel& kernel)
{
    return m_device->createKernel(m_context, name, kernel);
}

YamiStatus
OclContext::createImageFromFdIntel(const cl_import_image_info_intel* info, cl_mem* mem)
{
    return m_device->createImageFromFdIntel(m_context, info, mem);
}

OclDevice::OclDevice()
    :m_platform(0), m_device(0), m_oclCreateImageFromFdIntel(0)
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

#ifdef CL_VERSION_1_2
    m_oclCreateImageFromFdIntel = (OclCreateImageFromFdIntel)
        clGetExtensionFunctionAddressForPlatform(m_platform, "clCreateImageFromFdINTEL");
#else
    m_oclCreateImageFromFdIntel = (OclCreateImageFromFdIntel)
        clGetExtensionFunctionAddress("clCreateImageFromFdINTEL");
#endif
    if (!m_oclCreateImageFromFdIntel) {
        ERROR("failed to get extension function createImageFromFdIntel");
        return false;
    }
    return true;
}

bool OclDevice::loadFile(const char* path, vector<char>& dest)
{
    FILE* fp = fopen(path, "rb");
    if (!fp)
        return false;
    fseek(fp,0L,SEEK_END);
    long len = ftell(fp);
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

bool OclDevice::loadKernel_l(cl_context context, const char* name, cl_kernel& kernel)
{
    kernel = 0;
    string path = name;
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
    if (!checkOclStatus(status, "clBuildProgram"))
    {
        char log[1024];
        status = clGetProgramBuildInfo(prog, m_device, CL_PROGRAM_BUILD_LOG, sizeof(log), log, NULL);
        if (checkOclStatus(status, "clCreateProgramWithSource")) {
            //make sure null terminate
            log[sizeof(log) - 1] = '\0';
            ERROR("build cl kernel failed: %s", log);
        }
    } else {
        kernel = clCreateKernel(prog, name, &status);
        checkOclStatus(status, "clCreateKernel");
    }
    clReleaseProgram(prog);
    return kernel != 0;
}

bool OclDevice::createKernel(cl_context context, const char* name, cl_kernel& kernel)
{
      AutoLock lock(m_lock);
      return loadKernel_l(context, name, kernel);
}


YamiStatus OclDevice::createImageFromFdIntel(cl_context context, const cl_import_image_info_intel* info, cl_mem* mem)
{
    cl_int status;
    *mem = m_oclCreateImageFromFdIntel(context, info, &status);
    if (checkOclStatus(status, "clCreateImageFromFdINTEL")) {
        return YAMI_SUCCESS;
    } else {
        return YAMI_FAIL;
    }
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
