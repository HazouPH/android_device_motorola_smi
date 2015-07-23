/**
 * viddec_mpeg2_metadata.c
 * -----------------------
 * This file contains all the routines to parse the information from MPEG2
 * elementary stream and store it in the parser context. Based on the data
 * parsed, the state information in the context is updated.
 *
 * Headers currently parsed from MPEG2 stream include:
 * - Sequence Header
 * - Sequence Extension
 * - Sequence Display Extension
 * - GOP Header
 * - Picture Header
 * - Picture Coding Extension
 * - Quantization Matrix Extension
 * - Picture Display Extension
 *
 * The slice data is parsed and appended into workload in viddec_mpeg2_parse.c
 */

#include "viddec_mpeg2.h"

/* Default quantization matrix values */
const uint8_t mpeg2_default_intra_quant_matrix[MPEG2_QUANT_MAT_SIZE] = {
    8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};
const uint8_t mpeg2_default_non_intra_quant_matrix[MPEG2_QUANT_MAT_SIZE] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};

/* Matrix for converting scan order */
const uint8_t mpeg2_classic_scan[MPEG2_QUANT_MAT_SIZE] = {
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};
const uint8_t mpeg2_alternate_scan[MPEG2_QUANT_MAT_SIZE] = {
    0,  8, 16, 24,  1,  9,  2, 10,
    17, 25, 32, 40, 48, 56, 57, 49,
    41, 33, 26, 18,  3, 11,  4, 12,
    19, 27, 34, 42, 50, 58, 35, 43,
    51, 59, 20, 28,  5, 13,  6, 14,
    21, 29, 36, 44, 52, 60, 37, 45,
    53, 61, 22, 30,  7, 15, 23, 31,
    38, 46, 54, 62, 39, 47, 55, 63
};

/* Look-up tables for macro block address increment VLC */
const uint8_t mb_addr_inc_tab1[16] = {
    0, 0, 7, 6, 5, 5, 4, 4,
    3, 3, 3, 3, 2, 2, 2, 2
};
const uint8_t mb_addr_inc_tab2[8] = {
    13, 12, 11, 10, 9, 9, 8, 8
};
const uint8_t mb_addr_inc_tab3[40] = {
    33, 32, 31, 30, 29, 28, 27, 26,
    25, 24, 23, 22, 21, 21, 20, 20,
    19, 19, 18, 18, 17, 17, 16, 16,
    15, 15, 15, 15, 15, 15, 15, 15,
    14, 14, 14, 14, 14, 14, 14, 14
};

/* viddec_mpeg2_copy_default_matrix() - Copies quantization matrix from src  */
/* to dst                                                                    */
static inline void mpeg2_copy_matrix(const uint8_t *src, uint8_t *dst)
{
    register uint32_t index = 0;
    for (index=0; index < MPEG2_QUANT_MAT_SIZE; index++)
        dst[index] = src[index];
}

/* viddec_mpeg2_copy_matrix() - Copies next 64bytes in the stream into given */
/* matrix                                                                    */
static inline int32_t mpeg2_get_quant_matrix(void *parent, uint8_t *matrix, uint32_t alternate_scan)
{
    int32_t ret = 1;
    uint32_t index = 0, code = 0;
    const uint8_t  *zigzag_scan = (const uint8_t *) mpeg2_classic_scan;

    if (alternate_scan)
    {
        zigzag_scan = (const uint8_t *) mpeg2_alternate_scan;
    }

    /* Start extracting matrix co-efficients and copy them in */
    /* inverse zigzag scan order */
    for (index = 0; index < MPEG2_QUANT_MAT_SIZE; index++)
    {
        ret = viddec_pm_get_bits(parent, &code, MPEG2_BITS_EIGHT);
        /* Quantization values cannot be zero. If zero value if found, */
        /* further parsing is stopped and the existing values are used.*/
        if ((ret != 1) || (code == 0))
        {
            ret = -1;
            break;
        }
        matrix[zigzag_scan[index]] = (uint8_t)(code & 0xFF);
    }

    return ret;
}

