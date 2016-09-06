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
#ifndef vaapidecpicture_h
#define vaapidecpicture_h

#include "vaapi/vaapipicture.h"

namespace YamiMediaCodec{
class VaapiDecPicture : public VaapiPicture
{
public:
    VaapiDecPicture(const ContextPtr&, const SurfacePtr&, int64_t timeStamp);
    virtual ~VaapiDecPicture() {};


    template <class T>
    bool editPicture(T*& picParam);

    template <class T>
    bool editIqMatrix(T*& matrix);

    template <class T>
    bool editBitPlane(T*& plane, size_t size);

    template <class T>
    bool editHufTable(T*& hufTable);

    template <class T>
    bool editProbTable(T*& probTable);

    template <class T>
    bool newSlice(T*& sliceParam, const void* sliceData, uint32_t sliceSize);

    bool decode();

protected:
    VaapiDecPicture();

private:
    virtual bool doRender();

    BufObjectPtr m_picture;
    BufObjectPtr m_iqMatrix;
    BufObjectPtr m_bitPlane;
    BufObjectPtr m_hufTable;
    BufObjectPtr m_probTable;
    std::vector<std::pair<BufObjectPtr, BufObjectPtr> > m_slices;
};

template<class T>
bool VaapiDecPicture::editPicture(T*& picParam)
{
    return editObject(m_picture, VAPictureParameterBufferType, picParam);
}

template <class T>
bool VaapiDecPicture::editIqMatrix(T*& matrix)
{
    return editObject(m_iqMatrix, VAIQMatrixBufferType, matrix);
}

template <class T>
bool VaapiDecPicture::editBitPlane(T*& plane, size_t size)
{
    if (m_bitPlane)
        return false;
    m_bitPlane = createBufferObject(VABitPlaneBufferType, size, NULL, (void**)&plane);
    if (m_bitPlane && plane) {
        memset(plane, 0, size);
        return true;
    }
    return false;
}

template <class T>
bool VaapiDecPicture::editHufTable(T*& hufTable)
{
    return editObject(m_hufTable, VAHuffmanTableBufferType, hufTable);
}

template <class T>
bool VaapiDecPicture::editProbTable(T*& probTable)
{
    return editObject(m_probTable, VAProbabilityBufferType, probTable);
}

template <class T>
bool VaapiDecPicture::newSlice(T*& sliceParam, const void* sliceData, uint32_t sliceSize)
{
    BufObjectPtr data = createBufferObject(VASliceDataBufferType, sliceSize, sliceData, NULL);
    BufObjectPtr param = createBufferObject(VASliceParameterBufferType, sliceParam);

    bool ret = addObject(m_slices, param, data);
    if (ret && sliceParam) {
        sliceParam->slice_data_size = sliceSize;
        sliceParam->slice_data_offset = 0;
        sliceParam->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
        return true;
    }

    return false;
}
}
#endif //#ifndef vaapidecpicture_h
