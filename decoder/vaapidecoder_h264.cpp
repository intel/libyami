/*
 *  vaapidecoder_h264.cpp - h264 decoder 
 *
 *  Copyright (C) 2011-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li <xiaowei.li@intel.com>
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

#include "vaapidecoder_h264.h"
#include "codecparsers/bytereader.h"

#define MACROBLOCK_SIZE 16
#define MACROBLOCK_ALIGN (2 * MACROBLOCK_SIZE)
#define MB_ALIGN(arg) (((arg) + (MACROBLOCK_ALIGN - 1)) & (~(MACROBLOCK_ALIGN - 1)) )

static Decode_Status getStatus(H264ParserResult result)
{
    Decode_Status status;

    switch (result) {
    case H264_PARSER_OK:
        status = DECODE_SUCCESS;
        break;
    case H264_PARSER_NO_NAL_END:
        status = DECODE_INVALID_DATA;
        break;
    case H264_PARSER_ERROR:
        status = DECODE_PARSER_FAIL;
        break;
    default:
        status = DECODE_FAIL;
        break;
    }
    return status;
}

static VAProfile getH264VAProfile(H264PPS * pps)
{
    VAProfile profile = VAProfileH264High;
    H264SPS *const sps = pps->sequence;

    switch (sps->profile_idc) {
    case 66:
        if (sps->constraint_set1_flag ||
            (pps->num_slice_groups_minus1 == 0 &&
             !pps->redundant_pic_cnt_present_flag))
            profile = VAProfileH264ConstrainedBaseline;
        else
            profile = VAProfileH264Baseline;
        break;
    case 77:
    case 88:
        profile = VAProfileH264Main;
        break;
    case 100:
        profile = VAProfileH264High;
        break;
    }
    return profile;
}

static VaapiChromaType getH264ChromaType(H264SPS * sps)
{
    VaapiChromaType chromaType = VAAPI_CHROMA_TYPE_YUV420;

    switch (sps->chroma_format_idc) {
    case 0:
        chromaType = VAAPI_CHROMA_TYPE_YUV400;
        break;
    case 1:
        chromaType = VAAPI_CHROMA_TYPE_YUV420;
        break;
    case 2:
        chromaType = VAAPI_CHROMA_TYPE_YUV422;
        break;
    case 3:
        if (!sps->separate_colour_plane_flag)
            chromaType = VAAPI_CHROMA_TYPE_YUV444;
        break;
    }
    return chromaType;
}

static inline uint32_t
getSliceDataBitOffset(H264SliceHdr * sliceHdr, H264NalUnit * nalu)
{
    uint32_t epbCount;
    epbCount = sliceHdr->n_emulation_prevention_bytes;
    return 8 * sliceHdr->nal_header_bytes + sliceHdr->header_size -
        epbCount * 8;
}

static void
fillIqMatrix4x4(VAIQMatrixBufferH264 * iqMatrix, const H264PPS * pps)
{
    const uint8_t(*const ScalingList4x4)[6][16] = &pps->scaling_lists_4x4;
    uint32_t i, j;

    /* There are always 6 4x4 scaling lists */
    assert(N_ELEMENTS(iqMatrix->ScalingList4x4) == 6);
    assert(N_ELEMENTS(iqMatrix->ScalingList4x4[0]) == 16);

    if (sizeof(iqMatrix->ScalingList4x4[0][0]) == 1)
        memcpy(iqMatrix->ScalingList4x4, *ScalingList4x4,
               sizeof(iqMatrix->ScalingList4x4));
    else {
        for (i = 0; i < N_ELEMENTS(iqMatrix->ScalingList4x4); i++) {
            for (j = 0; j < N_ELEMENTS(iqMatrix->ScalingList4x4[i]); j++)
                iqMatrix->ScalingList4x4[i][j] = (*ScalingList4x4)[i][j];
        }
    }
}

static void
fillIqMatrix8x8(VAIQMatrixBufferH264 * iqMatrix, const H264PPS * pps)
{
    const uint8_t(*const ScalingList8x8)[6][64] = &pps->scaling_lists_8x8;
    const H264SPS *const sps = pps->sequence;
    uint32_t i, j, n;

    /* If chroma_format_idc != 3, there are up to 2 8x8 scaling lists */
    if (!pps->transform_8x8_mode_flag)
        return;

    assert(N_ELEMENTS(iqMatrix->ScalingList8x8) >= 2);
    assert(N_ELEMENTS(iqMatrix->ScalingList8x8[0]) == 64);

    if (sizeof(iqMatrix->ScalingList8x8[0][0]) == 1)
        memcpy(iqMatrix->ScalingList8x8, *ScalingList8x8,
               sizeof(iqMatrix->ScalingList8x8));
    else {
        n = (sps->chroma_format_idc != 3) ? 2 : 6;
        for (i = 0; i < n; i++) {
            for (j = 0; j < N_ELEMENTS(iqMatrix->ScalingList8x8[i]); j++)
                iqMatrix->ScalingList8x8[i][j] = (*ScalingList8x8)[i][j];
        }
    }
}

static inline int32_t
scanForStartCode(const uint8_t * data,
                 uint32_t offset, uint32_t size, uint32_t * scp)
{
/*
    ByteReader br;
    byte_reader_init(&br, data, size);
    return byte_reader_masked_scan_uint32(&br, 0xffffff00, 0x00000100, 0, size) - 1;
*/
    uint32_t i;
    const uint8_t *buf;

    if (offset + 3 > size)
        return -1;

    for (i = 0; i < size - offset - 3 + 1; i++) {
        buf = data + offset + i;
        if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
            return i;
    }

    return -1;
}

VaapiSliceH264::VaapiSliceH264(VADisplay display,
                               VAContextID ctx,
                               uint8_t * sliceData, uint32_t sliceSize)
{
    VASliceParameterBufferH264 *paramBuf;

    if (!display || !ctx) {
        ERROR("VA display or context not initialized yet");
        return;
    }

    /* new h264 slice data buffer */
    m_data = new VaapiBufObject(display,
                                ctx,
                                VASliceDataBufferType,
                                sliceData, sliceSize);

    /* new h264 slice parameter buffer */
    m_param = new VaapiBufObject(display,
                                 ctx,
                                 VASliceParameterBufferType,
                                 NULL, sizeof(VASliceParameterBufferH264));

    paramBuf = (VASliceParameterBufferH264 *) m_param->map();

    paramBuf->slice_data_size = sliceSize;
    paramBuf->slice_data_offset = 0;
    paramBuf->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    m_param->unmap();

    memset((void *) &m_sliceHdr, 0, sizeof(m_sliceHdr));

}

VaapiSliceH264::~VaapiSliceH264()
{
    if (m_data) {
        delete m_data;
        m_data = NULL;
    }

    if (m_param) {
        delete m_param;
        m_param = NULL;
    }
}

