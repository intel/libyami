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

// This file contains an implementation of a  Mpeg2 raw stream parser,
// as defined in ISO/IEC 13818-2
//
// parser is highly influenced on the way other parsers are written in
// this project in particular vp8_parser.
//
// The parser is built under the logic that the client will separate the
// input stream in chunks of information separated by start codes.
// Client can decide if partial start_code streams are permitted and owns
// the decision to parse partial information

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// config.h defines macros for log.h to define log levels
#include "common/log.h"
#include "mpeg2_parser.h"

namespace YamiParser {
namespace MPEG2 {

#define RETURN_ERROR(message)                                                  \
    do {                                                                       \
        ERROR("Error while reading %s", #message);                             \
        bitReaderDeInit();                                                     \
        return false;                                                          \
    } while (0)

#define READ_MARKER_OR_RETURN(flag)                                            \
    do {                                                                       \
        if (!bitReaderReadMarker(flag))                                        \
            RETURN_ERROR(flag);                                                \
    } while (0)

#define READ_FLAG_OR_RETURN(flag)                                              \
    do {                                                                       \
        if (!bitReaderReadFlag(flag))                                          \
            RETURN_ERROR(flag);                                                \
    } while (0)

#define READ_BITS_OR_RETURN(bits, var)                                         \
    do {                                                                       \
        if (!bitReaderReadBits(bits, var))                                     \
            RETURN_ERROR(var);                                                 \
    } while (0)

#define PEEK_BITS_OR_RETURN(bits, var)                                         \
    do {                                                                       \
        if (!bitReaderPeekBits(bits, var))                                     \
            RETURN_ERROR(var);                                                 \
    } while (0)

#define PEEK_BOOL_OR_RETURN(var)                                               \
    do {                                                                       \
        if (!bitReaderPeekBool(var))                                           \
            RETURN_ERROR(var);                                                 \
    } while (0)

#define SKIP_BITS_OR_RETURN(bits)                                              \
    do {                                                                       \
        if (!bitReaderSkipBits(bits))                                          \
            RETURN_ERROR("skip bits");                                         \
    } while (0)

#define SKIP_BYTE_OR_RETURN()                                                  \
    do {                                                                       \
        SKIP_BITS_OR_RETURN(8);                                                \
    } while (0)

    // vlc increment values per code
    // section B.1 table B-1

    const uint8_t kVLCTable[][3] = { // num_of_bits, code, increment_value
        { 1, 0x1, 1 },
        { 3, 0x3, 2 },
        { 3, 0x2, 3 },
        { 4, 0x3, 4 },
        { 4, 0x2, 5 },
        { 5, 0x3, 6 },
        { 5, 0x2, 7 },
        { 7, 0x7, 8 },
        { 7, 0x6, 9 },
        { 8, 0xb, 10 },
        { 8, 0xa, 11 },
        { 8, 0x9, 12 },
        { 8, 0x8, 13 },
        { 8, 0x7, 14 },
        { 8, 0x6, 15 },
        { 10, 0x17, 16 },
        { 10, 0x16, 17 },
        { 10, 0x15, 18 },
        { 10, 0x14, 19 },
        { 10, 0x13, 20 },
        { 10, 0x12, 21 },
        { 11, 0x23, 22 },
        { 11, 0x22, 23 },
        { 11, 0x21, 24 },
        { 11, 0x20, 25 },
        { 11, 0x1f, 26 },
        { 11, 0x1e, 27 },
        { 11, 0x1d, 28 },
        { 11, 0x1c, 29 },
        { 11, 0x1b, 30 },
        { 11, 0x1a, 31 },
        { 11, 0x19, 32 },
        { 11, 0x18, 33 },
        { 11, 0x8, 0xFF },
    };

    // default matrix for intra blocks
    // section 6.3.7
    const static uint8_t kDefaultIntraBlockMatrix[64]
        = { 8,  16, 16, 19, 16, 19, 22, 22, 22, 22, 22, 22, 26, 24, 26, 27,
            27, 27, 26, 26, 26, 26, 27, 27, 27, 29, 29, 29, 34, 34, 34, 29,
            29, 29, 27, 27, 29, 29, 32, 32, 34, 34, 37, 38, 37, 35, 35, 34,
            35, 38, 38, 40, 40, 40, 48, 48, 46, 46, 56, 56, 58, 69, 69, 83 };

    // default matrix for non-intra blocks
    const static uint8_t kDefaultNonIntraBlockMatrix[64]
        = { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 };

    Slice::Slice() { memset(this, 0, sizeof(*this)); }

    QuantMatrices::QuantMatrices() { memset(this, 0, sizeof(*this)); }

    QuantMatrixExtension::QuantMatrixExtension()
    {
        memset(this, 0, sizeof(*this));
    }

    PictureCodingExtension::PictureCodingExtension()
    {
        memset(this, 0, sizeof(*this));
    }

    PictureHeader::PictureHeader() { memset(this, 0, sizeof(*this)); }

    GOPHeader::GOPHeader() { memset(this, 0, sizeof(*this)); }

    SeqExtension::SeqExtension() { memset(this, 0, sizeof(*this)); }

    SeqHeader::SeqHeader() { memset(this, 0, sizeof(*this)); }

    StreamHeader::StreamHeader() { memset(this, 0, sizeof(*this)); }

    Parser::Parser()
    {
    }

    Parser::~Parser() {}

    bool Parser::nextStartCode(const StreamHeader* shdr, StartCodeType& start_code)
    {
        start_code
            = static_cast<StartCodeType>(shdr->nalData[kStartCodePrefixSize]);
        return true;
    }

    bool Parser::parseSlice(const StreamHeader* shdr)
    {
        uint32_t verticalSize;
        int32_t nalSize = shdr->nalSize;
        const uint8_t* nalData = shdr->nalData;
	bool marker;

        if (shdr->nalSize < kStartCodeSize + 1) {
            ERROR("Incomplete slice header");
            return false;
        }

        memset(&m_slice, 0, sizeof(Slice));

        m_slice.sliceData = nalData;
        m_slice.sliceDataSize = nalSize;

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);
        m_slice.verticalPosition = nalData[0];

        SKIP_BYTE_OR_RETURN(); // skip start code

        // parse one slice

        verticalSize = (m_sequenceExtension.vertical_size_extension & 0x3)
                       | (m_sequenceHdr.vertical_size_value & 0xFFF);

        if (verticalSize > 2800) {
            // really big picture
            READ_BITS_OR_RETURN(
                3, &(m_slice.slice_vertical_position_extension));
            m_slice.macroblockRow
                = (m_slice.slice_vertical_position_extension << 7)
                  + m_slice.verticalPosition - 1;
        } else {
            m_slice.macroblockRow = m_slice.verticalPosition - 1;
        }

        READ_BITS_OR_RETURN(5, &(m_slice.quantiser_scale_code));
        PEEK_BOOL_OR_RETURN(&marker);
        if (marker) {
            // read more intra bits
            READ_FLAG_OR_RETURN(&(m_slice.intra_slice_flag));
            READ_FLAG_OR_RETURN(&(m_slice.intra_slice));
            READ_BITS_OR_RETURN(7, &(m_slice.reserved_bits));

            PEEK_BOOL_OR_RETURN(&marker);
            while (marker) {
                // extra_information_slice
                READ_FLAG_OR_RETURN(&(m_slice.extra_bit_slice));
                if (!m_slice.extra_bit_slice) {
                    ERROR("Bad extra bit slice");
                    bitReaderDeInit();
                    return false;
                }
                SKIP_BYTE_OR_RETURN();
                PEEK_BOOL_OR_RETURN(&marker);
            }
        }

        READ_FLAG_OR_RETURN(&(m_slice.extra_bit_slice));

        if (m_slice.extra_bit_slice) {
            ERROR("Bad extra bit slice");
            bitReaderDeInit();
            return false;
        }

        // rest of slice is macro block information, decoder can process it
        // as long as the first macroblock_icrement is known

        // sliceHeaderSize is given in bits
        m_slice.sliceHeaderSize = bitReaderCurrentPosition();

        if (!calculateMBColumn())
            return false;


        DEBUG("slice header size                  : %ld",
              m_slice.sliceHeaderSize);
        DEBUG("slice number                       : %x",
              m_slice.verticalPosition);
        DEBUG("slice_vertical_position_extension  : %x",
              m_slice.slice_vertical_position_extension);
        DEBUG("quantiser_scale_code               : %x",
              m_slice.quantiser_scale_code);
        DEBUG("intra_slice_flag                   : %x",
              m_slice.intra_slice_flag);
        DEBUG("intra_slice                        : %x",
              m_slice.intra_slice);
        DEBUG("extra_bit_slice                    : %x",
              m_slice.extra_bit_slice);
        DEBUG("slice size                         : %ld",
              m_slice.sliceDataSize);
        DEBUG("size left on buffer                : %d", nalSize);
        DEBUG("slice data                         : %p",
              m_slice.sliceData);
        DEBUG("macroblockRow                      : %d",
              m_slice.macroblockRow);
        DEBUG("macroblockColumn                   : %d",
              m_slice.macroblockColumn);

        bitReaderDeInit();
        return true;
    }

