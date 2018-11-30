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

#include "codecparsers/bitReader.h"
#include "common/log.h"
#include "common/nalreader.h"
#include "vaapidecoder_mpeg2.h"
#include "vaapidecpicture.h"

#include <string.h>
#include <inttypes.h>

using namespace YamiParser::MPEG2;

namespace YamiMediaCodec {
typedef VaapiDecoderMPEG2::PicturePtr PicturePtr;

inline bool isFrame(VaapiPictureType type)
{
    return (type & PICTURE_STRUCTURE_MASK) == VAAPI_PICTURE_FRAME;
}

inline bool isField(VaapiPictureType type)
{
    return !isFrame(type);
}

inline bool isI(const PicturePtr& p)
{
    return (p->m_type & PICTURE_TYPE_MASK) == VAAPI_PICTURE_I;
}

inline bool isP(const PicturePtr& p)
{
    return (p->m_type & PICTURE_TYPE_MASK) == VAAPI_PICTURE_P;
}

inline bool isB(const PicturePtr& p)
{
    return (p->m_type & PICTURE_TYPE_MASK) == VAAPI_PICTURE_B;
}

inline bool isFrame(const PicturePtr& p)
{
    return isFrame(p->m_type);
}

inline bool isField(const PicturePtr& picture)
{
    return !isFrame(picture);
}

void VaapiDecoderMPEG2::DPB::flush()
{
    if (!m_refs.empty()) {
        m_output(m_refs.back());
    }
    m_refs.clear();
    m_firstField.reset();
}

void VaapiDecoderMPEG2::DPB::addNewFrame(const PicturePtr& frame)
{
    if (isB(frame)) {
        m_output(frame);
        return;
    }
    if (!m_refs.empty()) {
        m_output(m_refs.back());
    }
    if (m_refs.size() == 2) {
        m_refs.pop_front();
    }
    DEBUG("add new frame to store before %d", (int)m_refs.size());
    m_refs.push_back(frame);
}

void VaapiDecoderMPEG2::DPB::add(const PicturePtr& picture)
{
    if (isField(picture) && !m_firstField) {
        m_firstField = picture;
    } else {
        m_firstField.reset();
        addNewFrame(picture);
    }
}

bool VaapiDecoderMPEG2::DPB::getReferences(const PicturePtr& current, PicturePtr& forward, PicturePtr& backward)
{
    if (isI(current))
        return true;
    if (isB(current) && m_refs.size() == 2) {
        backward = m_refs.back();
        forward = m_refs.front();
        return true;
    }
    if (!m_refs.empty()) {
        forward = m_refs.back();
        return true;
    }
    return true;
}

VaapiDecoderMPEG2::VaapiDecoderMPEG2()
    : m_dpb(std::bind(&VaapiDecoderMPEG2::outputPicture, this,
          std::placeholders::_1))
{
    m_parser.reset(new Parser());
    INFO("VaapiDecoderMPEG2 constructor");
}

VaapiDecoderMPEG2::~VaapiDecoderMPEG2() { stop(); }

VAProfile VaapiDecoderMPEG2::convertToVAProfile(
    YamiParser::MPEG2::ProfileType profile)
{
    VAProfile vaProfile;

    switch (profile) {
    case YamiParser::MPEG2::MPEG2_PROFILE_MAIN:
        vaProfile = VAProfileMPEG2Main;
        break;
    case YamiParser::MPEG2::MPEG2_PROFILE_SIMPLE:
        vaProfile = VAProfileMPEG2Simple;
        break;
    default:
        vaProfile = VAProfileNone;
        break;
    }
    return vaProfile;
}

YamiStatus
VaapiDecoderMPEG2::checkLevel(YamiParser::MPEG2::LevelType level)
{
    switch (level) {
    case YamiParser::MPEG2::MPEG2_LEVEL_HIGH:
    case YamiParser::MPEG2::MPEG2_LEVEL_HIGH_1440:
    case YamiParser::MPEG2::MPEG2_LEVEL_MAIN:
    case YamiParser::MPEG2::MPEG2_LEVEL_LOW:
        return YAMI_SUCCESS;
        break;
    default:
        return YAMI_UNSUPPORTED;
        break;
    }
}

YamiStatus VaapiDecoderMPEG2::start(VideoConfigBuffer* buffer)
{
    if (buffer)
        m_configBuffer = *buffer;
    return YAMI_SUCCESS;
}

void fillReference(VASurfaceID& surface, const PicturePtr& picture)
{
    if (picture)
        surface = picture->getSurfaceID();
    else
        surface = VA_INVALID_SURFACE;
}

VaapiPictureType getPictureType(const PictureHeader& picture, const PictureCodingExtension& extension)
{
    uint32_t type;
    if (picture.picture_coding_type == kIFrame)
        type = VAAPI_PICTURE_I;
    else if (picture.picture_coding_type == kPFrame)
        type = VAAPI_PICTURE_P;
    else
        type = VAAPI_PICTURE_B;

    if (extension.picture_structure == kTopField)
        type |= VAAPI_PICTURE_TOP_FIELD;
    else if (extension.picture_structure == kBottomField)
        type |= VAAPI_PICTURE_BOTTOM_FIELD;
    else
        type |= VAAPI_PICTURE_FRAME;
    return (VaapiPictureType)type;
}

SurfacePtr VaapiDecoderMPEG2::createSurface(VaapiPictureType type)
{
    SurfacePtr s;
    if (isField(type) && m_dpb.m_firstField) {
        //reuse surface from first field
        s = m_dpb.m_firstField->getSurface();
    } else {
        s = VaapiDecoderBase::createSurface();
    }
    if (s) {
        s->setCrop(0, 0, m_parser->getWidth(), m_parser->getHeight());
    }
    return s;
}

YamiStatus VaapiDecoderMPEG2::createPicture()
{
    VaapiPictureType type = getPictureType(m_parser->m_pictureHeader, m_parser->m_pictureCodingExtension);
    SurfacePtr s = createSurface(type);
    if (!s)
        return YAMI_DECODE_NO_SURFACE;

    m_current.reset(new VaapiDecPicture(m_context, s, m_currentPTS));
    m_current->m_type = type;
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::ensurePicture()
{
    YamiStatus ret = createPicture();
    if (ret != YAMI_SUCCESS)
        return ret;

    VAPictureParameterBufferMPEG2* param;
    if (!m_current->editPicture(param))
        return YAMI_FAIL;

    PicturePtr forward, backward;
    if (!m_dpb.getReferences(m_current, forward, backward)) {
        return YAMI_DECODE_INVALID_DATA;
    }
    param->horizontal_size = m_parser->getWidth();
    param->vertical_size = m_parser->getHeight();
    param->picture_coding_type = m_parser->m_pictureHeader.picture_coding_type;

    fillReference(param->forward_reference_picture, forward);
    fillReference(param->backward_reference_picture, backward);

    PictureCodingExtension& extension = m_parser->m_pictureCodingExtension;

    // libva packs all in one int.
    param->f_code = extension.getFCode();

#define FILL(f) param->picture_coding_extension.bits.f = extension.f
    FILL(intra_dc_precision);
    FILL(picture_structure);
    FILL(top_field_first);
    FILL(frame_pred_frame_dct);
    FILL(concealment_motion_vectors);
    FILL(q_scale_type);
    FILL(intra_vlc_format);
    FILL(alternate_scan);
    FILL(repeat_first_field);
    FILL(progressive_frame);
#undef FILL

    param->picture_coding_extension.bits.is_first_field = !m_dpb.m_firstField;

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::ensureSlice(const Slice& slice)
{
    if (!m_current) {
        ERROR("No frame for slice");
        return YAMI_DECODE_INVALID_DATA;
    }
    VASliceParameterBufferMPEG2* sliceParam;
    if (!m_current->newSlice(sliceParam,
            slice.sliceData, slice.sliceDataSize)) {
        DEBUG("picture->newSlice failed");
        return YAMI_FAIL;
    }
    sliceParam->macroblock_offset = slice.sliceHeaderSize;
    sliceParam->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    sliceParam->slice_horizontal_position = slice.macroblockColumn;
    sliceParam->slice_vertical_position = slice.macroblockRow;
    sliceParam->quantiser_scale_code = slice.quantiser_scale_code;
    sliceParam->intra_slice_flag = slice.intra_slice_flag;
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::decodeSlice(const DecodeUnit& du)
{
    YamiStatus status;

    Slice slice;
    if (!m_parser->parseSlice(slice, du))
        return YAMI_DECODE_PARSER_FAIL;
    if (slice.isFirstSlice()) {
        status = ensurePicture();
        if (status != YAMI_SUCCESS)
            return status;
    }
    return ensureSlice(slice);
}

bool VaapiDecoderMPEG2::ensureMatrices()
{
    if (!m_matrices)
        return true;

    SharedPtr<YamiParser::MPEG2::QuantMatrices> matrices;
    matrices.swap(m_matrices);

    VAIQMatrixBufferMPEG2* iqMatrix;
    if (!m_current->editIqMatrix(iqMatrix))
        return false;

#define FILL(m)                                   \
    do {                                          \
        if (matrices->load_##m) {                 \
            iqMatrix->load_##m = 1;               \
            memcpy(iqMatrix->m, matrices->m, 64); \
        }                                         \
    } while (0)

    FILL(intra_quantiser_matrix);
    FILL(non_intra_quantiser_matrix);
    FILL(chroma_intra_quantiser_matrix);
    FILL(chroma_non_intra_quantiser_matrix);
#undef FILL
    return true;
}

YamiStatus VaapiDecoderMPEG2::decodeCurrent()
{
    if (!m_current)
        return YAMI_SUCCESS;

    if (!ensureMatrices()) {
        return YAMI_FAIL;
    }

    PicturePtr picture;
    picture.swap(m_current);
    if (!picture->decode()) {
        DEBUG("picture->decode failed");
        return YAMI_FAIL;
    }
    m_dpb.add(picture);
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::outputPicture(const PicturePtr& picture)
{
    VaapiDecoderBase::PicturePtr basePicture
        = std::static_pointer_cast<VaapiDecPicture>(picture);

    return VaapiDecoderBase::outputPicture(basePicture);
}

YamiStatus VaapiDecoderMPEG2::decodeSequnce(const DecodeUnit& du)
{
    if (!m_parser->parseSequenceHeader(du, m_matrices)) {
        return YAMI_DECODE_PARSER_FAIL;
    }
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::decodeGroup(const DecodeUnit& du)
{
    if (!m_parser->parseGOPHeader(du))
        return YAMI_DECODE_INVALID_DATA;
    GOPHeader& gop = m_parser->m_GOPHeader;
    if (gop.closed_gop && gop.broken_link)
        m_dpb.flush();
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::ensureProfileAndLevel()
{
    YamiStatus status = YAMI_SUCCESS;
    ProfileType profile = m_parser->getProfile();
    LevelType level = m_parser->getLevel();

    status = checkLevel(level);
    if (status != YAMI_SUCCESS) {
        ERROR("unsupported level %d", level);
        return status;
    }

    VAProfile vaProfile = convertToVAProfile(profile);
    if (vaProfile == VAProfileNone) {
        ERROR("unsupported profile %d", profile);
        return YAMI_UNSUPPORTED;
    }

    uint32_t width = m_parser->getWidth();
    uint32_t height = m_parser->getHeight();
    if (setFormat(width, height, width, height, kMinSurfaces)) {
        DEBUG("format changed to %dx%d", width, height);
        return YAMI_DECODE_FORMAT_CHANGE;
    }

    return ensureProfile(vaProfile);
}

YamiStatus VaapiDecoderMPEG2::decodeExtension(const DecodeUnit& du)
{
    YamiParser::BitReader br(du.m_data, du.m_size);

    uint32_t id;
    if (!br.read(id, 4)) {
        ERROR("failed to read extension id");
        return YAMI_FAIL;
    }
    DEBUG("extension id = %d", id);
    ExtensionIdentifierType type = (ExtensionIdentifierType)id;

    if (type == kSequence) {
        if (!m_parser->parseSequenceExtension(br))
            return YAMI_DECODE_INVALID_DATA;
        return ensureProfileAndLevel();
    }

    bool ret;
    switch (type) {
    case kPictureCoding:
        ret = m_parser->parsePictureCodingExtension(br);
        break;
    case kQuantizationMatrix:
        ret = m_parser->parseQuantMatrixExtension(br, m_matrices);
        break;
    default:
        ret = true;
        break;
    }
    return ret ? YAMI_SUCCESS : YAMI_DECODE_INVALID_DATA;
}

YamiStatus VaapiDecoderMPEG2::decodePicture(const DecodeUnit& du)
{
    if (!m_parser->parsePictureHeader(du))
        return YAMI_DECODE_INVALID_DATA;
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::decodeNalUnit(const uint8_t* nalData, int32_t nalSize)
{
    DecodeUnit du;
    if (!du.parse(nalData, nalSize)) {
        ERROR("invalid decode unit size = %d", nalSize);
        return YAMI_DECODE_INVALID_DATA;
    }
    DEBUG("du type = %x\n", du.m_type);
    if (du.isSlice()) {
        return decodeSlice(du);
    }

    YamiStatus status = decodeCurrent();
    if (status != YAMI_SUCCESS)
        return status;
    switch (du.m_type) {
    case MPEG2_SEQUENCE_HEADER_CODE:
        status = decodeSequnce(du);
        break;
    case MPEG2_GROUP_START_CODE:
        status = decodeGroup(du);
        break;
    case MPEG2_PICTURE_START_CODE:
        status = decodePicture(du);
        break;
    case MPEG2_EXTENSION_START_CODE:
        status = decodeExtension(du);
        break;
    default:
        break;
    }
    return status;
}


YamiStatus VaapiDecoderMPEG2::decode(VideoDecodeBuffer* buffer)
{
    if (!buffer || !buffer->data) {
        decodeCurrent();
        m_dpb.flush();
        return YAMI_SUCCESS;
    }

    m_currentPTS = buffer->timeStamp;

    DEBUG("decode size %ld timeStamp %" PRIu64 "", m_stream->streamSize,
          m_stream->time_stamp);

    YamiStatus status;

    NalReader nalReader(buffer->data, buffer->size);
    const uint8_t* nalData;
    int32_t nalSize;

    while (nalReader.read(nalData, nalSize)) {
        status = decodeNalUnit(nalData, nalSize);
        if (status != YAMI_SUCCESS)
            return status;
    }
    return YAMI_SUCCESS;
}

void VaapiDecoderMPEG2::stop()
{
    DEBUG("MPEG2: stop()");
    flush();
    VaapiDecoderBase::stop();
}

void VaapiDecoderMPEG2::flush()
{
    DEBUG("MPEG2: flush()");
    decodeCurrent();
    m_dpb.flush();
    m_parser.reset(new Parser());
    VaapiDecoderBase::flush();
}

}
