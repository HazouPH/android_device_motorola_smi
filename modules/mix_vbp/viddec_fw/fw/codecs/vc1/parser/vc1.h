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

#ifndef _VC1_H_
#define _VC1_H_

#ifdef MFD_FIRMWARE
typedef unsigned int size_t;
#define LOG(...)
#else

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#ifndef VBP
enum {
    NONE = 0,
    CRITICAL,
    WARNING,
    INFO,
    DEBUG,
} ;

#define vc1_log_level DEBUG

#define LOG( log_lev, format, args ... ) \
      if (vc1_log_level >= log_lev) { OS_INFO("%s[%d]:: " format "\n", __FUNCTION__ , __LINE__ ,  ## args ); }
#endif
#endif

#include "viddec_fw_workload.h"
#include "vc1parse_common_defs.h"
#include "vc1common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VBP
#define LOG_CRIT(format, args ... )  LOG( CRITICAL, format, ## args)
#define LOG_WARN(format, args ... )  LOG( WARNING,  format, ## args)
#define LOG_INFO(format, args ... )  LOG( INFO,     format, ## args)
#define LOG_DEBUG(format, args ... ) LOG( DEBUG,    format, ## args)
#else
#define LOG_CRIT(format, args ... )
#define LOG_WARN(format, args ... )
#define LOG_INFO(format, args ... )
#define LOG_DEBUG(format, args ... )
#endif

// Seems to be hardware bug: DO NOT TRY TO SWAP BITPLANE0 and BITPLANE2
// Block Control Register at offset 222C uses Bitplane_raw_ID0 to indicate directmb/fieldtx while
// and Bitplane_raw_ID2 for acpred/mvtypemb/forwardmb
// but when we send bitplane index 0 for directmb/fieldtx and bitplane index 2 for acpred/mvtypemb/forwardmb
// md5 mismatches are seen
    typedef enum
    {
        BPP_FORWARDMB  =  VIDDEC_WORKLOAD_VC1_BITPLANE0,
        BPP_ACPRED     =  VIDDEC_WORKLOAD_VC1_BITPLANE0,
        BPP_MVTYPEMB   =  VIDDEC_WORKLOAD_VC1_BITPLANE0,
        BPP_OVERFLAGS  =  VIDDEC_WORKLOAD_VC1_BITPLANE1,
        BPP_SKIPMB     =  VIDDEC_WORKLOAD_VC1_BITPLANE1,
        BPP_DIRECTMB   =  VIDDEC_WORKLOAD_VC1_BITPLANE2,
        BPP_FIELDTX    =  VIDDEC_WORKLOAD_VC1_BITPLANE2,
    } vc1_bpp_type_t;

    /* status codes */
    typedef enum {
        VC1_STATUS_EOF          =  1,   // end of file
        VC1_STATUS_OK           =  0,   // no error
        VC1_STATUS_NO_MEM       =  2,   // out of memory
        VC1_STATUS_FILE_ERROR   =  2,   // file error
        VC1_STATUS_NOTSUPPORT   =  2,   // not supported mode
        VC1_STATUS_PARSE_ERROR  =  2,   // fail in parse MPEG-4 stream
        VC1_STATUS_ERROR        =  2    // unknown/unspecified error
    } vc1_Status;

    /* VC1 start code values */
    typedef enum {
        vc1_Forbidden           = 0x80,/*0x80-0xFF*/
        vc1_Reserved1           = 0x09,/*0x00-0x09*/
        vc1_Reserved2           = 0x10,
        vc1_Reserved3           = 0x1A,
        vc1_Reserved4           = 0x20,/*0x20-0x7F*/
        vc1_SCEndOfSequence     = 0x0A,
        vc1_SCSlice             = 0x0B,
        vc1_SCField             = 0x0C,
        vc1_SCFrameHeader       = 0x0D,
        vc1_SCEntryPointHeader  = 0x0E,
        vc1_SCSequenceHeader    = 0x0F,
        vc1_SCSliceUser         = 0x1B,
        vc1_SCFieldUser         = 0x1C,
        vc1_SCFrameUser         = 0x1D,
        vc1_SCEntryPointUser    = 0x1E,
        vc1_SCSequenceUser      = 0x1F
    } vc1_sc;

#if 0
    typedef enum
    {
        vc1_ProfileSimple = 0,  /** Simple profile */
        vc1_ProfileMain,        /** Main profile */
        vc1_ProfileReserved,    /** Reserved */
        vc1_ProfileAdvanced     /** Advanced profile */
    } vc1_Profile;
#endif

    typedef enum
    {
        vc1_PtypeI  = 1,
        vc1_PtypeP  = 2,
        vc1_PtypeB  = 4,
        vc1_PtypeBI = 5,
        vc1_PtypeSkipped = 8|2,
    } vc1_ptype;

    typedef enum
    {
        vc1_PtypeII = 0,
        vc1_PtypeIP = 1,
        vc1_PtypePI = 2,
        vc1_PtypePP = 3,
        vc1_PtypeBB = 4,
        vc1_PtypeBBI = 5,
        vc1_PtypeBIB = 6,
        vc1_PtypeBIBI = 7
    } vc1_fptype;

    typedef enum
    {
        vc1_Imode_Raw  = 0,         //0x0000
        vc1_Imode_Norm2,        //0x10
        vc1_Imode_Diff2,        //0x001
        vc1_Imode_Norm6,        //0x11
        vc1_Imode_Diff6,        //0x0001
        vc1_Imode_Rowskip,      //0x010
        vc1_Imode_Colskip,      //0x011
    } vc1_Imode;

    /* calculation of MAX_BITPLANE_SZ 2048/16x1088/16 pel= 128x68 bit used for bitplane
     * as rows are packed in DWORDS
     * we have (128)/32 * 68 Dwords needed for bitplane storage
     */
#define MAX_BITPLANE_SZ 272

    /* Full Info */
    typedef struct {
        unsigned char*       bufptr;         /* current frame, point to header or data */
        int                  bitoff;         /* mostly point to next frame header or PSC */
        int                  picture_info_has_changed;
        vc1_metadata_t       metadata;
        vc1_PictureLayerHeader picLayerHeader;
        uint32_t             bitplane[MAX_BITPLANE_SZ];
    } vc1_Info;

#ifdef __cplusplus
}
#endif

enum {
    VC1_REF_FRAME_T_MINUS_1 = 0,
    VC1_REF_FRAME_T_MINUS_2,
    VC1_REF_FRAME_T_MINUS_0,
    VC1_NUM_REFERENCE_FRAMES,
};

enum vc1_sc_seen_flags
{
    VC1_SC_INVALID = 0 << 0,
    VC1_SC_SEQ     = 1 << 0,
    VC1_SC_EP      = 1 << 1,
    VC1_SC_FRM     = 1 << 2,
    VC1_SC_FLD     = 1 << 3,
    VC1_SC_SLC     = 1 << 4,
    VC1_SC_UD      = 1 << 5,
};
#define VC1_SEQ_MASK VC1_SC_SEQ
#define VC1_EP_MASK VC1_SC_SEQ | VC1_SC_EP
#define VC1_FRM_MASK VC1_SC_SEQ | VC1_SC_EP | VC1_SC_FRM
#define VC1_FLD_MASK VC1_SC_SEQ | VC1_SC_EP | VC1_SC_FRM | VC1_SC_FLD

typedef struct {
    int id;
    uint32_t intcomp_top;
    uint32_t intcomp_bot;
    int fcm;         /* frame coding mode */
    int type;
    int anchor[2];   /* one per field */
    int rr_en;       /* range reduction enable flag at sequence layer */
    int rr_frm;      /* range reduction flag at picture layer */
    int tff;
} ref_frame_t;

typedef struct
{
    uint32_t      sc_seen_since_last_wkld;
    uint32_t      sc_seen;
    uint32_t      is_frame_start;
    uint32_t      is_second_start;
    uint32_t      is_reference_picture;
    uint32_t      intcomp_last[4]; /* for B frames */
    uint32_t      intcomp_top[2];
    uint32_t      intcomp_bot[2];
    vc1_Info      info;
    VC1D_SPR_REGS spr;
    ref_frame_t   ref_frame[VC1_NUM_REFERENCE_FRAMES];
#ifdef VBP
    /* A storage area is provided for each type of bit plane.  Only one of */
    /* each type will ever be used for a picture and never more than three */
    /* bit-planes per picture, and often only one is used.  We never clear */
    /* this data and writes into it when we need to.  vc1parse_bitplane.c  */
    /* makes use of these set them to one of the bitplane types included   */
    /* in the picture header structure.  Those sturctures are set every    */
    /* time a picture parse begins. */
    uint32_t      bp_forwardmb[4096];
    uint32_t      bp_acpred[4096];
    uint32_t      bp_mvtypemb[4096];
    uint32_t      bp_overflags[4096];
    uint32_t      bp_skipmb[4096];
    uint32_t      bp_directmb[4096];
    uint32_t      bp_fieldtx[4096];
    uint32_t	  start_code;
#endif
} vc1_viddec_parser_t;

#endif  //_VC1_H_

