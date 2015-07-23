#ifndef VIDDEC_MP4_PARSE_H
#define VIDDEC_MP4_PARSE_H

#include "viddec_fw_debug.h"
#include "viddec_fw_mp4.h"

/* Macros for MP4 start code detection */
#define FIRST_STARTCODE_BYTE        0x00
#define SECOND_STARTCODE_BYTE       0x00
#define THIRD_STARTCODE_BYTE        0x01
#define SHORT_THIRD_STARTCODE_BYTE  0x80
#define SC_BYTE_MASK0               0x00ff0000
#define SC_BYTE_MASK1               0x000000ff

/* status codes */
typedef enum
{
    MP4_STATUS_OK               =  0,   /* Success */
    MP4_STATUS_PARSE_ERROR      = (1 << 0),   /* Invalid syntax */
    MP4_STATUS_NOTSUPPORT       = (1 << 1),   /* unsupported feature */
    MP4_STATUS_REQD_DATA_ERROR  = (1 << 2),   /* supported data either invalid or missing */
} mp4_Status_t;

/* feature codes */
typedef enum
{
    MP4_VOP_FEATURE_DEFAULT =  0,   // Default VOP features, no code image update needed
    MP4_VOP_FEATURE_SVH     =  1,   // VOP has Short Video Header
    MP4_VOP_FEATURE_DP      =  2    // VOP is Data Partitioned
} mp4_Vop_feature;

/* MPEG-4 start code values: Table 6-3 */
typedef enum
{
    MP4_SC_VIDEO_OBJECT_MIN          = 0x00,
    MP4_SC_VIDEO_OBJECT_MAX          = 0x1F,
    MP4_SC_VIDEO_OBJECT_LAYER_MIN    = 0x20,
    MP4_SC_VIDEO_OBJECT_LAYER_MAX    = 0x2F,
    MP4_SC_FGS_BP_MIN                = 0x40, /* Unsupported */
    MP4_SC_FGS_BP_MAX                = 0x5F, /* Unsupported */
    MP4_SC_VISUAL_OBJECT_SEQUENCE    = 0xB0,
    MP4_SC_VISUAL_OBJECT_SEQUENCE_EC = 0xB1,
    MP4_SC_USER_DATA                 = 0xB2,
    MP4_SC_GROUP_OF_VOP              = 0xB3,
    MP4_SC_VIDEO_SESSION_ERROR       = 0xB4,
    MP4_SC_VISUAL_OBJECT             = 0xB5,
    MP4_SC_VIDEO_OBJECT_PLANE        = 0xB6,
    MP4_SC_SLICE                     = 0xB7, /* Unsupported */
    MP4_SC_EXTENSION                 = 0xB8, /* Unsupported */
    MP4_SC_FGS_VOP                   = 0xB9, /* Unsupported */
    MP4_SC_FBA_OBJECT                = 0xBA, /* Unsupported */
    MP4_SC_FBA_OBJECT_PLANE          = 0xBB, /* Unsupported */
    MP4_SC_MESH_OBJECT               = 0xBC, /* Unsupported */
    MP4_SC_MESH_OBJECT_PLANE         = 0xBD, /* Unsupported */
    MP4_SC_STILL_TEXTURE_OBJECT      = 0xBE, /* Unsupported */
    MP4_SC_TEXTURE_SPATIAL_LAYER     = 0xBF, /* Unsupported */
    MP4_SC_TEXTURE_SNR_LAYER         = 0xC0, /* Unsupported */
    MP4_SC_TEXTURE_TILE              = 0xC1, /* Unsupported */
    MP4_SC_TEXTURE_SHAPE_LAYER       = 0xC2, /* Unsupported */
    MP4_SC_STUFFING                  = 0xC3,
    MP4_SC_SYTEM_MIN                 = 0xC6, /* Unsupported */
    MP4_SC_SYTEM_MAX                 = 0xFF, /* Unsupported */
    MP4_SC_INVALID                   = 0x100, /* Invalid */
} mp4_start_code_values_t;

/* MPEG-4 code values
   ISO/IEC 14496-2:2004 table 6-6 */
enum
{
    MP4_VISUAL_OBJECT_TYPE_VIDEO     = 1,
    MP4_VISUAL_OBJECT_TYPE_TEXTURE   = 2,
    MP4_VISUAL_OBJECT_TYPE_MESH      = 3,
    MP4_VISUAL_OBJECT_TYPE_FBA       = 4,
    MP4_VISUAL_OBJECT_TYPE_3DMESH    = 5
};

