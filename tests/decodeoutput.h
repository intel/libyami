/*
 *  decodeoutput.h - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
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

#ifndef decodeoutput2_h
#define decodeoutput2_h

#include "vppinputoutput.h"
#include "VideoPostProcessHost.h"
#include "VideoCommonDefs.h"

#if  __ENABLE_MD5__
#include <openssl/md5.h>
#endif

#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#include <va/va_x11.h>
#endif

#ifdef __ENABLE_TESTS_GLES__
#include "./egl/gles2_help.h"
#include "egl/egl_util.h"
#include "egl/egl_vaapi_image.h"
#endif

#include <vector>

class DecodeOutput
{
public:
	virtual ~DecodeOutput() {}
    virtual bool init();
    virtual bool output(SharedPtr<VideoFrame>& frame) = 0;
    SharedPtr<NativeDisplay> nativeDisplay(){ return m_nativeDisplay;}
    static SharedPtr<DecodeOutput> create(int renderMode, uint32_t fourcc, const char* inputFile, const char* outputFile);
protected:
    SharedPtr<VADisplay> m_vaDisplay;
    SharedPtr<NativeDisplay> m_nativeDisplay;
};

class DecodeOutputNull: public DecodeOutput
{
public:
	DecodeOutputNull() {}
    ~DecodeOutputNull() {}
    bool init();
    bool output(SharedPtr<VideoFrame>& frame);
};

class ColorConvert
{
public:
    uint8_t* convert(SharedPtr<VideoFrame>& frame, SharedPtr<VADisplay> display, uint32_t pitches[3], uint32_t offsets[3])
    {
        VASurfaceID surface = (VASurfaceID)frame->surface;
        VAImage image;
        VAStatus status = vaDeriveImage(*display,surface,&image);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("vaDeriveImage failed = %d", status);
            return NULL;
        }
        uint32_t byteWidth[3], byteHeight[3], planes;
        if (!getPlaneResolution(image.format.fourcc, image.width, image.height, byteWidth, byteHeight, planes)) {
            ERROR("get plane reoslution failed for %x, %dx%d", image.format.fourcc, image.width, image.height);
            return NULL;
        }
        uint8_t* buf;
        status = vaMapBuffer(*display, image.buf, (void**)&buf);
        if (status != VA_STATUS_SUCCESS) {
            vaDestroyImage(*display, image.image_id);
            ERROR("vaMapBuffer failed = %d", status);
            return NULL;
        }
        m_tmp.clear();
        //copy y
        pitches[0] = byteWidth[0];
        uint8_t* tmp = buf + image.offsets[0];
        for (uint32_t h = 0; h < byteHeight[0]; h++) {
            m_tmp.insert(m_tmp.end(), tmp, tmp + byteWidth[0]);
            tmp += image.pitches[0];
        }
        //copy uv
        for (uint32_t i = 0; i < 2; i++) {
            offsets[i+1] = m_tmp.size();
            pitches[i+1] = byteWidth[1]>>1;
            tmp = buf + (image.offsets[1] + i);
            for (uint32_t h = 0; h < byteHeight[1]; h++) {
                uint8_t* start = tmp;
                uint8_t* end = tmp + byteWidth[1];
                while (start < end) {
                   m_tmp.push_back(*start);
                   start += 2;
                }
                tmp += image.pitches[1];
           }
        }
        vaUnmapBuffer(*display, image.buf);
        vaDestroyImage(*display, image.image_id);
        return reinterpret_cast<uint8_t*>(&m_tmp[0]);
    }
private:
    std::vector<uint8_t> m_tmp;
};

class DecodeOutputFile: public DecodeOutput
{
public:
    DecodeOutputFile(const char* outputFile, const char* inputFile, uint32_t fourcc)
        : m_outputFile(outputFile)
        , m_inputFile(inputFile)
        , m_destFourcc(fourcc) {}
    ~DecodeOutputFile();
    bool init();
    bool output(SharedPtr<VideoFrame>& frame);
protected:
    virtual bool initFile();
    virtual bool write(SharedPtr<VideoFrame>& frame);
    virtual bool writeToFile(uint8_t*, uint32_t, uint32_t*, uint32_t*, uint32_t*, uint32_t*);

    FILE*        m_file;
    const char*  m_outputFile;
    const char*  m_inputFile;
    uint32_t     m_destFourcc;
private:
    bool                 m_isConverted;
    ColorConvert*        m_convert;
    SharedPtr<VppOutput> m_output;
    VideoFrameRawData*   m_convertedFrame;
};

#if  __ENABLE_MD5__
class DecoudeOutputMD5: public DecodeOutputFile
{
public:
    DecoudeOutputMD5(const char* outputFile, const char* inputFile, uint32_t fourcc)
    : DecodeOutputFile(outputFile, inputFile, fourcc) {}
    ~DecoudeOutputMD5();
protected:
    bool initFile();
    bool write(SharedPtr<VideoFrame>& frame);
    bool writeToFile(uint8_t*, uint32_t, uint32_t*, uint32_t*, uint32_t*, uint32_t*);
    bool writeMd5ToFile(MD5_CTX&);
private:
    MD5_CTX m_fileMd5;
};
#endif //__ENABLE_MD5__

#ifdef __ENABLE_X11__
class DecodeOutputX11 : public DecodeOutput
{
public:
    DecodeOutputX11(){ m_isInit = false;}
    virtual ~DecodeOutputX11();
    bool init();
    virtual bool initWindow();
protected:
    Display* m_display;
    Window   m_window;
    bool     m_isInit;
    int      m_width;
    int      m_height;
};

class DecodeOutputXWindow: public DecodeOutputX11
{
public:
    DecodeOutputXWindow(){}
    ~DecodeOutputXWindow(){}
    bool output(SharedPtr<VideoFrame>& frame);
};

#ifdef __ENABLE_TESTS_GLES__
class DecodeOutputPixelMap : public DecodeOutputX11
{
public:
    DecodeOutputPixelMap(){}
    ~DecodeOutputPixelMap();
    bool initWindow();
    bool output(SharedPtr<VideoFrame>& frame);
private:
    XID m_pixmap;
    GLuint m_textureId;
    EGLContextType *m_eglContext;
};

class DecodeOutputDmabuf: public DecodeOutputX11
{
public:
    DecodeOutputDmabuf(VideoDataMemoryType memoryType)
    : m_memoryType(memoryType){}
    ~DecodeOutputDmabuf();
    bool initWindow();
    bool output(SharedPtr<VideoFrame>& frame);
protected:
    void createEGLImage(EGLImageKHR&, SharedPtr<VideoFrame>&);
    bool draw2D(EGLImageKHR&);
private:
    GLuint m_textureId;
    EGLContextType *m_eglContext;
    VideoDataMemoryType m_memoryType;
    SharedPtr<FrameAllocator> m_allocator;
    SharedPtr<IVideoPostProcess> m_vpp;
};
#endif //__ENABLE_TESTS_GLES__
#endif //__ENABLE_X11__

#endif //decodeoutput2_h