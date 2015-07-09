/*
 *  vaapidecoder_h265.h
 *
 *  Copyright (C) 2014 Intel Corporation
 *    Author: Sreerenj Balachandran <sreerenj.balachandran@intel.com>
 *    Author: YuJiankang<jiankang.yu@intel.com>
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

#ifndef vaapidecoder_h265_h
#define vaapidecoder_h265_h

#include "codecparsers/h265parser.h"
#include "vaapidecoder_base.h"
#include "vaapidecpicture.h"
#include "va/va.h"
#include <vector>

namespace YamiMediaCodec{

class VaapiDecPictureH265 : public VaapiDecPicture
{
  public:
    typedef std::tr1::shared_ptr<VaapiDecPictureH265> PicturePtr;
    typedef std::tr1::weak_ptr<VaapiDecPictureH265> PictureWeakPtr;
    typedef std::tr1::shared_ptr<H265SliceHdr> SliceHeaderPtr;
    friend class VaapiDecoderH265;

    virtual ~VaapiDecPictureH265() {}

  private:
    VaapiDecPictureH265(ContextPtr context, const SurfacePtr& surface, int64_t timeStamp):
        VaapiDecPicture(context, surface, timeStamp),
        m_flags(0),
        m_structure(0),
        m_poc(0),
        m_poc_lsb(0),
        m_pic_latency_cnt(0),
        m_output_flag(0),
        m_output_needed(0),
        m_NoRaslOutputFlag(0),
        m_NoOutputOfPriorPicsFlag(0),
        m_RapPicFlag(0),
        m_IntraPicFlag(0),
        m_param(0)
    {
    }

  public:
    uint32_t m_flags;
    H265SliceHdr *m_last_slice_hdr;
    uint32_t m_structure;
    int32_t m_poc;
    int32_t m_poc_lsb;
    uint32_t m_pic_latency_cnt;
    uint32_t m_output_flag;
    uint32_t m_output_needed;
    uint32_t m_NoRaslOutputFlag;
    uint32_t m_NoOutputOfPriorPicsFlag;
    uint32_t m_RapPicFlag;
    uint32_t m_IntraPicFlag;
    void *m_param;

};

class VaapiH265FrameStore {
    typedef VaapiDecPictureH265::PicturePtr PicturePtr;
  public:
    typedef std::tr1::shared_ptr<VaapiH265FrameStore> Ptr;
    VaapiH265FrameStore(PicturePtr& pic);
    ~VaapiH265FrameStore();

    PicturePtr buffer;

  private:
    DISALLOW_COPY_AND_ASSIGN(VaapiH265FrameStore);
};

class VaapiDecoderH265:public VaapiDecoderBase {
  public:
    typedef VaapiDecPictureH265::PicturePtr PicturePtr;
    VaapiDecoderH265();
    virtual ~ VaapiDecoderH265();
    virtual Decode_Status start(VideoConfigBuffer * );
    virtual Decode_Status reset(VideoConfigBuffer * );
    virtual void stop(void);
    virtual void flush(void);
    virtual Decode_Status decode(VideoDecodeBuffer *);

  private:
    Decode_Status ensureContext();
    Decode_Status decodeNalu(H265NalUnit * nalu);
    uint32_t get_slice_data_byte_offset (H265SliceHdr * slice_hdr, uint32_t nal_header_bytes);
    bool fillPredWeightTable(VASliceParameterBufferHEVC*sliceParam);
    bool fillSlice(VaapiDecPictureH265 * picture, VASliceParameterBufferHEVC* sliceParam, H265SliceHdr * sliceHdr, H265NalUnit * nalu);
    bool fillPicture(PicturePtr& picture, H265NalUnit * nalu);
    void fill_iq_matrix_4x4 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    void fill_iq_matrix_8x8 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    void fill_iq_matrix_16x16 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    void fill_iq_matrix_32x32 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    void fill_iq_matrix_dc_16x16 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    void fill_iq_matrix_dc_32x32 (VAIQMatrixBufferHEVC * iq_matrix, H265ScalingList * scaling_list);
    bool ensureQuantMatrix(PicturePtr& pic);
    Decode_Status decodeSlice(H265NalUnit * nalu);
    Decode_Status decodeSPS(H265NalUnit * nalu);
    Decode_Status decodePPS(H265NalUnit * nalu);
    Decode_Status decodeSEI(H265NalUnit * nalu);

    void vaapi_init_picture (VAPictureHEVC * pic);
    void vaapi_fill_picture (VAPictureHEVC * pic, VaapiDecPictureH265 * picture,
        uint32_t picture_structure);
    uint32_t get_index_for_RefPicListX (VAPictureHEVC * ReferenceFrames,
        VaapiDecPictureH265 * pic);

    uint32_t  get_max_dec_frame_buffering (H265SPS * sps);
    Decode_Status ensure_dpb (H265SPS * sps);
    void init_picture_poc (PicturePtr& picture, H265SliceHdr *slice_hdr, H265NalUnit * nalu);
    void vaapi_picture_h265_set_reference(VaapiDecPictureH265* picture, uint32_t referenceFlags);
    VaapiDecPictureH265 *dpb_get_picture (int32_t poc, bool match_lsb);
    VaapiDecPictureH265 *dpb_get_ref_picture (int32_t poc, bool is_short);
    void dpb_remove_index(uint32_t index);
    int32_t dpb_find_lowest_poc (VaapiDecPictureH265 ** found_picture_ptr);
    bool vaapi_frame_store_has_reference(VaapiH265FrameStore::Ptr fs);
    bool dpb_output(VaapiH265FrameStore::Ptr &frameStore);
    bool dpb_bump (VaapiDecPictureH265 * picture);
    void dpb_clear (bool hard_flush);
    void dpb_flush ();
    int32_t dpb_get_num_need_output ();
    bool check_latency_cnt ();
    bool dpb_add (PicturePtr& pic,  H265SliceHdr *slice_hdr, H265NalUnit * nalu);
    bool dpb_init (VaapiDecPictureH265 * picture, H265SliceHdr *slice_hdr,  H265NalUnit * nalu);
    Decode_Status dpb_reset (uint32_t dpb_size);
    bool is_new_picture (H265SliceHdr *slice_hdr);
    bool has_entry_in_rps (VaapiDecPictureH265 * dpb_pic, VaapiDecPictureH265 ** rps_list, uint32_t rps_list_length);
    void derive_and_mark_rps (VaapiDecPictureH265 * picture, int32_t * CurrDeltaPocMsbPresentFlag, int32_t * FollDeltaPocMsbPresentFlag);
    bool decode_ref_pic_set (PicturePtr& picture, H265SliceHdr *slice_hdr, H265NalUnit * nalu);
    void init_picture_refs(VaapiDecPictureH265 * picture, H265SliceHdr * slice_hdr);
    bool fill_pred_weight_table (VASliceParameterBufferHEVC * slice_param, H265SliceHdr * slice_hdr);
    bool fill_RefPicList (VaapiDecPictureH265 * picture, VASliceParameterBufferHEVC* slice_param, H265SliceHdr * slice_hdr);
    bool init_picture (PicturePtr& picture,  H265SliceHdr *slice_hdr, H265NalUnit * nalu);
private:
    PicturePtr m_currentPicture;
    VaapiH265FrameStore::Ptr m_dpb[16];

    //folling member will be memset to zero in constructor
    uint8_t m_zeroInitStart;
    H265Parser m_parser;
    H265SPS m_lastSPS;
    H265PPS m_lastPPS;
    uint32_t m_mbWidth;
    uint32_t m_mbHeight;
    uint32_t m_gotSPS:1;
    uint32_t m_gotPPS:1;
    uint32_t m_dpb_size;
    uint32_t m_dpb_count;

    H265SliceHdr m_slice_hdr;
    H265SliceHdr *m_prev_slice;
    H265SliceHdr *m_prev_independent_slice;

    uint32_t m_prev_nal_is_eos;
    uint32_t m_associated_irap_NoRaslOutputFlag:1;

    uint32_t m_poc;
    uint32_t m_poc_msb;
    uint32_t m_poc_lsb;
    uint32_t m_prev_poc_msb;
    uint32_t m_prev_poc_lsb;
    uint32_t m_prev_tid0pic_poc_lsb;
    uint32_t m_prev_tid0pic_poc_msb;

    int32_t m_PocStCurrBefore[16];
    int32_t m_PocStCurrAfter[16];
    int32_t m_PocStFoll[16];
    int32_t m_PocLtCurr[16];
    int32_t m_PocLtFoll[16];
    uint32_t m_NumPocStCurrBefore;
    uint32_t m_NumPocStCurrAfter;
    uint32_t m_NumPocStFoll;
    uint32_t m_NumPocLtCurr;
    uint32_t m_NumPocLtFoll;
    uint32_t m_NumPocTotalCurr;
    uint32_t m_SpsMaxLatencyPictures;
    VaapiDecPictureH265 *m_RefPicSetStCurrBefore[16];
    VaapiDecPictureH265 *m_RefPicSetStCurrAfter[16];
    VaapiDecPictureH265 *m_RefPicSetStFoll[16];
    VaapiDecPictureH265 *m_RefPicSetLtCurr[16];
    VaapiDecPictureH265 *m_RefPicSetLtFoll[16];

    VaapiDecPictureH265 *m_RefPicList0[16];
    uint32_t m_RefPicList0_count;
    VaapiDecPictureH265 *m_RefPicList1[16];
    uint32_t m_RefPicList1_count;
    uint32_t m_flags;
    //memset end
    uint8_t m_zeroInitEnd;

    bool m_newBitstream;
    uint32_t m_nalLengthSize;

    static const bool s_registered; // VaapiDecoderFactory registration result

};
};

#endif