VaapiPictureH264::VaapiPictureH264(VADisplay display,
                                   VAContextID context,
                                   VaapiSurfaceBufferPool * surfBufPool,
                                   VaapiPictureStructure structure)
:  VaapiPicture(display, context, surfBufPool, structure)
{
    m_pps = NULL;
    m_fieldPoc[0] = 0;
    m_fieldPoc[1] = 0;
    m_frameNum = 0;
    m_frameNumWrap = 0;
    m_longTermFrameIdx = 0;
    m_picNum = 0;
    m_longTermPicNum = 0;
    m_outputFlag = 0;
    m_outputNeeded = 0;
    m_otherField = NULL;

    /* new h264 slice parameter buffer */
    m_picParam = new VaapiBufObject(display,
                                    context,
                                    VAPictureParameterBufferType,
                                    NULL,
                                    sizeof(VAPictureParameterBufferH264));
    if (!m_picParam)
        ERROR("create h264 picture parameter buffer object failed");

    m_iqMatrix = new VaapiBufObject(display,
                                    context,
                                    VAIQMatrixBufferType,
                                    NULL, sizeof(VAIQMatrixBufferH264));
    if (!m_iqMatrix)
        ERROR("create h264 iq matrix buffer object failed");

}

VaapiPictureH264 *VaapiPictureH264::newField()
{
    VaapiPictureH264 *field = NULL;

    field = new VaapiPictureH264(m_display, m_context, NULL, 0);
    if (!field)
        return NULL;

    field->attachSurfaceBuf(m_surfBufPool,
                            m_surfBuf,
                            VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD);

    field->m_fieldPoc[0] = m_fieldPoc[0];
    field->m_fieldPoc[1] = m_fieldPoc[1];
    field->m_frameNum = m_frameNum;

    return field;
}

VaapiFrameStore::VaapiFrameStore(VaapiPictureH264 * pic)
{
    m_structure = pic->m_structure;
    m_buffers[0] = pic;
    m_buffers[1] = NULL;
    m_numBuffers = 1;
    m_outputNeeded = pic->m_outputNeeded;
}

VaapiFrameStore::~VaapiFrameStore()
{
    uint32_t i;
    for (i = 0; i < m_numBuffers; i++)
        delete m_buffers[i];
}

void
 VaapiFrameStore::addPicture(VaapiPictureH264 * pic)
{
    uint8_t field;
    VaapiPictureH264 *const firstField = m_buffers[0];
    VaapiPictureH264 *secondField = pic;

    assert(m_numBuffers == 1);
    assert(pic->m_structure != VAAPI_PICTURE_STRUCTURE_FRAME);
    m_structure = VAAPI_PICTURE_STRUCTURE_FRAME;

    field = pic->m_structure == VAAPI_PICTURE_STRUCTURE_TOP_FIELD ? 0 : 1;

    assert(firstField->m_fieldPoc[field] != INT_MAX);
    firstField->m_fieldPoc[field] = secondField->m_fieldPoc[field];

    assert(pic->m_fieldPoc[!field] != INT_MAX);
    secondField->m_fieldPoc[!field] = firstField->m_fieldPoc[!field];

    m_buffers[1] = secondField;
    m_numBuffers = m_numBuffers + 1;
}

bool VaapiFrameStore::hasFrame()
{
    return m_structure == VAAPI_PICTURE_STRUCTURE_FRAME;
}

bool VaapiFrameStore::hasReference()
{
    uint32_t i;

    for (i = 0; i < m_numBuffers; i++) {
        if (!m_buffers[i])
            continue;
        if (VAAPI_PICTURE_IS_REFERENCE(m_buffers[i]))
            return true;
    }
    return false;
}

