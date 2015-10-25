#ifndef VIDDEC_FW_MP4_H
#define VIDDEC_FW_MP4_H

#include "viddec_fw_workload.h"

enum viddec_fw_mp4_ref_frame_id
{
    VIDDEC_MP4_FRAME_CURRENT = 0,
    VIDDEC_MP4_FRAME_PAST = 1,
    VIDDEC_MP4_FRAME_FUTURE = 2,
    VIDDEC_MP4_FRAME_MAX = 3,
};

enum mp4_workload_item_type
{
    VIDDEC_WORKLOAD_MP4_PAST_FRAME = VIDDEC_WORKLOAD_REF_FRAME_SOURCE_0,
    VIDDEC_WORKLOAD_MP4_FUTURE_FRAME,
    VIDDEC_WORKLOAD_MP4_VOL_INFO = VIDDEC_WORKLOAD_DECODER_SPECIFIC,
    VIDDEC_WORKLOAD_MP4_VOP_INFO,
    VIDDEC_WORKLOAD_MP4_BVOP_INFO,
    VIDDEC_WORKLOAD_MP4_SPRT_TRAJ,
    VIDDEC_WORKLOAD_MP4_IQUANT,
    VIDDEC_WORKLOAD_MP4_NIQUANT,
    VIDDEC_WORKLOAD_MP4_SVH,
};

enum viddec_fw_mp4_vop_coding_type_t
{
    VIDDEC_MP4_VOP_TYPE_I = 0,
    VIDDEC_MP4_VOP_TYPE_P,
    VIDDEC_MP4_VOP_TYPE_B,
    VIDDEC_MP4_VOP_TYPE_S
};

