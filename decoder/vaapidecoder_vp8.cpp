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
#include "vaapidecoder_vp8.h"

using ::YamiParser::Vp8Parser;
using ::YamiParser::Vp8FrameHeader;
using ::YamiParser::Vp8ParserResult;
using ::YamiParser::Vp8SegmentationHeader;

namespace YamiMediaCodec{
typedef VaapiDecoderVP8::PicturePtr PicturePtr;

// the following parameter apply to Intra-Predicted Macroblocks,
// $11.2 $11.4: key frame default probs
static const uint8_t keyFrameYModeProbs[4] = { 145, 156, 163, 128 };
static const uint8_t keyFrameUVModeProbs[3] = { 142, 114, 183 };

// $16.1: non-key frame default probs
static const uint8_t nonKeyFrameDefaultYModeProbs[4] = { 112, 86, 140, 37 };
static const uint8_t nonKeyFrameDefaultUVModeProbs[3] = { 162, 101, 204 };

static const uint32_t surfaceNumVP8 = 3;

static YamiStatus getStatus(Vp8ParserResult result)
{
    YamiStatus status;

    switch (result) {
    case YamiParser::VP8_PARSER_OK:
        status = YAMI_SUCCESS;
        break;
    default:
        /* we should return no faltal error for parser failed */
        status = YAMI_DECODE_INVALID_DATA;
        break;
    }
    return status;
}

/////////////////////////////////////////////////////

YamiStatus VaapiDecoderVP8::ensureContext()
{
    if (m_frameHdr.key_frame != Vp8FrameHeader::KEYFRAME) {
        return YAMI_SUCCESS;
    }

    /*
       - for VP8 spec, there are two resolution,
       1. one is frame resolution, it may (or may not) change upon key frame: m_frameWidth/m_frameHeight.
       2. another is stream resolution, it is the max resolution of frames. (for example, resolution in ivf header).
       (represented by m_configBuffer width/height below)
       - for codec, there are two set of resolution,
       1. one is in m_configBuffer: width/height and graphicsBufferWidth/graphicsBufferHeight
       it is set from upper layer to config codec
       2. another is m_videoFormatInfo: width/height and surfaceWidth/surfaceHeight
       it is reported to upper layer for configured codec
       - solution here:
       1. vp8 decoder only update m_configBuffer, since VaapiDecoderBase::start() will copy
       m_configBuffer resolution to  m_videoFormatInfo. (todo, is it ok to mark m_videoFormatInfo as private?)
       2. we use the resolution in m_configBuffer as stream resolution of VP8 spec.
       so, m_confiBuffer width/height may update upon key frame
       3. we don't care m_configBuffer graphicBufferWidth/graphicBufferHeight for now,
       since that is used for android
    */

    m_frameWidth = m_frameHdr.width;
    m_frameHeight = m_frameHdr.height;
    if (setFormat(m_frameWidth, m_frameHeight, m_frameWidth, m_frameHeight, surfaceNumVP8)) {
        return YAMI_DECODE_FORMAT_CHANGE;
    }
    return ensureProfile(VAProfileVP8Version0_3);
}

bool VaapiDecoderVP8::fillSliceParam(VASliceParameterBufferVP8* sliceParam)
{
    /* Fill in VASliceParameterBufferVP8 */
    sliceParam->slice_data_offset = 0; //uncompressed data chunk is parsed
    sliceParam->macroblock_offset = m_frameHdr.macroblock_bit_offset; // m_frameHdr.header_size;
    sliceParam->num_of_partitions = m_frameHdr.num_of_dct_partitions + 1;
    sliceParam->partition_size[0] =
        m_frameHdr.first_part_size - ((sliceParam->macroblock_offset + 7) >> 3);
    for (int32_t i = 1; i < sliceParam->num_of_partitions; i++)
        sliceParam->partition_size[i] = m_frameHdr.dct_partition_sizes[i - 1];
    return TRUE;
}

bool VaapiDecoderVP8::fillPictureParam(const PicturePtr&  picture)
{
    uint32_t i;
    VAPictureParameterBufferVP8 *picParam = NULL;

    if (!picture->editPicture(picParam))
        return false;

    /* Fill in VAPictureParameterBufferVP8 */
    Vp8SegmentationHeader *seg = &m_frameHdr.segmentation_hdr;

    /* Fill in VAPictureParameterBufferVP8 */
    if (m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) {
        if (m_frameHdr.horizontal_scale || m_frameHdr.vertical_scale)
            WARNING
                ("horizontal_scale or vertical_scale in VP8 isn't supported yet");
    }

    picParam->frame_width = m_frameWidth;
    picParam->frame_height = m_frameHeight;
    if (m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) {
        picParam->last_ref_frame = VA_INVALID_SURFACE;
        picParam->golden_ref_frame = VA_INVALID_SURFACE;
        picParam->alt_ref_frame = VA_INVALID_SURFACE;
    } else {
        picParam->last_ref_frame =
            m_lastPicture ? m_lastPicture->getSurfaceID() :
            VA_INVALID_SURFACE;
        picParam->golden_ref_frame =
            m_goldenRefPicture ? m_goldenRefPicture->getSurfaceID() :
            VA_INVALID_SURFACE;
        picParam->alt_ref_frame =
            m_altRefPicture ? m_altRefPicture->getSurfaceID() :
            VA_INVALID_SURFACE;
    }
    picParam->out_of_loop_frame = VA_INVALID_SURFACE;   // not used currently

    picParam->pic_fields.bits.key_frame = (m_frameHdr.key_frame != Vp8FrameHeader::KEYFRAME);
    picParam->pic_fields.bits.version = m_frameHdr.version;
    picParam->pic_fields.bits.segmentation_enabled =
        seg->segmentation_enabled;
    picParam->pic_fields.bits.update_mb_segmentation_map =
        seg->update_mb_segmentation_map;
    picParam->pic_fields.bits.update_segment_feature_data =
        seg->update_segment_feature_data;
    picParam->pic_fields.bits.filter_type = m_frameHdr.loopfilter_hdr.type;
    picParam->pic_fields.bits.sharpness_level = m_frameHdr.loopfilter_hdr.sharpness_level;
    picParam->pic_fields.bits.loop_filter_adj_enable =
        m_frameHdr.loopfilter_hdr.loop_filter_adj_enable;
    picParam->pic_fields.bits.mode_ref_lf_delta_update =
        m_frameHdr.loopfilter_hdr.mode_ref_lf_delta_update;
    picParam->pic_fields.bits.sign_bias_golden =
        m_frameHdr.sign_bias_golden;
    picParam->pic_fields.bits.sign_bias_alternate =
        m_frameHdr.sign_bias_alternate;
    picParam->pic_fields.bits.mb_no_coeff_skip =
        m_frameHdr.mb_no_skip_coeff;

    //using arraysize as the real limit
    for (i = 0; i < arraysize(seg->segment_prob); i++) {
        picParam->mb_segment_tree_probs[i] = seg->segment_prob[i];
    }

    for (i = 0; i < arraysize(seg->lf_update_value); ++i) {
        if (seg->segmentation_enabled) {
            if (seg->segment_feature_mode !=
                    Vp8SegmentationHeader::FEATURE_MODE_ABSOLUTE){
                seg->lf_update_value[i] +=
                    m_frameHdr.loopfilter_hdr.level;
            }
            picParam->loop_filter_level[i] = CLAMP(seg->lf_update_value[i], 0, 63);
        } else
            picParam->loop_filter_level[i] = CLAMP(m_frameHdr.loopfilter_hdr.level, 0, 63);

        picParam->loop_filter_deltas_ref_frame[i] =
            m_frameHdr.loopfilter_hdr.ref_frame_delta[i];
        picParam->loop_filter_deltas_mode[i] =
            m_frameHdr.loopfilter_hdr.mb_mode_delta[i];
    }

    picParam->pic_fields.bits.loop_filter_disable =
        m_frameHdr.loopfilter_hdr.level == 0;

    picParam->prob_skip_false = m_frameHdr.prob_skip_false;
    picParam->prob_intra = m_frameHdr.prob_intra;
    picParam->prob_last = m_frameHdr.prob_last;
    picParam->prob_gf = m_frameHdr.prob_gf;

    memcpy (picParam->y_mode_probs, m_frameHdr.entropy_hdr.y_mode_probs,
        sizeof (m_frameHdr.entropy_hdr.y_mode_probs));
    memcpy (picParam->uv_mode_probs, m_frameHdr.entropy_hdr.uv_mode_probs,
        sizeof (m_frameHdr.entropy_hdr.uv_mode_probs));
    memcpy (picParam->mv_probs, m_frameHdr.entropy_hdr.mv_probs,
        sizeof (m_frameHdr.entropy_hdr.mv_probs));

    picParam->bool_coder_ctx.range = m_frameHdr.bool_dec_range;
    picParam->bool_coder_ctx.value = m_frameHdr.bool_dec_value;
    picParam->bool_coder_ctx.count = m_frameHdr.bool_dec_count;

    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureQuantMatrix(const PicturePtr&  pic)
{
    Vp8SegmentationHeader *seg = &m_frameHdr.segmentation_hdr;
    VAIQMatrixBufferVP8 *iqMatrix;
    int32_t baseQI, i;

    if (!pic->editIqMatrix(iqMatrix))
        return false;

    for (i = 0; i < 4; i++) {
        int32_t tempIndex;
        const int32_t MAX_QI_INDEX = 127;
        if (seg->segmentation_enabled) {
            baseQI = seg->quantizer_update_value[i];
            if (!seg->segment_feature_mode) // 0 means delta update
                baseQI += m_frameHdr.quantization_hdr.y_ac_qi;;
        } else
            baseQI = m_frameHdr.quantization_hdr.y_ac_qi;

        // the first component is y_ac_qi
        tempIndex =
            baseQI < 0 ? 0 : (baseQI >
                              MAX_QI_INDEX ? MAX_QI_INDEX : baseQI);
        iqMatrix->quantization_index[i][0] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quantization_hdr.y_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][1] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quantization_hdr.y2_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][2] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quantization_hdr.y2_ac_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][3] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quantization_hdr.uv_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][4] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quantization_hdr.uv_ac_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][5] = tempIndex;
    }

    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureProbabilityTable(const PicturePtr&  pic)
{
    VAProbabilityDataBufferVP8 *probTable = NULL;

    // XXX, create/render VAProbabilityDataBufferVP8 in base class
    if (!pic->editProbTable(probTable))
        return false;
    memcpy(probTable->dct_coeff_probs,
        m_frameHdr.entropy_hdr.coeff_probs,
        sizeof(m_frameHdr.entropy_hdr.coeff_probs));
    return true;
}

