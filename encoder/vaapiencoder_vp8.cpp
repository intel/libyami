/*
 *  vaapiencoder_vp8.cpp - vp8 encoder for va
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Xu Guangxin <guangxin.xu@intel.com>
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
#include "vaapiencoder_vp8.h"
#include "scopedlogger.h"
#include "vaapi/vaapicontext.h"
#include "vaapi/vaapidisplay.h"
#include "vaapicodedbuffer.h"
#include "vaapiencpicture.h"
#include <algorithm>

namespace YamiMediaCodec{

//golden, alter, last
#define MAX_REFERECNE_FRAME 3

class VaapiEncPictureVP8 : public VaapiEncPicture
{
public:
    VAGenericID getCodedBufferID()
    {
        return m_codedBuffer->getID();
    }
};

VaapiEncoderVP8::VaapiEncoderVP8():m_frameCount(0)
{
    m_videoParamCommon.profile = VAProfileVP8Version0_3;
}

VaapiEncoderVP8::~VaapiEncoderVP8()
{
}

Encode_Status VaapiEncoderVP8::getMaxOutSize(uint32_t *maxSize)
{
    FUNC_ENTER();
    *maxSize = m_maxCodedbufSize;
    return ENCODE_SUCCESS;
}

void VaapiEncoderVP8::resetParams()
{
    //5 times compress ratio
    m_maxCodedbufSize = width() * height() * 3 / 2 / 5;
}

Encode_Status VaapiEncoderVP8::start()
{
    FUNC_ENTER();
    resetParams();
    return VaapiEncoderBase::start();
}

void VaapiEncoderVP8::flush()
{
    FUNC_ENTER();
    m_frameCount = 0;
    m_reference.clear();
    VaapiEncoderBase::flush();
}

Encode_Status VaapiEncoderVP8::stop()
{
    flush();
    return VaapiEncoderBase::stop();
}

Encode_Status VaapiEncoderVP8::setParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    Encode_Status status = ENCODE_SUCCESS;
    FUNC_ENTER();
    if (!videoEncParams)
        return ENCODE_INVALID_PARAMS;

    switch (type) {
    default:
        status = VaapiEncoderBase::setParameters(type, videoEncParams);
        break;
    }
    return status;
}

Encode_Status VaapiEncoderVP8::getParameters(VideoParamConfigType type, Yami_PTR videoEncParams)
{
    FUNC_ENTER();
    if (!videoEncParams)
        return ENCODE_INVALID_PARAMS;

    // TODO, update video resolution basing on hw requirement
    return VaapiEncoderBase::getParameters(type, videoEncParams);
}

Encode_Status VaapiEncoderVP8::doEncode(const SurfacePtr& surface, uint64_t timeStamp, bool forceKeyFrame)
{
    Encode_Status ret;
    if (!surface)
        return ENCODE_INVALID_PARAMS;

    PicturePtr picture(new VaapiEncPicture(m_context, surface, timeStamp));

    m_frameCount %= keyFramePeriod();
    picture->m_type = (m_frameCount ? VAAPI_PICTURE_TYPE_P : VAAPI_PICTURE_TYPE_I);
    m_frameCount++;

    CodedBufferPtr codedBuffer = VaapiCodedBuffer::create(m_context, m_maxCodedbufSize);
    if (!codedBuffer)
        return ENCODE_NO_MEMORY;
    picture->m_codedBuffer = codedBuffer;
    codedBuffer->setFlag(ENCODE_BUFFERFLAG_ENDOFFRAME);
    INFO("picture->m_type: 0x%x\n", picture->m_type);
    if (picture->m_type == VAAPI_PICTURE_TYPE_I) {
        codedBuffer->setFlag(ENCODE_BUFFERFLAG_SYNCFRAME);
    }
    ret = encodePicture(picture);
    if (ret != ENCODE_SUCCESS) {
        return ret;
    }
    output(picture);
    return ENCODE_SUCCESS;
}


bool VaapiEncoderVP8::fill(VAEncSequenceParameterBufferVP8* seqParam) const
{
    seqParam->frame_width = width();
    seqParam->frame_height = height();
    /* TODO: how to set this?
    seqParam->bits_per_second =
    */
    seqParam->bits_per_second = seqParam->frame_width * seqParam->frame_height * 3 / 2 * 30 * 8 / 80;
    seqParam->intra_period = intraPeriod();
    return true;
}

