/*
 *  vaapidecoder_h265.cpp - h265 decoder
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *    Author: YuJiankang<jiankang.yu@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <tr1/functional>
#include "common/log.h"
#include "vaapidecoder_h265.h"
#include "vaapidecoder_factory.h"

namespace YamiMediaCodec{

typedef enum {
    VAAPI_DECODER_UNIT_FLAG_FRAME_START = (1 << 0),
    VAAPI_DECODER_UNIT_FLAG_FRAME_END   = (1 << 1),
    VAAPI_DECODER_UNIT_FLAG_STREAM_END  = (1 << 2),
    VAAPI_DECODER_UNIT_FLAG_SLICE       = (1 << 3),
    VAAPI_DECODER_UNIT_FLAG_SKIP        = (1 << 4),
    VAAPI_DECODER_UNIT_FLAG_LAST        = (1 << 5)
} VaapiDecoderUnitFlags;

enum
{
  VAAPI_DECODER_UNIT_FLAG_AU_START =
      (VAAPI_DECODER_UNIT_FLAG_LAST << 0),
  VAAPI_DECODER_UNIT_FLAG_AU_END = (VAAPI_DECODER_UNIT_FLAG_LAST << 1),

  VAAPI_DECODER_UNIT_FLAGS_AU = (VAAPI_DECODER_UNIT_FLAG_AU_START |
      VAAPI_DECODER_UNIT_FLAG_AU_END),
};

enum
{
  VAAPI_PICTURE_FLAG_IDR = (VAAPI_PICTURE_FLAG_LAST << 0),
  VAAPI_PICTURE_FLAG_REFERENCE2 = (VAAPI_PICTURE_FLAG_LAST << 1),
  VAAPI_PICTURE_FLAG_AU_START = (VAAPI_PICTURE_FLAG_LAST << 4),
  VAAPI_PICTURE_FLAG_AU_END = (VAAPI_PICTURE_FLAG_LAST << 5),
  VAAPI_PICTURE_FLAG_RPS_ST_CURR_BEFORE =
      (VAAPI_PICTURE_FLAG_LAST << 6),
  VAAPI_PICTURE_FLAG_RPS_ST_CURR_AFTER = (VAAPI_PICTURE_FLAG_LAST << 7),
  VAAPI_PICTURE_FLAG_RPS_ST_FOLL = (VAAPI_PICTURE_FLAG_LAST << 8),
  VAAPI_PICTURE_FLAG_RPS_LT_CURR = (VAAPI_PICTURE_FLAG_LAST << 9),
  VAAPI_PICTURE_FLAG_RPS_LT_FOLL = (VAAPI_PICTURE_FLAG_LAST << 10),

  VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE =
      (VAAPI_PICTURE_FLAG_REFERENCE),
  VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE =
      (VAAPI_PICTURE_FLAG_REFERENCE | VAAPI_PICTURE_FLAG_REFERENCE2),
  VAAPI_PICTURE_FLAGS_REFERENCE =
      (VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
      VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE),

  VAAPI_PICTURE_FLAGS_RPS_ST =
      (VAAPI_PICTURE_FLAG_RPS_ST_CURR_BEFORE |
      VAAPI_PICTURE_FLAG_RPS_ST_CURR_AFTER |
      VAAPI_PICTURE_FLAG_RPS_ST_FOLL),
  VAAPI_PICTURE_FLAGS_RPS_LT =
      (VAAPI_PICTURE_FLAG_RPS_LT_CURR | VAAPI_PICTURE_FLAG_RPS_LT_FOLL),
};

#define VAAPI_PICTURE_IS_IDR(picture) \
    (VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_IDR))

#define VAAPI_PICTURE_IS_SHORT_TERM_REFERENCE(picture)      \
    ((VAAPI_PICTURE_FLAGS(picture) &                        \
      VAAPI_PICTURE_FLAGS_REFERENCE) ==                     \
     VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE)

#define VAAPI_PICTURE_IS_LONG_TERM_REFERENCE(picture)       \
    ((VAAPI_PICTURE_FLAGS(picture) &                        \
      VAAPI_PICTURE_FLAGS_REFERENCE) ==                     \
     VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE)

#define G_N_ELEMENTS(arr)		(sizeof (arr) / sizeof ((arr)[0]))

#define RSV_VCL_N10 10
#define RSV_VCL_N12 12
#define RSV_VCL_N14 14
VaapiH265FrameStore::VaapiH265FrameStore(PicturePtr& pic)
{
    buffer = pic;
}

VaapiH265FrameStore::~VaapiH265FrameStore()
{
}

VaapiDecoderH265::VaapiDecoderH265()
    :m_nalLengthSize(4), m_newBitstream(true)
{
    memset(&this->m_zeroInitStart, 0,
           &this->m_zeroInitEnd - &this->m_zeroInitStart);
}

VaapiDecoderH265::~VaapiDecoderH265()
{
    stop();
}
Decode_Status VaapiDecoderH265::start(VideoConfigBuffer *buffer)
{
    Decode_Status status;
    buffer->profile = VAProfileHEVCMain;
    buffer->surfaceNumber = 30;
    m_configBuffer = *buffer;

    /* check info and reset VA resource if necessary */
    status = ensureContext();
    return status;
}

