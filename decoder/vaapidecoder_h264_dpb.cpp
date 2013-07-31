/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/
#include "vaapidecoder_h264.h"

/* Defined to 1 if strict ordering of DPB is needed. Only useful for debug */
#define USE_STRICT_DPB_ORDERING 0

#define ARRAY_REMOVE_INDEX(array, index) \
    do {  \
      uint32_t size = array##_count; \
      assert(index < size); \
      if (index != --size); \
           (array)[index] = (array)[size]; \
      array[size] = NULL; \
      array##_count = size;\
    }while(0);

#define SORT_REF_LIST(list, n, compare_func) \
    qsort(list, n, sizeof(*(list)), compare_picture_##compare_func)

static uint32_t round_log2(uint32_t value)
{
    uint32_t ret = 0;
    uint32_t value_square = value * value;
    while(( 1 << (ret + 1)) <= value_square)
       ++ret;

    ret = (ret + 1) >> 1;
    return ret;
}

static int
compare_picture_pic_num_dec(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picB->pic_num - picA->pic_num;
}

static int
compare_picture_long_term_pic_num_inc(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picA->long_term_pic_num - picB->long_term_pic_num;
}

static int
compare_picture_poc_dec(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picB->mPoc - picA->mPoc;
}

static int
compare_picture_poc_inc(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picA->mPoc - picB->mPoc;
}

static int
compare_picture_frame_num_wrap_dec(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picB->frame_num_wrap - picA->frame_num_wrap;
}

static int
compare_picture_long_term_frame_idx_inc(const void *a, const void *b)
{
    const VaapiPictureH264 * const picA = *(VaapiPictureH264 **)a;
    const VaapiPictureH264 * const picB = *(VaapiPictureH264 **)b;

    return picA->long_term_frame_idx - picB->long_term_frame_idx;
}

static void
h264_picture_set_reference(VaapiPictureH264 *picture, 
                           uint32_t    reference_flags,
                           bool        other_field)
{
    VAAPI_PICTURE_FLAG_UNSET(picture, VAAPI_PICTURE_FLAGS_REFERENCE);
    VAAPI_PICTURE_FLAG_SET(picture, reference_flags);

    if (!other_field || !(picture = picture->other_field))
        return;

    VAAPI_PICTURE_FLAG_UNSET(picture, VAAPI_PICTURE_FLAGS_REFERENCE);
    VAAPI_PICTURE_FLAG_SET(picture, reference_flags);
}

static int32_t
get_picNumX(VaapiPictureH264 *picture, H264RefPicMarking *ref_pic_marking)
{
    int32_t pic_num;

    if (VAAPI_PICTURE_IS_FRAME(picture))
        pic_num = picture->frame_num_wrap;
    else
        pic_num = 2 * picture->frame_num_wrap + 1;
    pic_num -= ref_pic_marking->difference_of_pic_nums_minus1 + 1;
    return pic_num;
}

uint32_t
get_max_dec_frame_buffering(H264SPS *sps, uint32_t views)
{
    uint32_t max_dec_frame_buffering;
    uint32_t MaxDpbMbs = 0;
    uint32_t PicSizeMbs = 0;
    //H264SPSExtMVC * const  mvc = &sps->extension.mvc;
 
    /* Table A-1 - Level limits */
    switch (sps->level_idc) {
    case 10: MaxDpbMbs = 396;    break;
    case 11: MaxDpbMbs = 900;    break;
    case 12: MaxDpbMbs = 2376;   break;
    case 13: MaxDpbMbs = 2376;   break;
    case 20: MaxDpbMbs = 2376;   break;
    case 21: MaxDpbMbs = 4752;   break;
    case 22: MaxDpbMbs = 8100;   break;
    case 30: MaxDpbMbs = 8100;   break;
    case 31: MaxDpbMbs = 18000;  break;
    case 32: MaxDpbMbs = 20480;  break;
    case 40: MaxDpbMbs = 32768;  break;
    case 41: MaxDpbMbs = 32768;  break;
    case 42: MaxDpbMbs = 34816;  break;
    case 50: MaxDpbMbs = 110400; break;
    case 51: MaxDpbMbs = 184320; break;
    default:
        assert(0);
        break;
    }

    PicSizeMbs = ((sps->pic_width_in_mbs_minus1 + 1) *
                  (sps->pic_height_in_map_units_minus1 + 1) *
                  (sps->frame_mbs_only_flag ? 1 : 2));
    max_dec_frame_buffering = MaxDpbMbs / PicSizeMbs;

    if(views > 1) {
       max_dec_frame_buffering = MIN( 2 * max_dec_frame_buffering,
                                      16 * MAX(1, round_log2(views)));
       max_dec_frame_buffering = max_dec_frame_buffering / views;
    }

    /* VUI parameters */
    if (sps->vui_parameters_present_flag) {
        H264VUIParams * const vui_params = &sps->vui_parameters;
        if (vui_params->bitstream_restriction_flag)
            max_dec_frame_buffering = vui_params->max_dec_frame_buffering;
        else {
            switch (sps->profile_idc) {
            case 44:  // CAVLC 4:4:4 Intra profile
            case 86:  // Scalable High profile
            case 100: // High profile
            case 110: // High 10 profile
            case 122: // High 4:2:2 profile
            case 244: // High 4:4:4 Predictive profile
                if (sps->constraint_set3_flag)
                    max_dec_frame_buffering = 0;
                break;
            }
        }
    }

    if (max_dec_frame_buffering > 16)
        max_dec_frame_buffering = 16;
    else if (max_dec_frame_buffering < sps->num_ref_frames)
        max_dec_frame_buffering = sps->num_ref_frames;
    return MAX(1, max_dec_frame_buffering);
}

VaapiSliceH264::VaapiSliceH264(VADisplay display,
    VAContextID ctx,
    uint8_t *sliceData,
    uint32_t sliceSize)
{
    VASliceParameterBufferH264* paramBuf;

    if (!display || !ctx) {
         ERROR("VA display or context not initialized yet");
         return;
    } 

    /* new h264 slice data buffer */ 
    mData = new VaapiBufObject(display,
                 ctx,
                 VASliceDataBufferType,
                 sliceData, 
                 sliceSize);
    
    /* new h264 slice parameter buffer */ 
    mParam = new VaapiBufObject(display,
                 ctx,
                 VASliceParameterBufferType,
                 NULL, 
                 sizeof(VASliceParameterBufferH264));

    paramBuf = (VASliceParameterBufferH264*)mParam->map();

    paramBuf->slice_data_size   = sliceSize;
    paramBuf->slice_data_offset = 0;
    paramBuf->slice_data_flag   = VA_SLICE_DATA_FLAG_ALL;
    mParam->unmap();

    memset((void*)&slice_hdr, 0 , sizeof(slice_hdr));

}

VaapiSliceH264::~VaapiSliceH264()
{
    if (mData) {
        delete mData;
        mData = NULL;
    }
    
    if (mParam) {
        delete mParam;
        mParam = NULL;
    }
}

VaapiPictureH264::VaapiPictureH264(VADisplay display,
                     VAContextID context,
                     VaapiSurfaceBufferPool *surfBufPool,
                     VaapiPictureStructure structure)
:VaapiPicture(display, context, surfBufPool, structure)
{
   pps             = NULL;
   structure       = VAAPI_PICTURE_STRUCTURE_FRAME;
   field_poc[0]    = 0;
   field_poc[1]    = 0;
   frame_num       = 0;
   frame_num_wrap  = 0;
   long_term_frame_idx = 0;
   pic_num             = 0;
   long_term_pic_num   = 0;
   output_flag         = 0;
   output_needed       = 0;
   other_field         = NULL;

   /* new h264 slice parameter buffer */ 
   mPicParam = new VaapiBufObject(display,
                 context,
                 VAPictureParameterBufferType,
                 NULL, 
                 sizeof(VAPictureParameterBufferH264));
   if (!mPicParam) 
      ERROR("create h264 picture parameter buffer object failed");

   mIqMatrix = new VaapiBufObject(display,
                   context,
                   VAIQMatrixBufferType,
                   NULL,
                   sizeof(VAIQMatrixBufferH264));
   if (!mIqMatrix) 
      ERROR("create h264 iq matrix buffer object failed");

}

VaapiPictureH264*
VaapiPictureH264::new_field()
{
/*  
   return new VaapiPictureH264(mDisplay,
                               mContext,
                               mSurface,
                               mStructure);
*/
   ERROR("Not implemented yet");
   return NULL;
}

VaapiFrameStore::VaapiFrameStore(VaapiPictureH264 *pic)
{
   structure  =  pic->structure;
   buffers[0] =  pic;
   buffers[1] =  NULL;
   num_buffers = 1;
   output_needed = pic->output_needed;
}

VaapiFrameStore::~VaapiFrameStore()
{
   if (buffers[0]) {
      delete buffers[0];
      buffers[0] = NULL;
   }
   if (buffers[1]) {
      delete buffers[1];
      buffers[1] = NULL;
   }
}

void
VaapiFrameStore::add_picture(VaapiPictureH264 *pic)
{
    uint8_t field;
    assert(num_buffers == 1);
    assert(pic->structure != VAAPI_PICTURE_STRUCTURE_FRAME);
    buffers[1] = pic;

    if(pic->output_flag) {
       pic->output_needed = true;
       output_needed++;
    }

    structure = VAAPI_PICTURE_STRUCTURE_FRAME;

    field = pic->structure == VAAPI_PICTURE_STRUCTURE_TOP_FIELD ? 0 : 1; 

    assert(buffers[0]->field_poc[field] != MAXINT32);
    buffers[0] ->field_poc[field] = pic->field_poc[field]; 
    
    assert(pic->field_poc[!field] != MAXINT32);
    pic->field_poc[!field] = buffers[0]->field_poc[!field];
    num_buffers = num_buffers + 1;
}

bool
VaapiFrameStore::split_fields()
{
    VaapiPictureH264 * const first_field = buffers[0];
    VaapiPictureH264 * second_field;

    assert(num_buffers == 1);

    first_field->mStructure = VAAPI_PICTURE_STRUCTURE_TOP_FIELD;
    first_field->mFlags |= VAAPI_PICTURE_FLAG_INTERLACED;
   
    second_field = first_field->new_field();
    second_field->frame_num    = first_field->frame_num;
    second_field->field_poc[0] = first_field->field_poc[0];
    second_field->field_poc[1] = first_field->field_poc[1];
    second_field->output_flag  = first_field->output_flag;
    if (second_field->output_flag) {
        second_field->output_needed = true;
        output_needed++;
    }
   
    num_buffers ++;
    buffers[num_buffers] = second_field;
    
    return true;
}

bool
VaapiFrameStore::has_frame()
{
    return structure == VAAPI_PICTURE_STRUCTURE_FRAME;
}

bool
VaapiFrameStore::has_reference()
{
    uint32_t i;

    for (i = 0; i < num_buffers; i++) {
        if (!buffers[i]) continue;
        if (VAAPI_PICTURE_IS_REFERENCE(buffers[i]))
            return true;
    }
    return false;
}

VaapiDPBManager::VaapiDPBManager(
    VaapiDecoderH264 *dec,
    uint32_t dpb_size)
{
    uint32_t i;
    decoder   = dec; 
    dpb_layer = (VaapiDecPicBufLayer*)malloc(sizeof(VaapiDecPicBufLayer)) ;
    memset((void*)dpb_layer, 0, sizeof(VaapiDecPicBufLayer));
    dpb_layer->dpb_size = dpb_size;
}

bool
VaapiDPBManager::dpb_output(
    VaapiFrameStore *fs, 
    VaapiPictureH264 *picture)
{
    picture->output_needed = false;

    if (fs) {
        if (--fs->output_needed > 0)
            return true;
        picture = fs->buffers[0];
        INFO("picture from store");
    }

    INFO("dpb output: poc %d", picture->mPoc);
    return picture->output();
}

void 
VaapiDPBManager::dpb_evict(
    VaapiPictureH264 *pic, 
    uint32_t idx)
{
     VaapiFrameStore * const fs = dpb_layer->dpb[idx];
     if (!fs->output_needed && !fs->has_reference()) 
         dpb_remove_index(idx);
}

bool VaapiDPBManager::dpb_bump()
{
    VaapiPictureH264 *found_picture = NULL;
    uint32_t i, j, found_index;
    bool success;

    for (i = 0; i < dpb_layer->dpb_count; i++) {
        VaapiFrameStore * const fs = dpb_layer->dpb[i];
        if (!fs->output_needed)
            continue;

        for (j = 0; j < fs->num_buffers; j++) {
            VaapiPictureH264 * const picture = fs->buffers[j];
            if (!picture->output_needed)
                continue;
            if (!found_picture || found_picture->mPoc > picture->mPoc)
                found_picture = picture, found_index = i;
        }
    }
    if (!found_picture)
        return false;

    success = dpb_output(dpb_layer->dpb[found_index], found_picture);
    dpb_evict(found_picture, found_index);
    return success;
}

void 
VaapiDPBManager::dpb_clear()
{
    uint32_t i; 
    if(dpb_layer) {
      for (i = 0; i < dpb_layer->dpb_count; i++) {
         delete dpb_layer->dpb[i];
         dpb_layer->dpb[i] = NULL;
      }
      dpb_layer->dpb_count = 0;
    }
}

void 
VaapiDPBManager::dpb_flush()
{
    while (dpb_bump())
        ;
    dpb_clear();
}

bool
VaapiDPBManager::dpb_add(VaapiFrameStore *new_fs, VaapiPictureH264 *pic)
{
    uint32_t i, j;
    VaapiFrameStore *fs;

    // Remove all unused pictures
    if (!VAAPI_H264_PICTURE_IS_IDR(pic)) {
        i = 0;
        while (i < dpb_layer->dpb_count) {
            fs = dpb_layer->dpb[i];
            if (!fs->output_needed && !fs->has_reference()){
                dpb_remove_index(i);
            }
            else
                i++;
        }
    }

    // C.4.5.1 - Storage and marking of a reference decoded picture into the DPB
    if (VAAPI_PICTURE_IS_REFERENCE(pic)) {
        while (dpb_layer->dpb_count == dpb_layer->dpb_size) {
            if (!dpb_bump())
                return false;
        }

        dpb_layer->dpb[dpb_layer->dpb_count++] = new_fs;
        if (pic->output_flag) {
            pic->output_needed = true;
            new_fs->output_needed++;
        }
    }
    // C.4.5.2 - Storage and marking of a non-reference decoded picture into the DPB
    else {
        if (!pic->output_flag)
            return true;
        while (dpb_layer->dpb_count == dpb_layer->dpb_size) {
            bool found_picture = false;
            for (i = 0; !found_picture && i < dpb_layer->dpb_count; i++) {
                fs = dpb_layer->dpb[i];
                if (!fs->output_needed)
                    continue;
                for (j = 0; !found_picture && j < fs->num_buffers; j++)
                    found_picture = fs->buffers[j]->output_needed &&
                        fs->buffers[j]->mPoc < pic->mPoc;
            }
            if (!found_picture)
                return dpb_output(NULL, pic);
            if (!dpb_bump())
                return false;
        }

        dpb_layer->dpb[dpb_layer->dpb_count++] = new_fs;
        pic->output_needed = true;
        new_fs->output_needed++;
    }

    return true;
}

void
VaapiDPBManager::dpb_reset(H264SPS *sps)
{
    uint32_t i;   
    for( i = 0; i < 16; i ++) {
       if (dpb_layer->dpb[i]) {
           delete dpb_layer->dpb[i];
           dpb_layer->dpb[i] = NULL;
        }
    }
    memset((void*)dpb_layer, 0, sizeof(VaapiDecPicBufLayer));
    dpb_layer->dpb_size = get_max_dec_frame_buffering(sps, 1);
}

/* initialize reference list */
void
VaapiDPBManager::init_picture_refs(
    VaapiPictureH264 *pic,
    H264SliceHdr     *slice_hdr
)
{
    uint32_t i, num_refs;

    init_picture_ref_lists(pic);

    init_picture_refs_pic_num(pic, slice_hdr);

    dpb_layer->RefPicList0_count = 0;
    dpb_layer->RefPicList1_count = 0;

    switch (pic->mType) {
    case VAAPI_PICTURE_TYPE_P:
    case VAAPI_PICTURE_TYPE_SP:
        init_picture_refs_p_slice(pic, slice_hdr);
        break;
    case VAAPI_PICTURE_TYPE_B:
        init_picture_refs_b_slice(pic, slice_hdr);
        break;
    default:
        break;
    }

    exec_picture_refs_modification(pic, slice_hdr);

    switch (pic->mType) {
    case VAAPI_PICTURE_TYPE_B:
        num_refs = 1 + slice_hdr->num_ref_idx_l1_active_minus1;
        for (i = dpb_layer->RefPicList1_count; i < num_refs; i++)
            dpb_layer->RefPicList1[i] = NULL;
            //dpb_layer->RefPicList1_count = num_refs;

        // fall-through
    case VAAPI_PICTURE_TYPE_P:
    case VAAPI_PICTURE_TYPE_SP:
        num_refs = 1 + slice_hdr->num_ref_idx_l0_active_minus1;
        for (i = dpb_layer->RefPicList0_count; i < num_refs; i++)
            dpb_layer->RefPicList0[i] = NULL;
            //dpb_layer->RefPicList0_count = num_refs;
        break;
    default:
        break;
    }
}

bool
VaapiDPBManager::exec_ref_pic_marking(VaapiPictureH264 *pic, bool *has_mmco5)
{
    *has_mmco5 = false;

    if (!VAAPI_PICTURE_IS_REFERENCE(pic)){
        return true;
    }

   if (!VAAPI_H264_PICTURE_IS_IDR(pic)) {
        VaapiSliceH264 * const slice = (VaapiSliceH264*)pic->getLastSlice();
        H264DecRefPicMarking * const dec_ref_pic_marking =
            &slice->slice_hdr.dec_ref_pic_marking;
        if (dec_ref_pic_marking->adaptive_ref_pic_marking_mode_flag) {
            if (!exec_ref_pic_marking_adaptive(pic, dec_ref_pic_marking, has_mmco5))
                return false;
        }
        else {
            if (!exec_ref_pic_marking_sliding_window(pic))
                return false;
        }

    }

    return true;
}

/* private functions */

void
VaapiDPBManager::init_picture_ref_lists(VaapiPictureH264 *pic)
{
    uint32_t i, j, short_ref_count, long_ref_count;
    VaapiFrameStore  *fs;
    VaapiPictureH264 *picture;
    
    short_ref_count = 0;
    long_ref_count  = 0;
    if (pic->structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        for (i = 0; i < dpb_layer->dpb_count; i++) {
           fs = dpb_layer->dpb[i];
           if (!fs->has_frame())
                continue;
            picture = fs->buffers[0];
            if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                dpb_layer->short_ref[short_ref_count++] = picture;
            else if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture))
                dpb_layer->long_ref[long_ref_count++] = picture;
            picture->structure = VAAPI_PICTURE_STRUCTURE_FRAME;
            picture->other_field = fs->buffers[1];
        }
    }
    else {
        for (i = 0; i < dpb_layer->dpb_count; i++) {
            fs = dpb_layer->dpb[i];
            for (j = 0; j < fs->num_buffers; j++) {
                picture = fs->buffers[j];
                if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                    dpb_layer->short_ref[short_ref_count++] = picture;
                else if (VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(picture))
                    dpb_layer->long_ref[long_ref_count++] = picture;
                picture->structure = picture->mStructure;
                picture->other_field = fs->buffers[j ^ 1];
            }
        }
    }

    for (i = short_ref_count; i < dpb_layer->short_ref_count; i++)
        dpb_layer->short_ref[i] = NULL;
    dpb_layer->short_ref_count = short_ref_count;

    for (i = long_ref_count; i < dpb_layer->long_ref_count; i++)
        dpb_layer->long_ref[i] = NULL;
    dpb_layer->long_ref_count = long_ref_count;

}

