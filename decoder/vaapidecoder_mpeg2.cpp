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

#include "common/log.h"
#include "common/nalreader.h"
#include "vaapidecpicture.h"
#include "vaapidecoder_mpeg2.h"

#include <string.h>

using YamiParser::MPEG2::Slice;
using YamiParser::MPEG2::PictureCodingExtension;
using YamiParser::MPEG2::PictureHeader;
using YamiParser::MPEG2::GOPHeader;
using YamiParser::MPEG2::SeqExtension;
using YamiParser::MPEG2::SeqHeader;
using YamiParser::MPEG2::StreamHeader;
using YamiParser::MPEG2::Parser;
using YamiParser::MPEG2::QuantMatrices;

namespace YamiMediaCodec {
typedef VaapiDecoderMPEG2::PicturePtr PicturePtr;

IQMatricesRefs::IQMatricesRefs() { memset(this, 0, sizeof(*this)); }
// mpeg2picture class

class VaapiDecPictureMpeg2 : public VaapiDecPicture {
public:
    VaapiDecPictureMpeg2(const ContextPtr& context, const SurfacePtr& surface,
                         int64_t timeStamp)
        : VaapiDecPicture(context, surface, timeStamp)
        , m_VAPictureStructure_(VAAPI_PICTURE_FRAME)
        , m_sentToOutput_(false)
        , m_isFirstField_(false)
    {
    }
    VaapiDecPictureMpeg2() {}

