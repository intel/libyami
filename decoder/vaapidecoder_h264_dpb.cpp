/*
 *  vaapidecoder_h264_dpb.cpp - DPB manager for h264 decoder
 *
 *  Copyright (C) 2012-2014 Intel Corporation
 *    Author: Gwenole Beauchesne <gwenole.beauchesne@intel.com>
 *    Author: Xiaowei Li <xiaowei.li@intel.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include "vaapidecoder_h264.h"

namespace YamiMediaCodec{
typedef VaapiDecPictureH264::PicturePtr PicturePtr;
typedef VaapiDecPictureH264::PictureWeakPtr PictureWeakPtr;
typedef VaapiDecPictureH264::SliceHeaderPtr SliceHeaderPtr;


/* Defined to 1 if strict ordering of DPB is needed. Only useful for debug */
#define USE_STRICT_DPB_ORDERING 0

#define ARRAY_REMOVE_INDEX(array, index) \
    do {  \
      uint32_t size = array##Count; \
      assert(index < size); \
      if (index != --size); \
           (array)[index] = (array)[size]; \
      array[size] = NULL; \
      array##Count = size;\
    }while(0);

#define SORT_REF_LIST(list, n, compareFunc) \
    qsort(list, n, sizeof(*(list)), comparePicture##compareFunc)

static uint32_t roundLog2(uint32_t value)
{
    uint32_t ret = 0;
    uint32_t valueSquare = value * value;
    while ((1 << (ret + 1)) <= valueSquare)
        ++ret;

    ret = (ret + 1) >> 1;
    return ret;
}

static int comparePicturePicNumDec(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picB->m_picNum - picA->m_picNum;
}

static int comparePictureLongTermPicNumInc(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picA->m_longTermPicNum - picB->m_longTermPicNum;
}

static int comparePicturePOCDec(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picB->m_POC - picA->m_POC;
}

static int comparePicturePOCInc(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picA->m_POC - picB->m_POC;
}

static int comparePictureFrameNumWrapDec(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picB->m_frameNumWrap - picA->m_frameNumWrap;
}

static int comparePictureLongTermFrameIdxInc(const void *a, const void *b)
{
    const VaapiDecPictureH264 *const picA = *(VaapiDecPictureH264 **) a;
    const VaapiDecPictureH264 *const picB = *(VaapiDecPictureH264 **) b;

    return picA->m_longTermFrameIdx - picB->m_longTermFrameIdx;
}

static void
setH264PictureReference(VaapiDecPictureH264* picture,
                        uint32_t referenceFlags, bool otherField)
{
    VAAPI_PICTURE_FLAG_UNSET(picture, VAAPI_PICTURE_FLAGS_REFERENCE);
    VAAPI_PICTURE_FLAG_SET(picture, referenceFlags);

    PicturePtr strong;
    if (!otherField || !(strong = picture->m_otherField.lock()))
        return ;
    VaapiDecPictureH264* other = strong.get();
    VAAPI_PICTURE_FLAG_UNSET(other, VAAPI_PICTURE_FLAGS_REFERENCE);
    VAAPI_PICTURE_FLAG_SET(other, referenceFlags);
}

static int32_t
getPicNumX(const PicturePtr& picture, H264RefPicMarking * refPicMarking)
{
    int32_t picNum;

    if (VAAPI_PICTURE_IS_FRAME(picture))
        picNum = picture->m_frameNumWrap;
    else
        picNum = 2 * picture->m_frameNumWrap + 1;
    picNum -= refPicMarking->difference_of_pic_nums_minus1 + 1;
    return picNum;
}

uint32_t getMaxDecFrameBuffering(H264SPS * sps, uint32_t views)
{
    uint32_t maxDecFrameBuffering;
    uint32_t maxDpbMbs = 0;
    uint32_t picSizeMbs = 0;
    //H264SPSExtMVC * const  mvc = &sps->extension.mvc;

    /* Table A-1 - Level limits */
    switch (sps->level_idc) {
    case 10:
        maxDpbMbs = 396;
        break;
    case 11:
        maxDpbMbs = 900;
        break;
    case 12:
        maxDpbMbs = 2376;
        break;
    case 13:
        maxDpbMbs = 2376;
        break;
    case 20:
        maxDpbMbs = 2376;
        break;
    case 21:
        maxDpbMbs = 4752;
        break;
    case 22:
        maxDpbMbs = 8100;
        break;
    case 30:
        maxDpbMbs = 8100;
        break;
    case 31:
        maxDpbMbs = 18000;
        break;
    case 32:
        maxDpbMbs = 20480;
        break;
    case 40:
        maxDpbMbs = 32768;
        break;
    case 41:
        maxDpbMbs = 32768;
        break;
    case 42:
        maxDpbMbs = 34816;
        break;
    case 50:
        maxDpbMbs = 110400;
        break;
    case 51:
        maxDpbMbs = 184320;
        break;
    case 52:
        maxDpbMbs = 184320;
        break;
    default:
        assert(0);
        break;
    }

    picSizeMbs = ((sps->pic_width_in_mbs_minus1 + 1) *
                  (sps->pic_height_in_map_units_minus1 + 1) *
                  (sps->frame_mbs_only_flag ? 1 : 2));
    maxDecFrameBuffering = maxDpbMbs / picSizeMbs;

    if (views > 1) {
        maxDecFrameBuffering = MIN(2 * maxDecFrameBuffering,
                                   16 * MAX(1, roundLog2(views)));
        maxDecFrameBuffering = maxDecFrameBuffering / views;
    }

    /* VUI parameters */
    if (sps->vui_parameters_present_flag) {
        H264VUIParams *const vuiParams = &sps->vui_parameters;
        if (vuiParams->bitstream_restriction_flag)
            maxDecFrameBuffering = vuiParams->max_dec_frame_buffering;
        else {
            switch (sps->profile_idc) {
            case 44:           // CAVLC 4:4:4 Intra profile
            case 86:           // Scalable High profile
            case 100:          // High profile
            case 110:          // High 10 profile
            case 122:          // High 4:2:2 profile
            case 244:          // High 4:4:4 Predictive profile
                if (sps->constraint_set3_flag)
                    maxDecFrameBuffering = 0;
                break;
            }
        }
    }

    if (maxDecFrameBuffering > 16)
        maxDecFrameBuffering = 16;
    else if (maxDecFrameBuffering < sps->num_ref_frames)
        maxDecFrameBuffering = sps->num_ref_frames;
    return MAX(1, maxDecFrameBuffering);
}

VaapiDPBManager::VaapiDPBManager(VaapiDecoderH264* decoder, uint32_t DPBSize)
    :m_decoder(decoder)
{
    DPBLayer.reset(new VaapiDecPicBufLayer(DPBSize));
}

VaapiDPBManager::~VaapiDPBManager()
{
}

bool VaapiDPBManager::outputDPB(const VaapiFrameStore::Ptr &frameStore,
                                const PicturePtr& picture)
{
    picture->m_outputNeeded = false;

    PicturePtr frame;
    if (frameStore) {
        if (--frameStore->m_outputNeeded > 0)
            return true;
        frame = frameStore->m_buffers[0];
    }

    DEBUG("DPB: output picture(Addr:%p, Poc:%d)", picture.get(), picture->m_POC);

//FIXME:
#if 0
    if (!frameStore)
        picture->m_surfBuf->status &= ~SURFACE_DECODING;
#endif
    return m_decoder->outputPicture(frame) == DECODE_SUCCESS;
}

void VaapiDPBManager::evictDPB(uint32_t idx)
{
    const VaapiFrameStore::Ptr&  frameStore = DPBLayer->DPB[idx];
    if (!frameStore->m_outputNeeded && !frameStore->hasReference())
        removeDPBIndex(idx);
}

bool VaapiDPBManager::bumpDPB()
{
    PicturePtr foundPicture;
    uint32_t i, j, frameIndex = 0;
    bool success;

    for (i = 0; i < DPBLayer->DPBCount; i++) {
        const VaapiFrameStore::Ptr& frameStore = DPBLayer->DPB[i];

        if (!frameStore->m_outputNeeded)
            continue;

        for (j = 0; j < frameStore->m_numBuffers; j++) {
            const PicturePtr& picture = frameStore->m_buffers[j];
            if (!picture->m_outputNeeded)
                continue;
            if (!foundPicture || foundPicture->m_POC > picture->m_POC)
                foundPicture = picture, frameIndex = i;
        }
    }
    if (!foundPicture)
        return false;

    success = outputDPB(DPBLayer->DPB[frameIndex], foundPicture);

    evictDPB(frameIndex);
    return success;
}

void VaapiDPBManager::clearDPB()
{
    uint32_t i;
    if (DPBLayer) {
        for (i = 0; i < DPBLayer->DPBCount; i++) {
            DPBLayer->DPB[i].reset();
        }
        DPBLayer->DPBCount = 0;
    }
}

void VaapiDPBManager::flushDPB()
{
    drainDPB(); // it's safe for internal reset: IDR, picture-marking etc.
    clearDPB();
}

void VaapiDPBManager::drainDPB()
{
    outputImmediateBFrame();
    while (bumpDPB()) ;
}

void VaapiDPBManager::debugDPBStatus()
{
    int i;
    VaapiFrameStore::Ptr frameStore;

    for (i = 0; i < DPBLayer->DPBCount; i++) {
        frameStore = DPBLayer->DPB[i];
        if (frameStore->m_numBuffers == 2)
            DEBUG
                ("index: %d, m_outputNeeded: %d, POC (%d, %d), isReference: %d, surface ID: 0x%x",
                 i, frameStore->m_outputNeeded,
                 frameStore->m_buffers[0]->m_POC,
                 frameStore->m_buffers[1]->m_POC,
                 frameStore->hasReference(),
                 frameStore->m_buffers[0]->getSurfaceID());
        else
            DEBUG
                ("index: %d, m_outputNeeded: %d, POC: %d, , isReference: %d, surface ID: 0x%x",
                 i, frameStore->m_outputNeeded,
                 frameStore->m_buffers[0]->m_POC,
                 frameStore->hasReference(),
                 frameStore->m_buffers[0]->getSurfaceID());
    }
}

bool VaapiDPBManager::addDPB(const VaapiFrameStore::Ptr& newFrameStore,
                             const PicturePtr& pic)
{
    uint32_t i, j;
    VaapiFrameStore::Ptr frameStore;

#ifdef __ENABLE_DEBUG__
    debugDPBStatus();
#endif
    outputImmediateBFrame();

    // Remove all unused pictures
    if (!VAAPI_H264_PICTURE_IS_IDR(pic)) {
        i = 0;
        while (i < DPBLayer->DPBCount) {
            frameStore = DPBLayer->DPB[i];
            if (!frameStore->m_outputNeeded && !frameStore->hasReference()) {
                removeDPBIndex(i);
            } else
                i++;
        }
    }
    // C.4.5.1 - Storage and marking of a reference decoded picture into the DPB
    if (VAAPI_PICTURE_IS_REFERENCE(pic)) {
        while (DPBLayer->DPBCount == DPBLayer->DPBSize) {
            if (!bumpDPB())
                return false;
        }

        DPBLayer->DPB[DPBLayer->DPBCount++] = newFrameStore;
        if (pic->m_outputFlag) {
            pic->m_outputNeeded = true;
            newFrameStore->m_outputNeeded++;
        }
    }
    // C.4.5.2 - Storage and marking of a non-reference decoded picture into the DPB
    else {
        if (!pic->m_outputFlag)
            return true;
        while (DPBLayer->DPBCount == DPBLayer->DPBSize) {
            bool foundPicture = false;
            for (i = 0; !foundPicture && i < DPBLayer->DPBCount; i++) {
                frameStore = DPBLayer->DPB[i];
                if (!frameStore->m_outputNeeded)
                    continue;
                for (j = 0; !foundPicture && j < frameStore->m_numBuffers;
                     j++)
                    foundPicture = frameStore->m_buffers[j]->m_outputNeeded
                        && frameStore->m_buffers[j]->m_POC < pic->m_POC;
            }
            if (!foundPicture) {
                bool ret = true;
                if (newFrameStore->hasFrame()) {
                    newFrameStore->m_outputNeeded++;
                    ret = outputDPB(newFrameStore, pic);
                } else {
                    m_prevFrameStore = newFrameStore;   // wait for a complete frame to render
                }
                return ret;
            }
            if (!bumpDPB())
                return false;
        }

        DPBLayer->DPB[DPBLayer->DPBCount++] = newFrameStore;
        pic->m_outputNeeded = true;
        newFrameStore->m_outputNeeded++;
    }

    return true;
}

void VaapiDPBManager::resetDPB(H264SPS * sps)
{
    m_prevFrameStore.reset();
    uint32_t size = getMaxDecFrameBuffering(sps, 1);
    DPBLayer.reset(new VaapiDecPicBufLayer(size));
}

/*
    for a non-ref B filed, it outputs directly without adding to DPB.
    however, the other field is required before rendering.
    so we postpone this frame output to the timing when another new frame comes.
    another potential solution is to extend dpb size by one, which can also help SVC/MVC case
*/
bool VaapiDPBManager::outputImmediateBFrame()
{
    int i;
    int ret = true;

    if (m_prevFrameStore.get()) {
        for (i = 0; i < m_prevFrameStore->m_numBuffers; i++) {
            ret &= outputDPB(m_prevFrameStore, m_prevFrameStore->m_buffers[i]);
        }
        m_prevFrameStore.reset();
    }

    return ret;
}

/* initialize reference list */
void VaapiDPBManager::initPictureRefs(const PicturePtr& pic,
                                      const SliceHeaderPtr& sliceHdr,
                                      int32_t frameNum)
{
    uint32_t i, numRefs;

    initPictureRefLists(pic);

    initPictureRefsPicNum(pic, sliceHdr, frameNum);

    DPBLayer->refPicList0Count = 0;
    DPBLayer->refPicList1Count = 0;

    switch (sliceHdr->type % 5) {
    case H264_P_SLICE:
    case H264_SP_SLICE:
        initPictureRefsPSlice(pic, sliceHdr);
        break;
    case H264_B_SLICE:
        initPictureRefsBSlice(pic, sliceHdr);
        break;
    default:
        break;
    }

    execPictureRefsModification(pic, sliceHdr);

    switch (sliceHdr->type % 5) {
    case H264_B_SLICE:
        numRefs = 1 + sliceHdr->num_ref_idx_l1_active_minus1;
        for (i = DPBLayer->refPicList1Count; i < numRefs; i++)
            DPBLayer->refPicList1[i] = NULL;
        //DPBLayer->refPicList1Count = numRefs;

        // fall-through
    case H264_P_SLICE:
    case H264_SP_SLICE:
        numRefs = 1 + sliceHdr->num_ref_idx_l0_active_minus1;
        for (i = DPBLayer->refPicList0Count; i < numRefs; i++)
            DPBLayer->refPicList0[i] = NULL;
        //DPBLayer->refPicList0Count = numRefs;
        break;
    default:
        break;
    }
}

bool VaapiDPBManager::execRefPicMarking(const PicturePtr& pic,
                                        bool * hasMMCO5)
{
    *hasMMCO5 = false;

    if (!VAAPI_PICTURE_IS_REFERENCE(pic)) {
        return true;
    }

    if (!VAAPI_H264_PICTURE_IS_IDR(pic)) {
        H264SliceHdr* header = pic->getLastSliceHeader();
        if (!header)
            return false;
        H264DecRefPicMarking *const decRefPicMarking =
            &header->dec_ref_pic_marking;
        if (decRefPicMarking->adaptive_ref_pic_marking_mode_flag) {
            if (!execRefPicMarkingAdaptive(pic, decRefPicMarking, hasMMCO5))
                return false;
        } else if (!execRefPicMarkingSlidingWindow(pic))
            return false;
    }

    return true;
}

PicturePtr VaapiDPBManager::addDummyPicture(const PicturePtr& pic,
                                            int32_t frameNum,
                                            const SurfacePtr& surface)
{
    PicturePtr dummyPic(new VaapiDecPictureH264(pic->m_context, surface, 0));
    if (!dummyPic)
        return dummyPic;

    VAAPI_PICTURE_FLAG_SET(dummyPic,
                           (VAAPI_PICTURE_FLAG_SKIPPED |
                            VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
                            VAAPI_PICTURE_FLAG_FF));

    //FIXME : how do we handle poc if B frame exists.
    dummyPic->m_POC = INVALID_POC;
    dummyPic->m_frameNum = frameNum;
    dummyPic->m_frameNumWrap = frameNum;
    dummyPic->m_pps = pic->m_pps;
    dummyPic->m_outputNeeded = false;
    dummyPic->m_picStructure = VAAPI_PICTURE_STRUCTURE_FRAME;
    dummyPic->m_structure = dummyPic->m_picStructure;

    return dummyPic;
}

bool VaapiDPBManager::execDummyPictureMarking(const PicturePtr& dummyPic,
                                              const SliceHeaderPtr& sliceHdr,
                                              int32_t frameNum)
{
    initPictureRefLists(dummyPic);
    initPictureRefsPicNum(dummyPic, sliceHdr, frameNum);
    if (!execRefPicMarkingSlidingWindow(dummyPic))
        return false;
    removeShortReference(dummyPic);
    /* add to short reference */
    DPBLayer->shortRef[DPBLayer->shortRefCount++] = dummyPic.get();

    return true;
}

/* private functions */

void VaapiDPBManager::initPictureRefLists(const PicturePtr& pic)
{
    uint32_t i, j, shortRefCount, longRefCount;
    VaapiFrameStore::Ptr frameStore;
    VaapiDecPictureH264 *picture;

    shortRefCount = 0;
    longRefCount = 0;
    if (pic->m_structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        for (i = 0; i < DPBLayer->DPBCount; i++) {
            frameStore = DPBLayer->DPB[i];
            if (!frameStore->hasFrame())
                continue;
            picture = frameStore->m_buffers[0].get();
            if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                DPBLayer->shortRef[shortRefCount++] = picture;
            else if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture))
                DPBLayer->longRef[longRefCount++] = picture;
            picture->m_structure = VAAPI_PICTURE_STRUCTURE_FRAME;
            picture->m_otherField = frameStore->m_buffers[1];
        }
    } else {
        for (i = 0; i < DPBLayer->DPBCount; i++) {
            frameStore = DPBLayer->DPB[i];
            for (j = 0; j < frameStore->m_numBuffers; j++) {
                picture = frameStore->m_buffers[j].get();
                if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                    DPBLayer->shortRef[shortRefCount++] = picture;
                else if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE
                         (picture))
                    DPBLayer->longRef[longRefCount++] = picture;
                picture->m_structure = picture->m_picStructure;
                picture->m_otherField = frameStore->m_buffers[j ^ 1];
            }
        }
    }

    for (i = shortRefCount; i < DPBLayer->shortRefCount; i++)
        DPBLayer->shortRef[i] = NULL;
    DPBLayer->shortRefCount = shortRefCount;

    for (i = longRefCount; i < DPBLayer->longRefCount; i++)
        DPBLayer->longRef[i] = NULL;
    DPBLayer->longRefCount = longRefCount;

}

