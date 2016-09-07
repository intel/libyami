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

// This file contains an implementation of a Mpeg2 raw stream parser,
// as defined in ISO/IEC 13818-2
//
// parser is highly influenced on the way other parsers are written in
// this project in particular vp8_parser.
//
// The parser is built under the logic that the client will separate the
// input stream in chunks of information separated by start codes.
// Client can decide if partial start_code streams are permitted and owns
// the decision to parse partial information

#ifndef mpeg2_parser_h
#define mpeg2_parser_h

// local headers
#include "common/NonCopyable.h"
#include "bitReader.h"
#include "VideoCommonDefs.h"

// system headers
#include <stdio.h>
#include <string.h>

namespace YamiParser {
namespace MPEG2 {

    // See spec for definitions of values/fields.

    enum PictureCodingType {
        kIFrame = 1,
        kPFrame,
        kBFrame,
    };

    enum StartCodeSize {
        kStartCodePrefixSize = 0,
        kStartCodeSize,
    };

    enum ExtensionIdentifierType {
        kSequence = 1,
        kSequenceDisplay,
        kQuantizationMatrix,
        kSequenceScalable = 5,
        kPictureDisplay = 7,
        kPictureCoding,
        kPictureSpatialScalable,
        kPictureTempralScalable,
    };

    enum StartCodeType {
        MPEG2_INVALID_START_CODE = -1,
        MPEG2_PICTURE_START_CODE = 0x00,
        MPEG2_SLICE_START_CODE_MIN,
        MPEG2_SLICE_START_CODE_MAX = 0xaf,
        MPEG2_RESERVED_CODE_0,
        MPEG2_RESERVED_CODE_1,
        MPEG2_USER_DATA_START_CODE,
        MPEG2_SEQUENCE_HEADER_CODE,
        MPEG2_SEQUENCE_ERROR_CODE,
        MPEG2_EXTENSION_START_CODE,
        MPEG2_RESERVED_CODE_2,
        MPEG2_SEQUENCE_END_CODE,
        MPEG2_GROUP_START_CODE,
    };

    enum SystemStartCodeType {
        MPEG2_PROGRAM_END_CODE = 0xb9,
        MPEG2_PACK_HEADER_CODE,
        MPEG2_SYSTEM_HEADER_CODE,
        MPEG2_PROGRAM_STREAM_MAP_CODE,
        MPEG2_PRIVATE_STREAM_CODE_1,
        MPEG2_PADDING_STREAM_CODE,
        MPEG2_PRIVATE_STREAM_CODE_2,
        MPEG2_AUDIO_PES_CODE_MIN = 0xc0,
        MPEG2_AUDIO_PES_CODE_MAX = 0xdf,
        MPEG2_VIDEO_PES_CODE_MIN = 0xe0,
        MPEG2_VIDEO_PES_CODE_MAX = 0xef,
        MPEG2_ECM_STREAM_CODE = 0xf0,
        MPEG2_EMM_STREAM_CODE,
        MPEG2_DSMCC_STREAM_CODE,
        MPEG2_13522_STREAM_CODE,
        MPEG2_H_222_TYPE_A_CODE,
        MPEG2_H_222_TYPE_B_CODE,
        MPEG2_H_222_TYPE_C_CODE,
        MPEG2_H_222_TYPE_D_CODE,
        MPEG2_H_222_TYPE_E_CODE,

    };

    enum ParserResult {
        MPEG2_PARSER_OK = 0,
        MPEG2_PARSER_BROKEN_DATA,
        MPEG2_PARSER_ERROR,
    };

    enum ProfileType {
        MPEG2_PROFILE_HIGH = 1,
        MPEG2_PROFILE_SPATIALLY_SCALABLE,
        MPEG2_PROFILE_SNR_SCALABLE,
        MPEG2_PROFILE_MAIN,
        MPEG2_PROFILE_SIMPLE,
    };

    enum LevelType {
        MPEG2_LEVEL_HIGH = 4,
        MPEG2_LEVEL_HIGH_1440 = 6,
        MPEG2_LEVEL_MAIN = 8,
        MPEG2_LEVEL_LOW = 10,
    };

    struct StreamHeader {
        StreamHeader();

        const uint8_t* data;
        off_t streamSize;
        const uint8_t* nalData;
        int32_t nalSize;
        uint64_t time_stamp;
    };

    struct QuantMatrices {
        QuantMatrices();

