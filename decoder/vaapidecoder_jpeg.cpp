/*
 *  vaapidecoder_jpeg.c - JPEG decoder
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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

/**
 * SECTION:vaapidecoder_jpeg
 * @short_description: JPEG decoder
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include "vaapidecoder_jpeg.h"
#include "codecparsers/bytereader.h"

namespace YamiMediaCodec{
static VAProfile convertToVaProfile(VaapiProfile profile)
{
    if (profile == VAAPI_PROFILE_JPEG_BASELINE)
        return VAProfileJPEGBaseline;
    else {
        ERROR("Not supported JPEG profile");
        return VAProfileNone;
    }
}

static uint32_t getMaxHorizontalSamples(JpegFrameHdr * frameHdr)
{
    uint32_t i, maxFactor = 0;

    for (i = 0; i < frameHdr->num_components; i++) {
        if (frameHdr->components[i].horizontal_factor > maxFactor)
            maxFactor = frameHdr->components[i].horizontal_factor;
    }
    return maxFactor;
}

static uint32_t getMaxVerticalSamples(JpegFrameHdr * frameHdr)
{
    uint32_t i, maxFactor = 0;

    for (i = 0; i < frameHdr->num_components; i++) {
        if (frameHdr->components[i].vertical_factor > maxFactor)
            maxFactor = frameHdr->components[i].vertical_factor;
    }
    return maxFactor;
}

VaapiDecoderJpeg::VaapiDecoderJpeg()
{
    m_profile = VAAPI_PROFILE_JPEG_BASELINE;
    m_width = 0;
    m_height = 0;
    m_hasHufTable = FALSE;
    m_hasQuantTable = FALSE;
    m_hasContext = FALSE;
    m_mcuRestart = 0;
    memset(&m_frameHdr, 0, sizeof(m_frameHdr));
    memset(&m_hufTables, 0, sizeof(m_hufTables));
    memset(&m_quantTables, 0, sizeof(m_quantTables));
}

VaapiDecoderJpeg::~VaapiDecoderJpeg()
{
}

Decode_Status
    VaapiDecoderJpeg::parseFrameHeader(uint8_t * buf, uint32_t bufSize)
{
    memset(&m_frameHdr, 0, sizeof(m_frameHdr));
    if (!jpeg_parse_frame_hdr(&m_frameHdr, buf, bufSize, 0)) {
        ERROR("failed to parse image");
        return DECODE_PARSER_FAIL;
    }
    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderJpeg::parseHuffmanTable(uint8_t * buf, uint32_t bufSize)
{
    if (!jpeg_parse_huffman_table(&m_hufTables, buf, bufSize, 0)) {
        DEBUG("failed to parse Huffman table");
        return DECODE_PARSER_FAIL;
    }

    m_hasHufTable = TRUE;
    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderJpeg::parseQuantTable(uint8_t * buf, uint32_t bufSize)
{
    if (!jpeg_parse_quant_table(&m_quantTables, buf, bufSize, 0)) {
        DEBUG("failed to parse quantization table");
        return DECODE_PARSER_FAIL;
    }
    m_hasQuantTable = TRUE;
    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderJpeg::parseRestartInterval(uint8_t * buf, uint32_t bufSize)
{
    if (!jpeg_parse_restart_interval(&m_mcuRestart, buf, bufSize, 0)) {
        DEBUG("failed to parse restart interval");
        return DECODE_PARSER_FAIL;
    }

    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderJpeg::parseScanHeader(JpegScanHdr * scanHdr, uint8_t * buf,
                                      uint32_t bufSize)
{
    if (!jpeg_parse_scan_hdr(scanHdr, buf, bufSize, 0)) {
        DEBUG("failed to parse restart interval");
        return DECODE_PARSER_FAIL;
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::fillPictureParam()
{
    VAPictureParameterBufferJPEGBaseline *vaPicParam = NULL;
    VaapiBufObject *object;
    uint32_t i;

    if (!m_picture->editPicture(vaPicParam))
        return DECODE_FAIL;

    vaPicParam->picture_width = m_frameHdr.width;
    vaPicParam->picture_height = m_frameHdr.height;
    vaPicParam->num_components = m_frameHdr.num_components;

    if (m_frameHdr.num_components > 4) {
        object->unmap();
        return DECODE_FAIL;
    }

    for (i = 0; i < vaPicParam->num_components; i++) {
        vaPicParam->components[i].component_id =
            m_frameHdr.components[i].identifier;
        vaPicParam->components[i].h_sampling_factor =
            m_frameHdr.components[i].horizontal_factor;
        vaPicParam->components[i].v_sampling_factor =
            m_frameHdr.components[i].vertical_factor;
        vaPicParam->components[i].quantiser_table_selector =
            m_frameHdr.components[i].quant_table_selector;
    }

    return DECODE_SUCCESS;
}

Decode_Status
    VaapiDecoderJpeg::fillSliceParam(JpegScanHdr * scanHdr,
                                     uint8_t * scanData, uint32_t scanDataSize)
{

    VASliceParameterBufferJPEGBaseline *sliceParam = NULL;
    uint32_t totalHSamples, totalVSamples;
    uint32_t i;

    assert(scanHdr);
    if (!(m_picture->newSlice(sliceParam, scanData, scanDataSize)))
        return DECODE_FAIL;

    sliceParam->num_components = scanHdr->num_components;
    for (i = 0; i < scanHdr->num_components; i++) {
        sliceParam->components[i].component_selector =
            scanHdr->components[i].component_selector;
        sliceParam->components[i].dc_table_selector =
            scanHdr->components[i].dc_selector;
        sliceParam->components[i].ac_table_selector =
            scanHdr->components[i].ac_selector;
    }
    sliceParam->restart_interval = m_mcuRestart;

    if (scanHdr->num_components == 1) { /*non-interleaved */
        sliceParam->slice_horizontal_position = 0;
        sliceParam->slice_vertical_position = 0;
        /* Y mcu numbers */
        if (sliceParam->components[0].component_selector ==
            m_frameHdr.components[0].identifier) {
            sliceParam->num_mcus =
                (m_frameHdr.width / 8) * (m_frameHdr.height / 8);
        } else {                /*Cr, Cb mcu numbers */
            sliceParam->num_mcus =
                (m_frameHdr.width / 16) * (m_frameHdr.height / 16);
        }
    } else {                    /* interleaved */
        sliceParam->slice_horizontal_position = 0;
        sliceParam->slice_vertical_position = 0;
        totalVSamples = getMaxVerticalSamples(&m_frameHdr);
        totalHSamples = getMaxHorizontalSamples(&m_frameHdr);
        sliceParam->num_mcus =
            ((m_frameHdr.width + totalHSamples * 8 -
              1) / (totalHSamples * 8)) * ((m_frameHdr.height +
                                            totalVSamples * 8 -
                                            1) / (totalVSamples * 8));
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::fillQuantizationTable()
{
    VAIQMatrixBufferJPEGBaseline *vaIqMatrix = NULL;
    VaapiBufObject *object;
    uint32_t i, j, numTables;

    if (!m_hasQuantTable)
        jpeg_get_default_quantization_tables(&m_quantTables);

    if (!(m_picture->editIqMatrix(vaIqMatrix)))
        return DECODE_FAIL;

    numTables = MIN(N_ELEMENTS(vaIqMatrix->quantiser_table),
                    JPEG_MAX_QUANT_ELEMENTS);

    for (i = 0; i < numTables; i++) {
        JpegQuantTable *const quantTable = &m_quantTables.quant_tables[i];

        vaIqMatrix->load_quantiser_table[i] = quantTable->valid;
        if (!vaIqMatrix->load_quantiser_table[i])
            continue;

        assert(quantTable->quant_precision == 0);
        for (j = 0; j < JPEG_MAX_QUANT_ELEMENTS; j++)
            vaIqMatrix->quantiser_table[i][j] = quantTable->quant_table[j];
        vaIqMatrix->load_quantiser_table[i] = 1;
        quantTable->valid = FALSE;
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::fillHuffmanTable()
{
    VAHuffmanTableBufferJPEGBaseline *vaHuffmanTable = NULL;
    JpegHuffmanTables *const hufTables = &m_hufTables;
    VaapiBufObject *object;
    uint32_t i, numTables;

    if (!m_hasHufTable)
        jpeg_get_default_huffman_tables(&m_hufTables);

    if (!(m_picture->editHufTable(vaHuffmanTable)))
        return DECODE_FAIL;

    numTables = MIN(N_ELEMENTS(vaHuffmanTable->huffman_table),
                    JPEG_MAX_SCAN_COMPONENTS);

    for (i = 0; i < numTables; i++) {
        vaHuffmanTable->load_huffman_table[i] =
            hufTables->dc_tables[i].valid && hufTables->ac_tables[i].valid;
        if (!vaHuffmanTable->load_huffman_table[i])
            continue;

        memcpy(vaHuffmanTable->huffman_table[i].num_dc_codes,
               hufTables->dc_tables[i].huf_bits,
               sizeof(vaHuffmanTable->huffman_table[i].num_dc_codes));
        memcpy(vaHuffmanTable->huffman_table[i].dc_values,
               hufTables->dc_tables[i].huf_values,
               sizeof(vaHuffmanTable->huffman_table[i].dc_values));
        memcpy(vaHuffmanTable->huffman_table[i].num_ac_codes,
               hufTables->ac_tables[i].huf_bits,
               sizeof(vaHuffmanTable->huffman_table[i].num_ac_codes));
        memcpy(vaHuffmanTable->huffman_table[i].ac_values,
               hufTables->ac_tables[i].huf_values,
               sizeof(vaHuffmanTable->huffman_table[i].ac_values));
        memset(vaHuffmanTable->huffman_table[i].pad,
               0, sizeof(vaHuffmanTable->huffman_table[i].pad));
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::decodePictureStart()
{
    Decode_Status status;
    VAProfile profile;

    assert(m_profile == VAAPI_PROFILE_JPEG_BASELINE);

    m_height = m_frameHdr.height;
    m_width = m_frameHdr.width;
    profile = convertToVaProfile(VAAPI_PROFILE_JPEG_BASELINE);

    if (!m_hasContext) {
        m_configBuffer.surfaceNumber = 2;
        m_configBuffer.profile = profile;
        m_configBuffer.width = m_width;
        m_configBuffer.height = m_height;
        VaapiDecoderBase::start(&m_configBuffer);
        m_hasContext = TRUE;
        return DECODE_FORMAT_CHANGE;
    } else if (m_configBuffer.profile != profile ||
               m_configBuffer.width != m_width ||
               m_configBuffer.height != m_height) {
        m_configBuffer.profile = profile;
        m_configBuffer.width = m_width;
        m_configBuffer.height = m_height;
        VaapiDecoderBase::reset(&m_configBuffer);
        return DECODE_FORMAT_CHANGE;
    }

    m_picture = createPicture(m_currentPTS);
    ASSERT(m_picture);
    if (!m_picture)
        return false;

    if (!m_picture) {
        ERROR("failed to allocate picture");
        return DECODE_MEMORY_FAIL;
    }

    /* Update presentation time */
    //picture->pts = pts;
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::decodePictureEnd()
{
    Decode_Status status = TRUE;

    if (!m_picture) {
        ERROR("There is no Vaapipicture for decoding.");
        return DECODE_FAIL;
    }

    if (!fillPictureParam()) {
        ERROR("Fail to fill quantization table.");
        return DECODE_FAIL;
    }

    if (!fillQuantizationTable()) {
        ERROR("Fail to fill quantization table.");
        return DECODE_FAIL;
    }

    if (!fillHuffmanTable()) {
        ERROR("Fail to fill huffman table.");
        return DECODE_FAIL;
    }

    m_picture->m_timeStamp = m_currentPTS;

    if (!m_picture->decode())
        status = DECODE_FAIL;
    else if (!outputPicture(m_picture))
        status = DECODE_FAIL;

    return status;
}

Decode_Status VaapiDecoderJpeg::decode(VideoDecodeBuffer * buffer)
{
    Decode_Status status = DECODE_SUCCESS;
    JpegMarkerSegment seg;
    JpegScanSegment scanSeg;
    BOOL appendEcs;
    uint8_t *buf;
    uint32_t bufSize;
    uint32_t ofs;

    m_currentPTS = buffer->timeStamp;
    buf = buffer->data;
    bufSize = buffer->size;

    if (!buf && bufSize == 0)
        return DECODE_FAIL;

    memset(&scanSeg, 0, sizeof(scanSeg));

    ofs = 0;
    while (jpeg_parse(&seg, buf, bufSize, ofs)) {
        if (seg.size < 0) {
            DEBUG("JPEG: buffer too short for parsing");
            return DECODE_PARSER_FAIL;
        }
        // seg.offset points to the byte after current marker (oxFFXY)
        ofs = seg.offset+seg.size;

        /* Decode scan, if complete */
        if (seg.marker == JPEG_MARKER_EOI && scanSeg.m_headerSize > 0) {
            scanSeg.m_dataSize = seg.offset - scanSeg.m_dataOffset;
            scanSeg.m_isValid = TRUE;
        }

        if (scanSeg.m_isValid) {
            JpegScanHdr scanHdr;
            status = parseScanHeader(&scanHdr,
                                     buf + scanSeg.m_headerOffset,
                                     scanSeg.m_headerSize);

            if (status != DECODE_SUCCESS) {
                ERROR("JPEG: fail to parser a scan header");
                break;
            }

            status = fillSliceParam(&scanHdr,
                                    buf + scanSeg.m_dataOffset,
                                    scanSeg.m_dataSize);

            if (status != DECODE_SUCCESS) {
                ERROR("JPEG: fail to fill slice param");
                break;
            }
        }

        appendEcs = TRUE;
        switch (seg.marker) {
        case JPEG_MARKER_SOI:
            m_hasQuantTable = FALSE;
            m_hasHufTable = FALSE;
            m_mcuRestart = 0;
            status = DECODE_SUCCESS;
            break;
        case JPEG_MARKER_EOI:
            /* Get out of the loop, trailing data is not needed */
            status = decodePictureEnd();
            break;
        case JPEG_MARKER_DHT:
            status = parseHuffmanTable(buf + seg.offset, seg.size);
            break;
        case JPEG_MARKER_DQT:
            status = parseQuantTable(buf + seg.offset, seg.size);
            break;
        case JPEG_MARKER_DRI:
            status = parseRestartInterval(buf + seg.offset, seg.size);
            break;
        case JPEG_MARKER_DAC:
            ERROR("JPEG: unsupported arithmetic coding mode");
            status = DECODE_FAIL;
            break;
        case JPEG_MARKER_SOS:
            scanSeg.m_headerOffset = seg.offset;
            scanSeg.m_headerSize = seg.size;
            scanSeg.m_dataOffset = seg.offset + seg.size;
            scanSeg.m_dataSize = 0;
            appendEcs = FALSE;
            break;
        default:
            /* Restart marker */
            if (seg.marker >= JPEG_MARKER_RST_MIN &&
                seg.marker <= JPEG_MARKER_RST_MAX) {
                appendEcs = FALSE;
                break;
            }

            /* Frame header */
            if (seg.marker >= JPEG_MARKER_SOF_MIN &&
                seg.marker <= JPEG_MARKER_SOF_MAX) {

                status = parseFrameHeader(buf + seg.offset, seg.size);
                if (status != DECODE_SUCCESS) {
                    ERROR("JPEG: fail to parse frame header");
                    return status;
                }

                status = decodePictureStart();
                if (status != DECODE_SUCCESS) {
                    if (status != DECODE_FORMAT_CHANGE)
                        ERROR("JPEG: fail to start picture decoding");
                    return status;
                }

                break;
            }

            /* Application segments */
            if (seg.marker >= JPEG_MARKER_APP_MIN &&
                seg.marker <= JPEG_MARKER_APP_MAX || seg.marker == JPEG_MARKER_COM) {
                status = DECODE_SUCCESS;
                break;
            }

            WARNING("unsupported marker (0x%02x)", seg.marker);
            status = DECODE_PARSER_FAIL;
            break;
        }

        /* Append entropy coded segments */
        if (appendEcs)
            scanSeg.m_dataSize = seg.offset - scanSeg.m_dataOffset;

        if (status != DECODE_SUCCESS) {
            ERROR("JPEG: unknown error");
            break;
        }
    }
  end:
    return status;
}

Decode_Status VaapiDecoderJpeg::start(VideoConfigBuffer * buffer)
{
    DEBUG("Jpeg: start()");

    if (buffer == NULL) {
        ERROR("No config information");
        return DECODE_SUCCESS;
    }

    if (buffer->width > 0 && buffer->height > 0) {
        if (!buffer->surfaceNumber)
            buffer->surfaceNumber = 2;

        if (buffer->profile == VAProfileNone)
            buffer->profile = VAProfileJPEGBaseline;

        m_configBuffer.width = buffer->width;
        m_configBuffer.height = buffer->height;
        m_configBuffer.profile = buffer->profile;

        VaapiDecoderBase::start(buffer);
        m_hasContext = true;
        return DECODE_FORMAT_CHANGE;
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderJpeg::reset(VideoConfigBuffer * buffer)
{
    DEBUG("Jpeg: reset()");

    if (m_picture) {
        m_picture.reset();
    }

    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderJpeg::stop(void)
{
    DEBUG("Jpeg: stop()");
    VaapiDecoderBase::stop();
}

void VaapiDecoderJpeg::flush(void)
{
    DEBUG("Jpeg: flush()");
    VaapiDecoderBase::flush();
}
}
