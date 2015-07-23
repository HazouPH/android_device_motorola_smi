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


#ifndef _H264_H_
#define _H264_H_

#ifdef HOST_ONLY
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#endif

#include "stdint.h"
#include "viddec_debug.h"

#include "viddec_fw_workload.h"
#include "h264parse_sei.h"

#ifdef VBP
//#define SW_ERROR_CONCEALEMNT
#endif

#ifdef WIN32
#define mfd_printf OS_INFO
#endif

#ifdef H264_VERBOSE
#define PRINTF(format, args...) OS_INFO("%s:  %s[%d]:: " format, __FILE__, __FUNCTION__ , __LINE__ ,  ## args )
#else
//#define PRINTF(args...)
#endif

//#pragma warning(disable : 4710) // function not inlined
//#pragma warning(disable : 4514) // unreferenced inline function has been removed CL
//#pragma warning(disable : 4100) // unreferenced formal parameter CL

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INT32_VALUE 	0x7fffffff

#define MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE 256
#define MAX_CPB_CNT	32
#define MAX_NUM_SLICE_GRPS 	1				//As per Annex A for high profile, the num_slice_groups_minus1 is 0
#define MAX_PIC_LIST_NUM	8

//#define MAX_PIC_SIZE_IN_MAP_UNITS	1024 //0 ???????? Henry
#define MAX_NUM_REF_IDX_L0_ACTIVE	32
//#define STARTCODE_BUF_SIZE 			2048+1024

#define NUM_MMCO_OPERATIONS         17

// Used to check whether the SEI RP is the only way for recovery (cisco contents)
// This threshold will decide the interval of recovery even no error detected if no IDR during this time
#define SEI_REC_CHECK_TH				8

//SPS
#define MAX_NUM_SPS			32
#define SCL_DEFAULT 		1

//PPS
#define MAX_PIC_PARAMS		255
#define MAX_NUM_REF_FRAMES	32
#define MAX_QP				51
#define MAX_NUM_PPS			256

#define PUT_FS_IDC_BITS(w)                                (w&0x1F)
#define PUT_LIST_INDEX_FIELD_BIT(w)                       ((w&0x1)<<5)
#define PUT_LIST_LONG_TERM_BITS(w)                        ((w&0x1)<<6)
#define PUT_LIST_PTR_LIST_ID_BIT(id)                      (id<<5)


// DPB
#define FRAME_FLAG_DANGLING_TOP_FIELD        ( 0x1 << 3  )
#define FRAME_FLAG_DANGLING_BOTTOM_FIELD     ( 0x1 << 4  )

#define MPD_DPB_FS_NULL_IDC			31            // May need to be changed if we alter gaps_in_frame_num to use 

#define MFD_H264_MAX_FRAME_BUFFERS  17
#define NUM_DPB_FRAME_STORES        (MFD_H264_MAX_FRAME_BUFFERS + 1)  // 1 extra for storign non-existent pictures.

//Scalling Matrix Type
#define PPS_QM                  0
#define SPS_QM                  1
#define FB_QM                   2
#define DEFAULT_QM              3

//Frame Type
#define FRAME_TYPE_IDR        0x00
#define FRAME_TYPE_I          0x01
#define FRAME_TYPE_P          0x02
#define FRAME_TYPE_B          0x03
#define FRAME_TYPE_INVALID    0x04


#define FRAME_TYPE_FRAME_OFFSET     3
#define FRAME_TYPE_TOP_OFFSET       3
#define FRAME_TYPE_BOTTOM_OFFSET    0
#define FRAME_TYPE_STRUCTRUE_OFFSET 6

//// Error handling
#define FIELD_ERR_OFFSET		17			//offset for Field error flag ----refer to the structure definition viddec_fw_workload_error_codes in viddec_fw_common_defs.h

////Bits Handling
#define h264_bitfields_extract(x_32, start, mask)     (((x_32) >> (start)) & (mask) )
#define h264_bitfields_insert(x_32, val_32, start, mask) ((x_32) = (((x_32) & ~( (mask) << (start))) | (((val_32) & (mask)) << (start))))


//// PIP
    typedef enum _pip_setting_t
    {
        PIP_SCALER_DISABLED,
        PIP_SCALE_FACTOR_1_BY_4,
        PIP_SCALE_FACTOR_1_BY_2,
        PIP_SCALER_INVALID,

    } pip_setting_t;


#ifdef VERBOSE
#define DEBUGGETBITS(args...)  OS_INFO( args )
#else
//#define DEBUGGETBITS(args...)
#endif

    /* status codes */
    typedef enum _h264_Status
    {
        H264_STATUS_EOF          =  1,   // end of file
        H264_STATUS_OK           =  0,   // no error
        H264_STATUS_NO_MEM       =  2,   // out of memory
        H264_STATUS_FILE_ERROR   =  3,   // file error
        H264_STATUS_NOTSUPPORT   =  4,   // not supported mode
        H264_STATUS_PARSE_ERROR  =  5,   // fail in parse MPEG-4 stream
        H264_STATUS_ERROR        =  6,   // unknown/unspecified error
        H264_NAL_ERROR,
        H264_SPS_INVALID_PROFILE,
        H264_SPS_INVALID_LEVEL,
        H264_SPS_INVALID_SEQ_PARAM_ID,
        H264_SPS_ERROR,
        H264_PPS_INVALID_PIC_ID,
        H264_PPS_INVALID_SEQ_ID,
        H264_PPS_ERROR,
        H264_SliceHeader_INVALID_MB,
        H264_SliceHeader_ERROR,
        H264_FRAME_DONE,
        H264_SLICE_DONE,
        H264_STATUS_POLL_ONCE_ERROR,
        H264_STATUS_DEC_MEMINIT_ERROR,
        H264_STATUS_NAL_UNIT_TYPE_ERROR,
        H264_STATUS_SEI_ERROR,
        H264_STATUS_SEI_DONE,
    } h264_Status;



    typedef enum _picture_structure_t
    {
        TOP_FIELD		= 1,
        BOTTOM_FIELD		= 2,
        FRAME			= 3,
        INVALID			= 4
    } picture_structure_t;

