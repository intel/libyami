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

#include "vaapipic.h"

class VaapiDecSlice
{
    friend class VaapiDecPicture;
private:
    VaapiDecSlice(const BufObjectPtr& param, const BufObjectPtr& data)
        :m_param(param), m_data(data) {}
    BufObjectPtr m_param;
    BufObjectPtr m_data;
};

class VaapiDecPicture : public VaapiPic
{
public:
    typedef std::tr1::shared_ptr<VaapiDecSlice> DecSlicePtr;
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

protected:
    virtual bool doRender();

private:
    BufObjectPtr m_picture;
    BufObjectPtr m_iqMatrix;
    BufObjectPtr m_bitPlane;
    BufObjectPtr m_hufTable;
    BufObjectPtr m_probTable;
    std::vector<DecSlicePtr> m_slices;

    bool render(const DecSlicePtr& slice);

    template <typename P, typename O>
    friend bool render(P picture, std::vector<O>& objects);
};

template<class T>
bool VaapiDecPicture::editPicture(T*& picParam)
{
    return editMember(m_picture, VAPictureParameterBufferType, picParam);
}

template <class T>
bool VaapiDecPicture::editIqMatrix(T*& matrix)
{
    return editMember(m_iqMatrix, VAIQMatrixBufferType, matrix);
}

template <class T>
bool VaapiDecPicture::editBitPlane(T*& plane)
{
    return editMember(m_bitPlane, VABitPlaneBufferType, plane);
}

template <class T>
bool VaapiDecPicture::editHufTable(T*& hufTable)
{
    return editMember(m_hufTable, VAHuffmanTableBufferType, hufTable);
}

template <class T>
bool VaapiDecPicture::editProbTable(T*& probTable)
{
    return editMember(m_probTable, VAProbabilityBufferType, probTable);
}

template <class T>
bool VaapiDecPicture::newSlice(T*& sliceParam, const void* sliceData, uint32_t sliceSize)
{
    BufObjectPtr d = createBufferObject(VASliceDataBufferType, sliceSize, sliceData, NULL);
    BufObjectPtr p = createBufferObject(VASliceParameterBufferType, sliceParam);
    VaapiDecPicture::DecSlicePtr slice;
    if (d && p) {
        slice.reset(new VaapiDecSlice(p, d));
        sliceParam->slice_data_size = sliceSize;
        sliceParam->slice_data_offset = 0;
        sliceParam->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    }
    m_slices.push_back(slice);
    return slice;
}
