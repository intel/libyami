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

#include "common/log.h"
#include "vaapidecoder_vp8.h"
#include "codecparsers/bytereader.h"

// the following parameter apply to Intra-Predicted Macroblocks,
// $11.2 $11.4: key frame default probs
static const uint8_t keyFrameYModeProbs[4] = { 145, 156, 163, 128 };
static const uint8_t keyFrameUVModeProbs[3] = { 142, 114, 183 };

// $16.1: non-key frame default probs
static const uint8_t nonKeyFrameDefaultYModeProbs[4] = { 112, 86, 140, 37 };
static const uint8_t nonKeyFrameDefaultUVModeProbs[3] = { 162, 101, 204 };

bool VaapiDecoderVP8::replacePicture(VaapiPictureVP8 * &pic1,
                                     VaapiPictureVP8 * pic2)
{
    // VP8 picture ref count is simple, we just replace the pointer
    pic1 = pic2;
    return true;
}

static Decode_Status getStatus(Vp8ParseResult result)
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


VaapiSliceVP8::VaapiSliceVP8(VADisplay display,
                             VAContextID ctx,
                             const uint8_t * sliceData, uint32_t sliceSize)
{
    VASliceParameterBufferVP8 *paramBuf;

    if (!display || !ctx) {
        ERROR("VA display or context not initialized yet");
        return;
    }

    /* new vp8 slice data buffer */
    m_data = new VaapiBufObject(display,
                                ctx, VASliceDataBufferType, sliceData,
                                sliceSize);

    /* new vp8 slice parameter buffer */
    m_param = new VaapiBufObject(display,
                                 ctx,
                                 VASliceParameterBufferType, NULL,
                                 sizeof(VASliceParameterBufferVP8));

    paramBuf = (VASliceParameterBufferVP8 *) m_param->map();

    paramBuf->slice_data_size = sliceSize;
    paramBuf->slice_data_offset = 0;
    paramBuf->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    m_param->unmap();
}

VaapiSliceVP8::~VaapiSliceVP8()
{
    delete m_data;
    delete m_param;
}

VaapiPictureVP8::VaapiPictureVP8(VADisplay display,
                                 VAContextID context,
                                 VaapiSurfaceBufferPool * surfBufPool,
                                 VaapiPictureStructure structure)
:  VaapiPicture(display, context, surfBufPool, structure)
{
    structure = VAAPI_PICTURE_STRUCTURE_FRAME;

    /* new vp8 slice parameter buffer */
    m_picParam = new VaapiBufObject(display,
                                    context,
                                    VAPictureParameterBufferType,
                                    NULL,
                                    sizeof(VAPictureParameterBufferVP8));
    if (!m_picParam)
        ERROR("create vp8 picture parameter buffer object failed");

    m_iqMatrix = new VaapiBufObject(display,
                                    context, VAIQMatrixBufferType, NULL,
                                    sizeof(VAIQMatrixBufferVP8));
    if (!m_iqMatrix)
        ERROR("create vp8 iq matrix buffer object failed");

    m_probTable = new VaapiBufObject(display,
                                     context,
                                     VAProbabilityBufferType, NULL,
                                     sizeof(VAProbabilityDataBufferVP8));
    if (!m_probTable)
        ERROR("create vp8 probability table buffer object failed");

}

/////////////////////////////////////////////////////