void
VaapiDPBManager::init_picture_refs_p_slice(VaapiPictureH264 *pic, 
                         H264SliceHdr *slice_hdr)
{
    VaapiPictureH264 **ref_list;
    uint32_t i;

    if (pic->structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        /* 8.2.4.2.1 - P and SP slices in frames */
        if (dpb_layer->short_ref_count > 0) {
            ref_list = dpb_layer->RefPicList0;
            for (i = 0; i < dpb_layer->short_ref_count; i++)
                ref_list[i] = dpb_layer->short_ref[i];
            SORT_REF_LIST(ref_list, i, pic_num_dec);
            dpb_layer->RefPicList0_count += i;
        }

        if (dpb_layer->long_ref_count > 0) {
            ref_list = &dpb_layer->RefPicList0[dpb_layer->RefPicList0_count];
            for (i = 0; i < dpb_layer->long_ref_count; i++)
                ref_list[i] = dpb_layer->long_ref[i];
            SORT_REF_LIST(ref_list, i, long_term_pic_num_inc);
            dpb_layer->RefPicList0_count += i;
        } 
   } 
   else {
        /* 8.2.4.2.2 - P and SP slices in fields */
        VaapiPictureH264 *short_ref[32];
        uint32_t short_ref_count = 0;
        VaapiPictureH264 *long_ref[32];
        uint32_t long_ref_count = 0;

        if (dpb_layer->short_ref_count > 0) {
            for (i = 0; i < dpb_layer->short_ref_count; i++)
                short_ref[i] = dpb_layer->short_ref[i];
            SORT_REF_LIST(short_ref, i, frame_num_wrap_dec);
            short_ref_count = i;
        }

        if (dpb_layer->long_ref_count > 0) {
            for (i = 0; i < dpb_layer->long_ref_count; i++)
                long_ref[i] = dpb_layer->long_ref[i];
            SORT_REF_LIST(long_ref, i, long_term_frame_idx_inc);
            long_ref_count = i;
        }

        init_picture_refs_fields(
            pic,
            dpb_layer->RefPicList0, &dpb_layer->RefPicList0_count,
            short_ref,          short_ref_count,
            long_ref,           long_ref_count
        );
    }

}