///// Chorma format

    typedef enum _h264_chroma_format_t
    {
        H264_CHROMA_MONOCHROME,
        H264_CHROMA_420,
        H264_CHROMA_422,
        H264_CHROMA_444,
    } h264_chroma_format_t;

    /* H264 start code values */
    typedef enum _h264_nal_unit_type
    {
        h264_NAL_UNIT_TYPE_unspecified = 0,
        h264_NAL_UNIT_TYPE_SLICE,
        h264_NAL_UNIT_TYPE_DPA,
        h264_NAL_UNIT_TYPE_DPB,
        h264_NAL_UNIT_TYPE_DPC,
        h264_NAL_UNIT_TYPE_IDR,
        h264_NAL_UNIT_TYPE_SEI,
        h264_NAL_UNIT_TYPE_SPS,
        h264_NAL_UNIT_TYPE_PPS,
        h264_NAL_UNIT_TYPE_Acc_unit_delimiter,
        h264_NAL_UNIT_TYPE_EOSeq,
        h264_NAL_UNIT_TYPE_EOstream,
        h264_NAL_UNIT_TYPE_filler_data,
        h264_NAL_UNIT_TYPE_SPS_extension,
        h264_NAL_UNIT_TYPE_Reserved1			=14,		/*14-18*/
        h264_NAL_UNIT_TYPE_Reserved2			=15,		/*14-18*/
        h264_NAL_UNIT_TYPE_Reserved3			=16,		/*14-18*/
        h264_NAL_UNIT_TYPE_Reserved4			=17,		/*14-18*/
        h264_NAL_UNIT_TYPE_Reserved5			=18,		/*14-18*/
        h264_NAL_UNIT_TYPE_ACP				=19,
        h264_NAL_UNIT_TYPE_Reserved6			=20,		/*20-23*/
        h264_NAL_UNIT_TYPE_unspecified2		=24,		/*24-31*/
    } h264_nal_unit_type;

#define h264_NAL_PRIORITY_HIGHEST     3
#define h264_NAL_PRIORITY_HIGH        2
#define h264_NAL_PRIRITY_LOW          1
#define h264_NAL_PRIORITY_DISPOSABLE  0


    typedef enum _h264_Profile
    {
        h264_ProfileBaseline = 66,  	/** Baseline profile */
        h264_ProfileMain = 77,        	/** Main profile */
        h264_ProfileExtended = 88,    	/** Extended profile */
        h264_ProfileHigh = 100 ,     		/** High profile */
        h264_ProfileHigh10 = 110,			/** High 10 profile */
        h264_ProfileHigh422 = 122,		/** High profile 4:2:2 */
        h264_ProfileHigh444 = 144,		/** High profile 4:4:4 */
    } h264_Profile;


    typedef enum _h264_Level
    {
        h264_Level1b	= 9,		    /** Level 1b */
        h264_Level1		= 10,			/** Level 1 */
        h264_Level11	= 11, 		    /** Level 1.1 */
        h264_Level12	= 12, 		    /** Level 1.2 */
        h264_Level13	= 13, 		    /** Level 1.3 */
        h264_Level2		= 20,			/** Level 2 */
        h264_Level21 	= 21, 		    /** Level 2.1 */
        h264_Level22	= 22, 		    /** Level 2.2 */
        h264_Level3		= 30, 		    /** Level 3 */
        h264_Level31	= 31, 		    /** Level 3.1 */
        h264_Level32	= 32, 		    /** Level 3.2 */
        h264_Level4		= 40, 		    /** Level 4 */
        h264_Level41	= 41, 		    /** Level 4.1 */
        h264_Level42	= 42, 		    /** Level 4.2 */
        h264_Level5		= 50, 		    /** Level 5 */
        h264_Level51	= 51, 		    /** Level 5.1 */
        h264_LevelReserved = 255  /** Unknown profile */
    } h264_Level;


    typedef enum _h264_video_format
    {
        h264_Component	=0,
        h264_PAL,
        h264_NTSC,
        h264_SECAM,
        h264_MAC,
        h264_unspecified,
        h264_Reserved6,
        h264_Reserved7
    } h264_video_format;


    typedef enum _h264_fcm
    {
        h264_ProgressiveFrame = 0,
        h264_InterlacedFrame  = 1,
        h264_InterlacedField  = 3,
        h264_PictureFormatNone
    } h264_fcm;


///// Define the picture types []
    typedef enum _h264_ptype_t
    {
        h264_PtypeP = 0,
        h264_PtypeB = 1,
        h264_PtypeI = 2,
        h264_PtypeSP = 3,
        h264_PtypeSI = 4,
        h264_Ptype_unspecified,
    } h264_ptype_t;


