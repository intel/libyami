/*
 *  vaapipicture.h - objects for va decode
 *
 *  Copyright (C) 2010-2011 Splitted-Desktop Systems
 *  Copyright (C) 2011-2012 Intel Corporation
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

#ifndef VAAPI_PICTURE_H
#define VAAPI_PICTURE_H

#include <list>
#include <vector>
#include "common/vaapisurface.h"
#include "common/vaapibuffer.h"
#include "vaapisurfacebuf_pool.h"

using namespace std;

typedef enum {
    VAAPI_PICTURE_TYPE_NONE = 0,        // Undefined
    VAAPI_PICTURE_TYPE_I,               // Intra
    VAAPI_PICTURE_TYPE_P,               // Predicted
    VAAPI_PICTURE_TYPE_B,               // Bi-directional predicted
    VAAPI_PICTURE_TYPE_S,               // S(GMC)-VOP (MPEG-4)
    VAAPI_PICTURE_TYPE_SI,              // Switching Intra
    VAAPI_PICTURE_TYPE_SP,              // Switching Predicted
    VAAPI_PICTURE_TYPE_BI,              // BI type (VC-1)
} VaapiPictureType;

typedef enum {
    VAAPI_PICTURE_FLAG_SKIPPED    = (1 << 0),
    VAAPI_PICTURE_FLAG_REFERENCE  = (1 << 1),
    VAAPI_PICTURE_FLAG_OUTPUT     = (1 << 2),
    VAAPI_PICTURE_FLAG_INTERLACED = (1 << 3),
    VAAPI_PICTURE_FLAG_FF         = (1 << 4),
    VAAPI_PICTURE_FLAG_TFF        = (1 << 5),
    VAAPI_PICTURE_FLAG_LAST       = (1 << 6),
} VaapiPictureFlags;

#define VAAPI_PICTURE_FLAGS(picture) \
    ((picture)->mFlags)

#define VAAPI_PICTURE_FLAG_IS_SET(picture, flag) \
    (VAAPI_PICTURE_FLAGS(picture) & (flag))

#define VAAPI_PICTURE_FLAG_SET(picture, flag) \
     (VAAPI_PICTURE_FLAGS(picture) |= (flag))

#define VAAPI_PICTURE_FLAG_UNSET(picture, flag) \
     (VAAPI_PICTURE_FLAGS(picture) &= ~((uint32_t)flag))

#define VAAPI_PICTURE_IS_SKIPPED(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_SKIPPED)

#define VAAPI_PICTURE_IS_REFERENCE(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_REFERENCE)

#define VAAPI_PICTURE_IS_OUTPUT(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_OUTPUT)

#define VAAPI_PICTURE_IS_INTERLACED(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_INTERLACED)

#define VAAPI_PICTURE_IS_FIRST_FIELD(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_FF)

#define VAAPI_PICTURE_IS_TFF(picture) \
    VAAPI_PICTURE_FLAG_IS_SET(picture, VAAPI_PICTURE_FLAG_TFF)

#define VAAPI_PICTURE_IS_FRAME(picture) \
    ((picture)->mStructure == VAAPI_PICTURE_STRUCTURE_FRAME)

#define VAAPI_PICTURE_IS_COMPLETE(picture)          \
    (VAAPI_PICTURE_IS_FRAME(picture) ||             \
     !VAAPI_PICTURE_IS_FIRST_FIELD(picture))

typedef enum {
    VAAPI_PICTURE_STRUCTURE_TOP_FIELD       = 1 << 0,
    VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD    = 1 << 1,
    VAAPI_PICTURE_STRUCTURE_FRAME           =
    (
        VAAPI_PICTURE_STRUCTURE_TOP_FIELD |
        VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD
    ),
    VAAPI_COLOR_STANDARD_ITUR_BT_601        = 1 << 2,
    VAAPI_COLOR_STANDARD_ITUR_BT_709        = 1 << 3,
    VAAPI_PICTURE_LEFT_VIEW                 = 1 << 4,
    VAAPI_PICTURE_RIGHT_VIEW                = 1 << 5,
    VAAPI_PICTURE_MVC_VIEW                      =
    (
        VAAPI_PICTURE_LEFT_VIEW  |
        VAAPI_PICTURE_RIGHT_VIEW
    ),
    VAAPI_S3D_STRUCTURE_FRAME_PACKING       = 1 << 6,
    VAAPI_S3D_STRUCTURE_TOP_ON_BOTTOM       = 1 << 7,
    VAAPI_S3D_STRUCTURE_SIDE_BY_SIDE        = 1 << 8,
    VAAPI_S3D_STRUCTURE_COMPOSITED          =
    (
        VAAPI_S3D_STRUCTURE_FRAME_PACKING |
        VAAPI_S3D_STRUCTURE_TOP_ON_BOTTOM |
        VAAPI_S3D_STRUCTURE_SIDE_BY_SIDE  
    )
} VaapiPictureStructure;

class VaapiSlice {
public:
    VaapiBufObject *mParam;
    VaapiBufObject *mData;
};

class VaapiPicture {
public:
    VaapiPicture(VADisplay display,
                 VAContextID context,
                 VaapiSurfaceBufferPool *surfBufPool,
                 VaapiPictureStructure structure);

    ~VaapiPicture();

    void addSlice(VaapiSlice *slice);
    VaapiSlice* getLastSlice();
    bool decodePicture();
    bool output(); 

public:
    uint64_t              mTimeStamp;
    uint32_t              mPoc;
    uint32_t              mFlags;
    VaapiPictureStructure mStructure;
    VaapiPictureType      mType;
    VaapiBufObject       *mPicParam;
    VaapiBufObject       *mIqMatrix;
    VaapiBufObject       *mBitPlane;
    VaapiBufObject       *mHufTable;
    VASurfaceID           mSurfaceID;
    VADisplay             mDisplay;
    VAContextID           mContext;
    VideoSurfaceBuffer    *mSurfBuf;
 
private:
    VaapiSurfaceBufferPool *mSurfBufPool;
    vector<VaapiSlice*> mSliceArray;
};

#endif
