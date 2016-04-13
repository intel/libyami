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


#include "common/log.h"
#include "common/utils.h"
#include "tests/vppinputdecode.h"
#include "tests/vppinputasync.h"
#include "interface/VideoPostProcessHost.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <string.h>


using namespace YamiMediaCodec;
using namespace std;

#include <errno.h>
extern "C" {
#include <libdrm/drm_fourcc.h>
#include <libdrm/intel_bufmgr.h>
}
#include <va/va_drmcommon.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <map>
#include <sstream>
#include "vaapi/VaapiUtils.h"
#include "common/common_def.h"
#include "common/condition.h"
#include "common/lock.h"

using namespace std;

bool checkDrmRet(int ret,const char* msg)
{
    if (ret) {
        ERROR("%s failed: %s", msg, strerror(errno));
        return false;
    }
    return true;
}
//this is an onscreen RGB Surface.
//drm function uses getFbHandle
//yami uses SharedPtr<VideoFrame>
//but it all bind to same memory.
class DrmFrame : public VideoFrame
{
public:
    DrmFrame(VADisplay, int fd, uint32_t width, uint32_t height);
    ~DrmFrame();
    bool init();
    uint32_t getFbHandle();
private:
    bool createBo();
    //add current bo to frame buffer
    bool addToFb();
    //bind current bo to VA Surface
    bool bindToVaSurface();
    VADisplay m_display;
    int m_fd;
    uint32_t m_width;
    uint32_t m_height;
    int m_bo;
    uint32_t m_handle;
    uint32_t m_pitch;
    static const uint32_t BPP = 32;
};

DrmFrame::DrmFrame(VADisplay display, int fd, uint32_t width, uint32_t height)
    :m_display(display), m_fd(fd), m_width(width),m_height(height),m_bo(-1), m_handle(-1)
{
    //dirty but handy
    VideoFrame* frame = static_cast<VideoFrame*>(this);
    memset(frame, 0, sizeof(VideoFrame));
    frame->fourcc = YAMI_FOURCC_RGBX;
    frame->surface = static_cast<intptr_t>(VA_INVALID_ID);
}

bool DrmFrame::createBo()
{
    struct drm_mode_create_dumb arg;
    memset(&arg, 0, sizeof(arg));
    arg.bpp = BPP;
    arg.width = m_width,
    arg.height = m_height;
    int ret = drmIoctl(m_fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
    if (!checkDrmRet(ret, "DRM_IOCTL_MODE_CREATE_DUMB"))
        return false;
    m_bo = arg.handle;
    m_pitch = arg.pitch;
    return true;
}

bool DrmFrame::addToFb()
{
    int ret = drmModeAddFB(m_fd, m_width, m_height, 24,
            BPP, m_pitch, m_bo, &m_handle);
    return checkDrmRet(ret, "drmModeAddFB");
}

//bind m_bo to VaSurface.
//vpp need this.
bool DrmFrame::bindToVaSurface()
{
    int ret;

    struct drm_gem_flink arg;
    memset(&arg, 0, sizeof(arg));
    arg.handle = m_bo;
    ret = drmIoctl(m_fd, DRM_IOCTL_GEM_FLINK, &arg);
    if (!checkDrmRet(ret, "DRM_IOCTL_PRIME_HANDLE_TO_FD"))
        return false;

    VASurfaceAttribExternalBuffers external;
    unsigned long handle = (unsigned long)arg.name;
    memset(&external, 0, sizeof(external));
    external.pixel_format = VA_FOURCC_BGRX;
    external.width = m_width;
    external.height = m_height;
    external.data_size = m_width * m_height * BPP / 8;
    external.num_planes = 1;
    external.pitches[0] = m_pitch;
    external.buffers = &handle;
    external.num_buffers = 1;

    VASurfaceAttrib attribs[2];
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].type = VASurfaceAttribMemoryType;
    attribs[0].value.type = VAGenericValueTypeInteger;
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_KERNEL_DRM;

    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].type = VASurfaceAttribExternalBufferDescriptor;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = &external;

    VASurfaceID id;
    VAStatus vaStatus = vaCreateSurfaces(m_display, VA_RT_FORMAT_RGB32, m_width, m_height,
                                           &id, 1,attribs, N_ELEMENTS(attribs));
    if (!checkVaapiStatus(vaStatus, "vaCreateSurfaces"))
        return false;
    this->surface = static_cast<intptr_t>(id);
    return true;
}

