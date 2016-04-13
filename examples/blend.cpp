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

#include "bumpbox.h"
#include "font.h"
#include "vppinputdecode.h"
#include "common/log.h"
#include "common/utils.h"
#include "common/common_def.h"
#include "vaapi/VaapiUtils.h"
#include "VideoDecoderHost.h"
#include "VideoPostProcessHost.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include "vpp/oclpostprocess_osd.h"
#include <X11/Xlib.h>
#include <va/va.h>
#include <va/va_x11.h>

struct timespec start, end;
#define PERF_START(block) clock_gettime(CLOCK_REALTIME, &start);
#define PERF_STOP(block) clock_gettime(CLOCK_REALTIME, &end);     \
                         INFO("%s used %f ms\n", block,           \
                             (end.tv_sec - start.tv_sec) * 1000   \
                           + (end.tv_nsec - start.tv_nsec) / 1E6);

using namespace YamiMediaCodec;
using namespace std;

class VideoFrameDeleter
{
public:
    VideoFrameDeleter(SharedPtr<VADisplay> display)
        : m_display(display)
    {
    }
    void operator()(VideoFrame* frame)
    {
        VASurfaceID id = (VASurfaceID)frame->surface;
        vaDestroySurfaces(*m_display, &id, 1);
    }
private:
    SharedPtr<VADisplay> m_display;
};

void VADisplayDeleter(VADisplay* display)
{
    checkVaapiStatus(vaTerminate(*display), "Terminate");
    delete display;
}

#define ROOF(a, b) (a & (b - 1)