        bool load_intra_quantiser_matrix;
        uint8_t intra_quantiser_matrix[64];
        bool load_non_intra_quantiser_matrix;
        uint8_t non_intra_quantiser_matrix[64];
        bool load_chroma_intra_quantiser_matrix;
        uint8_t chroma_intra_quantiser_matrix[64];
        bool load_chroma_non_intra_quantiser_matrix;
        uint8_t chroma_non_intra_quantiser_matrix[64];

    };

    // ISO/IEC spec section 6.2.2.1
    struct SeqHeader {
        SeqHeader();

        uint32_t horizontal_size_value;
        uint32_t vertical_size_value;
        uint32_t aspect_ratio_info;
        uint32_t frame_rate_code;
        uint32_t bit_rate_value;
        bool marker_bit;
        uint32_t vbv_buffer_size_value;
        bool constrained_params_flag;
        QuantMatrices quantizationMatrices;
    };

    // ISO/IEC spec section 6.2.2.3
    struct SeqExtension {
        SeqExtension();

        uint32_t extension_start_code_identifier;
        uint32_t profile_and_level_indication;
        bool progressive_sequence;
        uint32_t chroma_format;
        uint32_t horizontal_size_extension;
        uint32_t vertical_size_extension;
        uint32_t bit_rate_extension;
        bool marker_bit;
        uint32_t vbv_buffer_size_extension;
        bool low_delay;
        uint32_t frame_rate_extension_n;
        uint32_t frame_rate_extension_d;
    };

    // ISO/IEC spec section 6.2.2.6 and 6.3.9
    struct GOPHeader {
        GOPHeader();

        bool drop_frame_flag;
        uint32_t time_code_hours;
        uint32_t time_code_minutes;
        bool marker_bit;
        uint32_t time_code_seconds;
        uint32_t time_code_pictures;
        bool closed_gop;
        bool broken_link;
    };

    // ISO/IEC spec section 6.2.3
    struct PictureHeader {
        PictureHeader();

        uint32_t temporal_reference;
        uint32_t picture_coding_type;
        uint32_t vbv_delay;
        bool full_pel_forward_vector;
        uint32_t forward_f_code;
        bool full_pel_backward_vector;
        uint32_t backward_f_code;
        bool extra_bit_picture;
    };

    // ISO/IEC spec section 6.2.3.1
    struct PictureCodingExtension {
        PictureCodingExtension();

        uint32_t extension_start_code_identifier;
        uint32_t f_code[2][2]; // forward_horizontal
        // forward_vertical
        // backward_horizontal
        // backward_vertical
        uint32_t intra_dc_precision;
        uint32_t picture_structure;
        bool top_field_first;
        bool frame_pred_frame_dct;
        bool concealment_motion_vectors;
        bool q_scale_type;
        bool intra_vlc_format;
        bool alternate_scan;
        bool repeat_first_field;
        bool chrome_420_type;
        bool progressive_frame;
        bool composite_display_flag;
        bool v_axis;
        uint32_t field_sequence;
        bool sub_carrier;
        uint32_t burst_amplitude;
        uint32_t sub_carrier_phase;
    };

    // ISO/IEC spec section 6.2.3.2
    struct QuantMatrixExtension {
        QuantMatrixExtension();

        uint32_t extension_start_code_identifier;
        QuantMatrices quantizationMatrices;
    };

    struct Slice {
        Slice();

        // helper variables
        const uint8_t* sliceData;
        uint64_t sliceDataSize;
        uint64_t sliceHeaderSize; // in bits
        uint32_t verticalPosition;
        uint32_t macroblockRow;
        uint32_t macroblockColumn;
        // spec variables
        uint32_t slice_vertical_position_extension;
        uint32_t priority_breakpoint;
        uint32_t quantiser_scale_code;
        bool intra_slice_flag;
        bool intra_slice;
        uint32_t reserved_bits;
        bool extra_bit_slice;
        uint32_t extra_information_slice;
    };

    // A parser for raw Mpeg2 streams as specified in ISO/IEC 13818-2.
    class Parser {
    public:
        Parser();
        ~Parser();

        // Try to parse exactly the information of interest in a Mpeg2 raw
        // stream
        // Client can seek for the next start code and then parse it as required

        // parseSequenceHeader will parse a SequenceHeader according to the spec
        // storing the information in the m_sequenceHdr structure and updating
        // the position to the last bit parsed. information will be returned
        bool parseSequenceHeader(const StreamHeader* shdr);

