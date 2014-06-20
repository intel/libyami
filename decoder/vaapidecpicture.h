/*
 *  vaapidecpicture.h - base picture for decoder
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
#ifndef vaapidecpicture_h
#define vaapidecpicture_h

#include "vaapi/vaapipicture.h"

class VaapiDecPicture : public VaapiPicture
{
public:
    VaapiDecPicture(VADisplay display, VAContextID context,const SurfacePtr& surface, int64_t timeStamp);
    virtual ~VaapiDecPicture() {};


    template <class T>
    bool editPicture(T*& picParam);

    template <class T>
    bool editIqMatrix(T*& matrix);

    template <class T>
    bool editBitPlane(T*& plane);

    template <class T>
    bool editHufTable(T*& hufTable);

    template <class T>
    bool editProbTable(T*& probTable);

    template <class T>
    bool newSlice(T*& sliceParam, const void* sliceData, uint32_t sliceSize);

    bool decode();

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
bool VaapiDecPicture::editBitPlane(T*& plane)
{
    return editObject(m_bitPlane, VABitPlaneBufferType, plane);
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
    if (ret) {
        sliceParam->slice_data_size = sliceSize;
        sliceParam->slice_data_offset = 0;
        sliceParam->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    }
    return ret;
}

#endif //#ifndef vaapidecpicture_h
