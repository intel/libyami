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

// primary header
#include "vaapiDecoderJPEG.h"

// library headers
#include "codecparsers/jpegParser.h"
#include "common/common_def.h"

// system headers
#include <cassert>

using ::YamiParser::JPEG::Component;
using ::YamiParser::JPEG::FrameHeader;
using ::YamiParser::JPEG::HuffTable;
using ::YamiParser::JPEG::HuffTables;
using ::YamiParser::JPEG::Parser;
using ::YamiParser::JPEG::QuantTable;
using ::YamiParser::JPEG::QuantTables;
using ::YamiParser::JPEG::ScanHeader;
using ::YamiParser::JPEG::Defaults;
using ::std::function;
using ::std::bind;
using ::std::ref;

namespace YamiMediaCodec {

#define JPEG_SURFACE_NUM 2

struct Slice {
    Slice() : data(NULL), start(0) , length(0) { }

    const uint8_t* data;
    uint32_t start;
    uint32_t length;
};

class VaapiDecoderJPEG::Impl
{
public:
    typedef function<YamiStatus(void)> DecodeHandler;

    Impl(const DecodeHandler& start, const DecodeHandler& finish)
        : m_startHandler(start)
        , m_finishHandler(finish)
        , m_parser()
        , m_dcHuffmanTables(Defaults::instance().dcHuffTables())
        , m_acHuffmanTables(Defaults::instance().acHuffTables())
        , m_quantizationTables(Defaults::instance().quantTables())
        , m_slice()
        , m_decodeStatus(YAMI_SUCCESS)
    {
    }

    YamiStatus decode(const uint8_t* data, const uint32_t size)
    {
        using namespace ::YamiParser::JPEG;

        //this mainly for codec flush, jpeg/mjpeg do not have to flush.
        //just return success for this
        if (!data || !size)
            return YAMI_SUCCESS;

        Parser::Callback defaultCallback =
            bind(&Impl::onMarker, ref(*this));
        Parser::Callback sofCallback =
            bind(&Impl::onStartOfFrame, ref(*this));
        m_slice.data = data;
        m_parser.reset(new Parser(data, size));
        m_parser->registerCallback(M_SOI, defaultCallback);
        m_parser->registerCallback(M_EOI, defaultCallback);
        m_parser->registerCallback(M_SOS, defaultCallback);
        m_parser->registerCallback(M_DHT, defaultCallback);
        m_parser->registerCallback(M_DQT, defaultCallback);
        m_parser->registerStartOfFrameCallback(sofCallback);

        if (!m_parser->parse())
            m_decodeStatus = YAMI_FAIL;

        return m_decodeStatus;
    }

    const FrameHeader::Shared& frameHeader() const
    {
        return m_parser->frameHeader();
    }

    const ScanHeader::Shared& scanHeader() const
    {
        return m_parser->scanHeader();
    }

    const unsigned restartInterval() const
    {
        return m_parser->restartInterval();
    }

    const HuffTables& dcHuffmanTables() const { return m_dcHuffmanTables; }
    const HuffTables& acHuffmanTables() const { return m_acHuffmanTables; }
    const QuantTables& quantTables() const { return m_quantizationTables; }
    const Slice& slice() const { return m_slice; }

private:
    Parser::CallbackResult onMarker()
    {
        using namespace ::YamiParser::JPEG;

        m_decodeStatus = YAMI_SUCCESS;

        switch(m_parser->current().marker) {
        case M_SOI:
            m_slice.start = 0;
            m_slice.length = 0;
            break;
        case M_SOS:
            m_slice.start = m_parser->current().position + 1
                + m_parser->current().length;
            break;
        case M_EOI:
            m_slice.length = m_parser->current().position - m_slice.start;
            m_decodeStatus = m_finishHandler();
            break;
        case M_DQT:
            m_quantizationTables = m_parser->quantTables();
            break;
        case M_DHT:
            m_dcHuffmanTables = m_parser->dcHuffTables();
            m_acHuffmanTables = m_parser->acHuffTables();
            break;
        default:
            m_decodeStatus = YAMI_FAIL;
        }

        if (m_decodeStatus != YAMI_SUCCESS)
            return Parser::ParseSuspend;
        return Parser::ParseContinue;
    }

