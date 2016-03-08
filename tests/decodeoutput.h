/*
 *  decodeoutput.h - decode outputs
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *            Xu Guangxin<guangxin.xu@intel.com>
 *            Liu Yangbin<yangbinx.liu@intel.com>
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

#ifndef decodeoutput_h
#define decodeoutput_h

#include "vppinputoutput.h"
#include "VideoPostProcessHost.h"
#include "VideoCommonDefs.h"
#include "common/common_def.h"

#include <string>
#include <cassert>

class DecodeOutput
{
public:
    virtual ~DecodeOutput(){}
    virtual bool init();
    virtual bool output(SharedPtr<VideoFrame>& frame) = 0;
    SharedPtr<NativeDisplay> nativeDisplay()
    {
        return m_nativeDisplay;
    }
    virtual void setVideoSize(uint32_t with, uint32_t height)
    {
        m_width = with;
        m_height = height;
    }
    static DecodeOutput* create(int renderMode, uint32_t fourcc, const char* inputFile, const char* outputFile);
protected:
    uint32_t m_width;
    uint32_t m_height;
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
    ColorConvert(const SharedPtr<VADisplay>& display, uint32_t fourcc)
        : m_destFourcc(fourcc)
        , m_display(display)
    {
        m_width = 0;
        m_height = 0;
        m_allocator.reset(new PooledFrameAllocator(m_display, 1));
    }
    SharedPtr<VideoFrame> convert(const SharedPtr<VideoFrame>& frame)
    {
        if (frame->fourcc == m_destFourcc)
            return frame;
        SharedPtr<VideoFrame> destFrame;
        if (frame->fourcc != VA_FOURCC_NV12) {
            fprintf(stderr, "cannot convert fourcc(%u) to fourcc(%u)\n", frame->fourcc, m_destFourcc);
            return destFrame;
        }
        VAImage srcImage, dstImage;
        char *srcBuf, *destBuf;
        VAStatus status = vaDeriveImage(*m_display, (VASurfaceID)frame->surface, &srcImage);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("vaDeriveImage failed = %d", status);
            return destFrame;
        }
        status = vaMapBuffer(*m_display, srcImage.buf, (void**)&srcBuf);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("vaMapBuffer failed = %d", status);
            vaDestroyImage(*m_display, srcImage.image_id);
            return destFrame;
        }

        if (!initAllocator(srcImage.width, srcImage.height)) {
            vaUnmapBuffer(*m_display, srcImage.buf);
            vaDestroyImage(*m_display, srcImage.image_id);
            return destFrame;
        }
        destFrame = m_allocator->alloc();
        destFrame->crop.width = frame->crop.width;
        destFrame->crop.height = frame->crop.height;
        destFrame->crop.x = frame->crop.x;
        destFrame->crop.y = frame->crop.y;
        destFrame->fourcc = m_destFourcc;

        status = vaDeriveImage(*m_display, (VASurfaceID)destFrame->surface, &dstImage);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("dstImage vaDeriveImage failed = %d", status);
            destFrame.reset();
            return destFrame;
        }
        status = vaMapBuffer(*m_display, dstImage.buf, (void**)&destBuf);
        if (status != VA_STATUS_SUCCESS) {
            ERROR("dstImage vaMapBuffer failed = %d", status);
            vaDestroyImage(*m_display, dstImage.image_id);
            destFrame.reset();
            return destFrame;
        }
        copy(frame->crop.width, frame->crop.height, srcBuf, srcImage, destBuf, dstImage);

        vaUnmapBuffer(*m_display, dstImage.buf);
        vaDestroyImage(*m_display, dstImage.image_id);

        vaUnmapBuffer(*m_display, srcImage.buf);
        vaDestroyImage(*m_display, srcImage.image_id);

        return destFrame;
    }

    //support i420,yv12
    void copy(uint32_t width, uint32_t height, char *src, VAImage& srcImage, char *dest, VAImage& dstImage)
    {
        ASSERT(srcImage.format.fourcc == VA_FOURCC_NV12 && dstImage.format.fourcc == VA_FOURCC_YV12);
        uint32_t  planes, byteWidth[3], byteHeight[3];
        if (!getPlaneResolution(srcImage.format.fourcc, width, height, byteWidth, byteHeight, planes)) {
            ERROR("get plane reoslution failed");
            return;
        }
        uint32_t row, col, index;
        char *y_src, *u_src;
        char *y_dst, *u_dst, *v_dst;
        y_src = src + srcImage.offsets[0];
        u_src = src + srcImage.offsets[1];

        //copy y
        y_dst = dest + dstImage.offsets[0];
        for (row = 0; row < byteHeight[0]; ++row) {
            memcpy(y_dst, y_src, byteWidth[0]);
            y_dst += dstImage.pitches[0];
            y_src += srcImage.pitches[0];
        }

        //copy uv
        int u, v;
        u = (m_destFourcc == VA_FOURCC_YV12) ? 2 : 1;
        v = (m_destFourcc == VA_FOURCC_YV12) ? 1 : 2;
        u_dst = dest + dstImage.offsets[u];
        v_dst = dest + dstImage.offsets[v];
        for (row = 0; row < byteHeight[1]; ++row) {
            col = 0, index = 0;
            while(index < byteWidth[1]) {
                u_dst[col] = u_src[index];
                v_dst[col] = u_src[index + 1];
                index = ++col * 2;
            }
            u_src += srcImage.pitches[1];
            u_dst += dstImage.pitches[u];
            v_dst += dstImage.pitches[v];
        }
    }
    bool initAllocator(uint32_t width, uint32_t height)
    {
        if (width > m_width || height > m_height) {
            m_width = width > m_width ? width : m_width;
            m_height = height > m_height ? height : m_height;
            //VPG driver is not supported VA_FOURCC_I420, so use VA_FOURCC_YV12
            //to replace VA_FOURCC_I420. But m_destFourcc keep the true fourcc
            if (!m_allocator->setFormat(VA_FOURCC_YV12, m_width, m_height)) {
                m_allocator.reset();
                fprintf(stderr, "m_allocator setFormat failed\n");
                return false;
            }
        }
        return true;
    }

private:
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_destFourcc;
    SharedPtr<VADisplay> m_display;
    SharedPtr<FrameAllocator> m_allocator;
};

class DecodeOutputFile: public DecodeOutput
{
public:
    DecodeOutputFile(const char* outputFile, const char* inputFile, uint32_t fourcc)
        : m_destFourcc(fourcc)
        , m_inputFile(inputFile)
        , m_outputFile(outputFile)
    {
    }
    ~DecodeOutputFile() {}
    bool init();
    bool output(SharedPtr<VideoFrame>& frame);

protected:
    virtual bool initFile();
    virtual bool write(SharedPtr<VideoFrame>& frame);

    uint32_t m_destFourcc;
    const char *m_inputFile;
    std::string m_outputFile;
private:
    bool m_isConverted;
    SharedPtr<ColorConvert> m_convert;
    SharedPtr<VppOutput> m_output;
};

#endif //decodeoutput_h