Decode_Status VaapiDecoderH265::reset(VideoConfigBuffer *buffer)
{
    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderH265::stop(void)
{
    VaapiDecoderBase::stop();
}

void VaapiDecoderH265::flush(void)
{
    if (m_currentPicture) {
        m_currentPicture->decode();
        dpb_add (m_currentPicture,  &m_slice_hdr, NULL);
        m_currentPicture.reset();
    }

    dpb_flush();
}

Decode_Status VaapiDecoderH265::ensureContext()
{
    Decode_Status status;
    m_configBuffer.surfaceWidth = m_configBuffer.width;
    m_configBuffer.surfaceHeight = m_configBuffer.height;
    status = VaapiDecoderBase::start(&m_configBuffer);
    return status;
}

static inline int32_t
scanForStartCode(const uint8_t * data,
                 uint32_t offset, uint32_t size, uint32_t * scp)
{
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

Decode_Status VaapiDecoderH265::decode(VideoDecodeBuffer * buffer)
{
    int32_t ofs = 0;
    H265NalUnit nalu;
    uint8_t *buf;
    uint32_t bufSize = 0;
    uint32_t i, naluSize, size;
    uint32_t startCode;
    bool isEOS = false;
    bool at_au_end = false;
    Decode_Status status = DECODE_SUCCESS;

    buf = buffer->data;
    size = buffer->size;

    if (buf == NULL && size == 0) {
        return DECODE_SUCCESS;
    }

    do {
        if (size < 4) {
            break;
        }
        if (buffer->flag & IS_NAL_UNIT) {
            bufSize = buffer->size;
            size = 0;
        } else {
            /* skip the un-used bit before start code */
            ofs = scanForStartCode(buf, 0, size, &startCode);
            if (ofs < 0) {
                break;
            }

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
	      if (size < 4) {
                at_au_end = true;
	      }
        }

        h265_parser_identify_nalu_unchecked(&m_parser,
                                                     buf, 0, bufSize,
                                                     &nalu);

        buf += bufSize;
        if (at_au_end) {
            m_flags |= VAAPI_DECODER_UNIT_FLAG_FRAME_END | VAAPI_DECODER_UNIT_FLAG_AU_END;
        }

        status = decodeNalu(&nalu);
    } while (status == DECODE_SUCCESS);

#if 0
    if (isEOS && status == DECODE_SUCCESS)
        status = decodeSequenceEnd();

    if (status == DECODE_FORMAT_CHANGE && m_resetContext) {
        WARNING("H264 decoder format change happens");
        m_resetContext = false;
    }
#endif
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH265::decodeNalu(H265NalUnit *nalu)
{
    Decode_Status status = DECODE_SUCCESS;

    switch (nalu->type) {
    case H265_NAL_SLICE_TRAIL_N:
    case H265_NAL_SLICE_TRAIL_R:
    case H265_NAL_SLICE_TSA_N:
    case H265_NAL_SLICE_TSA_R:
    case H265_NAL_SLICE_STSA_N:
    case H265_NAL_SLICE_STSA_R:
    case H265_NAL_SLICE_RADL_N:
    case H265_NAL_SLICE_RADL_R:
    case H265_NAL_SLICE_RASL_N:
    case H265_NAL_SLICE_RASL_R:
    case H265_NAL_SLICE_BLA_W_LP:
    case H265_NAL_SLICE_BLA_W_RADL:
    case H265_NAL_SLICE_BLA_N_LP:
    case H265_NAL_SLICE_IDR_W_RADL:
    case H265_NAL_SLICE_IDR_N_LP:
    case H265_NAL_SLICE_CRA_NUT:
        if (!m_gotSPS || !m_gotPPS)
            return DECODE_SUCCESS;

        //m_newBitstream = false;
        m_prev_nal_is_eos = false;
        status = decodeSlice(nalu);
        break;

    case H265_NAL_SPS:
        status = decodeSPS(nalu);
        break;

    case H265_NAL_PPS:
        status = decodePPS(nalu);
        break;

    case H265_NAL_PREFIX_SEI:
        status = decodeSEI(nalu);
        break;

    case H265_NAL_SUFFIX_SEI:
    case H265_NAL_EOS:
        m_flags |= VAAPI_DECODER_UNIT_FLAG_FRAME_END;
        m_flags |= VAAPI_DECODER_UNIT_FLAG_AU_END;
        break;

    case H265_NAL_VPS:
        break;
    default:
        WARNING("unsupported NAL unit type %d", nalu->type);
        break;
    }

    return status;
}

uint32_t VaapiDecoderH265::get_slice_data_byte_offset (H265SliceHdr *slice_hdr, uint32_t nal_header_bytes)
{
    uint32_t epb_count;
    epb_count = slice_hdr->n_emulation_prevention_bytes;

    return nal_header_bytes + (slice_hdr->header_size + 7) / 8 - epb_count;
}

void VaapiDecoderH265::vaapi_init_picture (VAPictureHEVC * pic)
{
    pic->pic_order_cnt = 0;
    pic->picture_id = VA_INVALID_SURFACE;
    pic->flags = VA_PICTURE_HEVC_INVALID;
}

void VaapiDecoderH265::vaapi_fill_picture (VAPictureHEVC * pic, VaapiDecPictureH265 * picture,
    uint32_t pictureStructure)
{

    if (!pictureStructure)
        pictureStructure = picture->m_structure;

    pic->picture_id = picture->getSurfaceID();
    pic->pic_order_cnt = picture->m_poc;
    pic->flags = 0;

    /* Set the VAPictureHEVC flags */
    if (VAAPI_PICTURE_IS_LONG_TERM_REFERENCE (picture))
        pic->flags |= VA_PICTURE_HEVC_LONG_TERM_REFERENCE;

    if (VAAPI_PICTURE_FLAG_IS_SET (picture,
        VAAPI_PICTURE_FLAG_RPS_ST_CURR_BEFORE))
        pic->flags |= VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE;

    else if (VAAPI_PICTURE_FLAG_IS_SET (picture,
        VAAPI_PICTURE_FLAG_RPS_ST_CURR_AFTER))
        pic->flags |= VA_PICTURE_HEVC_RPS_ST_CURR_AFTER;

    else if (VAAPI_PICTURE_FLAG_IS_SET (picture,
        VAAPI_PICTURE_FLAG_RPS_LT_CURR))
        pic->flags |= VA_PICTURE_HEVC_RPS_LT_CURR;

    switch (pictureStructure) {
    case VAAPI_PICTURE_STRUCTURE_FRAME:
        break;
    case VAAPI_PICTURE_STRUCTURE_TOP_FIELD:
        pic->flags |= VA_PICTURE_HEVC_FIELD_PIC;
        break;
    case VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD:
        pic->flags |= VA_PICTURE_HEVC_FIELD_PIC;
        pic->flags |= VA_PICTURE_HEVC_BOTTOM_FIELD;
        break;
    default:
        break;
    }
}

uint32_t VaapiDecoderH265::get_index_for_RefPicListX (VAPictureHEVC * ReferenceFrames,
    VaapiDecPictureH265 * pic)
{
  int32_t i;

  for (i = 0; i < 15; i++) {
    if ((ReferenceFrames[i].picture_id != VA_INVALID_ID) && pic) {
      if ((ReferenceFrames[i].pic_order_cnt == pic->m_poc) &&
          (ReferenceFrames[i].picture_id == pic->getSurfaceID())) {
        return i;
      }
    }
  }
  return 0xff;
}

bool VaapiDecoderH265::fillSlice(VaapiDecPictureH265 * picture, VASliceParameterBufferHEVC* sliceParam, H265SliceHdr * sliceHdr,
                                 H265NalUnit * nalu)
{
    /* Fill in VASliceParameterBufferHEVC */
    sliceParam->LongSliceFlags.value = 0;
    sliceParam->slice_data_byte_offset =
    get_slice_data_byte_offset (sliceHdr, nalu->header_bytes);

    sliceParam->slice_segment_address = sliceHdr->segment_address;

    #define COPY_LFF(f) \
    sliceParam->LongSliceFlags.fields.f = (sliceHdr)->f

    if (VAAPI_PICTURE_FLAG_IS_SET (picture, VAAPI_PICTURE_FLAG_AU_END)) {
        sliceParam->LongSliceFlags.fields.LastSliceOfPic = 1;
    }
    else {
        sliceParam->LongSliceFlags.fields.LastSliceOfPic = 0;
    }

    COPY_LFF (dependent_slice_segment_flag);

    /* use most recent independent slice segment header syntax elements
    * for filling the missing fields in dependent slice segment header */
    if (sliceHdr->dependent_slice_segment_flag)
        sliceHdr = &m_prev_independent_slice;

    COPY_LFF (mvd_l1_zero_flag);
    COPY_LFF (cabac_init_flag);
    COPY_LFF (collocated_from_l0_flag);
    sliceParam->LongSliceFlags.fields.color_plane_id =
    sliceHdr->colour_plane_id;
    sliceParam->LongSliceFlags.fields.slice_type = sliceHdr->type;
    sliceParam->LongSliceFlags.fields.slice_sao_luma_flag =
    sliceHdr->sao_luma_flag;
    sliceParam->LongSliceFlags.fields.slice_sao_chroma_flag =
    sliceHdr->sao_chroma_flag;
    sliceParam->LongSliceFlags.fields.slice_temporal_mvp_enabled_flag =
    sliceHdr->temporal_mvp_enabled_flag;
    sliceParam->LongSliceFlags.fields.slice_deblocking_filter_disabled_flag =
    sliceHdr->deblocking_filter_disabled_flag;
    sliceParam->LongSliceFlags.
    fields.slice_loop_filter_across_slices_enabled_flag =
    sliceHdr->loop_filter_across_slices_enabled_flag;

    if (!sliceHdr->temporal_mvp_enabled_flag)
        sliceParam->collocated_ref_idx = 0xFF;
    else
        sliceParam->collocated_ref_idx = sliceHdr->collocated_ref_idx;

    sliceParam->num_ref_idx_l0_active_minus1 =
    sliceHdr->num_ref_idx_l0_active_minus1;
    sliceParam->num_ref_idx_l1_active_minus1 =
    sliceHdr->num_ref_idx_l1_active_minus1;
    sliceParam->slice_qp_delta = sliceHdr->qp_delta;
    sliceParam->slice_cb_qp_offset = sliceHdr->cb_qp_offset;
    sliceParam->slice_cr_qp_offset = sliceHdr->cr_qp_offset;
    sliceParam->slice_beta_offset_div2 = sliceHdr->beta_offset_div2;
    sliceParam->slice_tc_offset_div2 = sliceHdr->tc_offset_div2;
    sliceParam->five_minus_max_num_merge_cand =
    sliceHdr->five_minus_max_num_merge_cand;

    if (!fill_RefPicList(picture, sliceParam, sliceHdr)) {
        return false;
    }
    if (!fill_pred_weight_table (sliceParam, sliceHdr)) {
        return false;
    }

    return true;
}

bool VaapiDecoderH265::fillPicture(PicturePtr& picture, H265NalUnit * nalu)
{
    uint32_t i, n;
    H265PPS *const pps = &m_lastPPS;
    H265SPS *const sps = &m_lastSPS;
    VAPictureParameterBufferHEVC *pic_param;
    if (!picture->editPicture(pic_param)) {
        return false;
    }

    picture->m_param = pic_param;

    /* Fill in VAPictureParameterBufferH264 */
    pic_param->pic_fields.value = 0;
    pic_param->slice_parsing_fields.value = 0;

    /* Fill in VAPictureHEVC */
    vaapi_fill_picture (&pic_param->CurrPic, picture.get(), 0);

    /* Fill in ReferenceFrames */
    for (i = 0, n = 0; i < m_dpb_count; i++) {

        VaapiH265FrameStore::Ptr& fs = m_dpb[i];
        if ((fs && vaapi_frame_store_has_reference (fs))) {
            vaapi_fill_picture (&pic_param->ReferenceFrames[n++], fs->buffer.get(),
            fs->buffer->m_structure);
        }

        if (n >= G_N_ELEMENTS (pic_param->ReferenceFrames))
            break;
    }

    for (; n < G_N_ELEMENTS (pic_param->ReferenceFrames); n++)
        vaapi_init_picture (&pic_param->ReferenceFrames[n]);

    #define COPY_FIELD(s, f) \
    pic_param->f = (s)->f
    #define COPY_BFM(a, s, f) \
    pic_param->a.bits.f = (s)->f

    COPY_FIELD (sps, pic_width_in_luma_samples);
    COPY_FIELD (sps, pic_height_in_luma_samples);
    COPY_BFM (pic_fields, sps, chroma_format_idc);
    COPY_BFM (pic_fields, sps, separate_colour_plane_flag);
    COPY_BFM (pic_fields, sps, pcm_enabled_flag);
    COPY_BFM (pic_fields, sps, scaling_list_enabled_flag);
    COPY_BFM (pic_fields, pps, transform_skip_enabled_flag);
    COPY_BFM (pic_fields, sps, amp_enabled_flag);
    COPY_BFM (pic_fields, sps, strong_intra_smoothing_enabled_flag);
    COPY_BFM (pic_fields, pps, sign_data_hiding_enabled_flag);
    COPY_BFM (pic_fields, pps, constrained_intra_pred_flag);
    COPY_BFM (pic_fields, pps, cu_qp_delta_enabled_flag);
    COPY_BFM (pic_fields, pps, weighted_pred_flag);
    COPY_BFM (pic_fields, pps, weighted_bipred_flag);
    COPY_BFM (pic_fields, pps, transquant_bypass_enabled_flag);
    COPY_BFM (pic_fields, pps, tiles_enabled_flag);
    COPY_BFM (pic_fields, pps, entropy_coding_sync_enabled_flag);
    pic_param->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag =
    pps->loop_filter_across_slices_enabled_flag;
    COPY_BFM (pic_fields, pps, loop_filter_across_tiles_enabled_flag);
    COPY_BFM (pic_fields, sps, pcm_loop_filter_disabled_flag);
    /* Fix: Assign value based on sps_max_num_reorder_pics */
    pic_param->pic_fields.bits.NoPicReorderingFlag = 0;
    /* Fix: Enable if picture has no B slices */
    pic_param->pic_fields.bits.NoBiPredFlag = 0;

    pic_param->sps_max_dec_pic_buffering_minus1 =
    sps->max_dec_pic_buffering_minus1[0];
    COPY_FIELD (sps, bit_depth_luma_minus8);
    COPY_FIELD (sps, bit_depth_chroma_minus8);
    COPY_FIELD (sps, pcm_sample_bit_depth_luma_minus1);
    COPY_FIELD (sps, pcm_sample_bit_depth_chroma_minus1);
    COPY_FIELD (sps, log2_min_luma_coding_block_size_minus3);
    COPY_FIELD (sps, log2_diff_max_min_luma_coding_block_size);
    COPY_FIELD (sps, log2_min_transform_block_size_minus2);
    COPY_FIELD (sps, log2_diff_max_min_transform_block_size);
    COPY_FIELD (sps, log2_min_pcm_luma_coding_block_size_minus3);
    COPY_FIELD (sps, log2_diff_max_min_pcm_luma_coding_block_size);
    COPY_FIELD (sps, max_transform_hierarchy_depth_intra);
    COPY_FIELD (sps, max_transform_hierarchy_depth_inter);
    COPY_FIELD (pps, init_qp_minus26);
    COPY_FIELD (pps, diff_cu_qp_delta_depth);
    pic_param->pps_cb_qp_offset = pps->cb_qp_offset;
    pic_param->pps_cr_qp_offset = pps->cr_qp_offset;
    COPY_FIELD (pps, log2_parallel_merge_level_minus2);
    COPY_FIELD (pps, num_tile_columns_minus1);
    COPY_FIELD (pps, num_tile_rows_minus1);
    for (i = 0; i <= pps->num_tile_columns_minus1; i++)
        pic_param->column_width_minus1[i] = pps->column_width_minus1[i];
    for (; i < 19; i++)
        pic_param->column_width_minus1[i] = 0;
    for (i = 0; i <= pps->num_tile_rows_minus1; i++)
        pic_param->row_height_minus1[i] = pps->row_height_minus1[i];
    for (; i < 21; i++)
        pic_param->row_height_minus1[i] = 0;

    COPY_BFM (slice_parsing_fields, pps, lists_modification_present_flag);
    COPY_BFM (slice_parsing_fields, sps, long_term_ref_pics_present_flag);
    pic_param->slice_parsing_fields.bits.sps_temporal_mvp_enabled_flag =
    sps->temporal_mvp_enabled_flag;
    COPY_BFM (slice_parsing_fields, pps, cabac_init_present_flag);
    COPY_BFM (slice_parsing_fields, pps, output_flag_present_flag);
    COPY_BFM (slice_parsing_fields, pps, dependent_slice_segments_enabled_flag);
    pic_param->slice_parsing_fields.
    bits.pps_slice_chroma_qp_offsets_present_flag =
    pps->slice_chroma_qp_offsets_present_flag;
    COPY_BFM (slice_parsing_fields, sps, sample_adaptive_offset_enabled_flag);
    COPY_BFM (slice_parsing_fields, pps, deblocking_filter_override_enabled_flag);
    pic_param->slice_parsing_fields.bits.pps_disable_deblocking_filter_flag =
    pps->deblocking_filter_disabled_flag;
    COPY_BFM (slice_parsing_fields, pps,
    slice_segment_header_extension_present_flag);
    pic_param->slice_parsing_fields.bits.RapPicFlag = picture->m_RapPicFlag;
    pic_param->slice_parsing_fields.bits.IdrPicFlag = VAAPI_PICTURE_FLAG_IS_SET (picture, VAAPI_PICTURE_FLAG_IDR);
    pic_param->slice_parsing_fields.bits.IntraPicFlag = picture->m_IntraPicFlag;

    COPY_FIELD (sps, log2_max_pic_order_cnt_lsb_minus4);
    COPY_FIELD (sps, num_short_term_ref_pic_sets);
    pic_param->num_long_term_ref_pic_sps = sps->num_long_term_ref_pics_sps;
    COPY_FIELD (pps, num_ref_idx_l0_default_active_minus1);
    COPY_FIELD (pps, num_ref_idx_l1_default_active_minus1);
    pic_param->pps_beta_offset_div2 = pps->beta_offset_div2;
    pic_param->pps_tc_offset_div2 = pps->tc_offset_div2;
    COPY_FIELD (pps, num_extra_slice_header_bits);

    /*Fixme: Set correct value as mentioned in va_dec_hevc.h */
    pic_param->st_rps_bits = 0;

    return true;
}

void VaapiDecoderH265::fill_iq_matrix_4x4 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;
    for (i = 0; i < G_N_ELEMENTS (iq_matrix->ScalingList4x4); i++) {
        h265_quant_matrix_4x4_get_raster_from_zigzag (iq_matrix->ScalingList4x4
        [i], scaling_list->scaling_lists_4x4[i]);
    }
}

void VaapiDecoderH265::fill_iq_matrix_8x8 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;

    for (i = 0; i < G_N_ELEMENTS (iq_matrix->ScalingList8x8); i++) {
        h265_quant_matrix_8x8_get_raster_from_zigzag (iq_matrix->ScalingList8x8
        [i], scaling_list->scaling_lists_8x8[i]);
    }
}

void VaapiDecoderH265::fill_iq_matrix_16x16 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;

    for (i = 0; i < G_N_ELEMENTS (iq_matrix->ScalingList16x16); i++) {
        h265_quant_matrix_16x16_get_raster_from_zigzag
        (iq_matrix->ScalingList16x16[i], scaling_list->scaling_lists_16x16[i]);
    }
}

