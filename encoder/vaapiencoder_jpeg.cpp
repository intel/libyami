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

#include "vaapiencoder_jpeg.h"
#include "codecparsers/jpegParser.h"
#include "common/Array.h"
#include "common/common_def.h"
#include "common/scopedlogger.h"
#include "vaapicodedbuffer.h"
#include "vaapiencpicture.h"
#include "common/log.h"
#include <stdio.h>

#define NUM_DC_RUN_SIZE_BITS 16
#define NUM_AC_RUN_SIZE_BITS 16
#define NUM_AC_CODE_WORDS_HUFFVAL 162
#define NUM_DC_CODE_WORDS_HUFFVAL 12

using ::YamiParser::JPEG::FrameHeader;
using ::YamiParser::JPEG::ScanHeader;
using ::YamiParser::JPEG::QuantTable;
using ::YamiParser::JPEG::QuantTables;
using ::YamiParser::JPEG::HuffTable;
using ::YamiParser::JPEG::HuffTables;
using ::YamiParser::JPEG::Component;
using ::YamiParser::JPEG::Defaults;

void generateFrameHdr(const FrameHeader::Shared& frameHdr, int picWidth, int picHeight)
{
    frameHdr->dataPrecision = 8;
    frameHdr->imageWidth = picWidth;
    frameHdr->imageHeight = picHeight;

    frameHdr->components.resize(3);

    for(int i = 0; i < 3; i++) {
        frameHdr->components[i].reset(new Component);

        frameHdr->components[i]->id = i + 1;
        frameHdr->components[i]->index = i;

        if(i == 0) {
            frameHdr->components[i]->hSampleFactor = 2;
            frameHdr->components[i]->vSampleFactor = 2;
            frameHdr->components[i]->quantTableNumber = 0;

        } else {
            //Analyzing the sampling factors for U/V, they are 1 for all formats except for Y8.
            //So, it is okay to have the code below like this. For Y8, we wont reach this code.
            frameHdr->components[i]->hSampleFactor = 1;
            frameHdr->components[i]->vSampleFactor = 1;
            frameHdr->components[i]->quantTableNumber = 1;
        }
    }
}

void generateScanHdr(const ScanHeader::Shared& scanHdr)
{
    scanHdr->numComponents = 3;

    //Y Component
    scanHdr->components[0].reset(new Component);
    scanHdr->components[0]->id = 1;
    scanHdr->components[0]->dcTableNumber = 0;
    scanHdr->components[0]->acTableNumber = 0;

    //U Component
    scanHdr->components[1].reset(new Component);
    scanHdr->components[1]->id = 2;
    scanHdr->components[1]->dcTableNumber = 1;
    scanHdr->components[1]->acTableNumber = 1;

    //V Component
    scanHdr->components[2].reset(new Component);
    scanHdr->components[2]->id = 3;
    scanHdr->components[2]->dcTableNumber = 1;
    scanHdr->components[2]->acTableNumber = 1;
}

static const size_t JPEG_HEADER_SIZE = 83 + (YamiParser::JPEG::DCTSIZE2 * 2)
    + (NUM_DC_RUN_SIZE_BITS * 2) + (NUM_DC_CODE_WORDS_HUFFVAL * 2)
    + (NUM_AC_RUN_SIZE_BITS * 2) + (NUM_AC_CODE_WORDS_HUFFVAL * 2);

typedef std::array<uint8_t, JPEG_HEADER_SIZE> JPEGHeader;

