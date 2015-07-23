/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    MPEG-4 header.
//
*/


#ifndef _H264_DPB_CTL_H_
#define _H264_DPB_CTL_H_


#include "h264.h"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////
///////////////////////////// Parser control functions
////////////////////////////////////////////////////////////////////

///// Reference list
    extern void h264_dpb_update_ref_lists(h264_Info * pInfo);
    extern void h264_dpb_reorder_lists(h264_Info * pInfo);

    extern void h264_dpb_insert_ref_lists(h264_DecodedPictureBuffer * p_dpb,int32_t NonExisting);

///// POC
    extern void h264_hdr_decoding_poc (h264_Info * pInfo,int32_t NonExisting, int32_t frame_num);
    extern void h264_hdr_post_poc(h264_Info* pInfo,int32_t NonExisting, int32_t frame_num, int32_t use_old);

///// DPB buffer mangement
    extern void h264_init_dpb(h264_DecodedPictureBuffer * p_dpb);

    extern void h264_dpb_unmark_for_reference(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc);
    extern void h264_dpb_unmark_for_long_term_reference(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc);
    extern void h264_dpb_unmark_long_term_frame_for_reference_by_frame_idx(h264_DecodedPictureBuffer *p_dpb, int32_t long_term_frame_idx);
    extern void h264_dpb_unmark_long_term_field_for_reference_by_frame_idx(h264_DecodedPictureBuffer *p_dpb, int32_t long_term_frame_idx, int32_t fs_idc, int32_t polarity);
    extern void h264_dpb_mark_pic_long_term(h264_Info * pInfo, int32_t long_term_frame_idx, int32_t picNumX);
    extern void h264_dpb_mark_dangling_field(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc);

    extern void h264_dpb_update_queue_dangling_field(h264_Info * pInfo);
    extern void h264_dpb_is_used_for_reference(h264_DecodedPictureBuffer * p_dpb, int32_t * flag);


    extern void h264_dpb_set_active_fs(h264_DecodedPictureBuffer * p_dpb,int32_t index);
    extern void h264_dpb_flush_dpb (h264_Info * pInfo,int32_t output_all, int32_t keep_complement, int32_t num_ref_frames);

    extern void h264_dpb_idr_memory_management (h264_Info * pInfo,
            seq_param_set_used_ptr active_sps,
            int32_t no_output_of_prior_pics_flag);

    extern void h264_dpb_init_frame_store(h264_Info * pInfo);
    extern void h264_dpb_reset_dpb(h264_Info * pInfo,int32_t PicWidthInMbs, int32_t FrameHeightInMbs,
                                   int32_t SizeChange, int32_t no_output_of_prior_pics_flag);

    extern void h264_dpb_gaps_in_frame_num_mem_management(h264_Info * pInfo);

    extern int32_t h264_dpb_assign_frame_store(h264_Info * pInfo, int32_t NonExisting);

    extern void h264_dpb_get_smallest_poc(h264_DecodedPictureBuffer *p_dpb, int32_t *poc, int32_t *pos);
    extern void h264_dpb_remove_unused_frame_from_dpb(h264_DecodedPictureBuffer *p_dpb, int32_t * flag);

    extern void h264_dpb_sliding_window_memory_management(h264_DecodedPictureBuffer *p_dpb,
            int32_t NonExisting,
            int32_t num_ref_frames);
    extern int32_t h264_dpb_queue_update(h264_Info * pInfo,
                                         int32_t push,
                                         int32_t direct,
                                         int32_t frame_request,
                                         int32_t num_ref_frames);

    extern void h264_dpb_split_field (h264_DecodedPictureBuffer *p_dpb, h264_Info * pInfo);
    extern void h264_dpb_combine_field(h264_DecodedPictureBuffer *p_dpb, int32_t use_old);

    extern void h264_dpb_insert_picture_in_dpb(h264_Info * pInfo,
            int32_t used_for_reference,
            int32_t add2dpb,
            int32_t NonExisting,
            int32_t use_old);

    extern void h264_dpb_store_previous_picture_in_dpb(h264_Info * pInfo,
            int32_t NonExisting,
            int32_t use_old);

    extern void h264_dpb_adaptive_memory_management (h264_Info * pInfo);

    extern int32_t h264_dpb_output_one_frame_from_dpb(h264_Info* pInfo,
            int32_t direct, int32_t request, int32_t num_ref_frames);

    extern void h264_dpb_remove_frame_from_dpb(h264_DecodedPictureBuffer *p_dpb, int32_t idx);
    extern void h264_dpb_frame_output(h264_Info * pInfo,int32_t fs_idc, int32_t direct, int32_t * existing);

//////////////////////////////////////////////////////////// Globals

#ifdef __cplusplus
}
#endif


#endif  //_H264_DPB_CTL_H_