void VaapiDecoderH265::fill_iq_matrix_32x32 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;
    for (i = 0; i < G_N_ELEMENTS (iq_matrix->ScalingList32x32); i++) {
        h265_quant_matrix_32x32_get_raster_from_zigzag
        (iq_matrix->ScalingList32x32[i], scaling_list->scaling_lists_32x32[i]);
    }
}

void VaapiDecoderH265::fill_iq_matrix_dc_16x16 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;

    for (i = 0; i < 6; i++)
        iq_matrix->ScalingListDC16x16[i] =
        scaling_list->scaling_list_dc_coef_minus8_16x16[i] + 8;
}

void VaapiDecoderH265::fill_iq_matrix_dc_32x32 (VAIQMatrixBufferHEVC * iq_matrix,
    H265ScalingList * scaling_list)
{
    uint32_t i;

    for (i = 0; i < 2; i++)
        iq_matrix->ScalingListDC32x32[i] =
        scaling_list->scaling_list_dc_coef_minus8_32x32[i] + 8;
}

/* fill slice parameter buffers functions*/
bool VaapiDecoderH265::ensureQuantMatrix(PicturePtr& pic)
{
    H265PPS *const pps = &m_lastPPS;
    H265SPS *const sps = &m_lastSPS;
    H265ScalingList *scaling_list = NULL;
    VAIQMatrixBufferHEVC *iq_matrix;

    if (pps &&
       (pps->scaling_list_data_present_flag ||
          (sps->scaling_list_enabled_flag
              && !sps->scaling_list_data_present_flag)))
        scaling_list = &pps->scaling_list;
    else if (sps && sps->scaling_list_enabled_flag
      && sps->scaling_list_data_present_flag)
        scaling_list = &sps->scaling_list;
    else
        return true;

    if (!pic->editIqMatrix(iq_matrix))
        return false;

    /* Only supporting 4:2:0 */
    if (sps->chroma_format_idc != 1)
        return false;

    fill_iq_matrix_4x4 (iq_matrix, scaling_list);
    fill_iq_matrix_8x8 (iq_matrix, scaling_list);
    fill_iq_matrix_16x16 (iq_matrix, scaling_list);
    fill_iq_matrix_32x32 (iq_matrix, scaling_list);
    fill_iq_matrix_dc_16x16 (iq_matrix, scaling_list);
    fill_iq_matrix_dc_32x32 (iq_matrix, scaling_list);
    return true;
}

