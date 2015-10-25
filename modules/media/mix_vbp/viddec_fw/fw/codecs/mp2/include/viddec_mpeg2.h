#ifndef _VIDDEC_MPEG2_H
#define _VIDDEC_MPEG2_H

/**
 * viddec_mpeg2.h
 * --------------
 * This header file contains all the necessary state information and function
 * prototypes for the MPEG2 parser. This header also defines the debug macros
 * used by the MPEG2 parser to emit debug messages in host mode.
 */

#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "mpeg2.h"

/* Debug Print Macros */
#define MPEG2_DEB(x...)        DEB("MPEG2_Parser: "x)
#define MPEG2_FA_DEB(x...)     DEB("MPEG2_Frame_attribute: "x)

/* Bit masks */
#define MPEG2_BIT_MASK_11      0x7ff /* Used for masking Height and Width */
#define MPEG2_BIT_MASK_8       0xff  /* Used fro masking start code byte */
#define MPEG2_BIT_MASK_4       0xf   /* Used for masking Level */
#define MPEG2_BIT_MASK_3       0x7   /* Used for masking Profile */

/* MPEG2 Start code and prefix size */
#define MPEG2_SC_AND_PREFIX_SIZE 32

/* Number of DMEM Workload Items */
#define MPEG2_NUM_DMEM_WL_ITEMS 2

/* Number of Quantization Matrix Workload Items */
#define MPEG2_NUM_QMAT_WL_ITEMS 32

/* Maximum supported content size */
#define MPEG2_MAX_CONTENT_WIDTH  2048
#define MPEG2_MAX_CONTENT_HEIGHT 2048

/* Others */
#define MPEG2_BITS_EIGHT        8


/* MPEG2 Stream Levels */
typedef enum {
    MPEG2_LEVEL_SEQ = 0,
    MPEG2_LEVEL_GOP,
    MPEG2_LEVEL_PIC
} mpeg2_stream_levels;

/* MPEG2 Headers and Extensions */
typedef enum {
    MPEG2_HEADER_NONE           = 0,
    MPEG2_HEADER_SEQ            = 1 << 0,
    MPEG2_HEADER_SEQ_EXT        = 1 << 1,
    MPEG2_HEADER_SEQ_DISP_EXT   = 1 << 2,
    MPEG2_HEADER_GOP            = 1 << 3,
    MPEG2_HEADER_PIC            = 1 << 4,
    MPEG2_HEADER_PIC_COD_EXT    = 1 << 5,
    MPEG2_HEADER_PIC_DISP_EXT   = 1 << 6,
    MPEG2_HEADER_SEQ_SCAL_EXT   = 1 << 7
} mpeg2_headers;

/* MPEG2 Parser Status Codes */
typedef enum {
    MPEG2_SUCCESS            = 0, /* No error */
    MPEG2_FRAME_COMPLETE     = 1, /* Frame parsing complete found */
    MPEG2_PARSE_ERROR        = 2, /* Failure in parsing */
} mpeg2_status;

/* MPEG2 Current Workload Status Codes */
typedef enum {
    MPEG2_WL_EMPTY          = 0,
    MPEG2_WL_DMEM_DATA      = (1 << 0),
    MPEG2_WL_REF_INFO       = (1 << 1),
    MPEG2_WL_PARTIAL_SLICE  = (1 << 2),
    MPEG2_WL_DANGLING_FIELD = (1 << 3),
    MPEG2_WL_COMPLETE       = (1 << 4),
    MPEG2_WL_MISSING_TF     = (1 << 5),
    MPEG2_WL_MISSING_BF     = (1 << 6),
    MPEG2_WL_UNSUPPORTED    = (1 << 7),
    /* Error codes */
    MPEG2_WL_CORRUPTED_SEQ_HDR      = (1 << 8),
    MPEG2_WL_CORRUPTED_SEQ_EXT      = (1 << 9),
    MPEG2_WL_CORRUPTED_SEQ_DISP_EXT = (1 << 10),
    MPEG2_WL_CORRUPTED_GOP_HDR      = (1 << 11),
    MPEG2_WL_CORRUPTED_PIC_HDR      = (1 << 12),
    MPEG2_WL_CORRUPTED_PIC_COD_EXT  = (1 << 13),
    MPEG2_WL_CORRUPTED_PIC_DISP_EXT = (1 << 14),
    MPEG2_WL_CORRUPTED_QMAT_EXT     = (1 << 15),
    /* Error concealment codes */
    MPEG2_WL_CONCEALED_PIC_COD_TYPE = (1 << 16),
    MPEG2_WL_CONCEALED_PIC_STRUCT   = (1 << 17),
    MPEG2_WL_CONCEALED_CHROMA_FMT   = (1 << 18),
    /* Type of dangling field */
    MPEG2_WL_DANGLING_FIELD_TOP     = (1 << 24),
    MPEG2_WL_DANGLING_FIELD_BOTTOM  = (1 << 25),
    MPEG2_WL_REPEAT_FIELD           = (1 << 26),
} mpeg2_wl_status_codes;

