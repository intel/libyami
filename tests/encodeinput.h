/*
 *  h264_encode.cpp - h264 encode test
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Cong Zhong<congx.zhong@intel.com>
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
#ifndef encodeinput_h
#define  encodeinput_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "VideoEncoderDefs.h"
#include "VideoEncoderInterface.h"
#include <vector>
#include <tr1/memory>

using namespace YamiMediaCodec;

class EncodeInput;
class EncodeInputFile;
class EncodeInputCamera;
class EncodeInput {
public:
    static EncodeInput* create(const char* inputFileName, uint32_t fourcc, int width, int height);
    EncodeInput() : m_width(0), m_height(0), m_frameSize(0) {};
    virtual ~EncodeInput() {};
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height) = 0;
    virtual bool getOneFrameInput(VideoFrameRawData &inputBuffer) = 0;
    virtual bool recycleOneFrameInput(VideoFrameRawData &inputBuffer) {return true;};
    virtual bool isEOS() = 0;
    int getWidth() { return m_width;}
    int getHeight() { return m_height;}

protected:
    uint32_t m_fourcc;
    int m_width;
    int m_height;
    int m_frameSize;
};

class EncodeInputFile : public EncodeInput {
public:
    EncodeInputFile();
    ~EncodeInputFile();
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
    virtual bool getOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool isEOS() {return m_readToEOS;}

protected:
    FILE *m_fp;
    uint8_t *m_buffer;
    bool m_readToEOS;
};

class EncodeInputCamera : public EncodeInput {
public:
    enum CameraDataMode{
        CAMERA_DATA_MODE_MMAP,
        // CAMERA_DATA_MODE_DMABUF_MMAP,
        // CAMERA_DATA_MODE_USRPTR,
        // CAMERA_DATA_MODE_DMABUF_USRPTR,
    };
    EncodeInputCamera() :m_frameBufferCount(5), m_frameBufferSize(0), m_dataMode(CAMERA_DATA_MODE_MMAP) {};
    ~EncodeInputCamera();
    virtual bool init(const char* cameraPath, uint32_t fourcc, int width, int height);
    bool setDataMode(CameraDataMode mode = CAMERA_DATA_MODE_MMAP) {m_dataMode = mode; return true;};

    virtual bool getOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool recycleOneFrameInput(VideoFrameRawData &inputBuffer);
    virtual bool isEOS() { return false; }
    // void getSupportedResolution();
private:
    int m_fd;
    CameraDataMode m_dataMode;
    std::vector<uint8_t*> m_frameBuffers;
    uint32_t m_frameBufferCount;
    uint32_t m_frameBufferSize;

    bool openDevice();
    bool initDevice(const char *cameraDevicePath);
    bool initMmap();
    bool startCapture();
    int32_t dequeFrame();
    bool enqueFrame(int32_t index);
    bool stopCapture();
    bool uninitDevice();

};

class EncodeOutput {
public:
    EncodeOutput();
    ~EncodeOutput();
    static  EncodeOutput* create(const char* outputFileName, int width , int height);
    virtual bool write(void* data, int size);
    virtual const char* getMimeType() = 0;
protected:
    virtual bool init(const char* outputFileName, int width , int height);
    FILE *m_fp;
};

class EncodeOutputH264 : public EncodeOutput
{
    virtual const char* getMimeType();
};

class EncodeOutputVP8 : public EncodeOutput {
public:
    EncodeOutputVP8();
    ~EncodeOutputVP8();
    virtual const char* getMimeType();
    virtual bool write(void* data, int size);
protected:
    virtual bool init(const char* outputFileName, int width , int height);
private:
    int m_frameCount;
};

class EncodeStreamOutputJpeg : public EncodeOutput
{
    virtual const char* getMimeType();
};

bool createOutputBuffer(VideoEncOutputBuffer* outputBuffer, int maxOutSize);
#ifdef __BUILD_GET_MV__
bool createMVBuffer(VideoEncMVBuffer* MVBuffer, int Size);
#endif
#endif