void
VaapiDPBManager::init_picture_refs_b_slice(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    VaapiPictureH264 **ref_list;
    uint32_t i, n;

    DEBUG("decode reference picture list for B slices");
    if (picture->structure == VAAPI_PICTURE_STRUCTURE_FRAME) {
        /* 8.2.4.2.3 - B slices in frames */

        /* RefPicList0 */
        if (dpb_layer->short_ref_count > 0) {
            // 1. Short-term references
            ref_list = dpb_layer->RefPicList0;
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc < picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_dec);
            dpb_layer->RefPicList0_count += n;

            ref_list = &dpb_layer->RefPicList0[dpb_layer->RefPicList0_count];
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc >= picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_inc);
            dpb_layer->RefPicList0_count += n;
       }

        if (dpb_layer->long_ref_count > 0) {
            // 2. Long-term references
            ref_list = &dpb_layer->RefPicList0[dpb_layer->RefPicList0_count];
            for (n = 0, i = 0; i < dpb_layer->long_ref_count; i++)
                ref_list[n++] = dpb_layer->long_ref[i];
            SORT_REF_LIST(ref_list, n, long_term_pic_num_inc);
            dpb_layer->RefPicList0_count += n;
        }

        /* RefPicList1 */
        if (dpb_layer->short_ref_count > 0) {
            // 1. Short-term references
            ref_list = dpb_layer->RefPicList1;
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc > picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_inc);
            dpb_layer->RefPicList1_count += n;

            ref_list = &dpb_layer->RefPicList1[dpb_layer->RefPicList1_count];
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc <= picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_dec);
            dpb_layer->RefPicList1_count += n;
       }

        if (dpb_layer->long_ref_count > 0) {
            // 2. Long-term references
            ref_list = &dpb_layer->RefPicList1[dpb_layer->RefPicList1_count];
            for (n = 0, i = 0; i < dpb_layer->long_ref_count; i++)
                ref_list[n++] = dpb_layer->long_ref[i];
            SORT_REF_LIST(ref_list, n, long_term_pic_num_inc);
            dpb_layer->RefPicList1_count += n;
        }
    }
    else {
        /* 8.2.4.2.4 - B slices in fields */
        VaapiPictureH264 *short_ref0[32];
        uint32_t short_ref0_count = 0;
        VaapiPictureH264 *short_ref1[32];
        uint32_t short_ref1_count = 0;
        VaapiPictureH264 *long_ref[32];
        uint32_t long_ref_count = 0;

        /* refFrameList0ShortTerm */
        if (dpb_layer->short_ref_count > 0) {
            ref_list = short_ref0;
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc <= picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_dec);
            short_ref0_count += n;

            ref_list = &short_ref0[short_ref0_count];
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc > picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_inc);
            short_ref0_count += n;
        }

        /* refFrameList1ShortTerm */
        if (dpb_layer->short_ref_count > 0) {
            ref_list = short_ref1;
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc > picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_inc);
            short_ref1_count += n;

            ref_list = &short_ref1[short_ref1_count];
            for (n = 0, i = 0; i < dpb_layer->short_ref_count; i++) {
                if (dpb_layer->short_ref[i]->mPoc <= picture->mPoc)
                    ref_list[n++] = dpb_layer->short_ref[i];
            }
            SORT_REF_LIST(ref_list, n, poc_dec);
            short_ref1_count += n;
        }

        /* refFrameListLongTerm */
        if (dpb_layer->long_ref_count > 0) {
            for (i = 0; i < dpb_layer->long_ref_count; i++)
                long_ref[i] = dpb_layer->long_ref[i];
            SORT_REF_LIST(long_ref, i, long_term_frame_idx_inc);
            long_ref_count = i;
        }

        init_picture_refs_fields(
            picture,
            dpb_layer->RefPicList0, &dpb_layer->RefPicList0_count,
            short_ref0,         short_ref0_count,
            long_ref,           long_ref_count
        );

        init_picture_refs_fields(
            picture,
            dpb_layer->RefPicList1, &dpb_layer->RefPicList1_count,
            short_ref1,         short_ref1_count,
            long_ref,           long_ref_count
        );
   }

   /* Check whether RefPicList1 is identical to RefPicList0, then
       swap if necessary */
    if (dpb_layer->RefPicList1_count > 1 &&
        dpb_layer->RefPicList1_count == dpb_layer->RefPicList0_count &&
        memcmp(dpb_layer->RefPicList0, dpb_layer->RefPicList1,
               dpb_layer->RefPicList0_count * sizeof(dpb_layer->RefPicList0[0])) == 0) {
        VaapiPictureH264 * const tmp = dpb_layer->RefPicList1[0];
        dpb_layer->RefPicList1[0] = dpb_layer->RefPicList1[1];
        dpb_layer->RefPicList1[1] = tmp;
    }
}