    bool Parser::calculateMBColumn()
    {
        uint32_t bitsToParse;
        uint32_t mbVLC, mbINC = 0, totalMBINC = 0;
        do {
            for (uint8_t i = 0; i < 34; i++) {
                bitsToParse = kVLCTable[i][0];
                PEEK_BITS_OR_RETURN(bitsToParse, &mbVLC);
                if (mbVLC == kVLCTable[i][1]) {
                    mbINC = kVLCTable[i][2];
                    break;
                }
            }
            if (mbINC == 255)
                totalMBINC += 33;
            else
                totalMBINC += mbINC;
            SKIP_BITS_OR_RETURN(bitsToParse);
        } while (mbINC == 255);

        m_slice.macroblockColumn = totalMBINC - 1;
        return true;
    }

    bool Parser::parsePictureHeader(const StreamHeader* shdr)
    {
        PictureCodingType picture_type;
	const uint8_t *nalData= shdr->nalData;
	int32_t nalSize = shdr->nalSize;

        // minium length required on picture header buffer
        if (nalSize < (kStartCodeSize + 3)) {
            ERROR("Incomplete Picture Header");
            return false;
        }

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);
        SKIP_BYTE_OR_RETURN();

        READ_BITS_OR_RETURN(10, &(m_pictureHeader.temporal_reference));
        READ_BITS_OR_RETURN(3, &(m_pictureHeader.picture_coding_type));
        picture_type = static_cast<PictureCodingType>(
            m_pictureHeader.picture_coding_type);
        READ_BITS_OR_RETURN(16, &(m_pictureHeader.vbv_delay));

