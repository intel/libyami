/*
 *  vaapiencoder_jpeg.h - jpeg encoder for va
 *
 *  Copyright (C) 2013-2014 Intel Corporation
 *    Author: Xin Tang <xin.t.tang@intel.com>
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
 *  Boston, 
 */
 
#ifndef vaapiencoder_jpeg_h
#define vaapiencoder_jpeg_h

#include "vaapiencoder_base.h"
#include "vaapi/vaapiptrs.h"
#include <tr1/memory>
#include <va/va_enc_jpeg.h>
#include "codecparsers/jpegparser.h"

namespace YamiMediaCodec {
class VaapiEncPictureJPEG;

class VaapiEncoderJpeg : public VaapiEncoderBase {
public:
    typedef std::tr1::shared_ptr<VaapiEncPictureJPEG> PicturePtr;
    
    VaapiEncoderJpeg();
    ~VaapiEncoderJpeg();
    virtual Encode_Status start();
    virtual void flush();
    virtual Encode_Status stop();

    virtual Encode_Status getParameters(VideoParamConfigType type, Yami_PTR videoEncParams);
    virtual Encode_Status setParameters(VideoParamConfigType type, Yami_PTR videoEncParams);
    virtual Encode_Status getMaxOutSize(uint32_t *maxSize);
    
protected:
    virtual Encode_Status doEncode(const SurfacePtr&, uint64_t timeStamp, bool forceKeyFrame);
    //virtual Encode_Status reorder(const SurfacePtr& , uint64_t timeStamp, bool forceKeyFrame = false);
    virtual Encode_Status submitEncode();
    virtual bool isBusy() { return false;};

private:
    Encode_Status encodePicture(const PicturePtr &);
    bool addSliceHeaders (const PicturePtr&) const;
    bool fill(VAEncPictureParameterBufferJPEG * picParam, const PicturePtr &, const SurfacePtr &) const;
    bool fill(VAQMatrixBufferJPEG * qMatrix) const;
    bool fill(VAEncSliceParameterBufferJPEG *sliceParam) const;
    bool fill(VAHuffmanTableBufferJPEGBaseline *huffTableParam) const;
    
    bool ensurePicture (const PicturePtr&, const SurfacePtr&);
    bool ensureQMatrix (const PicturePtr&);
    bool ensureSlice (const PicturePtr&);
    bool ensureHuffTable (const PicturePtr&);
    
    void resetParams();

    unsigned char quality;
};
}
#endif
