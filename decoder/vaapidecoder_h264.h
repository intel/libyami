/*
 *  vaapidecoder_h264.h 
 *
 *  Copyright (C) 2011-2014 Intel Corporation
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

#ifndef vaapidecoder_h264_h
#define vaapidecoder_h264_h

#include "codecparsers/h264parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include <list>

#define TOP_FIELD    0
#define BOTTOM_FIELD 1

//#define MAX_VIEW_NUM 2
namespace YamiMediaCodec{
/* extended picture flags for h264 */
enum {
    VAAPI_PICTURE_FLAG_IDR = (VAAPI_PICTURE_FLAG_LAST << 0),
    VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE =
        (VAAPI_PICTURE_FLAG_REFERENCE),
    VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE =
        (VAAPI_PICTURE_FLAG_REFERENCE | (VAAPI_PICTURE_FLAG_LAST << 1)),
    VAAPI_PICTURE_FLAGS_REFERENCE =
        (VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
         VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE),
};

#define VAAPI_H264_PICTURE_IS_IDR(picture) \
    (VAAPI_PICTURE_FLAG_IS_SET(((picture)), VAAPI_PICTURE_FLAG_IDR))

#define VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture)   \
    ((VAAPI_PICTURE_FLAGS((picture)) &                  \
      VAAPI_PICTURE_FLAGS_REFERENCE) ==                       \
     VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE)

#define VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture)  \
    ((VAAPI_PICTURE_FLAGS((picture)) &                \
      VAAPI_PICTURE_FLAGS_REFERENCE) ==                     \
     VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE)

//FIXME:move this to .cpp
class VaapiDecPictureH264 : public VaapiDecPicture
{
  public:
    typedef std::tr1::shared_ptr<VaapiDecPictureH264> PicturePtr;
    typedef std::tr1::weak_ptr<VaapiDecPictureH264> PictureWeakPtr;
    typedef std::tr1::shared_ptr<H264SliceHdr> SliceHeaderPtr;
    friend class VaapiDPBManager;
    friend class VaapiDecoderH264;
    friend class VaapiFrameStore;

    virtual ~VaapiDecPictureH264() {}

  private:
    //make this public after we move this to cpp.
    VaapiDecPictureH264(ContextPtr context, const SurfacePtr& surface, int64_t timeStamp):
        VaapiDecPicture(context, surface, timeStamp),
        m_pps(NULL),
        m_flags(0),
        m_POC(0),
        m_structure(0),
        m_picStructure(0),
        m_frameNum(0),
        m_frameNumWrap(0),
        m_longTermFrameIdx(0),
        m_picNum(0),
        m_longTermPicNum(0),
        m_outputFlag(0),
        m_outputNeeded(0)
    {
        m_fieldPoc[0] = INVALID_POC;
        m_fieldPoc[1] = INVALID_POC;
    }

    PicturePtr newField()
    {
        PicturePtr field(new VaapiDecPictureH264(m_context, m_surface, m_timeStamp));
        if (!field)
            return field;
        field->m_frameNum = m_frameNum;
        return field;
    }

    bool newSlice(VASliceParameterBufferH264*& sliceParam, const void* sliceData, uint32_t sliceSize, const SliceHeaderPtr& header)
    {
        if (!VaapiDecPicture::newSlice(sliceParam, sliceData, sliceSize))
            return false;
        m_headers.push_back(header);
        return true;
    }

    H264SliceHdr* getLastSliceHeader()
    {
        if (m_headers.empty())
            return NULL;
        return m_headers.back().get();
    }

  public: // XXXX temp declare it as public for local function in dpb
    H264PPS * m_pps;
    uint32_t m_flags;
    int32_t  m_POC; // XXX, duplicate ?
    uint32_t m_structure;
    uint32_t m_picStructure; // XXX, seems duplicate with m_structure. since the macro in vaapipicture.h uses it, add it back temporarily
    int32_t m_fieldPoc[2];
    int32_t m_frameNum;
    int32_t m_frameNumWrap;
    uint32_t m_longTermFrameIdx;
    int32_t m_picNum;
    int32_t m_longTermPicNum;
    uint32_t m_outputFlag;
    uint32_t m_outputNeeded;
    PictureWeakPtr m_otherField;

  private:
    //FIXME: do we really need a list, or just last header?
    std::list<SliceHeaderPtr> m_headers;
};

class VaapiFrameStore {
    typedef VaapiDecPictureH264::PicturePtr PicturePtr;
  public:
    typedef std::tr1::shared_ptr<VaapiFrameStore> Ptr;
    VaapiFrameStore(const PicturePtr& pic);
    ~VaapiFrameStore();
    bool addPicture(const PicturePtr& pic);
    bool splitFields();
    bool hasFrame();
    bool hasReference();

    uint32_t m_structure;
    PicturePtr  m_buffers[2];
    uint32_t m_numBuffers;
    uint32_t m_outputNeeded;

  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiFrameStore);
};

