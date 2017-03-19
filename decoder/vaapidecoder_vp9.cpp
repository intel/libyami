/*
 * Copyright (C) 2013-2016 Intel Corporation. All rights reserved.
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
#include "vaapidecoder_vp9.h"

namespace YamiMediaCodec{
#define VP9_SURFACE_NUM 8

typedef VaapiDecoderVP9::PicturePtr PicturePtr;

static VAProfile profileMap(VP9_PROFILE src_profile)
{
    switch (src_profile) {
    case VP9_PROFILE_0:
        return VAProfileVP9Profile0;
    case VP9_PROFILE_1:
        return VAProfileVP9Profile1;
    case VP9_PROFILE_2:
        return VAProfileVP9Profile2;
    case VP9_PROFILE_3:
        return VAProfileVP9Profile3;
    case MAX_VP9_PROFILES:
    default:
        return VAProfileNone;
    }
}

VaapiDecoderVP9::VaapiDecoderVP9()
{
    m_parser.reset(vp9_parser_new(), vp9_parser_free);
    m_reference.resize(VP9_REF_FRAMES);
}

VaapiDecoderVP9::~VaapiDecoderVP9()
{
    stop();
}

YamiStatus VaapiDecoderVP9::start(VideoConfigBuffer* buffer)
{
    DEBUG("VP9: start() buffer size: %d x %d", buffer->width,
          buffer->height);
    if (!(buffer->flag & HAS_VA_PROFILE))
        buffer->profile = VAProfileVP9Profile0;
    //VP9_SURFACE_NUM reference frame
    if (!(buffer->flag & HAS_SURFACE_NUMBER))
        buffer->surfaceNumber = VP9_SURFACE_NUM;

    m_parser->bit_depth = VP9_BITS_8;

    DEBUG("disable native graphics buffer");
    m_configBuffer = *buffer;
    m_configBuffer.data = NULL;
    m_configBuffer.size = 0;

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderVP9::reset(VideoConfigBuffer* buffer)
{
    DEBUG("VP9: reset()");
    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderVP9::stop(void)
{
    DEBUG("VP9: stop()");
    flush();
    VaapiDecoderBase::stop();
}

void VaapiDecoderVP9::flush(void)
{
    flush(true);
}

void VaapiDecoderVP9::flush(bool discardOutput)
{
    m_parser.reset(vp9_parser_new(), vp9_parser_free);
    m_reference.clear();
    m_reference.resize(VP9_REF_FRAMES);
    if (discardOutput)
        VaapiDecoderBase::flush();
}

YamiStatus VaapiDecoderVP9::ensureContext(const Vp9FrameHdr* hdr)
{
    VAProfile vp9_profile = profileMap(hdr->profile);
    if (vp9_profile == VAProfileNone) {
        return YAMI_FATAL_ERROR;
    }

    uint32_t fourcc = (m_parser->bit_depth == VP9_BITS_10) ? YAMI_FOURCC_P010 : YAMI_FOURCC_NV12;

    if (setFormat(hdr->width, hdr->height, ALIGN8(hdr->width), ALIGN32(hdr->height), VP9_SURFACE_NUM, fourcc)) {
        return YAMI_DECODE_FORMAT_CHANGE;
    }

    return ensureProfile(vp9_profile);
}

bool VaapiDecoderVP9::fillReference(VADecPictureParameterBufferVP9* param, const Vp9FrameHdr* hdr)
{
#define FILL_REFERENCE(vafield, ref) \
    do { \
        int idx = ref - VP9_LAST_FRAME; \
        if (!m_reference[idx]) { \
            ERROR("reference to %d is invalid", idx); \
            return false; \
        } \
        param->pic_fields.bits.vafield = hdr->ref_frame_indices[idx]; \
        param->pic_fields.bits.vafield##_sign_bias = hdr->ref_frame_sign_bias[idx]; \
    } while (0)


    if (hdr->frame_type == VP9_KEY_FRAME) {
        m_reference.clear();
        m_reference.resize(VP9_REF_FRAMES);
    } else {
        FILL_REFERENCE(last_ref_frame, VP9_LAST_FRAME);
        FILL_REFERENCE(golden_ref_frame, VP9_GOLDEN_FRAME);
        FILL_REFERENCE(alt_ref_frame, VP9_ALTREF_FRAME);
    }
    for (size_t i = 0; i < m_reference.size(); i++) {
        SurfacePtr& surface = m_reference[i];
        param->reference_frames[i] = surface.get() ? surface->getID():VA_INVALID_SURFACE;
    }
    return true;
#undef FILL_REFERENCE
}

void VaapiDecoderVP9::updateReference(const PicturePtr& picture, const Vp9FrameHdr* hdr)
{
    uint8_t flag  = 1;
    uint8_t refresh_frame_flags;
    if (hdr->frame_type == VP9_KEY_FRAME) {
        refresh_frame_flags = 0xff;
    } else {
        refresh_frame_flags = hdr->refresh_frame_flags;
    }

    for (int i = 0; i < VP9_REF_FRAMES; i++) {
        if (refresh_frame_flags & flag) {
            m_reference[i] = picture->getSurface();
        }
        flag <<= 1;
    }
}

bool VaapiDecoderVP9::ensurePicture(const PicturePtr& picture, const Vp9FrameHdr* hdr)
{
    VADecPictureParameterBufferVP9* param;
    if (!picture->editPicture(param))
        return false;
    param->frame_width = hdr->width;
    param->frame_height = hdr->height;
    if (!fillReference(param, hdr))
        return false;


#define FILL_PIC_FIELD(field) param->pic_fields.bits.field = hdr->field;
    FILL_PIC_FIELD(subsampling_x)
    FILL_PIC_FIELD(subsampling_y)
    FILL_PIC_FIELD(frame_type)
    FILL_PIC_FIELD(show_frame)
    FILL_PIC_FIELD(error_resilient_mode)
    FILL_PIC_FIELD(intra_only)
    FILL_PIC_FIELD(allow_high_precision_mv)
    FILL_PIC_FIELD(mcomp_filter_type)
    FILL_PIC_FIELD(frame_parallel_decoding_mode)
    FILL_PIC_FIELD(reset_frame_context)
    FILL_PIC_FIELD(refresh_frame_context)
    FILL_PIC_FIELD(frame_context_idx)
#undef FILL_PIC_FIELD

    param->pic_fields.bits.segmentation_enabled = hdr->segmentation.enabled;
    param->pic_fields.bits.segmentation_temporal_update = hdr->segmentation.temporal_update;
    param->pic_fields.bits.segmentation_update_map = hdr->segmentation.update_map;
    param->pic_fields.bits.lossless_flag = m_parser->lossless_flag;

    param->filter_level = hdr->loopfilter.filter_level;
    param->sharpness_level = hdr->loopfilter.sharpness_level;


#define FILL_FIELD(field) param->field = hdr->field;
    FILL_FIELD(log2_tile_rows);
    FILL_FIELD(log2_tile_columns);
    FILL_FIELD(frame_header_length_in_bytes)
    FILL_FIELD(first_partition_size)
#undef FILL_FIELD
    param->profile = hdr->profile;
    param->bit_depth = 8 + m_parser->bit_depth * 2;

    assert(sizeof(param->mb_segment_tree_probs) == sizeof(m_parser->mb_segment_tree_probs));
    assert(sizeof(param->segment_pred_probs) == sizeof(m_parser->segment_pred_probs));
    memcpy(param->mb_segment_tree_probs, m_parser->mb_segment_tree_probs, sizeof(m_parser->mb_segment_tree_probs));
    memcpy(param->segment_pred_probs, m_parser->segment_pred_probs, sizeof(m_parser->segment_pred_probs));

    return true;
}

bool VaapiDecoderVP9::ensureSlice(const PicturePtr& picture, const void* data, int size)
{
#define FILL_FIELD(field) vaseg.field = seg.field;

    VASliceParameterBufferVP9* slice;
    if (!picture->newSlice(slice, data, size))
        return false;
    for (int i = 0; i < VP9_MAX_SEGMENTS; i++) {
        VASegmentParameterVP9& vaseg = slice->seg_param[i];
        Vp9Segmentation& seg = m_parser->segmentation[i];
        memcpy(vaseg.filter_level, seg.filter_level, sizeof(seg.filter_level));
        FILL_FIELD(luma_ac_quant_scale)
        FILL_FIELD(luma_dc_quant_scale)
        FILL_FIELD(chroma_ac_quant_scale)
        FILL_FIELD(chroma_dc_quant_scale)

        vaseg.segment_flags.fields.segment_reference_skipped = seg.reference_skip;
        vaseg.segment_flags.fields.segment_reference_enabled = seg.reference_frame_enabled;
        vaseg.segment_flags.fields.segment_reference = seg.reference_frame;

    }
#undef FILL_FIELD
    return true;
}

YamiStatus VaapiDecoderVP9::decode(const Vp9FrameHdr* hdr, const uint8_t* data, uint32_t size, uint64_t timeStamp)
{

    YamiStatus ret;
    ret = ensureContext(hdr);
    if (ret != YAMI_SUCCESS)
        return ret;

    PicturePtr picture;
    ret = createPicture(picture, timeStamp);
    if (ret != YAMI_SUCCESS)
        return ret;

    if (hdr->show_existing_frame) {
        SurfacePtr& surface = m_reference[hdr->frame_to_show];
        if (!surface) {
            ERROR("frame to show is invalid, idx = %d", hdr->frame_to_show);
            return YAMI_SUCCESS;
        }
        picture->setSurface(surface);
        return outputPicture(picture);
    }

    if (!picture->getSurface()->setCrop(0, 0, hdr->width, hdr->height)) {
        ERROR("resize to %dx%d failed", hdr->width, hdr->height);
        return YAMI_OUT_MEMORY;
    }

    if (!ensurePicture(picture, hdr))
        return YAMI_FAIL;
    if (!ensureSlice(picture, data, size))
        return YAMI_FAIL;
    if (!picture->decode())
        return YAMI_FAIL;
    updateReference(picture, hdr);
    if (hdr->show_frame)
        return outputPicture(picture);
    return YAMI_SUCCESS;
}

static bool parse_super_frame(std::vector<uint32_t>& frameSize, const uint8_t* data, const size_t size)
{
    if (!data || !size)
        return false;
    uint8_t marker;
    marker =  *(data + size - 1);
    if ((marker & 0xe0) != 0xc0) {
        frameSize.push_back(size);
        return true;
    }
    const uint32_t frames = (marker & 0x7) + 1;
    const uint32_t mag = ((marker >> 3) & 0x3) + 1;
    const size_t indexSz = 2 + mag * frames;
    if (size < indexSz)
        return false;
    data += size - indexSz;
    const uint8_t marker2 = *data++;
    if (marker != marker2)
        return false;
    for (uint32_t i = 0; i < frames; i++) {
        uint32_t sz = 0;
        for (uint32_t j = 0; j < mag; j++) {
            sz |= (*data++) << (j * 8);
        }
        frameSize.push_back(sz);
    }
    return true;
}

YamiStatus VaapiDecoderVP9::decode(VideoDecodeBuffer* buffer)
{
    YamiStatus status;
    if (!buffer || !buffer->data) {
        flush(false);
        return YAMI_SUCCESS;
    }
    uint8_t* data = buffer->data;
    size_t  size = buffer->size;
    uint8_t* end = data + size;
    std::vector<uint32_t> frameSize;
    if (!parse_super_frame(frameSize, data, size))
        return YAMI_DECODE_INVALID_DATA;
    for (size_t i = 0; i < frameSize.size(); i++) {
        uint32_t sz = frameSize[i];
        if (data + sz > end)
            return YAMI_DECODE_INVALID_DATA;
        status = decode(data, sz, buffer->timeStamp);
        if (status != YAMI_SUCCESS)
            return status;
        data += sz;
    }
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderVP9::decode(const uint8_t* data, uint32_t size, uint64_t timeStamp)
{
    Vp9FrameHdr hdr;
    if (!m_parser)
        return YAMI_OUT_MEMORY;
    if (vp9_parse_frame_header(m_parser.get(), &hdr, data, size) != VP9_PARSER_OK)
        return YAMI_DECODE_INVALID_DATA;
    if (hdr.first_partition_size + hdr.frame_header_length_in_bytes > size)
        return YAMI_DECODE_INVALID_DATA;
    return decode(&hdr, data, size, timeStamp);
}

}