/* viddec_mpeg2_parse_seq_hdr() - Parse sequence header metadata and store   */
/* in parser context                                                         */
void viddec_mpeg2_parse_seq_hdr(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get Horizontal Frame Size */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.horizontal_size_value, 12);

    /* Get Vertical Frame Size */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.vertical_size_value, 12);

    /* Get Frame Aspect Ratio */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.aspect_ratio_information, 4);

    /* Get Frame Rate */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.frame_rate_code, 4);

    /* Get Bit Rate */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.bit_rate_value, 18);

    /* Skip Marker bit */
    ret_code |= viddec_pm_skip_bits(parent, 1);

    /* Get VBV Buffer Size Value */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.vbv_buffer_size_value, 10);

    /* Get Constrained Parameters Flag */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_hdr.constrained_parameters_flag, 1);

    /* Quantization Matrix Support */
    /* Get Intra Quantizer matrix, if available or use default values */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent, parser->info.qnt_mat.intra_quantiser_matrix, 0);
        mpeg2_copy_matrix(parser->info.qnt_mat.intra_quantiser_matrix, parser->info.qnt_mat.chroma_intra_quantiser_matrix);
    }
    else
    {
        if (!parser->mpeg2_custom_qmat_parsed)
        {
            mpeg2_copy_matrix(mpeg2_default_intra_quant_matrix, parser->info.qnt_mat.intra_quantiser_matrix);
            mpeg2_copy_matrix(mpeg2_default_intra_quant_matrix, parser->info.qnt_mat.chroma_intra_quantiser_matrix);
        }
    }

    /* Get Non-Intra Qualtizer matrix, if available or use default values */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_non_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_non_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent, parser->info.qnt_mat.non_intra_quantiser_matrix, 0);
        mpeg2_copy_matrix(parser->info.qnt_mat.non_intra_quantiser_matrix, parser->info.qnt_mat.chroma_non_intra_quantiser_matrix);
    }
    else
    {
        if (!parser->mpeg2_custom_qmat_parsed)
        {
            mpeg2_copy_matrix(mpeg2_default_non_intra_quant_matrix, parser->info.qnt_mat.non_intra_quantiser_matrix);
            mpeg2_copy_matrix(mpeg2_default_non_intra_quant_matrix, parser->info.qnt_mat.chroma_non_intra_quantiser_matrix);
        }
    }

    /* Error handling */
    /* The return value from get_bits() function is accumulated. If the return value is not 1, */
    /* then there was an error getting the required information from the stream and the status */
    /* is updated for the current workload. */
    if (ret_code == 1)
    {
        /* This flag indicates a valid sequence header has been parsed and so even if */
        /* a sequence haeder is corrupted in the future, this valid sequence header   */
        /* could be reused. */
        parser->mpeg2_valid_seq_hdr_parsed = true;
        /* This flag indicates a valid custom quantization matrix has been parsed.  */
        /* So, if in the future, there is an error parsing quantization matrix, the */
        /* parser will use the previously parsed custom values. */
        if ((parser->info.qnt_ext.load_intra_quantiser_matrix)
                || (parser->info.qnt_ext.load_non_intra_quantiser_matrix))
        {
            parser->mpeg2_custom_qmat_parsed = true;
        }
        MPEG2_DEB("Seqeunce header parsed successfully.\n");
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_SEQ_HDR;
        MPEG2_DEB("Sequence header corrupted.\n");
    }

    parser->mpeg2_stream               = false;
    parser->mpeg2_curr_seq_headers    |= MPEG2_HEADER_SEQ;
    parser->mpeg2_curr_frame_headers  |= MPEG2_HEADER_SEQ;
    parser->mpeg2_stream_level         = MPEG2_LEVEL_SEQ;

    return;
}

/* viddec_mpeg2_parse_gop_hdr() - Parse group of pictures header info and    */
/* store it in parser context                                                */
void viddec_mpeg2_parse_gop_hdr(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Skip first 25 bits */
    /* Skip time_code */
    ret_code |= viddec_pm_skip_bits(parent, 25);

    /* Get closed gop info */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.gop_hdr.closed_gop, 1);

    /* Get broken link info */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.gop_hdr.broken_link, 1);

    if (ret_code == 1)
    {
        MPEG2_DEB("GOP Header parsed successfully.\n");
    }
    else
    {
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_GOP_HDR;
        MPEG2_DEB("GOP header corrupted.\n");
    }

    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_GOP;
    parser->mpeg2_stream_level        = MPEG2_LEVEL_GOP;

    return;
}

