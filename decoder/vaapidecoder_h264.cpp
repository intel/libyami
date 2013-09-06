/*
 *  vaapidecoder_h264.cpp - h264 decoder 
 *
 *  Copyright (C) 2013 Intel Corporation
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


#include "vaapidecoder_h264.h"
#include "codecparsers/bytereader.h"

#define G_N_ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))
#define MACROBLOCK_SIZE 16
#define MACROBLOCK_ALIGN (2 * MACROBLOCK_SIZE) 
#define MB_ALIGN(arg) (((arg) + (MACROBLOCK_ALIGN - 1)) & (~(MACROBLOCK_ALIGN - 1)) )

static Decode_Status 
get_status(H264ParserResult result)
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
 
static VAProfile
h264_get_va_profile(H264SPS *sps)
{
    VAProfile profile;

    switch (sps->profile_idc) {
    case 66:
        profile = VAProfileH264Baseline;
        break;
    case 77:
        profile = VAProfileH264Main;
        break;
    case 100:
        profile = VAProfileH264High;
        break;
    }
    return profile;
}

static VaapiChromaType
h264_get_chroma_type(H264SPS *sps)
{
    VaapiChromaType chroma_type = VAAPI_CHROMA_TYPE_YUV420;

    switch (sps->chroma_format_idc) {
    case 0:
        chroma_type = VAAPI_CHROMA_TYPE_YUV400;
        break;
    case 1:
        chroma_type = VAAPI_CHROMA_TYPE_YUV420;
        break;
    case 2:
        chroma_type = VAAPI_CHROMA_TYPE_YUV422;
        break;
    case 3:
        if (!sps->separate_colour_plane_flag)
            chroma_type = VAAPI_CHROMA_TYPE_YUV444;
        break;
    }
    return chroma_type;
}


static inline uint32
get_slice_data_bit_offset(H264SliceHdr *slice_hdr, H264NalUnit *nalu)
{
    uint32_t epb_count;
    epb_count = slice_hdr->n_emulation_prevention_bytes;
    return 8 * slice_hdr->nal_header_bytes + slice_hdr->header_size - epb_count * 8;
}

static void
fill_iq_matrix_4x4(VAIQMatrixBufferH264 *iq_matrix, const H264PPS *pps)
{
    const uint8_t (* const ScalingList4x4)[6][16] = &pps->scaling_lists_4x4;
    uint32_t i, j;

    /* There are always 6 4x4 scaling lists */
    assert(G_N_ELEMENTS(iq_matrix->ScalingList4x4) == 6);
    assert(G_N_ELEMENTS(iq_matrix->ScalingList4x4[0]) == 16);

    if (sizeof(iq_matrix->ScalingList4x4[0][0]) == 1)
        memcpy(iq_matrix->ScalingList4x4, *ScalingList4x4,
               sizeof(iq_matrix->ScalingList4x4));
    else {
        for (i = 0; i < G_N_ELEMENTS(iq_matrix->ScalingList4x4); i++) {
            for (j = 0; j < G_N_ELEMENTS(iq_matrix->ScalingList4x4[i]); j++)
                iq_matrix->ScalingList4x4[i][j] = (*ScalingList4x4)[i][j];
        }
    }
}

static void
fill_iq_matrix_8x8(VAIQMatrixBufferH264 *iq_matrix, const H264PPS *pps)
{
    const uint8_t (* const ScalingList8x8)[6][64] = &pps->scaling_lists_8x8;
    const H264SPS * const sps = pps->sequence;
    uint32_t i, j, n;

    /* If chroma_format_idc != 3, there are up to 2 8x8 scaling lists */
    if (!pps->transform_8x8_mode_flag)
        return;

    assert(G_N_ELEMENTS(iq_matrix->ScalingList8x8) >= 2);
    assert(G_N_ELEMENTS(iq_matrix->ScalingList8x8[0]) == 64);

    if (sizeof(iq_matrix->ScalingList8x8[0][0]) == 1)
        memcpy(iq_matrix->ScalingList8x8, *ScalingList8x8,
               sizeof(iq_matrix->ScalingList8x8));
    else {
        n = (sps->chroma_format_idc != 3) ? 2 : 6;
        for (i = 0; i < n; i++) {
            for (j = 0; j < G_N_ELEMENTS(iq_matrix->ScalingList8x8[i]); j++)
                iq_matrix->ScalingList8x8[i][j] = (*ScalingList8x8)[i][j];
        }
    }
}

static inline int32_t
scan_for_start_code(const uint8_t* data , 
                    uint32_t offset,
                    uint32_t size, 
                    uint32_t *scp)
{
/*
    ByteReader br;
    byte_reader_init(&br, data, size);
    return byte_reader_masked_scan_uint32(&br, 0xffffff00, 0x00000100, 0, size) - 1;
*/

    uint32_t i;
    const uint8_t *buf;

    if(offset + 3 > size)
        return -1;
 
    for ( i = 0; i < size - offset - 3 + 1; i ++){
       buf = data + offset + i;
       if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
           return i;
    }

    return -1;
}

VaapiSliceH264::VaapiSliceH264(VADisplay display,
    VAContextID ctx,
    uint8_t *sliceData,
    uint32_t sliceSize)
{
    VASliceParameterBufferH264* paramBuf;

    if (!display || !ctx) {
         ERROR("VA display or context not initialized yet");
         return;
    }

    /* new h264 slice data buffer */
    mData = new VaapiBufObject(display,
                 ctx,
                 VASliceDataBufferType,
                 sliceData,
                 sliceSize);

    /* new h264 slice parameter buffer */
    mParam = new VaapiBufObject(display,
                 ctx,
                 VASliceParameterBufferType,
                 NULL,
                 sizeof(VASliceParameterBufferH264));

    paramBuf = (VASliceParameterBufferH264*)mParam->map();

    paramBuf->slice_data_size   = sliceSize;
    paramBuf->slice_data_offset = 0;
    paramBuf->slice_data_flag   = VA_SLICE_DATA_FLAG_ALL;
    mParam->unmap();

    memset((void*)&slice_hdr, 0 , sizeof(slice_hdr));

}

VaapiSliceH264::~VaapiSliceH264()
{
    if (mData) {
        delete mData;
        mData = NULL;
    }

    if (mParam) {
        delete mParam;
        mParam = NULL;
    }
}

VaapiPictureH264::VaapiPictureH264(VADisplay display,
                     VAContextID context,
                     VaapiSurfaceBufferPool *surfBufPool,
                     VaapiPictureStructure structure)