///// Aspect ratio
    typedef enum _h264_aspect_ratio
    {
        h264_AR_Unspecified = 0,
        h264_AR_1_1 = 1,
        h264_AR_12_11 = 2,
        h264_AR_10_11 = 3,
        h264_AR_16_11 = 4,
        h264_AR_40_33 = 5,
        h264_AR_24_11 = 6,
        h264_AR_20_11 = 7,
        h264_AR_32_11 = 8,
        h264_AR_80_33 = 9,
        h264_AR_18_11 = 10,
        h264_AR_15_11 = 11,
        h264_AR_64_33 = 12,
        h264_AR_160_99 = 13,
        h264_AR_4_3 = 14,
        h264_AR_3_2 = 15,
        h264_AR_2_1 = 16,
        h264_AR_RESERVED = 17,
        h264_AR_Extended_SAR = 255,
    } h264_aspect_ratio;


//////////////////////////////////////////////

//////////////////////////////////////////////
// storable_picture

    /* Structure details
       If all members remain ints
       Size = 11 ints, i.e. 44 bytes
    */

    typedef struct
    {
        int32_t	poc;
        int32_t	pic_num;

        int32_t	long_term_pic_num;

        uint8_t	long_term_frame_idx;
        uint8_t	is_long_term;
        uint8_t	used_for_reference;
        uint8_t	pad_flag;  		// Used to indicate the status

    } storable_picture, *storable_picture_ptr;

//////////////////////////////////////////////
// frame store

    /* Structure details
       If all members remain ints
       Size = 46 ints, i.e. 184 bytes
    */

    typedef struct _frame_store
    {
        storable_picture frame;
        storable_picture top_field;
        storable_picture bottom_field;

        int32_t	frame_num;

        int32_t	frame_num_wrap;


        uint8_t	fs_idc;
        uint8_t	pic_type;            //bit7 structure: 1 frame , 0 field;
        //bit4,5,6 top field (frame) pic type,  00 IDR 01 I 10 P 11 B 100 INVALID
        //bit1,2,3 bottom pic type,  00 IDR 01 I 10 P 11 B 100 INVALID
        uint8_t	long_term_frame_idx; // No two frame stores may have the same long-term frame index

#define viddec_h264_get_dec_structure(x)         h264_bitfields_extract( (x)->fs_flag_1, 0, 0x03)
#define viddec_h264_set_dec_structure(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 0, 0x03)
#define viddec_h264_get_is_used(x)         h264_bitfields_extract( (x)->fs_flag_1, 2, 0x03)
#define viddec_h264_set_is_frame_used(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 2, 0x03)
#define viddec_h264_set_is_top_used(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 2, 0x01)
#define viddec_h264_set_is_bottom_used(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 3, 0x01)
#define viddec_h264_get_is_skipped(x)         h264_bitfields_extract( (x)->fs_flag_1, 4, 0x03)
#define viddec_h264_set_is_frame_skipped(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 4, 0x03)
#define viddec_h264_set_is_top_skipped(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 4, 0x01)
#define viddec_h264_set_is_bottom_skipped(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 5, 0x01)
#define viddec_h264_get_is_long_term(x)         h264_bitfields_extract( (x)->fs_flag_1, 6, 0x03)
#define viddec_h264_set_is_frame_long_term(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 6, 0x03)
#define viddec_h264_set_is_top_long_term(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 6, 0x01)
#define viddec_h264_set_is_bottom_long_term(x, val)    h264_bitfields_insert ( (x)->fs_flag_1, (val), 7, 0x01)
        uint8_t  fs_flag_1;


#define viddec_h264_get_is_non_existent(x)            h264_bitfields_extract( (x)->fs_flag_2, 0, 0x01)
#define viddec_h264_set_is_non_existent(x, val)       h264_bitfields_insert ( (x)->fs_flag_2, (val), 0, 0x01)
#define viddec_h264_get_is_output(x)                  h264_bitfields_extract( (x)->fs_flag_2, 1, 0x01)
#define viddec_h264_set_is_output(x, val)             h264_bitfields_insert ( (x)->fs_flag_2, (val), 1, 0x01)
#define viddec_h264_get_is_dangling(x)                h264_bitfields_extract( (x)->fs_flag_2, 2, 0x01)
#define viddec_h264_set_is_dangling(x, val)           h264_bitfields_insert ( (x)->fs_flag_2, (val), 2, 0x01)
#define viddec_h264_get_recovery_pt_picture(x)        h264_bitfields_extract( (x)->fs_flag_2, 3, 0x01)
#define viddec_h264_set_recovery_pt_picture(x, val)   h264_bitfields_insert ( (x)->fs_flag_2, (val), 3, 0x01)
#define viddec_h264_get_broken_link_picture(x)        h264_bitfields_extract( (x)->fs_flag_2, 4, 0x01)
#define viddec_h264_set_broken_link_picture(x, val)   h264_bitfields_insert ( (x)->fs_flag_2, (val), 4, 0x01)
#define viddec_h264_get_open_gop_entry(x)             h264_bitfields_extract( (x)->fs_flag_2, 5, 0x01)
#define viddec_h264_set_open_gop_entry(x, val)        h264_bitfields_insert ( (x)->fs_flag_2, (val), 5, 0x01)
#define viddec_h264_get_first_field_intra(x)          h264_bitfields_extract( (x)->fs_flag_2, 6, 0x01)
#define viddec_h264_set_first_field_intra(x, val)     h264_bitfields_insert ( (x)->fs_flag_2, (val), 6, 0x01)
        uint8_t  fs_flag_2;

        uint8_t  fs_flag_reserve_1;
        uint8_t  fs_flag_reserve_2;
        uint8_t  fs_flag_reserve_3;

        // If non-reference, may have skipped pixel decode
        //uint8_t	non_ref_skipped;
    } frame_store, *frame_param_ptr;

