/*
 *  decodehelp.h - decode test help
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Halley Zhao<halley.zhao@intel.com>
 *             Xin Tang<xin.t.tang@intel.com>
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
#ifndef __DECODE_HELP__
#define __DECODE_HELP__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cpluscplus
extern "C" {
#endif

#include "interface/VideoDecoderDefs.h"
#ifdef __ENABLE_CAPI__
#include "capi/VideoDecoderCapi.h"
#else
#include "interface/VideoDecoderHost.h"
#endif
#include "common/log.h"
#include <unistd.h>

#ifndef VA_FOURCC_I420
#define VA_FOURCC_I420 VA_FOURCC('I','4','2','0')
#endif
static char *dumpOutputDir = NULL;
static uint32_t dumpFourcc = VA_FOURCC_I420;
static int renderMode = 1;
static bool waitBeforeQuit = false;
static char *inputFileName = NULL;
static FILE * outFile = NULL;
#ifdef __ENABLE_X11__
static Display *x11Display = NULL;
static Window window = 0;
#endif
static int64_t timeStamp = 0;
static int32_t videoWidth = 0, videoHeight = 0;
static bool useSoftNV12toI420 = 1;
static char *I420buf = NULL;
static int curWidth = 0;
static int curHeight = 0;
#if __ENABLE_TESTS_GLES__
#include "egl/gles2_help.h"
#include "egl/egl_util.h"
static EGLContextType *eglContext = NULL;
XID pixmap = 0;
static GLuint textureId = 0;
#endif

#if __ENABLE_CAPI__
#define DECODE_GetOutput(decoder, frame, drain) decodeGetOutputRawData(decoder, &frame, drain)
#else
#define DECODE_GetOutput(decoder, frame, drain) decoder->getOutput(&frame, drain)
#endif

static void print_help(const char* app)
{
    printf("%s <options>\n", app);
    printf("   -i media file to decode\n");
    printf("   -w wait before quit\n");
    printf("   -f dumped fourcc\n");
    printf("   -o dumped output dir\n");
    printf("   -m <render mode>\n");
    printf("      0: dump video frame to file\n");
    printf("      1: render to X window\n");
    printf("      2: texture: render to Pixmap + texture from Pixmap\n");
    printf("      3: texture: export video frame as drm name (RGBX) + texture from drm name\n");
    printf("      4: texture: export video frame as dma_buf(RGBX) + texutre from dma_buf\n");
    printf("      5: texture: export video frame as dma_buf(NV12) + texture from dma_buf\n");
}

static bool process_cmdline(int argc, char *argv[])
{
    char opt;

    while ((opt = getopt(argc, argv, "h:m:i:f:o:w?")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return false;
        case 'i':
            inputFileName = optarg;
            break;
        case 'w':
            waitBeforeQuit = true;
            break;
        case 'm':
            renderMode = atoi(optarg);
            break;
        case 'f':
            if (strlen(optarg) == 4) {
                dumpFourcc = VA_FOURCC(optarg[0], optarg[1], optarg[2], optarg[3]);
            } else {
                fprintf(stderr, "invalid fourcc: %s\n", optarg);
            }
            break;
        case 'o':
            if (optarg)
                dumpOutputDir = strdup(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    if (!inputFileName) {
        fprintf(stderr, "no input media file specified\n");
        return -1;
    }
    fprintf(stderr, "input file: %s, renderMode: %d\n", inputFileName, renderMode);

    if (!dumpOutputDir)
        dumpOutputDir = strdup ("./");

#ifndef __ENABLE_TESTS_GLES__
    if (renderMode > 1) {
        fprintf(stderr, "renderMode=%d is not supported, please rebuild with --enable-tests-gles option\n", renderMode);
        return -1;
    }
#endif
    return true;
}

void  NV12toI420(VideoFrameRawData frame, int width, int height)
{
    int32_t uvWidth = (width+1)/2;
    int32_t uvHeight = (height+1)/2;
    int size = uvWidth * uvHeight * 2;
    if(curWidth!=width || curHeight!=height) {
        curWidth = width;
        curHeight = height;
        if(I420buf)
            free(I420buf);
        I420buf = (char*)malloc(size);
    }

    int row = 0, plane = 0, planeCount = 2;
    const char *data = NULL;
    int ret;
    int widths[2], heights[2];
    widths[0] = width;
    heights[0] = height;
    widths[1] = widths[0] % 2 ? widths[0]+1 : widths[0];
    heights[1] = (heights[0] +1 )/2;

    data = (char*) frame.handle + frame.offset[0];
    for (row = 0; row < heights[0]; row++) {
        ret = fwrite(data, widths[0], 1, outFile);
        ASSERT(ret == 1);
        data += frame.pitch[0];
    }
    int i=0,j=0,k=0;
    int usize = uvHeight*uvWidth;
    data = (char*) frame.handle + frame.offset[1];
    for (row = 0; row < heights[1]; row++) {
        for( i=0;i<widths[1];i++) {
            if(i%2) {// v data
                I420buf[usize + k++] = data[i];
            }
            else {// u data
                I420buf[j++] = data[i];
            }
        }
        data += frame.pitch[1];
    }
    fwrite(I420buf, size, 1, outFile);
}

#ifndef __ENABLE_CAPI__
bool renderOutputFrames(YamiMediaCodec::IVideoDecoder* decoder, bool drain = false)
#else
bool renderOutputFrames(DecodeHandler decoder, bool drain)
#endif
{
    Decode_Status status;
    VideoFrameRawData frame;
#ifdef __ENABLE_DEBUG__
    static int renderFrames = 0;
#endif

#if __ENABLE_X11__
    if (renderMode > 0 && !window)
        return false;
#endif

#if __ENABLE_TESTS_GLES__
    if (renderMode > 1 && !eglContext)
        eglContext = eglInit(x11Display, window, VA_FOURCC_RGBA);
#endif

    do {
        switch (renderMode) {
        case 0:
            frame.memoryType = VIDEO_DATA_MEMORY_TYPE_RAW_POINTER;
            if(useSoftNV12toI420)
                frame.fourcc = VA_FOURCC_NV12;
            else
                frame.fourcc = dumpFourcc;
            frame.width = 0;
            frame.height = 0;
            status = DECODE_GetOutput(decoder, frame, drain);
            if (status == RENDER_SUCCESS) {
                if (!outFile) {
                    uint32_t fourcc = 0;
                    if(useSoftNV12toI420)
                        fourcc = dumpFourcc;
                    else
                        fourcc = frame.fourcc;
                    char *ch = (char*)&fourcc;
                    char outFileName[256];
                    char* baseFileName = inputFileName;
                    char *s = strrchr(inputFileName, '/');
                    if (s)
                        baseFileName = s+1;

                    sprintf(outFileName, "%s/%s_%dx%d.%c%c%c%c", dumpOutputDir, baseFileName, frame.width, frame.height, ch[0], ch[1], ch[2], ch[3]);
                    DEBUG("outFileName: %s", outFileName);
                    outFile = fopen(outFileName, "w+");
                }
                ASSERT(outFile);

                int widths[3], heights[3];
                int planeCount = 0;
                /* XXX
                 * use the video resolution from DECODE_FORMAT_CHANGE, since vp8 changes resolution dynamically.
                 * the decent fix should happen in libyami by adding crop region to surface/image, and update in frame.width/height
                 */
                widths[0] = videoWidth; //frame.width;
                heights[0] = videoHeight; // frame.height;
                DEBUG("current frame: %dx%d", videoWidth, videoHeight);
                switch (dumpFourcc) {
                case VA_FOURCC_NV12: {
                    planeCount = 2;
                    widths[1] = widths[0] % 2 ? widths[0]+1 : widths[0];
                    heights[1] = (heights[0] +1 )/2;
                }
                break;
                case VA_FOURCC_I420: {
                    if(useSoftNV12toI420) {
                        NV12toI420(frame ,widths[0], heights[0]);
                    }else {
                        planeCount = 3;
                        widths[1] = (widths[0]+1) / 2;
                        heights[1] = (heights[0] +1)/2;
                        widths[2] = (widths[0]+1) / 2;
                        heights[2] = (heights[0] +1)/2;
                    }
                }
                break;
                default:
                    ASSERT(0);
                    break;
                }

                if(!useSoftNV12toI420) {
                  int plane, row, ret;
                  const char *data = NULL;

                  for (plane = 0; plane<planeCount; plane++){
                      data = (char*) frame.handle + frame.offset[plane];
                      for (row = 0; row < heights[plane]; row++) {
                          ret = fwrite(data, widths[plane], 1, outFile);
                          ASSERT(ret == 1);
                          data += frame.pitch[plane];
                      }
                  }
                }