:VaapiPicture(display, context, surfBufPool, structure)
{
   pps             = NULL;
   structure       = VAAPI_PICTURE_STRUCTURE_FRAME;
   field_poc[0]    = 0;
   field_poc[1]    = 0;
   frame_num       = 0;
   frame_num_wrap  = 0;
   long_term_frame_idx = 0;
   pic_num             = 0;
   long_term_pic_num   = 0;
   output_flag         = 0;
   output_needed       = 0;
   other_field         = NULL;

   /* new h264 slice parameter buffer */
   mPicParam = new VaapiBufObject(display,
                 context,
                 VAPictureParameterBufferType,
                 NULL,
                 sizeof(VAPictureParameterBufferH264));
   if (!mPicParam)
      ERROR("create h264 picture parameter buffer object failed");

   mIqMatrix = new VaapiBufObject(display,
                   context,
                   VAIQMatrixBufferType,
                   NULL,
                   sizeof(VAIQMatrixBufferH264));
   if (!mIqMatrix)
      ERROR("create h264 iq matrix buffer object failed");

}

VaapiPictureH264*
VaapiPictureH264::new_field()
{
/*
   return new VaapiPictureH264(mDisplay,
                               mContext,
                               mSurface,
                               mStructure);
*/
   ERROR("Not implemented yet");
   return NULL;
}

VaapiFrameStore::VaapiFrameStore(VaapiPictureH264 *pic)
{
   structure  =  pic->structure;
   buffers[0] =  pic;
   buffers[1] =  NULL;
   num_buffers = 1;
   output_needed = pic->output_needed;
}

VaapiFrameStore::~VaapiFrameStore()
{
   if (buffers[0]) {
      delete buffers[0];
      buffers[0] = NULL;
   }
   if (buffers[1]) {
      delete buffers[1];
      buffers[1] = NULL;
   }
}

void
VaapiFrameStore::add_picture(VaapiPictureH264 *pic)
{
    uint8_t field;
    assert(num_buffers == 1);
    assert(pic->structure != VAAPI_PICTURE_STRUCTURE_FRAME);
    buffers[1] = pic;

    if(pic->output_flag) {
       pic->output_needed = true;
       output_needed++;
    }

    structure = VAAPI_PICTURE_STRUCTURE_FRAME;

    field = pic->structure == VAAPI_PICTURE_STRUCTURE_TOP_FIELD ? 0 : 1;

    assert(buffers[0]->field_poc[field] != INT32_MAX);
    buffers[0] ->field_poc[field] = pic->field_poc[field];

    assert(pic->field_poc[!field] != INT32_MAX);
    pic->field_poc[!field] = buffers[0]->field_poc[!field];
    num_buffers = num_buffers + 1;
}

bool
VaapiFrameStore::split_fields()
{
    VaapiPictureH264 * const first_field = buffers[0];
    VaapiPictureH264 * second_field;

    assert(num_buffers == 1);

    first_field->mStructure = VAAPI_PICTURE_STRUCTURE_TOP_FIELD;
    first_field->mFlags |= VAAPI_PICTURE_FLAG_INTERLACED;

    second_field = first_field->new_field();
    second_field->frame_num    = first_field->frame_num;
    second_field->field_poc[0] = first_field->field_poc[0];
    second_field->field_poc[1] = first_field->field_poc[1];
    second_field->output_flag  = first_field->output_flag;
    if (second_field->output_flag) {
        second_field->output_needed = true;
        output_needed++;
    }

    num_buffers ++;
    buffers[num_buffers] = second_field;

    return true;
}

bool
VaapiFrameStore::has_frame()
{
    return structure == VAAPI_PICTURE_STRUCTURE_FRAME;
}

bool
VaapiFrameStore::has_reference()
{
    uint32_t i;

    for (i = 0; i < num_buffers; i++) {
        if (!buffers[i]) continue;
        if (VAAPI_PICTURE_IS_REFERENCE(buffers[i]))
            return true;
    }
    return false;
}

/////////////////////////////////////////////////////