void
VaapiDPBManager::init_picture_refs_fields(
    VaapiPictureH264 *picture,
    VaapiPictureH264 *RefPicList[32],
    uint32_t         *RefPicList_count,
    VaapiPictureH264 *short_ref[32],
    uint32_t         short_ref_count,
    VaapiPictureH264 *long_ref[32],
    uint32_t          long_ref_count
)
{
    uint32_t n = 0;
    /* 8.2.4.2.5 - reference picture lists in fields */
    init_picture_refs_fields_1(picture->structure, RefPicList, &n,
        short_ref, short_ref_count);
    init_picture_refs_fields_1(picture->structure, RefPicList, &n,
        long_ref, long_ref_count);
    *RefPicList_count = n;
}

void
VaapiDPBManager::init_picture_refs_fields_1(
    uint32_t          picture_structure,
    VaapiPictureH264 *RefPicList[32],
    uint32_t         *RefPicList_count,
    VaapiPictureH264 *ref_list[32],
    uint32_t          ref_list_count)
{
    uint32_t i, j, n;

    i = 0;
    j = 0;
    n = *RefPicList_count;
    do {
        assert(n < 32);
        for (; i < ref_list_count; i++) {
            if (ref_list[i]->structure == picture_structure) {
                RefPicList[n++] = ref_list[i++];
                break;
            }
        }
        for (; j < ref_list_count; j++) {
            if (ref_list[j]->structure != picture_structure) {
                RefPicList[n++] = ref_list[j++];
                break;
            }
        }
    } while (i < ref_list_count || j < ref_list_count);
    *RefPicList_count = n;
}