void VaapiDPBManager::initPictureRefsPSlice(const PicturePtr& pic,
                                            const SliceHeaderPtr& sliceHdr)
{
    VaapiDecPictureH264 **refList;
    uint32_t i;

    if (pic->m_structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        /* 8.2.4.2.1 - P and SP slices in frames */
        if (DPBLayer->shortRefCount > 0) {
            refList = DPBLayer->refPicList0;
            for (i = 0; i < DPBLayer->shortRefCount; i++)
                refList[i] = DPBLayer->shortRef[i];
            SORT_REF_LIST(refList, i, PicNumDec);
            DPBLayer->refPicList0Count += i;
        }

        if (DPBLayer->longRefCount > 0) {
            refList = &DPBLayer->refPicList0[DPBLayer->refPicList0Count];
            for (i = 0; i < DPBLayer->longRefCount; i++)
                refList[i] = DPBLayer->longRef[i];
            SORT_REF_LIST(refList, i, LongTermPicNumInc);
            DPBLayer->refPicList0Count += i;
        }
    } else {
        /* 8.2.4.2.2 - P and SP slices in fields */
        VaapiDecPictureH264 *shortRef[32];
        uint32_t shortRefCount = 0;
        VaapiDecPictureH264 *longRef[32];
        uint32_t longRefCount = 0;

        if (DPBLayer->shortRefCount > 0) {
            for (i = 0; i < DPBLayer->shortRefCount; i++)
                shortRef[i] = DPBLayer->shortRef[i];
            SORT_REF_LIST(shortRef, i, FrameNumWrapDec);
            shortRefCount = i;
        }

        if (DPBLayer->longRefCount > 0) {
            for (i = 0; i < DPBLayer->longRefCount; i++)
                longRef[i] = DPBLayer->longRef[i];
            SORT_REF_LIST(longRef, i, LongTermFrameIdxInc);
            longRefCount = i;
        }

        initPictureRefsFields(pic,
                              DPBLayer->refPicList0,
                              &DPBLayer->refPicList0Count, shortRef,
                              shortRefCount, longRef, longRefCount);
    }

}

