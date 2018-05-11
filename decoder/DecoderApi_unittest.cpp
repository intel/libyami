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
#include "common/common_def.h"
#include "common/log.h"
#include "decoder/FrameData.h"
#include "vaapi/vaapidisplay.h"
#include "vaapi/vaapisurfaceallocator.h"
// primary header
#include "VideoDecoderHost.h"

// system headers
#include <algorithm>
#include <stdint.h>
#include <deque>

namespace YamiMediaCodec {

static FrameData g_h264data[] = {
    g_avc8x8I,
    g_avc8x8P,
    g_avc8x8B,
    g_avc8x16,
    g_avc16x16,
    g_avc8x8I,
    g_avc8x8P,
    g_avc8x8B,
    g_avc8x8I,
    g_avc8x8P,
    g_avc8x8B,
    g_EOF,
};

static FrameData g_h265data[] = {
    g_hevc8x8I,
    g_hevc8x8P,
    g_hevc8x8B,
    g_hevc8x18,
    g_hevc16x16,
    g_hevc8x8I,
    g_hevc8x8P,
    g_hevc8x8B,
    g_hevc8x8I,
    g_hevc8x8P,
    g_hevc8x8B,
    g_EOF,
};

static FrameData g_vp8data[] = {
    g_vp8_8x8I,
    g_vp8_8x8P1,
    g_vp8_8x8P2,
    g_vp8_16x16,
    g_vp8_8x8I,
    g_vp8_8x8P1,
    g_vp8_8x8P2,
    g_EOF,
};

static FrameData g_vp9data[] = {
    g_vp9_8x8I,
    g_vp9_8x8P1,
    g_vp9_8x8P2,
    g_vp9_16x16,
    g_vp9_8x8I,
    g_vp9_8x8P1,
    g_vp9_8x8P2,
    g_EOF,
};

static FrameData g_jpegdata[] = {
    g_jpeg1_8x8,
    g_jpeg2_8x8,
    g_jpeg_16x16,
    g_jpeg1_8x8,
    g_jpeg2_8x8,
    g_EOF,
};

static FrameData g_h264data_lowlatency[] = {
    g_avc8x8I,
    g_avc8x8P,
    g_avc8x16,
    g_avc16x16,
    g_avc8x8I,
    g_avc8x8P,
    g_avc8x8I,
    g_avc8x8P,
    g_EOF,
};

static FrameData g_vp8data_lowlatency[] = {
    g_vp8_8x8I,
    g_vp8_8x8P1,
    g_vp8_8x8P2,
    g_vp8_16x16,
    g_vp8_8x8I,
    g_vp8_8x8P1,
    g_vp8_8x8P2,
    g_EOF,
};

static FrameData g_vp9data_lowlatency[] = {
    g_vp9_8x8I,
    g_vp9_8x8P1,
    g_vp9_8x8P2,
    g_vp9_16x16,
    g_vp9_8x8I,
    g_vp9_8x8P1,
    g_vp9_8x8P2,
    g_EOF,
};

static FrameData g_jpegdata_lowlatency[] = {
    g_jpeg1_8x8,
    g_jpeg2_8x8,
    g_jpeg_16x16,
    g_jpeg1_8x8,
    g_jpeg2_8x8,
    g_EOF,
};

static const std::string getFullTestName()
{
    const ::testing::TestInfo* const info = ::testing::UnitTest::GetInstance()->current_test_info();
    return std::string(info->test_case_name())
        + std::string(".")
        + std::string(info->name());
}

class TestDecodeFrames {
public:
    typedef SharedPtr<TestDecodeFrames> Shared;

    static Shared create(
        const FrameData* data,
        const char* mime)
    {
        Shared frames(new TestDecodeFrames(data, mime));
        return frames;
    }

    const char* getMime() const
    {
        return m_mime;
    }

    const uint32_t getFrameCount() const
    {
        uint32_t i = 0;
        FrameData d = m_data[i];
        while (d.m_data != NULL) {
            i++;
            d = m_data[i];
        }
        return i;
    }

    bool getFrame(VideoDecodeBuffer& buffer, FrameInfo& info)
    {
        const FrameData& d = m_data[m_idx];
        if (d.m_data != NULL) {
            buffer.data = (uint8_t*)d.m_data;
            buffer.size = d.m_size;
            info = d.m_info;
            m_idx++;
            return true;
        }
        return false;
    }

    void seekToStart()
    {
        m_idx = 0;
    }

    friend std::ostream& operator<<(std::ostream& os, const TestDecodeFrames& t)
    {
        os << t.m_mime;
        return os;
    }

private:
    TestDecodeFrames(
        const FrameData* data,
        const char* mime)
        : m_data(data)
        , m_mime(mime)
        , m_idx(0)
    {
    }

    const FrameData* m_data;