void
VaapiDPBManager::init_picture_refs_pic_num(
    VaapiPictureH264 *picture,
    H264SliceHdr     *slice_hdr
)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    const int32_t MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
    uint32_t i;

    DEBUG("decode picture numbers");

    for (i = 0; i < dpb_layer->short_ref_count; i++) {
        VaapiPictureH264 * const pic = dpb_layer->short_ref[i];
        
        // (8-27)
        if (pic->frame_num > decoder->m_frame_num)
            pic->frame_num_wrap = pic->frame_num - MaxFrameNum;
        else
            pic->frame_num_wrap = pic->frame_num;

        // (8-28, 8-30, 8-31)
        if (VAAPI_PICTURE_IS_FRAME(picture))
            pic->pic_num = pic->frame_num_wrap;
        else {
            if (pic->structure == picture->structure)
                pic->pic_num = 2 * pic->frame_num_wrap + 1;
            else
                pic->pic_num = 2 * pic->frame_num_wrap;
        }
    }

    for (i = 0; i < dpb_layer->long_ref_count; i++) {
        VaapiPictureH264 * const pic = dpb_layer->long_ref[i];

        // (8-29, 8-32, 8-33)
        if (picture->structure == VAAPI_PICTURE_STRUCTURE_FRAME)
            pic->long_term_pic_num = pic->long_term_frame_idx;
        else {
            if (pic->structure == picture->structure)
                pic->long_term_pic_num = 2 * pic->long_term_frame_idx + 1;
            else
                pic->long_term_pic_num = 2 * pic->long_term_frame_idx;
        }
    }
}