/* ISO/IEC 14496-2:2004 table 6-7 */
enum
{
    MP4_VIDEO_FORMAT_COMPONENT      = 0,
    MP4_VIDEO_FORMAT_PAL            = 1,
    MP4_VIDEO_FORMAT_NTSC           = 2,
    MP4_VIDEO_FORMAT_SECAM          = 3,
    MP4_VIDEO_FORMAT_MAC            = 4,
    MP4_VIDEO_FORMAT_UNSPECIFIED    = 5
};

/* ISO/IEC 14496-2:2004 table 6-8..10 */
enum
{
    MP4_VIDEO_COLORS_FORBIDDEN         = 0,
    MP4_VIDEO_COLORS_ITU_R_BT_709      = 1,
    MP4_VIDEO_COLORS_UNSPECIFIED       = 2,
    MP4_VIDEO_COLORS_RESERVED          = 3,
    MP4_VIDEO_COLORS_ITU_R_BT_470_2_M  = 4,
    MP4_VIDEO_COLORS_ITU_R_BT_470_2_BG = 5,
    MP4_VIDEO_COLORS_SMPTE_170M        = 6,
    MP4_VIDEO_COLORS_SMPTE_240M        = 7,
    MP4_VIDEO_COLORS_GENERIC_FILM      = 8
};

/* ISO/IEC 14496-2:2004 table 6-11 */
enum
{
    MP4_VIDEO_OBJECT_TYPE_SIMPLE                     = 1,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_SCALABLE            = 2,
    MP4_VIDEO_OBJECT_TYPE_CORE                       = 3,
    MP4_VIDEO_OBJECT_TYPE_MAIN                       = 4,
    MP4_VIDEO_OBJECT_TYPE_NBIT                       = 5,
    MP4_VIDEO_OBJECT_TYPE_2DTEXTURE                  = 6,
    MP4_VIDEO_OBJECT_TYPE_2DMESH                     = 7,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_FACE                = 8,
    MP4_VIDEO_OBJECT_TYPE_STILL_SCALABLE_TEXTURE     = 9,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_REAL_TIME_SIMPLE  = 10,
    MP4_VIDEO_OBJECT_TYPE_CORE_SCALABLE              = 11,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_CODING_EFFICIENCY = 12,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_SCALABLE_TEXTURE  = 13,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_FBA                 = 14,
    MP4_VIDEO_OBJECT_TYPE_SIMPLE_STUDIO              = 15,
    MP4_VIDEO_OBJECT_TYPE_CORE_STUDIO                = 16,
    MP4_VIDEO_OBJECT_TYPE_ADVANCED_SIMPLE            = 17,
    MP4_VIDEO_OBJECT_TYPE_FINE_GRANULARITY_SCALABLE  = 18
};

/*  ISO/IEC 14496-2:2004 table 6.17 (maximum defined video_object_layer_shape_extension) */
#define MP4_SHAPE_EXT_NUM 13

/* ISO/IEC 14496-2:2004 table 6-14 */
enum
{
    MP4_ASPECT_RATIO_FORBIDDEN  = 0,
    MP4_ASPECT_RATIO_1_1        = 1,
    MP4_ASPECT_RATIO_12_11      = 2,
    MP4_ASPECT_RATIO_10_11      = 3,
    MP4_ASPECT_RATIO_16_11      = 4,
    MP4_ASPECT_RATIO_40_33      = 5,
    MP4_ASPECT_RATIO_EXTPAR     = 15
};

/* ISO/IEC 14496-2:2004 table 6-15 */
#define MP4_CHROMA_FORMAT_420    1

/* ISO/IEC 14496-2:2004 table 6-16 */
enum
{
    MP4_SHAPE_TYPE_RECTANGULAR  = 0,
    MP4_SHAPE_TYPE_BINARY       = 1,
    MP4_SHAPE_TYPE_BINARYONLY   = 2,
    MP4_SHAPE_TYPE_GRAYSCALE    = 3
};

/* ISO/IEC 14496-2:2004 table 6-19 */
#define MP4_SPRITE_STATIC   1
#define MP4_SPRITE_GMC      2

/* ISO/IEC 14496-2:2004 table 6-24 */
enum
{
    MP4_VOP_TYPE_I  = 0,
    MP4_VOP_TYPE_P  = 1,
    MP4_VOP_TYPE_B  = 2,
    MP4_VOP_TYPE_S  = 3,
};

/* ISO/IEC 14496-2:2004 table 6-26 */
enum
{
    MP4_SPRITE_TRANSMIT_MODE_STOP   = 0,
    MP4_SPRITE_TRANSMIT_MODE_PIECE  = 1,
    MP4_SPRITE_TRANSMIT_MODE_UPDATE = 2,
    MP4_SPRITE_TRANSMIT_MODE_PAUSE  = 3
};

