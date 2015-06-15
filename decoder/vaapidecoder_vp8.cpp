/*
 *  vaapidecoder_vp8.cpp - vp8 decoder
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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

#include <string.h>
#include "common/log.h"
#include "vaapidecoder_vp8.h"
#include "codecparsers/bytereader.h"

namespace YamiMediaCodec{
typedef VaapiDecoderVP8::PicturePtr PicturePtr;

// the following parameter apply to Intra-Predicted Macroblocks,
// $11.2 $11.4: key frame default probs
static const uint8_t keyFrameYModeProbs[4] = { 145, 156, 163, 128 };
static const uint8_t keyFrameUVModeProbs[3] = { 142, 114, 183 };

// $16.1: non-key frame default probs
static const uint8_t nonKeyFrameDefaultYModeProbs[4] = { 112, 86, 140, 37 };
static const uint8_t nonKeyFrameDefaultUVModeProbs[3] = { 162, 101, 204 };

static Decode_Status getStatus(Vp8ParserResult result)
{
    Decode_Status status;

    switch (result) {
    case VP8_PARSER_OK:
        status = DECODE_SUCCESS;
        break;
    case VP8_PARSER_ERROR:
        status = DECODE_PARSER_FAIL;
        break;
    default:
        status = DECODE_FAIL;
        break;
    }
    return status;
}

/////////////////////////////////////////////////////

Decode_Status VaapiDecoderVP8::ensureContext()
{
    bool resetContext = false;
    Decode_Status status = DECODE_SUCCESS;

    if (!m_frameHdr.key_frame) {
        return DECODE_SUCCESS;
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

    DEBUG("got frame size: %d x %d", m_frameHdr.width, m_frameHdr.height);
    m_frameWidth = m_frameHdr.width;
    m_frameHeight = m_frameHdr.height;

    // only reset va context when there is a larger frame
    if (m_configBuffer.width < m_frameHdr.width
        || m_configBuffer.height < m_frameHdr.height) {
        resetContext = true;
        INFO("frame size changed, reconfig codec. orig size %d x %d, new size: %d x %d", m_configBuffer.width, m_configBuffer.height, m_frameHdr.width, m_frameHdr.height);
        m_configBuffer.width = m_frameHdr.width;
        m_configBuffer.height = m_frameHdr.height;
        m_configBuffer.surfaceWidth = m_configBuffer.width;
        m_configBuffer.surfaceHeight = m_configBuffer.height;
        DEBUG("USE_NATIVE_GRAPHIC_BUFFER: %d",
              m_configBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER);
        if (m_configBuffer.flag & USE_NATIVE_GRAPHIC_BUFFER) {
            m_configBuffer.graphicBufferWidth = m_configBuffer.width;
            m_configBuffer.graphicBufferHeight = m_configBuffer.height;
        }

        if (m_hasContext)
            status = VaapiDecoderBase::terminateVA();
        m_hasContext = false;

        if (status != DECODE_SUCCESS)
            return status;
    } else if (m_videoFormatInfo.width != m_frameHdr.width
        || m_videoFormatInfo.height != m_frameHdr.height) {
        // notify client of resolution change, no need to reset hw context
            INFO("frame size changed, reconfig codec. orig size %d x %d, new size: %d x %d\n", m_videoFormatInfo.width, m_videoFormatInfo.height, m_frameHdr.width, m_frameHdr.height);
            m_videoFormatInfo.width = m_frameHdr.width;
            m_videoFormatInfo.height = m_frameHdr.height;
            // XXX, assume graphicBufferWidth/graphicBufferHeight are hw resolution, needn't update here
            return DECODE_FORMAT_CHANGE;
    }

    if (m_hasContext)
        return DECODE_SUCCESS;

    DEBUG("Start VA context");
    status = VaapiDecoderBase::start(&m_configBuffer);

    if (status != DECODE_SUCCESS)
        return status;

    m_hasContext = true;

    if (resetContext)
        return DECODE_FORMAT_CHANGE;

    return DECODE_SUCCESS;
}

bool VaapiDecoderVP8::fillSliceParam(VASliceParameterBufferVP8* sliceParam)
{
    int32_t lastPartitionSize, i;

    if (m_frameHdr.key_frame)
        sliceParam->slice_data_offset =
            VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
        sliceParam->slice_data_offset =
            VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;

    // XXX, the buf start address m_buffer
    sliceParam->macroblock_offset = m_frameHdr.header_size;
    sliceParam->num_of_partitions = (1 << m_frameHdr.log2_nbr_of_dct_partitions) + 1;   // +1 for the frame header partition

    // first_part_size doesn't include the uncompress data(frame-tage/magic-number/frame-width/height) at the begining of the frame.
    // partition_size[0] refer to 'first_part_size - parsed-bytes-by-range-decoder'
    sliceParam->partition_size[0] =
        m_frameHdr.first_part_size - ((sliceParam->macroblock_offset +
                                       7) >> 3);

#if __PSB_VP8_INTERFACE_WORK_AROUND__
    sliceParam->slice_data_offset = 0;
    sliceParam->macroblock_offset =
        8 - m_frameHdr.rangedecoder_state.remaining_bits;
#endif

    if (m_frameHdr.key_frame)
        lastPartitionSize =
            m_frameSize - VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
        lastPartitionSize =
            m_frameSize - VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;

    lastPartitionSize -= m_frameHdr.first_part_size;

    for (i = 1; i < sliceParam->num_of_partitions - 1; i++) {
        sliceParam->partition_size[i] = m_frameHdr.partition_size[i - 1];
        lastPartitionSize -= (m_frameHdr.partition_size[i - 1] + 3);
    }
    sliceParam->partition_size[sliceParam->num_of_partitions - 1] =
        lastPartitionSize;
    return true;
}

bool VaapiDecoderVP8::fillPictureParam(const PicturePtr&  picture)
{
    int32_t i, n;
    VAPictureParameterBufferVP8 *picParam = NULL;

    if (!picture->editPicture(picParam))
        return false;

    /* Fill in VAPictureParameterBufferVP8 */
    Vp8Segmentation *seg = &m_parser.segmentation;

    /* Fill in VAPictureParameterBufferVP8 */
    if (m_frameHdr.key_frame) {
        if (m_frameHdr.horiz_scale_code || m_frameHdr.vert_scale_code)
            WARNING
                ("horizontal_scale or vertical_scale in VP8 isn't supported yet");
    }

    picParam->frame_width = m_frameWidth;
    picParam->frame_height = m_frameHeight;
    if (m_frameHdr.key_frame) {
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

    picParam->pic_fields.bits.key_frame = !m_frameHdr.key_frame;
    picParam->pic_fields.bits.version = m_frameHdr.version;
    picParam->pic_fields.bits.segmentation_enabled =
        seg->segmentation_enabled;
    picParam->pic_fields.bits.update_mb_segmentation_map =
        seg->update_mb_segmentation_map;
    picParam->pic_fields.bits.update_segment_feature_data =
        seg->update_segment_feature_data;
    picParam->pic_fields.bits.filter_type = m_frameHdr.filter_type;
    picParam->pic_fields.bits.sharpness_level = m_frameHdr.sharpness_level;
    picParam->pic_fields.bits.loop_filter_adj_enable =
        m_parser.mb_lf_adjust.loop_filter_adj_enable;
    picParam->pic_fields.bits.mode_ref_lf_delta_update =
        m_parser.mb_lf_adjust.mode_ref_lf_delta_update;
    picParam->pic_fields.bits.sign_bias_golden =
        m_frameHdr.sign_bias_golden;
    picParam->pic_fields.bits.sign_bias_alternate =
        m_frameHdr.sign_bias_alternate;
    picParam->pic_fields.bits.mb_no_coeff_skip =
        m_frameHdr.mb_no_skip_coeff;

    for (i = 0; i < 3; i++) {
        picParam->mb_segment_tree_probs[i] = seg->segment_prob[i];
    }

    for (i = 0; i < 4; i++) {
        if (seg->segmentation_enabled) {
            picParam->loop_filter_level[i] = seg->lf_update_value[i];
            if (!seg->segment_feature_mode) {
                picParam->loop_filter_level[i] +=
                    m_frameHdr.loop_filter_level;
                picParam->loop_filter_level[i] =
                    CLAMP(picParam->loop_filter_level[i], 0, 63);
            }
        } else
            picParam->loop_filter_level[i] = m_frameHdr.loop_filter_level;

        picParam->loop_filter_deltas_ref_frame[i] =
            m_parser.mb_lf_adjust.ref_frame_delta[i];
        picParam->loop_filter_deltas_mode[i] =
            m_parser.mb_lf_adjust.mb_mode_delta[i];
    }

    picParam->pic_fields.bits.loop_filter_disable =
        m_frameHdr.loop_filter_level == 0;

    picParam->prob_skip_false = m_frameHdr.prob_skip_false;
    picParam->prob_intra = m_frameHdr.prob_intra;
    picParam->prob_last = m_frameHdr.prob_last;
    picParam->prob_gf = m_frameHdr.prob_gf;

    memcpy (picParam->y_mode_probs, m_frameHdr.mode_probs.y_prob,
        sizeof (m_frameHdr.mode_probs.y_prob));
    memcpy (picParam->uv_mode_probs, m_frameHdr.mode_probs.uv_prob,
        sizeof (m_frameHdr.mode_probs.uv_prob));
    memcpy (picParam->mv_probs, m_frameHdr.mv_probs.prob,
        sizeof (m_frameHdr.mv_probs));

    picParam->bool_coder_ctx.range = m_frameHdr.rd_range;
    picParam->bool_coder_ctx.value = m_frameHdr.rd_value;
    picParam->bool_coder_ctx.count = m_frameHdr.rd_count;

    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureQuantMatrix(const PicturePtr&  pic)
{
    Vp8Segmentation *seg = &m_parser.segmentation;
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
                baseQI += m_frameHdr.quant_indices.y_ac_qi;;
        } else
            baseQI = m_frameHdr.quant_indices.y_ac_qi;

        // the first component is y_ac_qi
        tempIndex =
            baseQI < 0 ? 0 : (baseQI >
                              MAX_QI_INDEX ? MAX_QI_INDEX : baseQI);
        iqMatrix->quantization_index[i][0] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quant_indices.y_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][1] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quant_indices.y2_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][2] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quant_indices.y2_ac_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][3] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quant_indices.uv_dc_delta;
        tempIndex =
            tempIndex < 0 ? 0 : (tempIndex >
                                 MAX_QI_INDEX ? MAX_QI_INDEX : tempIndex);
        iqMatrix->quantization_index[i][4] = tempIndex;

        tempIndex = baseQI + m_frameHdr.quant_indices.uv_ac_delta;
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
    int32_t i;

    // XXX, create/render VAProbabilityDataBufferVP8 in base class
    if (!pic->editProbTable(probTable))
        return false;
    memcpy (probTable->dct_coeff_probs, 
		   m_frameHdr.token_probs.prob,
           sizeof (m_frameHdr.token_probs.prob));
    return true;
}