void 
VaapiDPBManager::exec_picture_refs_modification(
    VaapiPictureH264 *picture,
    H264SliceHdr *slice_hdr)
{
    DEBUG("execute ref_pic_list_modification()");
    /* RefPicList0 */
    if (!H264_IS_I_SLICE(slice_hdr) && ! H264_IS_SI_SLICE(slice_hdr) &&
        slice_hdr->ref_pic_list_modification_flag_l0)
        exec_picture_refs_modification_1(picture, slice_hdr, 0);

    /* RefPicList1 */
    if (H264_IS_B_SLICE(slice_hdr) &&
        slice_hdr->ref_pic_list_modification_flag_l1)
        exec_picture_refs_modification_1(picture, slice_hdr, 1);
}
void
VaapiDPBManager::exec_picture_refs_modification_1(
    VaapiPictureH264           *picture,
    H264SliceHdr               *slice_hdr,
    uint32_t                    list)
{
    H264PPS * const pps = slice_hdr->pps;
    H264SPS * const sps = pps->sequence;
    H264RefPicListModification *ref_pic_list_modification;
    uint32_t num_ref_pic_list_modifications;
    VaapiPictureH264 **ref_list;
    uint32_t *ref_list_count_ptr, ref_list_count, ref_list_idx = 0;
    uint32_t i, j, n, num_refs;
    int32_t found_ref_idx;
    int32_t MaxPicNum, CurrPicNum, picNumPred;

    DEBUG("modification process of reference picture list %u", list);

    if (list == 0) {
        ref_pic_list_modification      = slice_hdr->ref_pic_list_modification_l0;
        num_ref_pic_list_modifications = slice_hdr->n_ref_pic_list_modification_l0;
        ref_list                       = dpb_layer->RefPicList0;
        ref_list_count_ptr             = &dpb_layer->RefPicList0_count;
        num_refs                       = slice_hdr->num_ref_idx_l0_active_minus1 + 1;
    }
    else {
        ref_pic_list_modification      = slice_hdr->ref_pic_list_modification_l1;
        num_ref_pic_list_modifications = slice_hdr->n_ref_pic_list_modification_l1;
        ref_list                       = dpb_layer->RefPicList1;
        ref_list_count_ptr             = &dpb_layer->RefPicList1_count;
        num_refs                       = slice_hdr->num_ref_idx_l1_active_minus1 + 1;
    }
    ref_list_count = *ref_list_count_ptr;

    if (picture->structure != VAAPI_PICTURE_STRUCTURE_FRAME) {
        MaxPicNum  = 1 << (sps->log2_max_frame_num_minus4 + 5); // 2 * MaxFrameNum
        CurrPicNum = 2 * slice_hdr->frame_num + 1;              // 2 * frame_num + 1
    }
    else {
        MaxPicNum  = 1 << (sps->log2_max_frame_num_minus4 + 4); // MaxFrameNum
        CurrPicNum = slice_hdr->frame_num;                      // frame_num
    }


    picNumPred = CurrPicNum;

    for (i = 0; i < num_ref_pic_list_modifications; i++) {
        H264RefPicListModification * const l = &ref_pic_list_modification[i];
        if (l->modification_of_pic_nums_idc == 3)
            break;

        /* 8.2.4.3.1 - Short-term reference pictures */
        if (l->modification_of_pic_nums_idc == 0 || l->modification_of_pic_nums_idc == 1) {
            int32_t abs_diff_pic_num = l->value.abs_diff_pic_num_minus1 + 1;
            int32_t picNum, picNumNoWrap;

            // (8-34)
            if (l->modification_of_pic_nums_idc == 0) {
                picNumNoWrap = picNumPred - abs_diff_pic_num;
                if (picNumNoWrap < 0)
                    picNumNoWrap += MaxPicNum;
            }

            // (8-35)
            else {
                picNumNoWrap = picNumPred + abs_diff_pic_num;
                if (picNumNoWrap >= MaxPicNum)
                    picNumNoWrap -= MaxPicNum;
            }
            picNumPred = picNumNoWrap;

            // (8-36)
            picNum = picNumNoWrap;
            if (picNum > CurrPicNum)
                picNum -= MaxPicNum;

            // (8-37)
            for (j = num_refs; j > ref_list_idx; j--)
                ref_list[j] = ref_list[j - 1];
            found_ref_idx = find_short_term_reference(picNum);
            ref_list[ref_list_idx++] =
                found_ref_idx >= 0 ? dpb_layer->short_ref[found_ref_idx] : NULL;
            n = ref_list_idx;
            for (j = ref_list_idx; j <= num_refs; j++) {
                int32_t PicNumF;
                if (!ref_list[j])
                    continue;
                PicNumF =
                    VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(ref_list[j]) ?
                    ref_list[j]->pic_num : MaxPicNum;
                if (PicNumF != picNum)
                   ref_list[n++] = ref_list[j];
            }
        }

        /* 8.2.4.3.2 - Long-term reference pictures */
        else if (l->modification_of_pic_nums_idc == 2 ) {

            for (j = num_refs; j > ref_list_idx; j--)
                ref_list[j] = ref_list[j - 1];
            found_ref_idx =
                find_long_term_reference(l->value.long_term_pic_num);
            ref_list[ref_list_idx++] =
                found_ref_idx >= 0 ? dpb_layer->long_ref[found_ref_idx] : NULL;
            n = ref_list_idx;
            for (j = ref_list_idx; j <= num_refs; j++) {
                uint32_t LongTermPicNumF;
                if (!ref_list[j])
                    continue;
                LongTermPicNumF =
                    VAAPI_H264_PICTURE_IS_LONG_TERM_REFERENCE(ref_list[j]) ?
                    ref_list[j]->long_term_pic_num : INT_MAX;
                if (LongTermPicNumF != l->value.long_term_pic_num)
                    ref_list[n++] = ref_list[j];
            }
        }

    }

#if DEBUG
    for (i = 0; i < num_refs; i++)
        if (!ref_list[i])
            ERROR("list %u entry %u is empty", list, i);
#endif

    for(i = num_refs; i > 0 && !ref_list[i - 1]; i--);
    *ref_list_count_ptr = i ;
}


