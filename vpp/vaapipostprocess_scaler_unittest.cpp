/*
 * Copyright 2016 Intel Corporation
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

//
// The unittest header must be included before va_x11.h (which might be included
// indirectly).  The va_x11.h includes Xlib.h and X.h.  And the X headers
// define 'Bool' and 'None' preprocessor types.  Gtest uses the same names
// to define some struct placeholders.  Thus, this creates a compile conflict
// if X defines them before gtest.  Hence, the include order requirement here
// is the only fix for this right now.
//
// See bug filed on gtest at https://github.com/google/googletest/issues/371
// for more details.
//
#include "common/factory_unittest.h"

// primary header
#include "vaapipostprocess_scaler.h"

#include "common/log.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/VaapiUtils.h"

namespace YamiMediaCodec {

typedef void (*VppTestFunction)(VaapiPostProcessScaler& scaler,
    const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest);

struct FrameDestroyer {
    FrameDestroyer(DisplayPtr display)
        : m_display(display)
    {
    }
    void operator()(VideoFrame* frame)
    {
        VASurfaceID id = (VASurfaceID)frame->surface;
        checkVaapiStatus(vaDestroySurfaces(m_display->getID(), &id, 1),
            "vaDestroySurfaces");
        delete frame;
    }

private:
    DisplayPtr m_display;
};

//TODO: refact it with decoder base
SharedPtr<VideoFrame> createVideoFrame(const DisplayPtr& display, uint32_t width, uint32_t height, uint32_t fourcc)
{
    SharedPtr<VideoFrame> frame;
    VASurfaceAttrib attrib;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = fourcc;
    uint32_t rtformat;
    if (fourcc == VA_FOURCC_BGRX
        || fourcc == VA_FOURCC_BGRA) {
        rtformat = VA_RT_FORMAT_RGB32;
        ERROR("rgb32");
    }
    else {
        rtformat = VA_RT_FORMAT_YUV420;
    }

    VASurfaceID id;
    VAStatus status = vaCreateSurfaces(display->getID(), rtformat, width, height,
        &id, 1, &attrib, 1);
    if (status != VA_STATUS_SUCCESS) {
        ERROR("create surface failed fourcc = %d", fourcc);
        return frame;
    }
    frame.reset(new VideoFrame, FrameDestroyer(display));
    memset(frame.get(), 0, sizeof(VideoFrame));
    frame->surface = (intptr_t)id;
    return frame;
}

class VaapiPostProcessScalerTest
    : public FactoryTest<IVideoPostProcess, VaapiPostProcessScaler> {
public:
    static void checkFilter(const VaapiPostProcessScaler& scaler, VppParamType type)
    {
        if (type == VppParamTypeDenoise) {
            EXPECT_TRUE(bool(scaler.m_denoise.filter));
        }
        else if (type == VppParamTypeSharpening) {
            EXPECT_TRUE(bool(scaler.m_sharpening.filter));
        }
    }

    static void checkColorBalanceFilter(VaapiPostProcessScaler& scaler, VppColorBalanceMode mode)
    {
        if (COLORBALANCE_NONE != mode)
            EXPECT_TRUE(bool(scaler.m_colorBalance[mode].filter));
    }

protected:
    /* invoked by gtest before the test */
    virtual void SetUp()
    {
        return;
    }

    /* invoked by gtest after the test */
    virtual void TearDown()
    {
        return;
    }

    static void checkFilterCaps(VaapiPostProcessScaler& scaler,
        VAProcFilterType filterType,
        int32_t minLevel, int32_t maxLevel, int32_t level)
    {

        SharedPtr<VAProcFilterCap> caps;
        float value;

        EXPECT_TRUE(scaler.mapToRange(value, level, minLevel, maxLevel, filterType, caps));
        ASSERT_TRUE(bool(caps));
        EXPECT_LT(caps->range.min_value, caps->range.max_value);
        EXPECT_LE(caps->range.min_value, value);
        EXPECT_LE(value, caps->range.max_value);
    }

    static void checkMapToRange()
    {
        float value;
        VaapiPostProcessScaler scaler;
        EXPECT_TRUE(scaler.mapToRange(value, 0, 64, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX));
        EXPECT_FLOAT_EQ(0, value);
        EXPECT_TRUE(scaler.mapToRange(value, 0, 64, DENOISE_LEVEL_MAX, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX));
        EXPECT_FLOAT_EQ(64, value);
        EXPECT_FALSE(scaler.mapToRange(value, 0, 64, DENOISE_LEVEL_NONE, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX));

        NativeDisplay nDisplay;
        memset(&nDisplay, 0, sizeof(nDisplay));
        ASSERT_EQ(YAMI_SUCCESS, scaler.setNativeDisplay(nDisplay));

        checkFilterCaps(scaler, VAProcFilterNoiseReduction, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX, DENOISE_LEVEL_MIN);
        checkFilterCaps(scaler, VAProcFilterNoiseReduction, DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX, DENOISE_LEVEL_MAX);

        checkFilterCaps(scaler, VAProcFilterSharpening, SHARPENING_LEVEL_MIN, SHARPENING_LEVEL_MAX, SHARPENING_LEVEL_MIN);
        checkFilterCaps(scaler, VAProcFilterSharpening, SHARPENING_LEVEL_MIN, SHARPENING_LEVEL_MAX, SHARPENING_LEVEL_MAX);
    }

    static void vppTest(VppTestFunction func)
    {
        NativeDisplay nDisplay;
        memset(&nDisplay, 0, sizeof(nDisplay));

        VaapiPostProcessScaler scaler;
        ASSERT_EQ(YAMI_SUCCESS, scaler.setNativeDisplay(nDisplay));

        SharedPtr<VideoFrame> src = createVideoFrame(scaler.m_display, 320, 240, YAMI_FOURCC_NV12);
        SharedPtr<VideoFrame> dest = createVideoFrame(scaler.m_display, 480, 272, YAMI_FOURCC_NV12);
        ASSERT_TRUE(src && dest);
        func(scaler, src, dest);
    }
};

