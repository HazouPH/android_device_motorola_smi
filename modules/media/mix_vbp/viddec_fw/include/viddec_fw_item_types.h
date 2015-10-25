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
#ifndef VIDDEC_FW_ITEM_TYPES_H
#define VIDDEC_FW_ITEM_TYPES_H


/* The following macros are defined to pack data into 32 bit words.
   mask:    A 32 bit value of N 1 bits starting from lsb where N represents the length of data we are packing.
   start:   Bit start position of data we want.
          ex:  If we want to pack Height(16bits), width(16bits) where width is from (1:16) and height is from (17:32), these are
          the start and mask values for width and height.
          width: start = 0 mask=0xFFFF
          Height:start=  16 mask=0xFFFF

   extract: will extract data from x_32 of unsigned integer type from bit start offset(0 to 31) of length based on mask and returns in
            unsigned integer type.
   insert:  Will insert N bits from val_32 ,where N is length based on mask, into x_32 from bit offset based on start. val_32 is expected to
           be a unsigned int of N bits starting with lsb.
*/

#define viddec_fw_bitfields_extract(x_32, start, mask)     (((x_32) >> (start)) & (mask) )
#define viddec_fw_bitfields_insert(x_32, val_32, start, mask) ((x_32) = (((x_32) & ~( (mask) << (start))) | (((val_32) & (mask)) << (start))))


/* Workload items type. Each item here represents data that Parser detected ex:slice data which
 is used either by host or decoder.*/
typedef enum workload_item_type
{
    VIDDEC_WORKLOAD_INVALID                               =0x0,/* Unknown type */
    VIDDEC_WORKLOAD_PIXEL_ES                              =0x100,/* Slice data tag */
    VIDDEC_WORKLOAD_TAG                                   =0x200,/* Frame association tag */
    VIDDEC_WORKLOAD_USERDATA                              =0x300,/* user data tag */

    VIDDEC_WORKLOAD_IBUF_DONE                             =0x500,/* Es buffer completely used tag */
    VIDDEC_WORKLOAD_IBUF_CONTINUED                        =0x600,/* Es buffer partially used tag */
    VIDDEC_WORKLOAD_IBUF_DISCONTINUITY                    =0x700,/* Discontinuity tag on first workload after discontinuity */
    VIDDEC_WORKLOAD_REFERENCE_FRAME_REORDER               =0x800, /* Reorder frames in DPB tag */
    VIDDEC_WORKLOAD_IBUF_EOS                              =0x900,/* EOS tag on last workload used for current stream */
    VIDDEC_WORKLOAD_SEQUENCE_INFO                         =0xa00,/* MPEG2 Seq Hdr, H264 SPS, VC1 SeqLayer */
    VIDDEC_WORKLOAD_DISPLAY_INFO                          =0xb00,/* MPEG2 Seq Disp Ext, H264 VUI */
    VIDDEC_WORKLOAD_GOP_INFO                              =0xc00,/* MPEG2 GOP, VC1 Entrypoint */
    VIDDEC_WORKLOAD_SEQ_USER_DATA                         =0xd00,/* MPEG2, VC1 Sequence Level User data */
    VIDDEC_WORKLOAD_GOP_USER_DATA                         =0xe00,/* MPEG2, VC1 Gop Level User data */
    VIDDEC_WORKLOAD_FRM_USER_DATA                         =0xf00,/* MPEG2 Picture User data, VC1 Frame User data */

    VIDDEC_WORKLOAD_FLD_USER_DATA                         =0x1000,/* MPEG2, VC1 Field User data */
    VIDDEC_WORKLOAD_SLC_USER_DATA                         =0x1100,/* VC1 Slice User data */
    VIDDEC_WORKLOAD_VISUAL_OBJ_USER_DATA                  =0x1200,/* MPEG4 Visual Object User data */
    VIDDEC_WORKLOAD_VC1_SEQ_HDR_STRUCT_A_C                =0x1200,/* VC1 Only */
    VIDDEC_WORKLOAD_VIDEO_OBJ_USER_DATA                   =0x1300,/* MPEG4 Video Object Layer User data */
    VIDDEC_WORKLOAD_H264_CROPPING                         =0x1400,/* H264 only */
    VIDDEC_WORKLOAD_H264_PAN_SCAN                         =0x1500,/* H264 only */
    VIDDEC_WORKLOAD_SEI_PIC_TIMING                        =0x1600,/* H264 only */
    VIDDEC_WORKLOAD_SEI_PAN_SCAN_RECT                     =0x1700,/* H264 only */
    VIDDEC_WORKLOAD_SEI_USER_DATA_REGISTERED              =0x1800,/* H264 only */
    VIDDEC_WORKLOAD_SEI_USER_DATA_UNREGISTERED            =0x1900,/* H264 only */
    VIDDEC_WORKLOAD_SEI_RECOVERY_POINT                    =0x1a00,/* H264 only */
    VIDDEC_WORKLOAD_MPEG2_SEQ_EXT                         =0x1b00,/* MPEG2 Only - Sequence Extension */
    VIDDEC_WORKLOAD_H264_MVC_SPS_VIEW_IDS                 =0x1c00,/* H264 only */
    VIDDEC_WORKLOAD_MPEG4_VISUAL_SEQ_OBJ                  =0x1d00,/* MPEG4 Only - Visual Sequence */
    VIDDEC_WORKLOAD_MPEG4_VIDEO_OBJ                       =0x1e00,/* MPEG4 Only - Video Object Layer */
    VIDDEC_WORKLOAD_MPEG4_GRP_VIDEO_OBJ                   =0x1f00,/* MPEG4 Only - Group of Video Object Planes */

    VIDDEC_WORKLOAD_MPEG4_VIDEO_PLANE_SHORT               =0x2000,/* MPEG4 Only - Video Plane with Short Header */
    VIDDEC_WORKLOAD_H264_VUI_TIMING_INFO                  =0x2100,/* H264 only */

    VIDDEC_WORKLOAD_REF_FRAME_SOURCE_0                    =0x10000,/* required reference frames tag,last eight bits indicate index in dpb */
    VIDDEC_WORKLOAD_REF_FRAME_RELEASE_0                   =0x20000,/* release frames tag, last eight bits indicate index in dpb*/
    VIDDEC_WORKLOAD_REF_FRAME_DISPLAY_0                   =0x30000,/* Display order in DPB tag, for H264 */
    VIDDEC_WORKLOAD_REF_FRAME_DROPOUT_0                   =0x40000,/* Release frames but not display, for H264 */
    VIDDEC_WORKLOAD_EOS_RELEASE_FRAME_0                   =0x50000,/* Release list while EOS, last eight bits indicate index in dpb */
    VIDDEC_WORKLOAD_EOS_DISPLAY_FRAME_0                   =0x60000,/* Display list while EOS, last eight bits indicate index in dpb */
    VIDDEC_WORKLOAD_DPB_ACTIVE_FRAME_0                    =0x70000,/* required for H264 as it needs whole DPB for each frame */
    VIDDEC_WORKLOAD_H264_REFR_LIST_0                      =0x80000,/* ref list 0 for H264 */
    VIDDEC_WORKLOAD_H264_REFR_LIST_1                      =0x90000,/* ref list 1 for H264 */
    VIDDEC_WORKLOAD_EOS_BEGIN_BOUNDARY                    =0xa0000,/* eos items begin after this */

    VIDDEC_WORKLOAD_DECODER_SPECIFIC                      =0x100000,/* pvt info for decoder tags */
    VIDDEC_WORKLOAD_MAX,
} workload_item_type;