int buildJpegHeader(JPEGHeader& header, int picture_width, int picture_height)
{
    using namespace ::YamiParser::JPEG;

    size_t idx(0);

    // Start Of Input
    header[idx] = 0xFF;
    header[++idx] = M_SOI;

    // Application Segment - JFIF standard 1.01
    header[++idx] = 0xFF;
    header[++idx] = M_APP0;
    header[++idx] = 0x00;
    header[++idx] = 0x10; // Segment length:16 (2-byte)
    header[++idx] = 0x4A; // J
    header[++idx] = 0x46; // F
    header[++idx] = 0x49; // I
    header[++idx] = 0x46; // F
    header[++idx] = 0x00; // 0
    header[++idx] = 0x01; // Major version
    header[++idx] = 0x01; // Minor version
    header[++idx] = 0x01; // Density units 0:no units, 1:pixels per inch, 2: pixels per cm
    header[++idx] = 0x00;
    header[++idx] = 0x48; // X density (2-byte)
    header[++idx] = 0x00;
    header[++idx] = 0x48; // Y density (2-byte)
    header[++idx] = 0x00; // Thumbnail width
    header[++idx] = 0x00; // Thumbnail height

    // Quantization Tables
    const QuantTables& quantTables = Defaults::instance().quantTables();
    for (size_t i(0); i < 2; ++i) {
        const QuantTable::Shared& quantTable = quantTables[i];
        header[++idx] = 0xFF;
        header[++idx] = M_DQT;
        header[++idx] = 0x00;
        header[++idx] = 0x03 + DCTSIZE2; // Segment length:67 (2-byte)

        // Only 8-bit (1 byte) precision is supported
        // 0:8-bit precision, 1:16-bit precision
        assert(quantTable->precision == 0);

        // Precision (4-bit high) = 0, Index (4-bit low) = i
        header[++idx] = static_cast<uint8_t>(i);

        for (size_t j(0); j < DCTSIZE2; ++j)
            header[++idx] = static_cast<uint8_t>(quantTable->values[j]);
    }

    // Start of Frame - Baseline
    FrameHeader::Shared frameHdr(new FrameHeader);
    generateFrameHdr(frameHdr, picture_width, picture_height);
    header[++idx] = 0xFF;
    header[++idx] = M_SOF0; // Baseline
    header[++idx] = 0x00;
    header[++idx] = 0x11; // Segment length:17 (2-byte)
    header[++idx] = static_cast<uint8_t>(frameHdr->dataPrecision);
    header[++idx] = static_cast<uint8_t>((frameHdr->imageHeight >> 8) & 0xFF);
    header[++idx] = static_cast<uint8_t>(frameHdr->imageHeight & 0xFF);
    header[++idx] = static_cast<uint8_t>((frameHdr->imageWidth >> 8) & 0xFF);
    header[++idx] = static_cast<uint8_t>(frameHdr->imageWidth & 0xFF);
    header[++idx] = 0x03; // Number of Components
    for (size_t i(0); i < 3; ++i) {
        const Component::Shared& component = frameHdr->components[i];
        header[++idx] = static_cast<uint8_t>(component->id);
        // Horizontal Sample Factor (4-bit high), Vertical Sample Factor (4-bit low)
        header[++idx] = static_cast<uint8_t>(component->hSampleFactor << 4)
                        | static_cast<uint8_t>(component->vSampleFactor);
        header[++idx] = static_cast<uint8_t>(component->quantTableNumber);
    }

    // Huffman Tables
    const HuffTables& dcTables = Defaults::instance().dcHuffTables();
    const HuffTables& acTables = Defaults::instance().acHuffTables();
    for(size_t i(0); i < 2; ++i) {
        // DC Table
        const HuffTable::Shared& dcTable = dcTables[i];
        header[++idx] = 0xFF;
        header[++idx] = M_DHT;
        header[++idx] = 0x00;
        header[++idx] = 0x1F; // Segment length:31 (2-byte)

        // Type (4-bit high) = 0:DC, Index (4-bit low)
        header[++idx] = static_cast<uint8_t>(i);

        for(size_t j(0); j < NUM_DC_RUN_SIZE_BITS; ++j)
            header[++idx] = dcTable->codes[j];
        for(size_t j(0); j < NUM_DC_CODE_WORDS_HUFFVAL; ++j)
            header[++idx] = dcTable->values[j];

        // AC Table
        const HuffTable::Shared& acTable = acTables[i];
        header[++idx] = 0xFF;
        header[++idx] = M_DHT;
        header[++idx] = 0x00;
        header[++idx] = 0xB5; // Segment length:181 (2-byte)

        // Type (4-bit high) = 1:AC, Index (4-bit low)
        header[++idx] = 0x10 | static_cast<uint8_t>(i);

        for(size_t j(0); j < NUM_AC_RUN_SIZE_BITS; ++j)
            header[++idx] = acTable->codes[j];
        for(size_t j(0); j < NUM_AC_CODE_WORDS_HUFFVAL; ++j)
            header[++idx] = acTable->values[j];
    }

    // Start of Scan
    ScanHeader::Shared scanHdr(new ScanHeader);
    generateScanHdr(scanHdr);
    header[++idx] = 0xFF;
    header[++idx] = M_SOS;
    header[++idx] = 0x00;
    header[++idx] = 0x0C; // Segment Length:12 (2-byte)
    header[++idx] = 0x03; // Number of components in scan
    for (size_t i(0); i < 3; ++i) {
        header[++idx] = static_cast<uint8_t>(scanHdr->components[i]->id);
        // DC Table Selector (4-bit high), AC Table Selector (4-bit low)
        header[++idx] = static_cast<uint8_t>(scanHdr->components[i]->dcTableNumber << 4)
                        | static_cast<uint8_t>(scanHdr->components[i]->acTableNumber);
    }
    header[++idx] = 0x00; // 0 for Baseline
    header[++idx] = 0x3F; // 63 for Baseline
    header[++idx] = 0x00; // 0 for Baseline

    return ++idx << 3;
}