bool
nal_is_idr (uint8_t nal_type)
{
    if ((nal_type == H265_NAL_SLICE_IDR_W_RADL) ||
        (nal_type == H265_NAL_SLICE_IDR_N_LP))
        return true;
    return false;
}

bool
nal_is_irap (uint8_t nal_type)
{
    if ((nal_type >= H265_NAL_SLICE_BLA_W_LP) &&
        (nal_type <= RESERVED_IRAP_NAL_TYPE_MAX))
        return true;
    return false;
}

bool
nal_is_bla (uint8_t nal_type)
{
    if ((nal_type >= H265_NAL_SLICE_BLA_W_LP) &&
        (nal_type <= H265_NAL_SLICE_BLA_N_LP))
        return true;
    return false;
}

bool
nal_is_cra (uint8_t nal_type)
{
    if (nal_type == H265_NAL_SLICE_CRA_NUT)
        return true;
    return false;
}

bool
nal_is_radl (uint8_t nal_type)
{
    if ((nal_type >= H265_NAL_SLICE_RADL_N) &&
        (nal_type <= H265_NAL_SLICE_RADL_R))
        return true;
    return false;
}

bool
nal_is_rasl (uint8_t nal_type)
{
  if ((nal_type >= H265_NAL_SLICE_RASL_N) &&
      (nal_type <= H265_NAL_SLICE_RASL_R))
    return true;
  return false;
}

bool
nal_is_slice (uint8_t nal_type)
{
  if ((nal_type >= H265_NAL_SLICE_TRAIL_N) &&
      (nal_type <= H265_NAL_SLICE_CRA_NUT))
    return true;
  return false;
}

bool
nal_is_ref (uint8_t nal_type)
{
    bool ret = false;
    switch (nal_type) {
    case H265_NAL_SLICE_TRAIL_N:
    case H265_NAL_SLICE_TSA_N:
    case H265_NAL_SLICE_STSA_N:
    case H265_NAL_SLICE_RADL_N:
    case H265_NAL_SLICE_RASL_N:
    case RSV_VCL_N10:
    case RSV_VCL_N12:
    case RSV_VCL_N14:
        ret = false;
        break;
    default:
        ret = true;
        break;
    }
    return ret;
}

/* Get number of reference frames to use */
uint32_t VaapiDecoderH265::get_max_dec_frame_buffering (H265SPS * sps)
{
  /* Fixme: Add limit check based on Annex A */
  return MAX (1, (sps->max_dec_pic_buffering_minus1[0] + 1));
}

Decode_Status VaapiDecoderH265::ensure_dpb (H265SPS * sps)
{
    uint32_t dpb_size;
    dpb_size = get_max_dec_frame_buffering (sps);
    if (m_dpb_size < dpb_size) {
    }

    /* Reset DPB */
    if (!dpb_reset (dpb_size))
        return DECODE_FAIL;

    return DECODE_SUCCESS;
}

/* 8.3.1 - Decoding process for picture order count */
void VaapiDecoderH265::init_picture_poc (PicturePtr& picture, H265SliceHdr *slice_hdr, H265NalUnit * nalu)
{
    H265PPS *const pps = slice_hdr->pps;
    H265SPS *const sps = pps->sps;

    const uint32_t MaxPicOrderCntLsb =
    1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    uint8_t nal_type = nalu->type;
    uint8_t temporal_id = nalu->temporal_id_plus1 - 1;

    m_prev_poc_lsb = m_poc_lsb;
    m_prev_poc_msb = m_poc_msb;

    if (!(nal_is_irap (nal_type) && picture->m_NoRaslOutputFlag)) {
        m_prev_poc_lsb = m_prev_tid0pic_poc_lsb;
        m_prev_poc_msb = m_prev_tid0pic_poc_msb;
    }

    /* Finding PicOrderCntMsb */
    if (nal_is_irap (nal_type) && picture->m_NoRaslOutputFlag)
        m_poc_msb = 0;
    else {
        /* (8-1) */
        if ((slice_hdr->pic_order_cnt_lsb < m_prev_poc_lsb) &&
           ((m_prev_poc_lsb - slice_hdr->pic_order_cnt_lsb) >=
            (MaxPicOrderCntLsb / 2)))
            m_poc_msb = m_prev_poc_msb + MaxPicOrderCntLsb;

        else if ((slice_hdr->pic_order_cnt_lsb > m_prev_poc_lsb) &&
            ((slice_hdr->pic_order_cnt_lsb - m_prev_poc_lsb) >
            (MaxPicOrderCntLsb / 2)))
            m_poc_msb = m_prev_poc_msb - MaxPicOrderCntLsb;
        else
            m_poc_msb = m_prev_poc_msb;
    }

    /* (8-2) */
    m_poc = picture->m_poc = m_poc_msb + slice_hdr->pic_order_cnt_lsb;
    m_poc_lsb = picture->m_poc_lsb = slice_hdr->pic_order_cnt_lsb;

    if (nal_is_idr (nal_type)) {
        picture->m_poc = 0;
        picture->m_poc_lsb = 0;
        m_poc_lsb = 0;
        m_poc_msb = 0;
        m_prev_poc_lsb = 0;
        m_prev_poc_msb = 0;
        m_prev_tid0pic_poc_lsb = 0;
        m_prev_tid0pic_poc_msb = 0;
    }

    if (!temporal_id && !nal_is_rasl (nal_type) &&
    !nal_is_radl (nal_type) && nal_is_ref (nal_type)) {
        m_prev_tid0pic_poc_lsb = slice_hdr->pic_order_cnt_lsb;
        m_prev_tid0pic_poc_msb = m_poc_msb;
    }
}

void VaapiDecoderH265::vaapi_picture_h265_set_reference(VaapiDecPictureH265* picture, uint32_t referenceFlags)
{
    VAAPI_PICTURE_FLAG_UNSET(picture, VAAPI_PICTURE_FLAGS_RPS_ST | VAAPI_PICTURE_FLAGS_RPS_LT);
    VAAPI_PICTURE_FLAG_UNSET(picture, VAAPI_PICTURE_FLAGS_REFERENCE);
    VAAPI_PICTURE_FLAG_SET(picture, referenceFlags);
}


/* Get the dpb picture having the specifed poc or poc_lsb */
VaapiDecPictureH265 *VaapiDecoderH265::dpb_get_picture (int32_t poc, bool match_lsb)
{
    uint32_t i;

    for (i = 0; i < m_dpb_count; i++) {
    VaapiDecPictureH265 *picture = m_dpb[i]->buffer.get();

    if (picture && VAAPI_PICTURE_FLAG_IS_SET (picture,
            VAAPI_PICTURE_FLAGS_REFERENCE)) {
        if (match_lsb) {
            if (picture->m_poc_lsb == poc)
                return picture;
            } else {
                if (picture->m_poc == poc)
                    return picture;
            }
        }
    }
    return NULL;
}

/* Get the dpb picture having the specifed poc and shor/long ref flags */
VaapiDecPictureH265 *VaapiDecoderH265::dpb_get_ref_picture (int32_t poc, bool is_short)
{
    uint32_t i;

    for (i = 0; i < m_dpb_count; i++) {
        VaapiDecPictureH265 *picture = m_dpb[i]->buffer.get();

        if (picture && picture->m_poc == poc) {
            if (is_short && VAAPI_PICTURE_IS_SHORT_TERM_REFERENCE (picture))
                return picture;
            else if (VAAPI_PICTURE_IS_LONG_TERM_REFERENCE (picture))
                return picture;
        }
    }
    return NULL;
}

void VaapiDecoderH265::dpb_remove_index(uint32_t index)
{
    uint32_t i, num_frames = --m_dpb_count;
    if (index != num_frames)
        m_dpb[index] = m_dpb[num_frames];
}

