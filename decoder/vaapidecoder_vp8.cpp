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


#include "vaapidecoder_vp8.h"
#include "codecparsers/bytereader.h"

// the following parameter apply to Intra-Predicted Macroblocks,
// $11.2 $11.4: key frame default probs
static const uint8 kf_y_mode_probs[4] = { 145, 156, 163, 128 };
static const uint8 kf_uv_mode_probs[3] = { 142, 114, 183 };

// $16.1: non-key frame default probs
static const uint8 nk_default_y_mode_probs[4] = { 112, 86, 140, 37 };
static const uint8 nk_default_uv_mode_probs[3] = { 162, 101, 204 };

// XXX, move it to VaapiDecoderBase
bool VaapiDecoderVP8::replacePicture (VaapiPictureVP8 ** pic1,
    VaapiPicture * pic2)
{
#if 0
    if (!*pic1)
        return false;
    delete *
        pic1;
#endif
    // XX increase ref count for pic2
    *pic1 = pic2;

    return true;
}

static Decode_Status
get_status (Vp8ParseResult result)
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


VaapiSliceVP8::VaapiSliceVP8 (VADisplay display,
    VAContextID ctx, uint8_t * sliceData, uint32_t sliceSize)
{
    VASliceParameterBufferVP8 *paramBuf;

    if (!display || !ctx) {
        ERROR ("VA display or context not initialized yet");
        return;
    }

    /* new vp8 slice data buffer */
    mData = new VaapiBufObject (display,
        ctx, VASliceDataBufferType, sliceData, sliceSize);

    /* new vp8 slice parameter buffer */
    mParam = new VaapiBufObject (display,
        ctx,
        VASliceParameterBufferType, NULL, sizeof (VASliceParameterBufferVP8));

    paramBuf = (VASliceParameterBufferVP8 *) mParam->map ();

    paramBuf->slice_data_size = sliceSize;
    paramBuf->slice_data_offset = 0;
    paramBuf->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    mParam->unmap ();
}

VaapiSliceVP8::~VaapiSliceVP8 ()
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

VaapiPictureVP8::VaapiPictureVP8 (VADisplay display,
    VAContextID context,
    VaapiSurfaceBufferPool * surfBufPool, VaapiPictureStructure structure)
:  VaapiPicture (display, context, surfBufPool, structure)
{
    structure = VAAPI_PICTURE_STRUCTURE_FRAME;

    /* new vp8 slice parameter buffer */
    mPicParam = new VaapiBufObject (display,
        context,
        VAPictureParameterBufferType,
        NULL, sizeof (VAPictureParameterBufferVP8));
    if (!mPicParam)
        ERROR ("create vp8 picture parameter buffer object failed");

    mIqMatrix = new VaapiBufObject (display,
        context, VAIQMatrixBufferType, NULL, sizeof (VAIQMatrixBufferVP8));
    if (!mIqMatrix)
        ERROR ("create vp8 iq matrix buffer object failed");

    mProbTable = new VaapiBufObject (display,
        context,
        VAProbabilityBufferType, NULL, sizeof (VAProbabilityDataBufferVP8));
    if (!mProbTable)
        ERROR ("create vp8 probability table buffer object failed");

}

/////////////////////////////////////////////////////


Decode_Status VaapiDecoderVP8::ensureContext ()
{
    if (m_hasContext)
        return DECODE_SUCCESS;

    // XXX, redudant to set surfaceNumber
    mConfigBuffer.surfaceNumber = 3 + VP8_EXTRA_SURFACE_NUMBER;
    VaapiDecoderBase::start (&mConfigBuffer);
    DEBUG ("First time to Start VA context");
    m_hasContext = true;

    return DECODE_SUCCESS;
}