namespace YamiMediaCodec {

typedef VaapiEncoderJpeg::PicturePtr PicturePtr;

class VaapiEncPictureJPEG:public VaapiEncPicture
{
public:
    VaapiEncPictureJPEG(const ContextPtr& context, const SurfacePtr& surface, int64_t timeStamp):
        VaapiEncPicture(context, surface, timeStamp)
    {
    }
     VAEncPictureParameterBufferJPEG picParam;
     VAQMatrixBufferJPEG             qMatrix;
     VAEncSliceParameterBufferJPEG   sliceParam;
     VAHuffmanTableBufferJPEGBaseline huffTableParam;
};

VaapiEncoderJpeg::VaapiEncoderJpeg():
    quality(50)
{
    m_videoParamCommon.profile = VAProfileJPEGBaseline;
    m_entrypoint = VAEntrypointEncPicture;
}

YamiStatus VaapiEncoderJpeg::getMaxOutSize(uint32_t* maxSize)
{
    FUNC_ENTER();
    *maxSize = m_maxCodedbufSize;
    return YAMI_SUCCESS;
}

void VaapiEncoderJpeg::resetParams()
{
    m_maxCodedbufSize = (width()*height()*3/2) + JPEG_HEADER_SIZE;
}

YamiStatus VaapiEncoderJpeg::start()
{
    FUNC_ENTER();
    resetParams();
    return VaapiEncoderBase::start();
}

void VaapiEncoderJpeg::flush()
{
    FUNC_ENTER();
    VaapiEncoderBase::flush();
}

YamiStatus VaapiEncoderJpeg::stop()
{
    flush();
    return VaapiEncoderBase::stop();
}

YamiStatus VaapiEncoderJpeg::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    YamiStatus status = YAMI_SUCCESS;
    FUNC_ENTER();
    if (!videoEncParams)
        return YAMI_INVALID_PARAM;

    switch (type) {
    default:
        status = VaapiEncoderBase::setParameters(type, videoEncParams);
        break;
    }
    return status;
}

YamiStatus VaapiEncoderJpeg::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    if (!videoEncParams)
        return YAMI_INVALID_PARAM;

    return VaapiEncoderBase::getParameters(type, videoEncParams);
}

YamiStatus VaapiEncoderJpeg::doEncode(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    FUNC_ENTER();
    YamiStatus ret;
    CodedBufferPtr codedBuffer = VaapiCodedBuffer::create(m_context, m_maxCodedbufSize);
    PicturePtr picture(new VaapiEncPictureJPEG(m_context, surface, timeStamp));
    picture->m_codedBuffer = codedBuffer;
    ret = encodePicture(picture);
    if (ret != YAMI_SUCCESS)
        return ret;
    INFO();
    if (!output(picture))
        return YAMI_INVALID_PARAM;
    return YAMI_SUCCESS;
}

bool VaapiEncoderJpeg::fill(VAEncPictureParameterBufferJPEG * picParam, const PicturePtr &picture,
                            const SurfacePtr &surface) const
{
    picParam->reconstructed_picture = surface->getID();
    picParam->picture_height = height();
    picParam->picture_width = width();
    picParam->coded_buf = picture->m_codedBuffer->getID();
    //Profile = Baseline
    picParam->pic_flags.bits.profile = 0;
    //Sequential encoding
    picParam->pic_flags.bits.progressive = 0;
    //Uses Huffman coding
    picParam->pic_flags.bits.huffman = 1;
    //Input format is interleaved (YUV)
    picParam->pic_flags.bits.interleaved = 0;
    //non-Differential Encoding
    picParam->pic_flags.bits.differential = 0;
    //only 8 bit sample depth is currently supported
    picParam->sample_bit_depth = 8;
    picParam->num_scan = 1;
    // Supporting only upto 3 components maximum
    picParam->num_components = 3;
    picParam->quality = quality;
    return TRUE;
}

bool VaapiEncoderJpeg::fill(VAQMatrixBufferJPEG * qMatrix) const
{
    const QuantTable::Shared luminance = Defaults::instance().quantTables()[0];
    qMatrix->load_lum_quantiser_matrix = 1;
    for (size_t i = 0; i < ::YamiParser::JPEG::DCTSIZE2; i++) {
        qMatrix->lum_quantiser_matrix[i] = luminance->values[i];
    }

    const QuantTable::Shared chrominance = Defaults::instance().quantTables()[1];
    qMatrix->load_chroma_quantiser_matrix = 1;
    for (size_t i = 0; i < ::YamiParser::JPEG::DCTSIZE2; i++) {
        qMatrix->chroma_quantiser_matrix[i] = chrominance->values[i];
    }

    return true;
}

