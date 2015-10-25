/*
    This file is provided under a dual BSD/GPLv2 license.  When using or
    redistributing this file, you may do so under either license.

    GPL LICENSE SUMMARY

    Copyright(c) 2007-2009 Intel Corporation. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
    The full GNU General Public License is included in this distribution
    in the file called LICENSE.GPL.

    Contact Information:

    BSD LICENSE

    Copyright(c) 2007-2009 Intel Corporation. All rights reserved.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.
    * Neither the name of Intel Corporation nor the names of its
    contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/
#ifndef VIDDEC_FW_FRAME_ATTR_H
#define VIDDEC_FW_FRAME_ATTR_H

#include "viddec_fw_item_types.h"

#define VIDDEC_PANSCAN_MAX_OFFSETS 4
#define VIDDEC_MAX_CPB_CNT 32

/**
This enumeration lists all the frame types defined by the MPEG, VC1 and H264 specifications.
Frame types applicable to a single codec are specified in the comments.
*/
typedef enum
{
    VIDDEC_FRAME_TYPE_INVALID=0,   /** Unknown type - default value */
    VIDDEC_FRAME_TYPE_IDR=0x1,       /** IDR frame - h264 only */
    VIDDEC_FRAME_TYPE_I=0x2,         /** I frame */
    VIDDEC_FRAME_TYPE_P=0x3,         /** P frame */
    VIDDEC_FRAME_TYPE_B=0x4,         /** B frame */
    VIDDEC_FRAME_TYPE_BI=0x5,        /** BI frame - Intracoded B frame - vc1 only */
    VIDDEC_FRAME_TYPE_SKIP=0x6,      /** Skipped frame - vc1 only */
    VIDDEC_FRAME_TYPE_D=0x7,         /** D frame - mpeg1 only */
    VIDDEC_FRAME_TYPE_S=0x8,         /** SVOP frame - mpeg4 only - sprite encoded frame - treat as P */
    VIDDEC_FRAME_TYPE_MAX,
} viddec_frame_type_t;

/**
This structure contains the content size info extracted from the stream.
*/
typedef struct viddec_rect_size
{
    unsigned int width;
    unsigned int height;
} viddec_rect_size_t;

/**
This structure contains MPEG2 specific pan scan offsets extracted from the stream.
*/
typedef struct viddec_mpeg2_frame_center_offset
{
    int horz;
    int vert;
} viddec_mpeg2_frame_center_offset_t;

/**
This structure contains the MPEG2 specific frame attributes.
*/
typedef struct viddec_mpeg2_frame_attributes
{
    /**
    10 bit unsigned integer corresponding to the display order of each coded picture
    in the stream (or gop if gop header is present).
    Refer to "temporal_reference" of the picture header in ITU-T H.262 Specification.
    */
    unsigned int temporal_ref;

    /**
    Pan/Scan rectangle info
    Refer to the picture display extension in ITU-T H.262 Specification.
    */
    viddec_mpeg2_frame_center_offset_t frame_center_offset[VIDDEC_PANSCAN_MAX_OFFSETS];
    unsigned int number_of_frame_center_offsets;

    /**
    Top-Field first flag
    Refer to "top_field_first" of the picture coding extension in ITU-T H.262 Specification.
    */
    unsigned int top_field_first;

    /**
    Progressive frame flag - Indicates if current frame is progressive or not.
    Refer to "progressive_frame" of the picture coding extension in ITU-T H.262 Specification.
    */
    unsigned int progressive_frame;

    /**
    Frame/field polarity for each coded picture.
    Refer to Table 6-14 in ITU-T H.262 Specification.
    */
    unsigned int picture_struct;

    /**
    Repeat field/frame flag.
    Refer to "repeat_first_field" of the picture coding extension in ITU-T H.262 Specification.
    */
    unsigned int repeat_first_field;


} viddec_mpeg2_frame_attributes_t;

/**
This structure contains MPEG2 specific pan scan offsets extracted from the stream.
*/
typedef struct viddec_vc1_pan_scan_window
{
    unsigned int hoffset;
    unsigned int voffset;
    unsigned int width;
    unsigned int height;
} viddec_vc1_pan_scan_window_t;

/**
This structure contains the VC1 specific frame attributes.
*/
typedef struct viddec_vc1_frame_attributes
{
    /**
    Temporal Reference of frame/field.
    Refer to "TFCNTR" in the picture layer of the SMPTE VC1 Specification.
    */
    unsigned int tfcntr;

    /**
    Frame/field repeat information in the bitstream.
    Refer to "RPTFRM", "TFF", "BFF" in the picture layer
    of the SMPTE VC1 Specification.
    */
    unsigned int rptfrm;
    unsigned int tff;
    unsigned int rff;

    /**
    Pan-scan information in the bitstream.
    Refer to "PANSCAN_FLAG" in the entrypoint layer, "PS_PRESENT", "PS_HOFFSET", "PS_VOFFSET",
    "PS_WIDTH" and "PS_HEIGHT" in the picture layer of the SMPTE VC1 Specification.
    */
    unsigned int panscan_flag;
    unsigned int ps_present;
    unsigned int num_of_pan_scan_windows;
    viddec_vc1_pan_scan_window_t pan_scan_window[VIDDEC_PANSCAN_MAX_OFFSETS];

} viddec_vc1_frame_attributes_t;