bool
VaapiDPBManager::exec_ref_pic_marking_adaptive(
    VaapiPictureH264     *picture,
    H264DecRefPicMarking *dec_ref_pic_marking,
    bool *has_mmco5)
{
    uint32_t i;
    for (i = 0; i < dec_ref_pic_marking->n_ref_pic_marking; i++) {
        H264RefPicMarking * const ref_pic_marking =
            &dec_ref_pic_marking->ref_pic_marking[i];

        uint32_t mmco = ref_pic_marking->memory_management_control_operation;
        if (mmco == 5)
            *has_mmco5 = true;

        exec_ref_pic_marking_adaptive_1(picture, ref_pic_marking, mmco);
    }
    return true;
}

bool
VaapiDPBManager::exec_ref_pic_marking_adaptive_1(
    VaapiPictureH264 *picture,
    H264RefPicMarking *ref_pic_marking,
    uint32_t mmco)
{
    uint32_t picNumX, long_term_frame_idx;
    VaapiPictureH264 *ref_picture;
    int32_t found_idx = 0;
    uint32_t i ;

    switch (mmco) {
      case 1:
        {
            picNumX = get_picNumX(picture, ref_pic_marking);
            found_idx = find_short_term_reference(picNumX);
            if (found_idx < 0)
               return false;

            i = (uint32_t) found_idx;
            h264_picture_set_reference(dpb_layer->short_ref[i], 0,
                VAAPI_PICTURE_IS_FRAME(picture));
            ARRAY_REMOVE_INDEX(dpb_layer->short_ref, i);
        }
        break;
      case 2:
        {
            found_idx = find_long_term_reference(ref_pic_marking->long_term_pic_num);
            if (found_idx < 0)
                return false;

            i = (uint32_t) found_idx;
            h264_picture_set_reference(dpb_layer->long_ref[i], 0,
                VAAPI_PICTURE_IS_FRAME(picture));
            ARRAY_REMOVE_INDEX(dpb_layer->long_ref, i);
        } 
        break;
      case 3:
        {
          for (i = 0; i < dpb_layer->long_ref_count; i++) {
             if ((int32_t)dpb_layer->long_ref[i]->long_term_frame_idx == 
                 ref_pic_marking->long_term_frame_idx)
                 break;
           }

           if (i != dpb_layer->long_ref_count) {
               h264_picture_set_reference(dpb_layer->long_ref[i], 0, true);
               ARRAY_REMOVE_INDEX(dpb_layer->long_ref, i);
           }

           picNumX = get_picNumX(picture, ref_pic_marking);
           found_idx = find_short_term_reference(picNumX);
           if (found_idx < 0)
               return false;

           i = (uint32_t) found_idx;
           ref_picture = dpb_layer->short_ref[i];
           ARRAY_REMOVE_INDEX(dpb_layer->short_ref, i);
           dpb_layer->long_ref[dpb_layer->long_ref_count++] = ref_picture;

           ref_picture->long_term_frame_idx = ref_pic_marking->long_term_frame_idx;
           h264_picture_set_reference(ref_picture,
                VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE,
                VAAPI_PICTURE_IS_FRAME(picture));
        } 
        break;
      case 4:
        {
             long_term_frame_idx = ref_pic_marking->max_long_term_frame_idx_plus1 - 1;

             for (i = 0; i < dpb_layer->long_ref_count; i++) {
                 if (dpb_layer->long_ref[i]->long_term_frame_idx <= long_term_frame_idx)
                     continue;
                  h264_picture_set_reference(dpb_layer->long_ref[i], 0, false);
                  ARRAY_REMOVE_INDEX(dpb_layer->long_ref, i);
                  i--;
              }
        } 
        break;
      case 5:
        {
            dpb_flush();
            /* The picture shall be inferred to have had frame_num equal to 0 (7.4.3) */
            picture->frame_num = 0;

            /* Update TopFieldOrderCnt and BottomFieldOrderCnt (8.2.1) */
            if (picture->structure != VAAPI_PICTURE_STRUCTURE_BOTTOM_FIELD)
                picture->field_poc[TOP_FIELD] -= picture->mPoc;
            if (picture->structure != VAAPI_PICTURE_STRUCTURE_TOP_FIELD)
                picture->field_poc[BOTTOM_FIELD] -= picture->mPoc;

                picture->mPoc = 0;

            if (VAAPI_H264_PICTURE_IS_SHORT_TERM_REFERENCE(picture))
                 remove_short_reference(picture);
        } 
        break;
      case 6:
        {
            picture->long_term_frame_idx = ref_pic_marking->long_term_frame_idx;
            h264_picture_set_reference(picture,
                VAAPI_PICTURE_FLAG_LONG_TERM_REFERENCE, false);
        } 
        break;
      default: 
        ERROR("unsupported mmco type %d", mmco);
        break;
    }
    return true;
}