/* viddec_mpeg2_parse_pic_hdr() - Parse picture header info and store it in  */
/* parser context                                                            */
void viddec_mpeg2_parse_pic_hdr(void *parent, void *ctxt)
{
    int32_t ret_code = 0, found_error = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get Temporal Reference info */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.temporal_reference, 10);

    /* Get Picture Coding type and skip the following byte */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.picture_coding_type, 3);

    /* Error Handling and Concealment */
    /* Picture coding type should be one I, P or B */
    if ((parser->info.pic_hdr.picture_coding_type != MPEG2_PC_TYPE_I) &&
            (parser->info.pic_hdr.picture_coding_type != MPEG2_PC_TYPE_P) &&
            (parser->info.pic_hdr.picture_coding_type != MPEG2_PC_TYPE_B))
    {
        found_error = 1;
    }
    /* The first frame after a gop header should be a coded I picture as per */
    /* section 6.3.1 in MPEG2 Specification. */
    else if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_GOP)
    {
        if (parser->info.pic_hdr.picture_coding_type != MPEG2_PC_TYPE_I)
        {
            found_error = 1;
        }
    }
    /* The first frame after a sequence header cannot be a coded B picture as per */
    /* section 6.1.1.6 in MPEG2 Specification. */
    else if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_SEQ)
    {
        if (parser->info.pic_hdr.picture_coding_type == MPEG2_PC_TYPE_B)
        {
            found_error = 1;
        }
    }

    /* If there is an error parsing picture coding type, do error concealment and continue. */
    if ((ret_code != 1) || (found_error))
    {
        if (found_error)
        {
            /* Setting status to mark parser error while emitting the current workload. */
            parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_PIC_HDR;
            MPEG2_DEB("Picture header corrupted.\n");
        }

        /* Error concealment for picture coding type - Default to I picture. */
        parser->info.pic_hdr.picture_coding_type = MPEG2_PC_TYPE_I;
        parser->mpeg2_wl_status |= MPEG2_WL_CONCEALED_PIC_COD_TYPE;
        MPEG2_DEB("Picture Coding Type corrupted. Concealing to I type.\n");
    }

    /* Skip next 16 bits */
    /* Skip vbv_delay */
    ret_code |= viddec_pm_skip_bits(parent, 16);

    /* If Picture Coding type is either P or B then */
    /* Get forward vector code */
    if ((MPEG2_PC_TYPE_P == parser->info.pic_hdr.picture_coding_type) ||
            (MPEG2_PC_TYPE_B == parser->info.pic_hdr.picture_coding_type))
    {
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.full_pel_forward_vect, 1);
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.forward_f_code, 3);
    }
    else
    {
        parser->info.pic_hdr.full_pel_forward_vect = 0;
        parser->info.pic_hdr.forward_f_code        = 0;
    }

    /* If Picture coding type is B then */
    /*    Get backward vector code */
    if (MPEG2_PC_TYPE_B == parser->info.pic_hdr.picture_coding_type)
    {
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.full_pel_backward_vect, 1);
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_hdr.backward_f_code, 3);
    }
    else
    {
        parser->info.pic_hdr.full_pel_backward_vect = 0;
        parser->info.pic_hdr.backward_f_code        = 0;
    }

    if (ret_code == 1)
    {
        MPEG2_DEB("Picture header parsed successfully.\n")
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_PIC_HDR;
        MPEG2_DEB("Picture header corrupted.\n");
    }

    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_PIC;
    parser->mpeg2_stream_level        = MPEG2_LEVEL_PIC;

    return;
}