// This structure contains the information extracted from the Video Object Layer.
// This info will be populated in the workload as item type VIDDEC_WORKLOAD_MP4_VOL_INFO, using
// the "vwi_payload" array in viddec_workload_item_t.
// TODO: Add default values in the comments for each item
typedef struct
{
    // Flags extracted from the Video Object Layer
    // 0:0 - short_video_header
    // 1:2 - vol_shape
    // 3:3 - interlaced
    // 4:4 - obmc_disable
    // 5:5 - quarter_sample
    // 6:6 - resync_marker_disable
    // 7:7 - data_partitioned
    // 8:8 - reversible_vlc
#define viddec_fw_mp4_get_reversible_vlc(x)      viddec_fw_bitfields_extract((x)->vol_flags,  8, 0x1)
#define viddec_fw_mp4_set_reversible_vlc(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 8, 0x1)
#define viddec_fw_mp4_get_data_partitioned(x)      viddec_fw_bitfields_extract((x)->vol_flags,  7, 0x1)
#define viddec_fw_mp4_set_data_partitioned(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 7, 0x1)
#define viddec_fw_mp4_get_resync_marker_disable(x)      viddec_fw_bitfields_extract((x)->vol_flags,  6, 0x1)
#define viddec_fw_mp4_set_resync_marker_disable(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 6, 0x1)
#define viddec_fw_mp4_get_quarter_sample(x)      viddec_fw_bitfields_extract((x)->vol_flags,  5, 0x1)
#define viddec_fw_mp4_set_quarter_sample(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 5, 0x1)
#define viddec_fw_mp4_get_obmc_disable(x)      viddec_fw_bitfields_extract((x)->vol_flags,  4, 0x1)
#define viddec_fw_mp4_set_obmc_disable(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 4, 0x1)
#define viddec_fw_mp4_get_interlaced(x)      viddec_fw_bitfields_extract((x)->vol_flags,  3, 0x1)
#define viddec_fw_mp4_set_interlaced(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 3, 0x1)
#define viddec_fw_mp4_get_vol_shape(x)      viddec_fw_bitfields_extract((x)->vol_flags,  1, 0x3)
#define viddec_fw_mp4_set_vol_shape(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 1, 0x3)
#define viddec_fw_mp4_get_short_video_header_flag(x)      viddec_fw_bitfields_extract((x)->vol_flags,  0, 0x1)
#define viddec_fw_mp4_set_short_video_header_flag(x, val) viddec_fw_bitfields_insert((x)->vol_flags, val, 0, 0x1)
    unsigned int vol_flags;

    // Size extracted from the Video Object Layer
    // 0:12 - width
    // 13:25 - height
    // MFD_MPG4VD_MB_PER_ROW can be calculated as (width+15) >> 4
    // MFD_MPG4VD_MB_ROWS can be calculated as (height+15) >> 4
#define viddec_fw_mp4_get_vol_width(x)      viddec_fw_bitfields_extract((x)->vol_size,  13, 0x1FFF)
#define viddec_fw_mp4_set_vol_width(x, val) viddec_fw_bitfields_insert((x)->vol_size, val, 13, 0x1FFF)
#define viddec_fw_mp4_get_vol_height(x)      viddec_fw_bitfields_extract((x)->vol_size,  0, 0x1FFF)
#define viddec_fw_mp4_set_vol_height(x, val) viddec_fw_bitfields_insert((x)->vol_size, val, 0, 0x1FFF)
    unsigned int vol_size;

    // Sprite, time increments and quantization details from the Video Object Layer
    // 0:15 - vop_time_increment_resolution
    // 16:17 - sprite_enable
    // 18:23 - sprite_warping_points
    // 24:25 - sprite_warping_accuracy
    // 26:29 - quant_precision
    // 30:30 - quant_type
#define viddec_fw_mp4_get_quant_type(x)      viddec_fw_bitfields_extract((x)->vol_item,  30, 0x1)
#define viddec_fw_mp4_set_quant_type(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 30, 0x1)
#define viddec_fw_mp4_get_quant_precision(x)      viddec_fw_bitfields_extract((x)->vol_item,  26, 0xF)
#define viddec_fw_mp4_set_quant_precision(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 26, 0xF)
#define viddec_fw_mp4_get_sprite_warping_accuracy(x)      viddec_fw_bitfields_extract((x)->vol_item,  24, 0x3)
#define viddec_fw_mp4_set_sprite_warping_accuracy(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 24, 0x3)
#define viddec_fw_mp4_get_sprite_warping_points(x)      viddec_fw_bitfields_extract((x)->vol_item,  18, 0x3F)
#define viddec_fw_mp4_set_sprite_warping_points(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 18, 0x3F)
#define viddec_fw_mp4_get_sprite_enable(x)      viddec_fw_bitfields_extract((x)->vol_item,  16, 0x3)
#define viddec_fw_mp4_set_sprite_enable(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 16, 0x3)
#define viddec_fw_mp4_get_vop_time_increment_resolution(x)      viddec_fw_bitfields_extract((x)->vol_item,  0, 0xFFFF)
#define viddec_fw_mp4_set_vop_time_increment_resolution(x, val) viddec_fw_bitfields_insert((x)->vol_item, val, 0, 0xFFFF)
    unsigned int vol_item;

} viddec_fw_mp4_vol_info_t;

