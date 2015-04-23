/*
 *  decodeoutput.cpp - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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

#include "decodeoutput.h"
#include "common/log.h"
#include "common/utils.h"
#include <sys/stat.h>
#include <assert.h>
#include <va/va.h>
#ifdef __ENABLE_X11__
#include <X11/Xlib.h>
#endif

#ifdef __ENABLE_TESTS_GLES__
#include "./egl/gles2_help.h"
#include "egl/egl_util.h"
#endif

bool DecodeOutput::setVideoSize(int width, int height)
{
    m_width = width;
    m_height = height;
    return true;
}

DecodeOutput::DecodeOutput(IVideoDecoder* decoder)
    :m_decoder(decoder), m_width(0), m_height(0), m_renderFrames(0)
{

}

Decode_Status DecodeOutput::processOneFrame(bool drain)
{
    Decode_Status status = renderOneFrame(drain);

    if (status == RENDER_SUCCESS)
        m_renderFrames++;

    m_decoder->renderDone(&m_frame);
    return status;
}

Decode_Status DecodeOutputNull::renderOneFrame(bool drain)
{
    m_frame.memoryType = VIDEO_DATA_MEMORY_TYPE_SURFACE_ID;
    m_frame.width = 0;
    m_frame.height = 0;
    m_frame.fourcc = 0;

    Decode_Status status = m_decoder->getOutput(&m_frame, drain);
    if (status == RENDER_SUCCESS)
        vaSyncSurface((void*)(m_frame.handle), m_frame.internalID);
    return status;
}

struct ColorConvert
{
    ColorConvert(uint32_t destFourcc)
        :m_destFourcc(destFourcc) {}
    VideoFrameRawData* convert(VideoFrameRawData* frame)
    {
        if (frame->fourcc == m_destFourcc) {
            //pass through
            return frame;
        }
        assert(m_destFourcc == VA_FOURCC_I420 && frame->fourcc == VA_FOURCC_NV12);

        uint32_t width[3];
        uint32_t height[3];
        uint32_t planes;
        if (!getPlaneResolution(frame->fourcc, frame->width, frame->height, width, height, planes))
            return NULL;
        return NV12ToI420(frame, width, height);
    }

private:
    VideoFrameRawData* NV12ToI420(VideoFrameRawData* frame, uint32_t width[3], uint32_t height[3])
    {
        m_data.clear();
        memset(&m_converted, 0, sizeof(m_converted));
        m_converted.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
        m_converted.fourcc = m_destFourcc;
        m_converted.width = frame->width;
        m_converted.height = frame->height;
        uint8_t* data = reinterpret_cast<uint8_t*>(frame->handle);
        copyY(m_data, data, frame->offset[0], width[0], height[0], frame->pitch[0]);
        m_converted.pitch[0] = width[0];
        for (int i = 1; i < 3; i++) {
            m_converted.offset[i] = m_data.size();
            m_converted.pitch[i] = width[1]>>1;
            copyUV(m_data, data, frame->offset[1] + (i - 1), width[1], height[1], frame->pitch[1]);
        }
        m_converted.handle = reinterpret_cast<intptr_t>(&m_data[0]);
        return &m_converted;
    }
    static void copyY(std::vector<uint8_t>& v, uint8_t* data, uint32_t offset, uint32_t width,
        uint32_t height, uint32_t pitch)
    {
        data += offset;
        for (int h = 0; h < height; h++) {
            v.insert(v.end(), data, data + width);
            data += pitch;
        }

    }
    static void copyUV(std::vector<uint8_t>& v, uint8_t* data, uint32_t offset, uint32_t width,
        uint32_t height, uint32_t pitch)
    {
        data += offset;
        for (int h = 0; h < height; h++) {
            uint8_t* start = data;
            uint8_t* end = data + width;
            while (start < end) {
                v.push_back(*start);
                start += 2 ;
            }
            data += pitch;
        }

    }

    uint32_t m_destFourcc;
    VideoFrameRawData m_converted;
    std::vector<uint8_t> m_data;
};

Decode_Status DecodeOutputRaw::renderOneFrame(bool drain)
{
    m_frame.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
    m_frame.width = 0;
    m_frame.height = 0;
    m_frame.fourcc = m_srcFourcc;

    Decode_Status status = m_decoder->getOutput(&m_frame, drain);
    if (status != RENDER_SUCCESS)
        return status;
    assert(m_convert && "need set dest fourcc first");
    VideoFrameRawData* converted = m_convert->convert(&m_frame);
    if (!converted || !render(converted)) {
        status = RENDER_FAIL;
    }
    return status;
}

void DecodeOutputRaw::setFourcc(uint32_t fourcc)
{
    delete m_convert;
    m_destFourcc = fourcc;
    if (m_destFourcc == VA_FOURCC_I420 && m_enableSoftI420Convert) {
        m_srcFourcc = VA_FOURCC_NV12;
    } else {
        m_srcFourcc = m_destFourcc;
    }
    m_convert = new ColorConvert(m_destFourcc);
}

uint32_t DecodeOutputRaw::getFourcc()
{
    return m_destFourcc;
}

DecodeOutputRaw::DecodeOutputRaw(IVideoDecoder* decoder)
    :DecodeOutput(decoder), m_convert(NULL),
    m_srcFourcc(0), m_destFourcc(0),
    m_enableSoftI420Convert(true)
{
}

DecodeOutputRaw::~DecodeOutputRaw()
{
    delete m_convert;
}

bool DecodeOutputFileDump::config(const char* source, const char* dest, uint32_t fourcc)
{
    if (!fourcc && dest)
        fourcc == guessFourcc(dest);
    setFourcc(fourcc);
    const char* baseFileName = source;
    const char* s = strrchr(source, '/');
    if (s)
        baseFileName = s + 1;
    assert(dest);
    struct stat buf;
    int r = stat(dest, &buf);
    if (r == 0 && buf.st_mode & S_IFDIR) {
        m_name<<dest<<"/"<<baseFileName;
        m_appendSize = true;
    } else {
        m_name<<dest;
        m_appendSize = false;
    }
    return true;
}

bool DecodeOutputFileDump::setVideoSize(int width, int height)
{
    if (!m_fp) {
        if (m_appendSize) {
            uint32_t fourcc = getFourcc();
            char* ch = reinterpret_cast<char*>(&fourcc);
            m_name<<"_"<<width<<'x'<<height<<"."<<ch[0]<<ch[1]<<ch[2]<<ch[3];
        }
        std::string name = m_name.str();
        m_fp = fopen(name.c_str(), "wb");
    }
    if (!DecodeOutputRaw::setVideoSize(width, height))
        return false;
    return m_fp;
}

bool DecodeOutputFileDump::render(VideoFrameRawData* frame)
{
    uint32_t width[3];
    uint32_t height[3];
    uint32_t planes;
    if (!m_fp)
        return false;

    //ASSERT(m_width == frame->width && m_height == frame->height);
    if (!getPlaneResolution(frame->fourcc, frame->width, frame->height, width, height, planes))
        return false;
    for (int i = 0; i < planes; i++) {
        uint8_t* data = reinterpret_cast<uint8_t*>(frame->handle);
        data += frame->offset[i];
        for (int h = 0; h < height[i]; h++) {
            fwrite(data, 1, width[i], m_fp);
            data += frame->pitch[i];
        }
    }
    return true;
}

DecodeOutputFileDump::DecodeOutputFileDump(IVideoDecoder* decoder)
    :DecodeOutputRaw(decoder), m_fp(NULL), m_appendSize(false)
{

}

DecodeOutputFileDump::~DecodeOutputFileDump()
{
    if (m_fp)
        fclose(m_fp);
}


#ifdef __ENABLE_X11__
class DecodeOutputX11 : public DecodeOutput
{
public:
    virtual bool setVideoSize(int width, int height);
    DecodeOutputX11(IVideoDecoder* decoder);
    virtual ~DecodeOutputX11();
    bool init();
protected:
    Display* m_display;
    Window   m_window;
};

class DecodeOutputXWindow : public DecodeOutputX11
{
public:
    virtual Decode_Status renderOneFrame(bool drain);
    DecodeOutputXWindow(IVideoDecoder* decoder);
};


bool DecodeOutputX11::setVideoSize(int width, int height)
{
    if (m_window) {
        //todo, resize window;
    } else {
        int screen = DefaultScreen(m_display);

        XSetWindowAttributes x11WindowAttrib;
        x11WindowAttrib.event_mask = KeyPressMask;
        m_window = XCreateWindow(m_display, DefaultRootWindow(m_display),
            0, 0, width, height, 0, CopyFromParent, InputOutput,
            CopyFromParent, CWEventMask, &x11WindowAttrib);
        XMapWindow(m_display, m_window);
    }
    XSync(m_display, false);
    {
        DEBUG("m_window=0x%x", m_window);
        XWindowAttributes wattr;
        XGetWindowAttributes(m_display, m_window, &wattr);
    }
    return DecodeOutput::setVideoSize(width, height);
}

bool DecodeOutputX11::init()
{
    extern int renderMode;
    m_display = XOpenDisplay(NULL);
    if (!m_display) {
        fprintf(stderr, "Failed to XOpenDisplay during DecodeOutputX11::%s\n", __FUNCTION__);
        return false;
    }
    m_window = 0;
    NativeDisplay nativeDisplay;
    if (renderMode == 0) {
        nativeDisplay.type = NATIVE_DISPLAY_DRM;
        nativeDisplay.handle = 0;
        m_decoder->setNativeDisplay(&nativeDisplay);
    } else {
        nativeDisplay.type = NATIVE_DISPLAY_X11;
        nativeDisplay.handle = (intptr_t)m_display;
        m_decoder->setNativeDisplay(&nativeDisplay);
    }

    return true;
}

DecodeOutputX11::DecodeOutputX11(IVideoDecoder* decoder):DecodeOutput(decoder)
{
}

DecodeOutputX11::~DecodeOutputX11()
{
    if (m_window)
        XDestroyWindow(m_display, m_window);
    if (m_display)
        XCloseDisplay(m_display);

}

DecodeOutputXWindow::DecodeOutputXWindow(IVideoDecoder* decoder)
    :DecodeOutputX11(decoder)
{
}

Decode_Status DecodeOutputXWindow::renderOneFrame(bool drain)
{
    int64_t timeStamp;
    return  m_decoder->getOutput(m_window, &timeStamp, 0, 0, m_width, m_height, drain);
}

#ifdef __ENABLE_TESTS_GLES__

class DecodeOutputEgl : public DecodeOutputX11
{
public:
    virtual bool setVideoSize(int width, int height) = 0;
    DecodeOutputEgl(IVideoDecoder* decoder);
    virtual ~DecodeOutputEgl();

protected:
    bool setVideoSize(int width, int height, bool externalTexture);
    EGLContextType *m_eglContext;
    GLuint m_textureId;
};

class DecodeOutputPixelMap : public DecodeOutputEgl
{
public:
    virtual bool setVideoSize(int width, int height);
    DecodeOutputPixelMap(IVideoDecoder* decoder);
    Decode_Status renderOneFrame(bool drain);
    virtual ~DecodeOutputPixelMap();
private:
    XID m_pixmap;
};

// supports either Linux dma_buf handle or DRM flink name
class DecodeOutputDmabuf: public DecodeOutputEgl
{
public:
    virtual bool setVideoSize(int width, int height);
    virtual Decode_Status renderOneFrame(bool drain);
    DecodeOutputDmabuf(IVideoDecoder* decoder, VideoDataMemoryType memoryType);
private:
    VideoDataMemoryType m_memoryType;

};


bool DecodeOutputEgl::setVideoSize(int width, int height, bool externalTexture)
{
    if (!DecodeOutputX11::setVideoSize(width, height))
        return false;
    if (!m_eglContext)
        m_eglContext = eglInit(m_display, m_window, VA_FOURCC_RGBA, externalTexture);
    return m_eglContext;
}

DecodeOutputEgl::DecodeOutputEgl(IVideoDecoder* decoder)
    :DecodeOutputX11(decoder),m_eglContext(NULL),m_textureId(0)
{
}

DecodeOutputEgl::~DecodeOutputEgl()
{
    if(m_textureId)
        glDeleteTextures(1, &m_textureId);
    if (m_eglContext)
        eglRelease(m_eglContext);
}

bool DecodeOutputPixelMap::setVideoSize(int width, int height)
{
    if (!DecodeOutputEgl::setVideoSize(width, height, false))
        return false;
    if (!m_pixmap) {
        int screen = DefaultScreen(m_display);
        m_pixmap = XCreatePixmap(m_display, DefaultRootWindow(m_display), m_width, m_height, XDefaultDepth(m_display, screen));
        if (!m_pixmap)
            return false;
        XSync(m_display, false);
        m_textureId = createTextureFromPixmap(m_eglContext, m_pixmap);
    }
    return m_textureId;
}

Decode_Status DecodeOutputPixelMap::renderOneFrame(bool drain)
{
    int64_t timeStamp;
    Decode_Status status = m_decoder->getOutput(m_pixmap, &timeStamp, 0, 0, m_width, m_height, drain);
    if (status == RENDER_SUCCESS) {
        drawTextures(m_eglContext, GL_TEXTURE_2D, &m_textureId, 1);
    }
    return status;
}

DecodeOutputPixelMap::DecodeOutputPixelMap(IVideoDecoder* decoder)
    :DecodeOutputEgl(decoder),m_pixmap(0)
{
}

DecodeOutputPixelMap::~DecodeOutputPixelMap()
{
    if(m_pixmap)
        XFreePixmap(m_display, m_pixmap);
}

bool DecodeOutputDmabuf::setVideoSize(int width, int height)
{
    if (!DecodeOutputEgl::setVideoSize(width, height, m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF))
        return false;
    if (!m_textureId)
        glGenTextures(1, &m_textureId);
    return m_textureId;
}

Decode_Status DecodeOutputDmabuf::renderOneFrame(bool drain)
{
    GLenum target = GL_TEXTURE_2D;
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        target = GL_TEXTURE_EXTERNAL_OES;
    glBindTexture(target, m_textureId);

    m_frame.memoryType = m_memoryType;
    m_frame.fourcc = VA_FOURCC_BGRX; // VAAPI BGRA match MESA ARGB
    m_frame.width = m_width;
    m_frame.height = m_height;
    Decode_Status status = m_decoder->getOutput(&m_frame, drain);
    if (status == RENDER_SUCCESS) {
       EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
        ASSERT(m_width == m_frame.width && m_height == m_frame.height);
        eglImage = createEglImageFromHandle(m_eglContext->eglContext.display, m_eglContext->eglContext.context, m_frame.memoryType, m_frame.handle,
                m_frame.width, m_frame.height, m_frame.pitch[0]);
        if (eglImage != EGL_NO_IMAGE_KHR) {
            glEGLImageTargetTexture2DOES(target, eglImage);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            drawTextures(m_eglContext, target, &m_textureId, 1);

            eglDestroyImageKHR(m_eglContext->eglContext.display, eglImage);
        } else {
            ERROR("fail to create EGLImage from dma_buf");
        }
    }
    return status;
}

DecodeOutputDmabuf::DecodeOutputDmabuf(IVideoDecoder* decoder, VideoDataMemoryType memoryType)
    :DecodeOutputEgl(decoder), m_memoryType(memoryType)
{
}

#endif // __ENABLE_TESTS_GLES__

#endif //__ENABLE_X11__

DecodeOutput* DecodeOutput::create(IVideoDecoder* decoder, int mode)
{
    if (mode == -1)
        return new DecodeOutputNull(decoder);
    if (mode == 0)
        return new DecodeOutputFileDump(decoder);
#ifdef __ENABLE_X11__
    DecodeOutputX11* output;
    switch (mode) {
        case 1:
            output = new DecodeOutputXWindow(decoder);
            break;
#ifdef __ENABLE_TESTS_GLES__
        case 2:
            output = new DecodeOutputPixelMap(decoder);
            break;
        case 3:
            output = new DecodeOutputDmabuf(decoder, VIDEO_DATA_MEMORY_TYPE_DRM_NAME);
            break;
        case 4:
            output = new DecodeOutputDmabuf(decoder, VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
            break;
#endif //__ENABLE_TESTS_GLES__
        default:
            assert(0 && "do not support this render mode");
    }
    if (output && !output->init()) {
        delete output;
        output = NULL;
    }
    return output;
#else //__ENABLE_X11__
    return NULL;
#endif //__ENABLE_X11__
}



bool renderOutputFrames(DecodeOutput* output, bool drain)
{
    Decode_Status status;
    do {
        status = output->processOneFrame(drain);
    } while (status != RENDER_NO_AVAILABLE_FRAME && status > 0);
}


extern char *dumpOutputName;
extern uint32_t dumpFourcc;
extern char *inputFileName;
bool configDecodeOutput(DecodeOutput* output)
{
    bool ret = true;
    DecodeOutputFileDump* dump = dynamic_cast<DecodeOutputFileDump*>(output);
    if (dump) {
        ret = dump->config(inputFileName, dumpOutputName, dumpFourcc);
    }
    return ret;
}