void VaapiDPBManager::initPictureRefsBSlice(const PicturePtr& picture,
                                            const SliceHeaderPtr& sliceHdr)
{
    VaapiDecPictureH264 **refList;
    uint32_t i, n;

    if (picture->m_structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        /* 8.2.4.2.3 - B slices in frames */

        /* refPicList0 */
        if (DPBLayer->shortRefCount > 0) {
            // 1. Short-term references
            refList = DPBLayer->refPicList0;
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC < picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCDec);
            DPBLayer->refPicList0Count += n;

            refList = &DPBLayer->refPicList0[DPBLayer->refPicList0Count];
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC >= picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCInc);
            DPBLayer->refPicList0Count += n;
        }

        if (DPBLayer->longRefCount > 0) {
            // 2. Long-term references
            refList = &DPBLayer->refPicList0[DPBLayer->refPicList0Count];
            for (n = 0, i = 0; i < DPBLayer->longRefCount; i++)
                refList[n++] = DPBLayer->longRef[i];
            SORT_REF_LIST(refList, n, LongTermPicNumInc);
            DPBLayer->refPicList0Count += n;
        }

        /* refPicList1 */
        if (DPBLayer->shortRefCount > 0) {
            // 1. Short-term references
            refList = DPBLayer->refPicList1;
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC > picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCInc);
            DPBLayer->refPicList1Count += n;

            refList = &DPBLayer->refPicList1[DPBLayer->refPicList1Count];
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC <= picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCDec);
            DPBLayer->refPicList1Count += n;
        }

        if (DPBLayer->longRefCount > 0) {
            // 2. Long-term references
            refList = &DPBLayer->refPicList1[DPBLayer->refPicList1Count];
            for (n = 0, i = 0; i < DPBLayer->longRefCount; i++)
                refList[n++] = DPBLayer->longRef[i];
            SORT_REF_LIST(refList, n, LongTermPicNumInc);
            DPBLayer->refPicList1Count += n;
        }
    } else {
        /* 8.2.4.2.4 - B slices in fields */
        VaapiDecPictureH264 *shortRef0[32];
        uint32_t shortRef0Count = 0;
        VaapiDecPictureH264 *shortRef1[32];
        uint32_t shortRef1Count = 0;
        VaapiDecPictureH264 *longRef[32];
        uint32_t longRefCount = 0;

        /* refFrameList0ShortTerm */
        if (DPBLayer->shortRefCount > 0) {
            refList = shortRef0;
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC <= picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCDec);
            shortRef0Count += n;

            refList = &shortRef0[shortRef0Count];
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC > picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCInc);
            shortRef0Count += n;
        }

        /* refFrameList1ShortTerm */
        if (DPBLayer->shortRefCount > 0) {
            refList = shortRef1;
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC > picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCInc);
            shortRef1Count += n;

            refList = &shortRef1[shortRef1Count];
            for (n = 0, i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i]->m_POC <= picture->m_POC)
                    refList[n++] = DPBLayer->shortRef[i];
            }
            SORT_REF_LIST(refList, n, POCDec);
            shortRef1Count += n;
        }

        /* refFrameListLongTerm */
        if (DPBLayer->longRefCount > 0) {
            for (i = 0; i < DPBLayer->longRefCount; i++)
                longRef[i] = DPBLayer->longRef[i];
            SORT_REF_LIST(longRef, i, LongTermFrameIdxInc);
            longRefCount = i;
        }

        initPictureRefsFields(picture,
                              DPBLayer->refPicList0,
                              &DPBLayer->refPicList0Count, shortRef0,
                              shortRef0Count, longRef, longRefCount);

        initPictureRefsFields(picture,
                              DPBLayer->refPicList1,
                              &DPBLayer->refPicList1Count, shortRef1,
                              shortRef1Count, longRef, longRefCount);
    }

    /* Check whether refPicList1 is identical to refPicList0, then
       swap if necessary */
    if (DPBLayer->refPicList1Count > 1 &&
        DPBLayer->refPicList1Count == DPBLayer->refPicList0Count &&
        memcmp(DPBLayer->refPicList0, DPBLayer->refPicList1,
               DPBLayer->refPicList0Count *
               sizeof(DPBLayer->refPicList0[0])) == 0) {
        VaapiDecPictureH264 *const tmp = DPBLayer->refPicList1[0];
        DPBLayer->refPicList1[0] = DPBLayer->refPicList1[1];
        DPBLayer->refPicList1[1] = tmp;
    }
}

