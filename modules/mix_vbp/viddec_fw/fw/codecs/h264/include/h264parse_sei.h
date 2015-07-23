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


#ifndef _H264_SEI_H_
#define _H264_SEI_H_

#include "h264.h"


//defines for SEI
#define MAX_CPB_CNT					32
#define MAX_NUM_CLOCK_TS			3
#define MAX_PAN_SCAN_CNT			3
#define MAX_NUM_SPARE_PICS			16
#define MAX_SUB_SEQ_LAYERS   		256
#define MAX_SLICE_GRPS				1    // for high profile
#define NUM_SPS						32
#define MAX_NUM_REF_SUBSEQS			256


#define SEI_SCAN_FORMAT_INTERLACED      0x1
#define SEI_SCAN_FORMAT_PROGRESSIVE     0x3
#define SEI_SCAN_FORMAT_VALID(r)        (r&0x1)
#define SEI_SCAN_FORMAT(r)              ((r&0x2)>>1)

typedef enum
{
    SEI_BUF_PERIOD = 0,
    SEI_PIC_TIMING,
    SEI_PAN_SCAN,
    SEI_FILLER_PAYLOAD,
    SEI_REG_USERDATA,
    SEI_UNREG_USERDATA,
    SEI_RECOVERY_POINT,
    SEI_DEC_REF_PIC_MARKING_REP,
    SEI_SPARE_PIC,
    SEI_SCENE_INFO,
    SEI_SUB_SEQ_INFO,
    SEI_SUB_SEQ_LAYER,
    SEI_SUB_SEQ,
    SEI_FULL_FRAME_FREEZE,
    SEI_FULL_FRAME_FREEZE_RELEASE,
    SEI_FULL_FRAME_SNAPSHOT,
    SEI_PROGRESSIVE_SEGMENT_START,
    SEI_PROGRESSIVE_SEGMENT_END,
    SEI_MOTION_CONSTRAINED_SLICE_GRP_SET,
    SEI_FILM_GRAIN_CHARACTERISTICS,
    SEI_DEBLK_FILTER_DISPLAY_PREFERENCE,
    SEI_STEREO_VIDEO_INFO,
    SEI_RESERVED,
} h264_sei_payloadtype;



typedef struct _h264_SEI_buffering_period
{
    int32_t seq_param_set_id;
    int32_t initial_cpb_removal_delay_nal;
    int32_t initial_cpb_removal_delay_offset_nal;
    int32_t initial_cpb_removal_delay_vcl;
    int32_t initial_cpb_removal_delay_offset_vcl;

} h264_SEI_buffering_period_t;

typedef struct _h264_SEI_pic_timing
{
    int32_t cpb_removal_delay;
    int32_t dpb_output_delay;
    int32_t pic_struct;
} h264_SEI_pic_timing_t;

#if 0
int32_t clock_timestamp_flag[MAX_NUM_CLOCK_TS];
int32_t ct_type[MAX_NUM_CLOCK_TS];
int32_t nuit_field_based_flag[MAX_NUM_CLOCK_TS];
int32_t counting_type[MAX_NUM_CLOCK_TS];
int32_t full_timestamp_flag[MAX_NUM_CLOCK_TS];
int32_t discontinuity_flag[MAX_NUM_CLOCK_TS];
int32_t cnt_dropped_flag[MAX_NUM_CLOCK_TS];
int32_t n_frames[MAX_NUM_CLOCK_TS];
int32_t seconds_value[MAX_NUM_CLOCK_TS];
int32_t minutes_value[MAX_NUM_CLOCK_TS];
int32_t hours_value[MAX_NUM_CLOCK_TS];
int32_t seconds_flag[MAX_NUM_CLOCK_TS];
int32_t minutes_flag[MAX_NUM_CLOCK_TS];
int32_t hours_flag[MAX_NUM_CLOCK_TS];
int32_t time_offset[MAX_NUM_CLOCK_TS];

#endif