Decode_Status VaapiDecoderH264::decodeSPS(H264NalUnit * nalu)
{
    H264SPS *const sps = &m_lastSPS;
    H264ParserResult result;

    DEBUG("H264: decode SPS");

    memset(sps, 0, sizeof(*sps));
    result = h264_parser_parse_sps(&m_parser, nalu, sps, true);
    if (result != H264_PARSER_OK) {
        ERROR("parse sps failed");
        m_gotSPS = false;
        return getStatus(result);
    }

    m_gotSPS = true;

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH264::decodePPS(H264NalUnit * nalu)
{
    H264PPS *const pps = &m_lastPPS;
    H264ParserResult result;

    DEBUG("H264: decode PPS");

    memset(pps, 0, sizeof(*pps));
    result = h264_parser_parse_pps(&m_parser, nalu, pps);
    if (result != H264_PARSER_OK) {
        m_gotPPS = false;
        return getStatus(result);
    }

    m_gotPPS = true;
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH264::decodeSEI(H264NalUnit * nalu)
{
    H264SEIMessage sei;
    H264ParserResult result;

    DEBUG("H264: decode SEI");

    memset(&sei, 0, sizeof(sei));
    result = h264_parser_parse_sei(&m_parser, nalu, &sei);
    if (result != H264_PARSER_OK) {
        WARNING("failed to decode SEI, payload type:%d", sei.payloadType);
        return getStatus(result);
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH264::decodeSequenceEnd()
{
    Decode_Status status;

    DEBUG("H264: decode sequence-end");

    status = decodeCurrentPicture();
    if (status != DECODE_SUCCESS)
        return status;

    m_DPBManager->flushDPB();
    return DECODE_SUCCESS;
}

void VaapiDecoderH264::initPicturePOC0(VaapiPictureH264 * picture,
                                       H264SliceHdr * sliceHdr)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    const int32_t MaxPicOrderCntLsb =
        1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    int32_t tempPOC;

    if (VAAPI_H264_PICTURE_IS_IDR(picture)) {
        m_prevPOCMsb = 0;
        m_prevPOCLsb = 0;
    } else if (m_prevPicHasMMCO5) {
        m_prevPOCMsb = 0;
        m_prevPOCLsb =
            (m_prevPicStructure == VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD ?
             0 : m_fieldPoc[TOP_FIELD]);
    } else {
        m_prevPOCMsb = m_POCMsb;
        m_prevPOCLsb = m_POCLsb;
    }

    // (8-3)
    m_POCLsb = sliceHdr->pic_order_cnt_lsb;
    if (m_POCLsb < m_prevPOCLsb &&
        (m_prevPOCLsb - m_POCLsb) >= (MaxPicOrderCntLsb / 2))
        m_POCMsb = m_prevPOCMsb + MaxPicOrderCntLsb;
    else if (m_POCLsb > m_prevPOCLsb &&
             (m_POCLsb - m_prevPOCLsb) > (MaxPicOrderCntLsb / 2))
        m_POCMsb = m_prevPOCMsb - MaxPicOrderCntLsb;
    else
        m_POCMsb = m_prevPOCMsb;

    tempPOC = m_POCMsb + m_POCLsb;
    switch (picture->m_structure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        // (8-4, 8-5)
        m_fieldPoc[TOP_FIELD] = tempPOC;
        m_fieldPoc[BOTTOM_FIELD] = tempPOC +
            sliceHdr->delta_pic_order_cnt_bottom;
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        // (8-4)
        m_fieldPoc[TOP_FIELD] = tempPOC;
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        // (8-5)
        m_fieldPoc[BOTTOM_FIELD] = tempPOC;
        break;
    }
}

void VaapiDecoderH264::initPicturePOC1(VaapiPictureH264 * picture,
                                       H264SliceHdr * sliceHdr)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    const int32_t maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    int32_t prevFrameNumOffset, absFrameNum, expectedPOC;
    uint32_t i;

    if (m_prevPicHasMMCO5)
        prevFrameNumOffset = 0;
    else
        prevFrameNumOffset = m_frameNumOffset;

    // (8-6)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        m_frameNumOffset = 0;
    else if (m_prevFrameNum > m_frameNum)
        m_frameNumOffset = prevFrameNumOffset + maxFrameNum;
    else
        m_frameNumOffset = prevFrameNumOffset;

    // (8-7)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
        absFrameNum = m_frameNumOffset + m_frameNum;
    else
        absFrameNum = 0;
    if (!VAAPI_PICTURE_IS_REFERENCE(picture) && absFrameNum > 0)
        absFrameNum = absFrameNum - 1;

    if (absFrameNum > 0) {
        int32_t expectedDeltaPerPocCycle;
        int32_t pocCycleCnt, frameNumInPocCycle;

        expectedDeltaPerPocCycle = 0;
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            expectedDeltaPerPocCycle += sps->offset_for_ref_frame[i];

        // (8-8)
        pocCycleCnt = (absFrameNum - 1) /
            sps->num_ref_frames_in_pic_order_cnt_cycle;
        frameNumInPocCycle = (absFrameNum - 1) %
            sps->num_ref_frames_in_pic_order_cnt_cycle;

        // (8-9)
        expectedPOC = pocCycleCnt * expectedDeltaPerPocCycle;
        for (i = 0; (int32_t) i <= frameNumInPocCycle; i++)
            expectedPOC += sps->offset_for_ref_frame[i];
    } else
        expectedPOC = 0;
    if (!VAAPI_PICTURE_IS_REFERENCE(picture))
        expectedPOC += sps->offset_for_non_ref_pic;

    // (8-10)
    switch (picture->m_structure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        m_fieldPoc[TOP_FIELD] = expectedPOC +
            sliceHdr->delta_pic_order_cnt[0];
        m_fieldPoc[BOTTOM_FIELD] = m_fieldPoc[TOP_FIELD] +
            sps->offset_for_top_to_bottom_field +
            sliceHdr->delta_pic_order_cnt[1];
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        m_fieldPoc[TOP_FIELD] = expectedPOC +
            sliceHdr->delta_pic_order_cnt[0];
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        m_fieldPoc[BOTTOM_FIELD] = expectedPOC +
            sps->offset_for_top_to_bottom_field +
            sliceHdr->delta_pic_order_cnt[0];
        break;
    }
}

void VaapiDecoderH264::initPicturePOC2(VaapiPictureH264 * picture,
                                       H264SliceHdr * sliceHdr)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    const int32_t maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    int32_t prevFrameNumOffset, tempPOC;

    if (m_prevPicHasMMCO5)
        prevFrameNumOffset = 0;
    else
        prevFrameNumOffset = m_frameNumOffset;

    // (8-11)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        m_frameNumOffset = 0;
    else if (m_prevFrameNum > m_frameNum)
        m_frameNumOffset = prevFrameNumOffset + maxFrameNum;
    else
        m_frameNumOffset = prevFrameNumOffset;

    // (8-12)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        tempPOC = 0;
    else if (!VAAPI_PICTURE_IS_REFERENCE(picture))
        tempPOC = 2 * (m_frameNumOffset + m_frameNum) - 1;
    else
        tempPOC = 2 * (m_frameNumOffset + m_frameNum);

    // (8-13)
    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
        m_fieldPoc[TOP_FIELD] = tempPOC;
    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        m_fieldPoc[BOTTOM_FIELD] = tempPOC;
}

/* 8.2.1 - Decoding process for picture order count */
void VaapiDecoderH264::initPicturePOC(VaapiPictureH264 * picture,
                                      H264SliceHdr * sliceHdr)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;

    switch (sps->pic_order_cnt_type) {
    case 0:
        initPicturePOC0(picture, sliceHdr);
        break;
    case 1:
        initPicturePOC1(picture, sliceHdr);
        break;
    case 2:
        initPicturePOC2(picture, sliceHdr);
        break;
    }

    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
        picture->m_fieldPoc[TOP_FIELD] = m_fieldPoc[TOP_FIELD];
    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        picture->m_fieldPoc[BOTTOM_FIELD] = m_fieldPoc[BOTTOM_FIELD];

    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        picture->m_POC =
            MIN(picture->m_fieldPoc[0], picture->m_fieldPoc[1]);
    else
        picture->m_POC = picture->m_fieldPoc[TOP_FIELD];

}

bool VaapiDecoderH264::initPicture(VaapiPictureH264 * picture,
                                   H264SliceHdr * sliceHdr,
                                   H264NalUnit * nalu)
{
    H264SPS *const sps = sliceHdr->pps->sequence;

    m_prevFrameNum = m_frameNum;
    m_frameNum = sliceHdr->frame_num;
    picture->m_frameNum = m_frameNum;
    picture->m_frameNumWrap = m_frameNum;
    picture->m_outputFlag = true;   /* XXX: conformant to Annex A only */
    picture->m_timeStamp = m_currentPTS;

    /* Reset decoder state for IDR pictures */
    if (nalu->idr_pic_flag) {
        DEBUG("H264: IDR frame detected");
        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_IDR);
        m_DPBManager->flushDPB();
        m_prevFrame = NULL;
    }

    /* Initialize slice type */
    switch (sliceHdr->type % 5) {
    case H264_P_SLICE:
        picture->m_type = VAAPI_PICTURE_TYPE_P;
        break;
    case H264_B_SLICE:
        picture->m_type = VAAPI_PICTURE_TYPE_B;
        break;
    case H264_I_SLICE:
        picture->m_type = VAAPI_PICTURE_TYPE_I;
        break;
    case H264_SP_SLICE:
        picture->m_type = VAAPI_PICTURE_TYPE_SP;
        break;
    case H264_SI_SLICE:
        picture->m_type = VAAPI_PICTURE_TYPE_SI;
        break;
    }

    /* Initialize picture structure */
    if (!sliceHdr->field_pic_flag)
        picture->m_picStructure = VAAPI_PICTURE_STRUCTURE_FRAME;
    else {
        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_INTERLACED);
        if (!sliceHdr->bottom_field_flag)
            picture->m_picStructure = VAAPI_PICTURE_STRUCTURE_TOP_FIELD;
        else
            picture->m_picStructure = VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD;
    }
    picture->m_structure = picture->m_picStructure;

    /* Initialize reference flags */
    if (nalu->ref_idc) {
        H264DecRefPicMarking *const decRefPicMarking =
            &sliceHdr->dec_ref_pic_marking;

        if (VAAPI_H264_PICTURE_IS_IDR(picture) &&
            decRefPicMarking->long_term_reference_flag) {
            VAAPI_PICTURE_FLAG_SET(picture,
                                   VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE);

        } else {
            VAAPI_PICTURE_FLAG_SET(picture,
                                   VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE);
        }
    }

    initPicturePOC(picture, sliceHdr);

    m_DPBManager->initPictureRefs(picture, sliceHdr, m_frameNum);
    return true;
}