void VaapiDPBManager::initPictureRefsFields(const PicturePtr& picture,
                                            VaapiDecPictureH264 *
                                            refPicList[32],
                                            uint32_t * refPicListCount,
                                            VaapiDecPictureH264 *
                                            shortRef[32],
                                            uint32_t shortRefCount,
                                            VaapiDecPictureH264 *
                                            longRef[32],
                                            uint32_t longRefCount)
{
    uint32_t n = 0;
    /* 8.2.4.2.5 - reference picture lists in fields */
    initPictureRefsFields1(picture->m_structure, refPicList, &n,
                           shortRef, shortRefCount);
    initPictureRefsFields1(picture->m_structure, refPicList, &n,
                           longRef, longRefCount);
    *refPicListCount = n;
}

void VaapiDPBManager::
initPictureRefsFields1(uint32_t pictureStructure,
                       VaapiDecPictureH264 * refPicList[32],
                       uint32_t * refPicListCount,
                       VaapiDecPictureH264 * refList[32],
                       uint32_t refListCount)
{
    uint32_t i, j, n;

    i = 0;
    j = 0;
    n = *refPicListCount;
    do {
        if (n >= 32)
            return;
        for (; i < refListCount; i++) {
            if (refList[i]->m_structure == pictureStructure) {
                refPicList[n++] = refList[i++];
                break;
            }
        }

        if (n >= 32)
            return;
        for (; j < refListCount; j++) {
            if (refList[j]->m_structure != pictureStructure) {
                refPicList[n++] = refList[j++];
                break;
            }
        }
    } while (i < refListCount || j < refListCount);
    *refPicListCount = n;
}