typedef struct _h264_SEI_pan_scan_rectangle
{
    int32_t pan_scan_rect_id;
    int32_t pan_scan_rect_cancel_flag;
    int32_t pan_scan_cnt_minus1;
    int32_t pan_scan_rect_left_offset[MAX_PAN_SCAN_CNT];
    int32_t pan_scan_rect_right_offset[MAX_PAN_SCAN_CNT];
    int32_t pan_scan_rect_top_offset[MAX_PAN_SCAN_CNT];
    int32_t pan_scan_rect_bottom_offset[MAX_PAN_SCAN_CNT];
    int32_t pan_scan_rect_repetition_period;
} h264_SEI_pan_scan_rectangle_t;

typedef struct _h264_SEI_filler_payload
{
    int32_t ff_byte;
} h264_SEI_filler_payload_t;

typedef struct _h264_SEI_userdata_registered
{
    int32_t itu_t_t35_country_code;
    int32_t itu_t_t35_country_code_extension_byte;
    int32_t itu_t_t35_payload_byte;
} h264_SEI_userdata_registered_t;

typedef struct _h264_SEI_userdata_unregistered
{
    int32_t uuid_iso_iec_11578[4];
    int32_t user_data_payload_byte;
} h264_SEI_userdata_unregistered_t;

typedef struct _h264_SEI_recovery_point
{
    int32_t recovery_frame_cnt;
    int32_t exact_match_flag;
    int32_t broken_link_flag;
    int32_t changing_slice_group_idc;
} h264_SEI_recovery_point_t;

typedef struct _h264_SEI_decoded_ref_pic_marking_repetition
{
    int32_t original_idr_flag;
    int32_t original_frame_num;
    int32_t orignal_field_pic_flag;
    int32_t original_bottom_field_pic_flag;
    int32_t no_output_of_prior_pics_flag;
    int32_t long_term_reference_flag;
    int32_t adaptive_ref_pic_marking_mode_flag;
    int32_t memory_management_control_operation;				//UE
    int32_t difference_of_pics_num_minus1;						//UE
    int32_t long_term_pic_num;									//UE
    int32_t long_term_frame_idx;								//UE
    int32_t max_long_term_frame_idx_plus1;						//UE
} h264_SEI_decoded_ref_pic_marking_repetition_t;

typedef struct _h264_SEI_spare_picture
{
    int32_t target_frame_num;
    int32_t spare_field_flag;
    int32_t target_bottom_field_flag;
    int32_t num_spare_pics_minus1;
    int32_t delta_spare_frame_num[MAX_NUM_SPARE_PICS];
    int32_t spare_bottom_field_flag[MAX_NUM_SPARE_PICS];
    int32_t spare_area_idc[MAX_NUM_SPARE_PICS];				// not complete
} h264_SEI_spare_picture_t;

typedef struct _h264_SEI_scene_info
{
    int32_t scene_info_present_flag;
    int32_t scene_id;
    int32_t scene_transitioning_type;
    int32_t second_scene_id;
} h264_SEI_scene_info_t;

typedef struct _h264_SEI_sub_sequence_info
{
    int32_t sub_seq_layer_num;
    int32_t sub_seq_id;
    int32_t first_ref_pic_flag;
    int32_t leading_non_ref_pic_flag;
    int32_t last_pic_flag;
    int32_t sub_seq_frame_num_flag;
    int32_t sub_seq_frame_num;
} h264_SEI_sub_sequence_info_t;

typedef struct _h264_SEI_sub_sequence_layer
{
    int32_t num_sub_seq_layers_minus1;
    int32_t accurate_statistics_flag[MAX_SUB_SEQ_LAYERS];
    int32_t average_bit_rate[MAX_SUB_SEQ_LAYERS];
    int32_t average_frame_rate[MAX_SUB_SEQ_LAYERS];
} h264_SEI_sub_sequence_layer_t;

typedef struct _h264_SEI_sub_sequence
{
    int32_t sub_seq_layer_num;
    int32_t sub_seq_id;
    int32_t duration_flag;
    int32_t sub_seq_duration;
    int32_t average_rate_flag;
    int32_t average_statistics_flag;
    int32_t average_bit_rate;
    int32_t average_frame_rate;
    int32_t num_referenced_subseqs;
    int32_t ref_sub_seq_layer_num;
    int32_t ref_sub_seq_id;
    int32_t ref_sub_seq_direction;
} h264_SEI_sub_sequence_t;