bool
VaapiDPBManager::exec_ref_pic_marking_sliding_window(VaapiPictureH264 *picture)
{
    H264PPS * const pps = picture->pps;
    H264SPS * const sps = pps->sequence;
    VaapiPictureH264 *ref_picture;
    uint32_t i, m, max_num_ref_frames;

    if (!VAAPI_PICTURE_IS_FIRST_FIELD(picture))
        return true;

    max_num_ref_frames = sps->num_ref_frames;

    if (max_num_ref_frames == 0)
        max_num_ref_frames = 1;
    if (!VAAPI_PICTURE_IS_FRAME(picture))
        max_num_ref_frames <<= 1;

    if (dpb_layer->short_ref_count + dpb_layer->long_ref_count < max_num_ref_frames)
        return true;
    if (dpb_layer->short_ref_count < 1)
        return false;

    for (m = 0, i = 1; i < dpb_layer->short_ref_count; i++) {
        VaapiPictureH264 * const pic = dpb_layer->short_ref[i];
        if (pic->frame_num_wrap < dpb_layer->short_ref[m]->frame_num_wrap)
            m = i;
    }

    ref_picture = dpb_layer->short_ref[m];
    DEBUG("sliding window ,remove short ref %p", ref_picture);
    h264_picture_set_reference(ref_picture, 0, true);   
    ARRAY_REMOVE_INDEX(dpb_layer->short_ref, m);

    /* Both fields need to be marked as "unused for reference", so
       remove the other field from the short_ref[] list as well */
    if (!VAAPI_PICTURE_IS_FRAME(picture) && ref_picture->other_field) {
        for (i = 0; i < dpb_layer->short_ref_count; i++) {
            if (dpb_layer->short_ref[i] == ref_picture->other_field) {
                ARRAY_REMOVE_INDEX(dpb_layer->short_ref, i);
                break;
            }
        }
    }
 
    return true;
}

int32_t 
VaapiDPBManager::find_short_term_reference(uint32_t pic_num)
{
    uint32_t i;

    for (i = 0; i < dpb_layer->short_ref_count; i++) {
        if (dpb_layer->short_ref[i]->pic_num == pic_num)
            return i;
    }
    ERROR("found no short-term reference picture with PicNum = %d",
              pic_num);
    return -1;
}

int32_t 
VaapiDPBManager::find_long_term_reference(uint32_t long_term_pic_num)
{
    uint32_t i;

    for (i = 0; i < dpb_layer->long_ref_count; i++) {
        if (dpb_layer->long_ref[i]->long_term_pic_num == long_term_pic_num)
            return i;
    }
    ERROR("found no long-term reference picture with LongTermPicNum = %d",
              long_term_pic_num);
    return -1;
}

void
VaapiDPBManager::remove_short_reference(VaapiPictureH264 *picture)
{
    VaapiPictureH264 *ref_picture;
    uint32_t i;
    uint32_t frame_num = picture->frame_num;

    for (i = 0; i < dpb_layer->short_ref_count; ++i) {
        if (dpb_layer->short_ref[i]->frame_num == frame_num) {
            ref_picture = dpb_layer->short_ref[i];
            if (ref_picture != picture->other_field){
                h264_picture_set_reference(ref_picture, 0, false);
                ARRAY_REMOVE_INDEX(dpb_layer->short_ref, i);
            }
            return;
        }
    }
}

void
VaapiDPBManager::dpb_remove_index(uint32_t index)
{
    uint32_t i, num_frames = --dpb_layer->dpb_count;

    /* delete the frame store */
    delete dpb_layer->dpb[index];
    dpb_layer->dpb[index] = NULL;

    if (USE_STRICT_DPB_ORDERING) {
        for (i = index; i < num_frames; i++)
            dpb_layer->dpb[i] = dpb_layer->dpb[i + 1];
    } else if (index != num_frames)
        dpb_layer->dpb[index] = dpb_layer->dpb[num_frames];
    
    dpb_layer->dpb[num_frames] = NULL;

}