/* viddec_mpeg2_parse_ext_seq() - Parse Sequence extension metadata and      */
/* store in parser context                                                   */
void viddec_mpeg2_parse_ext_seq(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get Profile and Level info */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.profile_and_level_indication, 8);

    /* Get Progressive Sequence Flag */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.progressive_sequence, 1);

    /* Get Chroma Format */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.chroma_format, 2);

    /* Error Concealment */
    /* If there is an error parsing chroma format, do error concealment and continue. */
    if ((ret_code != 1) || (parser->info.seq_ext.chroma_format == MPEG2_CF_RESERVED))
    {
        if (parser->info.seq_ext.chroma_format == MPEG2_CF_RESERVED)
        {
            /* Setting status to mark parser error while emitting the current workload. */
            parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_SEQ_EXT;
            MPEG2_DEB("Sequence extension corrupted.\n")
        }

        /* Error concealment for chroma format - Default to 4:2:0 */
        parser->info.seq_ext.chroma_format = MPEG2_CF_420;
        parser->mpeg2_wl_status |= MPEG2_WL_CONCEALED_CHROMA_FMT;
        MPEG2_DEB("Chroma Format corrupted. Concealing to 4:2:0.\n");
    }

    /* Get Content Size Extension Data */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.horizontal_size_extension, 2);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.vertical_size_extension, 2);

    /* Get Bit Rate Extension */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.bit_rate_extension, 12);

    /* Skip Marker bit */
    ret_code |= viddec_pm_skip_bits(parent, 1);

    /* Get VBV Buffer Size Extension Data */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.vbv_buffer_size_extension, 8);

    /* Skip 1 bit */
    /* Skip low_delay */
    ret_code |= viddec_pm_skip_bits(parent, 1);

    /* Get Frame Rate extension data */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.frame_rate_extension_n, 2);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_ext.frame_rate_extension_d, 5);

    if (ret_code == 1)
    {
        MPEG2_DEB("Sequence extension header parsed successfully.\n")
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_SEQ_EXT;
        MPEG2_DEB("Sequence extension corrupted.\n")
    }

    /* Check if the last parsed start code was that of sequence header. */
    /* If true, seq extension followed seq header => MPEG2 Stream */
    parser->mpeg2_stream = (parser->mpeg2_last_parsed_sc == MPEG2_SC_SEQ_HDR) ? true:false;
    parser->mpeg2_curr_seq_headers   |= MPEG2_HEADER_SEQ_EXT;
    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_SEQ_EXT;

    return;
}

/* viddec_mpeg2_parse_ext_seq_disp() - Parse Sequence Display extension      */
/* metadata and store in parser context                                      */
void viddec_mpeg2_parse_ext_seq_disp(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get video format */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.video_format, 3);

    /* Check if color description info is present */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.colour_description, 1);

    /* If color description is found, get color primaries info */
    /* and transfer characteristics */
    if (parser->info.seq_disp_ext.colour_description)
    {
        ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.colour_primaries, 8);
        ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.transfer_characteristics, 8);
        ret_code |= viddec_pm_skip_bits(parent, 8);
    }

    /* Get Display Horizontal Size */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.display_horizontal_size, 14);
    ret_code |= viddec_pm_skip_bits(parent, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_disp_ext.display_vertical_size, 14);

    if (ret_code == 1)
    {
        MPEG2_DEB("Sequence display extension parsed successfully.\n");
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_SEQ_DISP_EXT;
        MPEG2_DEB("Sequence display extension corrupted.\n")
    }

    /* Set flag to indicate Sequence Display Extension is present */
    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_SEQ_DISP_EXT;
    parser->mpeg2_curr_seq_headers   |= MPEG2_HEADER_SEQ_DISP_EXT;

    return;
}

/* viddec_mpeg2_parse_ext_seq_scal() - Parse Sequence Scalable extension     */
/* metadata and store in parser context                                      */
void viddec_mpeg2_parse_ext_seq_scal(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get video format */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.seq_scal_ext.scalable_mode, 2);

    if (ret_code == 1)
    {
        MPEG2_DEB("Sequence scalable extension parsed successfully.\n");
    }

    /* Set flag to indicate Sequence Display Extension is present */
    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_SEQ_SCAL_EXT;
    parser->mpeg2_curr_seq_headers   |= MPEG2_HEADER_SEQ_SCAL_EXT;

    return;
}

