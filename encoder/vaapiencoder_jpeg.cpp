/*
 *  vaapiencoder_jpeg.cpp - jpeg encoder for va
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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
#include "jpegparser.h"
#include "jpegenc_utils.h"
#include <assert.h>
#include <stdio.h>

void populate_quantdata(JPEGQuantSection *quantVal, int type)
{
    uint8_t zigzag_qm[NUM_QUANT_ELEMENTS];
    int i;

    quantVal->DQT = DQT;
    quantVal->Pq = 0;
    quantVal->Tq = type;
    if(type == 0) {
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            zigzag_qm[i] = jpeg_luma_quant[jpeg_zigzag[i]];
        }

        memcpy(quantVal->Qk, zigzag_qm, NUM_QUANT_ELEMENTS);
    } else {
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            zigzag_qm[i] = jpeg_chroma_quant[jpeg_zigzag[i]];
        }
        memcpy(quantVal->Qk, zigzag_qm, NUM_QUANT_ELEMENTS);
    }
    quantVal->Lq = 3 + NUM_QUANT_ELEMENTS;
}

void populate_frame_header(JPEGFrameHeader *frameHdr, int picture_width, int picture_height)
{
    int i=0;

    frameHdr->SOF = SOF0;
    frameHdr->Lf = 8 + (3 * 3); //Size of FrameHeader in bytes without the Marker SOF
    frameHdr->P = 8;
    frameHdr->Y = picture_height;
    frameHdr->X = picture_width;
    frameHdr->Nf = 3;

    for(i=0; i<3; i++) {
        frameHdr->JPEGComponent[i].Ci = i+1;

        if(i == 0) {
            frameHdr->JPEGComponent[i].Hi = 2;
            frameHdr->JPEGComponent[i].Vi = 2;
            frameHdr->JPEGComponent[i].Tqi = 0;

        } else {
            //Analyzing the sampling factors for U/V, they are 1 for all formats except for Y8.
            //So, it is okay to have the code below like this. For Y8, we wont reach this code.
            frameHdr->JPEGComponent[i].Hi = 1;
            frameHdr->JPEGComponent[i].Vi = 1;
            frameHdr->JPEGComponent[i].Tqi = 1;
        }
    }
}

void populate_huff_section_header(JPEGHuffSection *huffSectionHdr, int th, int tc)
{
    int i=0, totalCodeWords=0;

    huffSectionHdr->DHT = DHT;
    huffSectionHdr->Tc = tc;
    huffSectionHdr->Th = th;

    if(th == 0) { //If Luma

        //If AC
        if(tc == 1) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_luma_ac+1, NUM_AC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_luma_ac+17, NUM_AC_CODE_WORDS_HUFFVAL);
        }

        //If DC
        if(tc == 0) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_luma_dc+1, NUM_DC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_luma_dc+17, NUM_DC_CODE_WORDS_HUFFVAL);
        }

        for(i=0; i<NUM_AC_RUN_SIZE_BITS; i++) {
            totalCodeWords += huffSectionHdr->Li[i];
        }

        huffSectionHdr->Lh = 3 + 16 + totalCodeWords;

    } else { //If Chroma
        //If AC
        if(tc == 1) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_chroma_ac+1, NUM_AC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_chroma_ac+17, NUM_AC_CODE_WORDS_HUFFVAL);
        }

        //If DC
        if(tc == 0) {
            memcpy(huffSectionHdr->Li, jpeg_hufftable_chroma_dc+1, NUM_DC_RUN_SIZE_BITS);
            memcpy(huffSectionHdr->Vij, jpeg_hufftable_chroma_dc+17, NUM_DC_CODE_WORDS_HUFFVAL);
        }

    }
}

void populate_scan_header(JPEGScanHeader *scanHdr)
{

    scanHdr->SOS = SOS;
    scanHdr->Ns = 3;

    //Y Component
    scanHdr->ScanComponent[0].Csj = 1;
    scanHdr->ScanComponent[0].Tdj = 0;
    scanHdr->ScanComponent[0].Taj = 0;

   
        //U Component
        scanHdr->ScanComponent[1].Csj = 2;
        scanHdr->ScanComponent[1].Tdj = 1;
        scanHdr->ScanComponent[1].Taj = 1;

        //V Component
        scanHdr->ScanComponent[2].Csj = 3;
        scanHdr->ScanComponent[2].Tdj = 1;
        scanHdr->ScanComponent[2].Taj = 1;
   

    scanHdr->Ss = 0;  //0 for Baseline
    scanHdr->Se = 63; //63 for Baseline
    scanHdr->Ah = 0;  //0 for Baseline
    scanHdr->Al = 0;  //0 for Baseline

    scanHdr->Ls = 3 + (3 * 2) + 3;

}

int build_packed_jpeg_header_buffer(unsigned char **header_buffer, int picture_width, int picture_height, uint16_t restart_interval)
{
    bitstream bs;
    int i=0, j=0;
    
    bitstream_start(&bs);
    
    //Add SOI
    bitstream_put_ui(&bs, SOI, 16);
    
    //Add AppData
    bitstream_put_ui(&bs, APP0, 16);  //APP0 marker
    bitstream_put_ui(&bs, 16, 16);    //Length excluding the marker
    bitstream_put_ui(&bs, 0x4A, 8);   //J
    bitstream_put_ui(&bs, 0x46, 8);   //F
    bitstream_put_ui(&bs, 0x49, 8);   //I
    bitstream_put_ui(&bs, 0x46, 8);   //F
    bitstream_put_ui(&bs, 0x00, 8);   //0
    bitstream_put_ui(&bs, 1, 8);      //Major Version
    bitstream_put_ui(&bs, 1, 8);      //Minor Version
    bitstream_put_ui(&bs, 1, 8);      //Density units 0:no units, 1:pixels per inch, 2: pixels per cm
    bitstream_put_ui(&bs, 72, 16);    //X density
    bitstream_put_ui(&bs, 72, 16);    //Y density
    bitstream_put_ui(&bs, 0, 8);      //Thumbnail width
    bitstream_put_ui(&bs, 0, 8);      //Thumbnail height
    
    //Add QTable - Y
    JPEGQuantSection quantLuma;
    populate_quantdata(&quantLuma, 0);

    bitstream_put_ui(&bs, quantLuma.DQT, 16);
    bitstream_put_ui(&bs, quantLuma.Lq, 16);
    bitstream_put_ui(&bs, quantLuma.Pq, 4);
    bitstream_put_ui(&bs, quantLuma.Tq, 4);
    for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
        bitstream_put_ui(&bs, quantLuma.Qk[i], 8);
    }

    //Add QTable - U/V
    
        JPEGQuantSection quantChroma;
        populate_quantdata(&quantChroma, 1);
        
        bitstream_put_ui(&bs, quantChroma.DQT, 16);
        bitstream_put_ui(&bs, quantChroma.Lq, 16);
        bitstream_put_ui(&bs, quantChroma.Pq, 4);
        bitstream_put_ui(&bs, quantChroma.Tq, 4);
        for(i=0; i<NUM_QUANT_ELEMENTS; i++) {
            bitstream_put_ui(&bs, quantChroma.Qk[i], 8);
        }
    
    
    //Add FrameHeader
    JPEGFrameHeader frameHdr;
    memset(&frameHdr,0,sizeof(JPEGFrameHeader));
    populate_frame_header(&frameHdr,  picture_width, picture_height);

    bitstream_put_ui(&bs, frameHdr.SOF, 16);
    bitstream_put_ui(&bs, frameHdr.Lf, 16);
    bitstream_put_ui(&bs, frameHdr.P, 8);
    bitstream_put_ui(&bs, frameHdr.Y, 16);
    bitstream_put_ui(&bs, frameHdr.X, 16);
    bitstream_put_ui(&bs, frameHdr.Nf, 8);
    for(i=0; i<frameHdr.Nf;i++) {
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Ci, 8);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Hi, 4);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Vi, 4);
        bitstream_put_ui(&bs, frameHdr.JPEGComponent[i].Tqi, 8);
    }

    //Add HuffTable AC and DC for Y,U/V components
    JPEGHuffSection acHuffSectionHdr, dcHuffSectionHdr;
        
    for(i=0; (i<3 && (i<=1)); i++) {
        //Add DC component (Tc = 0)
        populate_huff_section_header(&dcHuffSectionHdr, i, 0); 
        
        bitstream_put_ui(&bs, dcHuffSectionHdr.DHT, 16);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Lh, 16);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Tc, 4);
        bitstream_put_ui(&bs, dcHuffSectionHdr.Th, 4);
        for(j=0; j<NUM_DC_RUN_SIZE_BITS; j++) {
            bitstream_put_ui(&bs, dcHuffSectionHdr.Li[j], 8);
        }
        
        for(j=0; j<NUM_DC_CODE_WORDS_HUFFVAL; j++) {
            bitstream_put_ui(&bs, dcHuffSectionHdr.Vij[j], 8);
        }

        //Add AC component (Tc = 1)
        populate_huff_section_header(&acHuffSectionHdr, i, 1);
        
        bitstream_put_ui(&bs, acHuffSectionHdr.DHT, 16);
        bitstream_put_ui(&bs, acHuffSectionHdr.Lh, 16);
        bitstream_put_ui(&bs, acHuffSectionHdr.Tc, 4);
        bitstream_put_ui(&bs, acHuffSectionHdr.Th, 4);
        for(j=0; j<NUM_AC_RUN_SIZE_BITS; j++) {
            bitstream_put_ui(&bs, acHuffSectionHdr.Li[j], 8);
        }

        for(j=0; j<NUM_AC_CODE_WORDS_HUFFVAL; j++) {
            bitstream_put_ui(&bs, acHuffSectionHdr.Vij[j], 8);
        }

    }
    
    //Add Restart Interval if restart_interval is not 0
    if(restart_interval != 0) {
        JPEGRestartSection restartHdr;
        restartHdr.DRI = DRI;
        restartHdr.Lr = 4;
        restartHdr.Ri = restart_interval;

        bitstream_put_ui(&bs, restartHdr.DRI, 16); 
        bitstream_put_ui(&bs, restartHdr.Lr, 16);
        bitstream_put_ui(&bs, restartHdr.Ri, 16); 
    }
    
    //Add ScanHeader
    JPEGScanHeader scanHdr;
    populate_scan_header(&scanHdr);
 
    bitstream_put_ui(&bs, scanHdr.SOS, 16);
    bitstream_put_ui(&bs, scanHdr.Ls, 16);
    bitstream_put_ui(&bs, scanHdr.Ns, 8);
    
    for(i=0; i<scanHdr.Ns; i++) {
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Csj, 8);
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Tdj, 4);
        bitstream_put_ui(&bs, scanHdr.ScanComponent[i].Taj, 4);
    }

    bitstream_put_ui(&bs, scanHdr.Ss, 8);
    bitstream_put_ui(&bs, scanHdr.Se, 8);
    bitstream_put_ui(&bs, scanHdr.Ah, 4);
    bitstream_put_ui(&bs, scanHdr.Al, 4);

    bitstream_end(&bs);
    *header_buffer = (unsigned char *)bs.buffer;
    
    return bs.bit_offset;
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
Encode_Status VaapiEncoderJpeg::submitEncode()
{
    Encode_Status ret = ENCODE_SUCCESS;
    return ret;
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

    //LibVA expects the QM in zigzag order
    for(int i=0; i<NUM_QUANT_ELEMENTS; i++) {
        qMatrix->lum_quantiser_matrix[i] = jpeg_luma_quant[jpeg_zigzag[i]];
    }

    qMatrix->load_chroma_quantiser_matrix = 1;
    for(int i=0; i<NUM_QUANT_ELEMENTS; i++) {
        qMatrix->chroma_quantiser_matrix[i] = jpeg_chroma_quant[jpeg_zigzag[i]];
    }
    return TRUE;
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
    return TRUE;
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

bool VaapiEncoderJpeg::fill(VAHuffmanTableBufferJPEGBaseline *huffTableParam) const
{

    huffTableParam->load_huffman_table[0] = 1; //Load Luma Hufftable
    huffTableParam->load_huffman_table[1] = 1; //Load Chroma Hufftable for other formats

    //Load Luma hufftable values
    //Load DC codes
    memcpy(huffTableParam->huffman_table[0].num_dc_codes, jpeg_hufftable_luma_dc+1, 16);
    //Load DC Values
    memcpy(huffTableParam->huffman_table[0].dc_values, jpeg_hufftable_luma_dc+17, 12);
    //Load AC codes
    memcpy(huffTableParam->huffman_table[0].num_ac_codes, jpeg_hufftable_luma_ac+1, 16);
    //Load AC Values
    memcpy(huffTableParam->huffman_table[0].ac_values, jpeg_hufftable_luma_ac+17, 162);
    memset(huffTableParam->huffman_table[0].pad, 0, 2);


    //Load Chroma hufftable values if needed
    //Load DC codes
    memcpy(huffTableParam->huffman_table[1].num_dc_codes, jpeg_hufftable_chroma_dc+1, 16);
    //Load DC Values
    memcpy(huffTableParam->huffman_table[1].dc_values, jpeg_hufftable_chroma_dc+17, 12);
    //Load AC codes
    memcpy(huffTableParam->huffman_table[1].num_ac_codes, jpeg_hufftable_chroma_ac+1, 16);
    //Load AC Values
    memcpy(huffTableParam->huffman_table[1].ac_values, jpeg_hufftable_chroma_ac+17, 162);
    memset(huffTableParam->huffman_table[1].pad, 0, 2);
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
    length_in_bits = build_packed_jpeg_header_buffer(&packed_header_buffer, width(), height(), 0);
    
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
