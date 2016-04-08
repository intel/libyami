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

#include "vppinputoutput.h"
#include "decodeinput.h"
#include "VideoDecoderHost.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "VideoPostProcessHost.h"
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>
#include <ui/DisplayInfo.h>
#include <android/native_window.h>
#include <ufo/gralloc.h>
#include <system/window.h>
#include <ui/GraphicBufferMapper.h>
#include <ufo/graphics.h>
#include <va/va_android.h>
#include <va/va.h>
#include <map>
#include <vector>

using namespace YamiMediaCodec;

#ifndef CHECK_EQ
#define CHECK_EQ(a, b) assert((a) == (b))
#endif

#define ANDROID_DISPLAY 0x18C34078

class AndroidPlayer
{
public:
    bool init(int argc, char** argv)
    {
        if (argc != 2) {
            printf("usage: androidplayer xxx.264\n");
            return false;
        }

        if(!initWindow()) {
            fprintf(stderr, "failed to create android surface\n");
            return false;
        }

        m_input.reset(DecodeInput::create(argv[1]));
        if (!m_input) {
            fprintf(stderr, "failed to open %s", argv[1]);
            return false;
        }

        if (!initDisplay()) {
            return false;
        }

        if(!createVpp()) {
            fprintf(stderr, "create vpp failed\n");
            return false;
        }

        //init decoder
        m_decoder.reset(createVideoDecoder(m_input->getMimeType()), releaseVideoDecoder);
        if (!m_decoder) {
            fprintf(stderr, "failed create decoder for %s", m_input->getMimeType());
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
                //resend the buffer
                status = m_decoder->decode(&inputBuffer);
            }
            if(status == DECODE_SUCCESS) {
                renderOutputs();
            } else {
                fprintf(stderr, "decode error %d\n", status);
                break;
            }
        }
        //renderOutputs();
        m_decoder->stop();
        return true;
    }

    AndroidPlayer() : m_width(0), m_height(0)
    {
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (hw_module_t const**)&m_pGralloc);
    }

    ~AndroidPlayer()
    {
    }
