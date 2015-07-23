/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    VC1 header.
//
*/

#ifndef _VC1_COMMON_H_
#define _VC1_COMMON_H_

/* If the pixel data is left near an emulation prevention sequence, the decoder will be unaware
   unless we send some previous bytes */
//#define PADDING_FOR_EMUL 3
#define PADDING_FOR_EMUL 0

#define GET_BLSB( name, bitf )  BLSB_MFD_##name##_##bitf
#define GET_BMSK( name, bitf )  BMSK_MFD_##name##_##bitf

#define BF_READ( name, bitf, value )  ((value & GET_BMSK(name, bitf) ) >> GET_BLSB(name, bitf) )
#define BF_WRITE( name, bitf, value, data ) value = ((value & ~GET_BMSK(name, bitf)) | ((data) << GET_BLSB(name, bitf)))

enum vc1_workload_item_type
{
    VIDDEC_WORKLOAD_VC1_DMEM = VIDDEC_WORKLOAD_DECODER_SPECIFIC,
    VIDDEC_WORKLOAD_VC1_BITOFFSET,
    VIDDEC_WORKLOAD_VC1_BITPLANE0,
    VIDDEC_WORKLOAD_VC1_BITPLANE1,
    VIDDEC_WORKLOAD_VC1_BITPLANE2,
    VIDDEC_WORKLOAD_VC1_REGS_SEQ_ENTRY,
    VIDDEC_WORKLOAD_VC1_REGS_SIZE_AND_AP_RANGEMAP,
    VIDDEC_WORKLOAD_VC1_REGS_INT_COM_FW,
    VIDDEC_WORKLOAD_VC1_REGS_INT_COM_BW,
    VIDDEC_WORKLOAD_VC1_REGS_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO,
    VIDDEC_WORKLOAD_VC1_REGS_SLICE_FRAME_TYPE_INFO,
    VIDDEC_WORKLOAD_VC1_REGS_SLICE_CONTROL_INFO,
    VIDDEC_WORKLOAD_VC1_REGS_SLICE_OTHER_INFO,
    VIDDEC_WORKLOAD_VC1_REGS_REF_FRAME_TYPE,
    VIDDEC_WORKLOAD_VC1_PAST_FRAME   = VIDDEC_WORKLOAD_REF_FRAME_SOURCE_0,
    VIDDEC_WORKLOAD_VC1_FUTURE_FRAME,
};

typedef enum
{
    vc1_ProgressiveFrame = 0,
    vc1_InterlacedFrame  = 2,
    vc1_InterlacedField  = 3,
    vc1_PictureFormatNone
} vc1_fcm;

/** This enumeration defines the various frame types as defined in PTYPE syntax
element.
PTYPE interpretation depends on bitstream profile. The value that needs to get
programmed in the frame_type register 0x2218 is this generic enum obtained
from Canmore code.
Changing this enum to match the spec for each profile caused md5 mismatches.
TODO: Why are these the values to program - is this the case with reference decoder?
*/
enum
{
    VC1_I_FRAME       = (1 << 0),
    VC1_P_FRAME       = (1 << 1),
    VC1_B_FRAME       = (1 << 2),
    VC1_BI_FRAME      = VC1_I_FRAME | VC1_B_FRAME,
    VC1_SKIPPED_FRAME = (1 << 3) | VC1_P_FRAME
};

enum {
    vc1_FrameDone   = 1 << 0,
    vc1_FieldDone   = 1 << 1,
    vc1_SliceDone   = 1 << 2,
    vc1_Field1Done  = 1 << 3,
    vc1_Field2Done  = 1 << 4,
    vc1_FrameError  = 1 << 8,
};

typedef struct {
    /* 0x00 */
    uint32_t general;
    /* 0x04 */
    uint32_t stream_format1;
    /* 0x08 */
    uint32_t coded_size;
    /* 0x0c */
    uint32_t stream_format2;
    /* 0x10 */
    uint32_t entrypoint1;
    /* 0x14 */
    uint32_t ap_range_map;
    /* 0x18 */
    uint32_t frame_type;
    /* 0x1c */
    uint32_t recon_control;
    /* 0x20 */
    uint32_t mv_control;
    /* 0x24 */
    uint32_t intcomp_fwd_top;
    /* 0x28 */
    uint32_t ref_bfraction;
    /* 0x2c */
    uint32_t blk_control;
    /* 0x30 */
    uint32_t trans_data;
    /* 0x34 */
    uint32_t vop_dquant;
#define NUM_REF_ID 4
    /* 0x38-0x48 */ uint32_t ref_frm_id[NUM_REF_ID];
    /* 0x48 */
    uint32_t fieldref_ctrl_id;
    /* 0x4c */
    uint32_t auxfrmctrl;
    /* 0x50 */
    uint32_t imgstruct;
    /* 0x54 */
    uint32_t alt_frame_type;
    /* 0x58 */
    uint32_t intcomp_fwd_bot;
    /* 0x5c */
    uint32_t intcomp_bwd_top;
    /* 0x60 */
    uint32_t intcomp_bwd_bot;
    /* 0x14 */
    uint32_t smp_range_map;
} VC1D_SPR_REGS;

/*
In VC1, past reference is the fwd reference and future reference is the backward reference
i.e. P frame has only a forward reference and B frame has both a forward and a backward reference.
*/
enum {
    VC1_FRAME_CURRENT_REF = 0,
    VC1_FRAME_CURRENT_DIS,
    VC1_FRAME_PAST,
    VC1_FRAME_FUTURE,
    VC1_FRAME_ALT
};

#endif  //_VC1_COMMON_H_