bool VaapiEncoderJpeg::fill(VAEncSliceParameterBufferJPEG *sliceParam) const
{
    sliceParam->restart_interval = 0;

    sliceParam->num_components = 3;

    sliceParam->components[0].component_selector = 1;
    sliceParam->components[0].dc_table_selector = 0;
    sliceParam->components[0].ac_table_selector = 0;

    sliceParam->components[1].component_selector = 2;
    sliceParam->components[1].dc_table_selector = 1;
    sliceParam->components[1].ac_table_selector = 1;

    sliceParam->components[2].component_selector = 3;
    sliceParam->components[2].dc_table_selector = 1;
    sliceParam->components[2].ac_table_selector = 1;
    return true;
}

bool VaapiEncoderJpeg::fill(VAHuffmanTableBufferJPEGBaseline *huffTableParam) const
{
    const HuffTables& dcHuffTables = Defaults::instance().dcHuffTables();
    const HuffTables& acHuffTables = Defaults::instance().acHuffTables();

    const size_t numTables = MIN(N_ELEMENTS(huffTableParam->huffman_table),
                                 ::YamiParser::JPEG::NUM_HUFF_TBLS);

    for (size_t i(0); i < numTables; ++i) {
        const HuffTable::Shared& dcTable = dcHuffTables[i];
        const HuffTable::Shared& acTable = acHuffTables[i];
        bool valid = bool(dcTable) && bool(acTable);
        huffTableParam->load_huffman_table[i] = valid;
        if (!valid)
            continue;

        // Load DC Table
        memcpy(huffTableParam->huffman_table[i].num_dc_codes,
               &dcTable->codes[0],
               sizeof(huffTableParam->huffman_table[i].num_dc_codes));
        memcpy(huffTableParam->huffman_table[i].dc_values,
               &dcTable->values[0],
               sizeof(huffTableParam->huffman_table[i].dc_values));

        // Load AC Table
        memcpy(huffTableParam->huffman_table[i].num_ac_codes,
               &acTable->codes[0],
               sizeof(huffTableParam->huffman_table[i].num_ac_codes));
        memcpy(huffTableParam->huffman_table[i].ac_values,
               &acTable->values[0],
               sizeof(huffTableParam->huffman_table[i].ac_values));

        memset(huffTableParam->huffman_table[i].pad,
               0, sizeof(huffTableParam->huffman_table[i].pad));
    }

    return true;
}

bool VaapiEncoderJpeg::ensurePicture (const PicturePtr& picture, const SurfacePtr& surface)
{
    VAEncPictureParameterBufferJPEG *picParam;

    if(!picture->editPicture(picParam) || !fill(picParam, picture, surface)) {
        ERROR("failed to create picture parameter buffer");
        return false;
    }
    memcpy(&picture->picParam, picParam, sizeof(*picParam));
    return true;
}

bool VaapiEncoderJpeg::ensureQMatrix (const PicturePtr& picture)
{
    VAQMatrixBufferJPEG *qMatrix;

    if(!picture->editQMatrix(qMatrix) || !fill(qMatrix)) {
        ERROR("failed to create qMatrix");
        return false;
    }
    memcpy(&picture->qMatrix, qMatrix, sizeof(*qMatrix));
    return true;
}

bool VaapiEncoderJpeg::ensureSlice (const PicturePtr& picture)
{
    VAEncSliceParameterBufferJPEG *sliceParam;

    if(!picture->newSlice(sliceParam) || !fill(sliceParam)) {
        ERROR("failed to create slice parameter");
        return false;
    }
    memcpy(&picture->sliceParam, sliceParam, sizeof(*sliceParam));
    return true;
}

bool VaapiEncoderJpeg::ensureHuffTable(const PicturePtr & picture)
{
    VAHuffmanTableBufferJPEGBaseline *huffTableParam;

    if(!picture->editHuffTable(huffTableParam) || !fill(huffTableParam)) {
        ERROR("failed to create Huffman Table");
        return false;
    }
    memcpy(&picture->huffTableParam, huffTableParam, sizeof(*huffTableParam));
    return true;
}

bool VaapiEncoderJpeg::addSliceHeaders (const PicturePtr& picture) const
{
    unsigned int length_in_bits;
    JPEGHeader header;
    length_in_bits = buildJpegHeader(header, width(), height());

    if(!picture->addPackedHeader(VAEncPackedHeaderRawData, header.data(), length_in_bits))
        return false;
    return true;
}

YamiStatus VaapiEncoderJpeg::encodePicture(const PicturePtr& picture)
{
    YamiStatus ret = YAMI_FAIL;
    SurfacePtr reconstruct = createSurface();
    if (!reconstruct)
        return ret;

    if (!ensurePicture(picture, reconstruct))
        return ret;
    
    if (!ensureQMatrix (picture))
        return ret;
    
    if (!ensureHuffTable(picture))
        return ret;
    
    if (!ensureSlice (picture))
        return ret;

    if (!addSliceHeaders (picture))
        return ret;
    
    if (!picture->encode())
        return ret;
    return YAMI_SUCCESS;
}

}