struct h264_witem_sps_mvc_id
{
    /*
      0-9:    num_views_minus1
      10-19:  start index of views in current item.
      20-23:  Number of valid items.
    */
#define viddec_fw_h264_sps_mvc_id_get_num_views_minus1(x)         viddec_fw_bitfields_extract( (x)->num_views, 0, 0x3FF)
#define viddec_fw_h264_sps_mvc_id_set_num_views_minus1(x, val)    viddec_fw_bitfields_insert( (x)->num_views, val, 0, 0x3FF)
#define viddec_fw_h264_sps_mvc_id_get_cur_start_index(x)          viddec_fw_bitfields_extract( (x)->num_views, 10, 0x3FF)
#define viddec_fw_h264_sps_mvc_id_set_cur_start_index(x, val)     viddec_fw_bitfields_insert( (x)->num_views, val, 10, 0x3FF)
#define viddec_fw_h264_sps_mvc_id_get_num_cur_valid_items(x)      viddec_fw_bitfields_extract( (x)->num_views, 20, 0x7)
#define viddec_fw_h264_sps_mvc_id_set_num_cur_valid_items(x, val) viddec_fw_bitfields_insert( (x)->num_views, val, 20, 0x7)
    unsigned int num_views;

    /* We pack six id's into two integers.Each packed_view(integer) contains three 10 bit ids at 0-9, 10-19, 20-29
       These values can be extracted/set using viddec_fw_h264_sps_mvc_id_get_data_frm_index()
       and viddec_fw_h264_sps_mvc_id_set_data_frm_index() functions.
    */
#define viddec_fw_h264_sps_mvc_id_max_packed_ids        6 /* Max number of packed ids in a workload item */
    unsigned int packed_view[2];
};

/* This function extracts a 10 bit view id of index( <6) that was packed into h264_witem_sps_mvc_id structure */
static inline unsigned int viddec_fw_h264_sps_mvc_id_get_data_frm_index(struct h264_witem_sps_mvc_id *data, unsigned int index)
{
    unsigned int start=0, *word;

    start = ((index > 2) ?(index - 3) : index) *10;
    word = &(data->packed_view[(index > 2) ? 1:0]);
    return viddec_fw_bitfields_extract(*word, start, 0x3FF);
}

/* This function packs  a 10 bit view id(val) at index( <6) in h264_witem_sps_mvc_id structure */
static inline void viddec_fw_h264_sps_mvc_id_set_data_frm_index(struct h264_witem_sps_mvc_id *data, unsigned int index, unsigned int val)
{
    unsigned int start=0, *word;

    start = ((index > 2) ?(index - 3) : index) *10;
    word = &(data->packed_view[(index > 2) ? 1:0]);
    viddec_fw_bitfields_insert(*word, val, start, 0x3FF);
}