void VaapiDPBManager::initPictureRefsPicNum(const PicturePtr& picture,
                                            const SliceHeaderPtr& sliceHdr,
                                            int32_t frameNum)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    const int32_t maxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    uint32_t i;

    for (i = 0; i < DPBLayer->shortRefCount; i++) {
        VaapiDecPictureH264 *const pic = DPBLayer->shortRef[i];

        // (8-27)
        if (pic->m_frameNum > frameNum)
            pic->m_frameNumWrap = pic->m_frameNum - maxFrameNum;
        else
            pic->m_frameNumWrap = pic->m_frameNum;

        // (8-28, 8-30, 8-31)
        if (VAAPI_PICTURE_IS_FRAME(picture))
            pic->m_picNum = pic->m_frameNumWrap;
        else {
            if (pic->m_structure == picture->m_structure)
                pic->m_picNum = 2 * pic->m_frameNumWrap + 1;
            else
                pic->m_picNum = 2 * pic->m_frameNumWrap;
        }
    }

    for (i = 0; i < DPBLayer->longRefCount; i++) {
        VaapiDecPictureH264 *const pic = DPBLayer->longRef[i];

        // (8-29, 8-32, 8-33)
        if (picture->m_structure == VAAPI_PICTURE_STRUCTURE_FRAME)
            pic->m_longTermPicNum = pic->m_longTermFrameIdx;
        else {
            if (pic->m_structure == picture->m_structure)
                pic->m_longTermPicNum = 2 * pic->m_longTermFrameIdx + 1;
            else
                pic->m_longTermPicNum = 2 * pic->m_longTermFrameIdx;
        }
    }
}