//! Decoded Picture Buffer
    typedef struct _h264_decoded_picture_buffer
    {
        ///
        int32_t     last_output_poc;
        int32_t     max_long_term_pic_idx;

        //// Resolutions
        int32_t		PicWidthInMbs;
        int32_t		FrameHeightInMbs;

        frame_store	fs[NUM_DPB_FRAME_STORES];
        frame_store*    active_fs;

        uint8_t		fs_ref_idc[16];
        uint8_t		fs_ltref_idc[16];

        uint8_t		fs_dpb_idc[NUM_DPB_FRAME_STORES+2];

        uint8_t		listX_0[33+3];  // [bit5}:field_flag:0 for top, 1 for bottom, [bit4~0]:fs_idc
        uint8_t		listX_1[33+3];

        uint8_t		listXsize[2]; // 1 to 32
        uint8_t		nInitListSize[2];

        //uint32_t	size;
        uint8_t		fs_dec_idc;
        uint8_t		fs_non_exist_idc;
        uint8_t		BumpLevel;
        uint8_t		used_size;

        uint8_t		OutputLevel;
        uint8_t		OutputLevelValid;
        uint8_t		OutputCtrl;
        uint8_t     num_ref_frames;

        uint8_t		ref_frames_in_buffer;
        uint8_t		ltref_frames_in_buffer;
        uint8_t		SuspendOutput;
        uint8_t		WaitSeiRecovery;


        uint8_t		frame_numbers_need_to_be_allocated;
        uint8_t		frame_id_need_to_be_allocated;

        //// frame list to release from dpb, need be displayed
        uint8_t		frame_numbers_need_to_be_removed;
        uint8_t		frame_id_need_to_be_removed[17];

        //// frame list to removed from dpb but not display
        uint8_t		frame_numbers_need_to_be_dropped;
        uint8_t		frame_id_need_to_be_dropped[17];

        //// frame list to display (in display order)
        uint8_t		frame_numbers_need_to_be_displayed;
        uint8_t		frame_id_need_to_be_displayed[17];


    } h264_DecodedPictureBuffer;


//////////////////////////////////////////////
// qm_matrix_set
    typedef struct _qm_matrix_set
    {
// uint8_t scaling_default_vector;
        uint8_t scaling_list[56];            // 0 to 23 for qm 0 to 5 (4x4), 24 to 55 for qm 6 & 7 (8x8)

    } qm_matrix_set, *qm_matrix_set_ptr;

    /*
    ///////// Currently not enabled in parser fw///////////////////
    typedef struct _h264_SPS_Extension_RBSP {
    	int32_t 			seq_parameter_set_id;					//UE
    	int32_t				aux_format_idc;							//UE
    	int32_t				bit_depth_aux_minus8;					//UE
    	int32_t				alpha_incr_flag;
    	int32_t				alpha_opaque_value;
    	int32_t				alpha_transparent_value;
    	int32_t				additional_extension_flag;
    //	h264_rbsp_trail_set* rbsp_trail_ptr;
    }h264_SPS_Extension_RBSP_t;
    */

    typedef struct _h264_hrd_param_set {
        int32_t				bit_rate_value_minus1[MAX_CPB_CNT];			// ue(v), 0 to (2^32)-2
        int32_t				cpb_size_value_minus1[MAX_CPB_CNT];			// ue(v), 0 to (2^32)-2

        uint8_t				cbr_flag[MAX_CPB_CNT];							// u(1) * 32

    } h264_hrd_param_set, *h264_hrd_param_set_ptr;

    typedef struct _vui_seq_parameters_t_used
    {
        uint32_t	num_units_in_tick;                             // u(32)
        uint32_t	time_scale;                                    // u(32)

        int32_t  num_reorder_frames;                               // ue(v), 0 to max_dec_frame_buffering
        int32_t	max_dec_frame_buffering;                          // ue(v), 0 to MaxDpbSize, specified in subclause A.3

        uint16_t	 sar_width;                                       // u(16)
        uint16_t	 sar_height;                                      // u(16)

        uint8_t   aspect_ratio_info_present_flag;                  // u(1)
        uint8_t   aspect_ratio_idc;                                // u(8)
        uint8_t   video_signal_type_present_flag;                  // u(1)
        uint8_t   video_format;                                    // u(3)
#ifdef VBP
        uint8_t   video_full_range_flag;                           // u(1)
        uint8_t   matrix_coefficients;                              // u(8)
        uint32_t  bit_rate_value;
#endif

        uint8_t   colour_description_present_flag;                 // u(1)
        uint8_t   colour_primaries;                                // u(8)
        uint8_t   transfer_characteristics;                        // u(8)
        uint8_t   timing_info_present_flag;                        // u(1)

        uint8_t   fixed_frame_rate_flag;                           // u(1)
        uint8_t   low_delay_hrd_flag;                              // u(1)
        uint8_t   bitstream_restriction_flag;                      // u(1)
        uint8_t   pic_struct_present_flag;

        uint8_t   nal_hrd_parameters_present_flag;                 // u(1)
        uint8_t 	 nal_hrd_cpb_removal_delay_length_minus1;				// u(5)
        uint8_t   nal_hrd_dpb_output_delay_length_minus1;				// u(5)
        uint8_t   nal_hrd_time_offset_length;								// u(5)

        uint8_t   nal_hrd_cpb_cnt_minus1;									// ue(v), 0 to 31
        uint8_t   nal_hrd_initial_cpb_removal_delay_length_minus1;	// u(5)
        uint8_t   vcl_hrd_parameters_present_flag;                 // u(1)
        uint8_t 	 vcl_hrd_cpb_removal_delay_length_minus1;				// u(5)

        uint8_t   vcl_hrd_dpb_output_delay_length_minus1;				// u(5)
        uint8_t   vcl_hrd_time_offset_length;								// u(5)
        uint8_t   vcl_hrd_cpb_cnt_minus1;									// ue(v), 0 to 31
        uint8_t   vcl_hrd_initial_cpb_removal_delay_length_minus1;	// u(5)

        /////// Here should be kept as 32-bits aligned for next structures
        /// 2 structures for NAL&VCL HRD


    } vui_seq_parameters_t_used;


    typedef struct _vui_seq_parameters_t_not_used
    {
        int16_t  chroma_sample_loc_type_top_field;                // ue(v)
        int16_t  chroma_sample_loc_type_bottom_field;             // ue(v)

        uint8_t   overscan_info_present_flag;                      // u(1)
        uint8_t   overscan_appropriate_flag;                       // u(1)

        uint8_t   video_full_range_flag;                           // u(1)
        uint8_t   matrix_coefficients;                             // u(8)

        uint8_t   chroma_location_info_present_flag;               // u(1)
        uint8_t   max_bytes_per_pic_denom;                          // ue(v), 0 to 16
        uint8_t   max_bits_per_mb_denom;                            // ue(v), 0 to 16
        uint8_t   log2_max_mv_length_vertical;                      // ue(v), 0 to 16, default to 16
        uint8_t   log2_max_mv_length_horizontal;                    // ue(v), 0 to 16, default to 16

        uint8_t   motion_vectors_over_pic_boundaries_flag;          // u(1)

        uint8_t   nal_hrd_bit_rate_scale;									// u(4)
        uint8_t   nal_hrd_cpb_size_scale;									// u(4)

        uint8_t   vcl_hrd_bit_rate_scale;									// u(4)
        uint8_t   vcl_hrd_cpb_size_scale;									// u(4)

        h264_hrd_param_set nal_hrd_parameters;
        h264_hrd_param_set vcl_hrd_parameters;


    } vui_seq_parameters_t_not_used, *vui_seq_parameters_t_not_used_ptr;


