/*
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _INTEL_METADATA_BUFFER_H_
#define _INTEL_METADATA_BUFFER_H_

#include <stdint.h>

#define STRING_TO_FOURCC(format) ((uint32_t)(((format)[0])|((format)[1]<<8)|((format)[2]<<16)|((format)[3]<<24)))

class IntelMetadataBuffer {
public:
    enum IMB_Result {
        IMB_SUCCESS = 0,
        IMB_INVAL_PARAM = 1,
        IMB_INVAL_BUFFER = 2,
    };

    enum MemMode {
        MEM_MODE_MALLOC = 1,
        MEM_MODE_CI = 2,
        MEM_MODE_V4L2 = 4,
        MEM_MODE_SURFACE = 8,
        MEM_MODE_USRPTR = 16,
        MEM_MODE_GFXHANDLE = 32,
        MEM_MODE_KBUFHANDLE = 64,
        MEM_MODE_ION = 128,
        MEM_MODE_DRMPRIME = 256,
        MEM_MODE_DRMFLINK = 512,
    };

    struct ValueInfo {
            MemMode mode;               //memory type, vasurface/malloc/gfx/ion/v4l2/ci etc
            uint32_t handle;            //handle
            uint32_t size;              //memory size
            uint32_t width;             //picture width
            uint32_t height;            //picture height
            uint32_t lumaStride;        //picture luma stride
            uint32_t chromStride;       //picture chrom stride
            uint32_t format;            //color format
            uint32_t s3dformat;         //S3D format
    };

    enum MetadataBufferType {
        MetadataBufferTypeCameraSource = 0,   //for CameraSource
        MetadataBufferTypeGrallocSource = 1,  //for SurfaceMediaSource
        MetadataBufferTypeEncoder = 2,        //for WiDi clone mode
        MetadataBufferTypeUser = 3,           //for WiDi user mode
        MetadataBufferTypeLast = 4,           //type number
    };

    IntelMetadataBuffer();                                          //for generator
    IntelMetadataBuffer(MetadataBufferType type, int32_t value);    //for quick generator
    ~IntelMetadataBuffer();

    IMB_Result GetType(MetadataBufferType &type);
    IMB_Result SetType(MetadataBufferType type);
    IMB_Result GetValue(int32_t &value);
    IMB_Result SetValue(int32_t value);
    IMB_Result GetValueInfo(ValueInfo* &info);
    IMB_Result SetValueInfo(ValueInfo *info);
    IMB_Result GetExtraValues(int32_t* &values, uint32_t &num);
    IMB_Result SetExtraValues(int32_t *values, uint32_t num);

    //for bytes input, also for parser
    IMB_Result SetBytes(uint8_t* data, uint32_t size);

    //for bytes output, also for generator
    IMB_Result GetBytes(uint8_t* &data, uint32_t& size);

    //Static, for get max IntelMetadataBuffer size
    static uint32_t GetMaxBufferSize();

private:
    MetadataBufferType mType;
    int32_t mValue;
    ValueInfo* mInfo;

    int32_t* mExtraValues;
    uint32_t mExtraValues_Count;

    uint8_t* mBytes;
    uint32_t mSize;
};

#endif