#if __ENABLE_CAPI__
                renderDoneRawData(decoder, &frame);
#else
                decoder->renderDone(&frame);
#endif
            }
            break;
#if __ENABLE_X11__
        case 1:
#if __ENABLE_CAPI__
            status = decodeGetOutput_x11(decoder, window, &timeStamp, 0, 0, videoWidth, videoHeight, drain, -1, -1, -1, -1);
#else
            status = decoder->getOutput(window, &timeStamp, 0, 0, videoWidth, videoHeight, drain);
#endif
            break;
#endif
#if __ENABLE_TESTS_GLES__
        case 2:
            if (!pixmap) {
                int screen = DefaultScreen(x11Display);
                pixmap = XCreatePixmap(x11Display, DefaultRootWindow(x11Display), videoWidth, videoHeight, XDefaultDepth(x11Display, screen));
                XSync(x11Display, false);
                textureId = createTextureFromPixmap(eglContext, pixmap);
            }
#if __ENABLE_CAPI__
            status = decodeGetOutput_x11(decoder, pixmap, &timeStamp, 0, 0, videoWidth, videoHeight, drain);
#else
            status = decoder->getOutput(pixmap, &timeStamp, 0, 0, videoWidth, videoHeight, drain);
#endif
            if (status == RENDER_SUCCESS) {
                drawTextures(eglContext, &textureId, 1);
            }
            break;
        case 3:
        case 4:
        if (!textureId)
                glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);

            frame.memoryType = renderMode == 3 ? VIDEO_DATA_MEMORY_TYPE_DRM_NAME : VIDEO_DATA_MEMORY_TYPE_DMA_BUF;
            frame.fourcc = VA_FOURCC_BGRX; // VAAPI BGRA match MESA ARGB
            frame.width = videoWidth;
            frame.height = videoHeight;
            status = DECODE_GetOutput(decoder, frame, drain);
            if (status == RENDER_SUCCESS) {
                EGLImageKHR eglImage = EGL_NO_IMAGE_KHR;
                if (renderMode == 3)
                    eglImage = createEglImageFromDrmBuffer(eglContext->eglContext.display, eglContext->eglContext.context, frame.handle, frame.width, frame.height, frame.pitch[0]);
                else
                    eglImage = createEglImageFromDmaBuf(eglContext->eglContext.display, eglContext->eglContext.context, frame.handle, frame.width, frame.height, frame.pitch[0]);

                if (eglImage != EGL_NO_IMAGE_KHR) {
                    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, eglImage);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    drawTextures(eglContext, &textureId, 1);

                    eglDestroyImageKHR(eglContext->eglContext.display, eglImage);
                } else {
                    ERROR("fail to create EGLImage from dma_buf");
                }

                decoder->renderDone(&frame);
            }
            break;
#endif
        default:
            ASSERT(0);
            break;
        }
#ifdef __ENABLE_DEBUG__
        if (status == RENDER_SUCCESS)
            renderFrames++;
#endif
    } while (status != RENDER_NO_AVAILABLE_FRAME && status > 0);

#ifdef __ENABLE_DEBUG__
    DEBUG("renderFrames: %d", renderFrames);
#endif

    return true;
}

#endif
