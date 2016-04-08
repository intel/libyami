/*
 * Copyright (C) 2013-2014 Intel Corporation. All rights reserved.
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "decodeinput.h"
#include "common/log.h"
#include "common/utils.h"
#include "VideoDecoderHost.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <va/va_x11.h>

using namespace YamiMediaCodec;

class SimplePlayer
{
public:
    bool init(int argc, char** argv)
    {
        if (argc != 2) {
            printf("usage: simpleplayer xxx.264\n");
            return false;
        }
        m_input.reset(DecodeInput::create(argv[1]));
        if (!m_input) {
            fprintf(stderr, "failed to open %s", argv[1]);
            return false;
        }

        //init decoder
        m_decoder.reset(createVideoDecoder(m_input->getMimeType()), releaseVideoDecoder);
        if (!m_decoder) {
            fprintf(stderr, "failed create decoder for %s", m_input->getMimeType());
            return false;
        }

        if (!initDisplay()) {
            return false;
        }
        //set native display
        m_decoder->setNativeDisplay(m_nativeDisplay.get());
        return true;
    }
    bool run()
    {
        VideoConfigBuffer configBuffer;
        memset(&configBuffer, 0, sizeof(configBuffer));
        configBuffer.profile = VAProfileNone;
        const string codecData = m_input->getCodecData();
        if (codecData.size()) {
            configBuffer.data = (uint8_t*)codecData.data();
            configBuffer.size = codecData.size();
        }

        Decode_Status status = m_decoder->start(&configBuffer);
        assert(status == DECODE_SUCCESS);

        VideoDecodeBuffer inputBuffer;
        while (m_input->getNextDecodeUnit(inputBuffer)) {
            status = m_decoder->decode(&inputBuffer);
            if (DECODE_FORMAT_CHANGE == status) {
                //drain old buffers
                renderOutputs();
                const VideoFormatInfo *formatInfo = m_decoder->getFormatInfo();
                resizeWindow(formatInfo->width, formatInfo->height);
                //resend the buffer
                status = m_decoder->decode(&inputBuffer);
            }
            if(status == DECODE_SUCCESS) {
                renderOutputs();
            } else {
                ERROR("decode error status = %d", status);
                break;
            }
        }
        m_decoder->stop();
        return true;
    }
    SimplePlayer():m_window(0), m_width(0), m_height(0) {}
    ~SimplePlayer()
    {
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
        VAStatus status = VA_STATUS_SUCCESS;
        do {
            SharedPtr<VideoFrame> frame = m_decoder->getOutput();
            if (!frame)
                break;
            status = vaPutSurface(m_vaDisplay, (VASurfaceID)frame->surface,
                m_window, 0, 0, m_width, m_height, 0, 0, m_width, m_height,
                NULL, 0, 0);
            if (status != VA_STATUS_SUCCESS) {
                ERROR("vaPutSurface return %d", status);
                break;
            }
        } while (1);
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
    SharedPtr<IVideoDecoder> m_decoder;
    SharedPtr<DecodeInput> m_input;
    int m_width, m_height;
};

int main(int argc, char** argv)
{

    SimplePlayer player;
    if (!player.init(argc, argv)) {
        ERROR("init player failed with %s", argv[1]);
        return -1;
    }
    if (!player.run()){
        ERROR("run simple player failed");
        return -1;
    }
    printf("play file done\n");
    return  0;

}