void VaapiDPBManager::execPictureRefsModification(const PicturePtr& picture,
                                                  const SliceHeaderPtr& sliceHdr)
{
    /* refPicList0 */
    if (!H264_IS_I_SLICE(sliceHdr) && !H264_IS_SI_SLICE(sliceHdr) &&
        sliceHdr->ref_pic_list_modification_flag_l0)
        execPictureRefsModification1(picture, sliceHdr, 0);

    /* refPicList1 */
    if (H264_IS_B_SLICE(sliceHdr) &&
        sliceHdr->ref_pic_list_modification_flag_l1)
        execPictureRefsModification1(picture, sliceHdr, 1);
}

void VaapiDPBManager::execPictureRefsModification1(const PicturePtr& picture,
                                                   const SliceHeaderPtr&sliceHdr, uint32_t list)
{
    H264PPS *const pps = sliceHdr->pps;
    H264SPS *const sps = pps->sequence;
    H264RefPicListModification *refPicListModification;
    uint32_t numRefPicListModifications;
    VaapiDecPictureH264 **refList;
    uint32_t *refListCountPtr, refListCount, refListIdx = 0;
    uint32_t i, j, n, numRefs;
    int32_t foundRefIdx;
    int32_t maxPicNum, currPicNum, picNumPred;

    if (list == 0) {
        refPicListModification = sliceHdr->ref_pic_list_modification_l0;
        numRefPicListModifications =
            sliceHdr->n_ref_pic_list_modification_l0;
        refList = DPBLayer->refPicList0;
        refListCountPtr = &DPBLayer->refPicList0Count;
        numRefs = sliceHdr->num_ref_idx_l0_active_minus1 + 1;
    } else {
        refPicListModification = sliceHdr->ref_pic_list_modification_l1;
        numRefPicListModifications =
            sliceHdr->n_ref_pic_list_modification_l1;
        refList = DPBLayer->refPicList1;
        refListCountPtr = &DPBLayer->refPicList1Count;
        numRefs = sliceHdr->num_ref_idx_l1_active_minus1 + 1;
    }
    refListCount = *refListCountPtr;

    //FIXME: without this we will crash in MR3_TANDBERG_B.264
    //shold review this after we fix it
    for (int j = refListCount; j < numRefs; j++)
        refList[j] = NULL;
    //end

    if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_FRAME) {
        maxPicNum = 1 << (sps->log2_max_frame_num_minus4 + 5);  // 2 * maxFrameNum
        currPicNum = 2 * sliceHdr->frame_num + 1;   // 2 * frame_num + 1
    } else {
        maxPicNum = 1 << (sps->log2_max_frame_num_minus4 + 4);  // maxFrameNum
        currPicNum = sliceHdr->frame_num;   // frame_num
    }


    picNumPred = currPicNum;

    for (i = 0; i < numRefPicListModifications; i++) {
        H264RefPicListModification *const l = &refPicListModification[i];
        if (l->modification_of_pic_nums_idc == 3)
            break;

        /* 8.2.4.3.1 - Short-term reference pictures */
        if (l->modification_of_pic_nums_idc == 0
            || l->modification_of_pic_nums_idc == 1) {
            int32_t absDiffPicNum = l->value.abs_diff_pic_num_minus1 + 1;
            int32_t picNum, picNumNoWrap;

            // (8-34)
            if (l->modification_of_pic_nums_idc == 0) {
                picNumNoWrap = picNumPred - absDiffPicNum;
                if (picNumNoWrap < 0)
                    picNumNoWrap += maxPicNum;
            }
            // (8-35)
            else {
                picNumNoWrap = picNumPred + absDiffPicNum;
                if (picNumNoWrap >= maxPicNum)
                    picNumNoWrap -= maxPicNum;
            }
            picNumPred = picNumNoWrap;

            // (8-36)
            picNum = picNumNoWrap;
            if (picNum > currPicNum)
                picNum -= maxPicNum;

            // (8-37)
            for (j = numRefs; j > refListIdx; j--)
                refList[j] = refList[j - 1];
            foundRefIdx = findShortTermReference(picNum);
            refList[refListIdx++] =
                foundRefIdx >= 0 ? DPBLayer->shortRef[foundRefIdx] : NULL;
            n = refListIdx;
            for (j = refListIdx; j <= numRefs; j++) {
                int32_t picNumF;
                if (!refList[j])
                    continue;
                picNumF =
                    VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(refList[j])
                    ? refList[j]->m_picNum : maxPicNum;
                if (picNumF != picNum)
                    refList[n++] = refList[j];
            }
        }

        /* 8.2.4.3.2 - Long-term reference pictures */
        else if (l->modification_of_pic_nums_idc == 2) {

            for (j = numRefs; j > refListIdx; j--)
                refList[j] = refList[j - 1];
            foundRefIdx =
                findLongTermReference(l->value.long_term_pic_num);
            refList[refListIdx++] =
                foundRefIdx >= 0 ? DPBLayer->longRef[foundRefIdx] : NULL;
            n = refListIdx;
            for (j = refListIdx; j <= numRefs; j++) {
                uint32_t longTermPicNumF;
                if (!refList[j])
                    continue;
                longTermPicNumF =
                    VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(refList[j])
                    ? refList[j]->m_longTermPicNum : std::numeric_limits<int32_t>::max();
                if (longTermPicNumF != l->value.long_term_pic_num)
                    refList[n++] = refList[j];
            }
        }

    }

    for (i = numRefs; i > 0 && !refList[i - 1]; i--);
    *refListCountPtr = i;
}