struct VaapiDecPicBufLayer {
    typedef std::tr1::shared_ptr<VaapiDecPicBufLayer> Ptr;
    VaapiDecPicBufLayer(uint32_t size)
    {
        memset(&DPBCount, 0, sizeof(*this) - offsetof(VaapiDecPicBufLayer, DPBCount));
        DPBSize = size;
    }
    VaapiFrameStore::Ptr DPB[16];
    uint32_t DPBCount;
    uint32_t DPBSize;
    VaapiDecPictureH264 *shortRef[32];
    uint32_t shortRefCount;
    VaapiDecPictureH264 *longRef[32];
    uint32_t longRefCount;
    VaapiDecPictureH264 *refPicList0[32];
    uint32_t refPicList0Count;
    VaapiDecPictureH264 *refPicList1[32];
    uint32_t refPicList1Count;
    bool isValid;
  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiDecPicBufLayer);
} ;

class VaapiDecoderH264;

class VaapiDPBManager {
  public:
    typedef std::tr1::shared_ptr<VaapiDPBManager> Ptr;
    typedef VaapiDecPictureH264::PicturePtr PicturePtr;
    typedef VaapiDecPictureH264::SliceHeaderPtr SliceHeaderPtr;
    VaapiDPBManager(VaapiDecoderH264* decoder, uint32_t DPBSize);
    ~VaapiDPBManager();

    /* Decode Picture Buffer operations */
    bool outputDPB(const VaapiFrameStore::Ptr &frameStore, const PicturePtr& pic);
    void evictDPB(uint32_t i);
    bool bumpDPB();
    void clearDPB();
    void drainDPB();
    void flushDPB();
    bool addDPB(const VaapiFrameStore::Ptr &newFrameStore, const PicturePtr& pic);
    void resetDPB(H264SPS * sps);
    /* initialize and reorder reference list */
    void initPictureRefs(const PicturePtr& pic,
                         const SliceHeaderPtr& sliceHdr, int32_t frameNum);
    /* marking pic after slice decoded */
    bool execRefPicMarking(const PicturePtr& pic, bool * hasMMCO5);
    PicturePtr addDummyPicture(const PicturePtr& pic,
                               int32_t frameNum);
    bool execDummyPictureMarking(const PicturePtr& dummyPic,
                                 const SliceHeaderPtr& sliceHdr,
                                 int32_t frameNum);
  private:
    bool outputImmediateBFrame();
    /* prepare reference list before decoding slice */
    void initPictureRefLists(const PicturePtr& pic);

    void initPictureRefsPSlice(const PicturePtr& pic,
                               const SliceHeaderPtr& sliceHdr);
    void initPictureRefsBSlice(const PicturePtr& pic,
                               const SliceHeaderPtr& sliceHdr);
    void initPictureRefsFields(const PicturePtr& picture,
                               VaapiDecPictureH264 * refPicList[32],
                               uint32_t * refPicListCount,
                               VaapiDecPictureH264 * shortRef[32],
                               uint32_t shortRefCount,
                               VaapiDecPictureH264 * longRef[32],
                               uint32_t longRefCount);

    void initPictureRefsFields1(uint32_t pictureStructure,
                                VaapiDecPictureH264 * refPicList[32],
                                uint32_t * refPicListCount,
                                VaapiDecPictureH264 * refList[32],
                                uint32_t refListCount);

    void initPictureRefsPicNum(const PicturePtr& pic,
                               const SliceHeaderPtr& sliceHdr, int32_t frameNum);

    void execPictureRefsModification(const PicturePtr& picture,
                                     const SliceHeaderPtr& sliceHdr);

    void execPictureRefsModification1(const PicturePtr& picture,
                                      const SliceHeaderPtr& sliceHdr,
                                      uint32_t list);

    /* marking reference list after decoding slice */
    bool execRefPicMarkingAdaptive(const PicturePtr& picture,
                                   H264DecRefPicMarking *
                                   decRefPicMarking, bool * hasMMCO5);
    bool execRefPicMarkingAdaptive1(const PicturePtr& picture,
                                    H264RefPicMarking *
                                    refPicMarking, uint32_t MMCO);

    bool execRefPicMarkingSlidingWindow(const PicturePtr& picture);

    int32_t findShortRermReference(uint32_t picNum);
    int32_t findLongTermReference(uint32_t longTermPicNum);
    void removeShortReference(const PicturePtr& picture);
    void removeDPBIndex(uint32_t idx);
    void debugDPBStatus();

 public:
    VaapiDecPicBufLayer::Ptr DPBLayer;
    VaapiFrameStore::Ptr m_prevFrameStore; // in case a non-ref B frame to be rendered immediate after decoding, but wait for the completion of the frame (both top and bottom field is ready)

 private:
    VaapiDecoderH264* m_decoder;
    DISALLOW_COPY_AND_ASSIGN(VaapiDPBManager);
};