//////////////////////////////////////////////
// picture parameter set

    typedef struct _PPS_PAR
    {
        //int32_t DOUBLE_ALIGN valid;                          // indicates the parameter set is valid

        int32_t pic_init_qp_minus26;                             // se(v), -26 to +25
        int32_t pic_init_qs_minus26;                             // se(v), -26 to +25
        int32_t chroma_qp_index_offset;                          // se(v), -12 to +12
        int32_t second_chroma_qp_index_offset;

        uint8_t pic_parameter_set_id;                            // ue(v), 0 to 255, restricted to 0 to 127 by MPD_CTRL_MAXPPS = 128
        uint8_t seq_parameter_set_id;                            // ue(v), 0 to 31
        uint8_t entropy_coding_mode_flag;                        // u(1)
        uint8_t pic_order_present_flag;                          // u(1)

        uint8_t num_slice_groups_minus1;                         // ue(v), shall be 0 for MP
        // Below are not relevant for main profile...
        uint8_t slice_group_map_type;                            // ue(v), 0 to 6
        uint8_t num_ref_idx_l0_active;							// ue(v), 0 to 31
        uint8_t num_ref_idx_l1_active;							// ue(v), 0 to 31

        uint8_t weighted_pred_flag;                              // u(1)
        uint8_t weighted_bipred_idc;                             // u(2)
        uint8_t deblocking_filter_control_present_flag;          // u(1)
        uint8_t constrained_intra_pred_flag;                     // u(1)

        uint8_t redundant_pic_cnt_present_flag;                  // u(1)
        uint8_t transform_8x8_mode_flag;
        uint8_t pic_scaling_matrix_present_flag;
        uint8_t pps_status_flag;

        //// Keep here with 32-bits aligned
        uint8_t	pic_scaling_list_present_flag[MAX_PIC_LIST_NUM];

        qm_matrix_set	pps_qm;

        uint8_t 		ScalingList4x4[6][16];
        uint8_t 		ScalingList8x8[2][64];
        uint8_t   	UseDefaultScalingMatrix4x4Flag[6+2];
        uint8_t		UseDefaultScalingMatrix8x8Flag[6+2];

    } pic_param_set, *pic_param_set_ptr, h264_PicParameterSet_t;

    typedef union _list_reordering_num_t
    {
        int32_t abs_diff_pic_num_minus1;
        int32_t long_term_pic_num;
    } list_reordering_num_t;

    typedef struct _h264_Ref_Pic_List_Reordering				////size = 8*33+ 1 + 33
    {
        list_reordering_num_t list_reordering_num[MAX_NUM_REF_FRAMES+1];

        uint8_t			ref_pic_list_reordering_flag;
        uint8_t			reordering_of_pic_nums_idc[MAX_NUM_REF_FRAMES+1];							//UE

    } h264_Ref_Pic_List_Reordering_t;

    typedef enum _H264_DANGLING_TYPE
    {
        DANGLING_TYPE_LAST_FIELD,
        DANGLING_TYPE_DPB_RESET,
        DANGLING_TYPE_FIELD,
        DANGLING_TYPE_FRAME,
        DANGLING_TYPE_GAP_IN_FRAME

    } H264_DANGLING_TYPE;


    typedef struct _h264_Dec_Ref_Pic_Marking			//size = 17*4*2 + 17*3 + 4 + 1
    {
        int32_t		difference_of_pic_num_minus1[NUM_MMCO_OPERATIONS];
        int32_t		long_term_pic_num[NUM_MMCO_OPERATIONS];

        /// MMCO
        uint8_t		memory_management_control_operation[NUM_MMCO_OPERATIONS];
        uint8_t		max_long_term_frame_idx_plus1[NUM_MMCO_OPERATIONS];
        uint8_t		long_term_frame_idx[NUM_MMCO_OPERATIONS];
        uint8_t		long_term_reference_flag;

        uint8_t		adaptive_ref_pic_marking_mode_flag;
        uint8_t		dec_ref_pic_marking_count;
        uint8_t		no_output_of_prior_pics_flag;

        uint8_t		pad;
    } h264_Dec_Ref_Pic_Marking_t;



    typedef struct old_slice_par
    {
        int32_t		frame_num;
        int32_t		pic_order_cnt_lsb;
        int32_t		delta_pic_order_cnt_bottom;
        int32_t		delta_pic_order_cnt[2];

        uint8_t		field_pic_flag;
        uint8_t		bottom_field_flag;
        uint8_t		nal_ref_idc;
        uint8_t		structure;

        uint8_t		idr_flag;
        uint8_t		idr_pic_id;
        uint8_t		pic_parameter_id;
        uint8_t		status;
    } OldSliceParams;