/* fetch and fill picture paramter buffer */
void VaapiDecoderH264::vaapiInitPicture(VAPictureH264 * pic)
{
    pic->picture_id = VA_INVALID_ID;
    pic->frame_idx = 0;
    pic->flags = VA_PICTURE_H264_INVALID;
    pic->TopFieldOrderCnt = 0;
    pic->BottomFieldOrderCnt = 0;
}

void VaapiDecoderH264::vaapiFillPicture(VAPictureH264 * pic,
                                        VaapiPictureH264 * picture,
                                        uint32_t pictureStructure)
{
    pic->picture_id = picture->m_surfaceID;
    pic->flags = 0;

    if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture)) {
        pic->flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;
        pic->frame_idx = picture->m_longTermFrameIdx;
    } else {
        if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
            pic->flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        pic->frame_idx = picture->m_frameNum;
    }

    if (!pictureStructure)
        pictureStructure = picture->m_structure;

    switch (pictureStructure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        pic->TopFieldOrderCnt = picture->m_fieldPoc[TOP_FIELD];
        pic->BottomFieldOrderCnt = picture->m_fieldPoc[BOTTOM_FIELD];
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        pic->flags |= VA_PICTURE_H264_TOP_FIELD;
        pic->TopFieldOrderCnt = picture->m_fieldPoc[TOP_FIELD];
        pic->BottomFieldOrderCnt = 0;
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        pic->flags |= VA_PICTURE_H264_BOTTOM_FIELD;
        pic->BottomFieldOrderCnt = picture->m_fieldPoc[BOTTOM_FIELD];
        pic->TopFieldOrderCnt = 0;
        break;
    }
}

bool VaapiDecoderH264::fillPicture(VaapiPictureH264 * picture,
                                   H264SliceHdr * sliceHdr,
                                   H264NalUnit * nalu)
{
    uint32_t i, n;
    H264PPS *const pps = picture->m_pps;
    H264SPS *const sps = pps->sequence;
    VaapiDecPicBufLayer *DPBLayer = m_DPBManager->DPBLayer;
    VaapiBufObject *picParamObj = picture->m_picParam;

    VAPictureParameterBufferH264 *picParam =
        (VAPictureParameterBufferH264 *) picParamObj->map();

    /* Fill in VAPictureParameterBufferH264 */
    vaapiFillPicture(&picParam->CurrPic, picture, 0);

    for (i = 0, n = 0; i < DPBLayer->DPBCount; i++) {
        VaapiFrameStore *const frameStore = DPBLayer->DPB[i];
        if (frameStore && frameStore->hasReference())
            vaapiFillPicture(&picParam->ReferenceFrames[n++],
                             frameStore->m_buffers[0],
                             frameStore->m_structure);
    }

    for (; n < N_ELEMENTS(picParam->ReferenceFrames); n++)
        vaapiInitPicture(&picParam->ReferenceFrames[n]);

#define COPY_FIELD(s, f) \
    picParam->f = (s)->f

#define COPY_BFM(a, s, f) \
    picParam->a.bits.f = (s)->f

    picParam->picture_width_in_mbs_minus1 = m_mbWidth - 1;
    picParam->picture_height_in_mbs_minus1 = m_mbHeight - 1;
    picParam->frame_num = m_frameNum;

    COPY_FIELD(sps, bit_depth_luma_minus8);
    COPY_FIELD(sps, bit_depth_chroma_minus8);
    COPY_FIELD(sps, num_ref_frames);
    COPY_FIELD(pps, num_slice_groups_minus1);
    COPY_FIELD(pps, slice_group_map_type);
    COPY_FIELD(pps, slice_group_change_rate_minus1);
    COPY_FIELD(pps, pic_init_qp_minus26);
    COPY_FIELD(pps, pic_init_qs_minus26);
    COPY_FIELD(pps, chroma_qp_index_offset);
    COPY_FIELD(pps, second_chroma_qp_index_offset);

    picParam->seq_fields.value = 0; /* reset all bits */
    picParam->seq_fields.bits.residual_colour_transform_flag =
        sps->separate_colour_plane_flag;
    picParam->seq_fields.bits.MinLumaBiPredSize8x8 = sps->level_idc >= 31;  /* A.3.3.2 */

    COPY_BFM(seq_fields, sps, chroma_format_idc);
    COPY_BFM(seq_fields, sps, gaps_in_frame_num_value_allowed_flag);
    COPY_BFM(seq_fields, sps, frame_mbs_only_flag);
    COPY_BFM(seq_fields, sps, mb_adaptive_frame_field_flag);
    COPY_BFM(seq_fields, sps, direct_8x8_inference_flag);
    COPY_BFM(seq_fields, sps, log2_max_frame_num_minus4);
    COPY_BFM(seq_fields, sps, pic_order_cnt_type);
    COPY_BFM(seq_fields, sps, log2_max_pic_order_cnt_lsb_minus4);
    COPY_BFM(seq_fields, sps, delta_pic_order_always_zero_flag);

    picParam->pic_fields.value = 0; /* reset all bits */
    picParam->pic_fields.bits.field_pic_flag = sliceHdr->field_pic_flag;
    picParam->pic_fields.bits.reference_pic_flag =
        VAAPI_PICTURE_IS_REFERENCE(picture);

    COPY_BFM(pic_fields, pps, entropy_coding_mode_flag);
    COPY_BFM(pic_fields, pps, weighted_pred_flag);
    COPY_BFM(pic_fields, pps, weighted_bipred_idc);
    COPY_BFM(pic_fields, pps, transform_8x8_mode_flag);
    COPY_BFM(pic_fields, pps, constrained_intra_pred_flag);
    COPY_BFM(pic_fields, pps, pic_order_present_flag);
    COPY_BFM(pic_fields, pps, deblocking_filter_control_present_flag);
    COPY_BFM(pic_fields, pps, redundant_pic_cnt_present_flag);
    picParamObj->unmap();

    return true;
}