/* 16-byte workload */
typedef struct viddec_workload_item
{
    enum workload_item_type vwi_type;
    union
    {
        struct
        {
            unsigned int    es_phys_addr;
            unsigned int    es_phys_len;
            unsigned int    es_flags;
        } es;
        struct
        {
            unsigned int    tag_phys_addr;
            unsigned int    tag_phys_len;
            unsigned int    tag_value;
        } tag;
        struct
        {
            unsigned int    data_offset;
            unsigned int    data_payload[2];
        } data;
        struct
        {
            signed int      reference_id;     /* Assigned by parser */
            unsigned int    luma_phys_addr;   /* assigned by host, for DM */
            unsigned int    chroma_phys_addr; /* assigned by host, for DM */
        } ref_frame;
        struct   /* when vwi_type == VIDDEC_WORKLOAD_REFERENCE_FRAME_REORDER */
        {
            signed int      ref_table_offset;    /* Index of first "reordered" */
            /* index from Current[] for Next[offset+0], Ref[offset+1], Ref[offset+2], Ref[offset+3] */
            unsigned int    ref_reorder_00010203;
            /* index from Current[] for Next[offset+4], Ref[offset+5], Ref[offset+6], Ref[offset+7] */
            unsigned int    ref_reorder_04050607;
        } ref_reorder;
        struct
        {
            /* we pack a maximum of 11 bytes of user data and 1 byte for size */
            /* TODO: we can pack 12 bytes and use bottom 8 bits of type to indicate size */
#define viddec_fw_get_user_data_size(x)  ((x)->user_data.size)
#define viddec_fw_get_user_data(x)   (unsigned char *)&((x)->user_data.data_payload[0])
            unsigned char size;
            unsigned char data_payload[11];
            /*
              ITEM TYPES WHICH use this:
              VIDDEC_WORKLOAD_SEI_USER_DATA_UNREGISTERED, VIDDEC_WORKLOAD_SEI_USER_DATA_REGISTERED,
              VIDDEC_WORKLOAD_SEQ_USER_DATA, VIDDEC_WORKLOAD_GOP_USER_DATA,
              VIDDEC_WORKLOAD_FRM_USER_DATA, VIDDEC_WORKLOAD_FLD_USER_DATA, VIDDEC_WORKLOAD_SLC_USER_DATA,
            */
        } user_data;
        struct
        {
            // Sequence Header Item I (From LSB):
            //    - horizontal_size_value    - 12 bits
            //    - vertical_size_value      - 12 bits
            //    - aspect_ratio_information - 4 bits
            //    - frame_rate_code          - 4 bits
#define viddec_fw_mp2_sh_get_horizontal_size_value(x)    viddec_fw_bitfields_extract( (x)->seq_hdr_item_1,  0, 0xFFF)
#define viddec_fw_mp2_sh_get_vertical_size_value(x)      viddec_fw_bitfields_extract( (x)->seq_hdr_item_1, 12, 0xFFF)
#define viddec_fw_mp2_sh_get_aspect_ratio_information(x) viddec_fw_bitfields_extract( (x)->seq_hdr_item_1, 24, 0xF)
#define viddec_fw_mp2_sh_get_frame_rate_code(x)          viddec_fw_bitfields_extract( (x)->seq_hdr_item_1, 28, 0xF)
#define viddec_fw_mp2_sh_set_horizontal_size_value(x, val)    viddec_fw_bitfields_insert ( (x)->seq_hdr_item_1, val,  0, 0xFFF)
#define viddec_fw_mp2_sh_set_vertical_size_value(x, val)      viddec_fw_bitfields_insert ( (x)->seq_hdr_item_1, val, 12, 0xFFF)
#define viddec_fw_mp2_sh_set_aspect_ratio_information(x, val) viddec_fw_bitfields_insert ( (x)->seq_hdr_item_1, val, 24, 0xF)
#define viddec_fw_mp2_sh_set_frame_rate_code(x, val)          viddec_fw_bitfields_insert ( (x)->seq_hdr_item_1, val, 28, 0xF)
            unsigned int seq_hdr_item_1;

            // Sequence Header Item II (From LSB):
            //    - bit_rate_value        - 18 bits
            //    - vbv_buffer_size_value - 10 bits
            //    - remaining pad bits
#define viddec_fw_mp2_sh_get_bit_rate_value(x)        viddec_fw_bitfields_extract( (x)->seq_hdr_item_2,  0, 0x3FFFF)
#define viddec_fw_mp2_sh_get_vbv_buffer_size_value(x) viddec_fw_bitfields_extract( (x)->seq_hdr_item_2, 18, 0x3FF)
#define viddec_fw_mp2_sh_set_bit_rate_value(x, val)        viddec_fw_bitfields_insert ( (x)->seq_hdr_item_2, val,  0, 0x3FFFF)
#define viddec_fw_mp2_sh_set_vbv_buffer_size_value(x, val) viddec_fw_bitfields_insert ( (x)->seq_hdr_item_2, val, 18, 0x3FF)
            unsigned int seq_hdr_item_2;

            unsigned int pad;
        } mp2_sh; // mp2 item of type VIDDEC_WORKLOAD_SEQUENCE_INFO
        struct
        {
            // Sequence Extension Item I (From LSB):
            //    - profile_and_level_indication - 8 bits
            //    - progressive_sequence         - 1 bit
            //    - chroma_format                - 2 bits
            //    - horizontal_size_extension    - 2 bits
            //    - vertical_size_extension      - 2 bits
            //    - bit_rate_extension           - 12 bits
            //    - remaining pad bits
#define viddec_fw_mp2_se_get_profile_and_level_indication(x) viddec_fw_bitfields_extract( (x)->seq_ext_item_1,  0, 0xFF)
#define viddec_fw_mp2_se_get_progressive_sequence(x)         viddec_fw_bitfields_extract( (x)->seq_ext_item_1,  8, 0x1)
#define viddec_fw_mp2_se_get_chroma_format(x)                viddec_fw_bitfields_extract( (x)->seq_ext_item_1,  9, 0x3)
#define viddec_fw_mp2_se_get_horizontal_size_extension(x)    viddec_fw_bitfields_extract( (x)->seq_ext_item_1, 11, 0x3)
#define viddec_fw_mp2_se_get_vertical_size_extension(x)      viddec_fw_bitfields_extract( (x)->seq_ext_item_1, 13, 0x3)
#define viddec_fw_mp2_se_get_bit_rate_extension(x)           viddec_fw_bitfields_extract( (x)->seq_ext_item_1, 15, 0xFFF)
#define viddec_fw_mp2_se_set_profile_and_level_indication(x, val) viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val,  0, 0xFF)
#define viddec_fw_mp2_se_set_progressive_sequence(x, val)         viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val,  8, 0x1)
#define viddec_fw_mp2_se_set_chroma_format(x, val)                viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val,  9, 0x3)
#define viddec_fw_mp2_se_set_horizontal_size_extension(x, val)    viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val, 11, 0x3)
#define viddec_fw_mp2_se_set_vertical_size_extension(x, val)      viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val, 13, 0x3)
#define viddec_fw_mp2_se_set_bit_rate_extension(x, val)           viddec_fw_bitfields_insert ( (x)->seq_ext_item_1, val, 15, 0xFFF)
            unsigned int seq_ext_item_1;

            // Sequence Extension Item II (From LSB):
            //    - vbv_buffer_size_extension - 8 bits
            //    - frame_rate_extension_n    - 2 bits
            //    - frame_rate_extension_d    - 5 bits
            //    - remaining pad bits
#define viddec_fw_mp2_se_get_vbv_buffer_size_extension(x) viddec_fw_bitfields_extract( (x)->seq_ext_item_2,  0, 0xFF)
#define viddec_fw_mp2_se_get_frame_rate_extension_n(x)    viddec_fw_bitfields_extract( (x)->seq_ext_item_2,  8, 0x3)
#define viddec_fw_mp2_se_get_frame_rate_extension_d(x)    viddec_fw_bitfields_extract( (x)->seq_ext_item_2, 10, 0x1F)
#define viddec_fw_mp2_se_set_vbv_buffer_size_extension(x, val) viddec_fw_bitfields_insert ( (x)->seq_ext_item_2, val,  0, 0xFF)
#define viddec_fw_mp2_se_set_frame_rate_extension_n(x, val)    viddec_fw_bitfields_insert ( (x)->seq_ext_item_2, val,  8, 0x3)
#define viddec_fw_mp2_se_set_frame_rate_extension_d(x, val)    viddec_fw_bitfields_insert ( (x)->seq_ext_item_2, val, 10, 0x1F)
            unsigned int seq_ext_item_2;

            unsigned int pad;
        } mp2_se; // mp2 item of type VIDDEC_WORKLOAD_MPEG2_SEQ_EXT
        struct
        {
            // Sequence Display Extension Item I (From LSB):
            //   - display_horizontal_size - 14 bits
            //   - display_vertical_size   - 14 bits
            //   - video_format            - 3 bits
            //   - color_description       - 1 bit
#define viddec_fw_mp2_sde_get_display_horizontal_size(x) viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_1,  0, 0x3FFF)
#define viddec_fw_mp2_sde_get_display_vertical_size(x)   viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_1, 14, 0x3FFF)
#define viddec_fw_mp2_sde_get_video_format(x)            viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_1, 28, 0x7)
#define viddec_fw_mp2_sde_get_color_description(x)       viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_1, 31, 0x1)
#define viddec_fw_mp2_sde_set_display_horizontal_size(x, val) viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_1, val,  0, 0x3FFF)
#define viddec_fw_mp2_sde_set_display_vertical_size(x, val)   viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_1, val, 14, 0x3FFF)
#define viddec_fw_mp2_sde_set_video_format(x, val)            viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_1, val, 28, 0x7)
#define viddec_fw_mp2_sde_set_color_description(x, val)       viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_1, val, 31, 0x1)
            unsigned int seq_disp_ext_item_1;

            // Sequence Display Extension II (From LSB):
            //   - color_primaries - 8 bits
            //   - transfer_characteristics - 8 bits
            //   - remaining pad bits
#define viddec_fw_mp2_sde_get_color_primaries(x)          viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_2,  0, 0xFF)
#define viddec_fw_mp2_sde_get_transfer_characteristics(x) viddec_fw_bitfields_extract( (x)->seq_disp_ext_item_2,  8, 0xFF)
#define viddec_fw_mp2_sde_set_color_primaries(x, val)          viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_2, val,  0, 0xFF)
#define viddec_fw_mp2_sde_set_transfer_characteristics(x, val) viddec_fw_bitfields_insert ( (x)->seq_disp_ext_item_2, val,  8, 0xFF)
            unsigned int seq_disp_ext_item_2;

            unsigned int pad;
        } mp2_sde; // mp2 item of type VIDDEC_WORKLOAD_DISPLAY_INFO
        struct
        {
            // Group of Pictures Header Item I (From LSB):
            //   - closed_gop - 1 bit
            //   - broken_link - 1 bit
            //   - remaining pad bits
#define viddec_fw_mp2_gop_get_closed_gop(x)  viddec_fw_bitfields_extract( (x)->gop_hdr_item_1,  0, 0x1)
#define viddec_fw_mp2_gop_get_broken_link(x) viddec_fw_bitfields_extract( (x)->gop_hdr_item_1,  1, 0x1)
#define viddec_fw_mp2_gop_set_closed_gop(x, val)  viddec_fw_bitfields_insert ( (x)->gop_hdr_item_1, val,  0, 0x1)
#define viddec_fw_mp2_gop_set_broken_link(x, val) viddec_fw_bitfields_insert ( (x)->gop_hdr_item_1, val,  1, 0x1)
            unsigned int gop_hdr_item_1;

            unsigned int pad1;
            unsigned int pad2;
        } mp2_gop; // mp2 item of type VIDDEC_WORKLOAD_GOP_INFO
        struct
        {
#define viddec_fw_vc1_get_profile(x) viddec_fw_bitfields_extract((x)->size, 30, 0x3)
#define viddec_fw_vc1_set_profile(x, val) viddec_fw_bitfields_insert((x)->size, val, 30, 0x3)

#define viddec_fw_vc1_get_level(x) viddec_fw_bitfields_extract((x)->size, 27, 0x7)
#define viddec_fw_vc1_set_level(x, val) viddec_fw_bitfields_insert((x)->size, val, 27, 0x7)

#define viddec_fw_vc1_get_colordiff_format(x) viddec_fw_bitfields_extract((x)->size, 25, 0x3)
#define viddec_fw_vc1_set_colordiff_format(x, val) viddec_fw_bitfields_insert((x)->size, val, 25, 0x3)

#define viddec_fw_vc1_get_pulldown(x) viddec_fw_bitfields_extract((x)->size, 24, 0x1)
#define viddec_fw_vc1_set_pulldown(x, val) viddec_fw_bitfields_insert((x)->size, val, 24, 0x1)

#define viddec_fw_vc1_get_max_coded_width(x) viddec_fw_bitfields_extract((x)->size, 12, 0xFFF)
#define viddec_fw_vc1_set_max_coded_width(x, val) viddec_fw_bitfields_insert((x)->size, val, 12, 0xFFF)

#define viddec_fw_vc1_get_max_coded_height(x) viddec_fw_bitfields_extract((x)->size, 0, 0xFFF)
#define viddec_fw_vc1_set_max_coded_height(x, val) viddec_fw_bitfields_insert((x)->size, val, 0, 0xFFF)

#define viddec_fw_vc1_get_bitrtq_postproc(x) viddec_fw_bitfields_extract((x)->flags, 8, 0x1F)
#define viddec_fw_vc1_set_bitrtq_postproc(x, val) viddec_fw_bitfields_insert((x)->flags, val, 8, 0x1F)

#define viddec_fw_vc1_get_frmrtq_postproc(x) viddec_fw_bitfields_extract((x)->flags, 5, 0x7)
#define viddec_fw_vc1_set_frmrtq_postproc(x, val) viddec_fw_bitfields_insert((x)->flags, val, 5, 0x7)

#define viddec_fw_vc1_get_interlace(x) viddec_fw_bitfields_extract((x)->flags, 4, 0x1)
#define viddec_fw_vc1_set_interlace(x, val) viddec_fw_bitfields_insert((x)->flags, val, 4, 0x1)

#define viddec_fw_vc1_get_tfcntrflag(x) viddec_fw_bitfields_extract((x)->flags, 3, 0x1)
#define viddec_fw_vc1_set_tfcntrflag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 3, 0x1)

#define viddec_fw_vc1_get_finterpflag(x) viddec_fw_bitfields_extract((x)->flags, 2, 0x1)
#define viddec_fw_vc1_set_finterpflag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 2, 0x1)

#define viddec_fw_vc1_get_psf(x) viddec_fw_bitfields_extract((x)->flags, 1, 0x1)
#define viddec_fw_vc1_set_psf(x, val) viddec_fw_bitfields_insert((x)->flags, val, 1, 0x1)

#define viddec_fw_vc1_get_display_ext(x) viddec_fw_bitfields_extract((x)->flags, 0, 0x1)
#define viddec_fw_vc1_set_display_ext(x, val) viddec_fw_bitfields_insert((x)->flags, val, 0, 0x1)

            uint32_t size;    // profile:2, level:3, colordiff_format:2, pulldown:1, max_coded_width:12, max_coded_height:12
            uint32_t flags;   // bitrtq_postproc:5, frmrtq_postproc:3, interlace:1, tfcntrflag:1, finterpflag:1, psf:1, display_ext:1
            uint32_t pad;
        } vc1_sl; // vc1 item of type VIDDEC_WORKLOAD_SEQUENCE_INFO
        struct
        {
            // This item is populated when display_ext flag is set in the sequence layer
            // therefore, no need to provide this flag
#define viddec_fw_vc1_get_disp_horiz_size(x) viddec_fw_bitfields_extract((x)->size, 18, 0x3FFF)
#define viddec_fw_vc1_set_disp_horiz_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 18, 0x3FFF)

#define viddec_fw_vc1_get_disp_vert_size(x) viddec_fw_bitfields_extract((x)->size, 4, 0x3FFF)
#define viddec_fw_vc1_set_disp_vert_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 4, 0x3FFF)

#define viddec_fw_vc1_get_disp_aspect_ratio_flag(x) viddec_fw_bitfields_extract((x)->size, 3, 0x1)
#define viddec_fw_vc1_set_disp_aspect_ratio_flag(x, val) viddec_fw_bitfields_insert((x)->size, val, 3, 0x1)

#define viddec_fw_vc1_get_disp_color_format_flag(x) viddec_fw_bitfields_extract((x)->size, 2, 0x1)
#define viddec_fw_vc1_set_disp_color_format_flag(x, val) viddec_fw_bitfields_insert((x)->size, val, 2, 0x1)

#define viddec_fw_vc1_get_disp_framerate_flag(x) viddec_fw_bitfields_extract((x)->size, 1, 0x1)
#define viddec_fw_vc1_set_disp_framerate_flag(x, val) viddec_fw_bitfields_insert((x)->size, val, 1, 0x1)

#define viddec_fw_vc1_get_disp_framerateind(x) viddec_fw_bitfields_extract((x)->size, 0, 0x1)
#define viddec_fw_vc1_set_disp_framerateind(x, val) viddec_fw_bitfields_insert((x)->size, val, 0, 0x1)

#define viddec_fw_vc1_get_disp_aspect_ratio(x) viddec_fw_bitfields_extract((x)->framerate, 28, 0xF)
#define viddec_fw_vc1_set_disp_aspect_ratio(x, val) viddec_fw_bitfields_insert((x)->framerate, val, 28, 0xF)

#define viddec_fw_vc1_get_disp_frameratenr(x) viddec_fw_bitfields_extract((x)->framerate, 20, 0xFF)
#define viddec_fw_vc1_set_disp_frameratenr(x, val) viddec_fw_bitfields_insert((x)->framerate, val, 20, 0xFF)

#define viddec_fw_vc1_get_disp_frameratedr(x) viddec_fw_bitfields_extract((x)->framerate, 16, 0xF)
#define viddec_fw_vc1_set_disp_frameratedr(x, val) viddec_fw_bitfields_insert((x)->framerate, val, 16, 0xF)

#define viddec_fw_vc1_get_disp_framerateexp(x) viddec_fw_bitfields_extract((x)->framerate, 0, 0xFFFF)
#define viddec_fw_vc1_set_disp_framerateexp(x, val) viddec_fw_bitfields_insert((x)->framerate, val, 0, 0xFFFF)

#define viddec_fw_vc1_get_disp_aspect_ratio_horiz_size(x) viddec_fw_bitfields_extract((x)->aspectsize, 24, 0xFF)
#define viddec_fw_vc1_set_disp_aspect_ratio_horiz_size(x, val) viddec_fw_bitfields_insert((x)->aspectsize, val, 24, 0xFF)

#define viddec_fw_vc1_get_disp_aspect_ratio_vert_size(x) viddec_fw_bitfields_extract((x)->aspectsize, 16, 0xFF)
#define viddec_fw_vc1_set_disp_aspect_ratio_vert_size(x, val) viddec_fw_bitfields_insert((x)->aspectsize, val, 16, 0xFF)

#define viddec_fw_vc1_get_disp_color_prim(x) viddec_fw_bitfields_extract((x)->aspectsize, 8, 0xFF)
#define viddec_fw_vc1_set_disp_color_prim(x, val) viddec_fw_bitfields_insert((x)->aspectsize, val, 8, 0xFF)

#define viddec_fw_vc1_get_disp_transfer_char(x) viddec_fw_bitfields_extract((x)->aspectsize, 0, 0xFF)
#define viddec_fw_vc1_set_disp_transfer_char(x, val) viddec_fw_bitfields_insert((x)->aspectsize, val, 0, 0xFF)

            uint32_t size; // disp_horiz_size:14, disp_vert_size:14, aspect_ratio_flag:1, color_format_flag:1, framerate_flag:1, framerateind:1
            uint32_t framerate;  // aspect_ratio:4, frameratenr:8, frameratedr:4, framerateexp:16
            uint32_t aspectsize; // aspect_ratio_horiz_size:8, aspect_ratio_vert_size:8, color_prim:8, transfer_char:8
        } vc1_sl_de; // vc1 item of type VIDDEC_WORKLOAD_DISPLAY_INFO
        struct
        {
#define viddec_fw_vc1_get_rcv_horiz_size(x) viddec_fw_bitfields_extract((x)->size, 16, 0xFFFF)
#define viddec_fw_vc1_set_rcv_horiz_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 16, 0xFFFF)

#define viddec_fw_vc1_get_rcv_vert_size(x) viddec_fw_bitfields_extract((x)->size, 0, 0xFFFF)
#define viddec_fw_vc1_set_rcv_vert_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 0, 0xFFFF)

#define viddec_fw_vc1_get_rcv_bitrtq_postproc(x) viddec_fw_bitfields_extract((x)->flags, 16, 0x1F)
#define viddec_fw_vc1_set_rcv_bitrtq_postproc(x, val) viddec_fw_bitfields_insert((x)->flags, val, 16, 0x1F)

#define viddec_fw_vc1_get_rcv_frmrtq_postproc(x) viddec_fw_bitfields_extract((x)->flags, 13, 0x7)
#define viddec_fw_vc1_set_rcv_frmrtq_postproc(x, val) viddec_fw_bitfields_insert((x)->flags, val, 13, 0x7)

#define viddec_fw_vc1_get_rcv_profile(x) viddec_fw_bitfields_extract((x)->flags, 9, 0xF)
#define viddec_fw_vc1_set_rcv_profile(x, val) viddec_fw_bitfields_insert((x)->flags, val, 9, 0xF)

#define viddec_fw_vc1_get_rcv_level(x) viddec_fw_bitfields_extract((x)->flags, 6, 0x7)
#define viddec_fw_vc1_set_rcv_level(x, val) viddec_fw_bitfields_insert((x)->flags, val, 6, 0x7)

#define viddec_fw_vc1_get_rcv_cbr(x) viddec_fw_bitfields_extract((x)->flags, 5, 0x1)
#define viddec_fw_vc1_set_rcv_cbr(x, val) viddec_fw_bitfields_insert((x)->flags, val, 5, 0x1)

#define viddec_fw_vc1_get_rcv_rangered(x) viddec_fw_bitfields_extract((x)->flags, 4, 0x1)
#define viddec_fw_vc1_set_rcv_rangered(x, val) viddec_fw_bitfields_insert((x)->flags, val, 4, 0x1)

#define viddec_fw_vc1_get_rcv_maxbframes(x) viddec_fw_bitfields_extract((x)->flags, 1, 0x7)
#define viddec_fw_vc1_set_rcv_maxbframes(x, val) viddec_fw_bitfields_insert((x)->flags, val, 1, 0x7)

#define viddec_fw_vc1_get_rcv_finterpflag(x) viddec_fw_bitfields_extract((x)->flags, 0, 0x1)
#define viddec_fw_vc1_set_rcv_finterpflag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 0, 0x1)

            uint32_t size;    // horiz_size:16, vert_size:16
            uint32_t flags;   // bitrtq_postproc:5, frmrtq_postproc:3, profile:4, level:3, cbr:1, rangered:1, maxbframes:3, finterpflag:1
            uint32_t pad;
        } vc1_sh_struct_a_c; // vc1 item of type VIDDEC_WORKLOAD_VC1_SEQ_HDR_STRUCT_A_C
        struct
        {
#define viddec_fw_vc1_get_ep_size_flag(x) viddec_fw_bitfields_extract((x)->size, 24, 0x1)
#define viddec_fw_vc1_set_ep_size_flag(x, val) viddec_fw_bitfields_insert((x)->size, val, 24, 0x1)

#define viddec_fw_vc1_get_ep_horiz_size(x) viddec_fw_bitfields_extract((x)->size, 12, 0xFFF)
#define viddec_fw_vc1_set_ep_horiz_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 12, 0xFFF)

#define viddec_fw_vc1_get_ep_vert_size(x) viddec_fw_bitfields_extract((x)->size, 0, 0xFFF)
#define viddec_fw_vc1_set_ep_vert_size(x, val) viddec_fw_bitfields_insert((x)->size, val, 0, 0xFFF)

#define viddec_fw_vc1_get_ep_broken_link(x) viddec_fw_bitfields_extract((x)->flags, 10, 0x1)
#define viddec_fw_vc1_set_ep_broken_link(x, val) viddec_fw_bitfields_insert((x)->flags, val, 10, 0x1)

#define viddec_fw_vc1_get_ep_closed_entry(x) viddec_fw_bitfields_extract((x)->flags, 9, 0x1)
#define viddec_fw_vc1_set_ep_closed_entry(x, val) viddec_fw_bitfields_insert((x)->flags, val, 9, 0x1)

#define viddec_fw_vc1_get_ep_panscan_flag(x) viddec_fw_bitfields_extract((x)->flags, 8, 0x1)
#define viddec_fw_vc1_set_ep_panscan_flag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 8, 0x1)

#define viddec_fw_vc1_get_ep_range_mapy_flag(x) viddec_fw_bitfields_extract((x)->flags, 7, 0x1)
#define viddec_fw_vc1_set_ep_range_mapy_flag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 7, 0x1)

#define viddec_fw_vc1_get_ep_range_mapy(x) viddec_fw_bitfields_extract((x)->flags, 4, 0x7)
#define viddec_fw_vc1_set_ep_range_mapy(x, val) viddec_fw_bitfields_insert((x)->flags, val, 4, 0x7)

#define viddec_fw_vc1_get_ep_range_mapuv_flag(x) viddec_fw_bitfields_extract((x)->flags, 3, 0x1)
#define viddec_fw_vc1_set_ep_range_mapuv_flag(x, val) viddec_fw_bitfields_insert((x)->flags, val, 3, 0x1)

#define viddec_fw_vc1_get_ep_range_mapuv(x) viddec_fw_bitfields_extract((x)->flags, 0, 0x7)
#define viddec_fw_vc1_set_ep_range_mapuv(x, val) viddec_fw_bitfields_insert((x)->flags, val, 0, 0x7)

            uint32_t size;    // coded_size_flag:1, coded_width:12, coded_height:12
            uint32_t flags;   // broken_link:1, closed_entry:1, panscan_flag:1, range_mapy_flag:1, range_mapy:3, range_mapuv_flag:1, range_mapuv:3
            uint32_t pad;
        } vc1_ep; // vc1 item of type VIDDEC_WORKLOAD_GOP_INFO
        struct
        {
            /*
              0-7 bits for profile_idc.
              8-15 bits for level_idc.
              16-17 bits for chroma_format_idc.
              18-22 bits for num_ref_frames.
              23  for gaps_in_frame_num_value_allowed_flag.
              24 for frame_mbs_only_flag.
              25 for frame_cropping_flag.
              26 for vui_parameters_present_flag.
             */
#define viddec_fw_h264_sps_get_profile_idc(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 0, 0xFF)
#define viddec_fw_h264_sps_set_profile_idc(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 0, 0xFF)
#define viddec_fw_h264_sps_get_level_idc(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 8, 0xFF)
#define viddec_fw_h264_sps_set_level_idc(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 8, 0xFF)
#define viddec_fw_h264_sps_get_chroma_format_idc(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 16, 0x3)
#define viddec_fw_h264_sps_set_chroma_format_idc(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 16, 0x3)
#define viddec_fw_h264_sps_get_num_ref_frames(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 18, 0x1F)
#define viddec_fw_h264_sps_set_num_ref_frames(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 18, 0x1F)
#define viddec_fw_h264_sps_get_gaps_in_frame_num_value_allowed_flag(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 23, 0x1)
#define viddec_fw_h264_sps_set_gaps_in_frame_num_value_allowed_flag(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 23, 0x1)
#define viddec_fw_h264_sps_get_frame_mbs_only_flag(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 24, 0x1)
#define viddec_fw_h264_sps_set_frame_mbs_only_flag(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 24, 0x1)
#define viddec_fw_h264_sps_get_frame_cropping_flag(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 25, 0x1)
#define viddec_fw_h264_sps_set_frame_cropping_flag(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 25, 0x1)
#define viddec_fw_h264_sps_get_vui_parameters_present_flag(x)  viddec_fw_bitfields_extract( (x)->sps_messages, 26, 0x1)
#define viddec_fw_h264_sps_set_vui_parameters_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->sps_messages, val, 26, 0x1)
            unsigned int sps_messages;
            unsigned int pic_width_in_mbs_minus1;
            unsigned int pic_height_in_map_units_minus1;
        } h264_sps; // h264 item of type VIDDEC_WORKLOAD_SEQUENCE_INFO

        struct h264_witem_sps_mvc_id h264_sps_mvc_id;

        struct
        {
#define viddec_fw_h264_cropping_get_left(x)  viddec_fw_bitfields_extract( (x)->left_right, 16, 0xFFFF)
#define viddec_fw_h264_cropping_get_right(x) viddec_fw_bitfields_extract( (x)->left_right, 0, 0xFFFF)
#define viddec_fw_h264_cropping_set_left(x, val)  viddec_fw_bitfields_insert( (x)->left_right, val, 16, 0xFFFF)
#define viddec_fw_h264_cropping_set_right(x, val) viddec_fw_bitfields_insert( (x)->left_right, val, 0, 0xFFFF)
            unsigned int left_right; /* Left in upper 16 bits and right in Lower 16 bits */
#define viddec_fw_h264_cropping_get_top(x)  viddec_fw_bitfields_extract( (x)->top_bottom, 16, 0xFFFF)
#define viddec_fw_h264_cropping_get_bottom(x) viddec_fw_bitfields_extract( (x)->top_bottom, 0, 0xFFFF)
#define viddec_fw_h264_cropping_set_top(x, val)  viddec_fw_bitfields_insert( (x)->top_bottom, val, 16, 0xFFFF)
#define viddec_fw_h264_cropping_set_bottom(x, val) viddec_fw_bitfields_insert( (x)->top_bottom, val, 0, 0xFFFF)
            unsigned int top_bottom; /* top in upper 16 bits and bottom in lower 16 bits */
            unsigned int pad;
        } h264_cropping; // h264 item of type VIDDEC_WORKLOAD_H264_CROPPING

        struct
        {
            /* 0 bit for aspect_ratio_info_present_flag
               1 st bit for video_signal_type_present_flag
               2 nd bit for colour_description_present_flag
               3 rd bit for timing_info_present_flag
               4 th bit for nal_hrd_parameters_present_flag
               5 th bit for vcl_hrd_parameters_present_flag
               6 th bit for fixed_frame_rate_flag
               7 th bit for pic_struct_present_flag
               8 th bit for low_delay_hrd_flag
               9,10,11 bits for video_format
            */
#define viddec_fw_h264_vui_get_aspect_ratio_info_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 0, 0x1)
#define viddec_fw_h264_vui_set_aspect_ratio_info_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 0, 0x1)
#define viddec_fw_h264_vui_get_video_signal_type_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 1, 0x1)
#define viddec_fw_h264_vui_set_video_signal_type_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 1, 0x1)
#define viddec_fw_h264_vui_get_colour_description_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 2, 0x1)
#define viddec_fw_h264_vui_set_colour_description_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 2, 0x1)
#define viddec_fw_h264_vui_get_timing_info_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 3, 0x1)
#define viddec_fw_h264_vui_set_timing_info_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 3, 0x1)
#define viddec_fw_h264_vui_get_nal_hrd_parameters_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 4, 0x1)
#define viddec_fw_h264_vui_set_nal_hrd_parameters_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 4, 0x1)
#define viddec_fw_h264_vui_get_vcl_hrd_parameters_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 5, 0x1)
#define viddec_fw_h264_vui_set_vcl_hrd_parameters_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 5, 0x1)
#define viddec_fw_h264_vui_get_fixed_frame_rate_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 6, 0x1)
#define viddec_fw_h264_vui_set_fixed_frame_rate_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 6, 0x1)
#define viddec_fw_h264_vui_get_pic_struct_present_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 7, 0x1)
#define viddec_fw_h264_vui_set_pic_struct_present_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 7, 0x1)
#define viddec_fw_h264_vui_get_low_delay_hrd_flag(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 8, 0x1)
#define viddec_fw_h264_vui_set_low_delay_hrd_flag(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 8, 0x1)
#define viddec_fw_h264_vui_get_video_format(x)  viddec_fw_bitfields_extract( (x)->vui_flags_and_format, 9, 0x7)
#define viddec_fw_h264_vui_set_video_format(x, val)  viddec_fw_bitfields_insert( (x)->vui_flags_and_format, val, 9, 0x7)
            unsigned int vui_flags_and_format;

#define viddec_fw_h264_vui_get_aspect_ratio_idc(x)  viddec_fw_bitfields_extract( (x)->aspc_color_transfer, 0, 0xFF)
#define viddec_fw_h264_vui_set_aspect_ratio_idc(x, val)  viddec_fw_bitfields_insert( (x)->aspc_color_transfer, val, 0, 0xFF)
#define viddec_fw_h264_vui_get_colour_primaries(x)  viddec_fw_bitfields_extract( (x)->aspc_color_transfer, 8, 0xFF)
#define viddec_fw_h264_vui_set_colour_primaries(x, val)  viddec_fw_bitfields_insert( (x)->aspc_color_transfer, val, 8, 0xFF)
#define viddec_fw_h264_vui_get_transfer_characteristics(x)  viddec_fw_bitfields_extract( (x)->aspc_color_transfer, 16, 0xFF)
#define viddec_fw_h264_vui_set_transfer_characteristics(x, val)  viddec_fw_bitfields_insert( (x)->aspc_color_transfer, val, 16, 0xFF)
            /* lower 8 bits for aspect_ratio, next 8bits for color primaries and next 8 bits for transfer characteristics */
            unsigned int aspc_color_transfer;

#define viddec_fw_h264_vui_get_sar_width(x)  viddec_fw_bitfields_extract( (x)->sar_width_height, 16, 0xFFFF)
#define viddec_fw_h264_vui_get_sar_height(x) viddec_fw_bitfields_extract( (x)->sar_width_height, 0, 0xFFFF)
#define viddec_fw_h264_vui_set_sar_width(x, val)  viddec_fw_bitfields_insert( (x)->sar_width_height, val, 16, 0xFFFF)
#define viddec_fw_h264_vui_set_sar_height(x, val) viddec_fw_bitfields_insert( (x)->sar_width_height, val, 0, 0xFFFF)
            unsigned int sar_width_height; /* Lower 16 for height upper 16 for width */
        } h264_vui; // h264 item of type VIDDEC_WORKLOAD_DISPLAY_INFO
        struct
        {
#define viddec_fw_h264_vui_get_num_units_in_tick_flag(x)  viddec_fw_bitfields_extract( (x)->num_units_in_tick, 0, 0xFFFFFFFF)
#define viddec_fw_h264_vui_set_num_units_in_tick_flag(x, val)  viddec_fw_bitfields_insert( (x)->num_units_in_tick, val, 0, 0xFFFFFFFF)
#define viddec_fw_h264_vui_get_time_scale_flag(x)  viddec_fw_bitfields_extract( (x)->time_scale, 0, 0xFFFFFFFF)
#define viddec_fw_h264_vui_set_time_scale_flag(x, val)  viddec_fw_bitfields_insert( (x)->time_scale, val, 0, 0xFFFFFFFF)
            unsigned int num_units_in_tick;
            unsigned int time_scale;
            unsigned int pad1;
        } h264_vui_time_info; // VIDDEC_WORKLOAD_H264_VUI_TIMING_INFO
        struct
        {
            unsigned int pic_struct; /* 4 bit length */
            unsigned int pad1;
            unsigned int pad2;
        } h264_sei_pic_timing; // h264 item of type VIDDEC_WORKLOAD_SEI_PIC_TIMING
        struct
        {
            unsigned int pan_scan_rect_id;

#define viddec_fw_h264_sei_pan_scan_get_cancel_flag(x)  viddec_fw_bitfields_extract( (x)->pan_scan_cancel_and_cnt, 0, 0x1)
#define viddec_fw_h264_sei_pan_scan_get_cnt_minus1(x) viddec_fw_bitfields_extract( (x)->pan_scan_cancel_and_cnt, 1, 0x3)
#define viddec_fw_h264_sei_pan_scan_set_cancel_flag(x, val)  viddec_fw_bitfields_insert( (x)->pan_scan_cancel_and_cnt, val, 0, 0x1)
#define viddec_fw_h264_sei_pan_scan_set_cnt_minus1(x, val) viddec_fw_bitfields_insert( (x)->pan_scan_cancel_and_cnt, val, 1, 0x3)
            unsigned int pan_scan_cancel_and_cnt; /* 0 bit for cancel flag and 2 bits for cnt_minus1 */
            unsigned int pan_scan_rect_repetition_period;
        } h264_sei_pan_scan; // h264 item of type VIDDEC_WORKLOAD_H264_PAN_SCAN

        struct
        {

#define viddec_fw_h264_pan_scan_get_left(x)  viddec_fw_bitfields_extract( (x)->left_right, 16, 0xFFFF)
#define viddec_fw_h264_pan_scan_get_right(x) viddec_fw_bitfields_extract( (x)->left_right, 0, 0xFFFF)
#define viddec_fw_h264_pan_scan_set_left(x, val)  viddec_fw_bitfields_insert( (x)->left_right, val, 16, 0xFFFF)
#define viddec_fw_h264_pan_scan_set_right(x, val) viddec_fw_bitfields_insert( (x)->left_right, val, 0, 0xFFFF)
            unsigned int left_right; /* Left in upper 16 bits and right in Lower 16 bits */

#define viddec_fw_h264_pan_scan_get_top(x)  viddec_fw_bitfields_extract( (x)->top_bottom, 16, 0xFFFF)
#define viddec_fw_h264_pan_scan_get_bottom(x) viddec_fw_bitfields_extract( (x)->top_bottom, 0, 0xFFFF)
#define viddec_fw_h264_pan_scan_set_top(x, val)  viddec_fw_bitfields_insert( (x)->top_bottom, val, 16, 0xFFFF)
#define viddec_fw_h264_pan_scan_set_bottom(x, val) viddec_fw_bitfields_insert( (x)->top_bottom, val, 0, 0xFFFF)
            unsigned int top_bottom; /* top in upper 16 bits and bottom in lower 16 bits */

            unsigned int pad;
        } h264_pan_scan_rect; // h264 item of type VIDDEC_WORKLOAD_SEI_PAN_SCAN_RECT
        struct
        {
            unsigned int recovery_frame_cnt;
#define viddec_fw_h264_h264_sei_recovery_get_exact_match_flag(x)  viddec_fw_bitfields_extract( (x)->broken_and_exctmatch_flags, 0, 0x1)
#define viddec_fw_h264_h264_sei_recovery_get_broken_link_flag(x) viddec_fw_bitfields_extract( (x)->broken_and_exctmatch_flags, 1, 0x1)
#define viddec_fw_h264_h264_sei_recovery_set_exact_match_flag(x, val)  viddec_fw_bitfields_insert( (x)->broken_and_exctmatch_flags, val, 0, 0x1)
#define viddec_fw_h264_h264_sei_recovery_set_broken_link_flag(x, val) viddec_fw_bitfields_insert( (x)->broken_and_exctmatch_flags, val, 1, 0x1)
            unsigned int broken_and_exctmatch_flags; /* 0 bit for exact match, 1 bit for brokenlink */

            unsigned int changing_slice_group_idc; /* 2bit value for slice_group idc */

        } h264_sei_recovery_point; // h264 item of type VIDDEC_WORKLOAD_SEI_RECOVERY_POINT


        struct
        {
            // Visual Sequence (From LSB):
            //   - profile_and_level_indication - 8 bits
#define viddec_fw_mp4_vs_get_profile_and_level_indication(x)      viddec_fw_bitfields_extract( (x)->vs_item,  0, 0xFF)
#define viddec_fw_mp4_vs_set_profile_and_level_indication(x, val) viddec_fw_bitfields_insert ( (x)->vs_item, val,  0, 0xFF)
            unsigned int vs_item;

            // Visual Object - video_signal_type
            //   - video_signal_type - 1b
            //   - video_format - 3b
            //   - video_range - 1b
            //   - colour_description - 1b
#define viddec_fw_mp4_vo_get_colour_description(x)      viddec_fw_bitfields_extract( (x)->video_signal_type, 5, 0x1)
#define viddec_fw_mp4_vo_set_colour_description(x, val) viddec_fw_bitfields_insert ( (x)->video_signal_type, val, 5, 0x1)
#define viddec_fw_mp4_vo_get_video_range(x)      viddec_fw_bitfields_extract( (x)->video_signal_type, 4, 0x1)
#define viddec_fw_mp4_vo_set_video_range(x, val) viddec_fw_bitfields_insert ( (x)->video_signal_type, val, 4, 0x1)
#define viddec_fw_mp4_vo_get_video_format(x)      viddec_fw_bitfields_extract( (x)->video_signal_type,  1, 0x7)
#define viddec_fw_mp4_vo_set_video_format(x, val) viddec_fw_bitfields_insert ( (x)->video_signal_type, val,  1, 0x7)
#define viddec_fw_mp4_vo_get_video_signal_type(x)      viddec_fw_bitfields_extract( (x)->video_signal_type,  0, 0x1)
#define viddec_fw_mp4_vo_set_video_signal_type(x, val) viddec_fw_bitfields_insert ( (x)->video_signal_type, val,  0, 0x1)
            unsigned int video_signal_type;

            // Visual Object - video_signal_type
            //   - color_primaries - 8 bits
            //   - transfer_characteristics - 8 bits
#define viddec_fw_mp4_vo_get_transfer_char(x)      viddec_fw_bitfields_extract( (x)->color_desc,  8, 0xFF)
#define viddec_fw_mp4_vo_set_transfer_char(x, val) viddec_fw_bitfields_insert ( (x)->color_desc, val,  8, 0xFF)
#define viddec_fw_mp4_vo_get_color_primaries(x)      viddec_fw_bitfields_extract( (x)->color_desc,  0, 0xFF)
#define viddec_fw_mp4_vo_set_color_primaries(x, val) viddec_fw_bitfields_insert ( (x)->color_desc, val,  0, 0xFF)
            unsigned int color_desc;
        } mp4_vs_vo; // mp4 item of type VIDDEC_WORKLOAD_MPEG4_VISUAL_SEQ_OBJ

        struct
        {
            // Video Object Layer(From LSB):
            //   - aspect_ratio_info - 4b
            //   - par_width - 8b
            //   - par_height - 8b
            //   - vol_control_param - 1b
            //   - chroma_format - 2b
            //   - interlaced - 1b
            //   - fixed_vop_rate - 1b
#define viddec_fw_mp4_vol_get_fixed_vop_rate(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 24, 0x1)
#define viddec_fw_mp4_vol_set_fixed_vop_rate(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 24, 0x1)
#define viddec_fw_mp4_vol_get_interlaced(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 23, 0x1)
#define viddec_fw_mp4_vol_set_interlaced(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 23, 0x1)
#define viddec_fw_mp4_vol_get_chroma_format(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 21, 0x3)
#define viddec_fw_mp4_vol_set_chroma_format(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 21, 0x3)
#define viddec_fw_mp4_vol_get_control_param(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 20, 0x1)
#define viddec_fw_mp4_vol_set_control_param(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 20, 0x1)
#define viddec_fw_mp4_vol_get_par_height(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 12, 0xFF)
#define viddec_fw_mp4_vol_set_par_height(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 12, 0xFF)
#define viddec_fw_mp4_vol_get_par_width(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 4, 0xFF)
#define viddec_fw_mp4_vol_set_par_width(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 4, 0xFF)
#define viddec_fw_mp4_vol_get_aspect_ratio_info(x)      viddec_fw_bitfields_extract( (x)->vol_aspect_ratio, 0, 0xF)
#define viddec_fw_mp4_vol_set_aspect_ratio_info(x, val) viddec_fw_bitfields_insert ( (x)->vol_aspect_ratio, val, 0, 0xF)
            unsigned int vol_aspect_ratio;

            // Video Object Layer(From LSB):
            //   - vbv_parameters - 1b
            //   - bit_rate - 30b
#define viddec_fw_mp4_vol_get_bit_rate(x)      viddec_fw_bitfields_extract( (x)->vol_bit_rate, 1, 0x3FFFFFFF)
#define viddec_fw_mp4_vol_set_bit_rate(x, val) viddec_fw_bitfields_insert ( (x)->vol_bit_rate, val, 1, 0x3FFFFFFF)
#define viddec_fw_mp4_vol_get_vbv_param(x)      viddec_fw_bitfields_extract( (x)->vol_bit_rate, 0, 0x1)
#define viddec_fw_mp4_vol_set_vbv_param(x, val) viddec_fw_bitfields_insert ( (x)->vol_bit_rate, val, 0, 0x1)
            unsigned int vol_bit_rate;

            // Video Object Layer(From LSB):
            //   - fixed_vop_time_increment - 16b
            //   - vop_time_increment_resolution - 16b
#define viddec_fw_mp4_vol_get_vop_time_increment_resolution(x) viddec_fw_bitfields_extract((x)->vol_frame_rate, 16, 0xFFFF)
#define viddec_fw_mp4_vol_set_vop_time_increment_resolution(x, val) viddec_fw_bitfields_insert((x)->vol_frame_rate, val, 16, 0xFFFF)
#define viddec_fw_mp4_vol_get_fixed_vop_time_increment(x)      viddec_fw_bitfields_extract((x)->vol_frame_rate, 1, 0xFFFF)
#define viddec_fw_mp4_vol_set_fixed_vop_time_increment(x, val) viddec_fw_bitfields_insert((x)->vol_frame_rate, val, 1, 0xFFFF)
            unsigned int vol_frame_rate;
        } mp4_vol; // mp4 item of type VIDDEC_WORKLOAD_MPEG4_VIDEO_OBJ

        struct
        {
            // Group of Video Object Planes(From LSB):
            //   - time_code - 18b
            //   - closed_gov - 1b
            //   - broken_link - 1b
#define viddec_fw_mp4_gvop_get_broken_link(x)      viddec_fw_bitfields_extract((x)->gvop_info, 19, 0x1)
#define viddec_fw_mp4_gvop_set_broken_link(x, val) viddec_fw_bitfields_insert((x)->gvop_info, val, 19, 0x1)
#define viddec_fw_mp4_gvop_get_closed_gov(x)      viddec_fw_bitfields_extract((x)->gvop_info, 18, 0x1)
#define viddec_fw_mp4_gvop_set_closed_gov(x, val) viddec_fw_bitfields_insert((x)->gvop_info, val, 18, 0x1)
#define viddec_fw_mp4_gvop_get_time_code(x)      viddec_fw_bitfields_extract((x)->gvop_info, 0, 0x3FFFF)
#define viddec_fw_mp4_gvop_set_time_code(x, val) viddec_fw_bitfields_insert((x)->gvop_info, val, 0, 0x3FFFF)
            unsigned int gvop_info;

            unsigned int pad1;
            unsigned int pad2;
        } mp4_gvop; // mp4 item of type VIDDEC_WORKLOAD_MPEG4_GRP_VIDEO_OBJ

        struct
        {
            // Group of Video Object Planes(From LSB):
            //   - source_format - 3b
#define viddec_fw_mp4_vpsh_get_source_format(x)      viddec_fw_bitfields_extract((x)->info, 0, 0x7)
#define viddec_fw_mp4_vpsh_set_source_format(x, val) viddec_fw_bitfields_insert((x)->info, val, 0, 0x7)
            unsigned int info;

            unsigned int pad1;
            unsigned int pad2;
        } mp4_vpsh; // mp4 item of type VIDDEC_WORKLOAD_MPEG4_VIDEO_PLANE_SHORT

        unsigned int    vwi_payload[3];
    };
} viddec_workload_item_t;



#endif /* VIDDEC_ITEM_TYPES_H */