#ifdef VBP
    typedef struct _h264__pred_weight_table
    {
        uint8_t luma_log2_weight_denom;
        uint8_t chroma_log2_weight_denom;
        uint8_t luma_weight_l0_flag;
        int16_t luma_weight_l0[32];
        int8_t luma_offset_l0[32];
        uint8_t chroma_weight_l0_flag;
        int16_t chroma_weight_l0[32][2];
        int8_t chroma_offset_l0[32][2];

        uint8_t luma_weight_l1_flag;
        int16_t luma_weight_l1[32];
        int8_t luma_offset_l1[32];
        uint8_t chroma_weight_l1_flag;
        int16_t chroma_weight_l1[32][2];
        int8_t chroma_offset_l1[32][2];
    } h264_pred_weight_table;
#endif

    typedef struct _h264_Slice_Header
    {
        int32_t 		first_mb_in_slice;								//UE
        int32_t		frame_num;											//UV
        int32_t		pic_order_cnt_lsb;								//UV
        int32_t		delta_pic_order_cnt_bottom;					//SE
        int32_t		delta_pic_order_cnt[2];								//SE
        int32_t		redundant_pic_cnt;									//UE

        uint32_t		num_ref_idx_l0_active;								//UE
        uint32_t		num_ref_idx_l1_active;								//UE

        int32_t		slice_qp_delta;										//SE
        int32_t		slice_qs_delta;										//SE
        int32_t		slice_alpha_c0_offset_div2;						//SE
        int32_t		slice_beta_offset_div2;								//SE
        int32_t		slice_group_change_cycle;							//UV

#ifdef VBP
        h264_pred_weight_table  sh_predwttbl;
#endif

        ///// Flags or IDs
        //h264_ptype_t	slice_type;											//UE
        uint8_t			slice_type;
        uint8_t 			nal_ref_idc;
        uint8_t			structure;
        uint8_t 			pic_parameter_id;									//UE

        uint8_t			field_pic_flag;
        uint8_t			bottom_field_flag;
        uint8_t			idr_flag;											//UE
        uint8_t			idr_pic_id;											//UE

        uint8_t 			sh_error;
        uint8_t			cabac_init_idc;										//UE
        uint8_t			sp_for_switch_flag;
        uint8_t			disable_deblocking_filter_idc;						//UE

        uint8_t			direct_spatial_mv_pred_flag;
        uint8_t			num_ref_idx_active_override_flag;
        int16_t			current_slice_nr;

        //// For Ref list reordering
        h264_Dec_Ref_Pic_Marking_t sh_dec_refpic;
        h264_Ref_Pic_List_Reordering_t sh_refpic_l0;
        h264_Ref_Pic_List_Reordering_t sh_refpic_l1;

    } h264_Slice_Header_t;


#define   MAX_USER_DATA_SIZE              1024
    typedef struct _h264_user_data_t
    {
        h264_sei_payloadtype    user_data_type;

        int32_t    user_data_id;
        int32_t    dsn;
        int32_t    user_data_size;
        int32_t    user_data[MAX_USER_DATA_SIZE>>2];
    } h264_user_data_t;

// SPS DISPLAY parameters: seq_param_set_disp, *seq_param_set_disp_ptr;
    typedef struct _SPS_DISP
    {
        ///// VUI info
        vui_seq_parameters_t_used vui_seq_parameters;    //size =

        ///// Resolution
        int16_t pic_width_in_mbs_minus1;
        int16_t pic_height_in_map_units_minus1;

        ///// Cropping
        int16_t frame_crop_rect_left_offset;
        int16_t frame_crop_rect_right_offset;

        int16_t frame_crop_rect_top_offset;
        int16_t frame_crop_rect_bottom_offset;

        ///// Flags
        uint8_t frame_mbs_only_flag;
        uint8_t mb_adaptive_frame_field_flag;
        uint8_t direct_8x8_inference_flag;
        uint8_t frame_cropping_flag;
#ifdef VBP
        uint8_t separate_colour_plane_flag;
#endif

        uint16_t vui_parameters_present_flag;
        uint16_t chroma_format_idc;
    } seq_param_set_disp, *seq_param_set_disp_ptr;


