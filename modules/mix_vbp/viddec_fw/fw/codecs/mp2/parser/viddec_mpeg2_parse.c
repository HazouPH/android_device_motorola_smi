/**
 * viddec_mpeg2_parse.c
 * --------------------
 * This file acts as the main interface between the parser manager and MPEG2
 * parser. All the operations done by the MPEG2 parser are defined here and
 * functions pointers for each operation is returned to the parser manager.
 */

#include "viddec_mpeg2.h"

/* viddec_mpeg2_parser_init() - Initializes parser context. */
static void viddec_mpeg2_parser_init
(
    void        *ctxt,
    uint32_t    *persist_mem,
    uint32_t     preserve
)
{
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Avoid compiler warning */
    persist_mem = persist_mem;

    /* Initialize state variables */
    parser->mpeg2_pic_metadata_complete         =  false;
    parser->mpeg2_picture_interlaced            =  false;
    parser->mpeg2_first_field                   =  false;
    parser->mpeg2_frame_start                   =  false;
    parser->mpeg2_ref_table_updated             =  false;
    parser->mpeg2_use_next_workload             =  false;
    parser->mpeg2_first_slice_flag              =  false;
    parser->mpeg2_curr_frame_headers            =  MPEG2_HEADER_NONE;
    parser->mpeg2_last_parsed_sc                =  MPEG2_SC_ALL;
    parser->mpeg2_last_parsed_slice_sc          =  MPEG2_SC_SLICE_MAX;
    parser->mpeg2_wl_status                     =  MPEG2_WL_EMPTY;
    parser->mpeg2_prev_picture_structure        =  MPEG2_PIC_STRUCT_FRAME;
    parser->mpeg2_prev_temp_ref                 =  0;
    parser->mpeg2_num_pan_scan_offsets          =  0;

    if (preserve)
    {
        /* Init all picture level header info */
        memset(&parser->info.pic_hdr, 0, sizeof(struct mpeg2_picture_hdr_info));
        memset(&parser->info.pic_cod_ext, 0, sizeof(struct mpeg2_picture_coding_ext_info));
        memset(&parser->info.pic_disp_ext, 0, sizeof(struct mpeg2_picture_disp_ext_info));
    }
    else
    {
        /* Init all header info */
        memset(&parser->info, 0, sizeof(struct mpeg2_info));

        parser->mpeg2_stream                        =  false;
        parser->mpeg2_custom_qmat_parsed            =  false;
        parser->mpeg2_valid_seq_hdr_parsed          =  false;
        parser->mpeg2_curr_seq_headers              =  MPEG2_HEADER_NONE;
    }

    MPEG2_DEB("MPEG2 Parser: Context Initialized.\n");

    return;
}

/* viddec_mpeg2_get_context_size() - Returns the memory size required by the */
/* MPEG2 parser. */
static void viddec_mpeg2_get_context_size
(
    viddec_parser_memory_sizes_t    *size
)
{
    /* Should return size of my structure */
    size->context_size = sizeof(struct viddec_mpeg2_parser);
    size->persist_size = 0;
}