class Blend
{
public:
    bool init(int argc, char** argv)
    {
        const char* optShort = "bom";
        const struct option optLong[] = {
            { "flip", required_argument, NULL, 'F' },
            { "rotate", required_argument, NULL, 'R' },
            { "ot", required_argument, NULL, 'T' },
            { "ms", required_argument, NULL, 'S' },
            {0, 0, 0, 0},
        };
        int opt;
        while ((opt = getopt_long(argc, argv, optShort, optLong, NULL)) != -1) {
            switch (opt) {
            case 'b':
                m_bBlend = true;
                break;
            case 'o':
                m_bOsd = true;
                break;
            case 'm':
                m_bMosaic = true;
                break;
            case 'F': {
                int flip = atoi(optarg);
                if (flip != 0 && flip != 1) {
                    fprintf(stderr, "invalid flip type\n");
                    usage();
                    return false;
                }
                m_flipRot |= (flip + VPP_TRANSFORM_FLIP_H);
                break;
            }
            case 'R': {
                int rot = atoi(optarg);
                if (rot != 90 && rot != 180 && rot != 270) {
                    fprintf(stderr, "invalid rotation degree\n");
                    usage();
                    return false;
                }
                m_flipRot |= (1 << (rot / 90 + 1));
                break;
            }
            case 'T': {
                int thr = atoi(optarg);
                if (thr < 0 || thr > 255) {
                    fprintf(stderr, "invalid osd threshold\n");
                    usage();
                    return false;
                }
                m_osdThreshold = thr;
                break;
            }
            case 'S': {
                int size = atoi(optarg);
                if (size < 1 || size >256) {
                    fprintf(stderr, "invalid mosaice block size\n");
                    usage();
                    return false;
                }
                m_mosaicSize = size;
                break;
            }
            default:
                fprintf(stderr, "invalid argument");
                usage();
                return false;
            }
        }
        printf("Blend: %d, OSD(%d): %d, Mosaic(%d): %d, Transform: %u\n",
            m_bBlend, m_osdThreshold, m_bOsd, m_mosaicSize, m_bMosaic, m_flipRot);

        m_input.reset(new VppInputDecode);
        if (optind >= argc) {
            fprintf(stderr, "please specify video clip\n");
            usage();
            return false;
        }

        if (!m_input->init(argv[optind])) {
            fprintf(stderr, "failed to open %s", argv[optind]);
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
        if (m_bBlend && !createBlendSurfaces(width, height))
            return false;
        if (m_bOsd && !createOSDSurfaces(width, height))
            return false;
        if (m_bMosaic && !createMosaicSurfaces(width, height))
            return false;
        if (!createDestSurface(width, height))
            return false;
        if (m_flipRot) {
            if (m_flipRot & (VPP_TRANSFORM_ROT_90 | VPP_TRANSFORM_ROT_270)) {
                if (!createDisplaySurface(height, width))
                    return false;
                resizeWindow(540, 960);
            }
            else {
                if (!createDisplaySurface(width, height))
                    return false;
                resizeWindow(960, 540);
            }
        }
        else {
            resizeWindow(960, 540);
        }
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

            if (m_bBlend) {
                process(m_blender, "blend", m_blendSurfaces, m_blendBumpBoxes, m_dest);
            }

            if (m_bOsd) {
                process(m_osd, "osd", m_osdSurfaces, m_osdBumpBoxes, m_dest);
            }

            if (m_bMosaic) {
                vector<SharedPtr<VideoFrame> > srcFrame;
                srcFrame.push_back(m_dest);
                process(m_mosaic, "mosaic", srcFrame, m_mosaicBumpBoxes, m_dest);
            }

            if (m_flipRot) {
                PERF_START("transform");
                memcpy(&m_dest->crop, &frame->crop, sizeof(VideoRect));
                if (m_flipRot & (VPP_TRANSFORM_ROT_90 | VPP_TRANSFORM_ROT_270)) {
                    m_displaySurface->crop.x = m_dest->crop.y;
                    m_displaySurface->crop.y = m_dest->crop.x;
                    m_displaySurface->crop.width = m_dest->crop.height;
                    m_displaySurface->crop.height = m_dest->crop.width;
                }
                else {
                    memcpy(&m_displaySurface->crop, &m_dest->crop, sizeof(VideoRect));
                }
                m_transform->process(m_dest, m_displaySurface);
                PERF_STOP("transform");
                //display it on screen
                status = vaPutSurface(*m_vaDisplay, (VASurfaceID)m_displaySurface->surface,
                    m_window, m_displaySurface->crop.x, m_displaySurface->crop.y, m_displaySurface->crop.width, m_displaySurface->crop.height, 0, 0, m_width, m_height,
                    NULL, 0, 0);
            }
            else {
                //display it on screen
                memcpy(&m_dest->crop, &frame->crop, sizeof(VideoRect));
                status = vaPutSurface(*m_vaDisplay, (VASurfaceID)m_dest->surface,
                    m_window, m_dest->crop.x, m_dest->crop.y, m_dest->crop.width, m_dest->crop.height, 0, 0, m_width, m_height,
                    NULL, 0, 0);
            }
            if (status != VA_STATUS_SUCCESS) {
                ERROR("vaPutSurface return %d", status);
                break;
            }
        }
        return true;
    }
    Blend()
        : m_window(0)
        , m_width(0)
        , m_height(0)
        , m_bBlend(false)
        , m_bOsd(false)
        , m_bMosaic(false)
        , m_osdThreshold(128)
        , m_mosaicSize(32)
        , m_flipRot(0)
    {
    }
    ~Blend()
    {
        m_input.reset();
        if (m_window) {
            XDestroyWindow(m_display.get(), m_window);
        }
    }
private:
    void usage()
    {
        fprintf(stderr, "Usage: blend [OPTIONS] xxx.264\n"
            "  -b               enable alpha blending\n"
            "  -o               enable OSD\n"
            "  -m               enable mosaic\n"
            "  --flip=TYPE      0: flip horizontal, 1: flip vertical\n"
            "  --rotate=DEGREE  rotate clock wise: 0, 90, 270\n"
            "  --ot=THRESHOLD   threshold for OSD font color: [0, 255]\n"
            "  --ms=SIZE        mosaic block size: [1, 256]\n");
    }
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
        VADisplay vaDisplay = vaGetDisplay(m_display.get());
        int major, minor;
        VAStatus status;
        status = vaInitialize(vaDisplay, &major, &minor);
        if (status != VA_STATUS_SUCCESS) {
            fprintf(stderr, "init va failed status = %d", status);
            return false;
        }
        m_vaDisplay.reset(new VADisplay(vaDisplay), VADisplayDeleter);
        m_nativeDisplay.reset(new NativeDisplay);
        m_nativeDisplay->type = NATIVE_DISPLAY_VA;
        m_nativeDisplay->handle = (intptr_t)*m_vaDisplay;
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

