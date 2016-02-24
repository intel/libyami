/*
 *  decodeoutput.cpp - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *            Xu Guangxin<guangxin.xu@intel.com>
 *            Liu Yangbin<yangbinx.liu@intel.com>
 *            Lin Hai<hai1.lin@intel.com>
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

#include <va/va.h>
#include <va/va_drmcommon.h>
#include <sys/stat.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>


bool DecodeOutput::init()
{
    m_nativeDisplay.reset(new NativeDisplay);
    m_nativeDisplay->type = NATIVE_DISPLAY_VA;
    m_nativeDisplay->handle = (intptr_t)*m_vaDisplay;
    if (!m_vaDisplay || !m_nativeDisplay) {
        ERROR("init display error");
        return false;
    }
    return true;
}

bool DecodeOutputNull::init()
{
    m_vaDisplay  = createVADisplay();
    if (!m_vaDisplay)
        return false;
    return DecodeOutput::init();
}

bool DecodeOutputNull::output(SharedPtr<VideoFrame>& frame)
{
    VAStatus status = vaSyncSurface(*m_vaDisplay, (VASurfaceID)frame->surface);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaSyncSurface error");
        return false;
    }
    return true;
}

bool DecodeOutputFile::init()
{
    m_vaDisplay  = createVADisplay();
    if (!m_vaDisplay)
        return false;
    if (m_destFourcc != VA_FOURCC_NV12) {
        if (m_destFourcc != VA_FOURCC_YV12 && m_destFourcc != VA_FOURCC_I420) {
            fprintf(stderr, "Output Fourcc isn't supported\n");
            return false;
        }
        m_isConverted = true;
        m_convert.reset(new ColorConvert(m_vaDisplay, m_destFourcc));
    }
    return (DecodeOutput::init()) && (initFile());
}

bool DecodeOutputFile::initFile()
{
    struct stat buf;
    int r = stat(m_outputFile.c_str(), &buf);
    std::ostringstream outputfile;
    if (r == 0 && buf.st_mode & S_IFDIR) {
        const char* fileName = m_inputFile;
        const char* s = strrchr(m_inputFile, '/');
        if (s) fileName = s + 1;
        outputfile<<m_outputFile<<"/"<<fileName;
        char* ch = reinterpret_cast<char*>(&m_destFourcc);
        outputfile<<"."<<ch[0]<<ch[1]<<ch[2]<<ch[3];
        m_outputFile = outputfile.str();
    }
    int width, height;
    guessResolution(m_outputFile.c_str(), width, height);
    m_width = width, m_height = height;
    m_output = VppOutput::create(m_outputFile.c_str(), m_destFourcc, m_width, m_height);
    SharedPtr<VppOutputFile> outputFile = std::tr1::dynamic_pointer_cast<VppOutputFile>(m_output);
    if (!outputFile) {
        ERROR("convert VppOutput to VppOutputFile failed");
        return false;
    }
    SharedPtr<FrameWriter> writer(new VaapiFrameWriter(m_vaDisplay));
    if (!outputFile->config(writer)) {
        ERROR("config writer failed");
        return false;
    }
    return true;
}

bool DecodeOutputFile::write(SharedPtr<VideoFrame>& frame)
{
    return m_output->output(frame);
}

bool DecodeOutputFile::output(SharedPtr<VideoFrame>& frame)
{
    if (m_isConverted)
        frame = m_convert->convert(frame);
    return write(frame);
}

#if __ENABLE_MD5__
class DecodeOutputMD5: public DecodeOutputFile
{
public:
    DecodeOutputMD5(const char* outputFile, const char* inputFile, uint32_t fourcc)
    : DecodeOutputFile(outputFile, inputFile, fourcc)
    {}
    ~DecodeOutputMD5();
protected:
    bool initFile();
    bool write(SharedPtr<VideoFrame>& frame);
    std::string writeToFile(MD5_CTX&);
    static bool calcMD5(char* ptr, int size, FILE* fp);

private:
    FILE *m_file;
    static MD5_CTX m_fileMD5;
    static MD5_CTX m_frameMD5;
    SharedPtr<VaapiFrameIO> m_frameIO;
};

MD5_CTX DecodeOutputMD5::m_frameMD5 = {0};
MD5_CTX DecodeOutputMD5::m_fileMD5 = {0};
bool DecodeOutputMD5::initFile()
{
    struct stat buf;
    int r = stat(m_outputFile.c_str(), &buf);
    std::ostringstream outputfile;
    if (r == 0 && buf.st_mode & S_IFDIR) {
        const char* fileName = m_inputFile;
        const char* s = strrchr(m_inputFile, '/');
        if (s) fileName = s + 1;
        outputfile<<m_outputFile<<"/"<<fileName<<".md5";
        m_outputFile = outputfile.str();
    }
    m_file = fopen(m_outputFile.c_str(), "wb");
    if (!m_file) {
        ERROR("fail to open input file: %s", m_outputFile.c_str());
        return false;
    }
    MD5_Init(&m_fileMD5);
    m_frameIO.reset(new VaapiFrameIO(m_vaDisplay, calcMD5));
    return true;
}

std::string DecodeOutputMD5::writeToFile(MD5_CTX& t_ctx)
{
    char        temp[4];
    uint8_t     result[16] = {0};
    std::string strMD5;
    MD5_Final(result, &t_ctx);
    for(uint32_t i = 0; i < 16; i++) {
        memset(temp, 0, sizeof(temp));
        snprintf(temp, sizeof(temp), "%02x", (uint32_t)result[i]);
        strMD5 += temp;
    }
    fprintf(m_file, "%s\n", strMD5.c_str());
    return strMD5;
}

bool DecodeOutputMD5::write(SharedPtr<VideoFrame>& frame)
{
    MD5_Init(&m_frameMD5);
    m_frameIO->doIO(m_file, frame);
    writeToFile(m_frameMD5);
    return true;
}

bool DecodeOutputMD5::calcMD5(char* ptr, int size, FILE* fp)
{
    MD5_Update(&m_frameMD5, ptr, size);
    MD5_Update(&m_fileMD5, ptr, size);
    return true;
}

DecodeOutputMD5::~DecodeOutputMD5()
{
    fprintf(m_file, "The whole frames MD5 ");
    std::string fileMd5 = writeToFile(m_fileMD5);
    fprintf(stderr, "The whole frames MD5:\n%s\n", fileMd5.c_str());
    fclose(m_file);
}
#endif //__ENABLE_MD5__


#ifdef __ENABLE_X11__
class DecodeOutputX11 : public DecodeOutput
{
public:
    DecodeOutputX11(){ m_isInit = false;}
    virtual ~DecodeOutputX11();
    bool init();
    virtual void checkInit(uint32_t, uint32_t);
    virtual bool initWindow();
protected:
    Display* m_display;
    Window   m_window;
    bool     m_isInit;
};

class DecodeOutputXWindow: public DecodeOutputX11
{
public:
    DecodeOutputXWindow(){}
    ~DecodeOutputXWindow(){}
    bool output(SharedPtr<VideoFrame>& frame);
};

static SharedPtr<VADisplay> createX11Display(Display* xdiplay)
{
    SharedPtr<VADisplay> display;
    VADisplay m_vaDisplay = vaGetDisplay(xdiplay);
    int major, minor;
    VAStatus status = vaInitialize(m_vaDisplay, &major, &minor);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaInitialize error");
        return display;
    }
    display.reset(new VADisplay(m_vaDisplay));
    return display;
}

bool DecodeOutputX11::init()
{
    m_display = XOpenDisplay(NULL);
    if (!m_display) {
        ERROR("Failed to XOpenDisplay for DecodeOutputX11");
        return false;
    }
    m_vaDisplay = createX11Display(m_display);
    return DecodeOutput::init();
}

inline void DecodeOutputX11::checkInit(uint32_t width, uint32_t height)
{
    if (!m_isInit) {
        m_width = width;
        m_height = height;
        if (!initWindow()) {
            ERROR("DecodeOutputXWindow::initWindow failed");
            assert(0);
        }
        m_isInit = true;
    }
}

bool DecodeOutputX11::initWindow()
{
    DefaultScreen(m_display);
    XSetWindowAttributes x11WindowAttrib;
    x11WindowAttrib.event_mask = KeyPressMask;
    m_window = XCreateWindow(m_display, DefaultRootWindow(m_display),0, 0,
        m_width, m_height, 0, CopyFromParent, InputOutput, CopyFromParent,
        CWEventMask, &x11WindowAttrib);
    XMapWindow(m_display, m_window);
    XSync(m_display, false);
    {
        XWindowAttributes wattr;
        XGetWindowAttributes(m_display, m_window, &wattr);
    }
    return true;
}

DecodeOutputX11::~DecodeOutputX11()
{
    if (m_window)
        XDestroyWindow(m_display, m_window);
    if (m_display)
        XCloseDisplay(m_display);
}

bool DecodeOutputXWindow::output(SharedPtr<VideoFrame>& frame)
{
    checkInit(frame->crop.width, frame->crop.height);

    VAStatus status = vaPutSurface(*m_vaDisplay, (VASurfaceID)frame->surface,
        m_window, 0, 0, frame->crop.width, frame->crop.height,
        frame->crop.x, frame->crop.y, frame->crop.width, frame->crop.height,
        NULL, 0, 0);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaPutSurface return %d", status);
        return false;
    }
    return true;
}

#ifdef __ENABLE_TESTS_GLES__
class DecodeOutputEgl: public DecodeOutputX11
{
public:
    DecodeOutputEgl(){}
    ~DecodeOutputEgl();
    bool initWindow(bool type);
protected:
    GLuint m_textureId;
    EGLContextType *m_eglContext;
};

class DecodeOutputPixelMap : public DecodeOutputEgl
{
public:
    DecodeOutputPixelMap(){}
    ~DecodeOutputPixelMap();
    bool initWindow();
    bool output(SharedPtr<VideoFrame>& frame);
private:
    XID m_pixmap;
};

class DecodeOutputDmabuf: public DecodeOutputEgl
{
public:
    DecodeOutputDmabuf(VideoDataMemoryType memoryType)
    : m_memoryType(memoryType){}
    ~DecodeOutputDmabuf(){}
    bool initWindow();
    void checkInit(uint32_t, uint32_t);
    bool output(SharedPtr<VideoFrame>& frame);
protected:
    void createEGLImage(EGLImageKHR&, SharedPtr<VideoFrame>&);
    bool draw2D(EGLImageKHR&);
private:
    VideoDataMemoryType m_memoryType;
    SharedPtr<FrameAllocator> m_allocator;
    SharedPtr<IVideoPostProcess> m_vpp;
};

DecodeOutputEgl::~DecodeOutputEgl()
{
    if(m_textureId)
        glDeleteTextures(1, &m_textureId);
    if (m_eglContext)
        eglRelease(m_eglContext);
}

bool DecodeOutputEgl::initWindow(bool type)
{
    DecodeOutputX11::initWindow();
    m_eglContext = eglInit(m_display, m_window, VA_FOURCC_BGRX, type);
    return true;
}

DecodeOutputPixelMap::~DecodeOutputPixelMap()
{
    if(m_pixmap)
        XFreePixmap(m_display, m_pixmap);
}

bool DecodeOutputPixelMap::initWindow()
{
    DecodeOutputEgl::initWindow(false);
    int screen = DefaultScreen(m_display);
    m_pixmap = XCreatePixmap(m_display, DefaultRootWindow(m_display),
        m_width, m_height,XDefaultDepth(m_display, screen));
    if (!m_pixmap) return false;
    XSync(m_display, false);
    m_textureId = createTextureFromPixmap(m_eglContext, m_pixmap);
    return  m_textureId;
}

bool DecodeOutputPixelMap::output(SharedPtr<VideoFrame>& frame)
{
    checkInit(frame->crop.width, frame->crop.height);

    VAStatus status = vaPutSurface(*m_vaDisplay, (VASurfaceID)frame->surface,
        m_pixmap, 0, 0, m_width, m_height,
        frame->crop.x, frame->crop.y, frame->crop.width, frame->crop.height,
        NULL, 0, 0);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaPutSurface return %d", status);
        return false;
    }
    drawTextures(m_eglContext, GL_TEXTURE_2D, &m_textureId, 1);
    return true;
}

bool DecodeOutputDmabuf::initWindow()
{
    SharedPtr<VADisplay> display = createVADisplay();
    if (!display)
        return false;
    DecodeOutputEgl::initWindow(m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
    glGenTextures(1, &m_textureId);
    m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
    m_vpp->setNativeDisplay(*m_nativeDisplay);
    m_allocator.reset(new PooledFrameAllocator(m_vaDisplay, 1));
    if (!m_allocator->setFormat(VA_FOURCC_BGRX, m_width, m_height)) {
        m_allocator.reset();
        fprintf(stderr, "m_allocator setFormat failed\n");
        return false;
    }
    return m_textureId;
}

inline void DecodeOutputDmabuf::checkInit(uint32_t width, uint32_t height)
{
    DecodeOutputX11::checkInit(width, height);

    if (width > m_width || height > m_height) {
        m_width = width > m_width ? width : m_width;
        m_height = height > m_height ? height : m_height;
        if (!m_allocator->setFormat(VA_FOURCC_BGRX, m_width, m_height)) {
            m_allocator.reset();
            fprintf(stderr, "m_allocator setFormat failed\n");
        }
    }
}

void DecodeOutputDmabuf::createEGLImage(EGLImageKHR& eglImage, SharedPtr<VideoFrame>& frame)
{
    VASurfaceID surface = (VASurfaceID)frame->surface;
    VAImage image;
    VAStatus status = vaDeriveImage(*m_vaDisplay, surface, &image);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaDeriveImage failed = %d", status);
        eglImage = NULL;
        return;
    }
    VABufferInfo    m_bufferInfo;
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DRM_NAME)
        m_bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;
    else if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        m_bufferInfo.mem_type = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
    VAStatus vaStatus = vaAcquireBufferHandle(*m_vaDisplay, image.buf, &m_bufferInfo);
    if (vaStatus != VA_STATUS_SUCCESS) {
        ERROR("vaAcquireBufferHandle failed");
        vaDestroyImage(*m_vaDisplay, image.image_id);
        eglImage = NULL;
        return;
    }
    eglImage = createEglImageFromHandle(m_eglContext->eglContext.display, m_eglContext->eglContext.context,
        m_memoryType, m_bufferInfo.handle, image.width, image.height, image.pitches[0]);
    vaReleaseBufferHandle(*m_vaDisplay, image.buf);
    vaDestroyImage(*m_vaDisplay, image.image_id);
}

bool DecodeOutputDmabuf::draw2D(EGLImageKHR& eglImage)
{
    GLenum target = GL_TEXTURE_2D;
    if (m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF)
        target = GL_TEXTURE_EXTERNAL_OES;
    glBindTexture(target, m_textureId);
    if (eglImage != EGL_NO_IMAGE_KHR) {
        imageTargetTexture2D(target, eglImage);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        drawTextures(m_eglContext, target, &m_textureId, 1);
        destroyImage(m_eglContext->eglContext.display, eglImage);
        return true;
    }
    ERROR("fail to create EGLImage from DecodeOutputDmabuf");
    return false;
}

bool DecodeOutputDmabuf::output(SharedPtr<VideoFrame>& frame)
{
    checkInit(frame->crop.width, frame->crop.height);

    SharedPtr<VideoFrame> dest = m_allocator->alloc();
    YamiStatus result = m_vpp->process(frame, dest);
    if (result != YAMI_SUCCESS) {
        ERROR("vpp process failed, status = %d", result);
        return false;
    }
    EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
    createEGLImage(eglImage, dest);

    return draw2D(eglImage);
}
#endif //__ENABLE_TESTS_GLES__
#endif //__ENABLE_X11__

DecodeOutput* DecodeOutput::create(int renderMode, uint32_t fourcc, const char* inputFile, const char* outputFile)
{
    DecodeOutput* output;
    switch(renderMode) {
#ifdef __ENABLE_MD5__
    case -2:
        output = new DecodeOutputMD5(outputFile, inputFile, fourcc);
        break;
#endif //__ENABLE_MD5__
    case -1:
        output = new DecodeOutputNull();
        break;
    case 0:
        output = new DecodeOutputFile(outputFile, inputFile, fourcc);
        break;
#ifdef __ENABLE_X11__
    case 1:
        output = new DecodeOutputXWindow();
        break;
#ifdef __ENABLE_TESTS_GLES__
    case 2:
        output = new DecodeOutputPixelMap();
        break;
    case 3:
        output = new DecodeOutputDmabuf(VIDEO_DATA_MEMORY_TYPE_DRM_NAME);
        break;
    case 4:
        output = new DecodeOutputDmabuf(VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
        break;
#endif //__ENABLE_TESTS_GLES__
#endif //__ENABLE_X11__
    default:
       fprintf(stderr, "renderMode:%d, do not support this render mode\n", renderMode);
       return NULL;
    }
    if (!output->init())
        fprintf(stderr, "DecodeOutput init failed\n");
    return output;
}