        if (picture_type == kPFrame || picture_type == kBFrame) {
            READ_FLAG_OR_RETURN(&(m_pictureHeader.full_pel_forward_vector));
            READ_BITS_OR_RETURN(3, &(m_pictureHeader.forward_f_code));
        }

        if (picture_type == kBFrame) {
            READ_FLAG_OR_RETURN(&(m_pictureHeader.full_pel_backward_vector));
            READ_BITS_OR_RETURN(3, &(m_pictureHeader.backward_f_code));
        }

        for (;;) {
            READ_FLAG_OR_RETURN(&(m_pictureHeader.extra_bit_picture));
            if (m_pictureHeader.extra_bit_picture == 1) {
                // decoder shall skip extra_information_picture byte
                SKIP_BYTE_OR_RETURN();
            } else {
                break;
            }
        }

        DEBUG("temporal_reference       : %x",
              m_pictureHeader.temporal_reference);
        DEBUG("picture_coding_type      : %x",
              m_pictureHeader.picture_coding_type);
        DEBUG("vbv_delay                : %x", m_pictureHeader.vbv_delay);
        DEBUG("full_pel_forward_vector  : %x",
              m_pictureHeader.full_pel_forward_vector);
        DEBUG("forward_f_code           : %x", m_pictureHeader.forward_f_code);
        DEBUG("full_pel_backward_vector : %x",
              m_pictureHeader.full_pel_backward_vector);
        DEBUG("backward_f_code          : %x",
              m_pictureHeader.backward_f_code);
        DEBUG("extra_bit_picture        : %x",
              m_pictureHeader.extra_bit_picture);