void VaapiDecoderVP8::updateReferencePictures()
{
    const PicturePtr& picture = m_currentPicture;

    // update picture reference
    if (m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) {
        m_goldenRefPicture = picture;
        m_altRefPicture = picture;
    } else {
        // process refresh_alternate_frame/copy_buffer_to_alternate first
        if (m_frameHdr.refresh_alternate_frame) {
            m_altRefPicture = picture;
        } else {
            switch (m_frameHdr.copy_buffer_to_alternate) {
	    case Vp8FrameHeader::COPY_LAST_TO_ALT:
                m_altRefPicture = m_lastPicture;
                break;
        case Vp8FrameHeader::COPY_GOLDEN_TO_ALT:
                m_altRefPicture = m_goldenRefPicture;
                break;
            default:
                WARNING
                    ("WARNING: VP8 decoder: unrecognized copy_buffer_to_alternate");
            }
        }

        if (m_frameHdr.refresh_golden_frame) {
            m_goldenRefPicture = picture;
        } else {
            switch (m_frameHdr.copy_buffer_to_golden) {
            case 1:
                m_goldenRefPicture = m_lastPicture;
                break;
            case 2:
                m_goldenRefPicture = m_altRefPicture;
                break;
            default:
                WARNING
                    ("WARNING: VP8 decoder: unrecognized copy_buffer_to_golden");
            }
        }
    }
    if ((m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) || m_frameHdr.refresh_last)
        m_lastPicture = picture;
    if (m_goldenRefPicture)
        DEBUG("m_goldenRefPicture: %p, SurfaceID: %x",
              m_goldenRefPicture.get(), m_goldenRefPicture->getSurfaceID());
    if (m_altRefPicture)
        DEBUG("m_altRefPicture: %p, SurfaceID: %x", m_altRefPicture.get(),
              m_altRefPicture->getSurfaceID());
    if (m_lastPicture)
        DEBUG("m_lastPicture: %p, SurfaceID: %x", m_lastPicture.get(),
              m_lastPicture->getSurfaceID());
    if (m_currentPicture)
        DEBUG("m_currentPicture: %p, SurfaceID: %x", m_currentPicture.get(),
              m_currentPicture->getSurfaceID());

}