bool VaapiDecoderVP8::fillSliceParam (VaapiSliceVP8 * slice)
{
    VaapiBufObject *
        slice_param_obj = slice->mParam;
    VASliceParameterBufferVP8 *
        slice_param = (VASliceParameterBufferVP8 *) slice_param_obj->map ();
    int32 last_partition_size, i;


    if (m_frameHdr.key_frame == VP8_KEY_FRAME)
        slice_param->slice_data_offset = VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
        slice_param->slice_data_offset = VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;

    // XXX, the buf start address m_buffer
    slice_param->macroblock_offset =
        (m_frameHdr.rangedecoder_state.buffer - m_buffer -
        slice_param->slice_data_offset) * 8 -
        m_frameHdr.rangedecoder_state.remaining_bits;
    slice_param->num_of_partitions = (1 << m_frameHdr.log2_nbr_of_dct_partitions) + 1;  // +1 for the frame header partition

    // first_part_size doesn't include the uncompress data(frame-tage/magic-number/frame-width/height) at the begining of the frame.
    // partition_size[0] refer to 'first_part_size - parsed-bytes-by-range-decoder'
    slice_param->partition_size[0] =
        m_frameHdr.first_part_size - ((slice_param->macroblock_offset +
            7) >> 3);

    if (m_frameHdr.key_frame == VP8_KEY_FRAME)
        last_partition_size =
            m_frameSize - VP8_UNCOMPRESSED_DATA_SIZE_KEY_FRAME;
    else
        last_partition_size =
            m_frameSize - VP8_UNCOMPRESSED_DATA_SIZE_NON_KEY_FRAME;

    last_partition_size -= m_frameHdr.first_part_size;

    for (i = 1; i < slice_param->num_of_partitions - 1; i++) {
        slice_param->partition_size[i] = m_frameHdr.partition_size[i - 1];
        last_partition_size -= (m_frameHdr.partition_size[i - 1] + 3);
    }
    slice_param->partition_size[slice_param->num_of_partitions - 1] =
        last_partition_size;

    slice_param_obj->unmap ();
    return true;
}