    const char* m_mime;
    int m_idx;
};

class DecodeSurfaceAllocator : public VaapiSurfaceAllocator {
public:
    DecodeSurfaceAllocator(const DisplayPtr& display)
        : VaapiSurfaceAllocator(display->getID())
        , m_width(0)
        , m_height(0)
    {
    }
    void onFormatChange(const VideoFormatInfo* format)
    {
        m_width = format->surfaceWidth;
        m_height = format->surfaceHeight;
    }

protected:
    YamiStatus doAlloc(SurfaceAllocParams* params)
    {
        EXPECT_EQ(m_width, params->width);
        EXPECT_EQ(m_height, params->height);
        return VaapiSurfaceAllocator::doAlloc(params);
    }
    void doUnref()
    {
        delete this;
    }

private:
    uint32_t m_width;
    uint32_t m_height;
};

class DecodeApiTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<TestDecodeFrames::Shared> {
};

class DecodeApiTestLowlatency
    : public ::testing::Test,
      public ::testing::WithParamInterface<TestDecodeFrames::Shared> {
};

bool checkOutput(SharedPtr<IVideoDecoder>& decoder, std::deque<VideoFormatInfo>& formats)
{
    SharedPtr<VideoFrame> output(decoder->getOutput());
    bool gotOutput = bool(output);
    if (gotOutput) {
        int size = (int)formats.size();
        EXPECT_GT(size, 0);

        const VideoFormatInfo& format = formats.front();

        if (format.width != output->crop.width
            || format.height != output->crop.height) {
            formats.pop_front();
            EXPECT_GT(size, 1);

            const VideoFormatInfo& f = formats.front();
            EXPECT_EQ(f.width, output->crop.width);
            EXPECT_EQ(f.height, output->crop.height);
        }

        //Consider a corner case here, nalu based h264 decoder.
        //We have three frames with different resolution. R1 R2 R3.
        //When we decode R3, it will generate format change, and we send R3 again.
        //The h264 decoder will cache R3 and wait for AUD or next frame to decide
        //it's a complete frame. So R3 is not *really* decoded,  It only has R1, R2 decoded.
        //According to h264 decode logical, IDR frame R2 will make all previous frames bumped.
        //But it does not include himself. So we only get R1 output.
        //So the formats will include three resolution R1, R2, R3
        size = (int)formats.size();
        EXPECT_LE(size, 3);

    }
    return gotOutput;
}

bool isFormatChanged(const std::deque<VideoFormatInfo>& formats, const FrameInfo& info)
{
    if (formats.size() == 0) {
        //no format yet any frame will generate format change
        return true;
    }
    const VideoFormatInfo& format = formats.back();
    return (format.width != info.m_width) || (format.height != info.m_height);
}

TEST_P(DecodeApiTest, Format_Change)
{
    NativeDisplay nativeDisplay;
    memset(&nativeDisplay, 0, sizeof(nativeDisplay));
    DisplayPtr display = VaapiDisplay::create(nativeDisplay);

    SharedPtr<IVideoDecoder> decoder;
    TestDecodeFrames frames = *GetParam();
    decoder.reset(createVideoDecoder(frames.getMime()), releaseVideoDecoder);
    ASSERT_TRUE(bool(decoder));
    DecodeSurfaceAllocator* allocator = new DecodeSurfaceAllocator(display);

    decoder->setAllocator(allocator);

    VideoConfigBuffer config;
    memset(&config, 0, sizeof(config));
    ASSERT_EQ(YAMI_SUCCESS, decoder->start(&config));

    VideoDecodeBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    FrameInfo info;
    uint32_t inFrames = 0;
    uint32_t outFrames = 0;
    //keep previous resolution
    std::deque<VideoFormatInfo> formats;

    while (frames.getFrame(buffer, info)) {

        YamiStatus status = decoder->decode(&buffer);
        if (isFormatChanged(formats, info)) {
            EXPECT_EQ(YAMI_DECODE_FORMAT_CHANGE, status);
        }
        if (status == YAMI_DECODE_FORMAT_CHANGE) {

            //check format
            const VideoFormatInfo* format = decoder->getFormatInfo();
            ASSERT_TRUE(format);
            EXPECT_EQ(info.m_width, format->width);
            EXPECT_EQ(info.m_height, format->height);
            formats.push_back(*format);
            allocator->onFormatChange(format);

            //send buffer again
            status = decoder->decode(&buffer);
            if (YAMI_UNSUPPORTED == status) {
                RecordProperty("skipped", true);
                std::cout << "[  SKIPPED ] " << getFullTestName()
                          << " Hw does not support this decoder." << std::endl;
                return;
            }
            EXPECT_EQ(YAMI_SUCCESS, status);
        }
        else {
            EXPECT_EQ(YAMI_SUCCESS, status);
        }
        inFrames++;
        while (checkOutput(decoder, formats))
            outFrames++;


    }
    //drain the decoder
    EXPECT_EQ(YAMI_SUCCESS, decoder->decode(NULL));
    while (checkOutput(decoder, formats))
        outFrames++;
    EXPECT_TRUE(inFrames > 0);
    EXPECT_EQ((int)formats.size(), 1);
    EXPECT_EQ(inFrames, outFrames);
}

TEST_P(DecodeApiTest, Flush)
{
    NativeDisplay nativeDisplay;
    memset(&nativeDisplay, 0, sizeof(nativeDisplay));
    DisplayPtr display = VaapiDisplay::create(nativeDisplay);

    SharedPtr<IVideoDecoder> decoder;
    TestDecodeFrames frames = *GetParam();
    decoder.reset(createVideoDecoder(frames.getMime()), releaseVideoDecoder);
    ASSERT_TRUE(bool(decoder));
    DecodeSurfaceAllocator* allocator = new DecodeSurfaceAllocator(display);

    decoder->setAllocator(allocator);

    VideoConfigBuffer config;
    memset(&config, 0, sizeof(config));
    ASSERT_EQ(YAMI_SUCCESS, decoder->start(&config));

    VideoDecodeBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    FrameInfo info;
    int64_t timeBeforeFlush = 0;
    int64_t timeCurrent = 0;
    SharedPtr<VideoFrame> output;
    int32_t size = frames.getFrameCount();
    int32_t outFrames = 0;

    for (int i = 0; i < size; i++) {
        outFrames = 0;
        frames.seekToStart();
        for (int j = 0; j <= i; j++) {
            if (!frames.getFrame(buffer, info))
                break;

            buffer.timeStamp = timeCurrent;
            timeCurrent++;
            YamiStatus status = decoder->decode(&buffer);
            while ((output = decoder->getOutput())) {
                EXPECT_TRUE(output->timeStamp > timeBeforeFlush);
                outFrames++;
            }

            if (status == YAMI_DECODE_FORMAT_CHANGE) {
                //check format
                const VideoFormatInfo* format = decoder->getFormatInfo();
                allocator->onFormatChange(format);

                //send buffer again
                status = decoder->decode(&buffer);
                if (YAMI_UNSUPPORTED == status) {
                    RecordProperty("skipped", true);
                    std::cout << "[  SKIPPED ] " << getFullTestName()
                              << " Hw does not support this decoder." << std::endl;
                    return;
                }
                EXPECT_EQ(YAMI_SUCCESS, status);
            }
            else {
                EXPECT_EQ(YAMI_SUCCESS, status);
            }
        }
        timeBeforeFlush = buffer.timeStamp;

        // No flush at last round, it has EOS at the end
        if (i < (size - 1))
            decoder->flush();
    }
    //drain the decoder
    EXPECT_EQ(YAMI_SUCCESS, decoder->decode(NULL));
    while (decoder->getOutput()) {
        outFrames++;
    }
    EXPECT_EQ(outFrames, size);
}

TEST_P(DecodeApiTestLowlatency, Format_Change)
{
    SharedPtr<IVideoDecoder> decoder;
    TestDecodeFrames frames = *GetParam();
    decoder.reset(createVideoDecoder(frames.getMime()), releaseVideoDecoder);
    ASSERT_TRUE(bool(decoder));

    VideoConfigBuffer config;
    memset(&config, 0, sizeof(config));
    config.enableLowLatency = true;
    ASSERT_EQ(YAMI_SUCCESS, decoder->start(&config));

    VideoDecodeBuffer buffer;
    memset(&buffer, 0, sizeof(buffer));
    FrameInfo info;
    uint32_t inFrames = 0;
    uint32_t outFrames = 0;
    //keep previous resolution

    while (frames.getFrame(buffer, info)) {
        buffer.flag = VIDEO_DECODE_BUFFER_FLAG_FRAME_END;
        YamiStatus status = decoder->decode(&buffer);
        if (status == YAMI_DECODE_FORMAT_CHANGE) {

            //send buffer again
            status = decoder->decode(&buffer);
            if (YAMI_UNSUPPORTED == status) {
                RecordProperty("skipped", true);
                std::cout << "[  SKIPPED ] " << getFullTestName()
                          << " Hw does not support this decoder." << std::endl;
                return;
            }
        }
        inFrames++;
        while (bool(decoder->getOutput()))
            outFrames++;
        EXPECT_EQ(inFrames, outFrames);
    }
}

/** Teach Google Test how to print a TestDecodeFrames::Shared object */
void PrintTo(const TestDecodeFrames::Shared& t, std::ostream* os)
{
    *os << *t;
}

INSTANTIATE_TEST_CASE_P(
    VaapiDecoder, DecodeApiTest,
    ::testing::Values(
        TestDecodeFrames::create(g_h264data, YAMI_MIME_H264),
        TestDecodeFrames::create(g_h265data, YAMI_MIME_H265),
        TestDecodeFrames::create(g_vp8data, YAMI_MIME_VP8),
        TestDecodeFrames::create(g_vp9data, YAMI_MIME_VP9),
        TestDecodeFrames::create(g_jpegdata, YAMI_MIME_JPEG)));

INSTANTIATE_TEST_CASE_P(
    VaapiDecoderLowlatency, DecodeApiTestLowlatency,
    ::testing::Values(
        TestDecodeFrames::create(g_h264data_lowlatency, YAMI_MIME_H264),
        TestDecodeFrames::create(g_vp8data_lowlatency, YAMI_MIME_VP8),
        TestDecodeFrames::create(g_vp9data_lowlatency, YAMI_MIME_VP9),
        TestDecodeFrames::create(g_jpegdata_lowlatency, YAMI_MIME_JPEG)));
}