bool DrmFrame::init()
{
    return createBo() && addToFb() && bindToVaSurface();
}

uint32_t DrmFrame::getFbHandle()
{
    return m_handle;
}


DrmFrame::~DrmFrame()
{
    int ret;
    VASurfaceID id = static_cast<VASurfaceID>(this->surface);
    if (id != VA_INVALID_ID) {
        checkVaapiStatus(vaDestroySurfaces(m_display, &id, 1), "vaDestroySurfaces");
    }
    if (m_handle != (uint32_t)-1) {
        ret = drmModeRmFB(m_fd, m_handle);
        checkDrmRet(ret, "drmModeRmFB");
    }
    if (m_bo != -1) {
        drm_mode_destroy_dumb arg;
        memset(&arg, 0, sizeof(arg));
        arg.handle = m_bo;
        ret = drmIoctl(m_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &arg);
        checkDrmRet(ret, "DRM_IOCTL_MODE_DESTROY_DUMB");
    }
}

class DrmRenderer
{
    typedef std::deque<SharedPtr<DrmFrame> > FrameQueue;
    class Flipper {
    public:
        Flipper(int fd, uint32_t crtcID,
            FrameQueue& fronts, FrameQueue& backs, SharedPtr<DrmFrame>& current,
            Condition&, Lock&);
        ~Flipper();
        bool init();

    private:
        //thread loop
        static void* start(void*);
        void loop();

        //flip buffer and wait it done
        bool flip_l();

        int        m_fd;
        uint32_t   m_crtcID;

        //all protected by m_lock;
        FrameQueue& m_fronts;
        FrameQueue& m_backs;
        SharedPtr<DrmFrame>& m_current;
        bool       m_quit;

        Condition& m_cond;
        Lock&      m_lock;

        pthread_t  m_thread;
    };
public:
    ///init the render;
    bool init();
    ///deque the the screen back buffer, this give ownship to caller
    ///you can render on it.
    SharedPtr<VideoFrame> dequeue();

    ///queue the frame, this will transfer owner ship to renderer.
    ///it will flip the frame to screen
    bool queue(const SharedPtr<VideoFrame>&);

    //you dequeued the frame but did not draw validate data on it
    //this will transfer owner ship to renderer.
    //it will return to back buffers.
    bool discard(const SharedPtr<VideoFrame>&);

    //wait until all frames flipped.
    void flush();

    uint32_t getWidth();
    uint32_t getHeight();


    DrmRenderer(VADisplay, int fd, int displayIdx);
    ~DrmRenderer();
private:
    bool createFrames(uint32_t width, uint32_t height, int size);
    bool createFlipper();
    bool getConnector(drmModeRes *resource);
    bool getCrtc(drmModeRes *resource);
    bool getPlane();
    bool setPlane();
    bool initDrm();

    VADisplay m_display;
    int m_fd;
    int m_displayIdx;
    uint32_t m_connectorID;
    uint32_t m_encoderID;
    uint32_t m_crtcID;
    uint32_t m_crtcIndex;
    uint32_t m_planeID;
    drmModeModeInfo m_mode;

    Condition m_cond;
    Lock      m_lock;
    FrameQueue m_backs;
    FrameQueue m_fronts;
    SharedPtr<DrmFrame> m_current;
    SharedPtr<Flipper>    m_flipper;
    int m_frameCount;
};

DrmRenderer::Flipper::Flipper(
    int fd, uint32_t crtcID,
    FrameQueue & fronts, FrameQueue & backs, SharedPtr<DrmFrame> & current,
    Condition &cond, Lock &lock)
    :m_fd(fd), m_crtcID(crtcID),
     m_fronts(fronts),m_backs(backs), m_current(current), m_quit(false),
     m_cond(cond), m_lock(lock),
     m_thread(-1)
{

}

