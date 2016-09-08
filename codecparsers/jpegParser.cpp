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
 *
 * ----
 *
 * Part of IJG's libjpeg library (primarily jdmarker.c) was used as a reference
 * while implementing this parser.  This implementation loosely reproduces some
 * of the parsing logic found in the libjpeg jdmarker.c file.  Although, this
 * logic has been refactored using C++-style syntax and data structures and
 * adjusted appropriately to fit into the overall libyami framework.  Therefore,
 * this implementation is considered to be partially derived from IJG's libjpeg.
 *
 * The following license preamble, below, is reproduced from libjpeg's
 * jdmarker.c file.  The README.ijg is also provided with this file:
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * Modified 2009-2013 by Guido Vollbeding.
 * The jdmarker.c file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README.ijg file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// primary header
#include "jpegParser.h"

// library headers
#include "common/log.h"

// system headers
#include <algorithm>
#include <cstring>

namespace YamiParser {
namespace JPEG {

#define INPUT_BYTE(var, action) \
    do { \
        if (m_input.end()) { \
            action; \
        } \
        var = m_input.read(8); \
    } while(0)

#define INPUT_2BYTES(var, action) \
    do { \
        uint16_t b1, b2; \
        INPUT_BYTE(b1, action); \
        INPUT_BYTE(b2, action); \
        var = ((b1 << 8) | b2) & 0xffff; \
    } while(0)

const Defaults Defaults::s_instance;

Defaults::Defaults()
    : m_acHuffTables()
    , m_dcHuffTables()
    , m_quantTables()
{
    // The following tables are from
    // https://www.w3.org/Graphics/JPEG/itu-t81.pdf

    { // K.3.3.1 Luminance DC coefficients
        std::array<uint8_t, 16> codes = {
            0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        std::array<uint8_t, 256> values = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b
        };

        m_dcHuffTables[0].reset(new HuffTable());
        m_dcHuffTables[0]->codes.swap(codes);
        m_dcHuffTables[0]->values.swap(values);
    }

    { // K.3.3.1 Chrominance DC coefficients
        std::array<uint8_t, 16> codes = {
            0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        std::array<uint8_t, 256> values = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b
        };

        m_dcHuffTables[1].reset(new HuffTable());
        m_dcHuffTables[1]->codes.swap(codes);
        m_dcHuffTables[1]->values.swap(values);
    }

    { // K.3.3.2 Luminance AC coefficients
        std::array<uint8_t, 16> codes = {
            0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
            0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d
        };
        std::array<uint8_t, 256> values = {
            0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
            0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
            0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
            0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
            0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
            0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
            0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
            0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
            0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
            0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
            0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
            0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
            0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
            0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
            0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
            0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
            0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
            0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
            0xf9, 0xfa
        };

        m_acHuffTables[0].reset(new HuffTable());
        m_acHuffTables[0]->codes.swap(codes);
        m_acHuffTables[0]->values.swap(values);
    }

    { // K.3.3.2 Chrominance AC coefficients
        std::array<uint8_t, 16> codes = {
            0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
            0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77
        };
        std::array<uint8_t, 256> values = {
            0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
            0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
            0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
            0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
            0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
            0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
            0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
            0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
            0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
            0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
            0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
            0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
            0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
            0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
            0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
            0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
            0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
            0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
            0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
            0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
            0xf9, 0xfa
        };

        m_acHuffTables[1].reset(new HuffTable());
        m_acHuffTables[1]->codes.swap(codes);
        m_acHuffTables[1]->values.swap(values);
    }

    static const std::array<uint8_t, 64> zigzag8x8 = {
        0,   1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };

    { // Table K.1 Luminance quantization table values
        std::array<uint16_t, DCTSIZE2> values = {
            16,  11,  10,  16,  24,  40,  51,  61,
            12,  12,  14,  19,  26,  58,  60,  55,
            14,  13,  16,  24,  40,  57,  69,  56,
            14,  17,  22,  29,  51,  87,  80,  62,
            18,  22,  37,  56,  68, 109, 103,  77,
            24,  35,  55,  64,  81, 104, 113,  92,
            49,  64,  78,  87, 103, 121, 120, 101,
            72,  92,  95,  98, 112, 100, 103,  99
        };

        m_quantTables[0].reset(new QuantTable());
        for (size_t i(0); i < DCTSIZE2; ++i)
            m_quantTables[0]->values[i] = values[zigzag8x8[i]];
        m_quantTables[0]->precision = 0; // 1-byte precision
    }

    { // Table K.2 Chrominance quantization table values
        std::array<uint16_t, DCTSIZE2> values = {
            17,  18,  24,  47,  99,  99,  99,  99,
            18,  21,  26,  66,  99,  99,  99,  99,
            24,  26,  56,  99,  99,  99,  99,  99,
            47,  66,  99,  99,  99,  99,  99,  99,
            99,  99,  99,  99,  99,  99,  99,  99,
            99,  99,  99,  99,  99,  99,  99,  99,
            99,  99,  99,  99,  99,  99,  99,  99,
            99,  99,  99,  99,  99,  99,  99,  99
        };

        m_quantTables[1].reset(new QuantTable());
        for (size_t i(0); i < DCTSIZE2; ++i)
            m_quantTables[1]->values[i] = values[zigzag8x8[i]];
        m_quantTables[1]->precision = 0; // 1-byte precision
    }
}

Parser::Parser(const uint8_t* data, const uint32_t size)
    : m_input(data, size)
    , m_data(data)
    , m_size(size)
    , m_current()
    , m_frameHeader()
    , m_scanHeader()
    , m_quantTables()
    , m_dcHuffTables()
    , m_acHuffTables()
    , m_arithDCL()
    , m_arithDCU()
    , m_arithACK()
    , m_callbacks()
    , m_sawSOI(false)
    , m_sawEOI(false)
    , m_restartInterval(0)
{
}

bool Parser::skipBytes(const uint32_t nBytes)
{
    if ((static_cast<uint64_t>(nBytes) << 3)
            > m_input.getRemainingBitsCount()) {
        ERROR("Not enough bytes in stream");
        return false;
    }

    // BitReader only supports skipping a max of CACHEBYTES at a time
    // for each call to skip.
    const uint32_t nSkips = nBytes / BitReader::CACHEBYTES;
    const uint32_t rem = nBytes % BitReader::CACHEBYTES;
    for (uint32_t i(0); i < nSkips; ++i)
        m_input.skip(BitReader::CACHEBYTES << 3);
    m_input.skip(rem << 3);

    return true;
}

void Parser::registerCallback(const Marker& marker, const Callback& callback)
{
    m_callbacks[marker].push_back(callback);
}

void Parser::registerStartOfFrameCallback(const Callback& callback)
{
    static const std::array<const Marker, 13> sofMarkers = {
        M_SOF0, M_SOF1 , M_SOF2 , M_SOF3 , M_SOF5 , M_SOF6 , M_SOF7,
        M_SOF9, M_SOF10, M_SOF11, M_SOF13, M_SOF14, M_SOF15 };

    for (size_t i(0); i < 13; ++i)
        registerCallback(sofMarkers[i], callback);
}

Parser::CallbackResult Parser::notifyCallbacks() const
{
    const Callbacks::const_iterator match(m_callbacks.find(m_current.marker));
    if (match != m_callbacks.end()) {
        const CallbackList& callbacks = match->second;
        const size_t nCallbacks = callbacks.size();
        for (size_t i(0); i < nCallbacks; ++i)
            if (callbacks[i]() == ParseSuspend)
                return ParseSuspend;
    }
    return ParseContinue;
}

bool Parser::firstMarker()
{
    uint32_t c, c2;

    INPUT_BYTE(c, return false);
    INPUT_BYTE(c2, return false);
    if (c != 0xFF || c2 != M_SOI) {
        ERROR("No SOI found. Not a JPEG");
        return false;
    }

    m_current.marker = static_cast<Marker>(c2);
    m_current.position = currentBytePosition() - 1;
    m_current.length = 0; // set by marker parse routines when appropriate

    return true;
}

bool Parser::nextMarker()
{
    const uint8_t* const end = m_data + m_size - 1;
    const uint8_t* const start = m_data + currentBytePosition();
    const uint8_t* current = start;
    const uint8_t* match;

    do {
        match = std::find(current, end, 0xFF);
        if (match == end)
            return false;
        if (*(match + 1) < 0xFF && *(match + 1) >= M_SOF0)
            break;
        current = match + 1;
    } while (true);

    skipBytes(std::distance(start, match) + 1);

    m_current.marker = static_cast<Marker>(m_input.read(8));
    m_current.position = currentBytePosition() - 1;
    m_current.length = 0; // set by marker parse routines when appropriate

    return true;
}

bool Parser::parse()
{
    while (true) {
        if (!m_sawSOI) {
            if (!firstMarker()) {
                return false;
            }
        } else if (!nextMarker()) {
            return m_sawEOI;
        }

        DEBUG("%s (byte:0x%02x position:%u)", __func__,
            m_current.marker, m_current.position);

        bool parseSegment = false;

        switch (m_current.marker) {
        case M_SOI:
            parseSegment = parseSOI();
            break;
        case M_SOF0: // Baseline
            parseSegment = parseSOF(true, false, false);
            break;
        case M_SOF1: // Extended sequential, Huffman
            parseSegment = parseSOF(false, false, false);
            break;
        case M_SOF2: // Progressive, Huffman
            parseSegment = parseSOF(false, true, false);
            break;
        case M_SOF9: // Extended sequential, arithmetic
            parseSegment = parseSOF(false, false, true);
            break;
        case M_SOF10: // Progressive, arithmetic
            parseSegment = parseSOF(false, true, true);
            break;
        case M_DAC:
            parseSegment = parseDAC();
            break;
        case M_DHT:
            parseSegment = parseDHT();
            break;
        case M_DQT:
            parseSegment = parseDQT();
            break;
        case M_DRI:
            parseSegment = parseDRI();
            break;
        case M_SOS:
            parseSegment = parseSOS();
            break;
        case M_EOI:
            parseSegment = parseEOI();
            break;

        case M_APP0:
        case M_APP1:
        case M_APP2:
        case M_APP3:
        case M_APP4:
        case M_APP5:
        case M_APP6:
        case M_APP7:
        case M_APP8:
        case M_APP9:
        case M_APP10:
        case M_APP11:
        case M_APP12:
        case M_APP13:
        case M_APP14:
        case M_APP15:
            parseSegment = parseAPP();
            break;

        case M_COM:
        case M_RST0:
        case M_RST1:
        case M_RST2:
        case M_RST3:
        case M_RST4:
        case M_RST5:
        case M_RST6:
        case M_RST7:
            parseSegment = true;
            break;

        /* Currently unsupported SOFn types */
        case M_SOF3:  // Lossless, Huffman
        case M_SOF5:  // Differential sequential, Huffman
        case M_SOF6:  // Differential progressive, Huffman
        case M_SOF7:  // Differential lossless, Huffman
        case M_JPG:   // Reserved for JPEG extensions
        case M_SOF11: // Lossless, arithmetic
        case M_SOF13: // Differential sequential, arithmetic
        case M_SOF14: // Differential progressive, arithmetic
        case M_SOF15: // Differential lossless, arithmetic
            //unsupported
            ERROR("Unsupported marker encountered: 0x%02x", m_current.marker);
            return false;

        default:
            // unknown or unhandled marker
            ERROR("Unknown or unhandled marker: 0x%02x", m_current.marker);
            return false;
        }

        if (!parseSegment)
            return false;

        if (notifyCallbacks() == ParseSuspend)
            return true;
    }
}

bool Parser::parseSOI()
{
    if (m_sawSOI) {
        ERROR("Duplicate SOI encountered");
        return false;
    }

    m_sawSOI = true;

    return true;
}

bool Parser::parseAPP()
{
    INPUT_2BYTES(m_current.length, return false);

    return skipBytes(m_current.length - 2);
}

bool Parser::parseSOF(bool isBaseline, bool isProgressive, bool isArithmetic)
{
    if (m_frameHeader) {
        ERROR("Duplicate SOF encountered");
        return false;
    }

    INPUT_2BYTES(m_current.length, return false);

    m_frameHeader.reset(new FrameHeader);

    m_frameHeader->isBaseline = isBaseline;
    m_frameHeader->isProgressive = isProgressive;
    m_frameHeader->isArithmetic = isArithmetic;

    INPUT_BYTE(m_frameHeader->dataPrecision, return false);
    INPUT_2BYTES(m_frameHeader->imageHeight, return false);
    INPUT_2BYTES(m_frameHeader->imageWidth, return false);

    uint32_t numComponents;

    INPUT_BYTE(numComponents, return false);

    DEBUG("baseline      : %d", m_frameHeader->isBaseline);
    DEBUG("progressive   : %d", m_frameHeader->isProgressive);
    DEBUG("arithmetic    : %d", m_frameHeader->isArithmetic);
    DEBUG("precision     : %d", m_frameHeader->dataPrecision);
    DEBUG("image width   : %u", m_frameHeader->imageWidth);
    DEBUG("image height  : %u", m_frameHeader->imageHeight);
    DEBUG("num components: %d", numComponents);

    if (m_frameHeader->imageWidth <= 0 || m_frameHeader->imageHeight <= 0
            || numComponents <= 0) {
        ERROR("Empty image");
        return false;
    }

    if ((m_current.length - 8) != (numComponents * 3)
            || numComponents > MAX_COMPS_IN_SCAN) {
        ERROR("Bad length");
        return false;
    }

    m_frameHeader->components.resize(numComponents);
    m_frameHeader->maxHSampleFactor = 0;
    m_frameHeader->maxVSampleFactor = 0;

    for (size_t index(0); index < numComponents; ++index) {
        int factor;
        Component::Shared& component = m_frameHeader->components[index];
        component.reset(new Component);
        component->index = index;

        INPUT_BYTE(component->id, return false);
        INPUT_BYTE(factor, return false);

        component->hSampleFactor = (factor >> 4) & 15;
        component->vSampleFactor = factor & 15;

        if (component->hSampleFactor > m_frameHeader->maxHSampleFactor)
            m_frameHeader->maxHSampleFactor = component->hSampleFactor;

        if (component->vSampleFactor > m_frameHeader->maxVSampleFactor)
            m_frameHeader->maxVSampleFactor = component->vSampleFactor;

        INPUT_BYTE(component->quantTableNumber, return false);
    }

    return true;
}

static bool componentIdMatches(const int id, const Component::Shared& component)
{
    return id == component->id;
}

bool Parser::parseSOS()
{
    using std::placeholders::_1;

    if (!m_frameHeader) {
        ERROR("SOS Encountered before SOF");
        return false;
    }

    INPUT_2BYTES(m_current.length, return false);

    size_t numComponents;

    INPUT_BYTE(numComponents, return false);

    if (m_current.length != (numComponents * 2 + 6)
            || numComponents < 1
            || numComponents > MAX_COMPS_IN_SCAN) {
        ERROR("Invalid SOS Length");
        return false;
    }

    m_scanHeader.reset(new ScanHeader);

    m_scanHeader->numComponents = numComponents;

    for (size_t i(0); i < numComponents; ++i)
        m_scanHeader->components[i].reset();

    for (size_t i(0); i < numComponents; ++i) {
        int id;
        int c;

        INPUT_BYTE(id, return false);

        Components::iterator component = std::find_if(
            m_frameHeader->components.begin(),
            m_frameHeader->components.end(),
            std::bind(&componentIdMatches, id, _1));

        if (component == m_frameHeader->components.end()
                || m_scanHeader->components[(*component)->index]) {
            ERROR("Bad Component Id (%d)", id);
            return false;
        }

        INPUT_BYTE(c, return false);

        m_scanHeader->components[i] = *component;
        (*component)->dcTableNumber = (c >> 4) & 15;
        (*component)->acTableNumber = c & 15;

        /* This CSi (cc) should differ from the previous CSi */
        for (size_t pi(0); pi < i; ++pi) {
            if (m_scanHeader->components[pi] == (*component)) {
                ERROR("Bad Component Id (%d)", id);
                return false;
            }
        }
    }

    // The following are used only in progressive mode
    // FIXME: validate these values according to JPEG specification
    INPUT_BYTE(m_scanHeader->ss, return false);
    INPUT_BYTE(m_scanHeader->se, return false);
    INPUT_BYTE(m_scanHeader->ah, return false);

    m_scanHeader->al = m_scanHeader->ah & 15;
    m_scanHeader->ah = (m_scanHeader->ah >> 4) & 15;

    return true;
}

bool Parser::parseEOI()
{
    if (m_sawEOI) {
        ERROR("Duplicate EOI encountered");
        return false;
    }

    m_sawEOI = true;

    return true;
}

bool Parser::parseDAC()
{
    INPUT_2BYTES(m_current.length, return false);

    long length = m_current.length - 2;

    while (length > 0) {
        size_t index;
        int value;

        INPUT_BYTE(index, return false);
        INPUT_BYTE(value, return false);

        length -= 2;

        if (index < 0 || index >= (2 * NUM_ARITH_TBLS)) {
            ERROR("Invalid DAC Index");
            return false;
        }

        if (index >= NUM_ARITH_TBLS) {
            m_arithACK[index - NUM_ARITH_TBLS] = value;
        } else {
            m_arithDCL[index] = value & 15;
            m_arithDCU[index] = (value >> 4) & 15;
            if (m_arithDCL[index] > m_arithDCU[index]) {
                ERROR("Invalid DAC Value");
                return false;
            }
        }
    }

    if (length != 0) {
        ERROR("Invalid DAC Length");
        return false;
    }

    return true;
}

bool Parser::parseDQT()
{
    INPUT_2BYTES(m_current.length, return false);

    long length = m_current.length - 2;

    while (length > 0) {
        int c;

        INPUT_BYTE(c, return false);

        const size_t index = c & 15;
        const int precision = (c >> 4) & 15;

        if (index >= NUM_QUANT_TBLS) {
            ERROR("Invalid quant table index encountered");
            return false;
        }

        QuantTable::Shared& quantTable = m_quantTables[index];
        if (!quantTable)
            quantTable.reset(new QuantTable);

        quantTable->precision = precision;

        for (size_t i(0); i < DCTSIZE2; ++i) {
            if (precision)
                INPUT_2BYTES(quantTable->values[i], return false);
            else
                INPUT_BYTE(quantTable->values[i], return false);
        }

        const uint32_t nbytes = precision ? 2 : 1;
        length -= (DCTSIZE2 * nbytes) + 1;
    }

    if (length != 0) {
        ERROR("Bad DQT length");
        return false;
    }

    return true;
}

bool Parser::parseDHT()
{
    INPUT_2BYTES(m_current.length, return false);

    long length = m_current.length - 2;

    while (length > 16) {
        size_t index;

        INPUT_BYTE(index, return false);

        long count = 0;
        std::vector<uint8_t> codes(16, 0);
        for (size_t i(0); i < 16; ++i) {
            INPUT_BYTE(codes[i], return false);
            count += codes[i];
        }

        length -= 1 + 16;

        if (count > 256 || count > length) {
            ERROR("Bad Huff Table");
            return false;
        }

        std::vector<uint8_t> huffval(256, 0);
        for (long i(0); i < count; ++i)
            INPUT_BYTE(huffval[i], return false);

        length -= count;

        HuffTables* huffTables;
        if (index & 0x10) { /* AC table definition */
            index -= 0x10;
            huffTables = &m_acHuffTables;
        } else { /* DC table definition */
            huffTables = &m_dcHuffTables;
        }

        if (index < 0 || index >= NUM_HUFF_TBLS) {
            ERROR("Bad Huff Table Index");
            return false;
        }

        HuffTable::Shared& huffTable = (*huffTables)[index];
        if (!huffTable)
            huffTable.reset(new HuffTable);

        std::memcpy(&huffTable->codes[0], &codes[0],
                    sizeof(huffTable->codes));
        std::memcpy(&huffTable->values[0], &huffval[0],
                    sizeof(huffTable->values));
    }

    if (length != 0) {
        ERROR("Bad DHT Length");
        return false;
    }

    return true;
}

bool Parser::parseDRI()
{
    INPUT_2BYTES(m_current.length, return false);

    if (m_current.length != 4) {
        ERROR("Bad DRI Length");
        return false;
    }

    INPUT_2BYTES(m_restartInterval, return false);

    return true;
}

} // namespace JPEG
} // namespace YamiParser