/* Fills in VA picture parameter buffer */
bool VaapiEncoderVP8::fill(VAEncPictureParameterBufferVP8* picParam, const PicturePtr& picture,
                           const SurfacePtr& surface) const
{
    picParam->reconstructed_frame = surface->getID();
    if (picture->m_type == VAAPI_PICTURE_TYPE_P) {
        picParam->pic_flags.bits.frame_type = 1;
        ReferenceQueue::const_iterator it = m_reference.begin();
        picParam->ref_arf_frame = (*it++)->getID();
        picParam->ref_gf_frame = (*it++)->getID();
        picParam->ref_last_frame = (*it)->getID();
        picParam->pic_flags.bits.refresh_last = 1;
        picParam->pic_flags.bits.refresh_golden_frame = 0;
        picParam->pic_flags.bits.copy_buffer_to_golden = 1;
        picParam->pic_flags.bits.refresh_alternate_frame = 0;
        picParam->pic_flags.bits.copy_buffer_to_alternate = 2;
    } else {
        picParam->ref_last_frame = VA_INVALID_SURFACE;
        picParam->ref_gf_frame = VA_INVALID_SURFACE;
        picParam->ref_arf_frame = VA_INVALID_SURFACE;
    }

    picParam->coded_buf = picture->getCodedBufferID();

    picParam->pic_flags.bits.show_frame = 1;
    /*TODO: multi partition*/
    picParam->pic_flags.bits.num_token_partitions = 0;
    //REMOVE THIS
    picParam->pic_flags.bits.refresh_entropy_probs = 0;
    /*pic_flags end */
    for (int i = 0; i < 4; i++) {
        picParam->loop_filter_level[i] = 19;
    }


    return TRUE;
}

bool VaapiEncoderVP8::fill(VAQMatrixBufferVP8* qMatrix) const
{
    //TODO: FIX THIS
    for (int i = 0; i < 4; i++) {
        qMatrix->quantization_index[i] = 40;
    }

    return true;
}


bool VaapiEncoderVP8::ensureSequence(const PicturePtr& picture)
{
    if (picture->m_type != VAAPI_PICTURE_TYPE_I)
        return true;

    VAEncSequenceParameterBufferVP8* seqParam;
    if (!picture->editSequence(seqParam) || !fill(seqParam)) {
        ERROR("failed to create sequence parameter buffer (SPS)");
        return false;
    }
    return true;
}

bool VaapiEncoderVP8::ensurePicture (const PicturePtr& picture,
                                     const SurfacePtr& surface)
{
    VAEncPictureParameterBufferVP8 *picParam;

    if (!picture->editPicture(picParam) || !fill(picParam, picture, surface)) {
        ERROR("failed to create picture parameter buffer (PPS)");
        return false;
    }
    return true;
}

bool VaapiEncoderVP8::ensureQMatrix (const PicturePtr& picture)
{
    VAQMatrixBufferVP8 *qMatrix;

    if (!picture->editQMatrix(qMatrix) || !fill(qMatrix)) {
        ERROR("failed to create qMatrix");
        return false;
    }
    return true;
}

bool VaapiEncoderVP8::referenceListUpdate (const PicturePtr& pic, const SurfacePtr& recon)
{

    if (pic->m_type == VAAPI_PICTURE_TYPE_I) {
        m_reference.clear();
        m_reference.insert(m_reference.end(), MAX_REFERECNE_FRAME, recon);
    } else {
        m_reference.pop_front();
        m_reference.push_back(recon);
    }

    return true;
}

Encode_Status VaapiEncoderVP8::encodePicture(const PicturePtr& picture)
{
    Encode_Status ret = ENCODE_FAIL;
    SurfacePtr reconstruct = createSurface();
    if (!reconstruct)
        return ret;

    if (!ensureSequence (picture))
        return ret;
/* TODO:
    if (!ensureMiscParams (picture.get()))
        return ret;
*/
    if (!ensurePicture(picture, reconstruct))
        return ret;

    if (!ensureQMatrix(picture))
        return ret;

    if (!picture->encode())
        return ret;

    if (!referenceListUpdate (picture, reconstruct))
        return ret;

    return ENCODE_SUCCESS;
}



}