/* Finds the picture with the lowest POC that needs to be output */
int32_t VaapiDecoderH265::dpb_find_lowest_poc (VaapiDecPictureH265 ** found_picture_ptr)
{
    VaapiDecPictureH265 *found_picture = NULL;
    uint32_t i, found_index;

    for (i = 0; i < m_dpb_count; i++) {
        VaapiDecPictureH265 *picture = m_dpb[i]->buffer.get();
        if (picture && !picture->m_output_needed)
            continue;
        if (!found_picture || found_picture->m_poc > picture->m_poc)
            found_picture = picture, found_index = i;
    }

    if (found_picture_ptr)
        *found_picture_ptr = found_picture;
    return found_picture ? found_index : -1;
}

bool VaapiDecoderH265::vaapi_frame_store_has_reference (VaapiH265FrameStore::Ptr fs)
{
    if (VAAPI_PICTURE_IS_REFERENCE (fs->buffer))
        return true;
    return false;
}

bool VaapiDecoderH265::dpb_output(VaapiH265FrameStore::Ptr &frameStore)
{
    PicturePtr frame;
    if (frameStore) {
        frame = frameStore->buffer;
    }
    frame->m_output_needed = false;

    DEBUG("DPB: output picture(Addr:%p, Poc:%d)", frame.get(), frame->m_POC);
    return outputPicture(frame) == DECODE_SUCCESS;
}

bool VaapiDecoderH265::dpb_bump (VaapiDecPictureH265 * picture)
{
    VaapiDecPictureH265 *found_picture;
    int32_t found_index;
    bool success;
    found_index = dpb_find_lowest_poc (&found_picture);
    if (found_index < 0)
        return false;
    success = dpb_output(m_dpb[found_index]);
    if (!vaapi_frame_store_has_reference (m_dpb[found_index]))
        dpb_remove_index (found_index);

    return success;
}

void VaapiDecoderH265::dpb_clear (bool hard_flush)
{
    PicturePtr pic;
    uint32_t i;

    if (hard_flush) {
        for (i = 0; i < m_dpb_count; i++) {
            dpb_remove_index (i);
        }
        m_dpb_count = 0;
    } else {
        /* Remove unused pictures from DPB */
        i = 0;
        while (i < m_dpb_count) {
            VaapiH265FrameStore::Ptr fs = m_dpb[i];
            pic = fs->buffer;
            if (!pic->m_output_needed && !vaapi_frame_store_has_reference (fs))
                dpb_remove_index (i);
            else
                i++;
        }
    }
}

void VaapiDecoderH265::dpb_flush ()
{
    /* Output any frame remaining in DPB */
    while (dpb_bump (NULL));
    dpb_clear (true);
}

int32_t VaapiDecoderH265::dpb_get_num_need_output ()
{
    uint32_t i = 0, n_output_needed = 0;

    while (i < m_dpb_count) {
        VaapiH265FrameStore::Ptr fs = m_dpb[i];
        if (fs->buffer->m_output_needed)
            n_output_needed++;
        i++;
    }
    return n_output_needed;
}

bool VaapiDecoderH265::check_latency_cnt ()
{
    PicturePtr tmp_pic;
    uint32_t i = 0;

    while (i < m_dpb_count) {
        VaapiH265FrameStore::Ptr fs = m_dpb[i];
        tmp_pic = fs->buffer;
        if (tmp_pic->m_output_needed) {
            if (tmp_pic->m_pic_latency_cnt >= m_SpsMaxLatencyPictures)
                return true;
        }
        i++;
    }

    return false;
}

bool VaapiDecoderH265::dpb_add (PicturePtr& pic, H265SliceHdr *slice_hdr, H265NalUnit * nalu)
{

    H265PPS *const pps = slice_hdr->pps;
    H265SPS *const sps = pps->sps;
    VaapiDecPictureH265 *picture = pic.get();
    VaapiH265FrameStore::Ptr fs;

    PicturePtr tmp_pic;
    uint32_t i = 0;
    /* C.5.2.3 */
    while (i < m_dpb_count) {
        VaapiH265FrameStore::Ptr fs = m_dpb[i];
        tmp_pic = fs->buffer;
        if (tmp_pic->m_output_needed)
            tmp_pic->m_pic_latency_cnt += 1;
        i++;
    }

    if (picture->m_output_flag) {
        picture->m_output_needed = 1;
        picture->m_pic_latency_cnt = 0;
    } else{
        picture->m_output_needed = 0;
    }

    /* set pic as short_term_ref */
    vaapi_picture_h265_set_reference (picture,
             VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE);

    /* C.5.2.4 "Bumping" process */
    while ((dpb_get_num_need_output () >
        sps->max_num_reorder_pics[sps->max_sub_layers_minus1])
        || (sps->max_latency_increase_plus1[sps->max_sub_layers_minus1]
        && check_latency_cnt ()))
        dpb_bump (picture);

    /* Create new frame store */
    fs.reset(new VaapiH265FrameStore(pic));

    m_dpb[m_dpb_count++] = fs;
    return true;
}

/* C.5.2.2 */
bool VaapiDecoderH265::dpb_init (VaapiDecPictureH265 * picture, H265SliceHdr *slice_hdr,  H265NalUnit * nalu)
{
    H265PPS *const pps = slice_hdr->pps;
    H265SPS *const sps = pps->sps;

    if (nal_is_irap (nalu->type)
      && picture->m_NoRaslOutputFlag && !m_newBitstream) {
        if (nalu->type == H265_NAL_SLICE_CRA_NUT)
            picture->m_NoOutputOfPriorPicsFlag = 1;
        else
            picture->m_NoOutputOfPriorPicsFlag =
            slice_hdr->no_output_of_prior_pics_flag;

        if (picture->m_NoOutputOfPriorPicsFlag)
            dpb_clear (true);
        else {
            dpb_clear (false);
            while (dpb_bump (NULL));
        }
    } else {
        dpb_clear (false);
        while ((dpb_get_num_need_output () >
            sps->max_num_reorder_pics[sps->max_sub_layers_minus1])
            || (sps->max_latency_increase_plus1[sps->max_sub_layers_minus1]
            && check_latency_cnt ())
            || (m_dpb_count >=
            (sps->max_dec_pic_buffering_minus1[sps->max_sub_layers_minus1] +
            1))) {
            dpb_bump (picture);
        }
    }

    return true;
}

Decode_Status VaapiDecoderH265::dpb_reset (uint32_t dpb_size)
{
    if (dpb_size > m_dpb_size) {
        memset (&m_dpb[m_dpb_size], 0,
        (dpb_size - m_dpb_size) * sizeof (m_dpb[0]));
        m_dpb_size = dpb_size;
    }
    return true;
}

/* Detection of the first VCL NAL unit of a coded picture (7.4.2.4.5 ) */
bool VaapiDecoderH265::is_new_picture (H265SliceHdr *slice_hdr)
{
    if (slice_hdr->first_slice_segment_in_pic_flag)
        return true;

    return false;
}

bool VaapiDecoderH265::has_entry_in_rps (VaapiDecPictureH265 * dpb_pic,
    VaapiDecPictureH265 ** rps_list, uint32_t rps_list_length)
{
    uint32_t i;

    if (!dpb_pic || !rps_list || !rps_list_length)
        return false;

    for (i = 0; i < rps_list_length; i++) {
        if (rps_list[i] && rps_list[i]->m_poc == dpb_pic->m_poc)
            return true;
    }
    return false;
}