/* ISO/IEC 14496-2:2004 table 7-3 */
enum
{
    MP4_BAB_TYPE_MVDSZ_NOUPDATE  = 0,
    MP4_BAB_TYPE_MVDSNZ_NOUPDATE = 1,
    MP4_BAB_TYPE_TRANSPARENT     = 2,
    MP4_BAB_TYPE_OPAQUE          = 3,
    MP4_BAB_TYPE_INTRACAE        = 4,
    MP4_BAB_TYPE_MVDSZ_INTERCAE  = 5,
    MP4_BAB_TYPE_MVDSNZ_INTERCAE = 6
};

#define MP4_DC_MARKER  0x6B001 // 110 1011 0000 0000 0001
#define MP4_MV_MARKER  0x1F001 //   1 1111 0000 0000 0001


/* ISO/IEC 14496-2:2004 table G.1 */
enum
{
    MP4_SIMPLE_PROFILE_LEVEL_1                     = 0x01,
    MP4_SIMPLE_PROFILE_LEVEL_2                     = 0x02,
    MP4_SIMPLE_PROFILE_LEVEL_3                     = 0x03,
    MP4_SIMPLE_PROFILE_LEVEL_4a                    = 0x04,
    MP4_SIMPLE_PROFILE_LEVEL_5                     = 0x05,
    MP4_SIMPLE_PROFILE_LEVEL_6                     = 0x06,
    MP4_SIMPLE_PROFILE_LEVEL_0                     = 0x08,
    MP4_CORE_PROFILE_LEVEL_1                       = 0x21,
    MP4_CORE_PROFILE_LEVEL_2                       = 0x22,
    MP4_MAIN_PROFILE_LEVEL_2                       = 0x32,
    MP4_MAIN_PROFILE_LEVEL_3                       = 0x33,
    MP4_MAIN_PROFILE_LEVEL_4                       = 0x34,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_1  = 0x91,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_2  = 0x92,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_3  = 0x93,
    MP4_ADVANCED_REAL_TIME_SIMPLE_PROFILE_LEVEL_4  = 0x94,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_1 = 0xB1,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_2 = 0xB2,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_3 = 0xB3,
    MP4_ADVANCED_CODING_EFFICIENCY_PROFILE_LEVEL_4 = 0xB4,
    MP4_ADVANCED_CORE_PROFILE_LEVEL_1              = 0xC1,
    MP4_ADVANCED_CORE_PROFILE_LEVEL_2              = 0xC2,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_0            = 0xF0,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_1            = 0xF1,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_2            = 0xF2,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3            = 0xF3,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_4            = 0xF4,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_5            = 0xF5,
    MP4_ADVANCED_SIMPLE_PROFILE_LEVEL_3B           = 0xF7
};

/* Group Of Video Object Plane Info */
typedef struct
{
    uint8_t closed_gov;
    uint8_t broken_link;
    uint8_t time_code_hours;
    uint8_t time_code_minutes;
    uint8_t time_code_seconds;
    uint8_t dummy1;
    uint16_t dummy2;
    uint32_t time_base;
} mp4_GroupOfVideoObjectPlane_t;


/* Video Object Plane Info */
typedef struct
{
    uint8_t     vop_coding_type;
    uint32_t    modulo_time_base;
    uint16_t    vop_time_increment;
    uint8_t     vop_coded;

    uint16_t    vop_id;
    uint16_t    vop_id_for_prediction;
    uint8_t     is_vop_id_for_prediction_indication;
    uint8_t     vop_rounding_type;
    uint8_t     vop_reduced_resolution;
    uint8_t     align_dummy;

    uint16_t    vop_width;
    uint16_t    vop_height;
    uint16_t    vop_horizontal_mc_spatial_ref;
    uint16_t    vop_vertical_mc_spatial_ref;

    uint8_t     background_composition;
    uint8_t     change_conv_ratio_disable;
    uint8_t     is_vop_constant_alpha;
    uint8_t     vop_constant_alpha_value;
    uint8_t     intra_dc_vlc_thr;
    uint8_t     top_field_first;
    uint8_t     alternate_vertical_scan_flag;
    uint8_t     sprite_transmit_mode;

    int32_t     brightness_change_factor;
    uint16_t    vop_quant;
    uint8_t     vop_fcode_forward;
    uint8_t     vop_fcode_backward;

    uint16_t    warping_mv_code_du[4];
    uint16_t    warping_mv_code_dv[4];

} mp4_VideoObjectPlane_t;