private:
    VADisplay m_vaDisplay;
    bool initDisplay()
    {
        unsigned int display = ANDROID_DISPLAY;
        m_vaDisplay = vaGetDisplay(&display);

        int major, minor;
        VAStatus status;
        status = vaInitialize(m_vaDisplay, &major, &minor);
        if (status != VA_STATUS_SUCCESS) {
            fprintf(stderr, "init vaDisplay failed\n");
            return false;
        }

        m_nativeDisplay.reset(new NativeDisplay);
        m_nativeDisplay->type = NATIVE_DISPLAY_VA;
        m_nativeDisplay->handle = (intptr_t)m_vaDisplay;
        return true;
    }

    bool createVpp()
    {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)m_vaDisplay;
        m_vpp.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
       return m_vpp->setNativeDisplay(nativeDisplay) == YAMI_SUCCESS;
    }

    void renderOutputs()
    {
        SharedPtr<VideoFrame> srcFrame;
        do {
            srcFrame = m_decoder->getOutput();
            if (!srcFrame)
                break;

            if(!displayOneFrame(srcFrame))
                break;
        } while (1);
    }

    bool initWindow()
    {
        static sp<SurfaceComposerClient> client = new SurfaceComposerClient();
        //create surface
        static sp<SurfaceControl> surfaceCtl = client->createSurface(String8("testsurface"), 800, 600, HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL, 0);

        // configure surface
        SurfaceComposerClient::openGlobalTransaction();
        surfaceCtl->setLayer(100000);
        surfaceCtl->setPosition(100, 100);
        surfaceCtl->setSize(800, 600);
        SurfaceComposerClient::closeGlobalTransaction();

        m_surface = surfaceCtl->getSurface();

        static sp<ANativeWindow> mNativeWindow = m_surface;
        int bufWidth = 640;
        int bufHeight = 480;
        CHECK_EQ(0,
                 native_window_set_usage(
                 mNativeWindow.get(),
                 GRALLOC_USAGE_SW_READ_NEVER | GRALLOC_USAGE_SW_WRITE_OFTEN
                 | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP));

        CHECK_EQ(0,
                 native_window_set_scaling_mode(
                 mNativeWindow.get(),
                 NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW));

        CHECK_EQ(0, native_window_set_buffers_geometry(
                    mNativeWindow.get(),
                    bufWidth,
                    bufHeight,
                    HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL));

        status_t err;
        err = native_window_set_buffer_count(mNativeWindow.get(), 5);
        if (err != 0) {
            ALOGE("native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);
            return false;
        }

        return true;
    }

    SharedPtr<VideoFrame> createVaSurface(const ANativeWindowBuffer* buf)
    {
        SharedPtr<VideoFrame> frame;

        intel_ufo_buffer_details_t info;
        memset(&info, 0, sizeof(info));
        *reinterpret_cast<uint32_t*>(&info) = sizeof(info);

        int err = 0;
        if (m_pGralloc)
            err = m_pGralloc->perform(m_pGralloc, INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_INFO, (buffer_handle_t)buf->handle, &info);

        if (0 != err || !m_pGralloc) {
            fprintf(stderr, "create vaSurface failed\n");
            return frame;
        }

        VASurfaceAttrib attrib;
        memset(&attrib, 0, sizeof(attrib));

        VASurfaceAttribExternalBuffers external;
        memset(&external, 0, sizeof(external));

        external.pixel_format = VA_FOURCC_NV12;
        external.width = buf->width;
        external.height = buf->height;
        external.pitches[0] = info.pitch;
        external.num_planes = 2;
        external.num_buffers = 1;
        uint8_t* handle = (uint8_t*)buf->handle;
        external.buffers = (long unsigned int*)&handle; //graphic handel
        external.flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;

        attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
        attrib.type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
        attrib.value.type = VAGenericValueTypePointer;
        attrib.value.value.p = &external;

        VASurfaceID id;
        VAStatus vaStatus = vaCreateSurfaces(m_vaDisplay, VA_RT_FORMAT_YUV420,
                            buf->width, buf->height, &id, 1, &attrib, 1);
        if (vaStatus != VA_STATUS_SUCCESS)
            return frame;

        frame.reset(new VideoFrame);
        memset(frame.get(), 0, sizeof(VideoFrame));

        frame->surface = static_cast<intptr_t>(id);
        frame->crop.width = buf->width;
        frame->crop.height = buf->height;

        return frame;
    }

    bool displayOneFrame(SharedPtr<VideoFrame>& srcFrame)
    {
        status_t err;
        SharedPtr<VideoFrame> dstFrame;
        sp<ANativeWindow> mNativeWindow = m_surface;
        ANativeWindowBuffer* buf;

        err = native_window_dequeue_buffer_and_wait(mNativeWindow.get(), &buf);
        if (err != 0) {
            fprintf(stderr, "dequeueBuffer failed: %s (%d)\n", strerror(-err), -err);
            return false;
        }

        std::map< ANativeWindowBuffer*, SharedPtr<VideoFrame> >::const_iterator it;
        it = m_buff.find(buf);
        if (it != m_buff.end()) {
            dstFrame = it->second;
        } else {
            dstFrame = createVaSurface(buf);
            m_buff.insert(std::pair<ANativeWindowBuffer*, SharedPtr<VideoFrame> >(buf, dstFrame));
        }

        m_vpp->process(srcFrame, dstFrame);

        if (mNativeWindow->queueBuffer(mNativeWindow.get(), buf, -1) != 0) {
            fprintf(stderr, "queue buffer to native window failed\n");
            return false;
        }
        return true;
    }

    SharedPtr<NativeDisplay> m_nativeDisplay;

    SharedPtr<IVideoDecoder> m_decoder;
    SharedPtr<DecodeInput> m_input;
    int m_width, m_height;

    sp<Surface> m_surface;
    gralloc_module_t* m_pGralloc;
    std::map< ANativeWindowBuffer*, SharedPtr<VideoFrame> > m_buff;
    SharedPtr<IVideoPostProcess> m_vpp;
};

int main(int argc, char** argv)
{
    AndroidPlayer player;
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
