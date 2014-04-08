/*
 *  vaapidecoder_h264.h 
 *
 *  Copyright (C) 2013 Intel Corporation
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
#include "vaapipicture.h"

#define TOP_FIELD    0
#define BOTTOM_FIELD 1

//#define MAX_VIEW_NUM 2

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

class VaapiSliceH264:public VaapiSlice {
  public:
    VaapiSliceH264(VADisplay display,
                   VAContextID ctx,
                   uint8_t * sliceData, uint32_t sliceSize);

    ~VaapiSliceH264();
    H264SliceHdr m_sliceHdr;
};

class VaapiPictureH264:public VaapiPicture {
  public:
    VaapiPictureH264(VADisplay display,
                     VAContextID context,
                     VaapiSurfaceBufferPool * surfBufPool,
                     VaapiPictureStructure structure);

    VaapiPictureH264 *newField();

  public:
    H264PPS * m_pps;
    uint32_t m_structure;
    int32_t m_fieldPoc[2];
    int32_t m_frameNum;
    int32_t m_frameNumWrap;
    int32_t m_longTermFrameIdx;
    int32_t m_picNum;
    int32_t m_longTermPicNum;
    uint32_t m_outputFlag;
    uint32_t m_outputNeeded;
    VaapiPictureH264 *m_otherField;
};

class VaapiFrameStore {
  public:
    VaapiFrameStore(VaapiPictureH264 * pic);
    ~VaapiFrameStore();
    void addPicture(VaapiPictureH264 * pic);
    bool splitFields();
    bool hasFrame();
    bool hasReference();

    uint32_t m_structure;
    VaapiPictureH264 *m_buffers[2];
    uint32_t m_numBuffers;
    uint32_t m_outputNeeded;
};

typedef struct _VaapiDecPicBufLayer {
    VaapiFrameStore *DPB[16];
    uint32_t DPBCount;
    uint32_t DPBSize;
    VaapiPictureH264 *shortRef[32];
    uint32_t shortRefCount;
    VaapiPictureH264 *longRef[32];
    uint32_t longRefCount;
    VaapiPictureH264 *refPicList0[32];
    uint32_t refPicList0Count;
    VaapiPictureH264 *refPicList1[32];
    uint32_t refPicList1Count;
    bool isValid;
} VaapiDecPicBufLayer;

class VaapiDecoderH264;

class VaapiDPBManager {
  public:
    VaapiDPBManager(uint32_t DPBSize);

    /* Decode Picture Buffer operations */
    bool outputDPB(VaapiFrameStore * frameStore, VaapiPictureH264 * pic);
    void evictDPB(VaapiPictureH264 * pic, uint32_t i);
    bool bumpDPB();
    void clearDPB();
    void flushDPB();
    bool addDPB(VaapiFrameStore * newFrameStore, VaapiPictureH264 * pic);
    void resetDPB(H264SPS * sps);
    /* initialize and reorder reference list */
    void initPictureRefs(VaapiPictureH264 * pic,
                         H264SliceHdr * sliceHdr, int32_t frameNum);
    /* marking pic after slice decoded */
    bool execRefPicMarking(VaapiPictureH264 * pic, bool * hasMMCO5);
  private:
    /* prepare reference list before decoding slice */
    void initPictureRefLists(VaapiPictureH264 * pic);

    void initPictureRefsPSlice(VaapiPictureH264 * pic,
                               H264SliceHdr * sliceHdr);
    void initPictureRefsBSlice(VaapiPictureH264 * pic,
                               H264SliceHdr * sliceHdr);
    void initPictureRefsFields(VaapiPictureH264 * picture,
                               VaapiPictureH264 * refPicList[32],
                               uint32_t * refPicListCount,
                               VaapiPictureH264 * shortRef[32],
                               uint32_t shortRefCount,
                               VaapiPictureH264 * longRef[32],
                               uint32_t longRefCount);

    void initPictureRefsFields1(uint32_t pictureStructure,
                                VaapiPictureH264 * refPicList[32],
                                uint32_t * refPicListCount,
                                VaapiPictureH264 * refList[32],
                                uint32_t refListCount);

    void initPictureRefsPicNum(VaapiPictureH264 * pic,
                               H264SliceHdr * sliceHdr, int32_t frameNum);

    void execPictureRefsModification(VaapiPictureH264 * picture,
                                     H264SliceHdr * sliceHdr);

    void execPictureRefsModification1(VaapiPictureH264 * picture,
                                      H264SliceHdr * sliceHdr,
                                      uint32_t list);

    /* marking reference list after decoding slice */
    bool execRefPicMarkingAdaptive(VaapiPictureH264 * picture,
                                   H264DecRefPicMarking *
                                   decRefPicMarking, bool * hasMMCO5);
    bool execRefPicMarkingAdaptive1(VaapiPictureH264 * picture,
                                    H264RefPicMarking *
                                    refPicMarking, uint32_t MMCO);

    bool execRefPicMarkingSlidingWindow(VaapiPictureH264 * picture);

    int32_t findShortRermReference(uint32_t picNum);
    int32_t findLongTermReference(uint32_t longTermPicNum);
    void removeShortReference(VaapiPictureH264 * picture);
    void removeDPBIndex(uint32_t idx);

  public:
    VaapiDecPicBufLayer * DPBLayer;
};