/**
This structure contains the H264 specific frame attributes.
*/
typedef struct viddec_h264_frame_attributes
{
    /**
       used_for_reference : 1 means this frame is used as ref frame of others. 0 means no any frame ref to this frame
    */
    unsigned int used_for_reference;
    /**
       Picture Order Count for the current frame/field.
       This value is computed using information from the bitstream.
       Refer to Section 8.2.1, function 8-1 of the ITU-T H.264 Specification.
       These fileds will be supported in future
    */
    int top_field_poc;
    int bottom_field_poc;

    /**
       Display size, which is cropped from content size.
       Currently, the cont_size is cropped, so this paramter is redundant, but in future, cont_size may be changed
    */
    viddec_rect_size_t cropped_size;

    /**
       top_field_first: 0 means bottom_field_POC is smaller than top_field_POC, else 1
    */
    unsigned int top_field_first;

    /**
       field_frame_flag: 0 means all slice of this frame are frame-base encoded, else 1
    */
    unsigned int field_pic_flag;

    /**
       This data type holds view specific information of current frame.
       The following information is packed into this data type:
         view_id(0-9 bits):        Assigned 10 bit value in the encoded stream.
         priority_id(10-15bits):   Assigned 6 bit priority id.
         is_base_view(16th bit):   Flag on true indicates current frame belongs to base view, else dependent view.
     */
#define viddec_fw_h264_mvc_get_view_id(x)              viddec_fw_bitfields_extract( (x)->view_spcific_info, 0, 0x3FF)
#define viddec_fw_h264_mvc_set_view_id(x, val)         viddec_fw_bitfields_insert( (x)->view_spcific_info, val, 0, 0x3FF)
#define viddec_fw_h264_mvc_get_priority_id(x)          viddec_fw_bitfields_extract( (x)->view_spcific_info, 10, 0x3F)
#define viddec_fw_h264_mvc_set_priority_id(x, val)     viddec_fw_bitfields_insert( (x)->view_spcific_info, val, 10, 0x3F)
#define viddec_fw_h264_mvc_get_is_base_view(x)         viddec_fw_bitfields_extract( (x)->view_spcific_info, 16, 0x1)
#define viddec_fw_h264_mvc_set_is_base_view(x, val)    viddec_fw_bitfields_insert( (x)->view_spcific_info, val, 16, 0x1)
    unsigned int view_spcific_info;
} viddec_h264_frame_attributes_t;

/**
This structure contains the MPEG4 specific frame attributes.
*/
typedef struct viddec_mpeg4_frame_attributes
{
    /**
    Top-Field first flag
    Refer to "top_field_first" of the Video Object Plane of the MPEG4 Spec.
    */
    unsigned int top_field_first;

} viddec_mpeg4_frame_attributes_t;

/**
This structure groups all the frame attributes that are exported by the firmware.
The frame attributes are split into attributes that are common to all codecs and
that are specific to codec type.
*/
typedef struct viddec_frame_attributes
{
    /**
    Content size specified in the stream.
    For MPEG2, refer to "horizontal_size_value, vertical_size_value" of the sequence header and
    "horizontal_size_extension, vertical_size_extension" of the sequence extension in ITU-T H.262 Specification.
    For H264, refer to "pic_width_in_mbs_minus1" and "pic_height_in_map_units_minus1" of the
    sequence parameter set in ITU-T H.264 Specification.
    For VC1, refer to "MAX_CODED_WIDTH" and "MAX_CODED_HEIGHT" in the sequence layer,
    "CODED_SIZE_FLAG", "CODED_WIDTH" and "CODED_HEIGHT" in the entrypoint layer of the SMPTE VC1 Specification.
    */
    viddec_rect_size_t cont_size;

    /**
    Type of frame populated in the workload.
    frame_type contains the frame type for progressive frame and the field type for the top field for interlaced frames.
    bottom_field_type contains the field type for the bottom field for interlaced frames.
    For MPEG2, refer to "picture_coding_type" in picture header (Table 6-12) in ITU-T H.262 Specification.
    For H264, refer to "slice_type" in slice header (Table 7-6) in ITU-T H.264 Specification.
    For VC1, refer to "PTYPE" and FPTYPE in the picture layer (Tables 33, 34, 35, 105) in SMPTE VC1 Specification.
    */
    viddec_frame_type_t frame_type;
    viddec_frame_type_t bottom_field_type;

    /** Codec specific attributes */
    union
    {
        viddec_mpeg2_frame_attributes_t  mpeg2;
        viddec_vc1_frame_attributes_t    vc1;
        viddec_h264_frame_attributes_t   h264;
        viddec_mpeg4_frame_attributes_t  mpeg4;
    };

} viddec_frame_attributes_t;

#endif /* VIDDEC_FRAME_ATTR_H */