/* the derivation process for the RPS and the picture marking */
void VaapiDecoderH265::derive_and_mark_rps (VaapiDecPictureH265 * picture, int32_t * CurrDeltaPocMsbPresentFlag, int32_t * FollDeltaPocMsbPresentFlag)
{
    VaapiDecPictureH265 *dpb_pic = NULL;
    uint32_t i;

    memset (m_RefPicSetLtCurr, 0, sizeof (VaapiDecPictureH265 *) * 16);
    memset (m_RefPicSetLtFoll, 0, sizeof (VaapiDecPictureH265 *) * 16);
    memset (m_RefPicSetStCurrBefore, 0, sizeof (VaapiDecPictureH265 *) * 16);
    memset (m_RefPicSetStCurrAfter, 0, sizeof (VaapiDecPictureH265 *) * 16);
    memset (m_RefPicSetStFoll, 0, sizeof (VaapiDecPictureH265 *) * 16);

    /* (8-6) */
    for (i = 0; i < m_NumPocLtCurr; i++) {
        if (!CurrDeltaPocMsbPresentFlag[i]) {
            dpb_pic = dpb_get_picture (m_PocLtCurr[i], true);
            if (dpb_pic)
                m_RefPicSetLtCurr[i] = dpb_pic;
            else
                m_RefPicSetLtCurr[i] = NULL;
        } else {
            dpb_pic = dpb_get_picture (m_PocLtCurr[i], false);
            if (dpb_pic)
                m_RefPicSetLtCurr[i] = dpb_pic;
            else
                m_RefPicSetLtCurr[i] = NULL;
        }
    }
    for (; i < 16; i++)
        m_RefPicSetLtCurr[i] = NULL;

    for (i = 0; i < m_NumPocLtFoll; i++) {
    if (!FollDeltaPocMsbPresentFlag[i]) {
        dpb_pic = dpb_get_picture (m_PocLtFoll[i], true);
        if (dpb_pic)
            m_RefPicSetLtFoll[i] = dpb_pic;
        else
            m_RefPicSetLtFoll[i] = NULL;
        } else {
            dpb_pic = dpb_get_picture (m_PocLtFoll[i], false);
            if (dpb_pic)
                m_RefPicSetLtFoll[i] = dpb_pic;
            else
                m_RefPicSetLtFoll[i] = NULL;
        }
    }
    for (; i < 16; i++)
        m_RefPicSetLtFoll[i] = NULL;

    /* Mark all ref pics in RefPicSetLtCurr and RefPicSetLtFol as long_term_refs */
    for (i = 0; i < m_NumPocLtCurr; i++) {
        if (m_RefPicSetLtCurr[i])
            vaapi_picture_h265_set_reference(m_RefPicSetLtCurr[i],
                          VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE |
                          VAAPI_PICTURE_FLAG_RPS_LT_CURR);
    }
    for (i = 0; i < m_NumPocLtFoll; i++) {
        if (m_RefPicSetLtFoll[i])
            vaapi_picture_h265_set_reference(m_RefPicSetLtFoll[i],
                          VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE |
                          VAAPI_PICTURE_FLAG_RPS_LT_FOLL);
    }

    /* (8-7) */
    for (i = 0; i < m_NumPocStCurrBefore; i++) {
        dpb_pic = dpb_get_ref_picture (m_PocStCurrBefore[i], true);
        if (dpb_pic) {
            vaapi_picture_h265_set_reference (dpb_pic,
                          VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
                          VAAPI_PICTURE_FLAG_RPS_ST_CURR_BEFORE);
            m_RefPicSetStCurrBefore[i] = dpb_pic;
        } else
            m_RefPicSetStCurrBefore[i] = NULL;
    }
    for (; i < 16; i++)
        m_RefPicSetStCurrBefore[i] = NULL;

    for (i = 0; i < m_NumPocStCurrAfter; i++) {
        dpb_pic = dpb_get_ref_picture (m_PocStCurrAfter[i], true);
        if (dpb_pic) {
          vaapi_picture_h265_set_reference (dpb_pic,
                        VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
                        VAAPI_PICTURE_FLAG_RPS_ST_CURR_AFTER);
            m_RefPicSetStCurrAfter[i] = dpb_pic;
        } else
            m_RefPicSetStCurrAfter[i] = NULL;
    }
    for (; i < 16; i++)
        m_RefPicSetStCurrAfter[i] = NULL;

    for (i = 0; i < m_NumPocStFoll; i++) {
        dpb_pic = dpb_get_ref_picture (m_PocStFoll[i], true);
        if (dpb_pic) {
            vaapi_picture_h265_set_reference (dpb_pic,
                          VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
                          VAAPI_PICTURE_FLAG_RPS_ST_FOLL);
            m_RefPicSetStFoll[i] = dpb_pic;
        } else
            m_RefPicSetStFoll[i] = NULL;
    }
    for (; i < 16; i++)
        m_RefPicSetStFoll[i] = NULL;

    /* Mark all dpb pics not beloging to RefPicSet*[] as unused for ref */
    for (i = 0; i < m_dpb_count; i++) {
        dpb_pic = m_dpb[i]->buffer.get();
        if (dpb_pic &&
            !has_entry_in_rps (dpb_pic, m_RefPicSetLtCurr, m_NumPocLtCurr)
            && !has_entry_in_rps (dpb_pic, m_RefPicSetLtFoll,
            m_NumPocLtFoll)
            && !has_entry_in_rps (dpb_pic, m_RefPicSetStCurrAfter,
            m_NumPocStCurrAfter)
            && !has_entry_in_rps (dpb_pic, m_RefPicSetStCurrBefore,
            m_NumPocStCurrBefore)
            && !has_entry_in_rps (dpb_pic, m_RefPicSetStFoll,
            m_NumPocStFoll))
            vaapi_picture_h265_set_reference (dpb_pic, 0);
    }

}

/* Decoding process for reference picture set (8.3.2) */
bool VaapiDecoderH265::decode_ref_pic_set (PicturePtr& picture, H265SliceHdr *slice_hdr, H265NalUnit * nalu)
{
    uint32_t i, j, k;
    int32_t CurrDeltaPocMsbPresentFlag[16] = { 0, };
    int32_t FollDeltaPocMsbPresentFlag[16] = { 0, };
    H265PPS *const pps = slice_hdr->pps;
    H265SPS *const sps = pps->sps;

    const int32_t MaxPicOrderCntLsb =
    1 << (sps->log2_max_pic_order_cnt_lsb_minus4 + 4);

    /* if it is an irap pic, set all ref pics in dpb as unused for ref */
    if (nal_is_irap (nalu->type) && picture->m_NoRaslOutputFlag) {

        for (i = 0; i < m_dpb_count; i++) {
            VaapiH265FrameStore::Ptr fs = m_dpb[i];
            if (fs) {
                vaapi_picture_h265_set_reference (fs->buffer.get(), 0);
            }
        }
    }

    /* Reset everything for IDR */
    if (nal_is_idr (nalu->type)) {
        memset (m_PocStCurrBefore, 0, sizeof (uint32_t) * 16);
        memset (m_PocStCurrAfter, 0, sizeof (uint32_t) * 16);
        memset (m_PocStFoll, 0, sizeof (uint32_t) * 16);
        memset (m_PocLtCurr, 0, sizeof (uint32_t) * 16);
        memset (m_PocLtFoll, 0, sizeof (uint32_t) * 16);
        m_NumPocStCurrBefore = m_NumPocStCurrAfter = m_NumPocStFoll = 0;
        m_NumPocLtCurr = m_NumPocLtFoll = 0;
    } else {
        H265ShortTermRefPicSet *stRefPic = NULL;
        int32_t num_lt_pics, pocLt, PocLsbLt[16] = { 0, }
        , UsedByCurrPicLt[16] = {
        0,};
        int32_t DeltaPocMsbCycleLt[16] = { 0, };
        int32_t numtotalcurr = 0;

        /* this is based on CurrRpsIdx described in spec */
        if (!slice_hdr->short_term_ref_pic_set_sps_flag)
            stRefPic = &slice_hdr->short_term_ref_pic_sets;
        else if (sps->num_short_term_ref_pic_sets)
            stRefPic =
            &sps->short_term_ref_pic_set[slice_hdr->short_term_ref_pic_set_idx];

        for (i = 0, j = 0, k = 0; i < stRefPic->NumNegativePics; i++) {
            if (stRefPic->UsedByCurrPicS0[i]) {
                m_PocStCurrBefore[j++] = picture->m_poc + stRefPic->DeltaPocS0[i];
                numtotalcurr++;
            } else
                m_PocStFoll[k++] = picture->m_poc + stRefPic->DeltaPocS0[i];
        }
        m_NumPocStCurrBefore = j;
        for (i = 0, j = 0; i < stRefPic->NumPositivePics; i++) {
            if (stRefPic->UsedByCurrPicS1[i]) {
                m_PocStCurrAfter[j++] = picture->m_poc + stRefPic->DeltaPocS1[i];
                numtotalcurr++;
            } else
                m_PocStFoll[k++] = picture->m_poc + stRefPic->DeltaPocS1[i];
        }
        m_NumPocStCurrAfter = j;
        m_NumPocStFoll = k;
        num_lt_pics = slice_hdr->num_long_term_sps + slice_hdr->num_long_term_pics;
        /* The variables PocLsbLt[i] and UsedByCurrPicLt[i] are derived as follows: */
        for (i = 0; i < num_lt_pics; i++) {
            if (i < slice_hdr->num_long_term_sps) {
                PocLsbLt[i] = sps->lt_ref_pic_poc_lsb_sps[slice_hdr->lt_idx_sps[i]];
                UsedByCurrPicLt[i] =
                sps->used_by_curr_pic_lt_sps_flag[slice_hdr->lt_idx_sps[i]];
            } else {
                PocLsbLt[i] = slice_hdr->poc_lsb_lt[i];
                UsedByCurrPicLt[i] = slice_hdr->used_by_curr_pic_lt_flag[i];
            }
            if (UsedByCurrPicLt[i])
                numtotalcurr++;
        }

        m_NumPocTotalCurr = numtotalcurr;

        /* The variable DeltaPocMsbCycleLt[i] is derived as follows: (7-38) */
        for (i = 0; i < num_lt_pics; i++) {
            if (i == 0 || i == slice_hdr->num_long_term_sps)
                DeltaPocMsbCycleLt[i] = slice_hdr->delta_poc_msb_cycle_lt[i];
            else
                DeltaPocMsbCycleLt[i] =
                slice_hdr->delta_poc_msb_cycle_lt[i] + DeltaPocMsbCycleLt[i - 1];
        }

        /* (8-5) */
        for (i = 0, j = 0, k = 0; i < num_lt_pics; i++) {
            pocLt = PocLsbLt[i];
            if (slice_hdr->delta_poc_msb_present_flag[i])
                pocLt +=
                picture->m_poc - DeltaPocMsbCycleLt[i] * MaxPicOrderCntLsb -
                slice_hdr->pic_order_cnt_lsb;
            if (UsedByCurrPicLt[i]) {
                m_PocLtCurr[j] = pocLt;
                CurrDeltaPocMsbPresentFlag[j++] =
                slice_hdr->delta_poc_msb_present_flag[i];
            } else {
                m_PocLtFoll[k] = pocLt;
                FollDeltaPocMsbPresentFlag[k++] =
                slice_hdr->delta_poc_msb_present_flag[i];
            }
        }
        m_NumPocLtCurr = j;
        m_NumPocLtFoll = k;

    }

    /* the derivation process for the RPS and the picture marking */
    derive_and_mark_rps (picture.get(), CurrDeltaPocMsbPresentFlag, FollDeltaPocMsbPresentFlag);

    return true;
}