bool VaapiDecoderVP8::fillPictureParam (VaapiPictureVP8 * picture)
{
    int32 i, n;
    VaapiBufObject * pic_param_obj = picture->mPicParam;

    VAPictureParameterBufferVP8 *
        pic_param = (VAPictureParameterBufferVP8 *) pic_param_obj->map ();

    /* Fill in VAPictureParameterBufferVP8 */
    VASliceParameterBufferVP8 *slice_param = NULL;
    Vp8Segmentation *
        seg = &m_frameHdr.multi_frame_data->segmentation;

    memset (pic_param, 0, sizeof (*pic_param));
    /* Fill in VAPictureParameterBufferVP8 */
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        m_frameWidth = m_frameHdr.width;
        m_frameHeight = m_frameHdr.height;
        if (m_frameHdr.horizontal_scale || m_frameHdr.vertical_scale)
            WARNING
                ("horizontal_scale or vertical_scale in VP8 isn't supported yet");
    }
    // XXX, we don't support horizontal_scale or vertical_scale yet.
    // reject frames for upscale, simple accept the downscale frames
    if (m_frameWidth > mVideoFormatInfo.width
        || m_frameHeight > mVideoFormatInfo.height)
        return FALSE;

    pic_param->frame_width = m_frameWidth;
    pic_param->frame_height = m_frameHeight;
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        pic_param->last_ref_frame = VA_INVALID_SURFACE;
        pic_param->golden_ref_frame = VA_INVALID_SURFACE;
        pic_param->alt_ref_frame = VA_INVALID_SURFACE;
    } else {
        pic_param->last_ref_frame =
            m_lastPicture ? m_lastPicture->mSurfaceID : VA_INVALID_SURFACE;
        pic_param->golden_ref_frame =
            m_goldenRefPicture ? m_goldenRefPicture->
            mSurfaceID : VA_INVALID_SURFACE;
        pic_param->alt_ref_frame =
            m_altRefPicture ? m_altRefPicture->mSurfaceID : VA_INVALID_SURFACE;
    }
    pic_param->out_of_loop_frame = VA_INVALID_SURFACE;  // not used currently

    pic_param->pic_fields.bits.key_frame = m_frameHdr.key_frame;
    pic_param->pic_fields.bits.version = m_frameHdr.version;
    pic_param->pic_fields.bits.segmentation_enabled = seg->segmentation_enabled;
    pic_param->pic_fields.bits.update_mb_segmentation_map =
        seg->update_mb_segmentation_map;
    pic_param->pic_fields.bits.update_segment_feature_data =
        seg->update_segment_feature_data;
    pic_param->pic_fields.bits.filter_type = m_frameHdr.filter_type;
    pic_param->pic_fields.bits.sharpness_level = m_frameHdr.sharpness_level;
    pic_param->pic_fields.bits.loop_filter_adj_enable =
        m_frameHdr.multi_frame_data->mb_lf_adjust.loop_filter_adj_enable;
    pic_param->pic_fields.bits.mode_ref_lf_delta_update =
        m_frameHdr.multi_frame_data->mb_lf_adjust.mode_ref_lf_delta_update;
    pic_param->pic_fields.bits.sign_bias_golden = m_frameHdr.sign_bias_golden;
    pic_param->pic_fields.bits.sign_bias_alternate =
        m_frameHdr.sign_bias_alternate;
    pic_param->pic_fields.bits.mb_no_coeff_skip = m_frameHdr.mb_no_skip_coeff;

    for (i = 0; i < 3; i++) {
        pic_param->mb_segment_tree_probs[i] = seg->segment_prob[i];
    }

    for (i = 0; i < 4; i++) {
        if (seg->segmentation_enabled) {
            pic_param->loop_filter_level[i] = seg->lf_update_value[i];
            if (!seg->segment_feature_mode)
                pic_param->loop_filter_level[i] += m_frameHdr.loop_filter_level;
        } else
            pic_param->loop_filter_level[i] = m_frameHdr.loop_filter_level;

        pic_param->loop_filter_deltas_ref_frame[i] =
            m_frameHdr.multi_frame_data->mb_lf_adjust.ref_frame_delta[i];
        pic_param->loop_filter_deltas_mode[i] =
            m_frameHdr.multi_frame_data->mb_lf_adjust.mb_mode_delta[i];
    }
    if ((pic_param->pic_fields.bits.version == 0)
        || (pic_param->pic_fields.bits.version == 1)) {
        pic_param->pic_fields.bits.loop_filter_disable =
            pic_param->loop_filter_level[0] == 0;
    }

    pic_param->prob_skip_false = m_frameHdr.prob_skip_false;
    pic_param->prob_intra = m_frameHdr.prob_intra;
    pic_param->prob_last = m_frameHdr.prob_last;
    pic_param->prob_gf = m_frameHdr.prob_gf;

    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        // key frame use fixed prob table
        memcpy (pic_param->y_mode_probs, kf_y_mode_probs, 4);
        memcpy (pic_param->uv_mode_probs, kf_uv_mode_probs, 3);
        // prepare for next frame which may be not a key frame
        memcpy (m_yModeProbs, nk_default_y_mode_probs, 4);
        memcpy (m_uvModeProbs, nk_default_uv_mode_probs, 3);
    } else {
        if (m_frameHdr.intra_16x16_prob_update_flag) {
            memcpy (pic_param->y_mode_probs, m_frameHdr.intra_16x16_prob, 4);
        } else {
            memcpy (pic_param->y_mode_probs, m_yModeProbs, 4);
        }

        if (m_frameHdr.intra_chroma_prob_update_flag) {
            memcpy (pic_param->uv_mode_probs, m_frameHdr.intra_chroma_prob, 3);
        } else {
            memcpy (pic_param->uv_mode_probs, m_uvModeProbs, 3);
        }
    }

    memcpy (pic_param->mv_probs,
        m_frameHdr.multi_frame_data->mv_prob_update.prob,
        sizeof pic_param->mv_probs);

    pic_param->bool_coder_ctx.range = m_frameHdr.rangedecoder_state.range;
    pic_param->bool_coder_ctx.value = m_frameHdr.rangedecoder_state.code_word;
    pic_param->bool_coder_ctx.count =
        m_frameHdr.rangedecoder_state.remaining_bits;


    pic_param_obj->unmap ();

    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensureQuantMatrix (VaapiPictureVP8 * pic)
{
    Vp8Segmentation *
        seg = &m_frameHdr.multi_frame_data->segmentation;
    VAIQMatrixBufferVP8 *iq_matrix;
    int32 base_qi, i;

    iq_matrix = (VAIQMatrixBufferVP8 *) pic->mIqMatrix->map ();

    for (i = 0; i < 4; i++) {
        int32 temp_index;
        const int32 MAX_QI_INDEX = 127;
        if (seg->segmentation_enabled) {
            base_qi = seg->quantizer_update_value[i];
            if (!seg->segment_feature_mode)     // 0 means delta update
                base_qi += m_frameHdr.quant_indices.y_ac_qi;;
        } else
            base_qi = m_frameHdr.quant_indices.y_ac_qi;

        // the first component is y_ac_qi
        temp_index =
            base_qi < 0 ? 0 : (base_qi > MAX_QI_INDEX ? MAX_QI_INDEX : base_qi);
        iq_matrix->quantization_index[i][0] = temp_index;

        temp_index = base_qi + m_frameHdr.quant_indices.y_dc_delta;
        temp_index =
            temp_index < 0 ? 0 : (temp_index >
            MAX_QI_INDEX ? MAX_QI_INDEX : temp_index);
        iq_matrix->quantization_index[i][1] = temp_index;

        temp_index = base_qi + m_frameHdr.quant_indices.y2_dc_delta;
        temp_index =
            temp_index < 0 ? 0 : (temp_index >
            MAX_QI_INDEX ? MAX_QI_INDEX : temp_index);
        iq_matrix->quantization_index[i][2] = temp_index;

        temp_index = base_qi + m_frameHdr.quant_indices.y2_ac_delta;
        temp_index =
            temp_index < 0 ? 0 : (temp_index >
            MAX_QI_INDEX ? MAX_QI_INDEX : temp_index);
        iq_matrix->quantization_index[i][3] = temp_index;

        temp_index = base_qi + m_frameHdr.quant_indices.uv_dc_delta;
        temp_index =
            temp_index < 0 ? 0 : (temp_index >
            MAX_QI_INDEX ? MAX_QI_INDEX : temp_index);
        iq_matrix->quantization_index[i][4] = temp_index;

        temp_index = base_qi + m_frameHdr.quant_indices.uv_ac_delta;
        temp_index =
            temp_index < 0 ? 0 : (temp_index >
            MAX_QI_INDEX ? MAX_QI_INDEX : temp_index);
        iq_matrix->quantization_index[i][5] = temp_index;
    }

    pic->mIqMatrix->unmap ();
    return true;
}