Decode_Status VaapiDecoderVP8::ensureContext()
{
    bool resetContext = false;
    Decode_Status status = DECODE_SUCCESS;

    if (m_frameHdr.key_frame != VP8_KEY_FRAME) {
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

bool VaapiDecoderVP8::fillSliceParam(VaapiSliceVP8 * slice)
{
    VaapiBufObject *sliceParamObj = slice->m_param;
    VASliceParameterBufferVP8 *sliceParam =
        (VASliceParameterBufferVP8 *) sliceParamObj->map();
    int32_t lastPartitionSize, i;


    if (m_frameHdr.key_frame == VP8_KEY_FRAME)
        sliceParam->slice_data_offset =
            VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
        sliceParam->slice_data_offset =
            VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;

    // XXX, the buf start address m_buffer
    sliceParam->macroblock_offset =
        (m_frameHdr.rangedecoder_state.buffer - m_buffer -
         sliceParam->slice_data_offset) * 8 -
        m_frameHdr.rangedecoder_state.remaining_bits;
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

    if (m_frameHdr.key_frame == VP8_KEY_FRAME)
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

    sliceParamObj->unmap();
    return true;
}

bool VaapiDecoderVP8::fillPictureParam(VaapiPictureVP8 * picture)
{
    int32_t i, n;
    VaapiBufObject *picParamObj = picture->m_picParam;

    VAPictureParameterBufferVP8 *picParam =
        (VAPictureParameterBufferVP8 *) picParamObj->map();

    /* Fill in VAPictureParameterBufferVP8 */
    VASliceParameterBufferVP8 *sliceParam = NULL;
    Vp8Segmentation *seg = &m_frameHdr.multi_frame_data->segmentation;

    memset(picParam, 0, sizeof(*picParam));
    /* Fill in VAPictureParameterBufferVP8 */
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        if (m_frameHdr.horizontal_scale || m_frameHdr.vertical_scale)
            WARNING
                ("horizontal_scale or vertical_scale in VP8 isn't supported yet");
    }

    picParam->frame_width = m_frameWidth;
    picParam->frame_height = m_frameHeight;
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        picParam->last_ref_frame = VA_INVALID_SURFACE;
        picParam->golden_ref_frame = VA_INVALID_SURFACE;
        picParam->alt_ref_frame = VA_INVALID_SURFACE;
    } else {
        picParam->last_ref_frame =
            m_lastPicture ? m_lastPicture->m_surfaceID :
            VA_INVALID_SURFACE;
        picParam->golden_ref_frame =
            m_goldenRefPicture ? m_goldenRefPicture->m_surfaceID :
            VA_INVALID_SURFACE;
        picParam->alt_ref_frame =
            m_altRefPicture ? m_altRefPicture->m_surfaceID :
            VA_INVALID_SURFACE;
    }
    picParam->out_of_loop_frame = VA_INVALID_SURFACE;   // not used currently

    picParam->pic_fields.bits.key_frame = m_frameHdr.key_frame;
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
        m_frameHdr.multi_frame_data->mb_lf_adjust.loop_filter_adj_enable;
    picParam->pic_fields.bits.mode_ref_lf_delta_update =
        m_frameHdr.multi_frame_data->mb_lf_adjust.mode_ref_lf_delta_update;
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
            m_frameHdr.multi_frame_data->mb_lf_adjust.ref_frame_delta[i];
        picParam->loop_filter_deltas_mode[i] =
            m_frameHdr.multi_frame_data->mb_lf_adjust.mb_mode_delta[i];
    }
    if ((picParam->pic_fields.bits.version == 0)
        || (picParam->pic_fields.bits.version == 1)) {
        picParam->pic_fields.bits.loop_filter_disable =
            m_frameHdr.loop_filter_level == 0;
    }

    picParam->prob_skip_false = m_frameHdr.prob_skip_false;
    picParam->prob_intra = m_frameHdr.prob_intra;
    picParam->prob_last = m_frameHdr.prob_last;
    picParam->prob_gf = m_frameHdr.prob_gf;

    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        // key frame use fixed prob table
        memcpy(picParam->y_mode_probs, keyFrameYModeProbs, 4);
        memcpy(picParam->uv_mode_probs, keyFrameUVModeProbs, 3);
        // prepare for next frame which may be not a key frame
        memcpy(m_yModeProbs, nonKeyFrameDefaultYModeProbs, 4);
        memcpy(m_uvModeProbs, nonKeyFrameDefaultUVModeProbs, 3);
    } else {
        if (m_frameHdr.intra_16x16_prob_update_flag) {
            memcpy(picParam->y_mode_probs, m_frameHdr.intra_16x16_prob, 4);
        } else {
            memcpy(picParam->y_mode_probs, m_yModeProbs, 4);
        }

        if (m_frameHdr.intra_chroma_prob_update_flag) {
            memcpy(picParam->uv_mode_probs, m_frameHdr.intra_chroma_prob,
                   3);
        } else {
            memcpy(picParam->uv_mode_probs, m_uvModeProbs, 3);
        }
    }

    memcpy(picParam->mv_probs,
           m_frameHdr.multi_frame_data->mv_prob_update.prob,
           sizeof picParam->mv_probs);

    picParam->bool_coder_ctx.range = m_frameHdr.rangedecoder_state.range;
    picParam->bool_coder_ctx.value =
        m_frameHdr.rangedecoder_state.code_word;
    picParam->bool_coder_ctx.count =
        m_frameHdr.rangedecoder_state.remaining_bits;


    picParamObj->unmap();

    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureQuantMatrix(VaapiPictureVP8 * pic)
{
    Vp8Segmentation *seg = &m_frameHdr.multi_frame_data->segmentation;
    VAIQMatrixBufferVP8 *iqMatrix;
    int32_t baseQI, i;

    iqMatrix = (VAIQMatrixBufferVP8 *) pic->m_iqMatrix->map();

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

    pic->m_iqMatrix->unmap();
    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureProbabilityTable(VaapiPictureVP8 * pic)
{
    VAProbabilityDataBufferVP8 *probTable;
    int32_t i;

    // XXX, create/render VAProbabilityDataBufferVP8 in base class
    probTable = (VAProbabilityDataBufferVP8 *) pic->m_probTable->map();

    memcpy(probTable->dct_coeff_probs,
           m_frameHdr.multi_frame_data->token_prob_update.coeff_prob,
           sizeof(probTable->dct_coeff_probs));

    pic->m_probTable->unmap();
    return true;
}

void VaapiDecoderVP8::updateReferencePictures()
{
    VaapiPictureVP8 *picture = m_currentPicture;

    // update picture reference
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        replacePicture(m_goldenRefPicture, picture);
        replacePicture(m_altRefPicture, picture);
    } else {
        // process refresh_alternate_frame/copy_buffer_to_alternate first
        if (m_frameHdr.refresh_alternate_frame) {
            replacePicture(m_altRefPicture, picture);
        } else {
            switch (m_frameHdr.copy_buffer_to_alternate) {
            case 0:
                // do nothing
                break;
            case 1:
                replacePicture(m_altRefPicture, m_lastPicture);
                break;
            case 2:
                replacePicture(m_altRefPicture, m_goldenRefPicture);
                break;
            default:
                WARNING
                    ("WARNING: VP8 decoder: unrecognized copy_buffer_to_alternate");
            }
        }

        if (m_frameHdr.refresh_golden_frame) {
            replacePicture(m_goldenRefPicture, picture);
        } else {
            switch (m_frameHdr.copy_buffer_to_golden) {
            case 0:
                // do nothing
                break;
            case 1:
                replacePicture(m_goldenRefPicture, m_lastPicture);
                break;
            case 2:
                replacePicture(m_goldenRefPicture, m_altRefPicture);
                break;
            default:
                WARNING
                    ("WARNING: VP8 decoder: unrecognized copy_buffer_to_golden");
            }
        }
    }
    if (m_frameHdr.key_frame == VP8_KEY_FRAME || m_frameHdr.refresh_last)
        replacePicture(m_lastPicture, picture);

    if (m_goldenRefPicture)
        DEBUG("m_goldenRefPicture: %p, m_surfaceID: %x",
              m_goldenRefPicture, m_goldenRefPicture->m_surfaceID);
    if (m_altRefPicture)
        DEBUG("m_altRefPicture: %p, m_surfaceID: %x", m_altRefPicture,
              m_altRefPicture->m_surfaceID);
    if (m_lastPicture)
        DEBUG("m_lastPicture: %p, m_surfaceID: %x", m_lastPicture,
              m_lastPicture->m_surfaceID);
    if (m_currentPicture)
        DEBUG("m_currentPicture: %p, m_surfaceID: %x", m_currentPicture,
              m_currentPicture->m_surfaceID);
    int i;
    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        if (m_pictures[i])
            DEBUG("m_pictures[%d]: %p, m_surfaceID: %x", i, m_pictures[i],
                  m_pictures[i]->m_surfaceID);
    }

}

