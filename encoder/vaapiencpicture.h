/*
 * Copyright (C) 2014 Intel Corporation. All rights reserved.
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

#ifndef vaapiencpicture_h
#define vaapiencpicture_h

#include "VideoEncoderDefs.h"

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
    bool editQMatrix(T * &qMatrix);

    template < class T >
    bool editHuffTable(T * &huffTable);

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
    virtual YamiStatus getOutput(VideoEncOutputBuffer* outBuffer);

#ifdef __BUILD_GET_MV__
    virtual bool editMVBuffer(void*& buffer, uint32_t *size);
#endif

    CodedBufferPtr m_codedBuffer;

  private:
    bool doRender();

    template < class T >
        BufObjectPtr createMiscObject(VAEncMiscParameterType, T * &bufPtr);

    BufObjectPtr m_sequence;
    BufObjectPtr m_picture;
    BufObjectPtr m_qMatrix;
    BufObjectPtr m_huffTable;
#ifdef __BUILD_GET_MV__
    BufObjectPtr m_MVBuffer;
    BufObjectPtr m_FEIBuffer;
#endif
    std::vector < BufObjectPtr > m_miscParams;
    std::vector < BufObjectPtr > m_slices;
    std::vector < std::pair<BufObjectPtr,BufObjectPtr > >m_packedHeaders;
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

template < class T > bool VaapiEncPicture::editQMatrix(T * &qMatrix)
{
    return editObject(m_qMatrix, VAQMatrixBufferType, qMatrix);
}

template < class T > bool VaapiEncPicture::editHuffTable(T * &huffTable)
{
    return editObject(m_huffTable, VAHuffmanTableBufferType, huffTable);
}

template < class T > bool VaapiEncPicture::newSlice(T * &sliceParam)
{
    BufObjectPtr slice =
        createBufferObject(VAEncSliceParameterBufferType, sliceParam);
    return (sliceParam != NULL) && addObject(m_slices, slice);
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
        memset(bufPtr, 0, sizeof(T));
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