DrmRenderer::Flipper::~Flipper()
{
    {
        AutoLock lock(m_lock);
        m_quit = true;
        m_cond.signal();
    }
    if (m_thread != (uint32_t)-1)
        pthread_join(m_thread, NULL);
}

bool DrmRenderer::Flipper::init()
{
    if (pthread_create(&m_thread, NULL, start, this)) {
        ERROR("create thread failed");
        return false;
    }
    return true;
}

bool DrmRenderer::Flipper::flip_l()
{
    //start a flip.
    SharedPtr<DrmFrame>& frame = m_fronts.front();
    uint32_t handle = frame->getFbHandle();
    int ret = drmModePageFlip(m_fd, m_crtcID, handle, DRM_MODE_PAGE_FLIP_EVENT, NULL);
    checkDrmRet(ret, "drmModePageFlip");

    //wait it done
    drmEventContext evctx;
    struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
    fd_set fds;
    memset(&evctx, 0, sizeof evctx);
    evctx.version = DRM_EVENT_CONTEXT_VERSION;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    //we hold the lock, release it here to
    m_lock.release();
    select(m_fd + 1, &fds, NULL, NULL, &timeout);
    drmHandleEvent(m_fd, &evctx);
    m_lock.acquire();
    if (m_current)
        m_backs.push_back(m_current);
    m_current = m_fronts.front();
    m_fronts.pop_front();
    m_cond.signal();

    return true;
}

void* DrmRenderer::Flipper::start(void* flipper)
{
    Flipper* f = (Flipper*)flipper;
    f->loop();
    return NULL;
}

void DrmRenderer::Flipper::loop()
{
    while (1) {
        AutoLock lock(m_lock);
        if (m_fronts.empty()) {
            if (m_quit)
                break;
            m_cond.wait();
        } else {
            flip_l();
        }
    }
}

DrmRenderer::DrmRenderer(VADisplay display, int fd, int displayIdx)
    :m_display(display), m_fd(fd), m_displayIdx(displayIdx), m_cond(m_lock), m_frameCount(0)
{
}

bool DrmRenderer::createFrames(uint32_t width, uint32_t height, int size)
{
    for (int i = 0; i < size; i++) {
        SharedPtr<DrmFrame> frame(new DrmFrame(m_display, m_fd, width, height));
        if (!frame->init())
            return false;
        m_backs.push_back(frame);
    }
    m_frameCount = size;
    return true;
}

uint32_t DrmRenderer::getWidth()
{
    return m_mode.hdisplay;
}

uint32_t DrmRenderer::getHeight()
{
    return m_mode.vdisplay;
}

bool DrmRenderer::getConnector(drmModeRes *resource)
{
    drmModeConnectorPtr connector = NULL;
    int idx = 0;
    for(int i = 0; i < resource->count_connectors; i++) {
        connector = drmModeGetConnector(m_fd, resource->connectors[i]);
        if(connector) {
            if (connector->connection == DRM_MODE_CONNECTED) {
                idx++;
                if (idx == m_displayIdx) {
                    m_connectorID = connector->connector_id;
                    m_encoderID = connector->encoder_id;
                    m_mode  = *connector->modes;
                    drmModeFreeConnector(connector);
                    return true;
                }
            }
            drmModeFreeConnector(connector);
        }
    }
    ERROR("target display %d, but you have only %d conntected display", m_displayIdx, idx);
    return false;
}

bool DrmRenderer::getCrtc(drmModeRes *resource)
{
    bool ret = false;
    drmModeEncoderPtr encoder = drmModeGetEncoder(m_fd, m_encoderID);
    if (encoder) {
        m_crtcID = encoder->crtc_id;
        for (int i = 0; i < resource->count_crtcs; i++) {
            if (resource->crtcs[i] == m_crtcID) {
                m_crtcIndex = i;
                ret = true;
                break;
            }
        }
        drmModeFreeEncoder(encoder);
    } else {
        ERROR("connect get encoder for id %d", m_encoderID);
    }
    return ret;
}