bool VaapiDecoderVP8::allocNewPicture()
{
    int i;

    /* Create new picture */

    /*accquire one surface from m_bufPool in base decoder  */
    VaapiPictureVP8 *picture;
    picture = new VaapiPictureVP8(m_VADisplay,
                                  m_VAContext, m_bufPool,
                                  VAAPI_PICTURE_STRUCTURE_FRAME);
    VAAPI_PICTURE_FLAG_SET(picture, VAAPI_PICTURE_FLAG_FF);

    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        DEBUG("m_pictures[%d] = %p, surfaceID: %x", i, m_pictures[i],
              m_pictures[i] ? m_pictures[i]->m_surfaceID :
              VA_INVALID_SURFACE);
        if (m_pictures[i]
            && (m_pictures[i] == m_goldenRefPicture
                || m_pictures[i] == m_altRefPicture
                || m_pictures[i] == m_lastPicture
                || m_pictures[i] == m_currentPicture))
            continue;

        if (m_pictures[i]) {
            delete m_pictures[i];
        }

        m_pictures[i] = picture;
        break;
    }
    if (i == VP8_MAX_PICTURE_COUNT) {
        delete picture;
        return false;
    }
    replacePicture(m_currentPicture, picture);
    DEBUG
        ("i: %d, alloc new picture: %p with surface ID: %x, iq matrix buffer id: %x",
         i, m_currentPicture, m_currentPicture->m_surfaceID,
         m_currentPicture->m_iqMatrix->getID());

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

    VaapiSliceVP8 *slice;
    slice = new VaapiSliceVP8(m_VADisplay, m_VAContext,
#if __PSB_VP8_INTERFACE_WORK_AROUND__
                              // PSB requires slice_data_offset normalize to 0 and macroblock_offset normalized to [0,7]
                              m_frameHdr.rangedecoder_state.buffer,
                              m_frameSize -
                              (m_frameHdr.rangedecoder_state.buffer -
                               m_buffer));