/* VOLControlParameters Info */
typedef struct
{
    uint8_t     chroma_format;
    uint8_t     low_delay;
    uint8_t     vbv_parameters;
    uint8_t     align_dummy1;
    uint32_t    bit_rate;
    uint32_t    vbv_buffer_size;
    uint32_t    vbv_occupancy;
} mp4_VOLControlParameters_t;

/* Video Object Plane with short header Info */
typedef struct _mp4_VideoObjectPlaneH263
{
    uint8_t         temporal_reference;
    uint8_t         split_screen_indicator;
    uint8_t         document_camera_indicator;
    uint8_t         full_picture_freeze_release;
    uint8_t         source_format;
    uint8_t         picture_coding_type;
    uint8_t         vop_quant;
    uint16_t        num_gobs_in_vop;
    uint16_t        num_macroblocks_in_gob;
    uint8_t         num_rows_in_gob;
#if 0
    uint8_t         gob_number;
    int             gob_header_empty;
    int             gob_frame_id;
    int             quant_scale;
#endif
    uint8_t         vop_rounding_type;
    //the following are required for PLUSPTYPE
    uint8_t         ufep;
    uint16_t        pixel_aspect_ratio_code;
    uint16_t        picture_width_indication;
    uint16_t        picture_height_indication;
} mp4_VideoObjectPlaneH263;

typedef struct
{
    uint16_t                  sprite_width;
    uint16_t                  sprite_height;
    uint16_t                  sprite_left_coordinate;
    uint16_t                  sprite_top_coordinate;
    uint16_t                  no_of_sprite_warping_points;
    uint16_t                  sprite_warping_accuracy;
    uint16_t                  sprite_brightness_change;
    uint16_t                  low_latency_sprite_enable;
} mp4_VOLSpriteInfo_t;

typedef struct
{
    uint8_t                  load_intra_quant_mat;
    uint8_t                  load_nonintra_quant_mat;
    uint16_t                 align_dummy1;
    uint8_t                  intra_quant_mat[64];
    uint8_t                  nonintra_quant_mat[64];
} mp4_VOLQuant_mat_t;

/* Video Object Layer Info */
typedef struct
{
    uint8_t                     video_object_layer_id; /* Last 4 bits of start code. */
    uint8_t                     short_video_header;
    uint8_t                     random_accessible_vol;
    uint8_t                     video_object_type_indication;

    uint8_t                     is_object_layer_identifier;
    uint8_t                     video_object_layer_verid;
    uint8_t                     video_object_layer_priority;
    uint8_t                     aspect_ratio_info;

    uint8_t                     aspect_ratio_info_par_width;
    uint8_t                     aspect_ratio_info_par_height;
    uint8_t                     align_dummy1;
    uint8_t                     is_vol_control_parameters;

    mp4_VOLControlParameters_t  VOLControlParameters;

    uint8_t                     video_object_layer_shape;
    uint16_t                    vop_time_increment_resolution;
    uint8_t                     vop_time_increment_resolution_bits;

    uint8_t                     fixed_vop_rate;
    uint16_t                    fixed_vop_time_increment;
    uint16_t                    video_object_layer_width;
    uint16_t                    video_object_layer_height;
    uint8_t                     interlaced;

    uint8_t                     obmc_disable;
    uint8_t                     sprite_enable;
    mp4_VOLSpriteInfo_t         sprite_info;
    uint8_t                     not_8_bit;
    uint8_t                     quant_precision;

    uint8_t                     bits_per_pixel;
    uint8_t                     quant_type;
    mp4_VOLQuant_mat_t          quant_mat_info;
    uint8_t                     quarter_sample;
    uint8_t                     complexity_estimation_disable;

    uint8_t                     resync_marker_disable;
    uint8_t                     data_partitioned;
    uint8_t                     reversible_vlc;
    uint8_t                     newpred_enable;

    uint8_t                     reduced_resolution_vop_enable;  // verid != 1
    uint8_t                     scalability;
    uint8_t                     low_latency_sprite_enable;

    mp4_GroupOfVideoObjectPlane_t  GroupOfVideoObjectPlane;
    mp4_VideoObjectPlane_t      VideoObjectPlane;
    mp4_VideoObjectPlaneH263    VideoObjectPlaneH263;

    // for interlaced B-VOP direct mode
    uint32_t                         Tframe;
    // for B-VOP direct mode
    uint32_t                         TRB, TRD;
    // time increment of past and future VOP for B-VOP
    uint32_t                      pastFrameTime, futureFrameTime;
    // VOP global time
    uint32_t                      vop_sync_time, vop_sync_time_b;

} mp4_VideoObjectLayer_t;

