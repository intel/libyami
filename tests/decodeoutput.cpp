/*
 *  decodeoutput.cpp - decode outputs
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decodeoutput.h"
#include "common/log.h"
#include "common/utils.h"

#include <va/va.h>
#include <va/va_drmcommon.h>
#include <sys/stat.h>
#include <sstream>
#include <stdio.h>

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

DecodeOutput* DecodeOutput::create(int renderMode, uint32_t fourcc, const char* inputFile, const char* outputFile)
{
    DecodeOutput* output;
    switch(renderMode)
    {
        #ifdef __ENABLE_MD5__
        case -2:
            output = new DecoudeOutputMD5(outputFile, inputFile, fourcc);
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
           fprintf(stderr, "do not support this render mode\n");
           return NULL;
    }
    if (!output->init())
        ERROR("DecodeOutput init failed.");
    return output;
}

bool DecodeOutputNull::init()
{
    m_vaDisplay  = createVADisplay();
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

DecodeOutputFile::~DecodeOutputFile()
{
    if (m_convert)
        delete m_convert;
    if (m_file)
        fclose(m_file);
}

bool DecodeOutputFile::initFile()
{
    struct stat buf;
    int r = stat(m_outputFile, &buf);
    std::ostringstream outputfile;
    if (r == 0 && buf.st_mode & S_IFDIR) {
        const char* fileName = m_inputFile;
        const char* s = strrchr(m_inputFile, '/');
        if (s) fileName = s + 1;
        outputfile<<m_outputFile<<"/"<<fileName;
        char* ch = reinterpret_cast<char*>(&m_destFourcc);
        outputfile<<"."<<ch[0]<<ch[1]<<ch[2]<<ch[3];
        m_outputFile = outputfile.str().c_str();
    }
    if (m_isConverted) {
        m_file = fopen(m_outputFile, "wb");
        if (!m_file) {
            ERROR("fail to open input file: %s", m_outputFile);
            return false;
        }
    } else {
        int width, height;
        guessResolution(m_inputFile, width, height);
        m_output = VppOutput::create(m_inputFile, m_destFourcc, width, height);
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
    }
    return true;
}

bool DecodeOutputFile::init()
{
	m_vaDisplay  = createVADisplay();
	if (m_destFourcc == VA_FOURCC_I420) {
        m_isConverted = true;
        m_convert = new ColorConvert();
    }
	return (DecodeOutput::init()) && (initFile());
}

bool DecodeOutputFile::writeToFile(uint8_t* src, uint32_t planes, uint32_t width[3], uint32_t height[3],
    uint32_t pitches[3], uint32_t offsets[3])
{
    for (int i = 0; i < planes; i++) {
        uint8_t* data = src + offsets[i];
        for (int h = 0; h < height[i]; h++) {
            fwrite(data, 1, width[i], m_file);
            data += pitches[i];
        }
    }
    return true;
}

bool DecodeOutputFile::write(SharedPtr<VideoFrame>& frame)
{
    return m_output->output(frame);
}

bool DecodeOutputFile::output(SharedPtr<VideoFrame>& frame)
{
	if (m_isConverted) {
        uint32_t pitches[3]={0}, offsets[3]={0};
        uint32_t width[3]={0}, height[3]={0};
        uint32_t planes;
        uint8_t* data = m_convert->convert(frame, m_vaDisplay, pitches, offsets);
        if (!data || !getPlaneResolution(m_destFourcc, frame->crop.width, frame->crop.height, width, height, planes))
            return false;
        return writeToFile(data, planes, width, height, pitches, offsets);
    } else
        return write(frame);
}

#if  __ENABLE_MD5__
bool DecoudeOutputMD5::initFile()
{
    struct stat buf;
    int r = stat(m_outputFile, &buf);
    std::ostringstream outputfile;
    if (r == 0 && buf.st_mode & S_IFDIR) {
        const char* fileName = m_inputFile;
        const char* s = strrchr(m_inputFile, '/');
        if (s) fileName = s + 1;
        outputfile<<m_outputFile<<"/"<<fileName<<".md5";
        m_outputFile = outputfile.str().c_str();
    }
    m_file = fopen(m_outputFile, "wb");
    if (!m_file) {
        ERROR("fail to open input file: %s", m_outputFile);
        return false;
    }
    MD5_Init(&m_fileMd5);
    return true;
}

bool DecoudeOutputMD5::write(SharedPtr<VideoFrame>& frame)
{
    VASurfaceID surface = (VASurfaceID)frame->surface;
    VAImage image;
    VAStatus status = vaDeriveImage(*m_vaDisplay,surface,&image);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("vaDeriveImage failed = %d", status);
        return false;
    }
    uint32_t byteWidth[3], byteHeight[3], planes;
    if (!getPlaneResolution(image.format.fourcc, image.width, image.height, byteWidth, byteHeight, planes)) {
        ERROR("get plane reoslution failed for %x, %dx%d", image.format.fourcc, image.width, image.height);
        return false;
    }
    uint8_t* buf;
    status = vaMapBuffer(*m_vaDisplay, image.buf, (void**)&buf);
    if (status != VA_STATUS_SUCCESS) {
        vaDestroyImage(*m_vaDisplay, image.image_id);
        ERROR("vaMapBuffer failed = %d", status);
        return false;
    }
    writeToFile(buf, planes, byteWidth, byteHeight, image.pitches, image.offsets);
    vaUnmapBuffer(*m_vaDisplay, image.buf);
    vaDestroyImage(*m_vaDisplay, image.image_id);
    return true;
}

inline bool DecoudeOutputMD5::writeMd5ToFile(MD5_CTX& t_ctx)
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
    return true;
}

bool DecoudeOutputMD5::writeToFile(uint8_t* src, uint32_t planes, uint32_t width[3], uint32_t height[3],
    uint32_t pitches[3], uint32_t offsets[3])
{
    MD5_CTX t_ctx = {0};
    MD5_Init(&t_ctx);
    for (uint32_t i = 0; i < planes; i++) {
        uint8_t* data = src + offsets[i];
        for (uint32_t h = 0; h < height[i]; h++) {
            MD5_Update(&t_ctx, data, width[i]);
            MD5_Update(&m_fileMd5, data, width[i]);
            data += pitches[i];
        }
    }
    return writeMd5ToFile(t_ctx);
}

DecoudeOutputMD5::~DecoudeOutputMD5()
{
    fprintf(m_file, "The whole frames MD5 ");
    writeMd5ToFile(m_fileMd5);
}
#endif   //__ENABLE_MD5__

#if __ENABLE_X11__
static inline SharedPtr<VADisplay> createX11Display(Display* xdiplay)
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
        ERROR("Failed to XOpenDisplay during DecodeOutputX11");
        return false;
    }
    m_vaDisplay = createX11Display(m_display);
    return DecodeOutput::init();
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
    if (!m_isInit) {
        m_width = frame->crop.width;
        m_height = frame->crop.height;
        if (!initWindow()) {
            ERROR("DecodeOutputXWindow::initWindow failed");
            return false;
        }
        m_isInit = true;
    }
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
DecodeOutputPixelMap::~DecodeOutputPixelMap()
{
    if(m_textureId)
        glDeleteTextures(1, &m_textureId);
    if (m_eglContext)
        eglRelease(m_eglContext);
    if(m_pixmap)
        XFreePixmap(m_display, m_pixmap);
}

inline bool DecodeOutputPixelMap::initWindow()
{
    DecodeOutputX11::initWindow();
    m_eglContext = eglInit(m_display, m_window, VA_FOURCC_RGBA, false);
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
    if (!m_isInit) {
        m_width = frame->crop.width;
        m_height = frame->crop.height;
        if (!initWindow()) {
            ERROR("DecodeOutputPixelMap::initWindow failed");
            return false;
        }
        m_isInit = true;
    }
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

DecodeOutputDmabuf::~DecodeOutputDmabuf()
{
    if(m_textureId)
        glDeleteTextures(1, &m_textureId);
    if (m_eglContext)
        eglRelease(m_eglContext);
}

inline bool DecodeOutputDmabuf::initWindow()
{
    DecodeOutputX11::initWindow();
    m_eglContext = eglInit(m_display, m_window, VA_FOURCC_BGRX,
        m_memoryType == VIDEO_DATA_MEMORY_TYPE_DMA_BUF);
    glGenTextures(1, &m_textureId);

    SharedPtr<VADisplay> display = createVADisplay();
    m_allocator.reset(new PooledFrameAllocator(m_vaDisplay, 5));
    //VA_FOURCC_RGBA the color will become yellow.
    if (!m_allocator->setFormat(VA_FOURCC_BGRX, m_width, m_height)) {
        m_allocator.reset();
        ERROR("get Format failed");
        return false;
    }
    m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
    m_vpp->setNativeDisplay(*m_nativeDisplay);
    return m_textureId;
}

inline void DecodeOutputDmabuf::createEGLImage(EGLImageKHR& eglImage, SharedPtr<VideoFrame>& frame)
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
    frame.reset();
    vaReleaseBufferHandle(*m_vaDisplay, image.buf);
    vaDestroyImage(*m_vaDisplay, image.image_id);
}

inline bool DecodeOutputDmabuf::draw2D(EGLImageKHR& eglImage)
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
    } else {
        ERROR("fail to create EGLImage from DecodeOutputDmabuf");
        return false;
    }
}

bool DecodeOutputDmabuf::output(SharedPtr<VideoFrame>& frame)
{
    if (!m_isInit) {
        m_width = frame->crop.width;
        m_height = frame->crop.height;
        if (!initWindow()) {
            ERROR("DecodeOutputDmabuf::initWindow failed");
            return false;
        }
        m_isInit = true;
    }
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