/* fill slice parameter buffers functions*/
bool VaapiDecoderH264::ensureQuantMatrix(VaapiPictureH264 * pic)
{
    H264PPS *const pps = pic->m_pps;
    H264SPS *const sps = pps->sequence;
    VAIQMatrixBufferH264 *iqMatrix;

    /* we can only support 4:2:0 or 4:2:2 since ScalingLists8x8[]
       is not large enough to hold lists for 4:4:4 */
    if (sps->chroma_format_idc == 3)
        return false;

    iqMatrix = (VAIQMatrixBufferH264 *) pic->m_iqMatrix->map();

    fillIqMatrix4x4(iqMatrix, pps);
    fillIqMatrix8x8(iqMatrix, pps);
    pic->m_iqMatrix->unmap();
    return true;
}

bool VaapiDecoderH264::fillPredWeightTable(VaapiSliceH264 * slice)
{
    int32_t i, j;
    uint32_t numWeightTables = 0;
    H264SliceHdr *const sliceHdr = &slice->m_sliceHdr;
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    H264PredWeightTable *const w = &sliceHdr->pred_weight_table;

    VaapiBufObject *sliceParamObj = slice->m_param;

    VASliceParameterBufferH264 *sliceParam =
        (VASliceParameterBufferH264 *) sliceParamObj->map();

    if (pps->weighted_pred_flag &&
        (H264_IS_P_SLICE(sliceHdr) || H264_IS_SP_SLICE(sliceHdr)))
        numWeightTables = 1;
    else if (pps->weighted_bipred_idc == 1 && H264_IS_B_SLICE(sliceHdr))
        numWeightTables = 2;
    else
        numWeightTables = 0;

    sliceParam->luma_log2_weight_denom = w->luma_log2_weight_denom;
    sliceParam->chroma_log2_weight_denom = w->chroma_log2_weight_denom;
    sliceParam->luma_weight_l0_flag = 0;
    sliceParam->chroma_weight_l0_flag = 0;
    sliceParam->luma_weight_l1_flag = 0;
    sliceParam->chroma_weight_l1_flag = 0;

    if (numWeightTables < 1)
        goto out;

    sliceParam->luma_weight_l0_flag = 1;
    for (i = 0; i <= sliceParam->num_ref_idx_l0_active_minus1; i++) {
        sliceParam->luma_weight_l0[i] = w->luma_weight_l0[i];
        sliceParam->luma_offset_l0[i] = w->luma_offset_l0[i];
    }

    sliceParam->chroma_weight_l0_flag = sps->chroma_array_type != 0;
    if (sliceParam->chroma_weight_l0_flag) {
        for (i = 0; i <= sliceParam->num_ref_idx_l0_active_minus1; i++) {
            for (j = 0; j < 2; j++) {
                sliceParam->chroma_weight_l0[i][j] =
                    w->chroma_weight_l0[i][j];
                sliceParam->chroma_offset_l0[i][j] =
                    w->chroma_offset_l0[i][j];
            }
        }
    }

    if (numWeightTables < 2)
        goto out;

    sliceParam->luma_weight_l1_flag = 1;
    for (i = 0; i <= sliceParam->num_ref_idx_l1_active_minus1; i++) {
        sliceParam->luma_weight_l1[i] = w->luma_weight_l1[i];
        sliceParam->luma_offset_l1[i] = w->luma_offset_l1[i];
    }

    sliceParam->chroma_weight_l1_flag = sps->chroma_array_type != 0;
    if (sliceParam->chroma_weight_l1_flag) {
        for (i = 0; i <= sliceParam->num_ref_idx_l1_active_minus1; i++) {
            for (j = 0; j < 2; j++) {
                sliceParam->chroma_weight_l1[i][j] =
                    w->chroma_weight_l1[i][j];
                sliceParam->chroma_offset_l1[i][j] =
                    w->chroma_offset_l1[i][j];
            }
        }
    }

  out:
    sliceParamObj->unmap();
    return true;
}

bool VaapiDecoderH264::fillRefPicList(VaapiSliceH264 * slice)
{
    uint32_t i, numRefLists = 0;
    VaapiDecPicBufLayer *DPBLayer = m_DPBManager->DPBLayer;
    H264SliceHdr *const sliceHdr = &slice->m_sliceHdr;
    VaapiBufObject *sliceParamObj = slice->m_param;
    VASliceParameterBufferH264 *sliceParam =
        (VASliceParameterBufferH264 *) sliceParamObj->map();

    sliceParam->num_ref_idx_l0_active_minus1 = 0;
    sliceParam->num_ref_idx_l1_active_minus1 = 0;

    if (H264_IS_B_SLICE(sliceHdr))
        numRefLists = 2;
    else if (H264_IS_I_SLICE(sliceHdr))
        numRefLists = 0;
    else
        numRefLists = 1;

    if (numRefLists < 1)
        goto out;

    sliceParam->num_ref_idx_l0_active_minus1 =
        sliceHdr->num_ref_idx_l0_active_minus1;

    for (i = 0;
         i < DPBLayer->refPicList0Count && DPBLayer->refPicList0[i]; i++)
        vaapiFillPicture(&sliceParam->RefPicList0[i],
                         DPBLayer->refPicList0[i], 0);
    for (; i <= sliceParam->num_ref_idx_l0_active_minus1; i++)
        vaapiInitPicture(&sliceParam->RefPicList0[i]);

    if (numRefLists < 2)
        goto out;

    sliceParam->num_ref_idx_l1_active_minus1 =
        sliceHdr->num_ref_idx_l1_active_minus1;

    for (i = 0;
         i < DPBLayer->refPicList1Count && DPBLayer->refPicList1[i]; i++)
        vaapiFillPicture(&sliceParam->RefPicList1[i],
                         DPBLayer->refPicList1[i], 0);
    for (; i <= sliceParam->num_ref_idx_l1_active_minus1; i++)
        vaapiInitPicture(&sliceParam->RefPicList1[i]);

  out:
    sliceParamObj->unmap();
    return true;
}

bool VaapiDecoderH264::fillSlice(VaapiSliceH264 * slice,
                                 H264NalUnit * nalu)
{
    H264SliceHdr *const sliceHdr = &slice->m_sliceHdr;
    VaapiBufObject *sliceParamObj = slice->m_param;
    VASliceParameterBufferH264 *sliceParam =
        (VASliceParameterBufferH264 *) sliceParamObj->map();

    /* Fill in VASliceParameterBufferH264 */
    sliceParam->slice_data_bit_offset =
        getSliceDataBitOffset(sliceHdr, nalu);
    sliceParam->first_mb_in_slice = sliceHdr->first_mb_in_slice;
    sliceParam->slice_type = sliceHdr->type % 5;
    sliceParam->direct_spatial_mv_pred_flag =
        sliceHdr->direct_spatial_mv_pred_flag;
    sliceParam->cabac_init_idc = sliceHdr->cabac_init_idc;
    sliceParam->slice_qp_delta = sliceHdr->slice_qp_delta;
    sliceParam->disable_deblocking_filter_idc =
        sliceHdr->disable_deblocking_filter_idc;
    sliceParam->slice_alpha_c0_offset_div2 =
        sliceHdr->slice_alpha_c0_offset_div2;
    sliceParam->slice_beta_offset_div2 = sliceHdr->slice_beta_offset_div2;

    sliceParamObj->unmap();

    if (!fillRefPicList(slice))
        return false;
    if (!fillPredWeightTable(slice))
        return false;
    return true;
}