// This structure contains the information extracted from the Video Object Layer.
// This info will be populated in the workload as item type VIDDEC_WORKLOAD_MP4_VOP_INFO, using
// the "vwi_payload" array in viddec_workload_item_t.
// TODO: Add default values in the comments for each item
typedef struct
{
    // Frame Info - to populate register MFD_MPG4VD_BSP_FRAME_INFO
    // 0:4 - current_frame_id
    // 5:5 - current_field_frame
    // 6:10 - future_frame_id
    // 11:11 - future_field_frame
    // 12:16 - past_frame_id
    // 17:17 - past_field_frame
#define viddec_fw_mp4_get_past_field_frame(x)      viddec_fw_bitfields_extract((x)->frame_info,  17, 0x1)
#define viddec_fw_mp4_set_past_field_frame(x, val) viddec_fw_bitfields_insert((x)->frame_info, val, 17, 0x1)
#define viddec_fw_mp4_get_past_frame_id(x)         viddec_fw_bitfields_extract((x)->frame_info,  12, 0x1F)
#define viddec_fw_mp4_set_past_frame_id(x, val)    viddec_fw_bitfields_insert((x)->frame_info, val, 12, 0x1F)
#define viddec_fw_mp4_get_future_field_frame(x)      viddec_fw_bitfields_extract((x)->frame_info,  11, 0x1)
#define viddec_fw_mp4_set_future_field_frame(x, val) viddec_fw_bitfields_insert((x)->frame_info, val, 11, 0x1)
#define viddec_fw_mp4_get_future_frame_id(x)         viddec_fw_bitfields_extract((x)->frame_info,  6, 0x1F)
#define viddec_fw_mp4_set_future_frame_id(x, val)    viddec_fw_bitfields_insert((x)->frame_info, val, 6, 0x1F)
#define viddec_fw_mp4_get_current_field_frame(x)      viddec_fw_bitfields_extract((x)->frame_info,  5, 0x1)
#define viddec_fw_mp4_set_current_field_frame(x, val) viddec_fw_bitfields_insert((x)->frame_info, val, 5, 0x1)
#define viddec_fw_mp4_get_current_frame_id(x)         viddec_fw_bitfields_extract((x)->frame_info,  0, 0x1F)
#define viddec_fw_mp4_set_current_frame_id(x, val)    viddec_fw_bitfields_insert((x)->frame_info, val, 0, 0x1F)
    unsigned int frame_info;

    // Video Object Plane Info
    // 0:1 - vop_coding_type
    // 2:2 - vop_rounding_type
    // 3:5 - intra_dc_vlc_thr
    // 6:6 - top_field_first
    // 7:7 - alternate_vertical_scan_flag
    // 8:16 - vop_quant
    // 17:19 - vop_fcode_forward
    // 20:22 - vop_fcode_backward
    // 23:31 - quant_scale
#define viddec_fw_mp4_get_vop_quant_scale(x)      viddec_fw_bitfields_extract((x)->vop_data, 23, 0x1FF)
#define viddec_fw_mp4_set_vop_quant_scale(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 23, 0x1FF)
#define viddec_fw_mp4_get_vop_fcode_backward(x)      viddec_fw_bitfields_extract((x)->vop_data, 20, 0x7)
#define viddec_fw_mp4_set_vop_fcode_backward(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 20, 0x7)
#define viddec_fw_mp4_get_vop_fcode_forward(x)      viddec_fw_bitfields_extract((x)->vop_data, 17, 0x7)
#define viddec_fw_mp4_set_vop_fcode_forward(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 17, 0x7)
#define viddec_fw_mp4_get_vop_quant(x)      viddec_fw_bitfields_extract((x)->vop_data, 8, 0x1FF)
#define viddec_fw_mp4_set_vop_quant(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 8, 0x1FF)
#define viddec_fw_mp4_get_alternate_vertical_scan_flag(x)      viddec_fw_bitfields_extract((x)->vop_data, 7, 0x1)
#define viddec_fw_mp4_set_alternate_vertical_scan_flag(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 7, 0x1)
#define viddec_fw_mp4_get_top_field_first(x)      viddec_fw_bitfields_extract((x)->vop_data, 6, 0x1)
#define viddec_fw_mp4_set_top_field_first(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 6, 0x1)
#define viddec_fw_mp4_get_intra_dc_vlc_thr(x)      viddec_fw_bitfields_extract((x)->vop_data, 3, 0x7)
#define viddec_fw_mp4_set_intra_dc_vlc_thr(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 3, 0x7)
#define viddec_fw_mp4_get_vop_rounding_type(x)      viddec_fw_bitfields_extract((x)->vop_data, 2, 0x1)
#define viddec_fw_mp4_set_vop_rounding_type(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 2, 0x1)
#define viddec_fw_mp4_get_vop_coding_type(x)      viddec_fw_bitfields_extract((x)->vop_data, 0, 0x3)
#define viddec_fw_mp4_set_vop_coding_type(x, val) viddec_fw_bitfields_insert((x)->vop_data, val, 0, 0x3)
    unsigned int vop_data;

    // No of bits used in first byte of MB data
    unsigned int bit_offset;

} viddec_fw_mp4_vop_info_t;