/* fill quant parameter buffers functions*/
bool VaapiDecoderVP8::ensure_probability_table (VaapiPictureVP8 * pic)
{
    VAProbabilityDataBufferVP8 * prob_table;
    int32 i;

    // XXX, create/render VAProbabilityDataBufferVP8 in base class
    prob_table = (VAProbabilityDataBufferVP8 *) pic->mProbTable->map ();

    memcpy (prob_table->dct_coeff_probs,
        m_frameHdr.multi_frame_data->token_prob_update.coeff_prob,
        sizeof (prob_table->dct_coeff_probs));

    pic->mProbTable->unmap ();
    return true;
}

void
VaapiDecoderVP8::update_reference_pictures ()
{
    VaapiPicture *picture = m_currentPicture;

    // update picture reference
    if (m_frameHdr.key_frame == VP8_KEY_FRAME) {
        replacePicture (&m_goldenRefPicture, picture);
        replacePicture (&m_altRefPicture, picture);
    } else {
        // process refresh_alternate_frame/copy_buffer_to_alternate first
        if (m_frameHdr.refresh_alternate_frame) {
            replacePicture (&m_altRefPicture, picture);
        } else {
            switch (m_frameHdr.copy_buffer_to_alternate) {
              case 0:
                  // do nothing
                  break;
              case 1:
                  replacePicture (&m_altRefPicture, m_lastPicture);
                  break;
              case 2:
                  replacePicture (&m_altRefPicture, m_goldenRefPicture);
                  break;
              default:
                  WARNING
                      ("WARNING: VP8 decoder: unrecognized copy_buffer_to_alternate");
            }
        }

        if (m_frameHdr.refresh_golden_frame) {
            replacePicture (&m_goldenRefPicture, picture);
        } else {
            switch (m_frameHdr.copy_buffer_to_golden) {
              case 0:
                  // do nothing
                  break;
              case 1:
                  replacePicture (&m_goldenRefPicture, m_lastPicture);
                  break;
              case 2:
                  replacePicture (&m_goldenRefPicture, m_altRefPicture);
                  break;
              default:
                  WARNING
                      ("WARNING: VP8 decoder: unrecognized copy_buffer_to_golden");
            }
        }
    }
    if (m_frameHdr.key_frame == VP8_KEY_FRAME || m_frameHdr.refresh_last)
        replacePicture (&m_lastPicture, picture);

}

