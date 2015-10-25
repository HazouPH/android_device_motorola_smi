#ifndef _MPEG2_H
#define _MPEG2_H

/**
 * mpeg2.h
 * -------
 * This file contains all the necessary enumerations and structures needed from
 * the MPEG-2 Specification.
 */

/* Max Pan-Scan offsets */
#define MPEG2_MAX_VID_OFFSETS 3

/* Quantization matrix size */
#define MPEG2_QUANT_MAT_SIZE  64

/* MPEG2 Start Code Values */
typedef enum {
    MPEG2_SC_PICTURE           = 0x00,
    MPEG2_SC_SLICE_HDR         = 0x01,
    MPEG2_SC_SLICE_MIN         = 0x01,
    MPEG2_SC_SLICE_MAX         = 0xAF,
    MPEG2_SC_USER_DATA         = 0xB2,
    MPEG2_SC_SEQ_HDR           = 0xB3,
    MPEG2_SC_SEQ_ERR           = 0xB4,
    MPEG2_SC_EXT               = 0xB5,
    MPEG2_SC_SEQ_END           = 0xB7,
    MPEG2_SC_GROUP             = 0xB8,
    MPEG2_SC_SYS_MIN           = 0xB9,
    MPEG2_SC_SYS_MAX           = 0xFF,
    MPEG2_SC_ALL               = 0xFF
} mpeg2_start_codes;

/* MPEG2 Extension Start Code ID */
typedef enum {
    MPEG2_EXT_SEQ              = 1,
    MPEG2_EXT_SEQ_DISP         = 2,
    MPEG2_EXT_QUANT_MAT        = 3,
    MPEG2_EXT_COPYRIGHT        = 4,
    MPEG2_EXT_SEQ_SCAL         = 5,
    MPEG2_EXT_PIC_DISP         = 7,
    MPEG2_EXT_PIC_CODING       = 8,
    MPEG2_EXT_PIC_SPA_SCAL     = 9,
    MPEG2_EXT_PIC_TEMP_SCAL    = 10,
    MPEG2_EXT_ALL              = 11
} mpeg2_ext_start_codes;

/* MPEG2 Picture Coding Type Values */
typedef enum {
    MPEG2_PC_TYPE_FORBIDDEN    = 0,
    MPEG2_PC_TYPE_I            = 1,
    MPEG2_PC_TYPE_P            = 2,
    MPEG2_PC_TYPE_B            = 3
} mpeg2_picture_type;

/* MPEG2 Picture Structure Type Values */
typedef enum {
    MPEG2_PIC_STRUCT_RESERVED  = 0,
    MPEG2_PIC_STRUCT_TOP       = 1,
    MPEG2_PIC_STRUCT_BOTTOM    = 2,
    MPEG2_PIC_STRUCT_FRAME     = 3
} mpeg2_picture_structure;

/* MPEG2 Chroma Format Values */
typedef enum {
    MPEG2_CF_RESERVED   = 0,
    MPEG2_CF_420        = 1,
    MPEG2_CF_422        = 2,
    MPEG2_CF_444        = 3
} mpeg2_chroma_format;

/* MPEG2 Parser Structures */
/* Sequence Header Info */
struct mpeg2_sequence_hdr_info
{
    uint32_t   horizontal_size_value;
    uint32_t   vertical_size_value;
    uint32_t   aspect_ratio_information;
    uint32_t   frame_rate_code;
    uint32_t   bit_rate_value;
    uint32_t   vbv_buffer_size_value;
    uint32_t   constrained_parameters_flag;
};

/* Group of Pictures Header Info */
struct mpeg2_gop_hdr_info
{
    uint32_t   closed_gop;
    uint32_t   broken_link;
};

/* Picture Header */
struct mpeg2_picture_hdr_info
{
    uint32_t   temporal_reference;
    uint32_t   picture_coding_type;
    uint32_t   full_pel_forward_vect;
    uint32_t   forward_f_code;
    uint32_t   full_pel_backward_vect;
    uint32_t   backward_f_code;
};

/* Sequence Extension Info */
struct mpeg2_sequence_ext_info
{
    uint32_t   profile_and_level_indication;
    uint32_t   progressive_sequence;
    uint32_t   chroma_format;
    uint32_t   horizontal_size_extension;
    uint32_t   vertical_size_extension;
    uint32_t   bit_rate_extension;
    uint32_t   vbv_buffer_size_extension;
    uint32_t   frame_rate_extension_n;
    uint32_t   frame_rate_extension_d;
};

/* Sequence Display Extension Info */
struct mpeg2_sequence_disp_ext_info
{
    uint32_t  video_format;
    uint32_t  colour_description;
    uint32_t  colour_primaries;
    uint32_t  transfer_characteristics;
    uint32_t  display_horizontal_size;
    uint32_t  display_vertical_size;
};

/* Sequence scalable extension Info */
struct mpeg2_sequence_scal_ext_info
{
    uint32_t  scalable_mode;
};

/* Picture Coding Extension */
struct mpeg2_picture_coding_ext_info
{
    uint32_t fcode00;
    uint32_t fcode01;
    uint32_t fcode10;
    uint32_t fcode11;
    uint32_t intra_dc_precision;
    uint32_t picture_structure;
    uint32_t top_field_first;
    uint32_t frame_pred_frame_dct;
    uint32_t concealment_motion_vectors;
    uint32_t q_scale_type;
    uint32_t intra_vlc_format;
    uint32_t alternate_scan;
    uint32_t repeat_first_field;
    uint32_t chroma_420_type;
    uint32_t progressive_frame;
    uint32_t composite_display_flag;
};

/* Picture Display Extension */
struct mpeg2_picture_disp_ext_info
{
    uint32_t frame_center_horizontal_offset[MPEG2_MAX_VID_OFFSETS];
    uint32_t frame_center_vertical_offset[MPEG2_MAX_VID_OFFSETS];
};

/* Quantization Matrix Extension */
struct mpeg2_quant_ext_info
{
    uint32_t load_intra_quantiser_matrix;
    uint32_t load_non_intra_quantiser_matrix;
    uint32_t load_chroma_intra_quantiser_matrix;
    uint32_t load_chroma_non_intra_quantiser_matrix;
};

/* Quantization Matrices */
struct mpeg2_quant_matrices
{
    uint8_t intra_quantiser_matrix[MPEG2_QUANT_MAT_SIZE];
    uint8_t non_intra_quantiser_matrix[MPEG2_QUANT_MAT_SIZE];
    uint8_t chroma_intra_quantiser_matrix[MPEG2_QUANT_MAT_SIZE];
    uint8_t chroma_non_intra_quantiser_matrix[MPEG2_QUANT_MAT_SIZE];
};

/* MPEG2 Info */
struct mpeg2_info
{
    struct mpeg2_sequence_hdr_info         seq_hdr;
    struct mpeg2_gop_hdr_info              gop_hdr;
    struct mpeg2_picture_hdr_info          pic_hdr;
    struct mpeg2_sequence_ext_info         seq_ext;
    struct mpeg2_sequence_disp_ext_info    seq_disp_ext;
    struct mpeg2_sequence_scal_ext_info    seq_scal_ext;
    struct mpeg2_picture_coding_ext_info   pic_cod_ext;
    struct mpeg2_picture_disp_ext_info     pic_disp_ext;
    struct mpeg2_quant_ext_info            qnt_ext;
    struct mpeg2_quant_matrices            qnt_mat;
};

#endif
