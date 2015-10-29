/*
 *  blend.cpp - blend to demo API usage, no tricks
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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

#include "bumpbox.h"
#include "vppinputdecode.h"
#include "common/log.h"
#include "common/utils.h"
#include "common/common_def.h"
#include "vaapi/vaapiutils.h"
#include "VideoDecoderHost.h"
#include "VideoPostProcessHost.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

#include <X11/Xlib.h>
#include <va/va.h>
#include <va/va_x11.h>

using namespace YamiMediaCodec;
using namespace std;

class VideoFrameDeleter
{
public:
    VideoFrameDeleter(VADisplay display):m_display(display)
    {
    }
    void operator()(VideoFrame* frame)
    {
        VASurfaceID id = (VASurfaceID)frame->surface;
        vaDestroySurfaces(m_display, &id, 1);
    }
private:
    VADisplay m_display;

};

#define ROOF(a, b) (a & (b - 1)

class Blend
{
public:
    bool init(int argc, char** argv)
    {
        if (argc != 2) {
            printf("usage: blend xxx.264\n");
            return false;
        }
        m_input.reset(new VppInputDecode);
        if (!m_input->init(argv[1])) {
            fprintf(stderr, "failed to open %s", argv[1]);
            return false;
        }

        if (!initDisplay()) {
            return false;
        }
        //set native display
        if (!m_input->config(*m_nativeDisplay))
            return false;
        if (!createVpp())
            return false;
        setRandomSeed();
        int width = m_input->getWidth();
        int height = m_input->getHeight();
        if (!createBlendSurfaces(width, height))
            return false;
        if (!createDestSurface(width, height))
            return false;
        resizeWindow(800, 600);
        return true;
    }
    bool run()
    {
        SharedPtr<VideoFrame> frame;
        VAStatus status;
        while (m_input->read(frame)) {
            //copy the decoded surface
            memset(&m_dest->crop, 0, sizeof(VideoRect));
            m_scaler->process(frame, m_dest);

            //blend it
            for (int i = 0; i < m_blendSurfaces.size(); i++) {
                m_bumpBoxes[i]->getPos(m_dest->crop.x, m_dest->crop.y, m_dest->crop.width, m_dest->crop.height);
                //printf("(%d, %d, %d, %d)\r\n", m_dest->crop.x, m_dest->crop.y, m_dest->crop.width, m_dest->crop.height);

                SharedPtr<VideoFrame>& src = m_blendSurfaces[i];
                m_blender->process(src, m_dest);
            }

            //display it on screen
            memcpy(&m_dest->crop, &frame->crop, sizeof(VideoRect));
            status = vaPutSurface(m_vaDisplay, (VASurfaceID)m_dest->surface,
                m_window, m_dest->crop.x, m_dest->crop.y, m_dest->crop.width, m_dest->crop.height, 0, 0, m_width, m_height,
                NULL, 0, 0);
            if (status != VA_STATUS_SUCCESS) {
                ERROR("vaPutSurface return %d", status);
                break;
            }

        }
        return true;
    }
    Blend():m_window(0), m_width(0), m_height(0) {}
    ~Blend()
    {
        m_input.reset();
        if (m_nativeDisplay) {
            vaTerminate(m_vaDisplay);
        }
        if (m_window) {
            XDestroyWindow(m_display.get(), m_window);
        }
    }
private:
    void setRandomSeed()
    {
        time_t t = time(NULL);
        unsigned int seed = (unsigned int)t;
        printf("set seed to %d\r\n", seed);
        srand(seed);
    }
    bool initDisplay()
    {
        Display* display = XOpenDisplay(NULL);
        if (!display) {
            fprintf(stderr, "Failed to XOpenDisplay \n");
            return false;
        }
        m_display.reset(display, XCloseDisplay);
        m_vaDisplay = vaGetDisplay(m_display.get());
        int major, minor;
        VAStatus status;
        status = vaInitialize(m_vaDisplay, &major, &minor);
        if (status != VA_STATUS_SUCCESS) {
            fprintf(stderr, "init va failed status = %d", status);
            return false;
        }
        m_nativeDisplay.reset(new NativeDisplay);
        m_nativeDisplay->type = NATIVE_DISPLAY_VA;
        m_nativeDisplay->handle = (intptr_t)m_vaDisplay;
        return true;
    }
    SharedPtr<VideoFrame> createSurface(uint32_t rtFormat, int pixelFormat, uint32_t width, uint32_t height)
    {
        SharedPtr<VideoFrame> frame;
        VAStatus status;
        VASurfaceID id;
        VASurfaceAttrib    attrib;
        attrib.type =  VASurfaceAttribPixelFormat;
        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib.value.type = VAGenericValueTypeInteger;
        attrib.value.value.i = pixelFormat;

        status = vaCreateSurfaces(m_vaDisplay, rtFormat, width, height, &id, 1, &attrib, 1);
        if (!checkVaapiStatus(status, "vaCreateSurfaces"))
            return frame;
        frame.reset(new VideoFrame, VideoFrameDeleter(m_vaDisplay));
        frame->surface = (intptr_t)id;
        frame->crop.x = frame->crop.y = 0;
        frame->crop.width = width;
        frame->crop.height = height;
        return frame;
    }

    bool drawSurfaceRGBA(SharedPtr<VideoFrame>& frame)
    {
        VASurfaceID surface = (VASurfaceID)frame->surface;
        VAImage image;

        VAStatus status = vaDeriveImage(m_vaDisplay,surface,&image);
        if (!checkVaapiStatus(status, "vaDeriveImage"))
            return false;
        assert(image.num_planes == 1);
        char* buf;
        status = vaMapBuffer(m_vaDisplay, image.buf, (void**)&buf);
        if (!checkVaapiStatus(status, "vaMapBuffer")) {
            vaDestroyImage(m_vaDisplay, image.image_id);
            return false;
        }

        uint8_t  r = rand() % 256;
        uint8_t  g = rand() % 256;
        uint8_t  b = rand() % 256;
        uint8_t  a = rand() % 256;

        uint32_t pixel = (r << 24) | (g << 16) | (b << 8) | a;
        char* ptr = buf + image.offsets[0];
        for (int i = 0; i < image.height; i++) {
            uint32_t* dest = (uint32_t*)(ptr + image.pitches[0] * i);
            for (int j = 0; j < image.width; j++) {
                *dest = pixel;
                dest++;
            }
        }
        checkVaapiStatus(vaUnmapBuffer(m_vaDisplay, image.buf), "vaUnmapBuffer");
        checkVaapiStatus(vaDestroyImage(m_vaDisplay, image.image_id), "vaDestroyImage");
        return true;
    }

    bool createBlendSurfaces(uint32_t targetWidth, uint32_t targetHeight)
    {
        uint32_t maxWidth = targetWidth / 2;
        uint32_t maxHeight = targetHeight / 2;
        for (int i = 0; i < 3; i++) {
            int w = ALIGN_POW2(rand() % maxWidth + 1, 2);
            int h = ALIGN_POW2(rand() % maxHeight + 1, 2);

            SharedPtr<VideoFrame> frame = createSurface(VA_RT_FORMAT_RGB32, VA_FOURCC_RGBA, w, h);
            if (!frame)
                return false;
            drawSurfaceRGBA(frame);
            m_blendSurfaces.push_back(frame);
            SharedPtr<BumpBox> box(new BumpBox(targetWidth, targetHeight, w, h));
            m_bumpBoxes.push_back(box);
        }
        return true;

    }

    bool createDestSurface(uint32_t targetWidth, uint32_t targetHeight)
    {
        m_dest = createSurface(VA_RT_FORMAT_YUV420, VA_FOURCC_NV12, targetWidth, targetHeight   );
        return m_dest;
    }

    bool createVpp()
    {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)m_vaDisplay;
        m_scaler.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
        m_blender.reset(createVideoPostProcess(YAMI_VPP_OCL_BLENDER), releaseVideoPostProcess);
        if (!m_scaler || !m_blender)
            return false;
        return m_scaler->setNativeDisplay(nativeDisplay) == YAMI_SUCCESS
            && m_blender->setNativeDisplay(nativeDisplay) == YAMI_SUCCESS;
    }

    void resizeWindow(int width, int height)
    {
        Display* display = m_display.get();
        if (m_window) {
        //todo, resize window;
        } else {
            DefaultScreen(display);

            XSetWindowAttributes x11WindowAttrib;
            x11WindowAttrib.event_mask = KeyPressMask;
            m_window = XCreateWindow(display, DefaultRootWindow(display),
                0, 0, width, height, 0, CopyFromParent, InputOutput,
                CopyFromParent, CWEventMask, &x11WindowAttrib);
            XMapWindow(display, m_window);
        }
        XSync(display, false);
        {
            DEBUG("m_window=%lu", m_window);
            XWindowAttributes wattr;
            XGetWindowAttributes(display, m_window, &wattr);
        }
        m_width = width;
        m_height = height;
    }
    SharedPtr<Display> m_display;
    SharedPtr<NativeDisplay> m_nativeDisplay;
    VADisplay m_vaDisplay;
    Window   m_window;
    SharedPtr<VppInputDecode> m_input;
    SharedPtr<IVideoPostProcess> m_scaler;
    SharedPtr<IVideoPostProcess> m_blender;
    int m_width, m_height;
    vector<SharedPtr<VideoFrame> > m_blendSurfaces;
    vector<SharedPtr<BumpBox> > m_bumpBoxes;
    SharedPtr<VideoFrame> m_dest;
};

int main(int argc, char** argv)
{

    Blend player;
    if (!player.init(argc, argv)) {
        ERROR("init player failed with %s", argv[1]);
        return -1;
    }
    if (!player.run()){
        ERROR("run blend failed");
        return -1;
    }
    printf("play file done\n");
    return  0;

}