bool VaapiDecoderVP8::alloc_new_picture ()
{
    int i;

    /* Create new picture */

    /*accquire one surface from mBufPool in base decoder  */
    VaapiPictureVP8 *picture;
    picture = new VaapiPictureVP8 (mVADisplay,
        mVAContext, mBufPool, VAAPI_PICTURE_STRUCTURE_FRAME);
    VAAPI_PICTURE_FLAG_SET (picture, VAAPI_PICTURE_FLAG_FF);

    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        DEBUG("m_pictures[%d] = %p", i, m_pictures[i]);
        if (m_pictures[i] && (m_pictures[i] == m_goldenRefPicture
                           || m_pictures[i] == m_altRefPicture
                           || m_pictures[i] == m_lastPicture
                           || m_pictures[i] == m_currentPicture)) // take m_currentPicture as a buffering area
            continue;

        if (m_pictures[i])
            delete m_pictures[i];
        m_pictures[i] = picture;
        break;
    }
    if (i == VP8_MAX_PICTURE_COUNT)
        return false;
    replacePicture (&m_currentPicture, picture);

    return true;
}

Decode_Status VaapiDecoderVP8::decodePicture ()
{
    Decode_Status status = DECODE_SUCCESS;

    status = ensureContext ();
    if (status != DECODE_SUCCESS)
        return status;

    if (!alloc_new_picture ())
        return DECODE_FAIL;

    if (!ensureQuantMatrix (m_currentPicture)) {
        ERROR ("failed to reset quantizer matrix");
        return DECODE_FAIL;
    }

    if (!ensure_probability_table (m_currentPicture)) {
        ERROR ("failed to reset probability table");
        return DECODE_FAIL;
    }

    if (!fillPictureParam (m_currentPicture)) {
        ERROR ("failed to fill picture parameters");
        return DECODE_FAIL;
    }

    VaapiSliceVP8 *slice;
    slice = new VaapiSliceVP8 (mVADisplay, mVAContext, m_buffer, m_frameSize);
    m_currentPicture->addSlice (slice);
    if (!fillSliceParam (slice)) {
        ERROR ("failed to fill slice parameters");
        return DECODE_FAIL;
    }

    if (!m_currentPicture->decodePicture ())
        return DECODE_FAIL;

    return status;
}

VaapiDecoderVP8::VaapiDecoderVP8 (const char *mimeType)
:  VaapiDecoderBase (mimeType)
{
    int32 i;

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
    memset (&m_frameHdr, 0, sizeof (Vp8FrameHdr));
    memset (&m_lastFrameContext, 0, sizeof (Vp8MultiFrameData));
    memset (&m_currFrameContext, 0, sizeof (Vp8MultiFrameData));

    // m_yModeProbs[4];
    // m_uvModeProbs[3];
    m_sizeChanged = 0;
    m_hasContext = false;
}

VaapiDecoderVP8::~VaapiDecoderVP8 ()
{
    stop ();
}