Decode_Status VaapiDecoderH264::ensureContext(H264PPS * pps)
{
    H264SPS *const sps = pps->sequence;
    VAProfile parsedProfile;
    VaapiChromaType parsedChroma;
    uint32_t mbWidth, mbHeight;
    bool resetContext = false;
    uint32_t DPBSize = 0;
    Decode_Status status;

    m_progressiveSequence = sps->frame_mbs_only_flag;

    if (!m_DPBManager) {
        DPBSize = getMaxDecFrameBuffering(sps, 1);
        m_DPBManager = new VaapiDPBManager(DPBSize);
    }

    VideoConfigBuffer *config = &m_configBuffer;
    parsedProfile = getH264VAProfile(pps);
    if (parsedProfile != m_configBuffer.profile) {
        DEBUG("H264: profile changed: old = %d, new = %d, \n",
              m_configBuffer.profile, parsedProfile);
        m_configBuffer.profile = parsedProfile;
        m_configBuffer.flag |= HAS_VA_PROFILE;
        resetContext = true;
    }

    /*
       parsedChroma = getH264ChromaType(sps);
       if (parsedChroma != m_chromaType) {
       WARNING("ensure context: chroma changed !\n");
       m_chromaType = parsedChroma;
       resetContext = true;
       }
     */

    mbWidth = sps->pic_width_in_mbs_minus1 + 1;
    mbHeight = (sps->pic_height_in_map_units_minus1 + 1) <<
        !sps->frame_mbs_only_flag;

    if (mbWidth != m_mbWidth || mbHeight != m_mbHeight) {
        DEBUG("H264: resolution changed: Orig w=%d, h=%d; New w=%d, h=%d",
              m_mbWidth * 16, m_mbHeight * 16, mbWidth * 16,
              mbHeight * 16);

        m_mbWidth = mbWidth;
        m_mbHeight = mbHeight;
        m_configBuffer.width = mbWidth * 16;
        m_configBuffer.height = mbHeight * 16;
        resetContext = true;
    }

    if (!resetContext && m_hasContext)
        return DECODE_SUCCESS;

    if (!m_hasContext) {
        DPBSize = getMaxDecFrameBuffering(sps, 1);
        m_configBuffer.surfaceNumber = DPBSize + H264_EXTRA_SURFACE_NUMBER;
        m_configBuffer.flag |= HAS_SURFACE_NUMBER;
        status = VaapiDecoderBase::start(&m_configBuffer);
        if (status != DECODE_SUCCESS)
            return status;

        DEBUG("First time to Start VA context");
        m_resetContext = true;
    } else if (resetContext) {
        m_hasContext = false;
        status = VaapiDecoderBase::reset(&m_configBuffer);
        if (status != DECODE_SUCCESS)
            return status;

        if (m_DPBManager)
            m_DPBManager->resetDPB(sps);

        DEBUG("Re-start VA context");
        m_resetContext = true;
    }

    m_hasContext = true;

    if (resetContext)
        return DECODE_FORMAT_CHANGE;

    return DECODE_SUCCESS;
}

bool VaapiDecoderH264::isNewPicture(H264NalUnit * nalu,
                                    H264SliceHdr * sliceHdr)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    VaapiSliceH264 *slice;
    H264SliceHdr *prevSliceHdr;

    if (!m_currentPicture)
        return true;

    slice = (VaapiSliceH264 *) (m_currentPicture->getLastSlice());
    if (!slice)
        return false;
    prevSliceHdr = &slice->m_sliceHdr;

#define CHECK_EXPR(expr, field_name) do {              \
        if (!(expr)) {                                 \
            DEBUG(field_name " differs in value"); \
            return true;                               \
        }                                              \
    } while (0)

#define CHECK_VALUE(newSliceHdr, oldSliceHdr, field) \
    CHECK_EXPR(((newSliceHdr)->field == (oldSliceHdr)->field), #field)

    /* frame_num differs in value, regardless of inferred values to 0 */
    CHECK_VALUE(sliceHdr, prevSliceHdr, frame_num);

    /* picParameter_set_id differs in value */
    CHECK_VALUE(sliceHdr, prevSliceHdr, pps);

    /* field_pic_flag differs in value */
    CHECK_VALUE(sliceHdr, prevSliceHdr, field_pic_flag);

    /* bottom_field_flag is present in both and differs in value */
    if (sliceHdr->field_pic_flag && prevSliceHdr->field_pic_flag)
        CHECK_VALUE(sliceHdr, prevSliceHdr, bottom_field_flag);

    /* nal_ref_idc differs in value with one of the nal_ref_idc values is 0 */
    CHECK_EXPR(((VAAPI_PICTURE_IS_REFERENCE(m_currentPicture) ^
                 (nalu->ref_idc != 0)) == 0), "nal_ref_idc");

    /* POC type is 0 for both and either pic_order_cnt_lsb differs in
       value or delta_pic_order_cnt_bottom differs in value */
    if (sps->pic_order_cnt_type == 0) {
        CHECK_VALUE(sliceHdr, prevSliceHdr, pic_order_cnt_lsb);
        if (pps->pic_order_present_flag && !sliceHdr->field_pic_flag)
            CHECK_VALUE(sliceHdr, prevSliceHdr,
                        delta_pic_order_cnt_bottom);
    }

    /* POC type is 1 for both and either delta_pic_order_cnt[0]
       differs in value or delta_pic_order_cnt[1] differs in value */
    else if (sps->pic_order_cnt_type == 1) {
        CHECK_VALUE(sliceHdr, prevSliceHdr, delta_pic_order_cnt[0]);
        CHECK_VALUE(sliceHdr, prevSliceHdr, delta_pic_order_cnt[1]);
    }

    /* IdrPicFlag differs in value */
    CHECK_EXPR(((VAAPI_H264_PICTURE_IS_IDR(m_currentPicture) ^
                 (nalu->type == H264_NAL_SLICE_IDR)) == 0), "IdrPicFlag");

    /* IdrPicFlag is equal to 1 for both and idr_pic_id differs in value */
    if (VAAPI_H264_PICTURE_IS_IDR(m_currentPicture))
        CHECK_VALUE(sliceHdr, prevSliceHdr, idr_pic_id);

#undef CHECK_EXPR
#undef CHECK_VALUE
    return false;
}

bool VaapiDecoderH264::markingPicture(VaapiPictureH264 * pic)
{
    if (!m_DPBManager->execRefPicMarking(pic, &m_prevPicHasMMCO5))
        return false;

    if (m_prevPicHasMMCO5) {
        m_frameNum = 0;
        m_frameNumOffset = 0;
        m_prevFrame = NULL;
    }

    m_prevPicStructure = pic->m_structure;

    return true;
}