    Parser::CallbackResult onStartOfFrame()
    {
        m_decodeStatus = m_startHandler();
        if (m_decodeStatus != YAMI_SUCCESS)
            return Parser::ParseSuspend;
        return Parser::ParseContinue;
    }

    const DecodeHandler m_startHandler; // called after SOF
    const DecodeHandler m_finishHandler; // called after EOI

    Parser::Shared m_parser;
    HuffTables m_dcHuffmanTables;
    HuffTables m_acHuffmanTables;
    QuantTables m_quantizationTables;

    Slice m_slice;

    YamiStatus m_decodeStatus;
};

VaapiDecoderJPEG::VaapiDecoderJPEG()
    : VaapiDecoderBase::VaapiDecoderBase()
    , m_impl()
    , m_picture()
{
    return;
}

YamiStatus VaapiDecoderJPEG::fillPictureParam()
{
    const FrameHeader::Shared frame = m_impl->frameHeader();

    const size_t numComponents = frame->components.size();

    if (numComponents > 4)
        return YAMI_FAIL;

    VAPictureParameterBufferJPEGBaseline* vaPicParam(NULL);

    if (!m_picture->editPicture(vaPicParam))
        return YAMI_FAIL;

    for (size_t i(0); i < numComponents; ++i) {
        const Component::Shared& component = frame->components[i];
        vaPicParam->components[i].component_id = component->id;
        vaPicParam->components[i].h_sampling_factor = component->hSampleFactor;
        vaPicParam->components[i].v_sampling_factor = component->vSampleFactor;
        vaPicParam->components[i].quantiser_table_selector =
            component->quantTableNumber;
    }

    vaPicParam->picture_width = frame->imageWidth;
    vaPicParam->picture_height = frame->imageHeight;
    vaPicParam->num_components = frame->components.size();

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::fillSliceParam()
{
    const ScanHeader::Shared scan = m_impl->scanHeader();
    const FrameHeader::Shared frame = m_impl->frameHeader();
    const Slice& slice = m_impl->slice();
    VASliceParameterBufferJPEGBaseline *sliceParam(NULL);

    if (!m_picture->newSlice(sliceParam, slice.data + slice.start, slice.length))
        return YAMI_FAIL;

    for (size_t i(0); i < scan->numComponents; ++i) {
        sliceParam->components[i].component_selector =
            scan->components[i]->id;
        sliceParam->components[i].dc_table_selector =
            scan->components[i]->dcTableNumber;
        sliceParam->components[i].ac_table_selector =
            scan->components[i]->acTableNumber;
    }

    sliceParam->restart_interval = m_impl->restartInterval();
    sliceParam->num_components = scan->numComponents;
    sliceParam->slice_horizontal_position = 0;
    sliceParam->slice_vertical_position = 0;

    int width = frame->imageWidth;
    int height = frame->imageHeight;
    int maxHSample = frame->maxHSampleFactor << 3;
    int maxVSample = frame->maxVSampleFactor << 3;
    int codedWidth;
    int codedHeight;

    if (scan->numComponents == 1) { /* Noninterleaved Scan */
        if (frame->components.front() == scan->components.front()) {
            /* Y mcu */
            codedWidth = width >> 3;
            codedHeight = height >> 3;
        } else {
            /* Cr, Cb mcu */
            codedWidth = width >> 4;
            codedHeight = height >> 4;
        }
    } else { /* Interleaved Scan */
        codedWidth = (width + maxHSample - 1) / maxHSample;
        codedHeight = (height + maxVSample - 1) / maxVSample;
    }

    sliceParam->num_mcus = codedWidth * codedHeight;

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::loadQuantizationTables()
{
    using namespace ::YamiParser::JPEG;

    VAIQMatrixBufferJPEGBaseline* vaIqMatrix(NULL);

    if (!m_picture->editIqMatrix(vaIqMatrix))
        return YAMI_FAIL;

    size_t numTables = std::min(
        N_ELEMENTS(vaIqMatrix->quantiser_table), size_t(NUM_QUANT_TBLS));

    for (size_t i(0); i < numTables; ++i) {
        const QuantTable::Shared& quantTable = m_impl->quantTables()[i];
        vaIqMatrix->load_quantiser_table[i] = bool(quantTable);
        if (!quantTable)
            continue;
        assert(quantTable->precision == 0);
        for (uint32_t j(0); j < DCTSIZE2; ++j)
            vaIqMatrix->quantiser_table[i][j] = quantTable->values[j];
    }

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::loadHuffmanTables()
{
    using namespace ::YamiParser::JPEG;

    VAHuffmanTableBufferJPEGBaseline* vaHuffmanTable(NULL);

    if (!m_picture->editHufTable(vaHuffmanTable))
        return YAMI_FAIL;

    size_t numTables = std::min(
        N_ELEMENTS(vaHuffmanTable->huffman_table), size_t(NUM_HUFF_TBLS));

    for (size_t i(0); i < numTables; ++i) {
        const HuffTable::Shared& dcTable = m_impl->dcHuffmanTables()[i];
        const HuffTable::Shared& acTable = m_impl->acHuffmanTables()[i];
        bool valid = bool(dcTable) && bool(acTable);
        vaHuffmanTable->load_huffman_table[i] = valid;
        if (!valid)
            continue;

        // Load DC Table
        memcpy(vaHuffmanTable->huffman_table[i].num_dc_codes,
            &dcTable->codes[0],
            sizeof(vaHuffmanTable->huffman_table[i].num_dc_codes));
        memcpy(vaHuffmanTable->huffman_table[i].dc_values,
            &dcTable->values[0],
            sizeof(vaHuffmanTable->huffman_table[i].dc_values));

        // Load AC Table
        memcpy(vaHuffmanTable->huffman_table[i].num_ac_codes,
            &acTable->codes[0],
            sizeof(vaHuffmanTable->huffman_table[i].num_ac_codes));
        memcpy(vaHuffmanTable->huffman_table[i].ac_values,
            &acTable->values[0],
            sizeof(vaHuffmanTable->huffman_table[i].ac_values));

        memset(vaHuffmanTable->huffman_table[i].pad,
                0, sizeof(vaHuffmanTable->huffman_table[i].pad));
    }

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::decode(VideoDecodeBuffer* buffer)
{
    if (!buffer || !buffer->data)
        return YAMI_SUCCESS;

    m_currentPTS = buffer->timeStamp;

    if (!m_impl.get())
        m_impl.reset(new VaapiDecoderJPEG::Impl(
            bind(&VaapiDecoderJPEG::start, ref(*this), &m_configBuffer),
            bind(&VaapiDecoderJPEG::finish, ref(*this))));

    return m_impl->decode(buffer->data, buffer->size);
}

#define RETURN_FORMAT(f)                             \
    do {                                             \
        uint32_t fourcc = f;                         \
        DEBUG("jpeg format %.4s", (char*) & fourcc); \
        return fourcc;                               \
    } while (0)

//get frame fourcc, return 0 for unsupport format
static uint32_t getFourcc(const FrameHeader::Shared& frame)
{
    if (frame->components.size() != 3) {
        ERROR("unsupported compoent size %d", (int)frame->components.size());
        return 0;
    }
    int h1 = frame->components[0]->hSampleFactor;
    int h2 = frame->components[1]->hSampleFactor;
    int h3 = frame->components[2]->hSampleFactor;
    int v1 = frame->components[0]->vSampleFactor;
    int v2 = frame->components[1]->vSampleFactor;
    int v3 = frame->components[2]->vSampleFactor;
    if (h2 != h3 || v2 != v3) {
        ERROR("unsupported format h1 = %d, h2 = %d, h3 = %d, v1 = %d, v2 = %d, v3 = %d", h1, h2, h3, v1, v2, v3);
        return 0;
    }
    if (h1 == h2) {
        if (v1 == v2)
            RETURN_FORMAT(YAMI_FOURCC_444P);
        if (v1 == 2 * v2)
            RETURN_FORMAT(YAMI_FOURCC_422V);
    }
    else if (h1 == 2 * h2) {
        if (v1 == v2)
            RETURN_FORMAT(YAMI_FOURCC_422H);
        if (v1 == 2 * v2)
            RETURN_FORMAT(YAMI_FOURCC_IMC3);
    }
    ERROR("unsupported format h1 = %d, h2 = %d, h3 = %d, v1 = %d, v2 = %d, v3 = %d", h1, h2, h3, v1, v2, v3);
    return 0;
}

YamiStatus VaapiDecoderJPEG::start(VideoConfigBuffer* buffer)
{
    DEBUG("%s", __func__);

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::finish()
{
    if (!m_impl->frameHeader()) {
        ERROR("Start of Frame (SOF) not found");
        return YAMI_FAIL;
    }

    if (!m_impl->scanHeader()) {
        ERROR("Start of Scan (SOS) not found");
        return YAMI_FAIL;
    }

    YamiStatus status;
    status = ensureContext();
    if (status != YAMI_SUCCESS) {
        return status;
    }
    status = createPicture(m_picture, m_currentPTS);
    if (status != YAMI_SUCCESS) {
        ERROR("Could not create a VAAPI picture.");
        return status;
    }

    m_picture->m_timeStamp = m_currentPTS;
    SurfacePtr surf;
    surf = m_picture->getSurface();
    surf->setCrop(0, 0, m_videoFormatInfo.width, m_videoFormatInfo.height);

    status = fillSliceParam();
    if (status !=  YAMI_SUCCESS) {
        ERROR("Failed to load VAAPI slice parameters.");
        return status;
    }

    status = fillPictureParam();
    if (status !=  YAMI_SUCCESS) {
        ERROR("Failed to load VAAPI picture parameters");
        return status;
    }

    status = loadQuantizationTables();
    if (status !=  YAMI_SUCCESS) {
        ERROR("Failed to load VAAPI quantization tables");
        return status;
    }

    status = loadHuffmanTables();

    if (status != YAMI_SUCCESS) {
        ERROR("Failed to load VAAPI huffman tables");
        return status;
    }

    if (!m_picture->decode())
        return YAMI_FAIL;

    status = outputPicture(m_picture);
    if (status != YAMI_SUCCESS)
        return status;

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderJPEG::reset(VideoConfigBuffer* buffer)
{
    DEBUG("%s", __func__);

    m_picture.reset();

    m_impl.reset();

    return VaapiDecoderBase::reset(buffer);
}

YamiStatus VaapiDecoderJPEG::ensureContext()
{
    const FrameHeader::Shared frame = m_impl->frameHeader();
    if (!frame->isBaseline) {
        ERROR("Unsupported JPEG profile. Only JPEG Baseline is supported.");
        return YAMI_FAIL;
    }

    if (!getFourcc(frame)) {
        return YAMI_UNSUPPORTED;
    }
    if (setFormat(frame->imageWidth, frame->imageHeight, frame->imageWidth,
            frame->imageHeight, JPEG_SURFACE_NUM, getFourcc(frame))) {
        return YAMI_DECODE_FORMAT_CHANGE;
    }
    return ensureProfile(VAProfileJPEGBaseline);
}
}