bool DrmRenderer::getPlane()
{
    drmModePlaneResPtr planes = drmModeGetPlaneResources(m_fd);
    if (!planes) {
        return false;
    }
    for (uint32_t i = 0; i < planes->count_planes; i++) {
        drmModePlanePtr plane = drmModeGetPlane(m_fd, planes->planes[i]);
        if (plane) {
            if (plane->possible_crtcs & (1 << m_crtcIndex)) {
                for (uint32_t j = 0; j < plane->count_formats; j++) {
                    if (plane->formats[j] == DRM_FORMAT_XRGB8888) {
                        m_planeID = plane->plane_id;
                        drmModeFreePlane(plane);
                        drmModeFreePlaneResources(planes);
                        return true;
                    }
                }
            }
            drmModeFreePlane(plane);
        }
    }
    drmModeFreePlaneResources(planes);
    return false;
}

bool DrmRenderer::setPlane()
{
    AutoLock lock(m_lock);
    SharedPtr<DrmFrame> frame = m_backs.front();
    uint32_t handle = frame->getFbHandle();
    int ret;
    ret = drmModeSetCrtc(m_fd, m_crtcID, handle, 0, 0, &m_connectorID, 1, &m_mode);
    if (!checkDrmRet(ret, "drmModeSetCrtc"))
        return false;

    m_backs.pop_front();
    m_current = frame;
    return true;
}

bool DrmRenderer::createFlipper()
{
    m_flipper.reset(new Flipper(m_fd, m_crtcID, m_fronts, m_backs, m_current, m_cond, m_lock));
    if (!m_flipper->init())
        return false;
    return true;
}

#define DEFAULT_DRM_BATCH_SIZE 0x80000
bool DrmRenderer::initDrm()
{
    bool ret = false;

    drmModeRes *resource = drmModeGetResources(m_fd);
    if (resource) {
        if (getConnector(resource) && getCrtc(resource) && getPlane()) {
            ret = true;
        }
        drmModeFreeResources(resource);
    }
    return ret;
}

bool DrmRenderer::init()
{
    if (!initDrm())
        return false;
    if (!createFrames(m_mode.hdisplay, m_mode.vdisplay, 5) || !setPlane() || !createFlipper())
        return false;
    ERROR("%dx%d@%d", m_mode.hdisplay, m_mode.vdisplay, m_mode.vrefresh);
    return true;
}

SharedPtr<VideoFrame> DrmRenderer::dequeue()
{
    AutoLock lock(m_lock);
    SharedPtr<VideoFrame> frame;
    while (m_backs.empty())
        m_cond.wait();
    frame = std::tr1::static_pointer_cast<VideoFrame>(m_backs.front());
    m_backs.pop_front();
    return frame;
}

bool DrmRenderer::queue(const SharedPtr<VideoFrame>& vframe)
{
    SharedPtr<DrmFrame> frame =
        std::tr1::static_pointer_cast<DrmFrame>(vframe);
    if (!frame) {
        ERROR("invalid frame queued");
        return false;
    }
    AutoLock lock(m_lock);
    m_fronts.push_back(frame);
    m_cond.signal();
    return true;
}

bool DrmRenderer::discard(const SharedPtr<VideoFrame>& vframe)
{
    SharedPtr<DrmFrame> frame =
        std::tr1::static_pointer_cast<DrmFrame>(vframe);
    if (!frame) {
        ERROR("invalid frame queued");
        return false;
    }
    AutoLock lock(m_lock);
    m_backs.push_back(frame);
    //no need signal
    return true;
}

void DrmRenderer::flush()
{
    AutoLock lock(m_lock);
    while (!m_fronts.empty())
        m_cond.wait();
}

DrmRenderer::~DrmRenderer()
{
    m_flipper.reset();
    int count = m_backs.size() + m_fronts.size();
    if (m_current)
        count++;
    if (m_frameCount != count) {
        ASSERT(0 && "!BUG, you need return all dequeued buffer before you destory render");
    }
}