/* viddec_mpeg2_get_error_code() - Returns the error code for the current */
/* workload. */
static void viddec_mpeg2_get_error_code
(
    struct viddec_mpeg2_parser  *parser,
    viddec_workload_t           *wl,
    uint32_t                    *error_code
)
{
    *error_code = 0;

    /* Dangling field error */
    if (parser->mpeg2_wl_status & MPEG2_WL_DANGLING_FIELD)
    {
        *error_code |= VIDDEC_FW_WORKLOAD_ERR_DANGLING_FLD;
        if (parser->mpeg2_wl_status & MPEG2_WL_DANGLING_FIELD_TOP)
        {
            *error_code |= VIDDEC_FW_WORKLOAD_ERR_TOPFIELD;
        }
        else
        {
            *error_code |= VIDDEC_FW_WORKLOAD_ERR_BOTTOMFIELD;
        }
    }

    /* Repeated same field */
    if (parser->mpeg2_wl_status & MPEG2_WL_REPEAT_FIELD)
    {
        *error_code |= (VIDDEC_FW_WORKLOAD_ERR_DANGLING_FLD
                        | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE);
    }

    /* If workload is not complete, set non-decodeable flag */
    if (!(parser->mpeg2_wl_status & MPEG2_WL_COMPLETE))
    {
        *error_code |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
    }

    /* If reference info is not updated, set missing reference flag */
    if (!(parser->mpeg2_wl_status & MPEG2_WL_REF_INFO))
    {
        *error_code |= VIDDEC_FW_WORKLOAD_ERR_MISSING_REFERENCE;
    }

    /* Missing DMEM data flag and irrecoverable flag is set */
    if (!(parser->mpeg2_wl_status & MPEG2_WL_DMEM_DATA))
    {
        *error_code |= ( VIDDEC_FW_WORKLOAD_ERR_MISSING_DMEM
                         | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE ) ;
    }

    /* Missing sequence header and irrecoverable flag is set */
    if ((!(parser->mpeg2_curr_seq_headers & MPEG2_HEADER_SEQ))
            && (!parser->mpeg2_valid_seq_hdr_parsed))
    {
        *error_code |= ( VIDDEC_FW_WORKLOAD_ERR_MISSING_SEQ_INFO
                         | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE ) ;
    }

    /* Unsupported features found in stream */
    if (parser->mpeg2_wl_status & MPEG2_WL_UNSUPPORTED)
    {
        *error_code |= ( VIDDEC_FW_WORKLOAD_ERR_UNSUPPORTED
                         | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE ) ;
    }

    /* If frame type is unknown, default to I frame. */
    if ((wl->attrs.frame_type != VIDDEC_FRAME_TYPE_I)
            && (wl->attrs.frame_type != VIDDEC_FRAME_TYPE_P)
            && (wl->attrs.frame_type != VIDDEC_FRAME_TYPE_B))
    {
        wl->attrs.frame_type = VIDDEC_FRAME_TYPE_I;
    }

    /* If there is a mismatch between the frame type and reference information */
    /* then mark the workload as not decodable */
    if (wl->attrs.frame_type == VIDDEC_FRAME_TYPE_B)
    {
        if (wl->is_reference_frame != 0) *error_code |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
    }
    else
    {
        if (wl->is_reference_frame == 0) *error_code |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
    }

    /* For non-decodable frames, do not set reference info so that the workload */
    /* manager does not increment ref count. */
    if (*error_code & VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE)
    {
        wl->is_reference_frame = 0;
    }

    /* Corrupted header notification */
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_SEQ_HDR)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_HDR;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_SEQ_EXT)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_EXT;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_SEQ_DISP_EXT)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_SEQ_DISP_EXT;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_GOP_HDR)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_GOP_HDR;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_PIC_HDR)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_HDR;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_PIC_COD_EXT)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_COD_EXT;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_PIC_DISP_EXT)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_PIC_DISP_EXT;
    if (parser->mpeg2_wl_status & MPEG2_WL_CORRUPTED_QMAT_EXT)
        *error_code |= VIDDEC_FW_MPEG2_ERR_CORRUPTED_QMAT_EXT;

    MPEG2_DEB("Workload error code: 0x%8X.\n", *error_code);
    return;
}

/* viddec_mpeg2_is_start_frame() - Returns if the current chunk of parsed */
/* data has start of a frame. */
static uint32_t viddec_mpeg2_is_start_frame
(
    void    *ctxt
)
{
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;
    return (parser->mpeg2_frame_start);
}

/* viddec_mpeg2_is_workload_done() - Returns current frame parsing status */
/* to the parser manager. */
static uint32_t viddec_mpeg2_is_workload_done
(
    void            *parent,
    void            *ctxt,
    unsigned int    next_sc,
    uint32_t        *codec_specific_errors
)
{
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;
    viddec_workload_t *wl = viddec_pm_get_header(parent);
    uint32_t ret = VIDDEC_PARSE_SUCESS;
    uint32_t frame_boundary = 0;
    uint8_t force_frame_complete = 0;
    parent = parent;

    /* Detect Frame Boundary */
    frame_boundary = ((MPEG2_SC_PICTURE == next_sc) || (MPEG2_SC_SEQ_HDR == next_sc) || (MPEG2_SC_GROUP == next_sc));
    if (frame_boundary)
    {
        parser->mpeg2_first_slice_flag = false;
    }

    force_frame_complete = ((VIDDEC_PARSE_EOS == next_sc) || (VIDDEC_PARSE_DISCONTINUITY == next_sc));

    if (force_frame_complete || (frame_boundary && (parser->mpeg2_pic_metadata_complete)))
    {
        if (!force_frame_complete)
        {
            parser->mpeg2_wl_status            |= MPEG2_WL_COMPLETE;
            parser->mpeg2_last_parsed_slice_sc  =  MPEG2_SC_SLICE_MAX;
            parser->mpeg2_pic_metadata_complete = false;
            parser->mpeg2_first_slice_flag = false;
        }

        viddec_mpeg2_get_error_code(parser, wl, codec_specific_errors);
        parser->mpeg2_wl_status          = MPEG2_WL_EMPTY;
        parser->mpeg2_curr_frame_headers = MPEG2_HEADER_NONE;
        /* Reset mpeg2_use_next_workload flag if it is set */
        if (parser->mpeg2_use_next_workload)
        {
            viddec_pm_set_late_frame_detect(parent);
            parser->mpeg2_use_next_workload  = false;
        }
        ret = VIDDEC_PARSE_FRMDONE;
    }
    return ret;
}

