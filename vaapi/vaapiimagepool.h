/*
 *  vaapiimagepool.h - image pool
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
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


#ifndef vaapiimagepool_h
#define vaapiimagepool_h

#include "common/condition.h"
#include "common/common_def.h"
#include "common/lock.h"
#include "vaapi/vaapiptrs.h"
#include "vaapi/vaapitypes.h"
#include <deque>
#include <map>
#include <vector>
#include <va/va.h>

namespace YamiMediaCodec{

/**
 * \class VaapiImagePool
 * \brief image pool, may be used for color format conversion or video resize. for example: convert decoded surface to other format (RGBX) by vaGetImage, convert input format to another format (NV12) by vaPutImage.
 * <pre>
 * 1. for decoder, usually, the decoded surface becomes reusable after the data is converted into an VAImage, the image will be recycled from client by renderDone()
 * 2. for encoder, usually, the input image becomes reusable after the data is converted into a VASurface, simple recycle the image by recycle().
 *</pre>
*/


class VaapiImagePool : public EnableSharedFromThis<VaapiImagePool>
{
public:
    static ImagePoolPtr create(const DisplayPtr&, uint32_t format, int32_t width, int32_t height, int32_t count);
    /// get a free image,
    ImagePtr acquireWithWait();

    ///after flush is called, acquireWithWait always return null image,until all ImagePtr are returned.
    void flush();

    /// set image acquire waitable or not, also wake up the previous wait when it is set to false
    void setWaitable(bool waitable);

private:

    VaapiImagePool(std::vector<ImagePtr>);
    /// recycle to image pool
    void recycleID(VAImageID imageID);    // usually recycle from client after rendering

    std::vector<ImagePtr> m_images;
    int32_t m_poolSize;
    std::deque<int32_t> m_freeIndex;
    typedef std::map<VAImageID, int32_t> MapImageIDIndex; // map between VAImageID and index (of m_images)
    MapImageIDIndex m_indexMap;

    Lock m_lock;
    Condition m_cond;
    bool m_flushing;

    struct ImageRecycler;

    DISALLOW_COPY_AND_ASSIGN(VaapiImagePool);
};

} //namespace YamiMediaCodec

#endif //vaapiimagepool_h