////SPS: seq_param_set, *seq_param_set_ptr;

    typedef struct _SPS_PAR_USED
    {
        uint32_t    is_updated;

        /////////// Required for display section //////////////////////////
        seq_param_set_disp sps_disp;

        int32_t		expectedDeltaPerPOCCycle;
        int32_t 		offset_for_non_ref_pic;                           // se(v), -2^31 to (2^31)-1, 32-bit integer
        int32_t 		offset_for_top_to_bottom_field;                   // se(v), -2^31 to (2^31)-1, 32-bit integer

        /////////// IDC
        uint8_t 		profile_idc;                                      // u(8), 0x77 for MP
        uint8_t 		constraint_set_flags;                             // bit 0 to 3 for set0 to set3
        uint8_t 		level_idc;                                        // u(8)
        uint8_t 		seq_parameter_set_id;                             // ue(v), 0 to 31


        uint8_t 		pic_order_cnt_type;                               // ue(v), 0 to 2
        uint8_t 		log2_max_frame_num_minus4;                        // ue(v), 0 to 12
        uint8_t 		log2_max_pic_order_cnt_lsb_minus4;                // ue(v), 0 to 12
        uint8_t 		num_ref_frames_in_pic_order_cnt_cycle;            // ue(v), 0 to 255

        //int32_t offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];   // se(v), -2^31 to (2^31)-1, 32-bit integer
        uint8_t 		num_ref_frames;                                   // ue(v), 0 to 16,
        uint8_t 		gaps_in_frame_num_value_allowed_flag;             // u(1)
        // This is my addition, we should calculate this once and leave it with the sps
        // as opposed to calculating it each time in h264_hdr_decoding_POC()

        uint8_t 		delta_pic_order_always_zero_flag;                 // u(1)
        uint8_t		residual_colour_transform_flag;

        uint8_t		bit_depth_luma_minus8;
        uint8_t		bit_depth_chroma_minus8;
        uint8_t		lossless_qpprime_y_zero_flag;
        uint8_t		seq_scaling_matrix_present_flag;

        uint8_t		seq_scaling_list_present_flag[MAX_PIC_LIST_NUM];			//0-7

        //// Combine the scaling matrix to word ( 24 + 32)
        uint8_t 		ScalingList4x4[6][16];
        uint8_t 		ScalingList8x8[2][64];
        uint8_t		UseDefaultScalingMatrix4x4Flag[6];
        uint8_t		UseDefaultScalingMatrix8x8Flag[6];

    } seq_param_set_used, *seq_param_set_used_ptr;


    typedef struct _SPS_PAR_ALL
    {

        seq_param_set_used  sps_par_used;
        vui_seq_parameters_t_not_used sps_vui_par_not_used;

    } seq_param_set_all, *seq_param_set_all_ptr;


///// Image control parameter////////////
    typedef struct _h264_img_par
    {
        int32_t frame_num;				// decoding num of current frame
        int32_t frame_count;				// count of decoded frames
        int32_t current_slice_num;
        int32_t gaps_in_frame_num;

        // POC decoding
        int32_t num_ref_frames_in_pic_order_cnt_cycle;
        int32_t delta_pic_order_always_zero_flag;
        int32_t offset_for_non_ref_pic;
        int32_t offset_for_top_to_bottom_field;

        int32_t pic_order_cnt_lsb;
        int32_t pic_order_cnt_msb;
        int32_t delta_pic_order_cnt_bottom;
        int32_t delta_pic_order_cnt[2];

        int32_t PicOrderCntMsb;
        int32_t CurrPicOrderCntMsb;
        int32_t PrevPicOrderCntLsb;

        int32_t FrameNumOffset;

        int32_t PreviousFrameNum;
        int32_t PreviousFrameNumOffset;

        int32_t toppoc;
        int32_t bottompoc;
        int32_t framepoc;
        int32_t ThisPOC;

        //int32_t sei_freeze_this_image;

        ///////////////////// Resolutions
        int32_t PicWidthInMbs;
        int32_t FrameHeightInMbs;

        ///////////////////// MMCO
        uint8_t last_has_mmco_5;
        uint8_t curr_has_mmco_5;

        /////////////////// Flags
        uint8_t g_new_frame;
        uint8_t g_new_pic;

        uint8_t structure;
        uint8_t second_field;           // Set to one if this is the second field of a set of paired fields...
        uint8_t field_pic_flag;
        uint8_t last_pic_bottom_field;

        uint8_t bottom_field_flag;
        uint8_t MbaffFrameFlag;
        uint8_t no_output_of_prior_pics_flag;
        uint8_t long_term_reference_flag;

        uint8_t skip_this_pic;
        uint8_t pic_order_cnt_type;
        // Recovery
        uint8_t recovery_point_found;
        uint8_t used_for_reference;
    } h264_img_par;


    typedef struct  _h264_slice_reg_data
    {
        uint32_t h264_bsd_slice_p1;      // 0x150
        //uint32_t h264_mpr_list0[8];       // from 0x380 to 0x3BC
        uint32_t h264_bsd_slice_p2;      // 0x154
        uint32_t h264_bsd_slice_start;   // 0x158

    } h264_slice_data;


    typedef struct  _h264_pic_data
    {
        uint32_t h264_dpb_init;          // 0x40
        //info For current pic
        uint32_t h264_cur_bsd_img_init;      // 0x140
        uint32_t h264_cur_mpr_tf_poc;        // 0x300
        uint32_t h264_cur_mpr_bf_poc;        // 0x304

        //info For framess in DPB
        //uint32_t h264_dpb_bsd_img_init[16];      //0x140
        //uint32_t h264_dpb_mpr_tf_poc[16];        // 0x300
        //uint32_t h264_dpb_mpr_bf_poc[16];        // 0x304
    } h264_pic_data;

    enum h264_workload_item_type
    {
        VIDDEC_WORKLOAD_H264_SLICE_REG = VIDDEC_WORKLOAD_DECODER_SPECIFIC,
        VIDDEC_WORKLOAD_H264_PIC_REG,
        VIDDEC_WORKLOAD_H264_DPB_FRAME_POC,
        VIDDEC_WORKLOAD_H264_SH_BITS_OFFSET,
        VIDDEC_WORKLOAD_H264_PWT_BITS_OFFSET,
        VIDDEC_WORKLOAD_H264_PWT_ES_BYTES,
        VIDDEC_WORKLOAD_H264_SCALING_MATRIX,
        VIDDEC_WORKLOAD_H264_DEBUG
    };



