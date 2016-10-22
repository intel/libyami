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

#include <string.h>

#include "common/log.h"
#include "vaapidecoder_vc1.h"

namespace YamiMediaCodec {
using namespace ::YamiParser::VC1;
VaapiDecoderVC1::VaapiDecoderVC1()
{
    m_dpbIdx = 0;
}

VaapiDecoderVC1::~VaapiDecoderVC1()
{
    stop();
}

YamiStatus VaapiDecoderVC1::start(VideoConfigBuffer* buffer)
{
    if (!buffer || !buffer->data || !buffer->size)
        return YAMI_INVALID_PARAM;
    uint32_t width = buffer->width;
    uint32_t height = buffer->height;
    m_parser.m_seqHdr.coded_width = width;
    m_parser.m_seqHdr.coded_height = height;
    if (!m_parser.parseCodecData(buffer->data, buffer->size))
        return YAMI_FAIL;
    setFormat(width, height, width, height, VC1_MAX_REFRENCE_SURFACE_NUMBER + 1);
    return YAMI_SUCCESS;
}

void VaapiDecoderVC1::stop(void)
{
    flush();
    VaapiDecoderBase::stop();
}

void VaapiDecoderVC1::flush(void)
{
    bumpAll();
    VaapiDecoderBase::flush();
}

YamiStatus VaapiDecoderVC1::ensureContext()
{
    return ensureProfile(VAProfileVC1Main);
}

bool VaapiDecoderVC1::makeBitPlanes(PicturePtr& picture, VAPictureParameterBufferVC1* param)
{
    uint8_t val = 0;
    uint32_t i = 0, j = 0, k = 0, t = 0, dstIdx = 0, srcIdx = 0;
    uint8_t* bitPlanes[3] = { NULL, NULL, NULL };
    uint8_t* bitPlanesPayLoad = NULL;
    if ((m_parser.m_frameHdr.picture_type == FRAME_I)
        || (m_parser.m_frameHdr.picture_type == FRAME_BI)) {
        if (param->bitplane_present.flags.bp_ac_pred)
            bitPlanes[1] = &m_parser.m_bitPlanes.acpred[0];
        if (param->bitplane_present.flags.bp_overflags)
            bitPlanes[2] = &m_parser.m_bitPlanes.overflags[0];
    }
    else if (m_parser.m_frameHdr.picture_type == FRAME_P) {
        if (param->bitplane_present.flags.bp_direct_mb)
            bitPlanes[0] = &m_parser.m_bitPlanes.directmb[0];
        if (param->bitplane_present.flags.bp_skip_mb)
            bitPlanes[1] = &m_parser.m_bitPlanes.skipmb[0];
        if (param->bitplane_present.flags.bp_mv_type_mb)
            bitPlanes[2] = &m_parser.m_bitPlanes.mvtypemb[0];
    }
    else if (m_parser.m_frameHdr.picture_type == FRAME_B) {
        if (param->bitplane_present.flags.bp_direct_mb)
            bitPlanes[0] = &m_parser.m_bitPlanes.directmb[0];
        if (param->bitplane_present.flags.bp_skip_mb)
            bitPlanes[1] = &m_parser.m_bitPlanes.skipmb[0];
    }
    picture->editBitPlane(bitPlanesPayLoad, (m_parser.m_mbWidth * m_parser.m_mbHeight + 1) >> 1);
    if (!bitPlanesPayLoad)
        return false;
    for (i = 0; i < m_parser.m_mbHeight; i++) {
        for (j = 0; j < m_parser.m_mbWidth; j++) {
            dstIdx = t++ >> 1;
            srcIdx = i * m_parser.m_mbWidth + j;
            val = 0;
            for (k = 0; k < 3; k++) {
                if (bitPlanes[k])
                    val |= bitPlanes[k][srcIdx] << k;
            }
            bitPlanesPayLoad[dstIdx] = (bitPlanesPayLoad[dstIdx] << 4) | val;
        }
    }
    if (t & 1)
        bitPlanesPayLoad[(t >> 1)] <<= 4;
    return true;
}

bool VaapiDecoderVC1::ensurePicture(PicturePtr& picture)
{
    VAPictureParameterBufferVC1* param;
    SeqHdr* seqHdr = &m_parser.m_seqHdr;
    EntryPointHdr* entryPointHdr = &m_parser.m_entryPointHdr;
    FrameHdr* frameHdr = &m_parser.m_frameHdr;
    if (!picture->editPicture(param))
        return false;

    param->forward_reference_picture = VA_INVALID_ID;
    param->backward_reference_picture = VA_INVALID_ID;
    param->inloop_decoded_picture = VA_INVALID_ID;
    param->coded_width = m_parser.m_seqHdr.coded_width;
    param->coded_height = m_parser.m_seqHdr.coded_height;

#define FILL(h, f) param->f = h->f
#define FILL_MV(h, f) param->mv_fields.bits.f = h->f
#define FILL_RAWCODING(h, f) param->raw_coding.flags.f = h->f
#define FILL_SEQUENCE(h, f) param->sequence_fields.bits.f = h->f
#define FILL_REFERENCE(h, f) param->reference_fields.bits.f = h->f
#define FILL_TRANSFORM(h, f) param->transform_fields.bits.f = h->f
#define FILL_ENTRYPOINT(h, f) param->entrypoint_fields.bits.f = h->f
#define FILL_PICTUREFIELDS(h, f) param->picture_fields.bits.f = h->f
#define FILL_PICQUANTIZER(h, f) param->pic_quantizer_fields.bits.f = h->f
    FILL_PICTUREFIELDS(frameHdr, picture_type);
    FILL_PICQUANTIZER(frameHdr, dq_frame);
    FILL_PICQUANTIZER(frameHdr, dq_profile);
    FILL_PICQUANTIZER(frameHdr, dq_binary_level);
    FILL_PICQUANTIZER(frameHdr, alt_pic_quantizer);
    param->pic_quantizer_fields.bits.half_qp = frameHdr->halfqp;
    param->pic_quantizer_fields.bits.pic_quantizer_scale = frameHdr->pquant;
    param->pic_quantizer_fields.bits.pic_quantizer_type = frameHdr->pquantizer;
    if (frameHdr->dq_profile == DQPROFILE_SINGLE_EDGE)
        FILL_PICQUANTIZER(frameHdr, dq_sb_edge);

    if (frameHdr->dq_profile == DQPROFILE_DOUBLE_EDGE)
        FILL_PICQUANTIZER(frameHdr, dq_db_edge);

    FILL_TRANSFORM(frameHdr, intra_transform_dc_table);
    FILL_TRANSFORM(frameHdr, mb_level_transform_type_flag);
    FILL_TRANSFORM(frameHdr, frame_level_transform_type);
    param->transform_fields.bits.transform_ac_codingset_idx1 = frameHdr->transacfrm;
    param->transform_fields.bits.transform_ac_codingset_idx2 = frameHdr->transacfrm2;
    FILL_SEQUENCE(seqHdr, profile);

    FILL_RAWCODING(frameHdr, mv_type_mb);
    FILL_RAWCODING(frameHdr, direct_mb);
    FILL_RAWCODING(frameHdr, skip_mb);

    FILL_MV(frameHdr, mv_table);
    FILL_MV(frameHdr, extended_mv_range);
    if (frameHdr->picture_type == FRAME_P
        || frameHdr->picture_type == FRAME_B)
        FILL_MV(frameHdr, mv_mode);

    if (frameHdr->picture_type == FRAME_P
        && frameHdr->mv_mode == MVMODE_INTENSITY_COMPENSATION)
        FILL_MV(frameHdr, mv_mode2);

    FILL(frameHdr, cbp_table);
    param->luma_scale = frameHdr->lumscale;
    param->luma_shift = frameHdr->lumshift;
    param->b_picture_fraction = frameHdr->bfraction;
    if ((!(frameHdr->mv_type_mb))
        && (frameHdr->picture_type == FRAME_P
               && (frameHdr->mv_mode == MVMODE_MIXED_MV
                      || (frameHdr->mv_mode == MVMODE_INTENSITY_COMPENSATION
                             && frameHdr->mv_mode2 == MVMODE_MIXED_MV))))
        param->bitplane_present.flags.bp_mv_type_mb = 1;

    if ((!(frameHdr->direct_mb))
        && (frameHdr->picture_type == FRAME_B))
        param->bitplane_present.flags.bp_direct_mb = 1;

    if ((!(frameHdr->skip_mb))
        && (frameHdr->picture_type == FRAME_P
               || frameHdr->picture_type == FRAME_B))
        param->bitplane_present.flags.bp_skip_mb = 1;

    if (seqHdr->profile == PROFILE_ADVANCED) {
        FILL_SEQUENCE(seqHdr, pulldown);
        FILL_SEQUENCE(seqHdr, interlace);
        FILL_SEQUENCE(seqHdr, tfcntrflag);
        FILL_SEQUENCE(seqHdr, finterpflag);
        FILL_SEQUENCE(seqHdr, psf);
        FILL_SEQUENCE(entryPointHdr, overlap);

        FILL_ENTRYPOINT(entryPointHdr, broken_link);
        FILL_ENTRYPOINT(entryPointHdr, closed_entry);
        FILL_ENTRYPOINT(entryPointHdr, panscan_flag);
        FILL_ENTRYPOINT(entryPointHdr, loopfilter);

        FILL(frameHdr, rounding_control);
        FILL(frameHdr, post_processing);
        param->fast_uvmc_flag = entryPointHdr->fastuvmc;
        param->conditional_overlap_flag = frameHdr->condover;

        param->picture_fields.bits.top_field_first = frameHdr->tff;
        param->picture_fields.bits.frame_coding_mode = frameHdr->fcm;
        param->picture_fields.bits.is_first_field = frameHdr->fcm == 0;

        FILL_RAWCODING(frameHdr, ac_pred);
        FILL_RAWCODING(frameHdr, overflags);
        FILL_REFERENCE(entryPointHdr, reference_distance_flag);

        param->mv_fields.bits.extended_mv_flag = entryPointHdr->extended_mv;
        FILL_MV(entryPointHdr, extended_dmv_flag);

        FILL_PICQUANTIZER(entryPointHdr, dquant);
        FILL_PICQUANTIZER(entryPointHdr, quantizer);
        FILL_TRANSFORM(entryPointHdr, variable_sized_transform_flag);
        param->range_mapping_fields.bits.luma_flag = entryPointHdr->range_mapy_flag;
        param->range_mapping_fields.bits.luma = entryPointHdr->range_mapy;
        param->range_mapping_fields.bits.chroma_flag = entryPointHdr->range_mapuv_flag;
        param->range_mapping_fields.bits.chroma = entryPointHdr->range_mapuv;

        if (frameHdr->mv_mode == MVMODE_INTENSITY_COMPENSATION)
            param->picture_fields.bits.intensity_compensation = 1;
        if ((!(frameHdr->ac_pred))
            && (frameHdr->picture_type == FRAME_I
                   || frameHdr->picture_type == FRAME_BI))
            param->bitplane_present.flags.bp_ac_pred = 1;

        if ((!(frameHdr->overflags))
            && ((frameHdr->picture_type == FRAME_I
                    || frameHdr->picture_type == FRAME_BI)
                   && (entryPointHdr->overlap
                          && frameHdr->pquant <= 8)
                   && frameHdr->condover == 2))
            param->bitplane_present.flags.bp_overflags = 1;
    }
    else {
        FILL_SEQUENCE(seqHdr, finterpflag);
        FILL_SEQUENCE(seqHdr, multires);
        FILL_SEQUENCE(seqHdr, overlap);
        FILL_SEQUENCE(seqHdr, syncmarker);
        FILL_SEQUENCE(seqHdr, rangered);
        FILL_SEQUENCE(seqHdr, max_b_frames);
        FILL(frameHdr, range_reduction_frame);
        FILL(frameHdr, picture_resolution_index);
        param->fast_uvmc_flag = seqHdr->fastuvmc;
        param->mv_fields.bits.extended_mv_flag = seqHdr->extended_mv;
        FILL_TRANSFORM(seqHdr, variable_sized_transform_flag);

        /* 8.3.7 Rounding control */
        if (frameHdr->picture_type == FRAME_I
            || frameHdr->picture_type == FRAME_BI) {
            param->rounding_control = 1;
        }
        else if (frameHdr->picture_type == FRAME_P) {
            param->rounding_control ^= 1;
        }
    }

    if (frameHdr->picture_type == FRAME_P
        || frameHdr->picture_type == FRAME_SKIPPED) {
        param->forward_reference_picture = m_dpb[m_dpbIdx-1]->getSurfaceID();
    }
    else if (frameHdr->picture_type == FRAME_B) {
        param->forward_reference_picture = m_dpb[0]->getSurfaceID();
        param->backward_reference_picture = m_dpb[1]->getSurfaceID();
    }

    if (param->bitplane_present.value)
        return makeBitPlanes(picture, param);

#undef FILL
#undef FILL_MV
#undef FILL_RAWCODING
#undef FILL_SEQUENCE
#undef FILL_REFERENCE
#undef FILL_TRANSFORM
#undef FILL_ENTRYPOINT
#undef FILL_PICTUREFIELDS
#undef FILL_PICQUANTIZER

    return true;
}

bool VaapiDecoderVC1::ensureSlice(PicturePtr& picture, void* data, int size)
{
    VASliceParameterBufferVC1* slice = NULL;
    if (!picture->newSlice(slice, data, size))
        return false;

    slice->macroblock_offset = m_parser.m_frameHdr.macroblock_offset;

    if (m_sliceFlag) {
        slice->macroblock_offset = m_parser.m_sliceHdr.macroblock_offset;
        slice->slice_vertical_position = m_parser.m_sliceHdr.slice_addr;
    }
    return true;
}

YamiStatus VaapiDecoderVC1::outputPicture(const PicturePtr& picture)
{
    YamiStatus ret = YAMI_SUCCESS;
    if (!picture->m_picOutputFlag) {
        ret = VaapiDecoderBase::outputPicture(picture);;
        picture->m_picOutputFlag = true;
    }
    return ret;
}

void VaapiDecoderVC1:: bumpAll()
{
    for(int32_t i = 0; i < m_dpbIdx; i++)
        outputPicture(m_dpb[i]);
    m_dpbIdx = 0;
    m_dpb[0].reset();
    m_dpb[1].reset();
}

YamiStatus VaapiDecoderVC1::decode(uint8_t* data, uint32_t size, uint64_t pts)
{
    YamiStatus ret;
    SurfacePtr surface;
    PicturePtr picture;
    int32_t offset, len;
    bool isReference = true;
    SeqHdr* seqHdr = &m_parser.m_seqHdr;
    m_sliceFlag = false;
    ret = ensureContext();
    if (ret != YAMI_SUCCESS)
        return ret;
    surface = VaapiDecoderBase::createSurface();
    if (!surface) {
        ret = YAMI_DECODE_NO_SURFACE;
    } else {
        picture.reset(
            new VaapiDecPictureVC1(m_context, surface, pts));
    }
    if (!ensurePicture(picture))
        return YAMI_FAIL;
    if (seqHdr->profile == PROFILE_ADVANCED) {
        while(1) {
            offset = m_parser.searchStartCode(data, size);
            len = (offset < 0) ? size : offset;
            if (m_sliceFlag)
                m_parser.parseSliceHeader(data, len);

            if (!ensureSlice(picture, data, len))
                return YAMI_FAIL;

            if (offset < 0)
                break;
            if (data[offset+3] == 0xB)
                m_sliceFlag = true;
            else
                m_sliceFlag = false;
            data += (offset + 4);
            size -= (offset + 4);
        }
    } else {
        if (!ensureSlice(picture, data, size))
            return YAMI_FAIL;
    }
    if (!picture->decode()) {
        return YAMI_FAIL;
    }

    if (m_parser.m_frameHdr.picture_type == FRAME_B)
        isReference = false;
    if (m_dpbIdx == 2) {
        if (!isReference) {
            outputPicture(m_dpb[0]);
            outputPicture(picture);
        } else {
            outputPicture(m_dpb[0]);
            m_dpb[0] = m_dpb[1];
            m_dpb[1] = picture;
        }
    } else {
         if (isReference) {
            m_dpb[m_dpbIdx] = picture;
            m_dpbIdx++;
         }
    }
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderVC1::decode(VideoDecodeBuffer* buffer)
{
    uint8_t* data;
    uint32_t size;
    FrameHdr* frameHdr = &m_parser.m_frameHdr;
    if (!buffer || !(buffer->data) || !(buffer->size)) {
        bumpAll();
        return YAMI_SUCCESS;
    }
    size = buffer->size;
    data = buffer->data;
    if (!m_parser.parseFrameHeader(data, size))
        return YAMI_DECODE_INVALID_DATA;
    if (((frameHdr->picture_type == FRAME_P
        || frameHdr->picture_type == FRAME_SKIPPED)
        && (m_dpbIdx < 1))
        || ((frameHdr->picture_type == FRAME_B)
        && (m_dpbIdx < 2))) {
        return YAMI_FAIL;
    }
    return decode(data, size, buffer->timeStamp);
}

}