class VaapiDecoderH264:public VaapiDecoderBase {
  public:
    VaapiDecoderH264(const char *mimeType);
    virtual ~ VaapiDecoderH264();
    virtual Decode_Status start(VideoConfigBuffer * buffer);
    virtual Decode_Status reset(VideoConfigBuffer * buffer);
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer * buf);
    virtual const VideoRenderBuffer *getOutput(bool draining = false);

  public:
    VaapiFrameStore * m_prevFrame;
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
    void initPicturePOC0(VaapiPictureH264 * picture,
                         H264SliceHdr * sliceHdr);
    void initPicturePOC1(VaapiPictureH264 * picture,
                         H264SliceHdr * sliceHdr);
    void initPicturePOC2(VaapiPictureH264 * picture,
                         H264SliceHdr * sliceHdr);
    void initPicturePOC(VaapiPictureH264 * picture,
                        H264SliceHdr * sliceHdr);
    bool initPicture(VaapiPictureH264 * picture,
                     H264SliceHdr * sliceHdr, H264NalUnit * nalu);
    /* fill vaapi parameters */
    void vaapiInitPicture(VAPictureH264 * pic);
    void vaapiFillPicture(VAPictureH264 * pic,
                          VaapiPictureH264 * picture,
                          uint32_t pictureStructure);
    bool fillPicture(VaapiPictureH264 * picture,
                     H264SliceHdr * sliceHdr, H264NalUnit * nalu);
    /* fill Quant matrix parameters */
    bool ensureQuantMatrix(VaapiPictureH264 * pic);
    /* fill slice parameter buffer */
    bool fillPredWeightTable(VaapiSliceH264 * slice);
    bool fillRefPicList(VaapiSliceH264 * slice);
    bool fillSlice(VaapiSliceH264 * slice, H264NalUnit * nalu);
    /* check the context reset senerios */
    Decode_Status ensureContext(H264PPS * pps);
    /* decoding functions */
    bool isNewPicture(H264NalUnit * nalu, H264SliceHdr * sliceHdr);

    bool markingPicture(VaapiPictureH264 * pic);
    bool storeDecodedPicture(VaapiPictureH264 * pic);
    Decode_Status decodeCurrentPicture();
    Decode_Status decodePicture(H264NalUnit * nalu,
                                H264SliceHdr * sliceHdr);
    Decode_Status decodeSlice(H264NalUnit * nalu);
    Decode_Status decodeNalu(H264NalUnit * nalu);
    bool decodeCodecData(uint8_t * buf, uint32_t bufSize);
    void updateFrameInfo();

  private:
    VaapiPictureH264 * m_currentPicture;
    VaapiDPBManager *m_DPBManager;
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
};

uint32_t getMaxDecFrameBuffering(H264SPS * sps, uint32_t views);

enum {
    H264_EXTRA_SURFACE_NUMBER = 11,
    MAX_REF_NUMBER = 16,
    DPB_SIE = 17,
    REF_LIST_SIZE = 32,
};


#endif