////////////////////////////////////////////
    /* Full Info set*/
////////////////////////////////////////////
    typedef struct _h264_Info
    {

        h264_DecodedPictureBuffer           dpb;

        //// Structures
        //// need to gurantee active_SPS and active_PPS start from 4-bytes alignment address
        seq_param_set_used	active_SPS;
        pic_param_set			active_PPS;


        h264_Slice_Header_t  SliceHeader;
        OldSliceParams       old_slice;
        sei_info             sei_information;

        h264_img_par      img;

        uint32_t          SPS_PADDR_GL;
        uint32_t          PPS_PADDR_GL;
        uint32_t          OFFSET_REF_FRAME_PADDR_GL;
        uint32_t				TMP_OFFSET_REFFRM_PADDR_GL;

        uint32_t          h264_list_replacement;

        uint32_t          h264_pwt_start_byte_offset;
        uint32_t          h264_pwt_start_bit_offset;
        uint32_t          h264_pwt_end_byte_offset;
        uint32_t          h264_pwt_end_bit_offset;
        uint32_t          h264_pwt_enabled;

        uint32_t          sps_valid;

        uint8_t           slice_ref_list0[32];
        uint8_t           slice_ref_list1[32];


        uint8_t           qm_present_list;
        //h264_NAL_Unit_t
        uint8_t           nal_unit_type;
        uint8_t           old_nal_unit_type;
        uint8_t    			got_start;

        //workload
        uint8_t           push_to_cur;
        uint8_t           Is_first_frame_in_stream;
        uint8_t           Is_SPS_updated;
        uint8_t           number_of_first_au_info_nal_before_first_slice;

        uint8_t           is_frame_boundary_detected_by_non_slice_nal;
        uint8_t           is_frame_boundary_detected_by_slice_nal;
        uint8_t           is_current_workload_done;
        uint8_t			 primary_pic_type_plus_one;	  //AUD---[0,7]

        //Error handling
        uint8_t			sei_rp_received;
        uint8_t			last_I_frame_idc;
        uint8_t			sei_b_state_ready;
        uint8_t			gop_err_flag;


        uint32_t		wl_err_curr;
        uint32_t		wl_err_next;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        uint32_t                sw_bail;
#endif
#endif
    } h264_Info;



    struct h264_viddec_parser
    {
        uint32_t     sps_pps_ddr_paddr;
        h264_Info    info;
    };

#ifdef __cplusplus
}
#endif

#ifdef USE_AVC_SHORT_FORMAT
#define MAX_OP  16

typedef struct _dec_ref_pic_marking_t {
    union {
        uint8_t flags;
        struct {
            uint8_t idr_pic_flag:1;
            uint8_t no_output_of_prior_pics_flag:1;
            uint8_t long_term_reference_flag:1;
            uint8_t adaptive_ref_pic_marking_mode_flag:1;
        };
    };
    struct {
        uint8_t memory_management_control_operation;
        union {
            struct {
                uint8_t difference_of_pic_nums_minus1;
            } op1;
            struct {
                uint8_t long_term_pic_num;
            } op2;
            struct {
                uint8_t difference_of_pic_nums_minus1;
                uint8_t long_term_frame_idx;
            } op3;
            struct {
                uint8_t max_long_term_frame_idx_plus1;
            } op4;
            struct {
                uint8_t long_term_frame_idx;
            } op6;
        };
    } op[MAX_OP];
} dec_ref_pic_marking_t;


typedef struct _slice_header_t {
    uint8_t nal_unit_type;
    uint8_t pps_id;
    uint8_t padding;
    union {
        uint8_t flags;
        struct {
            uint8_t field_pic_flag:1;
            uint8_t bottom_field_flag:1;
        };
    };
    uint32_t first_mb_in_slice;
    uint32_t frame_num;
    uint16_t idr_pic_id;
    uint16_t pic_order_cnt_lsb;
    int32_t delta_pic_order_cnt[2];
    int32_t delta_pic_order_cnt_bottom;
} slice_header_t;




typedef struct _vbp_h264_sliceheader {
    slice_header_t          slice_header;
    dec_ref_pic_marking_t   ref_pic_marking;
} vbp_h264_sliceheader;

#endif



#endif  //_H264_H_