/* viddec_mpeg2_parse_ext_pic() - Parse Picture Coding extension             */
/* metadata and store in parser context                                      */
void viddec_mpeg2_parse_ext_pic(void *parent, void *ctxt)
{
    int32_t ret_code = 0, found_error = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get Forward/Backward, Horizontal/Vertical codes */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.fcode00, 4);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.fcode01, 4);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.fcode10, 4);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.fcode11, 4);

    /* Get Intra DC Precision */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.intra_dc_precision, 2);

    /* Get Picture Structure */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.picture_structure,  2);

    /* Error Handling and Concealment */
    /* Picture structure should be frame, top field or bottom field */
    if (parser->info.pic_cod_ext.picture_structure == MPEG2_PIC_STRUCT_RESERVED)
    {
        found_error = 1;
    }
    /* All pictures in progressive sequence should be frame picture */
    else if (parser->info.seq_ext.progressive_sequence)
    {
        if (parser->info.pic_cod_ext.picture_structure != MPEG2_PIC_STRUCT_FRAME)
        {
            found_error = 1;
        }
    }

    /* If there is an error parsing picture structure, do error concealment and continue. */
    if ((ret_code != 1) || (found_error))
    {
        if (found_error)
        {
            /* Setting status to mark parser error while emitting the current workload. */
            parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_PIC_COD_EXT;
            MPEG2_DEB("Picture coding extension corrupted.\n");
        }

        /* Error concealment for picture structure - Default to frame picture. */
        parser->info.pic_cod_ext.picture_structure = MPEG2_PIC_STRUCT_FRAME;
        parser->mpeg2_wl_status |= MPEG2_WL_CONCEALED_PIC_STRUCT;
        MPEG2_DEB("Picture Structure corrupted. Concealing to Frame picture.\n");
    }

    /* Get flags */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.top_field_first, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.frame_pred_frame_dct, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.concealment_motion_vectors, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.q_scale_type, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.intra_vlc_format, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.alternate_scan, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.repeat_first_field, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.chroma_420_type, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.progressive_frame, 1);
    ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_cod_ext.composite_display_flag, 1);

    /* Error concealment for frame picture */
    if ((parser->info.pic_cod_ext.top_field_first)
            || (parser->info.pic_cod_ext.frame_pred_frame_dct)
            || (parser->info.pic_cod_ext.repeat_first_field)
            || (parser->info.pic_cod_ext.progressive_frame))
    {
        if (parser->info.pic_cod_ext.picture_structure != MPEG2_PIC_STRUCT_FRAME)
        {
            parser->info.pic_cod_ext.picture_structure = MPEG2_PIC_STRUCT_FRAME;
            parser->mpeg2_wl_status |= MPEG2_WL_CONCEALED_PIC_STRUCT;
            MPEG2_DEB("Picture Structure corrupted. Concealing to Frame picture.\n");
        }
    }

    if (ret_code == 1)
    {
        MPEG2_DEB("Picture coding extension parsed successfully.\n");
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_PIC_COD_EXT;
        MPEG2_DEB("Picture coding extension corrupted.\n");
    }

    /* Dangling field detection */
    /* If the previous picture is the first field, then the temporal reference number */
    /* should match with the second field. Otherwise, one of the fields in the previous */
    /* picture is missing and dangling field error is marked. The workload containing */
    /* the previous picture is emitted out and current picture data is added to the next */
    /* workload. The mpeg2_use_next_workload variable is used as a flag to direct the */
    /* items into the current/next workload. */
    if ((parser->mpeg2_picture_interlaced) && (parser->mpeg2_first_field))
    {
        if (parser->mpeg2_prev_temp_ref != parser->info.pic_hdr.temporal_reference)
        {
            /* Mark dangling field info in workload status */
            parser->mpeg2_wl_status |= MPEG2_WL_DANGLING_FIELD;
            if (parser->mpeg2_prev_picture_structure == MPEG2_PIC_STRUCT_BOTTOM)
            {
                parser->mpeg2_wl_status |= MPEG2_WL_DANGLING_FIELD_TOP;
            }
            else
            {
                parser->mpeg2_wl_status |= MPEG2_WL_DANGLING_FIELD_BOTTOM;
            }
            /* Set flag stating current workload is done */
            parser->mpeg2_pic_metadata_complete = true;
            /* Set flag to use the next workload for adding workitems for */
            /* the current frame */
            parser->mpeg2_use_next_workload = true;
            /* Toggle first field flag to compensate for missing field */
            parser->mpeg2_first_field = (parser->mpeg2_first_field) ? false : true;
        }
        else
        {
            /* Same field repeated */
            if (parser->mpeg2_prev_picture_structure == parser->info.pic_cod_ext.picture_structure)
            {
                /* Mark unsupported in workload status */
                parser->mpeg2_wl_status |= MPEG2_WL_REPEAT_FIELD;
            }
        }
    }

    /* Set context variables for interlaced picture handling */
    if (parser->info.pic_cod_ext.picture_structure == MPEG2_PIC_STRUCT_FRAME)
    {
        /* Frame picture found. Reset variables used for interlaced fields picture. */
        parser->mpeg2_picture_interlaced = false;
        parser->mpeg2_first_field        = false;
        parser->mpeg2_use_next_workload  = false;
    }
    else
    {
        /* Interlaced fields picture found. */
        parser->mpeg2_picture_interlaced = true;
        parser->mpeg2_first_field = (parser->mpeg2_first_field) ? false : true;
    }

    /* Set flags */
    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_PIC_COD_EXT;
    parser->mpeg2_prev_temp_ref = parser->info.pic_hdr.temporal_reference;
    parser->mpeg2_prev_picture_structure = parser->info.pic_cod_ext.picture_structure;
    if ((!parser->mpeg2_picture_interlaced)
            || ((parser->mpeg2_picture_interlaced) && (parser->mpeg2_first_field)))
    {
        parser->mpeg2_frame_start = true;
    }

    return;
}

