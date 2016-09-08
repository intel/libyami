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

#ifndef jpegParser_h
#define jpegParser_h

// library headers
#include "bitReader.h"
#include "common/Array.h"
#include "common/Functional.h"
#include "interface/VideoCommonDefs.h"

// system headers
#include <map>
#include <vector>

namespace YamiParser {
namespace JPEG {

const size_t DCTSIZE = 8;
const size_t DCTSIZE2 = 64;
const size_t NUM_QUANT_TBLS = 4;
const size_t NUM_HUFF_TBLS = 4;
const size_t NUM_ARITH_TBLS = 16;
const size_t MAX_COMPS_IN_SCAN = 4;

/**
 * JPEG marker codes
 */
enum Marker {
    M_SOF0  = 0xc0, M_SOF1  = 0xc1, M_SOF2  = 0xc2, M_SOF3  = 0xc3,
    M_SOF5  = 0xc5, M_SOF6  = 0xc6, M_SOF7  = 0xc7, M_JPG   = 0xc8,
    M_SOF9  = 0xc9, M_SOF10 = 0xca, M_SOF11 = 0xcb, M_SOF13 = 0xcd,
    M_SOF14 = 0xce, M_SOF15 = 0xcf, M_DHT   = 0xc4, M_DAC   = 0xcc,
    M_RST0  = 0xd0, M_RST1  = 0xd1, M_RST2  = 0xd2, M_RST3  = 0xd3,
    M_RST4  = 0xd4, M_RST5  = 0xd5, M_RST6  = 0xd6, M_RST7  = 0xd7,
    M_SOI   = 0xd8, M_EOI   = 0xd9, M_SOS   = 0xda, M_DQT   = 0xdb,
    M_DNL   = 0xdc, M_DRI   = 0xdd, M_DHP   = 0xde, M_EXP   = 0xdf,
    M_APP0  = 0xe0, M_APP1  = 0xe1, M_APP2  = 0xe2, M_APP3  = 0xe3,
    M_APP4  = 0xe4, M_APP5  = 0xe5, M_APP6  = 0xe6, M_APP7  = 0xe7,
    M_APP8  = 0xe8, M_APP9  = 0xe9, M_APP10 = 0xea, M_APP11 = 0xeb,
    M_APP12 = 0xec, M_APP13 = 0xed, M_APP14 = 0xee, M_APP15 = 0xef,
    M_JPG0  = 0xf0, M_JPG13 = 0xfd, M_COM   = 0xfe, M_ERROR = 0x100
};

struct QuantTable {
    typedef std::shared_ptr<QuantTable> Shared;

    std::array<uint16_t, DCTSIZE2> values;
    int precision;
};

typedef std::array<QuantTable::Shared, NUM_QUANT_TBLS> QuantTables;

struct HuffTable {
    typedef std::shared_ptr<HuffTable> Shared;

    std::array<uint8_t, 16> codes;
    std::array<uint8_t, 256> values;
};

typedef std::array<HuffTable::Shared, NUM_HUFF_TBLS> HuffTables;

struct Component {
    typedef std::shared_ptr<Component> Shared;

    int id;
    int index;
    int hSampleFactor;
    int vSampleFactor;
    int quantTableNumber;
    int dcTableNumber;
    int acTableNumber;
};

typedef std::vector<Component::Shared> Components;
typedef std::array<Component::Shared, MAX_COMPS_IN_SCAN> CurrComponents;

struct Segment {
    Segment() : marker(M_ERROR), position(0), length(0) { }

    Marker marker;
    uint32_t position;
    uint32_t length;
};

struct FrameHeader {
    typedef std::shared_ptr<FrameHeader> Shared;

    bool isBaseline;
    bool isProgressive;
    bool isArithmetic;
    int dataPrecision;
    unsigned imageHeight;
    unsigned imageWidth;
    int maxVSampleFactor;
    int maxHSampleFactor;
    Components components;
};

struct ScanHeader {
    typedef std::shared_ptr<ScanHeader> Shared;

    CurrComponents components;
    size_t numComponents;
    int ss;
    int se;
    int ah;
    int al;
};

typedef std::array<uint8_t, NUM_ARITH_TBLS> ArithmeticTable;

class Defaults {
public:
    static const Defaults& instance() { return s_instance; }
    const HuffTables& acHuffTables() const { return m_acHuffTables; }
    const HuffTables& dcHuffTables() const { return m_dcHuffTables; }

    /**
     * @return the default luminance and chrominance quantization
     * tables in zigzag order.
     */
    const QuantTables& quantTables() const { return m_quantTables; }

private:
    Defaults();

    static const Defaults s_instance;

    HuffTables m_acHuffTables;
    HuffTables m_dcHuffTables;
    QuantTables m_quantTables;
};

class Parser {
public:
    typedef std::shared_ptr<Parser> Shared;

    enum CallbackResult {
        ParseContinue,
        ParseSuspend
    };

    typedef std::function<CallbackResult (void)> Callback;
    typedef std::vector<Callback> CallbackList;
    typedef std::map<enum Marker, CallbackList> Callbacks;

    Parser(const uint8_t* data, uint32_t size);

    virtual ~Parser() { }

    /**
     * Parses the JPEG byte data.  Notifies registered callbacks after each
     * successfully parsed JPEG segment Marker.  If a registered Callback
     * returns ParseContinue, then this method continues parsing the JPEG byte
     * data.  If a Callback returns ParseSuspend, then this method returns
     * control to the caller whom can call this method again to resume parsing
     * the JPEG byte data.
     *
     * @retval true if parsing is successful
     * @retval false if parsing is unsuccessful
     */
    bool parse();

    /**
     * Register a Callback function for Marker. The Callback is called after
     * the Marker is parsed by the parse() method.
     */
    void registerCallback(const Marker&, const Callback&);

    /**
     * Register a single Callback for all SOFn markers.
     */
    void registerStartOfFrameCallback(const Callback&);

    /**
     * @return the most current Segment parsed by the parse() method.
     */
    const Segment& current() const { return m_current; }
    const FrameHeader::Shared& frameHeader() const { return m_frameHeader; }
    const ScanHeader::Shared& scanHeader() const { return m_scanHeader; }
    const QuantTables& quantTables() const { return m_quantTables; }
    const HuffTables& dcHuffTables() const { return m_dcHuffTables; }
    const HuffTables& acHuffTables() const { return m_acHuffTables; }
    unsigned restartInterval() const { return m_restartInterval; }

private:
    friend class JPEGParserTest;

    bool firstMarker();
    bool nextMarker();
    bool skipBytes(const uint32_t);
    uint32_t currentBytePosition() const { return m_input.getPos() >> 3; }
    CallbackResult notifyCallbacks() const;

    bool parseSOI();
    bool parseAPP();
    bool parseSOF(bool, bool, bool);
    bool parseSOS();
    bool parseEOI();
    bool parseDAC();
    bool parseDQT();
    bool parseDHT();
    bool parseDRI();

    BitReader m_input;

    const uint8_t* m_data;
    uint32_t m_size;

    Segment m_current;
    FrameHeader::Shared m_frameHeader;
    ScanHeader::Shared m_scanHeader;

    QuantTables m_quantTables;
    HuffTables m_dcHuffTables;
    HuffTables m_acHuffTables;

    ArithmeticTable m_arithDCL;
    ArithmeticTable m_arithDCU;
    ArithmeticTable m_arithACK;

    Callbacks m_callbacks;

    bool m_sawSOI;
    bool m_sawEOI;

    unsigned m_restartInterval;
};

} // namespace JPEG
} // namespace YamiParser

#endif // jpegParser_h
