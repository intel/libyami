/*
 *  vaapiencoder_jpeg.cpp - jpeg encoder for va
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Muppavarapu, Sirisha <sirisha.muppavarapu@intel.com>
 *              Xin Tang <xin.t.tang@intel.com>
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
#include "vaapiencoder_jpeg.h"
#include "common/common_def.h"
#include "scopedlogger.h"
#include "vaapicodedbuffer.h"
#include "vaapiencpicture.h"
#include "log.h"
#include "bitwriter.h"
#include <stdio.h>

#define NUM_QUANT_ELEMENTS 64
#define NUM_DC_RUN_SIZE_BITS 16
#define NUM_AC_RUN_SIZE_BITS 16
#define NUM_AC_CODE_WORDS_HUFFVAL 162
#define NUM_DC_CODE_WORDS_HUFFVAL 12

void generateFrameHdr(JpegFrameHdr *frameHdr, int picWidth, int picHeight)
{
    frameHdr->sample_precision = 8;
    frameHdr->width = picWidth;
    frameHdr->height = picHeight;
    frameHdr->num_components = 3;

    for(int i = 0; i < 3; i++) {
        frameHdr->components[i].identifier = i+1;

        if(i == 0) {
            frameHdr->components[i].horizontal_factor = 2;
            frameHdr->components[i].vertical_factor = 2;
            frameHdr->components[i].quant_table_selector = 0;

        } else {
            //Analyzing the sampling factors for U/V, they are 1 for all formats except for Y8.
            //So, it is okay to have the code below like this. For Y8, we wont reach this code.
            frameHdr->components[i].horizontal_factor = 1;
            frameHdr->components[i].vertical_factor = 1;
            frameHdr->components[i].quant_table_selector = 1;
        }
    }
}

void generateScanHdr(JpegScanHdr *scanHdr)
{
    scanHdr->num_components = 3;

    //Y Component
    scanHdr->components[0].component_selector = 1;
    scanHdr->components[0].dc_selector = 0;
    scanHdr->components[0].ac_selector = 0;

   
    //U Component
    scanHdr->components[1].component_selector = 2;
    scanHdr->components[1].dc_selector = 1;
    scanHdr->components[1].ac_selector = 1;

    //V Component
    scanHdr->components[2].component_selector = 3;
    scanHdr->components[2].dc_selector = 1;
    scanHdr->components[2].ac_selector = 1;
}

int buildJpegHeader(unsigned char **header_buffer, int picture_width, int picture_height)
{
    BitWriter bs;
    bit_writer_init(&bs, 256*4);

    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_SOI, 8);
    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_APP_MIN, 8);
    bit_writer_put_bits_uint16(&bs, 16, 16);
    bit_writer_put_bits_uint8(&bs, 0x4A, 8);   //J
    bit_writer_put_bits_uint8(&bs, 0x46, 8);   //F
    bit_writer_put_bits_uint8(&bs, 0x49, 8);   //I
    bit_writer_put_bits_uint8(&bs, 0x46, 8);   //F
    bit_writer_put_bits_uint8(&bs, 0x00, 8);   //0
    bit_writer_put_bits_uint8(&bs, 1, 8);      //Major Version
    bit_writer_put_bits_uint8(&bs, 1, 8);      //Minor Version
    bit_writer_put_bits_uint8(&bs, 1, 8);      //Density units 0:no units, 1:pixels per inch, 2: pixels per cm
    bit_writer_put_bits_uint16(&bs, 72, 16);    //X density
    bit_writer_put_bits_uint16(&bs, 72, 16);    //Y density
    bit_writer_put_bits_uint8(&bs, 0, 8);      //Thumbnail width
    bit_writer_put_bits_uint8(&bs, 0, 8);      //Thumbnail height

    JpegQuantTables quant_tables;
    jpeg_get_default_quantization_tables(&quant_tables);

    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_DQT, 8);
    bit_writer_put_bits_uint16(&bs, 3 + NUM_QUANT_ELEMENTS, 16); //lq
    bit_writer_put_bits_uint8(&bs, quant_tables.quant_tables[0].quant_precision, 4); //pq
    bit_writer_put_bits_uint8(&bs, 0, 4); //tq
    for(int i = 0; i < NUM_QUANT_ELEMENTS; i++) {
        bit_writer_put_bits_uint16(&bs, quant_tables.quant_tables[0].quant_table[i], 8);
    }

    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_DQT, 8);
    bit_writer_put_bits_uint16(&bs, 3 + NUM_QUANT_ELEMENTS, 16); //lq
    bit_writer_put_bits_uint8(&bs, quant_tables.quant_tables[1].quant_precision, 4); //pq
    bit_writer_put_bits_uint8(&bs, 1, 4); //tq
    for(int i = 0; i < NUM_QUANT_ELEMENTS; i++) {
        bit_writer_put_bits_uint16(&bs, quant_tables.quant_tables[1].quant_table[i], 8);
    }

    JpegFrameHdr frameHdr;
    memset(&frameHdr,0,sizeof(JpegFrameHdr));
    generateFrameHdr(&frameHdr,  picture_width, picture_height);

    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_SOF_MIN, 8);
    bit_writer_put_bits_uint16(&bs, 8 + (3 * 3), 16); //lf, Size of FrameHeader in bytes without the Marker SOF
    bit_writer_put_bits_uint8(&bs, frameHdr.sample_precision, 8);
    bit_writer_put_bits_uint16(&bs, frameHdr.height, 16);
    bit_writer_put_bits_uint16(&bs, frameHdr.width, 16);
    bit_writer_put_bits_uint8(&bs, frameHdr.num_components, 8);
    
    for(int i = 0; i < frameHdr.num_components; i++) {
        bit_writer_put_bits_uint8(&bs, frameHdr.components[i].identifier, 8);
        bit_writer_put_bits_uint8(&bs, frameHdr.components[i].horizontal_factor, 4);
        bit_writer_put_bits_uint8(&bs, frameHdr.components[i].vertical_factor, 4);
        bit_writer_put_bits_uint8(&bs, frameHdr.components[i].quant_table_selector, 8);
    }

    JpegHuffmanTables huffTable;
    jpeg_get_default_huffman_tables(&huffTable);
    for(int i = 0; i < 2; i++) {
        bit_writer_put_bits_uint8(&bs, 0xFF, 8);
        bit_writer_put_bits_uint8(&bs, JPEG_MARKER_DHT, 8);
        bit_writer_put_bits_uint16(&bs, 0x1F, 16); //length of table
        bit_writer_put_bits_uint8(&bs, 0, 4);
        bit_writer_put_bits_uint8(&bs, i, 4);
        for(int j = 0; j < NUM_DC_RUN_SIZE_BITS; j++) {
            bit_writer_put_bits_uint8(&bs, huffTable.dc_tables[i].huf_bits[j], 8);
        }

        for(int j = 0; j < NUM_DC_CODE_WORDS_HUFFVAL; j++) {
            bit_writer_put_bits_uint8(&bs, huffTable.dc_tables[i].huf_values[j], 8);
        }

        bit_writer_put_bits_uint8(&bs, 0xFF, 8);
        bit_writer_put_bits_uint8(&bs, JPEG_MARKER_DHT, 8);
        bit_writer_put_bits_uint16(&bs, 0xB5, 16); //length of table
        bit_writer_put_bits_uint8(&bs, 1, 4);
        bit_writer_put_bits_uint8(&bs, i, 4);
        for(int j = 0; j < NUM_AC_RUN_SIZE_BITS; j++) {
            bit_writer_put_bits_uint8(&bs, huffTable.ac_tables[i].huf_bits[j], 8);
        }

        for(int j = 0; j < NUM_AC_CODE_WORDS_HUFFVAL; j++) {
            bit_writer_put_bits_uint8(&bs, huffTable.ac_tables[i].huf_values[j], 8);
        }
    }

    //Add ScanHeader
    JpegScanHdr scanHdr;
    generateScanHdr(&scanHdr);
 
    bit_writer_put_bits_uint8(&bs, 0xFF, 8);
    bit_writer_put_bits_uint8(&bs, JPEG_MARKER_SOS, 8);
    bit_writer_put_bits_uint16(&bs, 3 + (3 * 2) + 3, 16); //Length of Scan
    bit_writer_put_bits_uint8(&bs, scanHdr.num_components, 8);
    
    for(int i = 0; i < scanHdr.num_components; i++) {
        bit_writer_put_bits_uint8(&bs, scanHdr.components[i].component_selector, 8);
        bit_writer_put_bits_uint8(&bs, scanHdr.components[i].dc_selector, 4);
        bit_writer_put_bits_uint8(&bs, scanHdr.components[i].ac_selector, 4);
    }

    bit_writer_put_bits_uint8(&bs, 0, 8); //0 for Baseline
    bit_writer_put_bits_uint8(&bs, 63, 8); //63 for Baseline
    bit_writer_put_bits_uint8(&bs, 0, 4); //0 for Baseline
    bit_writer_put_bits_uint8(&bs, 0, 4); //0 for Baseline

    *header_buffer = (unsigned char *)bs.data;
    return bs.bit_size;
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
    m_hasHufTable = FALSE;
    m_hasQuantTable = FALSE;
    memset(&m_hufTables, 0, sizeof(m_hufTables));
    memset(&m_quantTables, 0, sizeof(m_quantTables));
}

VaapiEncoderJpeg::~VaapiEncoderJpeg()
{
}

Encode_Status VaapiEncoderJpeg::getMaxOutSize(uint32_t *maxSize)
{
    FUNC_ENTER();
    *maxSize = m_maxCodedbufSize;
    return ENCODE_SUCCESS;
}

void VaapiEncoderJpeg::resetParams()
{
    m_maxCodedbufSize = width()*height()*3/2;
}

Encode_Status VaapiEncoderJpeg::start()
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

Encode_Status VaapiEncoderJpeg::stop()
{
    flush();
    VaapiEncoderBase::stop();
}

Encode_Status VaapiEncoderJpeg::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    Encode_Status status = ENCODE_SUCCESS;
    FUNC_ENTER();
    if (!videoEncParams)
        return ENCODE_INVALID_PARAMS;

    switch (type) {
    default:
        status = VaapiEncoderBase::setParameters(type, videoEncParams);
        break;
    }
    return status;
}

Encode_Status VaapiEncoderJpeg::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    if (!videoEncParams)
        return ENCODE_INVALID_PARAMS;

    return VaapiEncoderBase::getParameters(type, videoEncParams);
}

Encode_Status VaapiEncoderJpeg::doEncode(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    FUNC_ENTER();
    Encode_Status ret;
    CodedBufferPtr codedBuffer = VaapiCodedBuffer::create(m_context, m_maxCodedbufSize);
    PicturePtr picture(new VaapiEncPictureJPEG(m_context, surface, timeStamp));
    picture->m_codedBuffer = codedBuffer;
    ret = encodePicture(picture);
    if (ret != ENCODE_SUCCESS)
        return ret;
    INFO();
    if (!output(picture))
        return ENCODE_INVALID_PARAMS;
    return ENCODE_SUCCESS;
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
    qMatrix->load_lum_quantiser_matrix = 1;
  
    if (!m_hasQuantTable){
        for(int i=0; i<NUM_QUANT_ELEMENTS; i++) {
              qMatrix->lum_quantiser_matrix[i] = default_luminance_quant_table[zigzag_index[i]];
        }
        qMatrix->load_chroma_quantiser_matrix = 1;
        for(int i=0; i<NUM_QUANT_ELEMENTS; i++) {
              qMatrix->chroma_quantiser_matrix[i] = default_chrominance_quant_table[zigzag_index[i]];
        }
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
    JpegHuffmanTables *const hufTables = &m_hufTables;
    VaapiBufObject *object;
    uint32_t i, numTables;

    if (!m_hasHufTable)
        jpeg_get_default_huffman_tables(&m_hufTables);
    
    numTables = MIN(N_ELEMENTS(huffTableParam->huffman_table),
                    JPEG_MAX_SCAN_COMPONENTS);

    for (i = 0; i < numTables; i++) {
        huffTableParam->load_huffman_table[i] =
            hufTables->dc_tables[i].valid && hufTables->ac_tables[i].valid;
        if (!huffTableParam->load_huffman_table[i])
            continue;

        memcpy(huffTableParam->huffman_table[i].num_dc_codes,
               hufTables->dc_tables[i].huf_bits,
               sizeof(huffTableParam->huffman_table[i].num_dc_codes));
        memcpy(huffTableParam->huffman_table[i].dc_values,
               hufTables->dc_tables[i].huf_values,
               sizeof(huffTableParam->huffman_table[i].dc_values));
        memcpy(huffTableParam->huffman_table[i].num_ac_codes,
               hufTables->ac_tables[i].huf_bits,
               sizeof(huffTableParam->huffman_table[i].num_ac_codes));
        memcpy(huffTableParam->huffman_table[i].ac_values,
               hufTables->ac_tables[i].huf_values,
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
    VAEncPackedHeaderParameterBuffer packed_header_param_buffer;
    unsigned int length_in_bits;
    unsigned char *packed_header_buffer = NULL;
    //length_in_bits = build_packed_jpeg_header_buffer(&packed_header_buffer, width(), height(), 0);
    length_in_bits = buildJpegHeader(&packed_header_buffer, width(), height());
    
    if(!picture->addPackedHeader(VAEncPackedHeaderRawData, packed_header_buffer, length_in_bits))
        return false;
    return true;

}

Encode_Status VaapiEncoderJpeg::encodePicture(const PicturePtr &picture)
{
    Encode_Status ret = ENCODE_FAIL;
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
    return ENCODE_SUCCESS;
}

}