/* viddec_mpeg2_parse_ext_pic_disp() - Parse Picture Display extension       */
/* metadata and store in parser context                                      */
void viddec_mpeg2_parse_ext_pic_disp(void *parent, void *ctxt)
{
    int32_t ret_code = 0;
    uint32_t index = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Determine number of offsets */
    if (parser->info.seq_ext.progressive_sequence)
    {
        if (parser->info.pic_cod_ext.repeat_first_field)
        {
            parser->mpeg2_num_pan_scan_offsets =
                (parser->info.pic_cod_ext.top_field_first) ? 3 : 2;
        }
        else /* Not repeat field */
            parser->mpeg2_num_pan_scan_offsets = 1;
    }
    else /* Not progressive sequence */
    {
        /* Check if picture structure is a field */
        if ((parser->info.pic_cod_ext.picture_structure == MPEG2_PIC_STRUCT_TOP) ||
                (parser->info.pic_cod_ext.picture_structure == MPEG2_PIC_STRUCT_BOTTOM))
        {
            parser->mpeg2_num_pan_scan_offsets = 1;
        }
        else
        {
            parser->mpeg2_num_pan_scan_offsets =
                (parser->info.pic_cod_ext.repeat_first_field) ? 3 : 2;
        }
    }

    /* Get the offsets */
    for (index = 0; index < parser->mpeg2_num_pan_scan_offsets; index++)
    {
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_disp_ext.frame_center_horizontal_offset[index], 16);
        ret_code |= viddec_pm_skip_bits(parent, 1);
        ret_code |= viddec_pm_get_bits(parent, &parser->info.pic_disp_ext.frame_center_vertical_offset[index], 16);
        ret_code |= viddec_pm_skip_bits(parent, 1);
    }

    if (ret_code == 1)
    {
        MPEG2_DEB("Picture display extension parsed successfully.\n");
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_PIC_DISP_EXT;
        MPEG2_DEB("Picture display extension corrupted.\n");
    }

    /* Set flag to indicate picture display extension is found */
    parser->mpeg2_curr_frame_headers |= MPEG2_HEADER_PIC_DISP_EXT;
    return;
}

/* viddec_mpeg2_parse_ext_quant() - Parse Quantization Matrix extension      */
/* metadata and store in parser context                                      */
void viddec_mpeg2_parse_ext_quant(void *parent, void *ctxt)
{
    int32_t ret_code = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Quantization Matrix Support */
    /* Get Intra Quantizer matrix, if available or use default values */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent,
                                           parser->info.qnt_mat.intra_quantiser_matrix,
                                           parser->info.pic_cod_ext.alternate_scan);
        mpeg2_copy_matrix(parser->info.qnt_mat.intra_quantiser_matrix,
                          parser->info.qnt_mat.chroma_intra_quantiser_matrix);
    }

    /* Get Non-Intra Qualtizer matrix, if available */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_non_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_non_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent,
                                           parser->info.qnt_mat.non_intra_quantiser_matrix,
                                           parser->info.pic_cod_ext.alternate_scan);
        mpeg2_copy_matrix(parser->info.qnt_mat.non_intra_quantiser_matrix,
                          parser->info.qnt_mat.chroma_non_intra_quantiser_matrix);
    }

    /* Get Chroma Intra Quantizer matrix, if available */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_chroma_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_chroma_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent,
                                           parser->info.qnt_mat.chroma_intra_quantiser_matrix,
                                           parser->info.pic_cod_ext.alternate_scan);
    }

    /* Get Chroma Non-Intra Quantizer matrix, if available */
    ret_code |= viddec_pm_get_bits(parent, &parser->info.qnt_ext.load_chroma_non_intra_quantiser_matrix, 1);
    if (parser->info.qnt_ext.load_chroma_non_intra_quantiser_matrix)
    {
        ret_code |= mpeg2_get_quant_matrix(parent,
                                           parser->info.qnt_mat.chroma_non_intra_quantiser_matrix,
                                           parser->info.pic_cod_ext.alternate_scan);
    }

    if (ret_code == 1)
    {
        MPEG2_DEB("Quantization matrix extension parsed successfully.\n");
    }
    else
    {
        /* Setting status to mark parser error while emitting the current workload. */
        parser->mpeg2_wl_status |= MPEG2_WL_CORRUPTED_QMAT_EXT;
        MPEG2_DEB("Quantization matrix extension corrupted.\n");
    }

    /* Set quantization matrices updated flag */
    if ( (parser->info.qnt_ext.load_intra_quantiser_matrix) ||
            (parser->info.qnt_ext.load_non_intra_quantiser_matrix) ||
            (parser->info.qnt_ext.load_chroma_intra_quantiser_matrix) ||
            (parser->info.qnt_ext.load_chroma_non_intra_quantiser_matrix) )
    {
        MPEG2_DEB("Custom quantization matrix found.\n");
    }

    return;
}