typedef struct _h264_SEI_full_frame_freeze
{
    int32_t full_frame_freeze_repetition_period;
} h264_SEI_full_frame_freeze_t;

typedef struct _h264_SEI_full_frame_snapshot
{
    int32_t snapshot_id;
} h264_SEI_full_frame_snapshot_t;

typedef struct _h264_SEI_progressive_segment_start
{
    int32_t progressive_refinement_id;
    int32_t num_refinement_steps_minus1;
} h264_SEI_progressive_segment_start_t;

typedef struct _h264_SEI_progressive_segment_end
{
    int32_t progressive_refinement_id;
} h264_SEI_progressive_segment_end_t;

typedef struct _h264_SEI_motion_constrained_slice_group
{
    int32_t num_slice_groups_in_set_minus1;
    int32_t slice_group_id[MAX_SLICE_GRPS];
    int32_t exact_sample_value_match_flag;
    int32_t pan_scan_rect_flag;
    int32_t pan_scan_rect_id;
} h264_SEI_motion_constrained_slice_group_t;

typedef struct _h264_SEI_deblocking_filter_display_pref
{
    int32_t devlocking_display_preference_cancel_flag;
    int32_t display_prior_to_deblocking_preferred_flag;
    int32_t dec_frame_buffering_constraint_flag;
    int32_t deblocking_display_preference_repetition_period;
} h264_SEI_deblocking_filter_display_pref_t;

typedef struct _h264_SEI_stereo_video_info
{
    int32_t field_views_flag;
    int32_t top_field_is_left_view_flag;
    int32_t curent_frame_is_left_view_flag;
    int32_t next_frame_is_second_view_flag;
    int32_t left_view_self_contained_flag;
    int32_t right_view_self_contained_flag;
} h264_SEI_stereo_video_info_t;

typedef struct _h264_SEI_reserved
{
    int32_t reserved_sei_message_payload_byte;
} h264_SEI_reserved_t;


////////////////////////////
// SEI Info
/////////////////////////////

typedef struct sei_info
{
    int32_t recovery_point;
    int32_t recovery_frame_num;

    int32_t capture_POC;
    int32_t freeze_POC;
    int32_t release_POC;        // The POC which when reached will allow display update to re-commence
    int32_t disp_frozen;        // Indicates display is currently frozen
    int32_t freeze_rep_period;
    int32_t recovery_frame_cnt;
    int32_t capture_fn;
    int32_t recovery_fn;
    int32_t broken_link;
    int32_t scan_format;
    int32_t broken_link_pic;
} sei_info, *sei_info_ptr;

/*typedef struct _h264_SEI
{
	h264_SEI_buffering_period_t buf_period;
	h264_SEI_pic_timing_t pic_timing;
	h264_SEI_pan_scan_rectangle_t pan_scan_timing;
	h264_SEI_filler_payload_t filler_payload;
	h264_SEI_userdata_registered_t userdata_reg;
	h264_SEI_userdata_unregistered_t userdata_unreg;
	h264_SEI_recovery_point_t recovery_point;
	h264_SEI_decoded_ref_pic_marking_repetition_t dec_ref_pic_marking_rep;
	h264_SEI_spare_picture_t spare_pic;
	h264_SEI_scene_info_t scene_info;
	h264_SEI_sub_sequence_info_t sub_sequence_info;
	h264_SEI_sub_sequence_layer_t sub_sequence_layer;
	h264_SEI_sub_sequence_t sub_sequence;
	h264_SEI_full_frame_snapshot_t full_frame_snapshot;
	h264_SEI_full_frame_t full_frame;
	h264_SEI_progressive_segment_start_t progressive_segment_start;
	h264_SEI_progressive_segment_end_t progressive_segment_end;
	h264_SEI_motion_constrained_slice_group_t motion_constrained_slice_grp;
	h264_SEI_deblocking_filter_display_pref_t deblk_filter_display_pref;
	h264_SEI_stereo_video_info_t stereo_video_info;
	h264_SEI_reserved_t reserved;
}h264_SEI_t;
*/


#endif  //_H264_SEI_H_