        bitReaderDeInit();

        return true;
    }

    bool Parser::parseGOPHeader(const StreamHeader* shdr)
    {
	const uint8_t *nalData= shdr->nalData;
	int32_t nalSize = shdr->nalSize;

        if (nalSize < (kStartCodeSize + 3)) {
            ERROR("Incomplete GOP Header");
            return false;
        }

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);

        SKIP_BYTE_OR_RETURN(); // skip start_sequence_code

        READ_FLAG_OR_RETURN(&(m_GOPHeader.drop_frame_flag));
        READ_BITS_OR_RETURN(5, &(m_GOPHeader.time_code_hours));
        READ_BITS_OR_RETURN(6, &(m_GOPHeader.time_code_minutes));

        READ_MARKER_OR_RETURN(true);

        READ_BITS_OR_RETURN(6, &(m_GOPHeader.time_code_seconds));
        READ_BITS_OR_RETURN(6, &(m_GOPHeader.time_code_pictures));
        READ_FLAG_OR_RETURN(&(m_GOPHeader.closed_gop));
        READ_FLAG_OR_RETURN(&(m_GOPHeader.broken_link));
        // five marker bits all should be 0
        for (uint8_t i(0); i < 5; ++i) {
            READ_MARKER_OR_RETURN(false);
        }

        DEBUG("drop_frame_flag    : %x", m_GOPHeader.drop_frame_flag);
        DEBUG("time_code_hours    : %x", m_GOPHeader.time_code_hours);
        DEBUG("time_code_minutes  : %x", m_GOPHeader.time_code_minutes);
        DEBUG("time_code_seconds  : %x", m_GOPHeader.time_code_seconds);
        DEBUG("time_code_pictures : %x", m_GOPHeader.time_code_pictures);
        DEBUG("closed_gop 	  : %x", m_GOPHeader.closed_gop);

        bitReaderDeInit();

