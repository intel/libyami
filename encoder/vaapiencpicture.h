/*
 *  vaapiencpicture.h - va encoder picture
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

#ifndef vaapiencpicture_h
#define vaapiencpicture_h

#include "interface/VideoEncoderDefs.h"

#include "vaapi/vaapipicture.h"


namespace YamiMediaCodec{
class VaapiEncPicture:public VaapiPicture {
  public:
    VaapiEncPicture(const ContextPtr& context,
                    const SurfacePtr & surface, int64_t timeStamp);
    virtual ~VaapiEncPicture() { }

    template < class T >
    bool editSequence(T * &seqParam);

    template < class T >
    bool editPicture(T * &picParam);

    template < class T >
    bool newSlice(T * &sliceParam);


    template < class T >
    bool newMisc(VAEncMiscParameterType, T * &miscParam);

    bool addPackedHeader(VAEncPackedHeaderType, const void *header,
                         uint32_t headerBitSize);

    bool encode();

    // give subclass a chance to convert codec buffer to they wanted format.
    // vp8 hybrid driver may need entropy code the coded buffer
    // h264 encoder may need convert annexb to avcC
    virtual Encode_Status getOutput(VideoEncOutputBuffer * outBuffer);

    CodedBufferPtr m_codedBuffer;

  private:
    bool doRender();

    template < class T >
        BufObjectPtr createMiscObject(VAEncMiscParameterType, T * &bufPtr);

    BufObjectPtr m_sequence;
    BufObjectPtr m_picture;
    std::vector < BufObjectPtr > m_miscParams;
    std::vector < BufObjectPtr > m_slices;
    std::vector < std::pair < BufObjectPtr,
        BufObjectPtr > >m_packedHeaders;
};

template < class T > bool VaapiEncPicture::editSequence(T * &seqParam)
{
    return editObject(m_sequence, VAEncSequenceParameterBufferType,
                      seqParam);
}

template < class T > bool VaapiEncPicture::editPicture(T * &picParam)
{
    return editObject(m_picture, VAEncPictureParameterBufferType,
                      picParam);
}

template < class T > bool VaapiEncPicture::newSlice(T * &sliceParam)
{
    BufObjectPtr slice =
        createBufferObject(VAEncSliceParameterBufferType, sliceParam);
    return addObject(m_slices, slice);
}

template < class T >
BufObjectPtr VaapiEncPicture::
createMiscObject(VAEncMiscParameterType miscType, T * &bufPtr)
{
    VAEncMiscParameterBuffer *misc;
    int size = sizeof(VAEncMiscParameterBuffer) + sizeof(T);
    BufObjectPtr obj =
        createBufferObject(VAEncMiscParameterBufferType, size, NULL,
                           (void **) &misc);
    if (obj) {
        misc->type = miscType;
        bufPtr = (T *) (misc->data);
    }
    return obj;
}

template < class T >
bool VaapiEncPicture::newMisc(VAEncMiscParameterType miscType,
                              T * &miscParam)
{
    BufObjectPtr misc = createMiscObject(miscType, miscParam);
    return addObject(m_miscParams, misc);
}
}
#endif                          //vaapiencpicture_h