    // move to private and create get/set methods
    VaapiPictureType m_VAPictureStructure_;
    bool m_sentToOutput_;
    uint32_t m_temporalReference_;
    uint32_t m_pictureCodingType_;
    uint32_t m_topFieldFirst_;
    uint32_t m_progressiveFrame_;
    bool m_isFirstField_;

private:
};
// mpeg2picture class

void VaapiDecoderMPEG2::DPB::flush()
{
    DEBUG("MPEG2-DPB flush");
    m_referencePictures.clear();
}

YamiStatus
VaapiDecoderMPEG2::DPB::outputPreviousPictures(const PicturePtr& picture, bool empty)
{
    YamiStatus status = YAMI_SUCCESS;
    std::list<PicturePtr>::iterator it = m_referencePictures.begin();

    if (picture->m_pictureCodingType_ == YamiParser::MPEG2::kIFrame)
        empty = true;

    // send to the output all possible pictures
    for (; it != m_referencePictures.end(); it++) {
        DEBUG("candidate picture temporalReference %d",
              picture->m_temporalReference_);

        if ((picture->m_temporalReference_ >= (*it)->m_temporalReference_
             || empty)
            && (*it)->m_sentToOutput_ == false) {

            status = callOutputPicture((*it));
            if (status != YAMI_SUCCESS)
                return status;

            (*it)->m_sentToOutput_ = true;
            DEBUG("output temporalReference %d", (*it)->m_temporalReference_);
        }
    }

    return status;
}

YamiStatus
VaapiDecoderMPEG2::DPB::insertPictureToReferences(const PicturePtr& picture)
{
    PicturePtr outputPicture;
    YamiStatus status = YAMI_SUCCESS;

    if (picture->m_pictureCodingType_ != YamiParser::MPEG2::kBFrame)
    {
        DEBUG("Put picture %d on the reference queue",
              picture->m_temporalReference_);

        if (m_referencePictures.size() == 2) {
            outputPicture = m_referencePictures.front();
            DEBUG("Drop picture %d from reference queue",
                  outputPicture->m_temporalReference_);
            m_referencePictures.pop_front();
        }
        m_referencePictures.push_back(picture);
    } else {
        // this is a kBFrame
        // send it to output right away
        DEBUG("Send B frame picture to the output %d",
              picture->m_temporalReference_);
        status = callOutputPicture(picture);
        if (status != YAMI_SUCCESS)
            return status;
    }
    return status;
}

YamiStatus VaapiDecoderMPEG2::DPB::insertPicture(const PicturePtr& picture)
{
    INFO("insertPicture to DPB size %lu", m_referencePictures.size());

    YamiStatus status = YAMI_SUCCESS;

    DEBUG("temporalReference %d", picture->m_temporalReference_);
    DEBUG("pictureCodingType %d", picture->m_pictureCodingType_);
    DEBUG("topFieldFirst %d", picture->m_topFieldFirst_);
    DEBUG("progressiveFrame %d", picture->m_progressiveFrame_);

    status = outputPreviousPictures(picture);
    if (status != YAMI_SUCCESS)
        return status;

    if (picture->m_progressiveFrame_
        || picture->m_VAPictureStructure_ == VAAPI_PICTURE_FRAME) {

        INFO("Insert one frame picture");
        // frame picture can be inserted straight to DPB as it was decoded
        // completely before trying to insert, it can also be
        // output if temporalReference matches PTS

        status = insertPictureToReferences(picture);
        if (status != YAMI_SUCCESS)
            return status;

    } else if (!picture->m_progressiveFrame_
               && picture->m_VAPictureStructure_
                  != VAAPI_PICTURE_FRAME) {
        // field pictures received, top field and bottom field have their lists
        // based on those a decision will be made if frame can be displayed
        INFO("This is a field picture");
        if (!picture->m_isFirstField_) {
            status = insertPictureToReferences(picture);
            if (status != YAMI_SUCCESS)
                return status;
        }
    }

    DEBUG("insertPicture returns dpb size %lu", m_referencePictures.size());
    return status;
}

YamiStatus
VaapiDecoderMPEG2::DPB::getReferencePictures(const PicturePtr& current_picture,
    PicturePtr& previousPicture,
    PicturePtr& nextPicture)
{
    // m_referencePictures can have 0,1 or 2 reference pictures to comply with
    // the amount of reference frames needed by picture coding type
    // picture coding type kIFrame - 0 ref frames
    // picture coding type kPFrame - 1 ref frames
    // picture coding type kBFrame - 2 ref frames

    if (m_referencePictures.size() == 1) {
        previousPicture = m_referencePictures.front();
    } else if (m_referencePictures.size() == 2
               && current_picture->m_pictureCodingType_
                  == YamiParser::MPEG2::kBFrame) {
        previousPicture = m_referencePictures.front();
        nextPicture = m_referencePictures.back();
    } else if (m_referencePictures.size() == 2
               && current_picture->m_pictureCodingType_
                  == YamiParser::MPEG2::kPFrame) {
        previousPicture = m_referencePictures.back();
    }

    return YAMI_SUCCESS;
}

VaapiDecoderMPEG2::VaapiDecoderMPEG2()
    : m_DPB(std::bind(&VaapiDecoderMPEG2::outputPicture, this,
                                 std::placeholders::_1))
    , m_VAStart(false)
    , m_isParsingSlices(false)
    , m_loadNewIQMatrix(false)
{
    m_parser.reset(new Parser());
    m_stream.reset(new StreamHeader());
    m_previousStartCode = YamiParser::MPEG2::MPEG2_INVALID_START_CODE;
    m_nextStartCode = YamiParser::MPEG2::MPEG2_SEQUENCE_HEADER_CODE;
    INFO("VaapiDecoderMPEG2 constructor");
}

VaapiDecoderMPEG2::~VaapiDecoderMPEG2() { stop(); }

YamiStatus VaapiDecoderMPEG2::convertToVAProfile(
    const YamiParser::MPEG2::ProfileType& profile)
{
    YamiStatus status = YAMI_SUCCESS;

    switch (profile) {
    case YamiParser::MPEG2::MPEG2_PROFILE_MAIN:
        m_VAProfile = VAProfileMPEG2Main;
        break;
    case YamiParser::MPEG2::MPEG2_PROFILE_SIMPLE:
        m_VAProfile = VAProfileMPEG2Simple;
        break;
    default:
        m_VAProfile = VAProfileNone;
        status = YAMI_DECODE_PARSER_FAIL;
        break;
    }
    return status;
}

YamiStatus
VaapiDecoderMPEG2::checkLevel(const YamiParser::MPEG2::LevelType& level)
{
    switch (level) {
    case YamiParser::MPEG2::MPEG2_LEVEL_HIGH:
    case YamiParser::MPEG2::MPEG2_LEVEL_HIGH_1440:
    case YamiParser::MPEG2::MPEG2_LEVEL_MAIN:
    case YamiParser::MPEG2::MPEG2_LEVEL_LOW:
        return YAMI_SUCCESS;
        break;
    default:
        return YAMI_DECODE_PARSER_FAIL;
        break;
    }
}

YamiStatus VaapiDecoderMPEG2::start(VideoConfigBuffer* buffer)
{

    if (!buffer) {
        ERROR("Cannot start codec without config buffer");
        return YAMI_FAIL;
    }

    m_configBuffer = *buffer;

    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::processConfigBuffer()
{
    // buffer can contain one startcode of the mpeg2 sequence
    // or several, but they will be processed one by one

    YamiStatus status = YAMI_SUCCESS;
    YamiParser::MPEG2::StartCodeType next_code;
    YamiParser::MPEG2::ExtensionIdentifierType extID;

    m_parser->nextStartCode(m_stream.get(), next_code);
    INFO("processConfigBuffer Next start_code %x", next_code);

    switch (next_code) {
    case YamiParser::MPEG2::MPEG2_SEQUENCE_HEADER_CODE:
        INFO("parseSeqHdr");
        if (!m_parser->parseSequenceHeader(m_stream.get())) {
            return YAMI_DECODE_PARSER_FAIL;
        }
        m_sequenceHeader = m_parser->getSequenceHeader();
        m_previousStartCode = m_nextStartCode;
        m_nextStartCode = YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE;
        m_loadNewIQMatrix
            = updateIQMatrix(&m_sequenceHeader->quantizationMatrices, true);
        if (!m_DPB.isEmpty())
            m_DPB.flush();
        break;
    case YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE:
        INFO("parseSeqExt");
        extID = static_cast<YamiParser::MPEG2::ExtensionIdentifierType>(
            m_stream->nalData[1] >> 4);
        if (!m_VAStart) {

            switch (extID) {
            case YamiParser::MPEG2::kSequence:
                if (!m_parser->parseSequenceExtension(m_stream.get())) {
                    return YAMI_DECODE_PARSER_FAIL;
                }
                m_sequenceExtension = m_parser->getSequenceExtension();
                m_previousStartCode = m_nextStartCode;
                m_nextStartCode = YamiParser::MPEG2::MPEG2_GROUP_START_CODE;
                // ready to fillConfigBuffer
                // this should be returning YAMI_SUCCESS if
                // application
                // has not allocated output port earlier

                status = fillConfigBuffer();
                if (status != YAMI_SUCCESS) {
                    return status;
                }
                break;

            case YamiParser::MPEG2::kQuantizationMatrix:
                if (!m_parser->parseQuantMatrixExtension(m_stream.get())) {
                    return YAMI_DECODE_PARSER_FAIL;
                }
                m_quantMatrixExtension = m_parser->getQuantMatrixExtension();
                m_loadNewIQMatrix = updateIQMatrix(
                    &m_quantMatrixExtension->quantizationMatrices);
                break;
            default:
                break;
            }
        }
        break;
    default:
        status = processDecodeBuffer();
        INFO("Calling processDecodeBuffer from "
             "processConfigBuffer");
        if (status != YAMI_SUCCESS) {
            return status;
        }
        break;
    }
    return status;
}

YamiStatus VaapiDecoderMPEG2::processDecodeBuffer()
{
    // buffer can contain only one nal of the mpeg2 sequence

    YamiStatus status = YAMI_SUCCESS;
    YamiParser::MPEG2::StartCodeType next_code;
    YamiParser::MPEG2::ExtensionIdentifierType extID;

    m_parser->nextStartCode(m_stream.get(), next_code);
    DEBUG("processDecodeBuffer Next start_code %x", next_code);

    switch (next_code) {
    case YamiParser::MPEG2::MPEG2_SEQUENCE_HEADER_CODE:
        INFO("parseSeqHdr");
        if (!m_parser->parseSequenceHeader(m_stream.get())) {
            return YAMI_DECODE_PARSER_FAIL;
        }
        m_sequenceHeader = m_parser->getSequenceHeader();
        // reset the matrices
        m_previousStartCode = m_nextStartCode;
        m_nextStartCode = YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE;
        m_loadNewIQMatrix
            = updateIQMatrix(&m_sequenceHeader->quantizationMatrices, true);

        break;
    case YamiParser::MPEG2::MPEG2_GROUP_START_CODE:
        INFO("Group start code");

        if (m_isParsingSlices) {
            status = decodePicture();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            m_isParsingSlices = false;
            m_nextStartCode = YamiParser::MPEG2::MPEG2_GROUP_START_CODE;
        }

        if (!m_parser->parseGOPHeader(m_stream.get())) {
            return YAMI_DECODE_PARSER_FAIL;
        }
        m_GOPHeader = m_parser->getGOPHeader();
        m_previousStartCode = m_nextStartCode;
        m_nextStartCode = YamiParser::MPEG2::MPEG2_PICTURE_START_CODE;
        break;
    case YamiParser::MPEG2::MPEG2_PICTURE_START_CODE:
        INFO("picture start code");
        if (m_isParsingSlices) {
            status = decodePicture();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            m_isParsingSlices = false;
        }

        if (m_previousStartCode
            == YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE
            || m_previousStartCode
            == YamiParser::MPEG2::MPEG2_GROUP_START_CODE) {
            m_nextStartCode = YamiParser::MPEG2::MPEG2_PICTURE_START_CODE;
            INFO("parsePictureHeader size %ld", m_stream->streamSize);
            if (!m_parser->parsePictureHeader(m_stream.get())) {
                return YAMI_DECODE_PARSER_FAIL;
            }
            m_pictureHeader = m_parser->getPictureHeader();
            m_previousStartCode = m_nextStartCode;
            m_nextStartCode = YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE;
        }
        break;
    case YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE:
        extID = static_cast<YamiParser::MPEG2::ExtensionIdentifierType>(
            m_stream->nalData[1] >> 4);
        if (extID == YamiParser::MPEG2::kPictureCoding) {
            if (m_nextStartCode == next_code) {
                if (!m_parser->parsePictureCodingExtension(m_stream.get())) {
                    return YAMI_DECODE_PARSER_FAIL;
                }
                m_pictureCodingExtension
                    = m_parser->getPictureCodingExtension();

                // picture can be created as soon as the first slice is
                // parsed, but from here to there Extension Start Code can be
                // parsed to update information like the Quant Matrix

                m_canCreatePicture = true;
                m_previousStartCode = m_nextStartCode;
                m_nextStartCode = YamiParser::MPEG2::MPEG2_SLICE_START_CODE_MIN;
                m_isParsingSlices = true;
            }
        }
        else if (extID == YamiParser::MPEG2::kQuantizationMatrix) {
            INFO("this is a new quant matrix");
            if (!m_parser->parseQuantMatrixExtension(m_stream.get())) {
                return YAMI_DECODE_PARSER_FAIL;
            }
            m_quantMatrixExtension = m_parser->getQuantMatrixExtension();
            m_loadNewIQMatrix
                = updateIQMatrix(&m_quantMatrixExtension->quantizationMatrices);
        }
        else if (extID == YamiParser::MPEG2::kSequence) {
            if (!m_parser->parseSequenceExtension(m_stream.get())) {
                return YAMI_DECODE_PARSER_FAIL;
            }
            m_sequenceExtension = m_parser->getSequenceExtension();
            m_previousStartCode = m_nextStartCode;
            m_nextStartCode = YamiParser::MPEG2::MPEG2_GROUP_START_CODE;
            status = fillConfigBuffer();
            if (status != YAMI_SUCCESS) {
                return status;
            }
        }
        break;
    case YamiParser::MPEG2::MPEG2_SEQUENCE_END_CODE:
        INFO("End of sequence received");
        if (m_isParsingSlices) {
            status = decodePicture();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            status = m_DPB.outputPreviousPictures(m_currentPicture, true);
            if (status != YAMI_SUCCESS) {
                return status;
            }
            m_isParsingSlices = false;
        }
        break;
    default:
        // process a SliceCode
        if (isSliceCode(next_code)) {

            if (m_canCreatePicture)
            {
                // create a new picture with updated information a field
                // picture doesn't need to create Picture twice as both top
                // and bottom fields share the same slice number.
                status = createPicture();
                if (status != YAMI_SUCCESS) {
                    return status;
                }
                m_canCreatePicture = false;
            }
            INFO("processSlice on next_code %x", next_code);
            status = processSlice();
            if (status != YAMI_SUCCESS) {
                return status;
            }
        }
        break;
    }
    return status;
}

YamiStatus VaapiDecoderMPEG2::fillConfigBuffer()
{

    YamiStatus status = YAMI_SUCCESS;
    YamiParser::MPEG2::ProfileType profile;
    YamiParser::MPEG2::LevelType level;

    profile = static_cast<YamiParser::MPEG2::ProfileType>(
        (m_sequenceExtension->profile_and_level_indication & 0x70) >> 4);
    level = static_cast<YamiParser::MPEG2::LevelType>(
        m_sequenceExtension->profile_and_level_indication & 0xF);

    status = checkLevel(level);
    if (status != YAMI_SUCCESS) {
        return status;
    }

    status = convertToVAProfile(profile);
    if (status != YAMI_SUCCESS) {
        return status;
    }

    m_configBuffer.width
        = (m_sequenceExtension->horizontal_size_extension & 0x3)
          | (m_sequenceHeader->horizontal_size_value & 0xFFF);
    m_configBuffer.height = (m_sequenceExtension->vertical_size_extension & 0x3)
                            | (m_sequenceHeader->vertical_size_value & 0xFFF);

    if (m_VAStart) {
        // sequence start extension code with VA started has to conduct a
        // port reconfiguration when stream size changes
        if (m_configBuffer.width > m_videoFormatInfo.width
            || m_configBuffer.height > m_videoFormatInfo.height) {
            // need to re-start VA to generate new surfaces
            status = VaapiDecoderBase::terminateVA();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            m_VAStart = false;
        }
        else if (m_configBuffer.width != m_videoFormatInfo.width
                 || m_configBuffer.height != m_videoFormatInfo.height) {
            // VA surfaces can be re-used and client is notified
            // public member from base class
            m_videoFormatInfo.width = m_configBuffer.width;
            m_videoFormatInfo.height = m_configBuffer.height;
            m_videoFormatInfo.surfaceWidth = m_configBuffer.width;
            m_videoFormatInfo.surfaceHeight = m_configBuffer.height;
            m_configBuffer.surfaceWidth = m_configBuffer.width;
            m_configBuffer.surfaceHeight = m_configBuffer.height;
            status = YAMI_DECODE_FORMAT_CHANGE;
        }
    }

    if (!m_VAStart) {

        m_configBuffer.profile = m_VAProfile;

        DEBUG("MPEG2: start() buffer size: %d x %d m_VAProfile %x level %x",
              m_configBuffer.width, m_configBuffer.height,
              m_configBuffer.profile, level);

        m_configBuffer.surfaceWidth = m_configBuffer.width;
        m_configBuffer.surfaceHeight = m_configBuffer.height;
        if (!m_configBuffer.surfaceNumber) {
            m_configBuffer.surfaceNumber = kMinSurfaces;
        }
        // no information is sent to start libva at this point
        m_configBuffer.data = NULL;
        m_configBuffer.size = 0;

        // ready to start libva
        status = VaapiDecoderBase::start(&m_configBuffer);
        if (status != YAMI_SUCCESS) {
            return status;
        }
        m_VAStart = true;
        status = YAMI_DECODE_FORMAT_CHANGE;
    }

    return status;
}

bool VaapiDecoderMPEG2::isSliceCode(YamiParser::MPEG2::StartCodeType next_code)
{
    if (next_code >= YamiParser::MPEG2::MPEG2_SLICE_START_CODE_MIN
        && next_code <= YamiParser::MPEG2::MPEG2_SLICE_START_CODE_MAX) {
        DEBUG("Slice code slice number %x", next_code);
        return true;
    }

    return false;
}

YamiStatus VaapiDecoderMPEG2::processSlice()
{
    const Slice* slice;
    YamiStatus status = YAMI_DECODE_PARSER_FAIL;

    if (m_parser->parseSlice(m_stream.get())) {
        slice = m_parser->getMPEG2Slice();
        if (!m_currentPicture->newSlice(mpeg2SliceParams,
                slice->sliceData, slice->sliceDataSize)) {
            DEBUG("picture->newSlice failed");
            return YAMI_FAIL;
        }
        fillSliceParams(mpeg2SliceParams, slice);
        status = YAMI_SUCCESS;
    }

    return status;
}

inline bool findTempReference(const PicturePtr& picture,
                              uint32_t temporalReference)
{
    return picture->m_temporalReference_ == temporalReference;
}

YamiStatus VaapiDecoderMPEG2::assignSurface()
{
    SurfacePtr surface;
    YamiStatus status = YAMI_SUCCESS;

    surface = VaapiDecoderBase::createSurface();

    if (!surface) {
        status = YAMI_DECODE_NO_SURFACE;
    } else {
        surface->setCrop(0, 0, m_configBuffer.width, m_configBuffer.height);
        m_currentPicture.reset(
            new VaapiDecPictureMpeg2(m_context, surface, m_currentPTS));
        m_currentPicture->m_isFirstField_ = true;
    }

    return status;
}

YamiStatus VaapiDecoderMPEG2::findReusePicture(std::list<PicturePtr>& list,
    bool& reuse)
{
    std::list<PicturePtr>::iterator it;
    it = std::find_if(list.begin(), list.end(),
                      std::bind(findTempReference,
                                     std::placeholders::_1,
                                     m_pictureHeader->temporal_reference));
    if (it != list.end()) {
        m_currentPicture = (*it);
        m_currentPicture->m_isFirstField_ = false;
        list.erase(it);
        reuse = true;
    }
    return YAMI_SUCCESS;
}

YamiStatus VaapiDecoderMPEG2::assignPicture()
{
    YamiStatus status = YAMI_SUCCESS;
    bool reuseSurface = false;

    // reuse surface/picture when decoding a field picture, only complete
    // pictures can be pushed to the dpb, so it is up to the topfield and
    // bottomfield lists to hold  partial pictures before pushing

    // determine if the current Picture is a field picture and if it is
    // topfield or bottomfield

    if (!m_pictureCodingExtension->progressive_frame
        && m_pictureCodingExtension->picture_structure != kFramePicture) {
        DEBUG("Create a Field Picture");
        // this is a field picture
        //
        if (m_pictureCodingExtension->top_field_first) {
            // top field picture first
            DEBUG("Create Top Field Picture");

            status = assignSurface();
            if (status != YAMI_SUCCESS) {
                return status;
            }

            m_topFieldPictures.push_back(m_currentPicture);

        } else {
            // bottom field picture
            if (!m_bottomFieldPictures.empty()) {
                findReusePicture(m_bottomFieldPictures, reuseSurface);
                if (status != YAMI_SUCCESS) {
                    return status;
                }
            } else if (!m_topFieldPictures.empty()) {
                findReusePicture(m_topFieldPictures, reuseSurface);
                if (status != YAMI_SUCCESS) {
                    return status;
                }
            }
        }
        if (!reuseSurface) {
            status = assignSurface();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            m_bottomFieldPictures.push_back(m_currentPicture);
        }
    } else {

        DEBUG("Create a Frame Picture");
        status = assignSurface();
        if (status != YAMI_SUCCESS) {
            return status;
        }
    }
    return status;
}

bool VaapiDecoderMPEG2::updateIQMatrix(const QuantMatrices* refIQMatrix,
                                       bool reset)
{

    // when a new update to the IQMatrix is received, one or more can be
    // updated, leaving those previously updated untouched if this is an
    // update triggered by a Quant Matrix Extension. A Sequence Header
    // Extension must reset the matrices.

    if (reset)
        m_IQMatrices = IQMatricesRefs();

    if (refIQMatrix->load_intra_quantiser_matrix) {
        m_IQMatrices.intra_quantiser_matrix
            = refIQMatrix->intra_quantiser_matrix;
    }

    if (refIQMatrix->load_non_intra_quantiser_matrix) {
        m_IQMatrices.non_intra_quantiser_matrix
            = refIQMatrix->non_intra_quantiser_matrix;
    }

    if (refIQMatrix->load_chroma_intra_quantiser_matrix) {
        m_IQMatrices.chroma_intra_quantiser_matrix
            = refIQMatrix->chroma_intra_quantiser_matrix;
    }

    if (refIQMatrix->load_chroma_non_intra_quantiser_matrix) {
        m_IQMatrices.chroma_non_intra_quantiser_matrix
            = refIQMatrix->chroma_non_intra_quantiser_matrix;
    }

    return refIQMatrix->load_intra_quantiser_matrix
           || refIQMatrix->load_non_intra_quantiser_matrix
           || refIQMatrix->load_chroma_intra_quantiser_matrix
           || refIQMatrix->load_chroma_non_intra_quantiser_matrix;
}

YamiStatus VaapiDecoderMPEG2::loadIQMatrix()
{
    YamiStatus status = YAMI_SUCCESS;
    VAIQMatrixBufferMPEG2* IQMatrix = NULL;

    if (!m_currentPicture->editIqMatrix(IQMatrix)) {
        ERROR("picture->editIqMatrix failed");
        return YAMI_FAIL;
    }

    if (m_IQMatrices.intra_quantiser_matrix) {
        IQMatrix->load_intra_quantiser_matrix = 1;
        memcpy(IQMatrix->intra_quantiser_matrix,
               m_IQMatrices.intra_quantiser_matrix, 64);
    }

    if (m_IQMatrices.non_intra_quantiser_matrix) {
        IQMatrix->load_non_intra_quantiser_matrix = 1;
        memcpy(IQMatrix->non_intra_quantiser_matrix,
               m_IQMatrices.non_intra_quantiser_matrix, 64);
    }

    if (m_IQMatrices.chroma_intra_quantiser_matrix) {
        IQMatrix->load_chroma_intra_quantiser_matrix = 1;
        memcpy(IQMatrix->chroma_intra_quantiser_matrix,
               m_IQMatrices.chroma_intra_quantiser_matrix, 64);
    }

    if (m_IQMatrices.chroma_non_intra_quantiser_matrix) {
        IQMatrix->load_chroma_non_intra_quantiser_matrix = 1;
        memcpy(IQMatrix->chroma_non_intra_quantiser_matrix,
               m_IQMatrices.chroma_non_intra_quantiser_matrix, 64);
    }

    return status;
}

YamiStatus VaapiDecoderMPEG2::createPicture()
{
    YamiStatus status = YAMI_SUCCESS;
    VAPictureParameterBufferMPEG2* mpeg2PictureParams;

    status = assignPicture();
    if (status != YAMI_SUCCESS) {
        return status;
    }

    m_currentPicture->m_temporalReference_
        = m_pictureHeader->temporal_reference;
    m_currentPicture->m_pictureCodingType_
        = m_pictureHeader->picture_coding_type;

    switch (m_pictureCodingExtension->picture_structure) {
    case kTopField:
        m_currentPicture->m_VAPictureStructure_
            = VAAPI_PICTURE_TOP_FIELD;
        break;
    case kBottomField:
        m_currentPicture->m_VAPictureStructure_
            = VAAPI_PICTURE_BOTTOM_FIELD;
        break;
    case kFramePicture:
        m_currentPicture->m_VAPictureStructure_
            = VAAPI_PICTURE_FRAME;
        break;
    default:
        break;
    }

    m_currentPicture->m_topFieldFirst_
        = m_pictureCodingExtension->top_field_first;
    m_currentPicture->m_progressiveFrame_
        = m_pictureCodingExtension->progressive_frame;

    if (!m_currentPicture->editPicture(mpeg2PictureParams)) {
        ERROR("picture->editPicture failed");
        return YAMI_FAIL;
    }

    fillPictureParams(mpeg2PictureParams, m_currentPicture);

    if (m_loadNewIQMatrix) {
        status = loadIQMatrix();
        if (status != YAMI_SUCCESS) {
            ERROR("loadIQMatrix failed");
            return status;
        }
        m_loadNewIQMatrix = false;
    }

    return status;
}

YamiStatus VaapiDecoderMPEG2::decodePicture()
{
    YamiStatus status;

    if (!m_currentPicture->decode()) {
        DEBUG("picture->decode failed");
        return YAMI_FAIL;
    }
    // put full picture into the dpb
    status = m_DPB.insertPicture(m_currentPicture);
    if (status != YAMI_SUCCESS) {
        ERROR("insertPicture to DPB failed");
    }

    return status;
}

YamiStatus VaapiDecoderMPEG2::outputPicture(const PicturePtr& picture)
{
    VaapiDecoderBase::PicturePtr basePicture
        = std::static_pointer_cast<VaapiDecPicture>(picture);

    return VaapiDecoderBase::outputPicture(basePicture);
}

YamiStatus VaapiDecoderMPEG2::decode(VideoDecodeBuffer* buffer)
{
    YamiParser::MPEG2::StartCodeType next_code;
    YamiStatus status = YAMI_SUCCESS;
    YamiParser::MPEG2::ExtensionIdentifierType extID;

    // safe check, client can retry if this happens
    if (!buffer) {
        return YAMI_SUCCESS;
    }

    if (buffer->data == NULL || buffer->size == 0) {
        // no information provided in the buffer
        return YAMI_SUCCESS;
    }

    m_stream->data = buffer->data;
    m_stream->streamSize = buffer->size;
    m_stream->time_stamp = buffer->timeStamp;
    m_currentPTS = buffer->timeStamp;

    DEBUG("decode size %ld timeStamp %ld", m_stream->streamSize,
          m_stream->time_stamp);

    if (m_stream->streamSize < YamiParser::MPEG2::kStartCodeSize) {
        // not enough buffer, client to provide more input
        return YAMI_SUCCESS;
    }

    NalReader nalReader(m_stream->data, m_stream->streamSize);

    while (nalReader.read(m_stream->nalData, m_stream->nalSize)) {
        m_parser->nextStartCode(m_stream.get(), next_code);
        DEBUG("Next start_code %x", next_code);

        switch (next_code) {
        case YamiParser::MPEG2::MPEG2_SEQUENCE_HEADER_CODE:
            // the stream hasn't provided enough information to
            // properly configure it
            DEBUG("decode tries for a sequence_header");
            if (!m_VAStart) {
                status = processConfigBuffer();
                if (status != YAMI_SUCCESS
                    && status != YAMI_DECODE_FORMAT_CHANGE) {
                    return status;
                }
            } else {
                INFO("processDecodeBuffer is called");
                status = processDecodeBuffer();
                if (status != YAMI_SUCCESS) {
                    return status;
                }
            }
            break;
        case YamiParser::MPEG2::MPEG2_GROUP_START_CODE:
        case YamiParser::MPEG2::MPEG2_PICTURE_START_CODE:
        case YamiParser::MPEG2::MPEG2_SEQUENCE_END_CODE:
            status = processDecodeBuffer();
            if (status != YAMI_SUCCESS) {
                return status;
            }
            break;
        case YamiParser::MPEG2::MPEG2_EXTENSION_START_CODE:
            extID = static_cast<YamiParser::MPEG2::ExtensionIdentifierType>(
                m_stream->nalData[1] >> 4);
            if (m_previousStartCode
                == YamiParser::MPEG2::MPEG2_SEQUENCE_HEADER_CODE) {
                status = processConfigBuffer();
                if (status != YAMI_SUCCESS
                    && status != YAMI_DECODE_FORMAT_CHANGE) {
                    return status;
                }
            }
            else if (m_previousStartCode
                         == YamiParser::MPEG2::MPEG2_PICTURE_START_CODE
                     || extID == YamiParser::MPEG2::kQuantizationMatrix
                     || extID == YamiParser::MPEG2::kPictureCoding
                     || extID == YamiParser::MPEG2::kSequence) {
                status = processDecodeBuffer();
                if (status != YAMI_SUCCESS) {
                    return status;
                }
            }
            break;
        default:
            if (isSliceCode(next_code) && m_isParsingSlices) {
                status = processDecodeBuffer();
                if (status != YAMI_SUCCESS) {
                    return status;
                }
            }
            break;
        }
    }

    return YAMI_SUCCESS;
}

void VaapiDecoderMPEG2::fillPictureParams(VAPictureParameterBufferMPEG2* param,
                                          const PicturePtr& currentPicture)
{
    PicturePtr previousPicture, nextPicture;
    param->horizontal_size = m_configBuffer.width;
    param->vertical_size = m_configBuffer.height;
    param->picture_coding_type = m_pictureHeader->picture_coding_type;

    DEBUG("fillPictureParams picture coding type %d",
          param->picture_coding_type);

    m_DPB.getReferencePictures(currentPicture, previousPicture,
                                     nextPicture);

    // I-Frame does not use references
    param->forward_reference_picture = VA_INVALID_ID;
    param->backward_reference_picture = VA_INVALID_ID;
    if (param->picture_coding_type == YamiParser::MPEG2::kPFrame) {
        if (previousPicture) {
            DEBUG("Surface id on previous picture %d",
                  previousPicture->getSurfaceID());
            param->forward_reference_picture = previousPicture->getSurfaceID();
        }
    } else if (param->picture_coding_type == YamiParser::MPEG2::kBFrame) {
        if (previousPicture) {
            DEBUG("Surface id on previous picture %d",
                  previousPicture->getSurfaceID());
            param->forward_reference_picture = previousPicture->getSurfaceID();
        }
        if (nextPicture) {
            DEBUG("Surface id on next picture %d", nextPicture->getSurfaceID());
            param->backward_reference_picture = nextPicture->getSurfaceID();
        }
    }

    // libva packs all in one int.
    param->f_code = ((m_pictureCodingExtension->f_code[0][0] << 12)
                     | (m_pictureCodingExtension->f_code[0][1] << 8)
                     | (m_pictureCodingExtension->f_code[1][0] << 4)
                     | m_pictureCodingExtension->f_code[1][1]);

    param->picture_coding_extension.bits.intra_dc_precision
        = m_pictureCodingExtension->intra_dc_precision;
    param->picture_coding_extension.bits.picture_structure
        = m_pictureCodingExtension->picture_structure;
    param->picture_coding_extension.bits.top_field_first
        = m_pictureCodingExtension->top_field_first;
    param->picture_coding_extension.bits.frame_pred_frame_dct
        = m_pictureCodingExtension->frame_pred_frame_dct;
    param->picture_coding_extension.bits.concealment_motion_vectors
        = m_pictureCodingExtension->concealment_motion_vectors;
    param->picture_coding_extension.bits.q_scale_type
        = m_pictureCodingExtension->q_scale_type;
    param->picture_coding_extension.bits.intra_vlc_format
        = m_pictureCodingExtension->intra_vlc_format;
    param->picture_coding_extension.bits.alternate_scan
        = m_pictureCodingExtension->alternate_scan;
    param->picture_coding_extension.bits.repeat_first_field
        = m_pictureCodingExtension->repeat_first_field;
    param->picture_coding_extension.bits.progressive_frame
        = m_pictureCodingExtension->progressive_frame;

    DEBUG("currentPic temporalRef %d Progressive %d isFirstField %d",
          currentPicture->m_temporalReference_,
          currentPicture->m_progressiveFrame_, currentPicture->m_isFirstField_);
    if (currentPicture->m_progressiveFrame_ == 1
        || currentPicture->m_isFirstField_ == 1) {
        param->picture_coding_extension.bits.is_first_field = 1;
    } else {
        param->picture_coding_extension.bits.is_first_field = 0;
    }
}

void
VaapiDecoderMPEG2::fillSliceParams(VASliceParameterBufferMPEG2* slice_param,
                                   const Slice* slice)
{
    slice_param->macroblock_offset = slice->sliceHeaderSize;
    slice_param->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    slice_param->slice_horizontal_position = slice->macroblockColumn;
    slice_param->slice_vertical_position = slice->macroblockRow;
    slice_param->quantiser_scale_code = slice->quantiser_scale_code;
    slice_param->intra_slice_flag = slice->intra_slice_flag;
}

YamiStatus VaapiDecoderMPEG2::reset(VideoConfigBuffer* buffer)
{
    DEBUG("MPEG2: reset()");
    return VaapiDecoderBase::reset(buffer);
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
    m_DPB.flush();
    m_parser.reset(new Parser());
    VaapiDecoderBase::flush();
}

}