void usage(const char* app) {
        printf("%s: a tool to display MxN ways decode output in a grid\n", app);
        printf("usage: %s <options> file1 [file2] ...\n", app);
        printf("   -c <column> \n");
        printf("   -r <row> \n");
        printf("   -d <index>, target display index, start from 1 \n");
        printf("   -s, put vpp and decode in single thread \n");
        printf("   -g, <grid command line> create other grid instance  \n");
        printf("       example: grid a.mp4 -g \"b.mp4 -d 2\"\n");
        printf("       it will render a.mp4 in first display and b.mp4 in second\n");
}

class Grid
{
    class Arg
    {
    public:
        Arg()
        {
        }
        ~Arg()
        {
            for (size_t i = 0; i < m_argv.size(); i++)
                free(m_argv[i]);
        }
        void init (const string& args)
        {
            istringstream is(args);
            append("grid");
            while (is) {
                string tmp;
                is>>tmp;
                if (tmp.size()) {
                    append(tmp.c_str());
                }
            }
        }
        int getArgc()
        {
            return (int)m_argv.size();
        }
        char** getArgv()
        {
            return &m_argv[0];
        }
    private:
        void append(const char* param)
        {
            m_argv.push_back(strdup(param));
        }

        vector<char*> m_argv;
    };
public:
    bool init(const string& args)
    {
         m_arg.init(args);
         return init(m_arg.getArgc(), m_arg.getArgv());
    }
    bool run()
    {
        if (pthread_create(&m_vppThread, NULL, start, this)) {
            ERROR("create thread failed");
            return false;
        }
        return true;
    }
    Grid(int fd, const SharedPtr<NativeDisplay>& nativeDisplay):m_fd(fd), m_nativeDisplay(nativeDisplay),
        m_vaDisplay((VADisplay)nativeDisplay->handle),
        m_width(0), m_height(0), m_col(0), m_row(0),
        m_displayIdx(1), m_singleThread(false), m_vppThread(-1){}