        return true;
    }

    bool Parser::readQuantMatrixOrDefault(bool& loadMatrix, uint8_t matrix[],
                                          const uint8_t defaultMatrix[])
    {
        if (!readQuantMatrix(loadMatrix, matrix))
            return false;

        if (!loadMatrix) {
            memcpy(matrix, defaultMatrix, 64);
            loadMatrix = true;
        }
        return true;
    }

    bool Parser::readQuantMatrix(bool& loadMatrix, uint8_t matrix[])
    {

        READ_FLAG_OR_RETURN(&loadMatrix);

        if (loadMatrix) {
            // read 8 bits *64 from the stream
            uint32_t value;
            for (uint8_t i(0); i < 64; ++i) {
                READ_BITS_OR_RETURN(8, &value);
                matrix[i] = value;
            }
        }
        return true;
    }

    bool Parser::parseQuantMatrixExtension(const StreamHeader* shdr)
    {
        ExtensionIdentifierType extID;
        const uint8_t* nalData = shdr->nalData;
        int32_t nalSize = shdr->nalSize;
        QuantMatrices *quantMatrices = &m_quantMatrixExtension.quantizationMatrices;

        if (nalSize < kStartCodeSize) {
            ERROR("Incomplete Quant Extension Header");
            return false;
        }

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);
        SKIP_BYTE_OR_RETURN(); // skip start_sequence_code

        memset(&m_quantMatrixExtension, 0, sizeof(QuantMatrixExtension));

        // extension_start_code_identifier
        READ_BITS_OR_RETURN(
            4, &(m_quantMatrixExtension.extension_start_code_identifier));

        extID = static_cast<ExtensionIdentifierType>(
            m_quantMatrixExtension.extension_start_code_identifier);

        if (extID != kQuantizationMatrix) {
            ERROR("Wrong extension id type");
            bitReaderDeInit();
            return false;
        }

        if (!readQuantMatrix(quantMatrices->load_intra_quantiser_matrix,
                             quantMatrices->intra_quantiser_matrix))
            return false;

        if (!readQuantMatrix(quantMatrices->load_non_intra_quantiser_matrix,
                             quantMatrices->non_intra_quantiser_matrix))
            return false;

        if (!readQuantMatrix(quantMatrices->load_chroma_intra_quantiser_matrix,
                             quantMatrices->chroma_intra_quantiser_matrix))
            return false;

        if (!readQuantMatrix(
                quantMatrices->load_chroma_non_intra_quantiser_matrix,
                quantMatrices->chroma_non_intra_quantiser_matrix))
            return false;

        DEBUG("load_intra_quantiser_matrix             : %x",
              quantMatrices->load_intra_quantiser_matrix);
        DEBUG("load_non_intra_quantiser_matrix         : %x",
              quantMatrices->load_non_intra_quantiser_matrix);
        DEBUG("load_chroma_intra_quantiser_matrix      : %x",
              quantMatrices->load_chroma_intra_quantiser_matrix);
        DEBUG("load_chroma_non_intra_quantiser_matrix  : %x",
              quantMatrices->load_chroma_non_intra_quantiser_matrix);

        bitReaderDeInit();
        return true;
    }

    bool Parser::parsePictureCodingExtension(const StreamHeader* shdr)
    {
        ExtensionIdentifierType extID;
	const uint8_t *nalData= shdr->nalData;
	int32_t nalSize = shdr->nalSize;

        if (nalSize < (kStartCodeSize + 4)) {
            ERROR("Incomplete PictureCodingExtension Header");
            return false;
        }

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);
        SKIP_BYTE_OR_RETURN(); // skip start_sequence_code

        // extension_start_code_identifier
        READ_BITS_OR_RETURN(
            4, &(m_pictureCodingExtension.extension_start_code_identifier));

        extID = static_cast<ExtensionIdentifierType>(
            m_pictureCodingExtension.extension_start_code_identifier);

        if (extID != kPictureCoding) {
            ERROR("Wrong extension id type");
            bitReaderDeInit();
            return false;
        }

        READ_BITS_OR_RETURN(4, &(m_pictureCodingExtension.f_code[0][0]));
        READ_BITS_OR_RETURN(4, &(m_pictureCodingExtension.f_code[0][1]));
        READ_BITS_OR_RETURN(4, &(m_pictureCodingExtension.f_code[1][0]));
        READ_BITS_OR_RETURN(4, &(m_pictureCodingExtension.f_code[1][1]));
        READ_BITS_OR_RETURN(2, &(m_pictureCodingExtension.intra_dc_precision));
        READ_BITS_OR_RETURN(2, &(m_pictureCodingExtension.picture_structure));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.top_field_first));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.frame_pred_frame_dct));
        READ_FLAG_OR_RETURN(
            &(m_pictureCodingExtension.concealment_motion_vectors));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.q_scale_type));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.intra_vlc_format));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.alternate_scan));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.repeat_first_field));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.chrome_420_type));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.progressive_frame));
        READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.composite_display_flag));

        if (m_pictureCodingExtension.composite_display_flag) {
            READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.v_axis));
            READ_BITS_OR_RETURN(3, &(m_pictureCodingExtension.field_sequence));
            READ_FLAG_OR_RETURN(&(m_pictureCodingExtension.sub_carrier));
            READ_BITS_OR_RETURN(7, &(m_pictureCodingExtension.burst_amplitude));
            READ_BITS_OR_RETURN(8,
                              &(m_pictureCodingExtension.sub_carrier_phase));

        }

        DEBUG("f_code_0_0                  : %x",
              m_pictureCodingExtension.f_code[0][0]);
        DEBUG("f_code_0_1                  : %x",
              m_pictureCodingExtension.f_code[0][1]);
        DEBUG("f_code_1_0                  : %x",
              m_pictureCodingExtension.f_code[1][0]);
        DEBUG("f_code_1_1                  : %x",
              m_pictureCodingExtension.f_code[1][1]);
        DEBUG("intra_dc_precision          : %x",
              m_pictureCodingExtension.intra_dc_precision);
        DEBUG("picture_structure           : %x",
              m_pictureCodingExtension.picture_structure);
        DEBUG("top_field_first             : %x",
              m_pictureCodingExtension.top_field_first);
        DEBUG("frame_pred_frame_dct        : %x",
              m_pictureCodingExtension.frame_pred_frame_dct);
        DEBUG("concealment_motion_vectors  : %x",
              m_pictureCodingExtension.concealment_motion_vectors);
        DEBUG("q_scale_type                : %x",
              m_pictureCodingExtension.q_scale_type);
        DEBUG("intra_vlc_format            : %x",
              m_pictureCodingExtension.intra_vlc_format);
        DEBUG("alternate_scan              : %x",
              m_pictureCodingExtension.alternate_scan);
        DEBUG("repeat_first_field          : %x",
              m_pictureCodingExtension.repeat_first_field);
        DEBUG("chrome_420_type             : %x",
              m_pictureCodingExtension.chrome_420_type);
        DEBUG("progressive_frame           : %x",
              m_pictureCodingExtension.progressive_frame);
        DEBUG("composite_display_flag      : %x",
              m_pictureCodingExtension.composite_display_flag);

        bitReaderDeInit();

        return true;
    }

    bool Parser::parseSequenceExtension(const StreamHeader* shdr)
    {
        ExtensionIdentifierType extID;
	const uint8_t *nalData= shdr->nalData;
	int32_t nalSize = shdr->nalSize;

        if (nalSize < (kStartCodeSize + 5)) {
            ERROR("Incomplete Sequence Extension");
            return false;
        }

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);
        SKIP_BYTE_OR_RETURN();

        READ_BITS_OR_RETURN(
            4, &(m_sequenceExtension.extension_start_code_identifier));

        extID = static_cast<ExtensionIdentifierType>(
            m_sequenceExtension.extension_start_code_identifier);

        if (extID != kSequence) {
            ERROR("Wrong extension id type %d", extID);
            bitReaderDeInit();
            return false;
        }

        READ_BITS_OR_RETURN(8,
                          &(m_sequenceExtension.profile_and_level_indication));
        READ_FLAG_OR_RETURN(&(m_sequenceExtension.progressive_sequence));
        READ_BITS_OR_RETURN(2, &(m_sequenceExtension.chroma_format));
        READ_BITS_OR_RETURN(2, &(m_sequenceExtension.horizontal_size_extension));
        READ_BITS_OR_RETURN(2, &(m_sequenceExtension.vertical_size_extension));
        READ_BITS_OR_RETURN(12, &(m_sequenceExtension.bit_rate_extension));

        READ_MARKER_OR_RETURN(true);

        READ_BITS_OR_RETURN(8, &(m_sequenceExtension.vbv_buffer_size_extension));
        READ_FLAG_OR_RETURN(&(m_sequenceExtension.low_delay));
        READ_BITS_OR_RETURN(2, &(m_sequenceExtension.frame_rate_extension_n));
        READ_BITS_OR_RETURN(5, &(m_sequenceExtension.frame_rate_extension_d));

        DEBUG("extension_start_code_identifier  : %x",
              m_sequenceExtension.extension_start_code_identifier);
        DEBUG("profile_and_level_indication     : %x",
              m_sequenceExtension.profile_and_level_indication);
        DEBUG("progressive_sequence             : %x",
              m_sequenceExtension.progressive_sequence);
        DEBUG("chroma_format                    : %x",
              m_sequenceExtension.chroma_format);
        DEBUG("horizontal_size_extension        : %x",
              m_sequenceExtension.horizontal_size_extension);
        DEBUG("vertical_size_extension          : %x",
              m_sequenceExtension.vertical_size_extension);
        DEBUG("bit_rate_extension               : %x",
              m_sequenceExtension.bit_rate_extension);
        DEBUG("vbv_buffer_size_extension        : %x",
              m_sequenceExtension.vbv_buffer_size_extension);
        DEBUG("low_delay                        : %x",
              m_sequenceExtension.low_delay);
        DEBUG("frame_rate_extension_n           : %x",
              m_sequenceExtension.frame_rate_extension_n);
        DEBUG("frame_rate_extension_d           : %x",
              m_sequenceExtension.frame_rate_extension_d);

        bitReaderDeInit();

        return true;
    }

    bool Parser::parseSequenceHeader(const StreamHeader* shdr)
    {
        const uint8_t* nalData = shdr->nalData;
        int32_t nalSize = shdr->nalSize;
        QuantMatrices* quantMatrices = &m_sequenceHdr.quantizationMatrices;

        if (nalSize < (kStartCodeSize + 7)) {
            ERROR("Incomplete Sequence Header");
            return false;
        }

        memset(&m_sequenceHdr, 0, sizeof(SeqHeader));

        BitReader bitReader(nalData, nalSize);
        bitReaderInit(&bitReader);

        SKIP_BYTE_OR_RETURN();

        READ_BITS_OR_RETURN(12, &(m_sequenceHdr.horizontal_size_value));
        READ_BITS_OR_RETURN(12, &(m_sequenceHdr.vertical_size_value));
        READ_BITS_OR_RETURN(4, &(m_sequenceHdr.aspect_ratio_info));
        READ_BITS_OR_RETURN(4, &(m_sequenceHdr.frame_rate_code));
        READ_BITS_OR_RETURN(18, &(m_sequenceHdr.bit_rate_value));

        READ_MARKER_OR_RETURN(true);

        READ_BITS_OR_RETURN(10, &(m_sequenceHdr.vbv_buffer_size_value));
        READ_FLAG_OR_RETURN(&(m_sequenceHdr.constrained_params_flag));

        if (!readQuantMatrixOrDefault(quantMatrices->load_intra_quantiser_matrix,
                                 quantMatrices->intra_quantiser_matrix,
                                 &kDefaultIntraBlockMatrix[0]))
            return false;

        if (!readQuantMatrixOrDefault(quantMatrices->load_non_intra_quantiser_matrix,
                                 quantMatrices->non_intra_quantiser_matrix,
                                 &kDefaultNonIntraBlockMatrix[0]));

        DEBUG("horizontal_size_value            : %x",
              m_sequenceHdr.horizontal_size_value);
        DEBUG("vertical_size_value              : %x",
              m_sequenceHdr.vertical_size_value);
        DEBUG("aspect_ratio_info                : %x",
              m_sequenceHdr.aspect_ratio_info);
        DEBUG("frame_rate_code                  : %x",
              m_sequenceHdr.frame_rate_code);
        DEBUG("bit_rate_value                   : %x",
              m_sequenceHdr.bit_rate_value);
        DEBUG("vbv_buffer_size_value            : %x",
              m_sequenceHdr.vbv_buffer_size_value);
        DEBUG("constrained_params_flag          : %x",
              m_sequenceHdr.constrained_params_flag);
        DEBUG("load_intra_quantiser_matrix      : %x",
              quantMatrices->load_intra_quantiser_matrix);
        DEBUG("load_non_intra_quantiser_matrix  : %x",
              quantMatrices->load_non_intra_quantiser_matrix);

        bitReaderDeInit();

        return true;
    }

} // namespace MPEG2
} // namespace YamiParser