class VaapiDecoderH264:public VaapiDecoderBase {
 public:
    typedef VaapiDecPictureH264::PicturePtr PicturePtr;
    typedef VaapiDecPictureH264::SliceHeaderPtr SliceHeaderPtr;
    VaapiDecoderH264();
    virtual ~ VaapiDecoderH264();
    virtual Decode_Status start(VideoConfigBuffer * buffer);
    virtual Decode_Status reset(VideoConfigBuffer * buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer * buf);
    virtual const VideoRenderBuffer *getOutput(bool draining = false);
    virtual void flushOutport(void);

    //FIXME: make this private
    Decode_Status outputPicture(PicturePtr& picture);

  public:
    VaapiFrameStore::Ptr m_prevFrame;
    int32_t m_frameNum;         // frame_num (from slice_header())
    int32_t m_prevFrameNum;     // prevFrameNum
    bool m_prevPicHasMMCO5;     // prevMMCO5Pic
    uint32_t m_progressiveSequence;
    bool m_prevPicStructure;    // previous picture structure
    int32_t m_frameNumOffset;   // FrameNumOffset

  private:
    Decode_Status decodeSPS(H264NalUnit * nalu);
    Decode_Status decodePPS(H264NalUnit * nalu);
    Decode_Status decodeSEI(H264NalUnit * nalu);
    Decode_Status decodeSequenceEnd();

    /* initialize picture */
    void initPicturePOC0(const PicturePtr& picture,
                         const SliceHeaderPtr& sliceHdr);
    void initPicturePOC1(const PicturePtr& picture,
                         const SliceHeaderPtr& sliceHdr);
    void initPicturePOC2(const PicturePtr& picture,
                         const SliceHeaderPtr& sliceHdr);
    void initPicturePOC(const PicturePtr& picture,
                        const SliceHeaderPtr& sliceHdr);
    bool initPicture(const PicturePtr& picture,
                     const SliceHeaderPtr& sliceHdr, H264NalUnit * nalu);
    /* fill vaapi parameters */
    void vaapiInitPicture(VAPictureH264 * pic);
    void vaapiFillPicture(VAPictureH264 * pic,
                          const VaapiDecPictureH264* const picture,
                          uint32_t pictureStructure);
    bool fillPicture(const PicturePtr& picture,
                     const SliceHeaderPtr& sliceHdr, H264NalUnit * nalu);
    /* fill Quant matrix parameters */
    bool ensureQuantMatrix(const PicturePtr& pic);
    /* fill slice parameter buffer */
    bool fillPredWeightTable(VASliceParameterBufferH264 *,
                             const SliceHeaderPtr&);
    bool fillRefPicList(VASliceParameterBufferH264 *,
                             const SliceHeaderPtr&);
    bool fillSlice(VASliceParameterBufferH264 *,
                    const SliceHeaderPtr&, H264NalUnit * nalu);
    /* check the context reset senerios */
    Decode_Status ensureContext(H264PPS * pps);
    /* decoding functions */
    bool isNewPicture(H264NalUnit * nalu, const SliceHeaderPtr&);

    bool markingPicture(const PicturePtr& pic);
    bool storeDecodedPicture(const PicturePtr pic);
    Decode_Status decodeCurrentPicture();
    Decode_Status decodePicture(H264NalUnit * nalu,
                                const SliceHeaderPtr& sliceHdr);
    Decode_Status decodeSlice(H264NalUnit * nalu);
    Decode_Status decodeNalu(H264NalUnit * nalu);
    bool decodeCodecData(uint8_t * buf, uint32_t bufSize);
    void updateFrameInfo();
    bool processForGapsInFrameNum(const PicturePtr& pic,
                                  const SliceHeaderPtr& sliceHdr);

  private:
    PicturePtr m_currentPicture;
    VaapiDPBManager::Ptr m_DPBManager;
    H264NalParser m_parser;
    H264SPS m_lastSPS;
    H264PPS m_lastPPS;
    uint32_t m_mbWidth;
    uint32_t m_mbHeight;
    int32_t m_fieldPoc[2];      // 0:TopFieldOrderCnt / 1:BottomFieldOrderCnt
    int32_t m_POCMsb;           // PicOrderCntMsb
    int32_t m_POCLsb;           // pic_order_cnt_lsb (from slice_header())
    int32_t m_prevPOCMsb;       // prevPicOrderCntMsb
    int32_t m_prevPOCLsb;       // prevPicOrderCntLsb
    uint64_t m_prevTimeStamp;

    uint32_t m_gotSPS:1;
    uint32_t m_gotPPS:1;
    uint32_t m_hasContext:1;
    uint64_t m_nalLengthSize;
    bool m_isAVC;
    bool m_resetContext;
    DISALLOW_COPY_AND_ASSIGN(VaapiDecoderH264);
};

uint32_t getMaxDecFrameBuffering(H264SPS * sps, uint32_t views);

enum {
    H264_EXTRA_SURFACE_NUMBER = 11,
    MAX_REF_NUMBER = 16,
    DPB_SIE = 17,
    REF_LIST_SIZE = 32,
};
}

#endif