        status = vaCreateSurfaces(*m_vaDisplay, rtFormat, width, height, &id, 1, &attrib, 1);
        if (!checkVaapiStatus(status, "vaCreateSurfaces"))
            return frame;
        frame.reset(new VideoFrame, VideoFrameDeleter(m_vaDisplay));
        frame->surface = (intptr_t)id;
        frame->crop.x = frame->crop.y = 0;
        frame->crop.width = width;
        frame->crop.height = height;
        return frame;
    }

    bool drawSurfaceRGBA(SharedPtr<VideoFrame>& frame, bool text = false)
    {
        VASurfaceID surface = (VASurfaceID)frame->surface;
        VAImage image;

        VAStatus status = vaDeriveImage(*m_vaDisplay, surface, &image);
        if (!checkVaapiStatus(status, "vaDeriveImage"))
            return false;
        assert(image.num_planes == 1);
        char* buf;
        status = vaMapBuffer(*m_vaDisplay, image.buf, (void**)&buf);
        if (!checkVaapiStatus(status, "vaMapBuffer")) {
            vaDestroyImage(*m_vaDisplay, image.image_id);
            return false;
        }

        if (text) {
            int n = rand() % (sizeof(font) / sizeof(font[0]));
            char* ptr = buf + image.offsets[0];
            for (int i = 0; i < image.height; i++) {
                uint32_t* dest = (uint32_t*)(ptr + image.pitches[0] * i);
                for (int j = 0; j < image.width; j++) {
                    *dest = font[n][i * FONT_BLOCK_SIZE + j % FONT_BLOCK_SIZE];
                    dest++;
                }
            }
        }
        else {
            uint8_t r = rand() % 256;
            uint8_t g = rand() % 256;
            uint8_t b = rand() % 256;
            uint8_t a = rand() % 256;

            uint32_t pixel = (r << 24) | (g << 16) | (b << 8) | a;
            char* ptr = buf + image.offsets[0];
            for (int i = 0; i < image.height; i++) {
                uint32_t* dest = (uint32_t*)(ptr + image.pitches[0] * i);
                for (int j = 0; j < image.width; j++) {
                    *dest = pixel;
                    dest++;
                }
            }
        }

        checkVaapiStatus(vaUnmapBuffer(*m_vaDisplay, image.buf), "vaUnmapBuffer");
        checkVaapiStatus(vaDestroyImage(*m_vaDisplay, image.image_id), "vaDestroyImage");
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
            frame->fourcc = YAMI_FOURCC_RGBA;
            drawSurfaceRGBA(frame);
            m_blendSurfaces.push_back(frame);
            SharedPtr<BumpBox> box(new BumpBox(targetWidth, targetHeight, w, h));
            m_blendBumpBoxes.push_back(box);
        }
        return true;
    }

    bool createOSDSurfaces(uint32_t targetWidth, uint32_t targetHeight)
    {
        for (int i = 0; i < 3; i++) {
            int w = FONT_BLOCK_SIZE * 22;
            int h = FONT_BLOCK_SIZE;

            SharedPtr<VideoFrame> text = createSurface(VA_RT_FORMAT_RGB32, VA_FOURCC_RGBA, w, h);
            if (!text)
                return false;
            text->fourcc = YAMI_FOURCC_RGBA;
            drawSurfaceRGBA(text, true);
            m_osdSurfaces.push_back(text);
            SharedPtr<BumpBox> box(new BumpBox(targetWidth, targetHeight, w, h));
            m_osdBumpBoxes.push_back(box);
        }
        return true;
    }

    bool createMosaicSurfaces(uint32_t targetWidth, uint32_t targetHeight)
    {
        uint32_t maxWidth = targetWidth / 2;
        uint32_t maxHeight = targetHeight / 2;
        for (int i = 0; i < 1; i++) {
            int w = maxWidth;
            int h = maxHeight;
            SharedPtr<BumpBox> box(new BumpBox(targetWidth, targetHeight, w, h));
            m_mosaicBumpBoxes.push_back(box);
        }
        return true;
    }

    bool createDestSurface(uint32_t targetWidth, uint32_t targetHeight)
    {
        m_dest = createSurface(VA_RT_FORMAT_YUV420, VA_FOURCC_NV12, targetWidth, targetHeight);
        m_dest->fourcc = YAMI_FOURCC_NV12; 
        return m_dest;
    }

    bool createDisplaySurface(uint32_t targetWidth, uint32_t targetHeight)
    {
        m_displaySurface = createSurface(VA_RT_FORMAT_YUV420, VA_FOURCC_NV12, targetWidth, targetHeight);
        m_displaySurface->fourcc = YAMI_FOURCC_NV12;
        return m_displaySurface;
    }

    bool createVpp()
    {
        NativeDisplay nativeDisplay;
        nativeDisplay.type = NATIVE_DISPLAY_VA;
        nativeDisplay.handle = (intptr_t)*m_vaDisplay;
        m_scaler.reset(createVideoPostProcess(YAMI_VPP_SCALER), releaseVideoPostProcess);
        if (!m_scaler || m_scaler->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
            return false;

        if (m_bBlend) {
            m_blender.reset(createVideoPostProcess(YAMI_VPP_OCL_BLENDER), releaseVideoPostProcess);
            if (!m_blender || m_blender->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
                return false;
        }

        if (m_bOsd) {
            m_osd.reset(createVideoPostProcess(YAMI_VPP_OCL_OSD), releaseVideoPostProcess);
            if (!m_osd || m_osd->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
                return false;
            VppParamOsd osdParam;
            osdParam.size = sizeof(VppParamOsd);
            osdParam.threshold = m_osdThreshold;
            m_osd->setParameters(VppParamTypeOsd, &osdParam);
        }

        if (m_bMosaic) {
            m_mosaic.reset(createVideoPostProcess(YAMI_VPP_OCL_MOSAIC), releaseVideoPostProcess);
            if (!m_mosaic || m_mosaic->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
                return false;
            VppParamMosaic mosaicParam;
            mosaicParam.size = sizeof(VppParamMosaic);
            mosaicParam.blockSize = m_mosaicSize;
            m_mosaic->setParameters(VppParamTypeMosaic, &mosaicParam);
        }

        if (m_flipRot) {
            m_transform.reset(createVideoPostProcess(YAMI_VPP_OCL_TRANSFORM), releaseVideoPostProcess);
            if (!m_transform || m_transform->setNativeDisplay(nativeDisplay) != YAMI_SUCCESS)
                return false;
            VppParamTransform transformParam;
            transformParam.size = sizeof(VppParamTransform);
            transformParam.transform = m_flipRot;
            m_transform->setParameters(VppParamTypeTransform, &transformParam);
        }

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

    bool process(SharedPtr<IVideoPostProcess>& vpp,
        const char* type,
        vector<SharedPtr<VideoFrame> >& srcSurfaces,
        vector<SharedPtr<BumpBox> >& srcBumpBoxes,
        SharedPtr<VideoFrame>& dstSurface)
    {
        PERF_START(type);
        for (uint32_t i = 0; i < srcSurfaces.size(); i++) {
            srcBumpBoxes[i]->getPos(dstSurface->crop.x, dstSurface->crop.y, dstSurface->crop.width, dstSurface->crop.height);
            //printf("(%d, %d, %d, %d)\r\n", dstSurface->crop.x, dstSurface->crop.y, dstSurface->crop.width, dstSurface->crop.height);

            vpp->process(srcSurfaces[i], m_dest);
        }
        PERF_STOP(type);

        return true;
    }

    SharedPtr<Display> m_display;
    SharedPtr<NativeDisplay> m_nativeDisplay;
    SharedPtr<VADisplay> m_vaDisplay;
    Window   m_window;
    SharedPtr<VppInputDecode> m_input;
    SharedPtr<IVideoPostProcess> m_scaler;
    SharedPtr<IVideoPostProcess> m_blender;
    SharedPtr<IVideoPostProcess> m_osd;
    SharedPtr<IVideoPostProcess> m_mosaic;
    SharedPtr<IVideoPostProcess> m_transform;
    int m_width, m_height;
    vector<SharedPtr<VideoFrame> > m_blendSurfaces;
    vector<SharedPtr<BumpBox> > m_blendBumpBoxes;
    vector<SharedPtr<VideoFrame> > m_osdSurfaces;
    vector<SharedPtr<BumpBox> > m_osdBumpBoxes;
    vector<SharedPtr<BumpBox> > m_mosaicBumpBoxes;
    SharedPtr<VideoFrame> m_dest;
    SharedPtr<VideoFrame> m_displaySurface;
    bool m_bBlend;
    bool m_bOsd;
    bool m_bMosaic;
    int m_osdThreshold;
    int m_mosaicSize;
    uint32_t m_flipRot;
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