bool VaapiDecoderH264::storeDecodedPicture(VaapiPictureH264 * pic)
{
    VaapiFrameStore *frameStore;
    // Check if picture is the second field and the first field is still in DPB
    if (m_prevFrame && !m_prevFrame->hasFrame()) {
        RETURN_VAL_IF_FAIL(m_prevFrame->m_numBuffers == 1, false);

        m_prevFrame->addPicture(m_currentPicture);
        m_currentPicture = NULL;
        return true;
    }
    // Create new frame store, and split fields if necessary
    frameStore = new VaapiFrameStore(pic);
    if (!frameStore)
        return false;

    m_prevFrame = frameStore;

    if (!m_DPBManager->addDPB(frameStore, pic))
        return false;

    if (m_prevFrame && m_prevFrame->hasFrame())
        m_currentPicture = NULL;

    return true;
}

Decode_Status VaapiDecoderH264::decodeCurrentPicture()
{
    Decode_Status status;

    if (!m_currentPicture)
        return DECODE_SUCCESS;

    status = ensureContext(m_currentPicture->m_pps);
    if (status != DECODE_SUCCESS)
        goto error;

    if (!markingPicture(m_currentPicture))
        goto error;

    if (!m_currentPicture->decodePicture())
        goto error;

    if (!storeDecodedPicture(m_currentPicture))
        goto error;

    return DECODE_SUCCESS;

  error:
    delete m_currentPicture;
    m_currentPicture = NULL;
    return DECODE_FAIL;
}

Decode_Status
    VaapiDecoderH264::decodePicture(H264NalUnit * nalu,
                                    H264SliceHdr * sliceHdr)
{
    VaapiPictureH264 *picture;
    Decode_Status status;
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;

    status = decodeCurrentPicture();
    if (status != DECODE_SUCCESS)
        return status;

    if (m_currentPicture) {
        /* Re-use current picture where the first field was decoded */
        picture = m_currentPicture->newField();
        if (!picture) {
            ERROR("failed to allocate field picture");
            m_currentPicture = NULL;
            return DECODE_FAIL;
        }

    } else {
        /*accquire one surface from m_bufPool in base decoder  */
        picture = new VaapiPictureH264(m_VADisplay,
                                       m_VAContext,
                                       m_bufPool,
                                       VAAPI_PICTURE_STRUCTURE_FRAME);

        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_FF);

        /* hack code here */
    }
    m_currentPicture = picture;

    picture->m_pps = pps;

    status = ensureQuantMatrix(picture);
    if (status != DECODE_SUCCESS) {
        ERROR("failed to reset quantizer matrix");
        return status;
    }
    if (!initPicture(picture, sliceHdr, nalu))
        return DECODE_FAIL;
    if (!fillPicture(picture, sliceHdr, nalu))
        return DECODE_FAIL;

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH264::decodeSlice(H264NalUnit * nalu)
{
    Decode_Status status;
    VaapiPictureH264 *picture;
    VaapiSliceH264 *slice = NULL;
    H264SliceHdr *sliceHdr;
    H264SliceHdr tmpSliceHdr;
    H264ParserResult result;

    /* parser the slice header info */
    memset((void *) &tmpSliceHdr, 0, sizeof(tmpSliceHdr));
    result = h264_parser_parse_slice_hdr(&m_parser, nalu,
                                         &tmpSliceHdr, true, true);
    if (result != H264_PARSER_OK) {
        status = getStatus(result);
        goto error;
    }

    /* check info and reset VA resource if necessary */
    status = ensureContext(tmpSliceHdr.pps);
    if (status != DECODE_SUCCESS)
        return status;

    /* construct slice and parsing slice header */
    slice = new VaapiSliceH264(m_VADisplay,
                               m_VAContext,
                               nalu->data + nalu->offset, nalu->size);
    sliceHdr = &(slice->m_sliceHdr);

    memcpy((void *) sliceHdr, (void *) &tmpSliceHdr, sizeof(*sliceHdr));

    if (isNewPicture(nalu, sliceHdr)) {
        status = decodePicture(nalu, sliceHdr);
        if (status != DECODE_SUCCESS)
            goto error;
    }

    if (!fillSlice(slice, nalu)) {
        status = DECODE_FAIL;
        goto error;
    }

    m_currentPicture->addSlice((VaapiSlice *) slice);

    return DECODE_SUCCESS;

  error:
    if (slice)
        delete slice;
    return status;
}

Decode_Status VaapiDecoderH264::decodeNalu(H264NalUnit * nalu)
{
    Decode_Status status;

    switch (nalu->type) {
    case H264_NAL_SLICE_IDR:
        /* fall-through. IDR specifics are handled in initPicture() */
    case H264_NAL_SLICE:
        if (!m_gotSPS || !m_gotPPS)
            return DECODE_SUCCESS;
        status = decodeSlice(nalu);
        break;
    case H264_NAL_SPS:
        status = decodeSPS(nalu);
        break;
    case H264_NAL_PPS:
        status = decodePPS(nalu);
        break;
    case H264_NAL_SEI:
        status = decodeSEI(nalu);
        break;
    case H264_NAL_SEQ_END:
        status = decodeSequenceEnd();
        break;
    case H264_NAL_AU_DELIMITER:
        /* skip all Access Unit NALs */
        status = DECODE_SUCCESS;
        break;
    case H264_NAL_FILLER_DATA:
        /* skip all Filler Data NALs */
        status = DECODE_SUCCESS;
        break;
    default:
        WARNING("unsupported NAL unit type %d", nalu->type);
        status = DECODE_PARSER_FAIL;
        break;
    }

    return status;
}

bool VaapiDecoderH264::decodeCodecData(uint8_t * buf, uint32_t bufSize)
{
    Decode_Status status;
    H264NalUnit nalu;
    H264ParserResult result;
    uint32_t i, ofs, numSPS, numPPS;

    DEBUG("H264: codec data detected");

    if (!buf || bufSize == 0)
        return false;

    if (bufSize < 8)
        return false;

    if (buf[0] != 1) {
        ERROR("failed to decode codec-data, not in avcC format");
        return false;
    }

    m_nalLengthSize = (buf[4] & 0x03) + 1;

    numSPS = buf[5] & 0x1f;
    ofs = 6;

    for (i = 0; i < numSPS; i++) {
        result = h264_parser_identify_nalu_avc(&m_parser,
                                               buf, ofs, bufSize, 2,
                                               &nalu);
        if (result != H264_PARSER_OK)
            return false;

        status = decodeSPS(&nalu);
        if (status != DECODE_SUCCESS)
            return false;
        ofs = nalu.offset + nalu.size;
    }

    numPPS = buf[ofs];
    ofs++;

    for (i = 0; i < numPPS; i++) {
        result = h264_parser_identify_nalu_avc(&m_parser,
                                               buf, ofs, bufSize, 2,
                                               &nalu);
        if (result != H264_PARSER_OK)
            return false;

        status = decodePPS(&nalu);
        if (status != DECODE_SUCCESS)
            return false;
        ofs = nalu.offset + nalu.size;
    }

    m_isAVC = true;
    return true;
}