void VaapiDecoderH265::init_picture_refs(VaapiDecPictureH265 * picture, H265SliceHdr * slice_hdr)
{
    uint32_t NumRpsCurrTempList0 = 0, NumRpsCurrTempList1 = 0;
    VaapiDecPictureH265 *RefPicListTemp0[16] = { NULL, }, *RefPicListTemp1[16] = {
    NULL,};
    uint32_t i, rIdx = 0;
    uint32_t num_ref_idx_l0_active_minus1 = 0;
    uint32_t num_ref_idx_l1_active_minus1 = 0;
    H265RefPicListModification *ref_pic_list_modification;
    uint32_t type;

    memset (m_RefPicList0, 0, sizeof (VaapiDecPictureH265 *) * 16);
    memset (m_RefPicList1, 0, sizeof (VaapiDecPictureH265 *) * 16);
    m_RefPicList0_count = m_RefPicList1_count = 0;

    if ((slice_hdr->type != H265_B_SLICE) &&
        (slice_hdr->type != H265_P_SLICE))
        return;

    if (slice_hdr->dependent_slice_segment_flag) {
        H265SliceHdr *tmp = &m_prev_independent_slice;
        num_ref_idx_l0_active_minus1 = tmp->num_ref_idx_l0_active_minus1;
        num_ref_idx_l1_active_minus1 = tmp->num_ref_idx_l1_active_minus1;
        ref_pic_list_modification = &tmp->ref_pic_list_modification;
        type = tmp->type;
    } else {
        num_ref_idx_l0_active_minus1 = slice_hdr->num_ref_idx_l0_active_minus1;
        num_ref_idx_l1_active_minus1 = slice_hdr->num_ref_idx_l1_active_minus1;
        ref_pic_list_modification = &slice_hdr->ref_pic_list_modification;
        type = slice_hdr->type;
    }

    NumRpsCurrTempList0 =
    MAX ((num_ref_idx_l0_active_minus1 + 1), m_NumPocTotalCurr);
    NumRpsCurrTempList1 =
    MAX ((num_ref_idx_l1_active_minus1 + 1), m_NumPocTotalCurr);

    /* (8-8) */
    while (rIdx < NumRpsCurrTempList0) {
        for (i = 0; i < m_NumPocStCurrBefore && rIdx < NumRpsCurrTempList0;
              rIdx++, i++)
            RefPicListTemp0[rIdx] = m_RefPicSetStCurrBefore[i];
        for (i = 0; i < m_NumPocStCurrAfter && rIdx < NumRpsCurrTempList0;
              rIdx++, i++)
            RefPicListTemp0[rIdx] = m_RefPicSetStCurrAfter[i];
        for (i = 0; i < m_NumPocLtCurr && rIdx < NumRpsCurrTempList0;
              rIdx++, i++)
            RefPicListTemp0[rIdx] = m_RefPicSetLtCurr[i];
    }

    /* construct RefPicList0 (8-9) */
    for (rIdx = 0; rIdx <= num_ref_idx_l0_active_minus1; rIdx++)
        m_RefPicList0[rIdx] =
        ref_pic_list_modification->ref_pic_list_modification_flag_l0 ?
        RefPicListTemp0[ref_pic_list_modification->list_entry_l0[rIdx]] :
        RefPicListTemp0[rIdx];
    m_RefPicList0_count = rIdx;

    if (type == H265_B_SLICE) {
    rIdx = 0;

    /* (8-10) */
    while (rIdx < NumRpsCurrTempList1) {
        for (i = 0; i < m_NumPocStCurrAfter && rIdx < NumRpsCurrTempList1;
              rIdx++, i++)
            RefPicListTemp1[rIdx] = m_RefPicSetStCurrAfter[i];
        for (i = 0; i < m_NumPocStCurrBefore && rIdx < NumRpsCurrTempList1;
              rIdx++, i++)
            RefPicListTemp1[rIdx] = m_RefPicSetStCurrBefore[i];
        for (i = 0; i < m_NumPocLtCurr && rIdx < NumRpsCurrTempList1;
              rIdx++, i++)
            RefPicListTemp1[rIdx] = m_RefPicSetLtCurr[i];
        }

        /* construct RefPicList1 (8-10) */
        for (rIdx = 0; rIdx <= num_ref_idx_l1_active_minus1; rIdx++)
            m_RefPicList1[rIdx] =
            ref_pic_list_modification->ref_pic_list_modification_flag_l1 ?
            RefPicListTemp1[ref_pic_list_modification->list_entry_l1
            [rIdx]] : RefPicListTemp1[rIdx];
        m_RefPicList1_count = rIdx;
    }
}

static int32_t
clip3 (int32_t x, int32_t y, int32_t z)
{
    if (z < x)
        return x;
    if (z > y)
        return y;
    return z;
}

bool VaapiDecoderH265::fill_pred_weight_table (VASliceParameterBufferHEVC * slice_param, H265SliceHdr * slice_hdr)
{
    H265PPS *const pps = slice_hdr->pps;
    H265SPS *const sps = pps->sps;

    H265PredWeightTable *const w = &slice_hdr->pred_weight_table;
    int32_t chroma_weight, chroma_log2_weight_denom;
    int32_t i, j;

    slice_param->luma_log2_weight_denom = 0;
    slice_param->delta_chroma_log2_weight_denom = 0;

    if ((pps->weighted_pred_flag && H265_IS_P_SLICE (slice_hdr)) ||
        (pps->weighted_bipred_flag && H265_IS_B_SLICE (slice_hdr))) {

        slice_param->luma_log2_weight_denom = w->luma_log2_weight_denom;
        if (!sps->chroma_format_idc)
            slice_param->delta_chroma_log2_weight_denom =
            w->delta_chroma_log2_weight_denom;

        chroma_log2_weight_denom =
        slice_param->luma_log2_weight_denom +
        slice_param->delta_chroma_log2_weight_denom;

        for (i = 0; i <= slice_param->num_ref_idx_l0_active_minus1; i++) {
            if (slice_hdr->pred_weight_table.luma_weight_l0_flag[i]) {
                slice_param->delta_luma_weight_l0[i] = w->delta_luma_weight_l0[i];
                slice_param->luma_offset_l0[i] = w->luma_offset_l0[i];
            }
            if (slice_hdr->pred_weight_table.chroma_weight_l0_flag[i]) {
                for (j = 0; j < 2; j++) {
                    slice_param->delta_chroma_weight_l0[i][j] =
                    w->delta_chroma_weight_l0[i][j];
                    /* Find  ChromaWeightL0 */
                    chroma_weight =
                    (1 << chroma_log2_weight_denom) + w->delta_chroma_weight_l0[i][j];
                    /* 7-44 */
                    slice_param->ChromaOffsetL0[i][j] = clip3 (-128, 127,
                    (w->delta_chroma_offset_l0[i][j] -
                    ((128 * chroma_weight) >> chroma_log2_weight_denom)));
                }
            }
        }

        if (H265_IS_B_SLICE (slice_hdr)) {
            for (i = 0; i <= slice_param->num_ref_idx_l1_active_minus1; i++) {
                if (slice_hdr->pred_weight_table.luma_weight_l1_flag[i]) {
                    slice_param->delta_luma_weight_l1[i] = w->delta_luma_weight_l1[i];
                    slice_param->luma_offset_l1[i] = w->luma_offset_l1[i];
                }
                if (slice_hdr->pred_weight_table.chroma_weight_l1_flag[i]) {
                    for (j = 0; j < 2; j++) {
                        slice_param->delta_chroma_weight_l1[i][j] =
                        w->delta_chroma_weight_l1[i][j];
                        /* Find  ChromaWeightL1 */
                        chroma_weight =
                        (1 << chroma_log2_weight_denom) +
                        w->delta_chroma_weight_l1[i][j];
                        slice_param->ChromaOffsetL1[i][j] =
                        clip3 (-128, 127,
                        (w->delta_chroma_offset_l1[i][j] -
                        ((128 * chroma_weight) >> chroma_log2_weight_denom)));
                    }
                }
            }
        }
    }
    /* Fixme: Optimize */
    else {
        for (i = 0; i < 15; i++) {
            slice_param->delta_luma_weight_l0[i] = 0;
            slice_param->luma_offset_l0[i] = 0;
            slice_param->delta_luma_weight_l1[i] = 0;
            slice_param->luma_offset_l1[i] = 0;
        }
        for (i = 0; i < 15; i++) {
            for (j = 0; j < 2; j++) {
                slice_param->delta_chroma_weight_l0[i][j] = 0;
                slice_param->ChromaOffsetL0[i][j] = 0;
                slice_param->delta_chroma_weight_l1[i][j] = 0;
                slice_param->ChromaOffsetL1[i][j] = 0;
            }
        }
    }
    return true;
}