/* viddec_mpeg2_parse() - Parse metadata info from the buffer for the prev */
/* start code found. */
static mpeg2_status viddec_mpeg2_parse
(
    void    *parent,
    void    *ctxt
)
{
    uint32_t current_sc = 0, sc_bits = MPEG2_SC_AND_PREFIX_SIZE;
    int32_t  ret = MPEG2_SUCCESS;
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Reset frame start flag. For Mpeg1 we want to set frame start after
     we parsed pich header, since there is no extension*/
    parser->mpeg2_frame_start =  (!parser->mpeg2_stream) && (parser->mpeg2_last_parsed_sc == MPEG2_SC_PICTURE);

    /* Peak current start code - First 32 bits of the stream */
    ret = viddec_pm_peek_bits(parent, &current_sc, sc_bits);
    if (ret == -1)
    {
        MPEG2_DEB("Unable to get start code.\n");
        return MPEG2_PARSE_ERROR;
    }
    current_sc &= MPEG2_BIT_MASK_8;
    MPEG2_DEB("Start Code found = 0x%.8X\n", current_sc);

    /* Get rid of the start code prefix for all start codes except slice */
    /* start codes. */
    if ((current_sc < MPEG2_SC_SLICE_MIN) || (current_sc > MPEG2_SC_SLICE_MAX))
    {
        viddec_pm_skip_bits(parent, sc_bits);
    }

    /* Parse Metadata based on the start code found */
    switch ( current_sc )
    {
        /* Sequence Start Code */
    case MPEG2_SC_SEQ_HDR:
    {
        parser->mpeg2_curr_seq_headers = MPEG2_HEADER_NONE;
        viddec_mpeg2_parse_seq_hdr(parent, ctxt);
    }
    break;

    /* Picture Start Code */
    case MPEG2_SC_PICTURE:
    {
        viddec_mpeg2_parse_pic_hdr(parent, ctxt);
    }
    break;

    /* Extension Code */
    case MPEG2_SC_EXT:
    {
        viddec_mpeg2_parse_ext(parent, ctxt);
    }
    break;

    /* Group of Pictures Header */
    case MPEG2_SC_GROUP:
    {
        viddec_mpeg2_parse_gop_hdr(parent, ctxt);
    }
    break;

    /* Unused Start Code */
    case MPEG2_SC_SEQ_END:
    case MPEG2_SC_SEQ_ERR:
        break;

        /* User Data */
    case MPEG2_SC_USER_DATA:
    {
        viddec_mpeg2_parse_and_append_user_data(parent, ctxt);
    }
    break;

    default:
    {
        /* Slice Data - Append slice data to the workload */
        if ((current_sc >= MPEG2_SC_SLICE_MIN) &&
                (current_sc <= MPEG2_SC_SLICE_MAX))
        {
            if (!parser->mpeg2_first_slice_flag)
            {
                /* At this point, all the metadata required by the MPEG2 */
                /* hardware for decoding is extracted and stored. So the */
                /* metadata can be packed into workitems and emitted out.*/
                viddec_mpeg2_emit_workload(parent, ctxt);

                /* If the current picture is progressive or it is the */
                /* second field of interlaced field picture then, set */
                /* the workload done flag. */
                if ((!parser->mpeg2_picture_interlaced)
                        || ((parser->mpeg2_picture_interlaced) && (!parser->mpeg2_first_field)))
                {
                    parser->mpeg2_pic_metadata_complete = true;
                }
                else if ((parser->mpeg2_picture_interlaced) && (parser->mpeg2_first_field))
                {
                    parser->mpeg2_curr_frame_headers = MPEG2_HEADER_NONE;
                }

                parser->mpeg2_first_slice_flag = true;
            }
            parser->mpeg2_last_parsed_slice_sc = current_sc;
            viddec_mpeg2_parse_and_append_slice_data(parent, ctxt);
            parser->mpeg2_wl_status |= MPEG2_WL_PARTIAL_SLICE;
        }
    }
    } /* Switch */

    /* Save last parsed start code */
    parser->mpeg2_last_parsed_sc = current_sc;
    return ret;
}

/* viddec_mpeg2_get_ops() - Register parser ops with the parser manager. */
void viddec_mpeg2_get_ops
(
    viddec_parser_ops_t     *ops
)
{
    ops->init         = viddec_mpeg2_parser_init;
    ops->parse_syntax = viddec_mpeg2_parse;
    ops->get_cxt_size = viddec_mpeg2_get_context_size;
    ops->is_wkld_done = viddec_mpeg2_is_workload_done;
    ops->is_frame_start = viddec_mpeg2_is_start_frame;
    return;
}