void VaapiDecoderH264::updateFrameInfo()
{
    INFO("H264: update frame info ");
    bool sizeChanged = false;
    H264SPS *sps = &m_lastSPS;
    uint32_t width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
    uint32_t height = (sps->pic_height_in_map_units_minus1 + 1) *
        (sps->frame_mbs_only_flag ? 1 : 2) * 16;

    uint32_t widthAlign = MB_ALIGN(width);
    uint32_t heightAlign = MB_ALIGN(height);

    uint32_t formatInfoWidthAlign = MB_ALIGN(m_videoFormatInfo.width);
    uint32_t formatInfoHeightAlign = MB_ALIGN(m_videoFormatInfo.height);

    if (widthAlign != formatInfoWidthAlign ||
        heightAlign != formatInfoHeightAlign) {
        sizeChanged = true;
        m_videoFormatInfo.width = width;
        m_videoFormatInfo.height = height;
        m_configBuffer.width = width;
        m_configBuffer.height = height;
    }
}

VaapiDecoderH264::VaapiDecoderH264(const char *mimeType)
:VaapiDecoderBase(mimeType)
{
    m_currentPicture = NULL;
    m_DPBManager = NULL;

    memset((void *) &m_parser, 0, sizeof(H264NalParser));
    memset((void *) &m_lastSPS, 0, sizeof(H264SPS));
    memset((void *) &m_lastPPS, 0, sizeof(H264PPS));

    m_prevFrame = NULL;
    m_frameNum = 0;
    m_prevFrameNum = 0;
    m_prevPicHasMMCO5 = false;
    m_progressiveSequence = 0;
    m_prevPicStructure = VAAPI_PICTURE_STRUCTURE_FRAME;
    m_frameNumOffset = 0;

    m_currentPicture = NULL;
    m_mbWidth = 0;
    m_mbHeight = 0;

    m_gotSPS = false;
    m_gotPPS = false;
    m_hasContext = false;
    m_nalLengthSize = 0;
    m_isAVC = false;
    m_resetContext = false;
}

VaapiDecoderH264::~VaapiDecoderH264()
{
    stop();
}

Decode_Status VaapiDecoderH264::start(VideoConfigBuffer * buffer)
{
    DEBUG("H264: start()");
    Decode_Status status;
    bool gotConfig = false;

    if (buffer->data == NULL || buffer->size == 0) {
        gotConfig = false;
        if ((buffer->flag & HAS_SURFACE_NUMBER)
            && (buffer->flag & HAS_VA_PROFILE)) {
            gotConfig = true;
        }
    } else {
        if (decodeCodecData((uint8_t *) buffer->data, buffer->size)) {
            H264SPS *sps = &(m_parser.sps[0]);
            uint32_t maxSize = getMaxDecFrameBuffering(sps, 1);
            buffer->profile = VAProfileH264Baseline;
            buffer->surfaceNumber = maxSize + H264_EXTRA_SURFACE_NUMBER;
            gotConfig = true;
        } else {
            ERROR("codec data has some error");
            return DECODE_FAIL;
        }
    }

    if (gotConfig) {
        status = VaapiDecoderBase::start(buffer);
        if (status != DECODE_SUCCESS)
            return status;

        m_hasContext = true;
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH264::reset(VideoConfigBuffer * buffer)
{
    DEBUG("H264: reset()");
    if (m_DPBManager)
        m_DPBManager->flushDPB();

    m_prevFrame = NULL;
    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderH264::stop(void)
{
    DEBUG("H264: stop()");
    flush();
    VaapiDecoderBase::stop();

    delete m_DPBManager;

    m_DPBManager = NULL;
}

void VaapiDecoderH264::flush(void)
{
    DEBUG("H264: flush()");
    decodeCurrentPicture();

    if (m_DPBManager)
        m_DPBManager->flushDPB();

    VaapiDecoderBase::flush();
}

Decode_Status VaapiDecoderH264::decode(VideoDecodeBuffer * buffer)
{
    Decode_Status status = DECODE_SUCCESS;
    H264ParserResult result;
    H264NalUnit nalu;

    uint8_t *buf;
    uint32_t bufSize = 0;
    uint32_t i, naluSize, size;
    int32_t ofs = 0;
    uint32_t startCode;
    bool isEOS = false;

    m_currentPTS = buffer->timeStamp;
    buf = buffer->data;
    size = buffer->size;

    DEBUG("H264: Decode(bufsize =%d, timestamp=%ld)", size, m_currentPTS);

    do {
        if (m_isAVC) {
            if (size < m_nalLengthSize)
                break;

            naluSize = 0;
            for (i = 0; i < m_nalLengthSize; i++)
                naluSize = (naluSize << 8) | buf[i];

            bufSize = m_nalLengthSize + naluSize;
            if (size < bufSize)
                break;

            result = h264_parser_identify_nalu_avc(&m_parser,
                                                   buf, 0, bufSize,
                                                   m_nalLengthSize, &nalu);

            size -= bufSize;
            buf += bufSize;

        } else {
            if (size < 4)
                break;

            /* skip the un-used bit before start code */
            ofs = scanForStartCode(buf, 0, size, &startCode);
            if (ofs < 0)
                break;

            buf += ofs;
            size -= ofs;

            /* find the length of the nal */
            ofs =
                (size < 7) ? -1 : scanForStartCode(buf, 3, size - 3, NULL);
            if (ofs < 0) {
                ofs = size - 3;
            }

            bufSize = ofs + 3;
            size -= (ofs + 3);

            result = h264_parser_identify_nalu_unchecked(&m_parser,
                                                         buf, 0, bufSize,
                                                         &nalu);

            buf += bufSize;
        }

        status = getStatus(result);
        if (status == DECODE_SUCCESS) {
            status = decodeNalu(&nalu);
        } else {
            ERROR("parser nalu uncheck failed code =%ld", status);
        }

    } while (status == DECODE_SUCCESS);

    if (isEOS && status == DECODE_SUCCESS)
        status = decodeSequenceEnd();

    if (status == DECODE_FORMAT_CHANGE && m_resetContext) {
        WARNING("H264 decoder format change happens");
        m_resetContext = false;
    }

    return status;
}

const VideoRenderBuffer *VaapiDecoderH264::getOutput(bool draining)
{
    INFO("VaapiDecoderH264: getOutput()");
    VideoSurfaceBuffer *surfBuf = NULL;

    if (m_bufPool)
        surfBuf = m_bufPool->getOutputByMinPOC();

    if (!surfBuf)
        return NULL;

    return &(surfBuf->renderBuffer);
}