    ~Grid()
    {
        if (m_vppThread != (unsigned long)-1)
            pthread_join(m_vppThread, NULL);
        m_renderer.reset();
        m_vpp.reset();
        m_inputs.clear();
    }
private:
    bool init(int argc, char** argv)
    {
        if (!processCmdline(argc, argv)) {
            usage("grid");
            return false;
        }

        int len = m_col * m_row;
        for (int i = 0; i < len; i++) {
            SharedPtr<VppInputDecode> decodeInput(new VppInputDecode);
            char* file = m_files[i % m_files.size()];
            if (!decodeInput->init(file)) {
                fprintf(stderr, "failed to open %s\n", file);
                return false;
            }
            //set native display
            if (!decodeInput->config(*m_nativeDisplay)) {
                return false;
            }
            SharedPtr<VppInput> input;
            if (m_singleThread) {
                input = decodeInput;
            } else {
                input = VppInputAsync::create(decodeInput, 3);
                if (!input) {
                    ERROR("can't create async input");
                    return false;
                }
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

        SharedPtr<DrmRenderer> tmp(new DrmRenderer(m_vaDisplay, m_fd, m_displayIdx));
        if (!tmp->init()) {
            ERROR("init drm renderer failed");
            return false;
        }
        m_renderer = tmp;
        m_width = m_renderer->getWidth();
        m_height = m_renderer->getHeight();
        return true;
    }

    static void* start(void* grid)
    {
        Grid* g = (Grid*)grid;
        g->renderOutputs();
        return NULL;
    }
    void renderOutputs()
    {
        SharedPtr<VideoFrame> frame;
        FpsCalc fps;
        int width = m_width / m_col;
        int height = m_height / m_row;
        do {
            SharedPtr<VideoFrame> dest = m_renderer->dequeue();
            for (int i = 0; i < m_row; i++) {
                for (int j = 0; j < m_col; j++) {
                    SharedPtr<VppInput>& input = m_inputs[i * m_col + j];
                    if (!input->read(frame)) {
                        m_renderer->discard(dest);
                        m_renderer->flush();
                        goto DONE;
                    }
                    dest->crop.x = j * width;
                    dest->crop.y = i * height;
                    dest->crop.width = width;
                    dest->crop.height = height;
                    m_vpp->process(frame, dest);
                }
            }
            if (!m_renderer->queue(dest)) {
                ERROR("queue to drm failed");
                goto DONE;
            }

            fps.addFrame();
        } while (1);
DONE:
        printf("playback on display %d done\n", m_displayIdx);
        fps.log();
    }
    bool processCmdline(int argc, char** argv)
    {
        char opt;
        optind = 0;
        while ((opt = getopt(argc, argv, "c:r:d:s")) != -1)
        {
            switch (opt) {
                case 'c':
                    m_col = atoi(optarg);
                    break;
                case 'r':
                    m_row = atoi(optarg);
                    break;
                case 'd':
                    m_displayIdx = atoi(optarg);
                    break;
                case 's':
                    m_singleThread = true;
                    break;
                default:
                    return false;
            }
        }
        for (int i = optind; i < argc; i++) {
            m_files.push_back(argv[i]);
        }
        if (m_files.empty())
            return false;

        if (!m_col) {
            m_col = 4;
        }
        if (!m_row) {
            m_row = 4;
        }
        return true;
    }

    int m_fd;
    SharedPtr<NativeDisplay> m_nativeDisplay;
    VADisplay m_vaDisplay;

    vector< SharedPtr<VppInput> > m_inputs;
    SharedPtr<IVideoPostProcess> m_vpp;
    SharedPtr<DrmRenderer> m_renderer;
    uint32_t m_width, m_height;
    int m_col, m_row;
    int m_displayIdx;
    //put decode and vpp in single thread
    bool m_singleThread;
    vector<char*> m_files;
    pthread_t m_vppThread;
    Arg m_arg;
};

class App
{
public:
    bool init(int argc, char** argv)
    {
        if (!processCmdline(argc, argv)) {
            usage("grid");
            return false;
        }
        if (!initDisplay()) {
            fprintf(stderr, "init display failed");
            return false;
        }
        for (size_t i = 0; i < m_args.size(); i++) {
            string& arg = m_args[i];
            SharedPtr<Grid> grid(new Grid(m_fd, m_nativeDisplay));
            if (!grid->init(arg)) {
                return false;
            }
            m_grids.push_back(grid);
        }
        return true;
    }
    bool run()
    {
        for (size_t i = 0; i < m_grids.size(); i++) {
            SharedPtr<Grid>& grid = m_grids[i];
            if (!grid->run()) {
                ERROR("run grid %lu failed", i);
                return false;
            }
        }
        return true;
    }
    ~App()
    {
        //make sure we destory all grid instance before we destory va
        m_grids.clear();

        if (m_nativeDisplay) {
            vaTerminate(m_vaDisplay);
        }
        if (m_fd != 0) {
            close(m_fd);
        }
    }
private:

    bool initDisplay()
    {
        m_fd = open("/dev/dri/card0", O_RDWR);
        if (m_fd == -1) {
            fprintf(stderr, "Failed to open card0 \n");
            return false;
        }
        m_vaDisplay = vaGetDisplayDRM(m_fd);
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

    void append(string& cmd, char* arg)
    {
        cmd += arg;
        cmd += ' ';
    }
    bool processCmdline(int argc, char** argv)
    {
        //input param not in -g option.
        string cmd;

        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-g") == 0) {
                i++;
                if (i == argc)
                    return false;
                string tmp = argv[i];
                m_args.push_back(tmp);
            } else {
                append(cmd, argv[i]);
            }
        }
        if (cmd.size()) {
            m_args.push_back(cmd);
        }
        return m_args.size();
    }

    int m_fd;
    SharedPtr<NativeDisplay> m_nativeDisplay;
    VADisplay m_vaDisplay;

    vector<string> m_args;
    vector< SharedPtr<Grid> > m_grids;
};

int main(int argc, char** argv)
{
    App app;
    if (!app.init(argc, argv)) {
        ERROR("init grid app failed");
        return -1;
    }
    if (!app.run()){
        ERROR("run grid failed");
        return -1;
    }

    return  0;

}