// This structure contains the information extracted from the Video Object Layer.
// This info will be populated in the workload as item type VIDDEC_WORKLOAD_MP4_BVOP_INFO, using
// the "vwi_payload" array in viddec_workload_item_t.
// TODO: Add default values in the comments for each item
typedef struct
{
    // Frame period = T(first B-VOP after VOL) - T(past reference of first B-VOP after VOL)
    unsigned int Tframe;

    // TRD is the difference in temporal reference of the temporally next reference VOP with
    // temporally previous reference VOP, assuming B-VOPs or skipped VOPs in between.
    unsigned int TRD;

    // TRB is the difference in temporal reference of the B-VOP and the previous reference VOP.
    unsigned int TRB;

} viddec_fw_mp4_bvop_info_t;

// This structure contains the information extracted from the sprite trajectory.
// This info will be populated in the workload as item type VIDDEC_WORKLOAD_MP4_SPRT_TRAJ,
// using the fields vwi_payload in viddec_workload_item_t.
// TODO: Add default values in the comments for each item
typedef struct
{
    // Sprite Trajectory can have dmv_codes for each warping point.
    // 0:13 - warping_mv_code_dv
    // 14:27 - warping_mv_code_du
    // 28:31 - warping_point_index - identifies which warping point the warping code refers to.
    // The default value for index is 0xF which should be treated as invalid.
#define viddec_fw_mp4_get_warping_point_index(x)      viddec_fw_bitfields_extract((x), 28, 0xF)
#define viddec_fw_mp4_set_warping_point_index(x, val) viddec_fw_bitfields_insert((x), val, 28, 0xF)
#define viddec_fw_mp4_get_warping_mv_code_du(x)      viddec_fw_bitfields_extract((x), 14, 0x3FFF)
#define viddec_fw_mp4_set_warping_mv_code_du(x, val) viddec_fw_bitfields_insert((x), val, 14, 0x3FFF)
#define viddec_fw_mp4_get_warping_mv_code_dv(x)      viddec_fw_bitfields_extract((x), 0, 0x3FFF)
#define viddec_fw_mp4_set_warping_mv_code_dv(x, val) viddec_fw_bitfields_insert((x), val, 0, 0x3FFF)
    unsigned int warping_mv_code[3];
} viddec_fw_mp4_sprite_trajectory_t;

// IQUANT entries will be populated in the workload using items of type VIDDEC_WORKLOAD_MP4_IQUANT and the
// vwi_payload array. The entries will be in the order in which they need to be programmed in the registers.
// There is no need for a separate structure for these values.

// This structure contains the information extracted from the Video Plane with Short Header.
// This info will be populated in the workload as item type VIDDEC_WORKLOAD_MP4_SVH, using
// the "vwi_payload" array in viddec_workload_item_t.
// TODO: Add default values in the comments for each item
typedef struct
{
    // Video Plane with Short Header
    // 0:7 - temporal_reference
    // 8:19 - num_macroblocks_in_gob
    // 20:24 - num_gobs_in_vop
    // 25:27 - num_rows_in_gob
#define viddec_fw_mp4_get_num_rows_in_gob(x)      viddec_fw_bitfields_extract((x)->svh_data, 25, 0x7)
#define viddec_fw_mp4_set_num_rows_in_gob(x, val) viddec_fw_bitfields_insert((x)->svh_data, val, 25, 0x7)
#define viddec_fw_mp4_get_num_gobs_in_vop(x)      viddec_fw_bitfields_extract((x)->svh_data, 20, 0x1F)
#define viddec_fw_mp4_set_num_gobs_in_vop(x, val) viddec_fw_bitfields_insert((x)->svh_data, val, 20, 0x1F)
#define viddec_fw_mp4_get_num_macroblocks_in_gob(x)      viddec_fw_bitfields_extract((x)->svh_data, 8, 0xFFF)
#define viddec_fw_mp4_set_num_macroblocks_in_gob(x, val) viddec_fw_bitfields_insert((x)->svh_data, val, 8, 0xFFF)
#define viddec_fw_mp4_get_temporal_reference(x)      viddec_fw_bitfields_extract((x)->svh_data, 0, 0xFF)
#define viddec_fw_mp4_set_temporal_reference(x, val) viddec_fw_bitfields_insert((x)->svh_data, val, 0, 0xFF)
    unsigned int svh_data;

    unsigned int pad1;
    unsigned int pad2;
} viddec_fw_mp4_svh_t;

#endif