/* viddec_mpeg2_parse_ext() - Parse extension metadata and store in parser   */
/* context                                                                   */
void viddec_mpeg2_parse_ext(void *parent, void *ctxt)
{
    uint32_t ext_code  = 0;

    /* Get extension start code */
    viddec_pm_get_bits(parent, &ext_code, 4);

    /* Switch on extension type */
    switch ( ext_code )
    {
        /* Sequence Extension Info */
    case MPEG2_EXT_SEQ:
        viddec_mpeg2_parse_ext_seq(parent, ctxt);
        break;

        /* Sequence Display Extension info */
    case MPEG2_EXT_SEQ_DISP:
        viddec_mpeg2_parse_ext_seq_disp(parent, ctxt);
        break;

    case MPEG2_EXT_SEQ_SCAL:
        viddec_mpeg2_parse_ext_seq_scal(parent, ctxt);
        break;

        /* Picture Coding Extension */
    case MPEG2_EXT_PIC_CODING:
        viddec_mpeg2_parse_ext_pic(parent, ctxt);
        break;

        /* Picture Display Extension */
    case MPEG2_EXT_PIC_DISP:
        viddec_mpeg2_parse_ext_pic_disp(parent, ctxt);
        break;

        /*  Quantization Extension*/
    case MPEG2_EXT_QUANT_MAT:
        viddec_mpeg2_parse_ext_quant(parent, ctxt);
        break;

    default:
        break;
    } /* Switch, on extension type */

    return;
}

/* viddec_mpeg2_parse_ext() - Parse user data and append to workload.        */
void viddec_mpeg2_parse_and_append_user_data(void *parent, void *ctxt)
{
    uint32_t user_data = 0;
    viddec_workload_item_t wi;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Set the user data level (SEQ/GOP/PIC) in the workitem type. */
    switch (parser->mpeg2_stream_level)
    {
    case MPEG2_LEVEL_SEQ:
    {
        wi.vwi_type = VIDDEC_WORKLOAD_SEQ_USER_DATA;
        break;
    }
    case MPEG2_LEVEL_GOP:
    {
        wi.vwi_type = VIDDEC_WORKLOAD_GOP_USER_DATA;
        break;
    }
    case MPEG2_LEVEL_PIC:
    {
        wi.vwi_type = VIDDEC_WORKLOAD_FRM_USER_DATA;
        break;
    }
    default:
    {
        wi.vwi_type = VIDDEC_WORKLOAD_INVALID;
        break;
    }
    }

    /* Read 1 byte of user data and store it in workitem for the current      */
    /* stream level (SEQ/GOP/PIC). Keep adding data payloads till it reaches  */
    /* size 11. When it is 11, the maximum user data payload size, append the */
    /* workitem. This loop is repeated till all user data is extracted and    */
    /* appended. */
    wi.user_data.size = 0;
    memset(&(wi.user_data), 0, sizeof(wi.user_data));
    while (viddec_pm_get_bits(parent, &user_data, MPEG2_BITS_EIGHT) != -1)
    {
        /* Store the valid byte in data payload */
        wi.user_data.data_payload[wi.user_data.size] = user_data;
        wi.user_data.size++;

        /* When size exceeds payload size, append workitem and continue */
        if (wi.user_data.size >= 11)
        {
            viddec_pm_setup_userdata(&wi);
            viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
            viddec_fw_reset_workload_item(&wi);
            wi.user_data.size = 0;
        }
    }
    /* If size is not 0, append remaining user data. */
    if (wi.user_data.size > 0)
    {
        viddec_pm_setup_userdata(&wi);
        viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
        wi.user_data.size = 0;
    }

    MPEG2_DEB("User data @ Level %d found.\n", parser->mpeg2_stream_level);
    return;
}