Decode_Status
VaapiDecoderH264::decode_sps(H264NalUnit *nalu)
{
    H264SPS * const sps = &m_last_sps;
    H264ParserResult result;

    DEBUG("decode SPS");

    memset(sps, 0, sizeof(*sps));
    result = h264_parser_parse_sps(&m_parser, nalu, sps, true);
    if (result != H264_PARSER_OK) {
        ERROR("parse sps failed");
        m_got_sps = false;
        return get_status(result);
    }

    m_got_sps = true;

    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderH264::decode_pps(H264NalUnit *nalu)
{
    H264PPS * const pps = &m_last_pps;
    H264ParserResult result;

    DEBUG("decode PPS");

    memset(pps, 0, sizeof(*pps));
    result = h264_parser_parse_pps(&m_parser, nalu, pps);
    if (result != H264_PARSER_OK) {
        m_got_pps = false;
        return get_status(result);
    }

    m_got_pps = true;
    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderH264::decode_sei(H264NalUnit *nalu)
{
    H264SEIMessage sei;
    H264ParserResult result;

    DEBUG("decode SEI");

    memset(&sei, 0, sizeof(sei));
    result = h264_parser_parse_sei(&m_parser, nalu, &sei);
    if (result != H264_PARSER_OK) {
        WARNING("failed to decode SEI, payload type:%d", sei.payloadType);
        return get_status(result);
    }

    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderH264::decode_sequence_end()
{
    Decode_Status status;

    DEBUG("decode sequence-end");

    status = decode_current_picture();
    if (status != DECODE_SUCCESS)
        return status;

    m_dpb_manager->dpb_flush();
    return DECODE_SUCCESS;
}

void
VaapiDecoderH264::init_picture_poc_0(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    const int32_t MaxPicOrderCntLsb = 1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    int32_t temp_poc;

    DEBUG("decode picture order count type 0");

    if (VAAPI_H264_PICTURE_IS_IDR(picture)) {
        m_prev_poc_msb = 0;
        m_prev_poc_lsb = 0;
    }
    else if (m_prev_pic_has_mmco5) {
        m_prev_poc_msb = 0;
        m_prev_poc_lsb =
            (m_prev_pic_structure == VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD ?
             0 : m_field_poc[TOP_FIELD]);
    }
    else {
        m_prev_poc_msb = m_poc_msb;
        m_prev_poc_lsb = m_poc_lsb;
    }

    // (8-3)
    m_poc_lsb = slice_hdr->pic_order_cnt_lsb;
    if (m_poc_lsb < m_prev_poc_lsb &&
        (m_prev_poc_lsb - m_poc_lsb) >= (MaxPicOrderCntLsb / 2))
        m_poc_msb = m_prev_poc_msb + MaxPicOrderCntLsb;
    else if (m_poc_lsb > m_prev_poc_lsb &&
             (m_poc_lsb - m_prev_poc_lsb) > (MaxPicOrderCntLsb / 2))
        m_poc_msb = m_prev_poc_msb - MaxPicOrderCntLsb;
    else
        m_poc_msb = m_prev_poc_msb;

    temp_poc = m_poc_msb + m_poc_lsb;
    switch (picture->structure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        // (8-4, 8-5)
        m_field_poc[TOP_FIELD] = temp_poc;
        m_field_poc[BOTTOM_FIELD] = temp_poc +
            slice_hdr->delta_pic_order_cnt_bottom;
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        // (8-4)
        m_field_poc[TOP_FIELD] = temp_poc;
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        // (8-5)
        m_field_poc[BOTTOM_FIELD] = temp_poc;
        break;
    }
}

void
VaapiDecoderH264::init_picture_poc_1(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    const int32_t MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    int32_t prev_frame_num_offset, abs_frame_num, expected_poc;
    uint32_t i;

    DEBUG("decode picture order count type 1");

    if (m_prev_pic_has_mmco5)
        prev_frame_num_offset = 0;
    else
        prev_frame_num_offset = m_frame_num_offset;

    // (8-6)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        m_frame_num_offset = 0;
    else if (m_prev_frame_num > m_frame_num)
        m_frame_num_offset = prev_frame_num_offset + MaxFrameNum;
    else
        m_frame_num_offset = prev_frame_num_offset;

    // (8-7)
    if (sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
        abs_frame_num = m_frame_num_offset + m_frame_num;
    else
        abs_frame_num = 0;
    if (!VAAPI_PICTURE_IS_REFERENCE(picture) && abs_frame_num > 0)
        abs_frame_num = abs_frame_num - 1;

    if (abs_frame_num > 0) {
        int32_t expected_delta_per_poc_cycle;
        int32_t poc_cycle_cnt, frame_num_in_poc_cycle;

        expected_delta_per_poc_cycle = 0;
        for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
            expected_delta_per_poc_cycle += sps->offset_for_ref_frame[i];

        // (8-8)
        poc_cycle_cnt = (abs_frame_num - 1) /
            sps->num_ref_frames_in_pic_order_cnt_cycle;
        frame_num_in_poc_cycle = (abs_frame_num - 1) %
            sps->num_ref_frames_in_pic_order_cnt_cycle;

        // (8-9)
        expected_poc = poc_cycle_cnt * expected_delta_per_poc_cycle;
        for (i = 0; (int32_t)i <= frame_num_in_poc_cycle; i++)
            expected_poc += sps->offset_for_ref_frame[i];
    }
    else
        expected_poc = 0;
    if (!VAAPI_PICTURE_IS_REFERENCE(picture))
        expected_poc += sps->offset_for_non_ref_pic;

    // (8-10)
    switch (picture->structure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        m_field_poc[TOP_FIELD] = expected_poc +
            slice_hdr->delta_pic_order_cnt[0];
        m_field_poc[BOTTOM_FIELD] = m_field_poc[TOP_FIELD] +
            sps->offset_for_top_to_bottom_field +
            slice_hdr->delta_pic_order_cnt[1];
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        m_field_poc[TOP_FIELD] = expected_poc +
            slice_hdr->delta_pic_order_cnt[0];
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        m_field_poc[BOTTOM_FIELD] = expected_poc + 
            sps->offset_for_top_to_bottom_field +
            slice_hdr->delta_pic_order_cnt[0];
        break;
    }
}

void
VaapiDecoderH264::init_picture_poc_2(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    const int32_t MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    int32_t prev_frame_num_offset, temp_poc;

    DEBUG("decode picture order count type 2");

    if (m_prev_pic_has_mmco5)
        prev_frame_num_offset = 0;
    else
        prev_frame_num_offset = m_frame_num_offset;

    // (8-11)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        m_frame_num_offset = 0;
    else if (m_prev_frame_num > m_frame_num)
        m_frame_num_offset = prev_frame_num_offset + MaxFrameNum;
    else
        m_frame_num_offset = prev_frame_num_offset;

    // (8-12)
    if (VAAPI_H264_PICTURE_IS_IDR(picture))
        temp_poc = 0;
    else if (!VAAPI_PICTURE_IS_REFERENCE(picture))
        temp_poc = 2 * (m_frame_num_offset + m_frame_num) - 1;
    else
        temp_poc = 2 * (m_frame_num_offset + m_frame_num);

    // (8-13)
    if (picture->structure != VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
        m_field_poc[TOP_FIELD] = temp_poc;
    if (picture->structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        m_field_poc[BOTTOM_FIELD] = temp_poc;
}

/* 8.2.1 - Decoding process for picture order count */
void
VaapiDecoderH264::init_picture_poc(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;

    switch (sps->pic_order_cnt_type) {
    case 0:
        init_picture_poc_0(picture, slice_hdr);
        break;
    case 1:
        init_picture_poc_1(picture, slice_hdr);
        break;
    case 2:
        init_picture_poc_2(picture, slice_hdr);
        break;
    }

    if (picture->structure != VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
        picture->field_poc[TOP_FIELD] = m_field_poc[TOP_FIELD];
    if (picture->structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
        picture->field_poc[BOTTOM_FIELD] = m_field_poc[BOTTOM_FIELD];

    picture->mPoc = MIN(picture->field_poc[0], picture->field_poc[1]); 

}

bool
VaapiDecoderH264::init_picture(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr,
    H264NalUnit      *nalu
)
{
    H264SPS * const sps = slice_hdr->pps->sequence;

    m_prev_frame_num         = m_frame_num;
    m_frame_num              = slice_hdr->frame_num;
    picture->frame_num       = m_frame_num;
    picture->frame_num_wrap  = m_frame_num;
    picture->output_flag     = true; /* XXX: conformant to Annex A only */
    picture->mTimeStamp      = mCurrentPTS;

    /* Reset decoder state for IDR pictures */
    if (nalu->idr_pic_flag) {
        INFO("<IDR>");
        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_IDR);
        m_dpb_manager->dpb_flush();
        m_prev_frame = NULL;
    } 

    /* Initialize slice type */
    switch (slice_hdr->type % 5) {
    case H264_P_SLICE:
        picture->mType = VAAPI_PICTURE_TYPE_P;
        break;
    case H264_B_SLICE:
        picture->mType = VAAPI_PICTURE_TYPE_B;
        break;
    case H264_I_SLICE:
        picture->mType = VAAPI_PICTURE_TYPE_I;
        break;
    case H264_SP_SLICE:
        picture->mType = VAAPI_PICTURE_TYPE_SP;
        break;
    case H264_SI_SLICE:
        picture->mType = VAAPI_PICTURE_TYPE_SI;
        break;
    }

    /* Initialize picture structure */
    if (!slice_hdr->field_pic_flag)
        picture->mStructure = VAAPI_PICTURE_STRUCTURE_FRAME;
    else {
        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_INTERLACED);
        if (!slice_hdr->bottom_field_flag)
            picture->mStructure = VAAPI_PICTURE_STRUCTURE_TOP_FIELD;
        else
            picture->mStructure = VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD;
    }
    picture->structure = picture->mStructure;

    /* Initialize reference flags */
    if (nalu->ref_idc) {
        H264DecRefPicMarking * const dec_ref_pic_marking =
            &slice_hdr->dec_ref_pic_marking;

        if (VAAPI_H264_PICTURE_IS_IDR(picture) &&
            dec_ref_pic_marking->long_term_reference_flag){
            VAAPI_PICTURE_FLAG_SET(picture,
                VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE);
 
        }
        else {
            VAAPI_PICTURE_FLAG_SET(picture,
                VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE);
        }
    }

    init_picture_poc(picture, slice_hdr);

    m_dpb_manager->init_picture_refs(picture, slice_hdr,
                   m_frame_num);
    return true;
}

/* fetch and fill picture paramter buffer */
void
VaapiDecoderH264::vaapi_init_picture(VAPictureH264 *pic)
{
    pic->picture_id           = VA_INVALID_ID;
    pic->frame_idx            = 0;
    pic->flags                = VA_PICTURE_H264_INVALID;
    pic->TopFieldOrderCnt     = 0;
    pic->BottomFieldOrderCnt  = 0;
}

void
VaapiDecoderH264::vaapi_fill_picture(
    VAPictureH264 *pic, 
    VaapiPictureH264 *picture,
    uint32_t picture_structure)
{
    pic->picture_id = picture->mSurfaceID;
    pic->flags = 0;

    if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture)) {
        pic->flags |= VA_PICTURE_H264_LONG_TERM_REFERENCE;
        pic->frame_idx = picture->long_term_frame_idx;
    }
    else {
        if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
            pic->flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        pic->frame_idx = picture->frame_num;
    }

    if (!picture_structure)
        picture_structure = picture->structure;

    switch (picture_structure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        pic->TopFieldOrderCnt = picture->field_poc[TOP_FIELD];
        pic->BottomFieldOrderCnt = picture->field_poc[BOTTOM_FIELD];
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        pic->flags |= VA_PICTURE_H264_TOP_FIELD;
        pic->TopFieldOrderCnt = picture->field_poc[TOP_FIELD];
        pic->BottomFieldOrderCnt = 0;
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        pic->flags |= VA_PICTURE_H264_BOTTOM_FIELD;
        pic->BottomFieldOrderCnt = picture->field_poc[BOTTOM_FIELD];
        pic->TopFieldOrderCnt = 0;
        break;
    }
}

bool
VaapiDecoderH264::fill_picture(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr,
    H264NalUnit      *nalu
)
{
    uint32_t i, n;
    H264PPS * const pps = picture->pps;
    H264SPS * const sps = pps->sequence;
    VaapiDecPicBufLayer *dpb_layer = m_dpb_manager->dpb_layer;
    VaapiBufObject* pic_param_obj = picture->mPicParam;

    VAPictureParameterBufferH264 *  pic_param = 
          (VAPictureParameterBufferH264 *)pic_param_obj->map();

    /* Fill in VAPictureParameterBufferH264 */
    vaapi_fill_picture(&pic_param->CurrPic, picture, 0);

    for (i = 0, n = 0; i < dpb_layer->dpb_count; i++) {
        VaapiFrameStore * const fs = dpb_layer->dpb[i];
        if (fs && fs->has_reference())
            vaapi_fill_picture(&pic_param->ReferenceFrames[n++],
                fs->buffers[0], fs->structure);
    }

    for (; n < G_N_ELEMENTS(pic_param->ReferenceFrames); n++)
        vaapi_init_picture(&pic_param->ReferenceFrames[n]);

#define COPY_FIELD(s, f) \
    pic_param->f = (s)->f

#define COPY_BFM(a, s, f) \
    pic_param->a.bits.f = (s)->f

    pic_param->picture_width_in_mbs_minus1  = m_mb_width - 1;
    pic_param->picture_height_in_mbs_minus1 = m_mb_height - 1;
    pic_param->frame_num                    = m_frame_num;

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

    pic_param->seq_fields.value                                 = 0; /* reset all bits */
    pic_param->seq_fields.bits.residual_colour_transform_flag   = sps->separate_colour_plane_flag;
    pic_param->seq_fields.bits.MinLumaBiPredSize8x8             = sps->level_idc >= 31; /* A.3.3.2 */

    COPY_BFM(seq_fields, sps, chroma_format_idc);
    COPY_BFM(seq_fields, sps, gaps_in_frame_num_value_allowed_flag);
    COPY_BFM(seq_fields, sps, frame_mbs_only_flag); 
    COPY_BFM(seq_fields, sps, mb_adaptive_frame_field_flag); 
    COPY_BFM(seq_fields, sps, direct_8x8_inference_flag); 
    COPY_BFM(seq_fields, sps, log2_max_frame_num_minus4);
    COPY_BFM(seq_fields, sps, pic_order_cnt_type);
    COPY_BFM(seq_fields, sps, log2_max_pic_order_cnt_lsb_minus4);
    COPY_BFM(seq_fields, sps, delta_pic_order_always_zero_flag);

    pic_param->pic_fields.value                                  = 0; /* reset all bits */
    pic_param->pic_fields.bits.field_pic_flag                    = slice_hdr->field_pic_flag;
    pic_param->pic_fields.bits.reference_pic_flag                = VAAPI_PICTURE_IS_REFERENCE(picture);

    COPY_BFM(pic_fields, pps, entropy_coding_mode_flag);
    COPY_BFM(pic_fields, pps, weighted_pred_flag);
    COPY_BFM(pic_fields, pps, weighted_bipred_idc);
    COPY_BFM(pic_fields, pps, transform_8x8_mode_flag);
    COPY_BFM(pic_fields, pps, constrained_intra_pred_flag);
    COPY_BFM(pic_fields, pps, pic_order_present_flag);
    COPY_BFM(pic_fields, pps, deblocking_filter_control_present_flag);
    COPY_BFM(pic_fields, pps, redundant_pic_cnt_present_flag);
    pic_param_obj->unmap();

    return true;
}

/* fill slice parameter buffers functions*/ 
bool 
VaapiDecoderH264::ensure_quant_matrix(VaapiPictureH264 *pic)
{
    H264PPS * const pps = pic->pps;
    H264SPS * const sps = pps->sequence;
    VAIQMatrixBufferH264 *iq_matrix;      

    /* we can only support 4:2:0 or 4:2:2 since ScalingLists8x8[]
       is not large enough to hold lists for 4:4:4 */
    if (sps->chroma_format_idc == 3)
        return false;

    iq_matrix =(VAIQMatrixBufferH264*) pic->mIqMatrix->map();

    fill_iq_matrix_4x4(iq_matrix, pps);
    fill_iq_matrix_8x8(iq_matrix, pps);
    pic->mIqMatrix->unmap();
    return true;
}

bool
VaapiDecoderH264::fill_pred_weight_table(VaapiSliceH264 *slice)
{
    int32_t i, j;
    uint32_t num_weight_tables = 0;
    H264SliceHdr * const slice_hdr = &slice->slice_hdr;
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    H264PredWeightTable * const w = &slice_hdr->pred_weight_table;

    VaapiBufObject *slice_param_obj = slice->mParam;

    VASliceParameterBufferH264 * slice_param = 
       (VASliceParameterBufferH264*) slice_param_obj->map();

    if (pps->weighted_pred_flag &&
        (H264_IS_P_SLICE(slice_hdr) || H264_IS_SP_SLICE(slice_hdr)))
        num_weight_tables = 1;
    else if (pps->weighted_bipred_idc == 1 && H264_IS_B_SLICE(slice_hdr))
        num_weight_tables = 2;
    else
        num_weight_tables = 0;

    slice_param->luma_log2_weight_denom   = w->luma_log2_weight_denom;
    slice_param->chroma_log2_weight_denom = w->chroma_log2_weight_denom;
    slice_param->luma_weight_l0_flag      = 0;
    slice_param->chroma_weight_l0_flag    = 0;
    slice_param->luma_weight_l1_flag      = 0;
    slice_param->chroma_weight_l1_flag    = 0;

    if (num_weight_tables < 1)
        goto out;

    slice_param->luma_weight_l0_flag = 1;
    for (i = 0; i <= slice_param->num_ref_idx_l0_active_minus1; i++) {
        slice_param->luma_weight_l0[i] = w->luma_weight_l0[i];
        slice_param->luma_offset_l0[i] = w->luma_offset_l0[i];
    }

    slice_param->chroma_weight_l0_flag = sps->chroma_array_type != 0;
    if (slice_param->chroma_weight_l0_flag) {
        for (i = 0; i <= slice_param->num_ref_idx_l0_active_minus1; i++) {
            for (j = 0; j < 2; j++) {
                slice_param->chroma_weight_l0[i][j] = w->chroma_weight_l0[i][j];
                slice_param->chroma_offset_l0[i][j] = w->chroma_offset_l0[i][j];
            }
        }
    }

    if (num_weight_tables < 2)
        goto out;

    slice_param->luma_weight_l1_flag = 1;
    for (i = 0; i <= slice_param->num_ref_idx_l1_active_minus1; i++) {
        slice_param->luma_weight_l1[i] = w->luma_weight_l1[i];
        slice_param->luma_offset_l1[i] = w->luma_offset_l1[i];
    }

    slice_param->chroma_weight_l1_flag = sps->chroma_array_type != 0;
    if (slice_param->chroma_weight_l1_flag) {
        for (i = 0; i <= slice_param->num_ref_idx_l1_active_minus1; i++) {
            for (j = 0; j < 2; j++) {
                slice_param->chroma_weight_l1[i][j] = w->chroma_weight_l1[i][j];
                slice_param->chroma_offset_l1[i][j] = w->chroma_offset_l1[i][j];
            }
        }
    }

out:
    slice_param_obj->unmap();
    return true;
}

bool
VaapiDecoderH264::fill_RefPicList(
    VaapiSliceH264 *slice)
{
    uint32_t i, num_ref_lists = 0;
    VaapiDecPicBufLayer * dpb_layer = m_dpb_manager->dpb_layer;
    H264SliceHdr * const slice_hdr = &slice->slice_hdr;
    VaapiBufObject *slice_param_obj = slice->mParam;
    VASliceParameterBufferH264 * slice_param = 
       (VASliceParameterBufferH264*) slice_param_obj->map();
    
    slice_param->num_ref_idx_l0_active_minus1 = 0;
    slice_param->num_ref_idx_l1_active_minus1 = 0;

    if (H264_IS_B_SLICE(slice_hdr))
        num_ref_lists = 2;
    else if (H264_IS_I_SLICE(slice_hdr))
        num_ref_lists = 0;
    else
        num_ref_lists = 1;

    if (num_ref_lists < 1)
        goto out;

    slice_param->num_ref_idx_l0_active_minus1 =
        slice_hdr->num_ref_idx_l0_active_minus1;

    for (i = 0; i < dpb_layer->RefPicList0_count && dpb_layer->RefPicList0[i]; i++)
        vaapi_fill_picture(&slice_param->RefPicList0[i], dpb_layer->RefPicList0[i], 0);
    for (; i <= slice_param->num_ref_idx_l0_active_minus1; i++)
        vaapi_init_picture(&slice_param->RefPicList0[i]);

    if (num_ref_lists < 2)
        goto out;

    slice_param->num_ref_idx_l1_active_minus1 =
        slice_hdr->num_ref_idx_l1_active_minus1;

    for (i = 0; i < dpb_layer->RefPicList1_count && dpb_layer->RefPicList1[i]; i++)
        vaapi_fill_picture(&slice_param->RefPicList1[i], dpb_layer->RefPicList1[i], 0);
    for (; i <= slice_param->num_ref_idx_l1_active_minus1; i++)
        vaapi_init_picture(&slice_param->RefPicList1[i]);

out:
    slice_param_obj->unmap();
    return true;
}

bool
VaapiDecoderH264::fill_slice(
    VaapiSliceH264   *slice,
    H264NalUnit      *nalu)
{
    H264SliceHdr * const slice_hdr = &slice->slice_hdr;
    VaapiBufObject *slice_param_obj = slice->mParam;
    VASliceParameterBufferH264 * slice_param = 
       (VASliceParameterBufferH264*) slice_param_obj->map();

    /* Fill in VASliceParameterBufferH264 */
    slice_param->slice_data_bit_offset          = get_slice_data_bit_offset(slice_hdr, nalu);
    slice_param->first_mb_in_slice              = slice_hdr->first_mb_in_slice;
    slice_param->slice_type                     = slice_hdr->type % 5;
    slice_param->direct_spatial_mv_pred_flag    = slice_hdr->direct_spatial_mv_pred_flag;
    slice_param->cabac_init_idc                 = slice_hdr->cabac_init_idc;
    slice_param->slice_qp_delta                 = slice_hdr->slice_qp_delta;
    slice_param->disable_deblocking_filter_idc  = slice_hdr->disable_deblocking_filter_idc;
    slice_param->slice_alpha_c0_offset_div2     = slice_hdr->slice_alpha_c0_offset_div2;
    slice_param->slice_beta_offset_div2         = slice_hdr->slice_beta_offset_div2;

    slice_param_obj->unmap();

    if (!fill_RefPicList(slice))
        return false;
    if (!fill_pred_weight_table(slice))
        return false;
    return true;
}

Decode_Status
VaapiDecoderH264::ensure_context(H264SPS *sps)
{
   VAProfile parsed_profile;
   VaapiChromaType parsed_chroma;
   uint32_t mb_width, mb_height;
   bool  reset_context = false;
   uint32_t dpb_size = 0;
 
   m_progressive_sequence = sps->frame_mbs_only_flag;

    if (!m_dpb_manager) {
        dpb_size = get_max_dec_frame_buffering(sps, 1); 
        m_dpb_manager = new VaapiDPBManager(dpb_size);
    }
 
   VideoConfigBuffer *config = &mConfigBuffer; 
   parsed_profile = h264_get_va_profile(sps);
   if (parsed_profile != mConfigBuffer.profile) {
      INFO("ensure context: profile old: %d, new:%d, changed !\n", mConfigBuffer.profile, parsed_profile);
      mConfigBuffer.profile = parsed_profile;
      reset_context = true;
   }

   /*
   parsed_chroma = h264_get_chroma_type(sps);
   if (parsed_chroma != m_chroma_type) {
      WARNING("ensure context: chroma changed !\n");
      m_chroma_type = parsed_chroma;
      reset_context = true;
   }
   */

   mb_width  = sps->pic_width_in_mbs_minus1 + 1;
   mb_height = (sps->pic_height_in_map_units_minus1 + 1) <<
        !sps->frame_mbs_only_flag;

   if (mb_width != m_mb_width || mb_height != m_mb_height) {
        INFO("ensure context: Original: w=%d, h=%d, New: w=%d, h=%d ",
              m_mb_width*16, m_mb_height*16, mb_width*16, mb_height*16);

        m_mb_width  = mb_width;
        m_mb_height = mb_height;
        mConfigBuffer.width  = mb_width  * 16;
        mConfigBuffer.height = mb_height * 16;
        reset_context  = true;
    }

    if (!reset_context && m_has_context)
          return DECODE_SUCCESS;

    if (!m_has_context) {
         dpb_size  = get_max_dec_frame_buffering(sps, 1); 
         mConfigBuffer.surfaceNumber = dpb_size + H264_EXTRA_SURFACE_NUMBER;
         VaapiDecoderBase::start(&mConfigBuffer);    
         DEBUG("First time to Start VA context");
         m_reset_context = true;
    } else if (reset_context) {
       VaapiDecoderBase::reset(&mConfigBuffer);    
       if  (m_dpb_manager)
            m_dpb_manager->dpb_reset(sps);

       DEBUG("Re-Start VA context");
       m_reset_context = true;
    }

    m_has_context = true;

    return DECODE_SUCCESS;
}

bool
VaapiDecoderH264::is_new_picture(
    H264NalUnit      *nalu,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    VaapiSliceH264 *slice;
    H264SliceHdr *prev_slice_hdr;

    if (!m_current_picture)
        return true;

    slice = (VaapiSliceH264*) (m_current_picture->getLastSlice());
    if (!slice)
        return false;
    prev_slice_hdr = &slice->slice_hdr;

#define CHECK_EXPR(expr, field_name) do {              \
        if (!(expr)) {                                 \
            DEBUG(field_name " differs in value"); \
            return true;                               \
        }                                              \
    } while (0)

#define CHECK_VALUE(new_slice_hdr, old_slice_hdr, field) \
    CHECK_EXPR(((new_slice_hdr)->field == (old_slice_hdr)->field), #field)

    /* frame_num differs in value, regardless of inferred values to 0 */
    CHECK_VALUE(slice_hdr, prev_slice_hdr, frame_num);

    /* pic_parameter_set_id differs in value */
    CHECK_VALUE(slice_hdr, prev_slice_hdr, pps);

    /* field_pic_flag differs in value */
    CHECK_VALUE(slice_hdr, prev_slice_hdr, field_pic_flag);

    /* bottom_field_flag is present in both and differs in value */
    if (slice_hdr->field_pic_flag && prev_slice_hdr->field_pic_flag)
        CHECK_VALUE(slice_hdr, prev_slice_hdr, bottom_field_flag);

    /* nal_ref_idc differs in value with one of the nal_ref_idc values is 0 */
    CHECK_EXPR(((VAAPI_PICTURE_IS_REFERENCE(m_current_picture) ^
                 (nalu->ref_idc != 0)) == 0), "nal_ref_idc");

    /* POC type is 0 for both and either pic_order_cnt_lsb differs in
       value or delta_pic_order_cnt_bottom differs in value */
    if (sps->pic_order_cnt_type == 0) {
        CHECK_VALUE(slice_hdr, prev_slice_hdr, pic_order_cnt_lsb);
        if (pps->pic_order_present_flag && !slice_hdr->field_pic_flag)
            CHECK_VALUE(slice_hdr, prev_slice_hdr, delta_pic_order_cnt_bottom);
    }

    /* POC type is 1 for both and either delta_pic_order_cnt[0]
       differs in value or delta_pic_order_cnt[1] differs in value */
    else if (sps->pic_order_cnt_type == 1) {
        CHECK_VALUE(slice_hdr, prev_slice_hdr, delta_pic_order_cnt[0]);
        CHECK_VALUE(slice_hdr, prev_slice_hdr, delta_pic_order_cnt[1]);
    }

    /* IdrPicFlag differs in value */
    CHECK_EXPR(((VAAPI_H264_PICTURE_IS_IDR(m_current_picture) ^
                 (nalu->type == H264_NAL_SLICE_IDR)) == 0), "IdrPicFlag");

    /* IdrPicFlag is equal to 1 for both and idr_pic_id differs in value */
    if (VAAPI_H264_PICTURE_IS_IDR(m_current_picture))
        CHECK_VALUE(slice_hdr, prev_slice_hdr, idr_pic_id);

#undef CHECK_EXPR
#undef CHECK_VALUE
    return false;
}

bool
VaapiDecoderH264::marking_picture(VaapiPictureH264 *pic)
{
    if (!m_dpb_manager->exec_ref_pic_marking(pic, &m_prev_pic_has_mmco5))
       return false;

    if (m_prev_pic_has_mmco5) {
        m_frame_num = 0;
        m_frame_num_offset = 0;
    }

    m_prev_pic_structure = pic->structure;

    return true;
}

bool
VaapiDecoderH264::store_decoded_picture(VaapiPictureH264 *pic)
{
     VaapiFrameStore *fs;
    // Check if picture is the second field and the first field is still in DPB
    if (m_prev_frame && !m_prev_frame->has_frame()) {
        RETURN_VAL_IF_FAIL(m_prev_frame->num_buffers == 1, false);
        RETURN_VAL_IF_FAIL(VAAPI_PICTURE_IS_FRAME(m_current_picture), false);
        RETURN_VAL_IF_FAIL(VAAPI_PICTURE_IS_FIRST_FIELD(m_current_picture), false);
        m_prev_frame->add_picture(m_current_picture);
        // Remove all unused pictures
        INFO("field pictre appear here ");
        return true;
    }

    // Create new frame store, and split fields if necessary
    fs = new VaapiFrameStore(pic);

    if (!fs)
        return false;
    m_prev_frame = fs;

    if (!m_progressive_sequence && fs->has_frame()) {
        if (!fs->split_fields())
            return false;
    }

    if (!m_dpb_manager->dpb_add(fs, pic))
        return false;
 
    if (m_prev_frame && m_prev_frame->has_frame())
        m_current_picture = NULL;
 
    return true;
}

Decode_Status
VaapiDecoderH264::decode_current_picture()
{
    Decode_Status status;

    if (!m_current_picture)
        return DECODE_SUCCESS;

    status = ensure_context(m_current_picture->pps->sequence);
    if (status != DECODE_SUCCESS)
        return status;

    if (!marking_picture(m_current_picture))
        goto error;

    if (!m_current_picture->decodePicture())
        goto error;

    if (!store_decoded_picture(m_current_picture))
        goto error;

    return DECODE_SUCCESS;
error:
    /* XXX: fix for cases where first field failed to be decoded */
    delete m_current_picture;
    m_current_picture = NULL;
    return DECODE_FAIL;
}

Decode_Status
VaapiDecoderH264::decode_picture(H264NalUnit *nalu, H264SliceHdr *slice_hdr)
{
    VaapiPictureH264 *picture;
    Decode_Status status;
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;

    status = decode_current_picture();
    if (status != DECODE_SUCCESS)
        return status;

    status = ensure_context(sps);
    if (status != DECODE_SUCCESS)
        return status;

    if (m_current_picture) {
        /* Re-use current picture where the first field was decoded */
        INFO("new filed() is called");
        picture = m_current_picture->new_field();
        if (!picture) {
            ERROR("failed to allocate field picture");
            return DECODE_FAIL;
        }
    }
    else {
        /*accquire one surface from mBufPool in base decoder  */
       // INFO("h264: acquire one free surface from pool");
        picture = new VaapiPictureH264(mVADisplay,
                                       mVAContext, 
                                       mBufPool,
                                       VAAPI_PICTURE_STRUCTURE_FRAME);          

        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_FF);
        INFO("H264: new picture %p", picture);
 
       // picture->mTimeStamp = mCurrentPTS;
        /* hack code here */
    }
    m_current_picture = picture;

    picture->pps = pps;

    status = ensure_quant_matrix(picture);
    if (status != DECODE_SUCCESS) {
        ERROR("failed to reset quantizer matrix");
        return status;
    }
    if (!init_picture(picture, slice_hdr, nalu))
        return DECODE_FAIL;
    if (!fill_picture(picture, slice_hdr, nalu))
        return DECODE_FAIL;

    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderH264::decode_slice(H264NalUnit *nalu)
{
    Decode_Status status;
    VaapiPictureH264 *picture;
    VaapiSliceH264 *slice = NULL;
    H264SliceHdr *slice_hdr;
    H264SliceHdr tmp_slice_hdr;
    H264ParserResult result;

    static uint32_t count = 0;
    //INFO("H264: decode slice %d, size=%d, timestamp=%d",count++, nalu->size, mCurrentPTS);

    /* parser the slice header info */
    memset((void*)&tmp_slice_hdr, 0, sizeof(tmp_slice_hdr));
    result = h264_parser_parse_slice_hdr(&m_parser, nalu,
               &tmp_slice_hdr, true, true);
    if (result != H264_PARSER_OK) {
         status = get_status(result);
         goto error;
    }

    /* check info and reset VA resource if necessary */
    status = ensure_context(tmp_slice_hdr.pps->sequence);
      if (status != DECODE_SUCCESS)
          return status;

    /* construct slice and parsing slice header */
    slice = new VaapiSliceH264(mVADisplay, 
                  mVAContext, 
                  nalu->data + nalu->offset,
                  nalu->size);
    slice_hdr = &(slice->slice_hdr);
 
    memcpy((void*)slice_hdr, (void*)&tmp_slice_hdr, sizeof(*slice_hdr));

    if (is_new_picture(nalu, slice_hdr)) {
        status = decode_picture(nalu, slice_hdr);
        if (status != DECODE_SUCCESS)
            goto error;
    }

    if (!fill_slice(slice, nalu)) {
        status = DECODE_FAIL;
        goto error;
    }

    m_current_picture->addSlice((VaapiSlice*)slice);    

    return DECODE_SUCCESS;

error:
    if (slice)
       delete slice;
    return status;
}

Decode_Status
VaapiDecoderH264::decode_nalu(H264NalUnit *nalu)
{
    Decode_Status status;

    switch (nalu->type) {
    case H264_NAL_SLICE_IDR:
        /* fall-through. IDR specifics are handled in init_picture() */
    case H264_NAL_SLICE:
        if (!m_got_sps || !m_got_pps)
            return DECODE_SUCCESS;
        status = decode_slice(nalu);
        break;
    case H264_NAL_SPS:
        status = decode_sps(nalu);
        break;
    case H264_NAL_PPS:
        status = decode_pps(nalu);
        break;
    case H264_NAL_SEI:
        status = decode_sei(nalu);
        break;
    case H264_NAL_SEQ_END:
        status = decode_sequence_end();
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

bool
VaapiDecoderH264::decode_codec_data(uint8_t *buf, uint32_t buf_size)
{
    Decode_Status status;
    H264NalUnit nalu;
    H264ParserResult result;
    uint32_t i, ofs, num_sps, num_pps;

    if (!buf || buf_size == 0)
        return false;

    if (buf_size < 8)
        return false;

    if (buf[0] != 1) {
        ERROR("failed to decode codec-data, not in avcC format");
        return false;
    }

    m_nal_length_size = (buf[4] & 0x03) + 1;

    num_sps = buf[5] & 0x1f;
    ofs = 6;

    for (i = 0; i < num_sps; i++) {
        result = h264_parser_identify_nalu_avc(
            &m_parser,
            buf, ofs, buf_size, 2,
            &nalu
        );
        if (result != H264_PARSER_OK)
            return get_status(result);

        status = decode_sps(&nalu);
        if (status != DECODE_SUCCESS)
            return status;
        ofs = nalu.offset + nalu.size;
    }

    num_pps = buf[ofs];
    ofs++;

    for (i = 0; i < num_pps; i++) {
        result = h264_parser_identify_nalu_avc(
            &m_parser,
            buf, ofs, buf_size, 2,
            &nalu
        );
        if (result != H264_PARSER_OK)
            return get_status(result);

        status = decode_pps(&nalu);
        if (status != DECODE_SUCCESS)
            return status;
        ofs = nalu.offset + nalu.size;
    }

    m_is_avc = true;
    return status;
}

void
VaapiDecoderH264::update_frame_info()
{
    INFO("H264: update frame info ");
    bool size_changed = FALSE;
    H264SPS *sps = &m_last_sps;
    uint32_t width  = (sps->pic_width_in_mbs_minus1 + 1) * 16;
    uint32_t height = (sps->pic_height_in_map_units_minus1 + 1) *
                      (sps->frame_mbs_only_flag ? 1 : 2) * 16;

    uint32_t width_align  = MB_ALIGN(width);
    uint32_t height_align = MB_ALIGN(height);
    
    uint32_t format_info_width_align  = MB_ALIGN(mVideoFormatInfo.width); 
    uint32_t format_info_height_align = MB_ALIGN(mVideoFormatInfo.height); 

    if (width_align != format_info_width_align ||
        height_align != format_info_height_align) {
         size_changed = TRUE;
         mVideoFormatInfo.width = width;
         mVideoFormatInfo.height = height;
         mConfigBuffer.width  = width;
         mConfigBuffer.height = height;
    }    
}

VaapiDecoderH264::VaapiDecoderH264(const char *mimeType)
:VaapiDecoderBase(mimeType)
{
    m_current_picture = NULL;
    m_dpb_manager = NULL;

    memset((void*)&m_parser, 0, sizeof(H264NalParser));
    memset((void*)&m_last_sps, 0, sizeof(H264SPS));
    memset((void*)&m_last_pps, 0, sizeof(H264PPS));

    m_prev_frame             = NULL;
    m_frame_num              = 0;             
    m_prev_frame_num         = 0;         
    m_prev_pic_has_mmco5     = false;    
    m_progressive_sequence   = 0;
    m_prev_pic_structure     = VAAPI_PICTURE_STRUCTURE_FRAME;
    m_frame_num_offset       = 0;

    m_current_picture  = NULL;
    m_mb_width         = 0;
    m_mb_height        = 0;

    m_got_sps          = false;
    m_got_pps          = false;
    m_has_context      = false;
    m_nal_length_size  = 0;
    m_is_avc           = false; 
    m_reset_context    = false;
}

VaapiDecoderH264::~VaapiDecoderH264()
{

   INFO("H264: de-constuct");
   stop();

   if(m_dpb_manager) {
      delete m_dpb_manager;
      m_dpb_manager = NULL;
   }
}


Decode_Status 
VaapiDecoderH264::start(VideoConfigBuffer *buffer)
{
    DEBUG("H264: start");
    Decode_Status status;
    bool  got_config = false;
   
    if(buffer->data == NULL || buffer->size == 0) {
        got_config = false;
        if ((buffer->flag & HAS_SURFACE_NUMBER) && (buffer->flag & HAS_VA_PROFILE)){
            got_config  = true;
        }
    }else{
       if(decode_codec_data((uint8_t*)buffer->data, buffer->size)) {
            H264SPS *sps = &(m_parser.sps[0]);
            uint32_t max_size = get_max_dec_frame_buffering(sps, 1); 
            buffer->profile       = VAProfileH264Baseline;
            buffer->surfaceNumber = max_size + H264_EXTRA_SURFACE_NUMBER; 
            got_config            = true;
       } else {
           ERROR("codec data has some error");
           return DECODE_FAIL;
      } 
    }

    if (got_config) {
        VaapiDecoderBase::start(buffer);
        m_has_context = true;
    }

    return DECODE_SUCCESS;
}

Decode_Status
VaapiDecoderH264::reset(VideoConfigBuffer *buffer)
{
   DEBUG("H264: reset");
   if(m_dpb_manager)
      m_dpb_manager->dpb_flush();

   m_prev_frame = NULL;
   return VaapiDecoderBase::reset(buffer);
}

void 
VaapiDecoderH264::stop(void)
{
   DEBUG("H264: stop");
   flush();
   VaapiDecoderBase::stop();
}

void 
VaapiDecoderH264::flush(void)
{
    DEBUG("H264: flush");
    decode_current_picture();

   if(m_dpb_manager)
       m_dpb_manager->dpb_flush();

    VaapiDecoderBase::flush();
}

Decode_Status 
VaapiDecoderH264::decode(VideoDecodeBuffer *buffer)
{
    Decode_Status status;
    H264ParserResult result;
    H264NalUnit nalu;

    uint8_t *buf;
    uint32_t buf_size = 0;
    uint32_t i, nalu_size, size;
    int32_t  ofs = 0;
    uint32_t start_code;
    bool is_eos = false;
   
    mCurrentPTS = buffer->timeStamp;
    buf         = buffer->data;
    size        = buffer->size;

    do {   
        if (m_is_avc) {
            if (size < m_nal_length_size)
                break;

            nalu_size = 0;
            for (i = 0; i < m_nal_length_size; i++)
                nalu_size = (nalu_size << 8) | buf[i];

            buf_size = m_nal_length_size + nalu_size;
            if (size < buf_size)
                break;

            result = h264_parser_identify_nalu_avc(
                &m_parser,
                buf, 0, buf_size, m_nal_length_size,
                &nalu
            );

            size -= buf_size;
            buf += buf_size;

        } else {
            if (size < 4)
                break;

            /* skip the un-used bit before start code */
            ofs = scan_for_start_code(buf, 0, size, &start_code);
            if (ofs < 0)
                break;

            buf  += ofs;
            size -= ofs;
          
            /* find the length of the nal */
            ofs = (size < 7) ? -1 : scan_for_start_code(buf, 3, size - 3, NULL);
            if (ofs < 0) {
                ofs = size - 3;
            }

            buf_size = ofs + 3;
            size -=  (ofs + 3);

            result = h264_parser_identify_nalu_unchecked(
                &m_parser,
                buf, 0, buf_size,
                &nalu
            );

            buf += buf_size;
        }

        status = get_status(result);
        if (status == DECODE_SUCCESS) {
            status = decode_nalu(&nalu);
        }
        else {
            ERROR("parser nalu uncheck failed code =%d", status);
        }

    } while (status == DECODE_SUCCESS);

    if (is_eos && status == DECODE_SUCCESS)
       status = decode_sequence_end();
  
    if (status == DECODE_SUCCESS && m_reset_context) {
        m_reset_context = false;
        status = DECODE_FORMAT_CHANGE;
    }

    return status;
}


