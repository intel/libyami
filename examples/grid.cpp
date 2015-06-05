/*
 *  simpleplayer.cpp - simpeplayer to demo API usage, no tricks
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


#include "common/log.h"
#include "common/utils.h"
#include "tests/vppinputdecode.h"
#include "interface/VideoPostProcessHost.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <deque>

#include <X11/Xlib.h>
#include <va/va_x11.h>

using namespace YamiMediaCodec;
using namespace std;

class Grid
{
    struct NullDeleter
    {
        void operator ()(VADisplay *) {}
    };
public:
    bool init(int argc, char** argv)
    {
        if (argc != 3) {
            printf("usage: grid xxx.264\n");
            return false;
        }

        if (!initDisplay()) {
            return false;
        }

        resizeWindow(0, 0);
        m_width = m_winWidth;
        m_height = m_winHeight;
/*        m_width = 4096;
        m_height = 2160;*/
        m_col = 4;
        m_row = 4;


        int len = m_col * m_row;
        for (int i = 0; i < len; i++) {
            SharedPtr<VppInputDecode> input(new VppInputDecode);
            if (!input->init(argv[i%2 + 1])) {
                fprintf(stderr, "failed to open %s", argv[1]);
                return false;
            }
            //set native display
            if (!input->config(*m_nativeDisplay)) {
                return false;
            }
            m_inputs.push_back(input);
        }

        m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
        if (!m_vpp) {
            ERROR("can't create vpp");
            return false;
        }
        if (m_vpp->setNativeDisplay(*m_nativeDisplay) != YAMI_SUCCESS) {
            ERROR("set display for vpp failed");
            return false;
        }

        SharedPtr<VADisplay> vaDisplay(&m_vaDisplay, NullDeleter());
        m_allocator.reset(new PooledFrameAllocator(vaDisplay, 3));

        if (!m_allocator->setFormat(VA_FOURCC_RGBX, m_width, m_height)) {
            ERROR("set alloctor format failed");
            return false;
        }
        return true;
    }
    bool run()
    {
        renderOutputs();
        return true;
    }
    Grid():m_window(0), m_width(0), m_height(0), m_col(0), m_row(0) {}
    ~Grid()
    {
        //reset before we terminate the va
        m_vpp.reset();
        m_inputs.clear();
        m_allocator.reset();

        if (m_nativeDisplay) {
            vaTerminate(m_vaDisplay);
        }
        if (m_window) {
            XDestroyWindow(m_display.get(), m_window);
        }
    }
private:
    void renderOutputs()
    {
        SharedPtr<VideoFrame> frame;
        VAStatus status = VA_STATUS_SUCCESS;
        FpsCalc fps;
        int width = m_width / m_col;
        int height = m_height / m_row;



        SharedPtr<VADisplay> vaDisplay(&m_vaDisplay, NullDeleter());

        SharedPtr<VppOutput> output =  VppOutput::create("3840x2160.BGRX");
        SharedPtr<VppOutputFile> outputFile = std::tr1::dynamic_pointer_cast<VppOutputFile>(output);
        if (outputFile) {
            SharedPtr<FrameWriter> writer(new VaapiFrameWriter(vaDisplay));
            if (!outputFile->config(writer)) {
                ERROR("config writer failed");
                output.reset();
                return;
            }
        }
#if 1

        SharedPtr<PooledFrameAllocator> allocator;
        allocator.reset(new PooledFrameAllocator(vaDisplay, 3));

        if (!allocator->setFormat(VA_FOURCC_NV12, 1920, 1080)) {
            ERROR("set alloctor format failed");
            return ;
        }
        #endif
        do {
            SharedPtr<VideoFrame> dest = m_allocator->alloc();
            //static deque<SharedPtr<VideoFrame> > ds;
            //ds.push_back(dest);
            for (int i = 0; i < m_row; i++) {
                for (int j = 0; j < m_col; j++) {
                    SharedPtr<VppInputDecode>& input = m_inputs[i * m_col + j];
                    if (!input->read(frame))
                        goto DONE;
                    dest->crop.x = j * width;
                    dest->crop.y = i * height;
                    dest->crop.width = width;
                    dest->crop.height = height;
                    m_vpp->process(frame, dest);
                    //status = vaPutSurface(m_vaDisplay, (VASurfaceID)frame->surface,
                    //m_window, 0, 0, frame->crop.width, frame->crop.height,dest->crop.x, dest->crop.y, dest->crop.width, dest->crop.height,
                    //NULL, 0, 0);
                }
            }

            #if 1
            SharedPtr<VideoFrame> src = dest;
            memset(&src->crop, 0, sizeof(src->crop));
   /*                             src->crop.x = 0;
                    src->crop.y = 0;
                    src->crop.width = m_width/2;
                    src->crop.height = m_height/2;
*/
            dest = allocator->alloc();
            m_vpp->process(src, dest);
            //output->output(dest);
             //ERROR("width = %d, height = %d, win = %dx%d", m_width, m_height, m_winWidth, m_winHeight);
            goto DONE;
            #endif

            #if 1
            status = vaPutSurface(m_vaDisplay, (VASurfaceID)dest->surface,
                m_window, 0, 0, 1920, 1080, 0, 0, m_winWidth, m_winHeight,
                NULL, 0, 0);
            #endif
            #if 0
            if (ds.size() > 2) {
                dest = ds.front();
                ds.pop_front();
            }
            vaSyncSurface(m_vaDisplay, (VASurfaceID)dest->surface);
            dest.reset();
            #endif
            if (status != VA_STATUS_SUCCESS) {
                ERROR("vaPutSurface return %d", status);
                break;
            }
            fps.addFrame();
        } while (1);
DONE:
        fps.log();
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
    void resizeWindow(int width, int height)
    {
        Display* display = m_display.get();
        if (m_window) {
        //todo, resize window;
        } else {
            Screen* screen = XDefaultScreenOfDisplay(display);
            if (!screen) {
                ERROR("can't get default screen");
                return;
            }
            int screenWidth = WidthOfScreen(screen);
            int screenHeight = HeightOfScreen(screen);
            if (width == 0 || height == 0
                || width > screenWidth || height > screenHeight) {
                width = screenWidth;
                height = screenHeight;
                ERROR("width = %d, height = %d", width, height);
            }

            XSetWindowAttributes x11WindowAttrib;
            x11WindowAttrib.event_mask = KeyPressMask;
            m_window = XCreateWindow(display, DefaultRootWindow(display),
                0, 0, width, height, 0, CopyFromParent, InputOutput,
                CopyFromParent, CWEventMask, &x11WindowAttrib);
            XMapWindow(display, m_window);
        }
        XSync(display, false);
        {
            DEBUG("m_window=0x%x", m_window);
            XWindowAttributes wattr;
            XGetWindowAttributes(display, m_window, &wattr);
        }
        m_winWidth = width;
        m_winHeight = height;
    }
    SharedPtr<Display> m_display;
    SharedPtr<NativeDisplay> m_nativeDisplay;
    VADisplay m_vaDisplay;
    Window   m_window;
    vector< SharedPtr<VppInputDecode> > m_inputs;
    SharedPtr<IVideoPostProcess> m_vpp;
    SharedPtr<FrameAllocator> m_allocator;
    int m_width, m_height;
    int m_col, m_row;
    int m_winWidth, m_winHeight;
};

int main(int argc, char** argv)
{

    Grid grid;
    if (!grid.init(argc, argv)) {
        ERROR("init grid failed with %s", argv[1]);
        return -1;
    }
    if (!grid.run()){
        ERROR("run grid failed");
        return -1;
    }
    printf("play file done\n");
    return  0;

}