static inline uint32_t get_mb_addr_increment(uint32_t *data)
{
    if (*data >= 1024)
    {
        return 1;
    }
    else if (*data >= 128)
    {
        *data >>= 6;
        return mb_addr_inc_tab1[*data];
    }
    else if (*data >= 64)
    {
        *data >>= 3;
        *data -= 8;
        return mb_addr_inc_tab2[*data];
    }
    else
    {
        *data -= 24;
        return mb_addr_inc_tab3[*data];
    }
}

static void viddec_mpeg2_get_first_mb_number(void *parent, void *ctxt, uint32_t *first_mb)
{
    uint32_t mb_row = 0, mb_width = 0, prev_mb_addr = 0;
    uint32_t temp = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;
    *first_mb = 0;
    mb_row   = ((parser->mpeg2_last_parsed_slice_sc & 0xFF) - 1);
    mb_width = parser->info.seq_hdr.horizontal_size_value >> 4;
    prev_mb_addr = (mb_row * mb_width) - 1;

    /* Skip slice start code */
    viddec_pm_skip_bits(parent, 32);

    if (parser->info.seq_hdr.vertical_size_value > 2800)
    {
        /* Get 3 bits of slice_vertical_position_extension */
        viddec_pm_get_bits(parent, &temp, 3);
        mb_row += (temp << 7);
    }

    /* Skip proprity_breakpoint if sequence scalable extension is present */
    if (parser->mpeg2_curr_seq_headers & MPEG2_HEADER_SEQ_SCAL_EXT)
    {
        /* Skip 7 bits if scalable mode is 00 (Data partition) */
        if (parser->info.seq_scal_ext.scalable_mode == 0)
        {
            viddec_pm_skip_bits(parent, 7);
        }
    }

    /* Skip quantizer_scale */
    viddec_pm_skip_bits(parent, 5);

    /* Skip a few bits with slice information */
    temp = 0;
    viddec_pm_peek_bits(parent, &temp, 1);
    if (temp == 0x1)
    {
        /* Skip intra_slice_flag(1), intra_slice(1) and reserved_bits(7) */
        viddec_pm_skip_bits(parent, 9);
        temp=0;
        viddec_pm_peek_bits(parent, &temp, 1);
        while (temp == 0x1)
        {
            /* Skip extra_bit_slice(1) and extra_information_slice(8) */
            viddec_pm_skip_bits(parent, 9);
            temp=0;
            viddec_pm_peek_bits(parent, &temp, 1);
        }
    }

    /* Skip extra_bit_slice flag */
    viddec_pm_skip_bits(parent, 1);

    /* Increment prev_mb_addr by 33 for every 11 bits of macroblock_escape string */
    temp=0;
    viddec_pm_peek_bits(parent, &temp, 11);
    while (temp == 0x8)
    {
        viddec_pm_skip_bits(parent, 11);
        prev_mb_addr += 33;
        temp=0;
        viddec_pm_peek_bits(parent, &temp, 11);
    }

    /* Get the mb_addr_increment and add it to prev_mb_addr to get the current mb number. */
    *first_mb = prev_mb_addr + get_mb_addr_increment(&temp);
    MPEG2_DEB("First MB number in slice is 0x%08X.\n", *first_mb);

    return;
}

/* Parse slice data to get the number of macroblocks in the current slice and then */
/* append as pixel data. */
void viddec_mpeg2_parse_and_append_slice_data(void *parent, void *ctxt)
{
    uint32_t bit_off=0, start_byte=0, first_mb = 0;
    uint8_t  is_emul=0;
    viddec_workload_item_t wi;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get current byte position */
    viddec_pm_get_au_pos(parent, &bit_off, &start_byte, &is_emul);

    /* Populate wi type */
    viddec_mpeg2_get_first_mb_number(parent, ctxt, &first_mb);
    wi.vwi_type = VIDDEC_WORKLOAD_PIXEL_ES;
    wi.es.es_flags = (first_mb << 16);

    /* Append data from given byte position as pixel data */
    viddec_pm_append_misc_tags(parent, start_byte, (unsigned int) -1, &wi, !parser->mpeg2_use_next_workload);
    return;
}
