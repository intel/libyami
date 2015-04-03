/*
 *  vppinputoutput.h - input and output for vpp
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Guangxin Xu<Guangxin.Xu@intel.com>
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
#ifndef vppinputoutput_h
#define vppinputoutput_h

#include "common/log.h"
#include "common/utils.h"
#include "VideoCommonDefs.h"
#include "videopool.h"
#include <stdio.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <vector>


using namespace YamiMediaCodec;

//virtual bool setFormat(uint32_t fourcc, int width, int height) = 0;
class FrameReader
{
public:
    virtual bool read(FILE* fp,const SharedPtr<VideoFrame>& frame) = 0;
    virtual ~FrameReader() {}
};

class FrameWriter
{
public:
    virtual bool write(FILE* fp,const SharedPtr<VideoFrame>& frame) = 0;
    virtual ~FrameWriter() {}
};

class FrameAllocator
{
public:
    virtual bool setFormat(uint32_t fourcc, int width, int height) = 0;
    virtual SharedPtr<VideoFrame> alloc() = 0;
    virtual ~FrameAllocator() {}
};


//vaapi related operation
class PooledFrameAllocator : public FrameAllocator
{
public:
    PooledFrameAllocator(const SharedPtr<VADisplay>& display, int poolsize):
        m_display(display), m_poolsize(poolsize)
    {
    }
    bool setFormat(uint32_t fourcc, int width, int height)
    {
        m_surfaces.resize(m_poolsize);
        VASurfaceAttrib attrib;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib.type = VASurfaceAttribPixelFormat;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = fourcc;
        VAStatus status = vaCreateSurfaces(*m_display, VA_RT_FORMAT_YUV420, width, height,
                                           &m_surfaces[0], m_surfaces.size(),&attrib, 1);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("create surface failed fourcc = %p", fourcc);
            m_surfaces.clear();
            return false;
        }
        std::deque<SharedPtr<VideoFrame> > buffers;
        for (int i = 0;  i < m_surfaces.size(); i++) {
            SharedPtr<VideoFrame> f(new VideoFrame);
            memset(f.get(), 0, sizeof(VideoFrame));
            f->surface = (intptr_t)m_surfaces[i];
            buffers.push_back(f);
        }
        m_pool = VideoPool<VideoFrame>::create(buffers);
        return true;
    }
    SharedPtr<VideoFrame> alloc()
    {
        return m_pool->alloc();
    }
    ~PooledFrameAllocator()
    {
        if (m_surfaces.size())
            vaDestroySurfaces(*m_display, &m_surfaces[0], m_surfaces.size());
    }
private:
    SharedPtr<VADisplay> m_display;
    std::vector<VASurfaceID> m_surfaces;
    SharedPtr<VideoPool<VideoFrame> > m_pool;
    int m_poolsize;
};

class VaapiFrameIO
{
public:
    typedef bool (*FileIoFunc)(char* ptr, int size, FILE* fp);
    VaapiFrameIO(const SharedPtr<VADisplay>& display, FileIoFunc io)
        :m_display(display), m_io(io)
    {

    };
    bool doIO(FILE* fp, const SharedPtr<VideoFrame>& frame)
    {
        if (!fp || !frame) {
            ERROR("invalid param");
            return false;
        }
        VASurfaceID surface = (VASurfaceID)frame->surface;
        VAImage image;

        VAStatus status = vaDeriveImage(*m_display,surface,&image);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("vaDeriveImage failed = %d", status);
            return false;
        }
        uint32_t byteWidth[3], byteHeight[3], planes;
        if (!getPlaneResolution(image.format.fourcc, image.width, image.height, byteWidth, byteHeight, planes)) {
            ERROR("get plane reoslution failed for %x, %dx%d", image.format.fourcc, image.width, image.height);
            return false;
        }
        char* buf;
        status = vaMapBuffer(*m_display, image.buf, (void**)&buf);
        if (status != VA_STATUS_SUCCESS) {
            vaDestroyImage(*m_display, image.image_id);
            ERROR("vaMapBuffer failed = %d", status);
            return false;
        }
        bool ret = true;
        for (int i = 0; i < planes; i++) {
            char* ptr = buf + image.offsets[i];
            int w = byteWidth[i];
            for (int j = 0; j < byteHeight[i]; j++) {
                ret = m_io(ptr, w, fp);
                if (!ret)
                    goto out;
                ptr += image.pitches[i];
            }
        }
    out:
        vaUnmapBuffer(*m_display, image.buf);
        vaDestroyImage(*m_display, image.image_id);
        return ret;

    }
private:
    FileIoFunc  m_io;
    SharedPtr<VADisplay>  m_display;
};


class VaapiFrameReader:public FrameReader
{
public:
    VaapiFrameReader(const SharedPtr<VADisplay>& display)
        :m_frameio(new VaapiFrameIO(display, readFromFile))
    {
    }
    bool read(FILE* fp, const SharedPtr<VideoFrame>& frame)
    {
        return m_frameio->doIO(fp, frame);
    }
private:
    SharedPtr<VaapiFrameIO> m_frameio;
    static bool readFromFile(char* ptr, int size, FILE* fp)
    {
        return fread(ptr, 1, size, fp) == size;
    }
};

class VaapiFrameWriter:public FrameWriter
{
public:
    VaapiFrameWriter(const SharedPtr<VADisplay>& display)
        :m_frameio(new VaapiFrameIO(display, writeToFile))
    {
    }
    bool write(FILE* fp, const SharedPtr<VideoFrame>& frame)
    {
        return m_frameio->doIO(fp, frame);
    }
private:
    SharedPtr<VaapiFrameIO> m_frameio;
    static bool writeToFile(char* ptr, int size, FILE* fp)
    {
        return fwrite(ptr, 1, size, fp) == size;
    }
};
//vaapi related operation end

class VppInput;
class VppInputFile;

class VppInput {
public:
    static SharedPtr<VppInput>
        create(const char* inputFileName, uint32_t fourcc = 0, int width = 0, int height = 0);
    virtual bool init(const char* inputFileName, uint32_t fourcc, int width, int height) = 0;
    virtual bool read(SharedPtr<VideoFrame>& frame) = 0;
    int getWidth() { return m_width;}
    int getHeight() { return m_height;}

    virtual ~VppInput() {};

protected:
    uint32_t m_fourcc;
    int m_width;
    int m_height;
};

class VppInputFile : public VppInput {
public:
    //inherit VppInput
    bool init(const char* inputFileName, uint32_t fourcc, int width, int height);
    virtual bool read(SharedPtr<VideoFrame>& frame);

    bool config(const SharedPtr<FrameAllocator>& allocator, const SharedPtr<FrameReader>& reader);
    VppInputFile();
    ~VppInputFile();
protected:
    FILE *m_fp;
    bool m_readToEOS;
    SharedPtr<FrameReader> m_reader;
    SharedPtr<FrameAllocator> m_allocator;
};

class VppOutput
{
public:
    static SharedPtr<VppOutput>
        create(const char* outputFileName);
    bool getFormat(uint32_t& fourcc, int& width, int& height);
    virtual bool output(const SharedPtr<VideoFrame>& frame) = 0;
    VppOutput();
    virtual ~VppOutput(){}
protected:
    virtual bool init(const char* outputFileName) = 0;
    uint32_t m_fourcc;
    int m_width;
    int m_height;
};

class VppOutputFile : public VppOutput
{
public:
    bool config(const SharedPtr<FrameWriter>& writer);

    //inherit VppOutput
    bool output(const SharedPtr<VideoFrame>& frame);
    virtual ~VppOutputFile();
    VppOutputFile();
protected:
    bool init(const char* outputFileName);
private:
    bool write(const SharedPtr<VideoFrame>& frame);
    SharedPtr<FrameWriter> m_writer;
    FILE* m_fp;
};

#endif      //vppinputoutput_h
