/*
 * Copyright (C) 2011-2014 Intel Corporation. All rights reserved.
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
#ifndef encodeinput_h
#define  encodeinput_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "VideoEncoderDefs.h"
#include "VideoEncoderInterface.h"
#include "common/NonCopyable.h"
#include <vector>
#if ANDROID
#include <gui/Surface.h>
#include <android/native_window.h>
#include <system/window.h>
#include <queue>
using namespace android;
#endif

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
    uint32_t m_width;
    uint32_t m_height;
    size_t m_frameSize;
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
private:
    DISALLOW_COPY_AND_ASSIGN(EncodeInputFile);
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
    std::vector<uint8_t*> m_frameBuffers;
    uint32_t m_frameBufferCount;
    uint32_t m_frameBufferSize;
    CameraDataMode m_dataMode;

    bool openDevice();
    bool initDevice(const char *cameraDevicePath);
    bool initMmap();
    bool startCapture();
    int32_t dequeFrame();
    bool enqueFrame(int32_t index);
    bool stopCapture();
    bool uninitDevice();

};

#if ANDROID
#define SURFACE_BUFFER_COUNT 5
class EncodeInputSurface : public EncodeInput {
public:
    EncodeInputSurface() { m_bufferCount = SURFACE_BUFFER_COUNT; }
    ~EncodeInputSurface(){};
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
    virtual bool getOneFrameInput(VideoFrameRawData& inputBuffer);
    virtual bool isEOS() { return false; }

protected:
    int m_bufferCount;
    sp<Surface> m_surface;
    sp<ANativeWindow> mNativeWindow;
    std::queue<ANativeWindowBuffer*> mBufferInfo;

private:
    bool prepareInputBuffer();
    DISALLOW_COPY_AND_ASSIGN(EncodeInputSurface);
};
#endif

class EncodeOutput {
public:
    EncodeOutput();
    virtual ~EncodeOutput();
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

class EncodeOutputHEVC : public EncodeOutput
{
    virtual const char* getMimeType();
};

bool createOutputBuffer(VideoEncOutputBuffer* outputBuffer, int maxOutSize);
#ifdef __BUILD_GET_MV__
bool createMVBuffer(VideoEncMVBuffer* MVBuffer, int Size);
#endif
#endif