/* video_signal_type Info */
typedef struct
{
    uint8_t is_video_signal_type;
    uint8_t video_format;
    uint8_t video_range;
    uint8_t is_colour_description;
    uint8_t colour_primaries;
    uint8_t transfer_characteristics;
    uint8_t matrix_coefficients;
} mp4_VideoSignalType_t;

typedef struct _mp4_Frame {
    long long int    time;
} mp4_Frame;

/* Visual Object Info */
typedef struct
{
    uint8_t                 is_visual_object_identifier;
    uint8_t                 visual_object_verid;
    uint8_t                 visual_object_priority;
    uint8_t                 visual_object_type;
    mp4_VideoSignalType_t   VideoSignalType;
    mp4_VideoObjectLayer_t  VideoObject;

    mp4_Frame               currentFrame;      // current
    mp4_Frame               pastFrame;      // reference in past
    mp4_Frame               futureFrame;      // reference in future
} mp4_VisualObject_t;

/* Full Info */
typedef struct
{
    mp4_VisualObject_t    VisualObject;
    uint8_t               profile_and_level_indication;
} mp4_Info_t;

enum
{
    MP4_SC_SEEN_INVALID = 0x0,
    MP4_SC_SEEN_VOL = 0x1,
    MP4_SC_SEEN_VOP = 0x2,
    MP4_SC_SEEN_SVH = 0x4,
};

enum
{
    MP4_BS_ERROR_NONE =          (0 << 0),
    MP4_BS_ERROR_HDR_PARSE =     (1 << 0),
    MP4_BS_ERROR_HDR_NONDEC =    (1 << 1),
    MP4_BS_ERROR_HDR_UNSUP =     (1 << 2),
    MP4_BS_ERROR_FRM_PARSE =     (1 << 3),
    MP4_BS_ERROR_FRM_NONDEC =    (1 << 4),
    MP4_BS_ERROR_FRM_UNSUP =     (1 << 5),
};

#define MP4_HDR_ERROR_MASK (MP4_BS_ERROR_HDR_PARSE | MP4_BS_ERROR_HDR_NONDEC | MP4_BS_ERROR_HDR_UNSUP)

typedef enum
{
    VIDDEC_MP4_INDX_0 = 0,
    VIDDEC_MP4_INDX_1 = 1,
    VIDDEC_MP4_INDX_2 = 2,
    VIDDEC_MP4_INDX_MAX = 3,
} viddec_fw_mp4_ref_index_t;

typedef struct
{
    uint8_t is_field;
} viddec_mp4_ref_info_t;

typedef struct
{
    // The relevant bitstream data for current stream
    mp4_Info_t info;

    // The previous start code (without the prefix)
    uint32_t   prev_sc;

    // The current start code (without the prefix)
    // TODO: Revisit for SVH
    uint32_t   current_sc;

    // Indicates if we look for both short and long video header or just the long video header
    // If false, sc detection looks for both short and long video headers.
    // If true, long video header has been seen and sc detection does not look for short video header any more.
    uint8_t    ignore_scs;

    // Indicates if the current start code prefix is long (if true).
    uint8_t    cur_sc_prefix;

    // Indicates if the next start code prefix is long (if true).
    uint8_t    next_sc_prefix;

    // Indicates start of a frame
    uint8_t    is_frame_start;

    // Indicates which start codes were seen for this workload
    uint8_t    sc_seen;

    // Indicates bitstream errors if any
    uint16_t    bitstream_error;

    // Reference frame information
    viddec_mp4_ref_info_t ref_frame[VIDDEC_MP4_INDX_MAX];

} viddec_mp4_parser_t;

#define BREAK_GETBITS_FAIL(x, ret) {            \
        if(x == -1){                            \
            FWTRACE;                            \
            ret = MP4_STATUS_PARSE_ERROR;       \
            break;}                             \
    }

#define BREAK_GETBITS_REQD_MISSING(x, ret) {            \
        if(x == -1){                            \
            FWTRACE;                            \
            ret = MP4_STATUS_REQD_DATA_ERROR;       \
            break;}                             \
    }

extern void *memset(void *s, int32_t c, uint32_t n);

uint32_t viddec_fw_mp4_emit_workload(void *parent, void *ctxt);

void mp4_set_hdr_bitstream_error(viddec_mp4_parser_t *parser, uint8_t hdr_flag, mp4_Status_t parse_status);

#endif
