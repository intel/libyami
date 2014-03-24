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

#ifndef VAAPI_DECODER_H264_H
#define VAAPI_DECODER_H264_H

#include "vaapipicture.h"
#include "vaapidecoder_base.h"
#include "codecparsers/h264parser.h"

#define TOP_FIELD    0
#define BOTTOM_FIELD 1

//#define MAX_VIEW_NUM 2

/* extended picture flags for h264 */
enum {
    VAAPI_PICTURE_FLAG_IDR = (VAAPI_PICTURE_FLAG_LAST << 0),
    VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE = (
        VAAPI_PICTURE_FLAG_REFERENCE),
    VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE = (
        VAAPI_PICTURE_FLAG_REFERENCE | (VAAPI_PICTURE_FLAG_LAST << 1)),
    VAAPI_PICTURE_FLAGS_REFERENCE = (
        VAAPI_PICTURE_FLAG_SHORT_TERM_REFERENCE |
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

class VaapiSliceH264 : public VaapiSlice {
public:
   VaapiSliceH264(VADisplay display,
                  VAContextID ctx,
                  uint8_t * sliceData,
                  uint32_t  sliceSize);

   ~VaapiSliceH264();
   H264SliceHdr slice_hdr;
};

class VaapiPictureH264: public VaapiPicture{
public:
    VaapiPictureH264(VADisplay display,
                     VAContextID context,
                     VaapiSurfaceBufferPool *surfBufPool,
                     VaapiPictureStructure structure);

    VaapiPictureH264* new_field();

public: 
    H264PPS     *pps;
    uint32_t     structure;
    int32_t      field_poc[2];
    int32_t      frame_num;
    int32_t      frame_num_wrap;
    int32_t      long_term_frame_idx;
    int32_t      pic_num;
    int32_t      long_term_pic_num;
    uint32_t     output_flag;
    uint32_t     output_needed;
    VaapiPictureH264 *other_field;
};

class VaapiFrameStore {
public:
   VaapiFrameStore(VaapiPictureH264 *pic);
   ~VaapiFrameStore();
   void add_picture(VaapiPictureH264 *pic);
   bool split_fields();
   bool has_frame();
   bool has_reference();

   uint32_t structure;
   VaapiPictureH264 *buffers[2];
   uint32_t num_buffers;
   uint32_t output_needed;
};

typedef struct _VaapiDecPicBufLayer {
    VaapiFrameStore      *dpb[16];
    uint32_t              dpb_count;
    uint32_t              dpb_size;
    VaapiPictureH264     *short_ref[32];
    uint32_t              short_ref_count;
    VaapiPictureH264     *long_ref[32];
    uint32_t              long_ref_count;
    VaapiPictureH264     *RefPicList0[32];
    uint32_t              RefPicList0_count;
    VaapiPictureH264     *RefPicList1[32];
    uint32_t              RefPicList1_count;
    bool                  is_valid;
} VaapiDecPicBufLayer;

class VaapiDecoderH264;

class VaapiDPBManager{
public:
    VaapiDPBManager(uint32_t dpb_size);

    /* Decode Picture Buffer operations */
    bool dpb_output(VaapiFrameStore *fs, VaapiPictureH264 *pic); 
    void dpb_evict(VaapiPictureH264 *pic, uint32_t i);
    bool dpb_bump();
    void dpb_clear();
    void dpb_flush();
    bool dpb_add(VaapiFrameStore *new_fs, VaapiPictureH264 *pic);
    void dpb_reset(H264SPS *sps);
    /* initialize and reorder reference list */
    void init_picture_refs(VaapiPictureH264 *pic, H264SliceHdr *slice_hdr,
                           int32_t frame_num);
    /* marking pic after slice decoded */
    bool exec_ref_pic_marking(VaapiPictureH264 *pic, bool *has_mmco5);
private:
    /* prepare reference list before decoding slice */
    void init_picture_ref_lists(VaapiPictureH264 *pic);

    void init_picture_refs_p_slice(VaapiPictureH264 *pic, 
                         H264SliceHdr *slice_hdr);
    void init_picture_refs_b_slice(VaapiPictureH264 *pic, 
                         H264SliceHdr *slice_hdr);
    void init_picture_refs_fields(VaapiPictureH264 *picture,
                                  VaapiPictureH264 *RefPicList[32],
                                  uint32_t         *RefPicList_count,
                                  VaapiPictureH264 *short_ref[32],
                                  uint32_t          short_ref_count,
                                  VaapiPictureH264 *long_ref[32],
                                  uint32_t          long_ref_count);

    void init_picture_refs_fields_1(uint32_t picture_structure,
                                    VaapiPictureH264 *RefPicList[32],
                                    uint32_t         *RefPicList_count,
                                    VaapiPictureH264 *ref_list[32],
                                    uint32_t          ref_list_count);

    void init_picture_refs_pic_num(VaapiPictureH264 *pic, 
                                   H264SliceHdr *slice_hdr,
                                   int32_t frame_num);

    void exec_picture_refs_modification(VaapiPictureH264 *picture,
                                        H264SliceHdr *slice_hdr);

    void exec_picture_refs_modification_1(VaapiPictureH264 *picture,
                                          H264SliceHdr     *slice_hdr,
                                          uint32_t          list);

    /* marking reference list after decoding slice*/
    bool exec_ref_pic_marking_adaptive(VaapiPictureH264 *picture,
                              H264DecRefPicMarking *dec_ref_pic_marking,
                              bool *has_mmco5);
    bool exec_ref_pic_marking_adaptive_1(VaapiPictureH264 *picture,
                              H264RefPicMarking *ref_pic_marking,
                              uint32_t mmco);
 
    bool exec_ref_pic_marking_sliding_window(VaapiPictureH264 *picture);

    int32_t find_short_term_reference(uint32_t picNum);
    int32_t find_long_term_reference(uint32_t long_term_picNum);
    void remove_short_reference(VaapiPictureH264 *picture);
    void dpb_remove_index(uint32_t idx);

public:
    VaapiDecPicBufLayer  *dpb_layer;
};

class VaapiDecoderH264 : public VaapiDecoderBase {
public:
   VaapiDecoderH264(const char *mimeType);
   virtual ~VaapiDecoderH264();
   virtual Decode_Status start(VideoConfigBuffer *buffer);
   virtual Decode_Status reset(VideoConfigBuffer *buffer);
   virtual void stop(void);
   virtual void flush(void);
   virtual Decode_Status decode(VideoDecodeBuffer *buf);
   virtual const VideoRenderBuffer* getOutput(bool draining = false);
public:
   VaapiFrameStore      *m_prev_frame;
   int32_t               m_frame_num;              // frame_num (from slice_header())
   int32_t               m_prev_frame_num;         // prevFrameNum
   bool                  m_prev_pic_has_mmco5;     // prevMmco5Pic
   uint32_t              m_progressive_sequence;
   bool                  m_prev_pic_structure;     // previous picture structure
   int32_t               m_frame_num_offset;       // FrameNumOffset

private:
   Decode_Status decode_sps(H264NalUnit *nalu);
   Decode_Status decode_pps(H264NalUnit *nalu);
   Decode_Status decode_sei(H264NalUnit *nalu);
   Decode_Status decode_sequence_end();
 
   /* initialize picture */
   void init_picture_poc_0(VaapiPictureH264 *picture,
                           H264SliceHdr *slice_hdr);
   void init_picture_poc_1(VaapiPictureH264 *picture,
                           H264SliceHdr *slice_hdr);
   void init_picture_poc_2(VaapiPictureH264 *picture,
                           H264SliceHdr *slice_hdr);
   void init_picture_poc(VaapiPictureH264 *picture,
                         H264SliceHdr *slice_hdr);
   bool init_picture(VaapiPictureH264 *picture,
                     H264SliceHdr *slice_hdr,
                     H264NalUnit  *nalu);
   /* fill vaapi parameters */
   void vaapi_init_picture(VAPictureH264 *pic);
   void vaapi_fill_picture(VAPictureH264 *pic, 
                           VaapiPictureH264 *picture,
                           uint32_t picture_structure);
   bool fill_picture(VaapiPictureH264 *picture,
                     H264SliceHdr     *slice_hdr,
                     H264NalUnit      *nalu);
   /* fill Quant matrix parameters */   
   bool ensure_quant_matrix(VaapiPictureH264 *pic);
   /* fill slice parameter buffer*/
   bool fill_pred_weight_table(VaapiSliceH264 *slice);
   bool fill_RefPicList(VaapiSliceH264 *slice);
   bool fill_slice(VaapiSliceH264 *slice,
                   H264NalUnit   *nalu);
   /* check the context reset senerios */
   Decode_Status ensure_context(H264SPS *sps);
   /* decoding functions */
   bool is_new_picture(H264NalUnit *nalu,
                       H264SliceHdr *slice_hdr);
   
   bool marking_picture(VaapiPictureH264 *pic);
   bool store_decoded_picture(VaapiPictureH264 *pic);
   Decode_Status decode_current_picture();
   Decode_Status decode_picture(H264NalUnit *nalu, 
                       H264SliceHdr *slice_hdr);
   Decode_Status decode_slice(H264NalUnit *nalu);
   Decode_Status decode_nalu(H264NalUnit *nalu);
   bool decode_codec_data(uint8_t *buf, uint32_t buf_size);
   void update_frame_info();

private:
    VaapiPictureH264     *m_current_picture;
    VaapiDPBManager*      m_dpb_manager;
    H264NalParser         m_parser;
    H264SPS               m_last_sps;
    H264PPS               m_last_pps;
    uint32_t              m_mb_width;
    uint32_t              m_mb_height;
    int32_t               m_field_poc[2];  // 0:TopFieldOrderCnt / 1:BottomFieldOrderCnt
    int32_t               m_poc_msb;       // PicOrderCntMsb
    int32_t               m_poc_lsb;       // pic_order_cnt_lsb (from slice_header())
    int32_t               m_prev_poc_msb;  // prevPicOrderCntMsb
    int32_t               m_prev_poc_lsb;  // prevPicOrderCntLsb
    uint64_t              m_prev_time_stamp;

    uint32_t              m_got_sps            : 1;
    uint32_t              m_got_pps            : 1;
    uint32_t              m_has_context        : 1;
    uint64_t              m_nal_length_size;
    bool                  m_is_avc;
    bool                  m_reset_context;
}; 

uint32_t
get_max_dec_frame_buffering(H264SPS *sps, uint32_t views);

enum {
   H264_EXTRA_SURFACE_NUMBER = 11,
   MAX_REF_NUMBER = 16,
   DPB_SIE = 17,
   REF_LIST_SIZE = 32,
};

 
#endif