/* MPEG2 Parser Workload types */
typedef enum
{
    /* MPEG2 Decoder Specific data */
    VIDDEC_WORKLOAD_MPEG2_DMEM = VIDDEC_WORKLOAD_DECODER_SPECIFIC,

    /* MPEG2 Quantization Matrix data */
    VIDDEC_WORKLOAD_MPEG2_QMAT,

    /* Past reference frame */
    VIDDEC_WORKLOAD_MPEG2_REF_PAST = VIDDEC_WORKLOAD_REF_FRAME_SOURCE_0,

    /* Future reference frame */
    VIDDEC_WORKLOAD_MPEG2_REF_FUTURE,

    /* Use current frame as reference */
    VIDDEC_WORKLOAD_MPEG2_REF_CURRENT_FRAME,

    /* User Data */
    VIDDEC_WORKLOAD_MPEG2_USERDATA = VIDDEC_WORKLOAD_USERDATA
} viddec_mpeg2_workloads;

/* MPEG2 Decoder Specific Workitems */
struct mpeg2_workitems
{
    /* Core Sequence Info 1 */
    uint32_t csi1;

    /* Core Sequence Info 2 */
    uint32_t csi2;

    /* Core Picture Info 1 */
    uint32_t cpi1;

    /* Core Picture Coding Extension Info 1 */
    uint32_t cpce1;

    /* Quantization Matrices */
    /*  0-15: Intra Quantization Matrix */
    /* 16-31: Non-Intra Quantization Matrix */
    /* 32-47: Chroma Intra Quantization Matrix */
    /* 48-63: Chroma Non-Intra Quantization Matrix */
    uint32_t qmat[MPEG2_QUANT_MAT_SIZE];
};

/* MPEG2 Video Parser Context */
struct viddec_mpeg2_parser
{
    /* MPEG2 Metadata Structure */
    struct mpeg2_info info;

    /* MPEG2 Workitems */
    struct mpeg2_workitems wi;

    /* Workload Status */
    uint32_t  mpeg2_wl_status;

    /* Last parsed start code */
    int32_t   mpeg2_last_parsed_sc;

    /* Last parsed slice start code. Used to start emitting workload items. */
    int32_t   mpeg2_last_parsed_slice_sc;

    /* Current sequence headers parsed */
    uint8_t   mpeg2_curr_seq_headers;

    /* Current frame headers parsed */
    uint8_t   mpeg2_curr_frame_headers;

    /* Flag to indicate a valid sequence header was successfully parsed for */
    /* the current stream. */
    uint8_t   mpeg2_valid_seq_hdr_parsed;

    /* Flag to indicate if quantization matrices are updated */
    uint8_t   mpeg2_custom_qmat_parsed;

    /* Flag to indicate if reference table is updated with an entry */
    uint8_t   mpeg2_ref_table_updated;

    /* Flag to indicate if the stream is MPEG2 */
    uint8_t   mpeg2_stream;

    /* Flag to indicate if the previous picture metadata is parsed */
    uint8_t   mpeg2_pic_metadata_complete;

    /* Number of active pan scan offsets */
    uint8_t   mpeg2_num_pan_scan_offsets;

    /* Indicates the current stream level (Sequence/GOP/Picture) */
    /* Used for identifying the level for User Data */
    uint8_t   mpeg2_stream_level;

    /* Flag to indicate if the current picture is interlaced or not */
    uint8_t   mpeg2_picture_interlaced;

    /* Flag to indicate if the current field for interlaced picture is first */
    /* field or not. This flag is used only when mpeg2_picture_interlaced is */
    /* set to 1. */
    uint8_t   mpeg2_first_field;

    /* Flag to indicate if the current parsed data has start of a frame */
    uint8_t   mpeg2_frame_start;

    /* Temporal reference of the previous picture - Used to detect dangling fields */
    uint32_t  mpeg2_prev_temp_ref;

    /* Previous picture structure - Used to identify the type of missing field */
    uint8_t   mpeg2_prev_picture_structure;

    /* Flag to decide whether to use the current or next workload to dump workitems */
    uint8_t   mpeg2_use_next_workload;
    uint8_t   mpeg2_first_slice_flag;
};

/* External Function Declarations */
extern void *memset(void *s, int32_t c, uint32_t n);

/* MPEG2 Parser Function Prototypes */
void     viddec_mpeg2_translate_attr            (void *parent, void *ctxt);
void     viddec_mpeg2_emit_workload             (void *parent, void *ctxt);
void     viddec_mpeg2_parse_seq_hdr             (void *parent, void *ctxt);
void     viddec_mpeg2_parse_gop_hdr             (void *parent, void *ctxt);
void     viddec_mpeg2_parse_pic_hdr             (void *parent, void *ctxt);
void     viddec_mpeg2_parse_and_append_user_data(void *parent, void *ctxt);
void     viddec_mpeg2_parse_and_append_slice_data(void *parent, void *ctxt);
void     viddec_mpeg2_parse_ext                 (void *parent, void *ctxt);

/* MPEG2 wrapper functions for workload operations */
void    viddec_mpeg2_append_workitem        (void *parent, viddec_workload_item_t *wi, uint8_t flag);
void    viddec_mpeg2_append_pixeldata       (void *parent, uint8_t flag);
viddec_workload_t*  viddec_mpeg2_get_header (void *parent, uint8_t flag);
#endif