#define VAAPIPOSTPROCESS_SCALER_TEST(name) \
    TEST_F(VaapiPostProcessScalerTest, name)

VAAPIPOSTPROCESS_SCALER_TEST(Factory)
{
    FactoryKeys mimeTypes;
    mimeTypes.push_back(YAMI_VPP_SCALER);
    doFactoryTest(mimeTypes);
}

VAAPIPOSTPROCESS_SCALER_TEST(mapToRange)
{
    checkMapToRange();
}

template <class P>
void VppSetParams(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest,
    P& params, int32_t& value, int32_t min, int32_t max, int32_t none, VppParamType type)
{
    memset(&params, 0, sizeof(P));
    params.size = sizeof(P);
    value = min;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(type, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    value = max;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(type, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    value = none;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(type, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    value = (min + max) / 2;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(type, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));
    VaapiPostProcessScalerTest::checkFilter(scaler, type);
}

void VppColorBalanceSetParams(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest,
    VppColorBalanceMode mode)
{
    VPPColorBalanceParameter params;
    memset(&params, 0, sizeof(params));
    params.size = sizeof(params);

    if (COLORBALANCE_COUNT == mode) {
        params.mode = mode;
        EXPECT_EQ(YAMI_UNSUPPORTED, scaler.setParameters(VppParamTypeColorBalance, &params));
        return;
    }

    if (COLORBALANCE_NONE == mode) {
        params.mode = mode;
        EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeColorBalance, &params));
        EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));
        return;
    }

    params.mode = mode;
    params.level = COLORBALANCE_LEVEL_MIN;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeColorBalance, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    params.mode = mode;
    params.level = COLORBALANCE_LEVEL_MAX;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeColorBalance, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    params.mode = mode;
    params.level = COLORBALANCE_LEVEL_NONE;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeColorBalance, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    params.mode = mode;
    params.level = COLORBALANCE_LEVEL_MIN - 10;
    EXPECT_EQ(YAMI_DRIVER_FAIL, scaler.setParameters(VppParamTypeColorBalance, &params));

    params.mode = mode;
    params.level = COLORBALANCE_LEVEL_MAX + 10;
    EXPECT_EQ(YAMI_DRIVER_FAIL, scaler.setParameters(VppParamTypeColorBalance, &params));

    params.mode = mode;
    params.level = (COLORBALANCE_LEVEL_MIN + COLORBALANCE_LEVEL_MAX) / 2;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeColorBalance, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    VaapiPostProcessScalerTest::checkColorBalanceFilter(scaler, mode);
}

void VppDenoise(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest)
{
    VPPDenoiseParameters params;
    VppSetParams(scaler, src, dest, params, params.level,
        DENOISE_LEVEL_MIN, DENOISE_LEVEL_MAX, DENOISE_LEVEL_NONE, VppParamTypeDenoise);
}

VAAPIPOSTPROCESS_SCALER_TEST(Denoise)
{
    vppTest(VppDenoise);
}

void VppSharpening(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest)
{
    VPPSharpeningParameters params;
    VppSetParams(scaler, src, dest, params, params.level,
        SHARPENING_LEVEL_MIN, SHARPENING_LEVEL_MAX, SHARPENING_LEVEL_NONE, VppParamTypeSharpening);
}

VAAPIPOSTPROCESS_SCALER_TEST(Sharpening)
{
    vppTest(VppSharpening);
}

void VppDeinterlace(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest)
{
    VPPDeinterlaceParameters params;
    memset(&params, 0, sizeof(params));
    params.size = sizeof(params);

    params.mode = DEINTERLACE_MODE_BOB;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeDeinterlace, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    params.mode = DEINTERLACE_MODE_NONE;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeDeinterlace, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));

    params.mode = DEINTERLACE_MODE_WEAVE;
    EXPECT_EQ(YAMI_SUCCESS, scaler.setParameters(VppParamTypeDeinterlace, &params));
    EXPECT_EQ(YAMI_SUCCESS, scaler.process(src, dest));
}

VAAPIPOSTPROCESS_SCALER_TEST(Deinterlace)
{
    vppTest(VppDeinterlace);
}

void VppColorBalance(VaapiPostProcessScaler& scaler, const SharedPtr<VideoFrame>& src, const SharedPtr<VideoFrame>& dest)
{
    VppParamWireframe wireFrameParams;
    EXPECT_EQ(YAMI_INVALID_PARAM, scaler.setParameters(VppParamTypeColorBalance, &wireFrameParams));

    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_NONE);
    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_HUE);
    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_SATURATION);
    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_BRIGHTNESS);
    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_CONTRAST);
    VppColorBalanceSetParams(scaler, src, dest, COLORBALANCE_COUNT);
}

VAAPIPOSTPROCESS_SCALER_TEST(Colorbalance)
{
    vppTest(VppColorBalance);
}
}