#else
                              m_buffer, m_frameSize);
#endif
    m_currentPicture->addSlice(slice);
    if (!fillSliceParam(slice)) {
        ERROR("failed to fill slice parameters");
        return DECODE_FAIL;
    }

    if (!m_currentPicture->decodePicture())
        return DECODE_FAIL;

    DEBUG("VaapiDecoderVP8::decodePicture success");
    return status;
}

VaapiDecoderVP8::VaapiDecoderVP8(const char *mimeType)
:  VaapiDecoderBase(mimeType)
{
    int32_t i;

    m_currentPicture = NULL;
    m_altRefPicture = NULL;
    m_goldenRefPicture = NULL;
    m_lastPicture = NULL;
    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        m_pictures[i] = NULL;
    }

    m_frameWidth = 0;
    m_frameHeight = 0;
    m_buffer = 0;
    m_frameSize = 0;
    memset(&m_frameHdr, 0, sizeof(Vp8FrameHdr));
    memset(&m_lastFrameContext, 0, sizeof(Vp8MultiFrameData));
    memset(&m_currFrameContext, 0, sizeof(Vp8MultiFrameData));

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

    vp8_parse_init_default_multi_frame_data(&m_lastFrameContext);

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
    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        if (m_pictures[i]) {
            delete m_pictures[i];
            m_pictures[i] = NULL;
        }
    }

    VaapiDecoderBase::stop();
}

void VaapiDecoderVP8::flush(void)
{
    DEBUG("VP8: flush()");
    VaapiDecoderBase::flush();
}

Decode_Status VaapiDecoderVP8::decode(VideoDecodeBuffer * buffer)
{
    Decode_Status status;
    Vp8ParseResult result;
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
        m_frameHdr.multi_frame_data = &m_currFrameContext;
        result =
            vp8_parse_frame_header(&m_frameHdr, m_buffer, 0, m_frameSize);
        status = getStatus(result);
        if (status != DECODE_SUCCESS) {
            break;
        }

        if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
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
            m_currentPicture->output();
        } else {
            WARNING("warning: this picture isn't sent to render");
        }

        updateReferencePictures();

        if (m_frameHdr.refresh_entropy_probs) {
            memcpy(&m_lastFrameContext.token_prob_update,
                   &m_currFrameContext.token_prob_update,
                   sizeof(Vp8TokenProbUpdate));
            memcpy(&m_lastFrameContext.mv_prob_update,
                   &m_currFrameContext.mv_prob_update,
                   sizeof(Vp8MvProbUpdate));

            if (m_frameHdr.intra_16x16_prob_update_flag)
                memcpy(m_yModeProbs, m_frameHdr.intra_16x16_prob, 4);
            if (m_frameHdr.intra_chroma_prob_update_flag)
                memcpy(m_uvModeProbs, m_frameHdr.intra_chroma_prob, 3);
        } else {
            memcpy(&m_currFrameContext.token_prob_update,
                   &m_lastFrameContext.token_prob_update,
                   sizeof(Vp8TokenProbUpdate));
            memcpy(&m_currFrameContext.mv_prob_update,
                   &m_lastFrameContext.mv_prob_update,
                   sizeof(Vp8MvProbUpdate));
        }

    } while (0);

    if (status != DECODE_SUCCESS) {
        DEBUG("decode fail!!");
    }

    return status;
}
