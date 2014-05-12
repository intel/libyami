/*
 *  gstvaapidecoder_jpeg.h - Jpeg decoder
 *
 *  Copyright (C) 2011-2013 Intel Corporation
 *    Author: Wind Yuan <feng.yuan@intel.com>
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
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

#ifndef VAAPI_DECODER_Jpeg_H
#define VAAPI_DECODER_Jpeg_H

#include "vaapipicture.h"
#include "vaapidecoder_base.h"
#include "codecparsers/jpegparser.h"

class VaapiSliceJpeg:public VaapiSlice {
  public:
    VaapiSliceJpeg(VADisplay display,
                   VAContextID ctx,
                   uint8_t * sliceData, uint32_t sliceSize);

    ~VaapiSliceJpeg();
    // JpegScanHdr slice_hdr;
};

class VaapiDecoderJpeg:public VaapiDecoderBase {
  public:
    VaapiDecoderJpeg(const char *mimeType);
    virtual ~ VaapiDecoderJpeg();
    virtual Decode_Status start(VideoConfigBuffer * buffer);
    virtual Decode_Status reset(VideoConfigBuffer * buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer * buf);

  private:
    Decode_Status parseFrameHeader(uint8_t * buf, uint32_t bufSize);
    Decode_Status parseHuffmanTable(uint8_t * buf, uint32_t bufSize);
    Decode_Status parseQuantTable(uint8_t * buf, uint32_t bufSize);
    Decode_Status parseRestartInterval(uint8_t * buf, uint32_t bufSize);
    Decode_Status parseScanHeader(JpegScanHdr * scanHdr, int8_t * buf,
                                  uint32_t bufSize);
    Decode_Status fillSliceParam(JpegScanHdr * scanHdr, uint8_t * scanData,
                                 uint32_t scanDataSize);
    Decode_Status fillPictureParam();
    Decode_Status fillQuantizationTable();
    Decode_Status fillHuffmanTable();

    Decode_Status decodePictureStart();
    Decode_Status decodePictureEnd();

  private:
    VaapiProfile m_profile;
    uint32_t m_width;
    uint32_t m_height;
    VaapiPicture *m_picture;
    JpegFrameHdr m_frameHdr;
    JpegHuffmanTables m_hufTables;
    JpegQuantTables m_quantTables;
    bool m_hasContext;
    bool m_hasHufTable;
    bool m_hasQuantTable;
    uint32_t m_mcuRestart;
};

typedef struct _JpegScanSegment {
    uint32_t m_headerOffset;
    uint32_t m_headerSize;
    uint32_t m_dataOffset;
    uint32_t m_dataSize;
    uint32_t m_isValid;
} JpegScanSegment;

#endif                          /* VAAPI_DECODER_Jpeg_H */