        // parseSequenceExtension will parse a SequenceExtension according to
        // the
        // spec
        // storing the information in the sequence_extension structure and
        // updating
        // the position to the last bit parsed.
        bool parseSequenceExtension(const StreamHeader* shdr);

        // parseGOPHeader will parse a GOPHeader according to the spec storing
        // the information in the gop_header_ structure and updating the
        // position
        // to the last bit parsed.
        bool parseGOPHeader(const StreamHeader* shdr);

        // parsePictureHeader will parse a PictureHeader according to the spec
        // storing
        // the information in the m_pictureHeader_ structure and updating the
        // position
        // to the last bit parsed.
        bool parsePictureHeader(const StreamHeader* shdr);

        // parsePictureCodingExtension will parse a PictureHeader according to
        // the
        // spec storing the information in the picture_coding_extension_
        // structure
        // and updating the position to the last bit parsed.
        bool parsePictureCodingExtension(const StreamHeader* shdr);

        // parseQuantMatrixExtension will parse a Quant Matrix Extension ID
	// within a Extension Start Code and keep it on the quantization
	// Extension structure for later use
        bool parseQuantMatrixExtension(const StreamHeader* shdr);

        // parseSlice will parse a Slice according to the spec storing the
        // information
        // in the Slice structure and updating the position to the last
        // bit parsed.
        bool parseSlice(const StreamHeader* shdr);

        // nextStartCodeNew will update with next start code and return true if one
        // is found,
        bool nextStartCode(const StreamHeader* shdr, StartCodeType& start_code);


        const SeqHeader* getSequenceHeader() { return &m_sequenceHdr; }
        const SeqExtension* getSequenceExtension()
        {
            return &m_sequenceExtension;
        }
        const GOPHeader* getGOPHeader() { return &m_GOPHeader; }
        const PictureHeader* getPictureHeader() { return &m_pictureHeader; }
        const PictureCodingExtension* getPictureCodingExtension()
        {
            return &m_pictureCodingExtension;
        }
        const QuantMatrixExtension* getQuantMatrixExtension()
        {
            return &m_quantMatrixExtension;
        }
        const Slice* getMPEG2Slice() { return &m_slice; }

    private:
        friend class MPEG2ParserTest;

        bool readQuantMatrixOrDefault(bool& loadMatrix, uint8_t matrix[],
                                      const uint8_t defaultMatrix[]);

        bool readQuantMatrix(bool& loadMatrix, uint8_t matrix[]);
        bool calculateMBColumn();

        // bitReader functions

        inline void bitReaderInit(const BitReader* bitReader)
        {
            m_bitReader = const_cast<BitReader*>(bitReader);
        }

        inline void bitReaderDeInit() { m_bitReader = NULL; }

        inline bool bitReaderSkipBits(uint32_t num_bits) const
        {
            return m_bitReader->skip(num_bits);
        }

        inline bool bitReaderReadBits(uint32_t num_bits, uint32_t* out) const
        {
            return m_bitReader->readT(*out, num_bits);
        }

        inline bool bitReaderPeekBits(uint32_t num_bits, uint32_t* out) const
        {
            return m_bitReader->peek(*out, num_bits);
        }

        inline bool bitReaderPeekBool(bool* out) const
        {
            return m_bitReader->peek(*out, 1);
        }

        inline bool bitReaderReadMarker(bool value) const
        {
            bool readMarker, ret;
            // read 1 bit marker
            ret = m_bitReader->readT(readMarker);
            return ret && readMarker == value;
        }

        inline bool bitReaderReadFlag(bool* flag) const
        {
            return m_bitReader->readT(*flag);
        }

        inline uint64_t bitReaderCurrentPosition() const
        {
            return m_bitReader->getPos();
        }

        inline bool bitReaderIsByteAligned() const
        {
            return !(m_bitReader->getPos() % 8);
        }

        BitReader* m_bitReader;

        SeqHeader m_sequenceHdr;
        SeqExtension m_sequenceExtension;
        GOPHeader m_GOPHeader;
        PictureHeader m_pictureHeader;
        PictureCodingExtension m_pictureCodingExtension;
        QuantMatrixExtension m_quantMatrixExtension;
        Slice m_slice;

        DISALLOW_COPY_AND_ASSIGN(Parser);
    };

} // namespace MPEG2
} // namespace YamiParser

#endif // mpeg2_parser_h
