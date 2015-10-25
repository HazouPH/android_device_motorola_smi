/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
* Copyright (c) Imagination Technologies Limited, UK
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

#ifndef _VP8_H_
#define _VP8_H_
#include "bool_coder.h"

#ifdef __cplusplus
extern "C" {
#endif

/* VP8 specifies only frame is supported */
#define VP8_MAX_NUM_PICTURES    1
/* VP8 has no definition of slice */
#define VP8_MAX_NUM_SLICES      1

#define MAX_MB_SEGMENTS         4
#define MB_FEATURE_TREE_PROBS   3
#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      4
#define MAX_PARTITIONS          9
#define BLOCK_TYPES             4
#define COEF_BANDS              8
#define PREV_COEF_CONTEXTS      3
#define MAX_COEF_TOKENS         12
#define MAX_ENTROPY_TOKENS      12
#define SEGMENT_DELTADATA       0
#define SEGMENT_ABSDATA         1
#define MAX_LOOP_FILTER         63
#define MAX_QINDEX              127

    typedef uint8_t vp8_prob;

    typedef enum
    {
        /*!\brief Operation completed without error */
        VP8_NO_ERROR,

        /*!\brief Unspecified error */
        VP8_UNKNOWN_ERROR,

        /*!\brief Memory operation failed */
        VP8_MEMORY_ERROR,

        VP8_NO_INITIALIZATION,

        VP8_CORRUPT_FRAME,

        VP8_UNSUPPORTED_BITSTREAM,

        VP8_UNSUPPORTED_VERSION,

        VP8_INVALID_FRAME_SYNC_CODE,

        VP8_UNEXPECTED_END_OF_BITSTREAM,

    } vp8_Status;

    enum
    {
        VP8_MV_max  = 1023,                   /* max absolute value of a MV component */
        VP8_MV_vals = (2 * VP8_MV_max) + 1,   /* # possible values "" */

        VP8_MV_long_width = 10,       /* Large MVs have 9 bit magnitudes */
        VP8_MV_num_short = 8,         /* magnitudes 0 through 7 */

        /* probability offsets for coding each MV component */
        VP8_MV_pis_short = 0,        /* short (<= 7) vs long (>= 8) */
        VP8_MV_Psign,                /* sign for non-zero */
        VP8_MV_Pshort,               /* 8 short values = 7-position tree */

        VP8_MV_Pbits = VP8_MV_Pshort + VP8_MV_num_short - 1, /* mvlong_width long value bits */
        VP8_MV_Pcount = VP8_MV_Pbits + VP8_MV_long_width     /* (with independent probabilities) */
    };

    typedef enum
    {
        DC_PRED,            // average of above and left pixels
        V_PRED,             // vertical prediction
        H_PRED,             // horizontal prediction
        TM_PRED,            // Truemotion prediction
        B_PRED,             // block based prediction, each block has its own prediction mode
        NEARESTMV,
        NEARMV,
        ZEROMV,
        NEWMV,
        SPLITMV,
        MB_MODE_COUNT
    } VP8_MB_PREDICTION_MODE;

// Segment Feature Masks
#define VP8_SEGMENT_ALTQ    0x01
#define VP8_SEGMENT_ALT_LF  0x02

#define VP8_YMODES  (B_PRED + 1)
#define VP8_UV_MODES (TM_PRED + 1)

#define VP8_MVREFS (1 + SPLITMV - NEARESTMV)

    typedef enum
    {
        B_DC_PRED,          // average of above and left pixels
        B_TM_PRED,

        B_VE_PRED,           // vertical prediction
        B_HE_PRED,           // horizontal prediction

        B_LD_PRED,
        B_RD_PRED,

        B_VR_PRED,
        B_VL_PRED,
        B_HD_PRED,
        B_HU_PRED,

        LEFT4X4,
        ABOVE4X4,
        ZERO4X4,
        NEW4X4,

        B_MODE_COUNT
    } VP8_B_PREDICTION_MODE;

#define VP8_BINTRAMODES (B_HU_PRED + 1)  /* 10 */
#define VP8_SUBMVREFS (1 + NEW4X4 - LEFT4X4)

// frame type
    typedef enum
    {
        KEY_FRAME = 0,
        INTER_FRAME,
        SKIPPED_FRAME
    } FRAME_TYPE;


// Color Space
    typedef enum
    {
        REG_YUV = 0,    /* Regular yuv */
        INT_YUV = 1     /* The type of yuv that can be tranfer to and from RGB through integer transform */
    } YUV_TYPE;

// Clamp type
    typedef enum
    {
        RECON_CLAMP_REQUIRED        = 0,
        RECON_CLAMP_NOTREQUIRED     = 1
    } CLAMP_TYPE;

    /* Token partition */
    typedef enum
    {
        ONE_PARTITION  = 0,
        TWO_PARTITION  = 1,
        FOUR_PARTITION = 2,
        EIGHT_PARTITION = 3
    } TOKEN_PARTITION;

// Buffer copied
    typedef enum
    {
        BufferCopied_NoneToGolden   = 0,
        BufferCopied_LastToGolden   = 1,
        BufferCopied_AltRefToGolden = 2
    } GoldenBufferCopiedType;

    typedef enum
    {
        BufferCopied_NoneToAltref   = 0,
        BufferCopied_LastToAltRef   = 1,
        BufferCopied_GoldenToAltRef = 2
    } AltRefBufferCopiedType;

// Macroblock level features
    typedef enum
    {
        MB_LVL_ALT_Q = 0,   /* Use alternate Quantizer .... */
        MB_LVL_ALT_LF = 1,  /* Use alternate loop filter value... */
        MB_LVL_MAX = 2      /* Number of MB level features supported */
    } MB_LVL_FEATURES;

// Loop filter Type
    typedef enum
    {
        NORMAL_LOOPFILTER = 0,
        SIMPLE_LOOPFILTER = 1
    } LoopFilterType;

// Segmentation data
    typedef struct
    {
        uint8_t              Enabled;
        uint8_t              UpdateMap;
        uint8_t              UpdateData;
        uint8_t              AbsDelta;
        int8_t               FeatureData[MB_LVL_MAX][MAX_MB_SEGMENTS];
        vp8_prob             TreeProbs[MB_FEATURE_TREE_PROBS];
    } SegmentationData;

// Loop filter data
    typedef struct
    {
        LoopFilterType       Type;
        uint8_t              Level;
        uint8_t              Sharpness;
        uint8_t              DeltaEnabled;
        uint8_t              DeltaUpdate;
        int8_t               DeltasRef[MAX_REF_LF_DELTAS];
        int8_t               DeltasMode[MAX_MODE_LF_DELTAS];
    } LoopFilterData;

// Quantization data
    typedef struct
    {
        int8_t               Y1_AC;
        int8_t               Y1_DC_Delta;
        int8_t               Y2_DC_Delta;
        int8_t               Y2_AC_Delta;
        int8_t               UV_DC_Delta;
        int8_t               UV_AC_Delta;
    } QuantizationData;

// Frame context
    typedef struct
    {
        vp8_prob            B_Mode_Prob[VP8_BINTRAMODES][VP8_BINTRAMODES][VP8_BINTRAMODES-1];
        vp8_prob            Y_Mode_Prob [VP8_YMODES-1];   /* interframe intra mode probs */
        vp8_prob            UV_Mode_Prob [VP8_UV_MODES-1];
        vp8_prob            DCT_Coefficients [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [MAX_COEF_TOKENS-1];
        vp8_prob            MVContext[2][VP8_MV_Pcount];
        vp8_prob            Pre_MVContext[2][VP8_MV_Pcount];  //not to caculate the mvcost for the frame if mvc doesn't change.
    } FrameContextData;

// Extern to tables
    extern const vp8_prob    VP8_Coefficient_Default_Probabilites[BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [MAX_COEF_TOKENS-1];
    extern const vp8_prob    VP8_Coefficient_Update_Probabilites[BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [MAX_COEF_TOKENS-1];
    extern const int         VP8_MB_FeatureDataBits[MB_LVL_MAX];
    extern const vp8_prob    VP8_BMode_Const[VP8_BINTRAMODES][VP8_BINTRAMODES][VP8_BINTRAMODES-1];
    extern const vp8_prob    VP8_YMode_Const[VP8_YMODES-1];
    extern const vp8_prob    VP8_UVMode_Const[VP8_UV_MODES-1];
    extern const vp8_prob    VP8_MV_UpdateProbs[2][VP8_MV_Pcount], VP8_MV_DefaultMVContext[2][VP8_MV_Pcount];

    typedef struct
    {
        FRAME_TYPE            frame_type;
        uint8_t               version;
        uint8_t               show_frame;
        uint32_t              first_part_size;
    } FrameTagHeader;

    typedef struct _vp8_Info
    {
        // Frame Tag Header
        FrameTagHeader         frame_tag;

        // Key Frame data
        uint32_t               width;
        uint32_t               height;
        uint32_t               horiz_scale;
        uint32_t               vert_scale;
        YUV_TYPE               clr_type;
        CLAMP_TYPE             clamp_type;

        vp8_prob               prob_intra;
        vp8_prob               prob_lf;
        vp8_prob               prob_gf;

        uint8_t                y_prob_valid;
        uint8_t                c_prob_valid;

        uint32_t               header_bits;
        uint32_t               frame_data_offset;

        uint8_t                *source;
        uint32_t               source_sz;

        // Decoded picture number
        uint32_t               decoded_frame_number;

        BOOL_CODER             bool_coder;

        // Refresh flags
        uint8_t                refresh_lf;

        uint8_t                refresh_gf;
        uint8_t                refresh_af;
        uint8_t                sign_bias_golden;
        uint8_t                sign_bias_alternate;

        GoldenBufferCopiedType golden_copied;
        AltRefBufferCopiedType altref_copied;

        // Degmentation data
        SegmentationData       Segmentation;

        // Loop filter data
        LoopFilterData         LoopFilter;

        // Partitions
        uint8_t                partition_count;
        uint8_t                partition_number;
        uint32_t               partition_size[1<<EIGHT_PARTITION];

        // Quantization
        QuantizationData       Quantization;

        // Refresh entropy
        uint8_t                refresh_entropy;
        // Refresh entropy
        uint8_t                refresh_entropy_lf;

        // Macroblock No Coeff Skip
        uint8_t                mb_no_coeff_skip;
        vp8_prob               prob_skip_false;
        vp8_prob               mb_skip_coeff;

        // Frame context
        FrameContextData       FrameContext;
        // Same thing exist in the reference.
        // The variable RefreshEntropy is controling storage/saving of that.
        FrameContextData       LastFrameContext;
    } vp8_Info;

    typedef struct _vp8_viddec_parser
    {
        int got_start;

        vp8_Info info;
    } vp8_viddec_parser;

#ifdef __cplusplus
}
#endif
#endif