Decode_Status VaapiDecoderVP8::start (VideoConfigBuffer * buffer)
{
    DEBUG ("VP8: start()");
    Decode_Status status;
    bool got_config = false;

    if ((buffer->flag & HAS_SURFACE_NUMBER) && (buffer->flag & HAS_VA_PROFILE)) {
        got_config = true;
    }

    buffer->profile = VAProfileVP8Version0_3;
    buffer->surfaceNumber = 3 + VP8_EXTRA_SURFACE_NUMBER;
    got_config = true;

    vp8_parse_init_default_multi_frame_data (&m_lastFrameContext);

    // XXX
    buffer->graphicBufferWidth = buffer->width;
    buffer->graphicBufferHeight = buffer->height;
    buffer->flag &= ~USE_NATIVE_GRAPHIC_BUFFER;
    if (got_config) {
        VaapiDecoderBase::start (buffer);
        m_hasContext = true;
    }

    return DECODE_FORMAT_CHANGE; // notify up layer for necessary re-configuration
}

Decode_Status VaapiDecoderVP8::reset (VideoConfigBuffer * buffer)
{
    DEBUG ("VP8: reset()");
    return VaapiDecoderBase::reset (buffer);
}

void
VaapiDecoderVP8::stop (void)
{
    int i;
    DEBUG ("VP8: stop()");
    flush ();
    // XXX, is it possible that the picture is still displaying?
    // m_lastPicture = NULL;
    // m_currentPicture = NULL;
    // XXX, should we use delete?
    // replacePicture (&m_goldenRefPicture , NULL);
    // replacePicture (&m_altRefPicture, NULL);
    for (i = 0; i < VP8_MAX_PICTURE_COUNT; i++) {
        if (m_pictures[i]) {
            delete m_pictures[i];
            m_pictures[i] = NULL;
        }
    }

    VaapiDecoderBase::stop ();
}

void
VaapiDecoderVP8::flush (void)
{
    DEBUG ("VP8: flush()");
    VaapiDecoderBase::flush ();
}

Decode_Status VaapiDecoderVP8::decode (VideoDecodeBuffer * buffer)
{
    Decode_Status status;
    Vp8ParseResult result;
    bool is_eos = false;

    mCurrentPTS = buffer->timeStamp;

    m_buffer = buffer->data;
    m_frameSize = buffer->size;

    DEBUG ("VP8: Decode(bufsize =%d, timestamp=%ld)", m_frameSize, mCurrentPTS);

    do {
        if (m_frameSize == 0) {
            status = DECODE_FAIL;
            break;
        }

        memset (&m_frameHdr, 0, sizeof (m_frameHdr));
        m_frameHdr.multi_frame_data = &m_currFrameContext;
        result = vp8_parse_frame_header (&m_frameHdr, m_buffer, 0, m_frameSize);
        status = get_status (result);
        if (status != DECODE_SUCCESS) {
            break;
        }

        status = decodePicture ();
        if (status != DECODE_SUCCESS)
            break;

        if (m_frameHdr.show_frame) {
            m_currentPicture->mTimeStamp = mCurrentPTS;
            m_currentPicture->output ();
        }

        update_reference_pictures ();

        if (m_frameHdr.refresh_entropy_probs) {
            memcpy (&m_lastFrameContext.token_prob_update,
                &m_currFrameContext.token_prob_update,
                sizeof (Vp8TokenProbUpdate));
            memcpy (&m_lastFrameContext.mv_prob_update,
                &m_currFrameContext.mv_prob_update, sizeof (Vp8MvProbUpdate));

            if (m_frameHdr.intra_16x16_prob_update_flag)
                memcpy (m_yModeProbs, m_frameHdr.intra_16x16_prob, 4);
            if (m_frameHdr.intra_chroma_prob_update_flag)
                memcpy (m_uvModeProbs, m_frameHdr.intra_chroma_prob, 3);
        } else {
            memcpy (&m_currFrameContext.token_prob_update,
                &m_lastFrameContext.token_prob_update,
                sizeof (Vp8TokenProbUpdate));
            memcpy (&m_currFrameContext.mv_prob_update,
                &m_lastFrameContext.mv_prob_update, sizeof (Vp8MvProbUpdate));
        }

    } while (0);

    if (status != DECODE_SUCCESS) {
        DEBUG("decode fail!!");
    }

    return status;
}