bool VaapiDPBManager::execRefPicMarkingAdaptive(const PicturePtr& picture,
                                                H264DecRefPicMarking *decRefPicMarking,
                                                bool * hasMMCO5)
{
    uint32_t i;
    for (i = 0; i < decRefPicMarking->n_ref_pic_marking; i++) {
        H264RefPicMarking *const refPicMarking =
            &decRefPicMarking->ref_pic_marking[i];

        uint32_t MMCO = refPicMarking->memory_management_control_operation;
        if (MMCO == 5)
            *hasMMCO5 = true;

        execRefPicMarkingAdaptive1(picture, refPicMarking, MMCO);
    }
    return true;
}

bool VaapiDPBManager::execRefPicMarkingAdaptive1(const PicturePtr& picture,
                                                 H264RefPicMarking *refPicMarking,
                                                 uint32_t MMCO)
{
    uint32_t picNumX, longTermFrameIdxPlus1, i;
    VaapiDecPictureH264 *refPicture;
    int32_t foundIdx = 0;

    switch (MMCO) {
    case 1:
        {
            picNumX = getPicNumX(picture, refPicMarking);
            foundIdx = findShortTermReference(picNumX);
            if (foundIdx < 0)
                return false;

            i = (uint32_t) foundIdx;
            setH264PictureReference(DPBLayer->shortRef[i], 0,
                                    VAAPI_PICTURE_IS_FRAME(picture));
            ARRAY_REMOVE_INDEX(DPBLayer->shortRef, i);
        }
        break;
    case 2:
        {
            foundIdx =
                findLongTermReference(refPicMarking->long_term_pic_num);
            if (foundIdx < 0)
                return false;

            i = (uint32_t) foundIdx;
            setH264PictureReference(DPBLayer->longRef[i], 0,
                                    VAAPI_PICTURE_IS_FRAME(picture));
            ARRAY_REMOVE_INDEX(DPBLayer->longRef, i);
        }
        break;
    case 3:
        {
            for (i = 0; i < DPBLayer->longRefCount; i++) {
                if (DPBLayer->longRef[i]->m_longTermFrameIdx ==
                    refPicMarking->long_term_frame_idx)
                    break;
            }

            if (i != DPBLayer->longRefCount) {
                setH264PictureReference(DPBLayer->longRef[i], 0, true);
                ARRAY_REMOVE_INDEX(DPBLayer->longRef, i);
            }

            picNumX = getPicNumX(picture, refPicMarking);
            foundIdx = findShortTermReference(picNumX);
            if (foundIdx < 0)
                return false;

            i = (uint32_t) foundIdx;
            refPicture = DPBLayer->shortRef[i];
            ARRAY_REMOVE_INDEX(DPBLayer->shortRef, i);
            DPBLayer->longRef[DPBLayer->longRefCount++] = refPicture;

            refPicture->m_longTermFrameIdx =
                refPicMarking->long_term_frame_idx;
            setH264PictureReference(refPicture,
                                    VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE,
                                    VAAPI_PICTURE_IS_COMPLETE(picture));
        }
        break;
    case 4:
        {
            longTermFrameIdxPlus1 =
                refPicMarking->max_long_term_frame_idx_plus1;

            for (i = 0; i < DPBLayer->longRefCount; i++) {
                if (DPBLayer->longRef[i]->m_longTermFrameIdx + 1 <=
                    longTermFrameIdxPlus1)
                    continue;
                setH264PictureReference(DPBLayer->longRef[i], 0, false);
                ARRAY_REMOVE_INDEX(DPBLayer->longRef, i);
                i--;
            }
        }
        break;
    case 5:
        {
            /* The picture shall be inferred to have had frame_num equal to 0 (7.4.3) */
            picture->m_frameNum = 0;

            /* Update TopFieldOrderCnt and BottomFieldOrderCnt (8.2.1) */
            if (picture->m_structure !=
                VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
                picture->m_fieldPoc[TOP_FIELD] -= picture->m_POC;
            if (picture->m_structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
                picture->m_fieldPoc[BOTTOM_FIELD] -= picture->m_POC;

            picture->m_POC = 0;

            if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                removeShortReference(picture);

            flushDPB();
        }
        break;
    case 6:
        {
            for (i = 0; i < DPBLayer->longRefCount; i++) {
                if (DPBLayer->longRef[i]->m_longTermFrameIdx ==
                    refPicMarking->long_term_frame_idx)
                    break;
            }

            if (i != DPBLayer->longRefCount) {
                setH264PictureReference(DPBLayer->longRef[i], 0, true);
                ARRAY_REMOVE_INDEX(DPBLayer->longRef, i);
            }

            picture->m_longTermFrameIdx =
                refPicMarking->long_term_frame_idx;
            setH264PictureReference(picture.get(),
                                    VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE,
                                    VAAPI_PICTURE_IS_COMPLETE(picture));
        }
        break;
    default:
        ERROR("unsupported MMCO type %d", MMCO);
        break;
    }
    return true;
}

bool VaapiDPBManager::execRefPicMarkingSlidingWindow(const PicturePtr& picture)
{
    H264PPS *const pps = picture->m_pps;
    H264SPS *const sps = pps->sequence;
    VaapiDecPictureH264 *refPicture;
    uint32_t i, m, maxNumRefFrames;

    if (!VAAPI_PICTURE_IS_FIRST_FIELD(picture))
        return true;

    maxNumRefFrames = sps->num_ref_frames;

    if (maxNumRefFrames == 0)
        maxNumRefFrames = 1;
    if (!VAAPI_PICTURE_IS_FRAME(picture))
        maxNumRefFrames <<= 1;

    if (DPBLayer->shortRefCount + DPBLayer->longRefCount < maxNumRefFrames)
        return true;
    if (DPBLayer->shortRefCount < 1)
        return false;

    for (m = 0, i = 1; i < DPBLayer->shortRefCount; i++) {
        VaapiDecPictureH264 *const pic = DPBLayer->shortRef[i];
        if (pic->m_frameNumWrap < DPBLayer->shortRef[m]->m_frameNumWrap)
            m = i;
    }

    refPicture = DPBLayer->shortRef[m];
    setH264PictureReference(refPicture, 0, true);
    ARRAY_REMOVE_INDEX(DPBLayer->shortRef, m);

    /* Both fields need to be marked as "unused for reference", so
       remove the other field from the shortRef[] list as well */
    if (!VAAPI_PICTURE_IS_FRAME(picture)) {
        PicturePtr strong = refPicture->m_otherField.lock();
        VaapiDecPictureH264* other = strong.get();
        if (other) {
            for (i = 0; i < DPBLayer->shortRefCount; i++) {
                if (DPBLayer->shortRef[i] == other) {
                    ARRAY_REMOVE_INDEX(DPBLayer->shortRef, i);
                    break;
                }
            }
        }
    }

    return true;
}

int32_t VaapiDPBManager::findShortTermReference(uint32_t picNum)
{
    uint32_t i;

    for (i = 0; i < DPBLayer->shortRefCount; i++) {
        if (DPBLayer->shortRef[i]->m_picNum == picNum)
            return i;
    }
    ERROR("found no short-term reference picture with PicNum = %d",
          picNum);
    return -1;
}

int32_t VaapiDPBManager::findLongTermReference(uint32_t longTermPicNum)
{
    uint32_t i;

    for (i = 0; i < DPBLayer->longRefCount; i++) {
        if (DPBLayer->longRef[i]->m_longTermPicNum == longTermPicNum)
            return i;
    }
    ERROR("found no long-term reference picture with LongTermPicNum = %d",
          longTermPicNum);
    return -1;
}

void VaapiDPBManager::removeShortReference(const PicturePtr& picture)
{
    VaapiDecPictureH264 *refPicture;
    uint32_t i;
    uint32_t frameNum = picture->m_frameNum;

    PicturePtr strong = picture->m_otherField.lock();
    VaapiDecPictureH264* other = strong.get();
    for (i = 0; i < DPBLayer->shortRefCount; ++i) {
        if (DPBLayer->shortRef[i]->m_frameNum == frameNum) {
            refPicture = DPBLayer->shortRef[i];
            if (refPicture != other) {
                setH264PictureReference(refPicture, 0, false);
                ARRAY_REMOVE_INDEX(DPBLayer->shortRef, i);
            }
            return;
        }
    }
}

void VaapiDPBManager::removeDPBIndex(uint32_t index)
{
    uint32_t i, numFrames = --DPBLayer->DPBCount;

    /* delete the frame store */
    DPBLayer->DPB[index].reset();

    if (USE_STRICT_DPB_ORDERING) {
        for (i = index; i < numFrames; i++)
            DPBLayer->DPB[i] = DPBLayer->DPB[i + 1];
    } else if (index != numFrames)
        DPBLayer->DPB[index] = DPBLayer->DPB[numFrames];

    DPBLayer->DPB[numFrames].reset();

}
}