void VaapiDecoderVP8::updateReferencePictures()
{
    const PicturePtr& picture = m_currentPicture;

    // update picture reference
    if (m_frameHdr.key_frame) {
        m_goldenRefPicture = picture;
        m_altRefPicture = picture;
    } else {
        // process refresh_alternate_frame/copy_buffer_to_alternate first
        if (m_frameHdr.refresh_alternate_frame) {
            m_altRefPicture = picture;
        } else {
            switch (m_frameHdr.copy_buffer_to_alternate) {
            case 0:
                // do nothing
                break;
            case 1:
                m_altRefPicture = m_lastPicture;
                break;
            case 2:
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
            case 0:
                // do nothing
                break;
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
    if (m_frameHdr.key_frame || m_frameHdr.refresh_last)
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

bool VaapiDecoderVP8::allocNewPicture()
{
    m_currentPicture = createPicture(m_currentPTS);

    if (!m_currentPicture)
        return false;

    SurfacePtr surface = m_currentPicture->getSurface();
    ASSERT(m_frameWidth && m_frameHeight);
    if (!surface->resize(m_frameWidth, m_frameHeight)) {
        ASSERT(0 && "frame size is bigger than internal surface resolution");
        return false;
    }

    DEBUG ("alloc new picture: %p with surface ID: %x",
         m_currentPicture.get(), m_currentPicture->getSurfaceID());

    return true;
}

Decode_Status VaapiDecoderVP8::decodePicture()
{
    Decode_Status status = DECODE_SUCCESS;

    if (!allocNewPicture())
        return DECODE_FAIL;

    if (!ensureQuantMatrix(m_currentPicture)) {
        ERROR("failed to reset quantizer matrix");
        return DECODE_FAIL;
    }

    if (!ensureProbabilityTable(m_currentPicture)) {
        ERROR("failed to reset probability table");
        return DECODE_FAIL;
    }

    if (!fillPictureParam(m_currentPicture)) {
        ERROR("failed to fill picture parameters");
        return DECODE_FAIL;
    }

    VASliceParameterBufferVP8* sliceParam = NULL;
    const void* sliceData = m_buffer;;
    uint32_t sliceSize = m_frameSize;

#if __PSB_VP8_INTERFACE_WORK_AROUND__
    sliceData = m_frameHdr.rangedecoder_state.buffer,
    sliceSize = m_frameSize - (m_frameHdr.rangedecoder_state.buffer - m_buffer);
#endif


    if (!m_currentPicture->newSlice(sliceParam, sliceData, sliceSize))
        return DECODE_FAIL;

    if (!fillSliceParam(sliceParam))
        return DECODE_FAIL;
    if (!m_currentPicture->decode())
        return DECODE_FAIL;

    DEBUG("VaapiDecoderVP8::decodePicture success");
    return status;
}

VaapiDecoderVP8::VaapiDecoderVP8()
{
    int32_t i;

    m_frameWidth = 0;
    m_frameHeight = 0;
    m_buffer = 0;
    m_frameSize = 0;
    memset(&m_frameHdr, 0, sizeof(Vp8FrameHdr));
    memset(&m_parser, 0, sizeof(Vp8Parser));

    // m_yModeProbs[4];
    // m_uvModeProbs[3];
    m_sizeChanged = 0;
    m_hasContext = false;
}

VaapiDecoderVP8::~VaapiDecoderVP8()
{
    stop();
}


Decode_Status VaapiDecoderVP8::start(VideoConfigBuffer * buffer)
{
    DEBUG("VP8: start() buffer size: %d x %d", buffer->width,
          buffer->height);
    Decode_Status status;

    if ((buffer->flag & HAS_SURFACE_NUMBER)
        && (buffer->flag & HAS_VA_PROFILE)) {
    }

    buffer->profile = VAProfileVP8Version0_3;
    buffer->surfaceNumber = 3 + VP8_EXTRA_SURFACE_NUMBER;


    DEBUG("disable native graphics buffer");
    buffer->flag &= ~USE_NATIVE_GRAPHIC_BUFFER;
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
    return DECODE_SUCCESS;
}

Decode_Status VaapiDecoderVP8::reset(VideoConfigBuffer * buffer)
{
    DEBUG("VP8: reset()");
    return VaapiDecoderBase::reset(buffer);
}

void VaapiDecoderVP8::stop(void)
{
    int i;
    DEBUG("VP8: stop()");
    flush();
    VaapiDecoderBase::stop();
}

void VaapiDecoderVP8::flush(void)
{
    DEBUG("VP8: flush()");
    /*FIXME: should output all surfaces in drain mode*/
    m_currentPicture.reset();
    m_lastPicture.reset();
    m_goldenRefPicture.reset();
    m_altRefPicture.reset();

    VaapiDecoderBase::flush();
}

Decode_Status VaapiDecoderVP8::decode(VideoDecodeBuffer * buffer)
{
    Decode_Status status;
    Vp8ParserResult result;
    bool isEOS = false;

    m_currentPTS = buffer->timeStamp;

    m_buffer = buffer->data;
    m_frameSize = buffer->size;

    DEBUG("VP8: Decode(bufsize =%d, timestamp=%ld)", m_frameSize,
          m_currentPTS);

    do {
        if (m_frameSize == 0) {
            status = DECODE_FAIL;
            break;
        }

        memset(&m_frameHdr, 0, sizeof(m_frameHdr));
        result =
            vp8_parser_parse_frame_header(&m_parser, &m_frameHdr, m_buffer, m_frameSize);
        status = getStatus(result);
        if (status != DECODE_SUCCESS) {
            break;
        }

        if (m_frameHdr.key_frame) {
            status = ensureContext();
            if (status != DECODE_SUCCESS)
                return status;
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
        } while (status == DECODE_SUCCESS && ++ii < decodeCount);

#else
        status = decodePicture();
#endif

        if (status != DECODE_SUCCESS)
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

    if (status != DECODE_SUCCESS) {
        DEBUG("decode fail!!");
    }

    return status;
}
}