bool VaapiDecoderH265::fill_RefPicList (VaapiDecPictureH265 * picture, VASliceParameterBufferHEVC* slice_param, H265SliceHdr * slice_hdr)
{
    VAPictureParameterBufferHEVC *pic_param = (VAPictureParameterBufferHEVC *)(picture->m_param);
    uint32_t i, num_ref_lists = 0, j;

    slice_param->num_ref_idx_l0_active_minus1 = 0;
    slice_param->num_ref_idx_l1_active_minus1 = 0;
    for (j = 0; j < 2; j++)
        for (i = 0; i < 15; i++)
            slice_param->RefPicList[j][i] = 0xFF;

    if (H265_IS_B_SLICE (slice_hdr))
        num_ref_lists = 2;
    else if (H265_IS_I_SLICE (slice_hdr))
        num_ref_lists = 0;
    else
        num_ref_lists = 1;

    if (num_ref_lists < 1)
        return true;

    slice_param->num_ref_idx_l0_active_minus1 =
                     slice_hdr->num_ref_idx_l0_active_minus1;
    slice_param->num_ref_idx_l1_active_minus1 =
                     slice_hdr->num_ref_idx_l1_active_minus1;

    for (i = 0; i < m_RefPicList0_count; i++)
        slice_param->RefPicList[0][i] =
            get_index_for_RefPicListX(pic_param->ReferenceFrames,
            m_RefPicList0[i]);
    for (; i < 15; i++)
        slice_param->RefPicList[0][i] = 0xFF;

    if (num_ref_lists < 2)
        return true;

    for (i = 0; i < m_RefPicList1_count; i++)
        slice_param->RefPicList[1][i] =
            get_index_for_RefPicListX(pic_param->ReferenceFrames,
            m_RefPicList1[i]);
    for (; i < 15; i++)
        slice_param->RefPicList[1][i] = 0xFF;

    return true;
}

bool VaapiDecoderH265::init_picture (PicturePtr& picture,  H265SliceHdr *slice_hdr, H265NalUnit * nalu)
{
    if (nal_is_idr (nalu->type)) {
        VAAPI_PICTURE_FLAG_SET (picture, VAAPI_PICTURE_FLAG_IDR);
    }

    if (nalu->type >= H265_NAL_SLICE_BLA_W_LP &&
        nalu->type <= H265_NAL_SLICE_CRA_NUT)
        picture->m_RapPicFlag = true;

    picture->m_structure = VAAPI_PICTURE_STRUCTURE_FRAME;//base_picture->structure;

    /*NoRaslOutputFlag ==1 if the current picture is
    1) an IDR picture
    2) a BLA picture
    3) a CRA picture that is the first access unit in the bitstream
    4) first picture that follows an end of sequence NAL unit in decoding order
    5) has HandleCraAsBlaFlag == 1 (set by external means, so not considering )
    */
    if (nal_is_idr (nalu->type) || nal_is_bla (nalu->type) ||
       (nal_is_cra (nalu->type) && m_newBitstream)
        || m_prev_nal_is_eos) {
        picture->m_NoRaslOutputFlag = 1;
    }

    if (nal_is_irap (nalu->type)) {
        picture->m_IntraPicFlag = true;
        m_associated_irap_NoRaslOutputFlag = picture->m_NoRaslOutputFlag;
    }

    if (nal_is_rasl (nalu->type) && m_associated_irap_NoRaslOutputFlag) {
        picture->m_output_flag = false;
    }
    else{
        picture->m_output_flag = slice_hdr->pic_output_flag;
    }

    init_picture_poc (picture, slice_hdr, nalu);
    return true;
}

Decode_Status VaapiDecoderH265::decodeSlice(H265NalUnit * nalu)
{
    PicturePtr picture;
    Decode_Status status;
    H265ParserResult result;
    H265SliceHdr *sliceHdr = &m_slice_hdr;

    /* parser the slice header info */
    memset(sliceHdr, 0, sizeof(H265SliceHdr));

    result = h265_parser_parse_slice_hdr(&m_parser, nalu,&m_slice_hdr);
    if (result != H265_PARSER_OK) {
        return DECODE_FAIL;
    }

    if (!m_slice_hdr.dependent_slice_segment_flag) {
        memcpy(&m_prev_independent_slice, &m_slice_hdr, sizeof(m_prev_independent_slice));
    }
    H265PPS *const pps = sliceHdr->pps;
    H265SPS *const sps = pps->sps;
    ensure_dpb(sps);
    if (is_new_picture (sliceHdr)) {
        if (m_currentPicture) {
            m_currentPicture->decode();
            dpb_add (m_currentPicture,  sliceHdr, nalu);
            m_currentPicture.reset();
        }

        SurfacePtr s = createSurface();
        if (!s)
            return DECODE_FAIL;
        picture.reset(new VaapiDecPictureH265(m_context, s, 0));
        /* test code */

        VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_FF);
        status = ensureQuantMatrix(picture);
        if (status != DECODE_SUCCESS) {
            ERROR("failed to reset quantizer matrix");
            return status;
        }

        if (!init_picture(picture, sliceHdr, nalu))
            return DECODE_FAIL;

        /* Drop all RASL pictures having NoRaslOutputFlag is TRUE for the
        * associated IRAP picture */
        if (nal_is_rasl (nalu->type) && m_associated_irap_NoRaslOutputFlag) {
            m_currentPicture.reset();
            return DECODE_SUCCESS;
        }

        if (!decode_ref_pic_set (picture, sliceHdr, nalu))
            return DECODE_FAIL;

        if (!dpb_init (picture.get(), sliceHdr, nalu))
            return DECODE_FAIL;

        if (!fillPicture(picture, nalu)) {
            return DECODE_FAIL;
        }
        m_newBitstream = false;
        m_currentPicture = picture;
    }

    if (m_flags & VAAPI_DECODER_UNIT_FLAG_AU_END)
        VAAPI_PICTURE_FLAG_SET (m_currentPicture, VAAPI_PICTURE_FLAG_AU_END);

    VASliceParameterBufferHEVC* sliceParam = 0;
    if (!m_currentPicture->newSlice(sliceParam, nalu->data+nalu->offset, nalu->size))
        return false;

    init_picture_refs (m_currentPicture.get(), sliceHdr);
    if (!fillSlice(m_currentPicture.get(), sliceParam, sliceHdr, nalu)) {
        return DECODE_FAIL;
    }

    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH265::decodeSPS(H265NalUnit * nalu)
{
    H265SPS *const sps = &m_lastSPS;
    H265ParserResult result;
    memset(sps, 0, sizeof(*sps));
    m_gotSPS = true;
    result = h265_parser_parse_sps(&m_parser, nalu, sps, true);
    if (result != H265_PARSER_OK) {
        m_gotSPS = false;
    }
#if 0
      if (sps->max_latency_increase_plus1[sps->max_sub_layers_minus1])
    m_SpsMaxLatencyPictures =
        sps->max_num_reorder_pics[sps->max_sub_layers_minus1] +
        sps->max_latency_increase_plus1[sps->max_sub_layers_minus1] - 1;
#endif
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH265::decodePPS(H265NalUnit * nalu)
{
    H265PPS *const pps = &m_lastPPS;
    H265ParserResult result;

    memset(pps, 0, sizeof(*pps));
    result = h265_parser_parse_pps(&m_parser, nalu, pps);
    if (result != H265_PARSER_OK) {
        m_gotPPS = false;
    }

    m_gotPPS = true;
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderH265::decodeSEI(H265NalUnit * nalu)
{
    return DECODE_SUCCESS;
}

const bool VaapiDecoderH265::s_registered =
    VaapiDecoderFactory::register_<VaapiDecoderH265>(YAMI_MIME_H265);

}