YamiStatus VaapiDecoderVP8::allocNewPicture()
{
    YamiStatus status = createPicture(m_currentPicture, m_currentPTS);

    if (status != YAMI_SUCCESS)
        return status;

    SurfacePtr surface = m_currentPicture->getSurface();
    ASSERT(m_frameWidth && m_frameHeight);
    if (!surface->setCrop(0, 0, m_frameWidth, m_frameHeight)) {
        ASSERT(0 && "frame size is bigger than internal surface resolution");
        return YAMI_FAIL;
    }

    DEBUG ("alloc new picture: %p with surface ID: %x",
         m_currentPicture.get(), m_currentPicture->getSurfaceID());

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderVP8::decodePicture()
{
    YamiStatus status = YAMI_SUCCESS;

    status = allocNewPicture();
    if (status != YAMI_SUCCESS)
        return status;

    if (!ensureQuantMatrix(m_currentPicture)) {
        ERROR("failed to reset quantizer matrix");
        return YAMI_FAIL;
    }

    if (!ensureProbabilityTable(m_currentPicture)) {
        ERROR("failed to reset probability table");
        return YAMI_FAIL;
    }

    if (!fillPictureParam(m_currentPicture)) {
        ERROR("failed to fill picture parameters");
        return YAMI_FAIL;
    }

    VASliceParameterBufferVP8* sliceParam = NULL;
    const void* sliceData = m_buffer + m_frameHdr.first_part_offset;
    uint32_t sliceSize = m_frameSize - m_frameHdr.first_part_offset;
DEBUG("sliceData %p sliceSize %d", sliceData, sliceSize);

    if (!m_currentPicture->newSlice(sliceParam, sliceData, sliceSize))
        return YAMI_FAIL;

    if (!fillSliceParam(sliceParam))
        return YAMI_FAIL;
    if (!m_currentPicture->decode())
        return YAMI_FAIL;

    DEBUG("VaapiDecoderVP8::decodePicture success");
    return status;
}

VaapiDecoderVP8::VaapiDecoderVP8()
{
    m_frameWidth = 0;
    m_frameHeight = 0;
    m_buffer = 0;
    m_frameSize = 0;
    memset(&m_frameHdr, 0, sizeof(m_frameHdr));
    memset(&m_parser, 0, sizeof(Vp8Parser));

    // m_yModeProbs[4];
    // m_uvModeProbs[3];
    m_sizeChanged = 0;
    m_hasContext = false;
    m_gotKeyFrame = false;
}

VaapiDecoderVP8::~VaapiDecoderVP8()
{
    stop();
}

YamiStatus VaapiDecoderVP8::start(VideoConfigBuffer* buffer)
{
    DEBUG("VP8: start() buffer size: %d x %d", buffer->width,
          buffer->height);

    if ((buffer->flag & HAS_SURFACE_NUMBER)
        && (buffer->flag & HAS_VA_PROFILE)) {
    }

    buffer->profile = VAProfileVP8Version0_3;
    buffer->surfaceNumber = 3;


    DEBUG("disable native graphics buffer");
    m_configBuffer = *buffer;
    m_configBuffer.data = NULL;
    m_configBuffer.size = 0;

    // it is a good timing to report resolution change (gst-omx does), however, it fails on chromeos
    // so we force to update resolution on first key frame
    m_configBuffer.width = 0;
    m_configBuffer.height = 0;
#if __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__
    m_isFirstFrame = true;
#endif
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderVP8::reset(VideoConfigBuffer* buffer)
{
    DEBUG("VP8: reset()");
    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderVP8::stop(void)
{
    DEBUG("VP8: stop()");
    flush();
    VaapiDecoderBase::stop();
}

void VaapiDecoderVP8::flush(bool discardOutput)
{
    DEBUG("VP8: flush()");
    /*FIXME: should output all surfaces in drain mode*/
    m_currentPicture.reset();
    m_lastPicture.reset();
    m_goldenRefPicture.reset();
    m_altRefPicture.reset();
    m_gotKeyFrame = false;

    if (discardOutput)
        VaapiDecoderBase::flush();
}

void VaapiDecoderVP8::flush(void)
{
    DEBUG("VP8: flush()");
    /*FIXME: should output all surfaces in drain mode*/
    flush(true);
}

YamiStatus VaapiDecoderVP8::decode(VideoDecodeBuffer* buffer)
{
    YamiStatus status;
    Vp8ParserResult result;
    if (!buffer || !buffer->data) {
        flush(false);
        return YAMI_SUCCESS;
    }

    m_currentPTS = buffer->timeStamp;

    m_buffer = buffer->data;
    m_frameSize = buffer->size;

    DEBUG("VP8: Decode(bufsize =%d, timestamp=%ld)", m_frameSize,
        m_currentPTS);

    do {
        if (m_frameSize == 0) {
            status = YAMI_FAIL;
            break;
        }

        memset(&m_frameHdr, 0, sizeof(m_frameHdr));
        result = m_parser.ParseFrame(m_buffer,m_frameSize,&m_frameHdr);
        status = getStatus(result);
        if (status != YAMI_SUCCESS) {
            break;
        }

        if (!targetTemporalFrame())
            return YAMI_SUCCESS;

        if (m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) {
            status = ensureContext();
            if (status != YAMI_SUCCESS)
                return status;
            m_gotKeyFrame = true;
        }
        else {
            if (!m_gotKeyFrame) {
                WARNING("we can't decode p frame without key");
                return YAMI_DECODE_INVALID_DATA;
            }
        }
#if __PSB_CACHE_DRAIN_FOR_FIRST_FRAME__
        int ii = 0;
        int decodeCount = 1;

        if (m_isFirstFrame) {
            decodeCount = 1280 * 720 / m_frameWidth / m_frameHeight * 2;
            m_isFirstFrame = false;
        }

        do {
            status = decodePicture();
        } while (status == YAMI_SUCCESS && ++ii < decodeCount);

#else
        status = decodePicture();
#endif

        if (status != YAMI_SUCCESS)
            break;

        if (m_frameHdr.show_frame) {
            m_currentPicture->m_timeStamp = m_currentPTS;
            //FIXME: add output
            outputPicture(m_currentPicture);
        } else {
            WARNING("warning: this picture isn't sent to render");
        }

        updateReferencePictures();

    } while (0);

    if (status != YAMI_SUCCESS) {
        DEBUG("decode fail!!");
    }

    return status;
}

bool VaapiDecoderVP8::targetTemporalFrame()
{
    switch (m_configBuffer.temporalLayer) {
    case 0: //decode all layers.
        return true;
    case 1:
        if ((m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) ||
            m_frameHdr.refresh_last)
            return true;
        break;
    case 2:
        if ((m_frameHdr.key_frame == Vp8FrameHeader::KEYFRAME) ||
            m_frameHdr.refresh_last || m_frameHdr.refresh_golden_frame)
            return true;
        break;
    case 3:
        return true;
    default: //decode all layers.
        return true;
    }

    return false;
}
}
