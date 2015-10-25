/**
 * viddec_mpeg2_workload.c
 * -----------------------
 * This file packs the data parsed and stored in the context into workload and
 * emits it out. The current list of workitems emitter into the workload
 * include:
 *
 * - DMEM - Register Data
 * - Past and Future picture references
 * - Quantization matrix data
 *
 * Slice data gets appended into the workload in viddec_mpeg2_parse.c
 *
 * Also, the frame attributes are updated in the workload.
 */

#include "viddec_mpeg2.h"
#include "viddec_fw_item_types.h"

void viddec_mpeg2_append_workitem(void *parent, viddec_workload_item_t *wi, uint8_t next_wl)
{
    if (next_wl)
    {
        viddec_pm_append_workitem_next(parent, wi);
    }
    else
    {
        viddec_pm_append_workitem(parent, wi);
    }
    return;
}

viddec_workload_t* viddec_mpeg2_get_header(void *parent, uint8_t next_wl)
{
    viddec_workload_t *ret;
    if (next_wl)
    {
        ret = viddec_pm_get_next_header(parent);
    }
    else
    {
        ret = viddec_pm_get_header(parent);
    }
    return ret;
}

/* viddec_mpeg2_set_seq_ext_defaults() - Sets non-zero default values for    */
/* sequence extension items in case sequence extension is not present.       */
static void viddec_mpeg2_set_seq_ext_defaults(struct viddec_mpeg2_parser *parser)
{
    parser->info.seq_ext.progressive_sequence = true;
    parser->info.seq_ext.chroma_format        = MPEG2_CF_420;
}

/* viddec_mpeg2_set_pic_cod_ext_defaults() - Sets non-zero default values for*/
/* picture coding extension items in case picture coding extension is not    */
/* present.                                                                  */
static void viddec_mpeg2_set_pic_cod_ext_defaults(struct viddec_mpeg2_parser *parser)
{
    parser->info.pic_cod_ext.picture_structure    = MPEG2_PIC_STRUCT_FRAME;
    parser->info.pic_cod_ext.frame_pred_frame_dct = true;
    parser->info.pic_cod_ext.progressive_frame    = true;
}

/* viddec_mpeg2_pack_qmat() - Packs the 256 byte quantization matrix data    */
/* 64 32-bit values.                                                         */
#ifdef MFDBIGENDIAN
static void viddec_mpeg2_pack_qmat(struct viddec_mpeg2_parser *parser)
{
    /* Quantization Matrix Support */
    /* Populate Quantization Matrices */
    uint32_t index = 0;
    uint32_t *qmat_packed, *qmat_unpacked;

    /* When transferring the quantization matrix data from the parser */
    /* context into workload items, we are packing four 8 bit         */
    /* quantization values into one DWORD (32 bits). To do this, the  */
    /* array of values of type uint8_t, is typecast as uint32 * and   */
    /* read.                                                          */
    qmat_packed   = (uint32_t *) parser->wi.qmat;
    qmat_unpacked = (uint32_t *) &parser->info.qnt_mat;

    for (index=0; index<MPEG2_QUANT_MAT_SIZE; index++)
    {
        qmat_packed[index] = qmat_unpacked[index];
    }
    return;
}
#else
static void viddec_mpeg2_pack_qmat(struct viddec_mpeg2_parser *parser)
{
    /* Quantization Matrix Support */
    /* Populate Quantization Matrices */
    uint32_t index = 0;
    uint32_t *qmat_packed;
    uint8_t  *qmat_unpacked;

    /* When transferring the quantization matrix data from the parser */
    /* context into workload items, we are packing four 8 bit         */
    /* quantization values into one DWORD (32 bits). To do this, the  */
    /* array of values of type uint8_t, is typecast as uint32 * and   */
    /* read.                                                          */
    qmat_packed   = (uint32_t *) parser->wi.qmat;
    qmat_unpacked = (uint8_t *) &parser->info.qnt_mat;

    for (index=0; index<MPEG2_QUANT_MAT_SIZE; index++)
    {
        qmat_packed[index] =
            (((uint32_t)qmat_unpacked[(index<<2)+0])<< 24) |
            (((uint32_t)qmat_unpacked[(index<<2)+1])<< 16) |
            (((uint32_t)qmat_unpacked[(index<<2)+2])<<  8) |
            (((uint32_t)qmat_unpacked[(index<<2)+3])<<  0) ;
    }
    return;
}
#endif

/* viddec_mpeg2_trans_metadata_workitems() - Transfers the metadata stored   */
/* in parser context into workitems by bit masking. These workitems are then */
/* sent through emitter                                                      */
static void viddec_mpeg2_trans_metadata_workitems(void *ctxt)
{
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Reset register values */
    parser->wi.csi1  = 0x0;
    parser->wi.csi2  = 0x0;
    parser->wi.cpi1  = 0x0;
    parser->wi.cpce1 = 0x0;

    /* Set defaults for missing fields */
    if (!(parser->mpeg2_curr_seq_headers & MPEG2_HEADER_SEQ_EXT))
    {
        viddec_mpeg2_set_seq_ext_defaults(parser);
    }
    if (!(parser->mpeg2_curr_frame_headers & MPEG2_HEADER_PIC_COD_EXT))
    {
        viddec_mpeg2_set_pic_cod_ext_defaults(parser);
    }

    /* Populate Core Sequence Info 1 */
    parser->wi.csi1 |= (parser->mpeg2_stream) <<  1;
    parser->wi.csi1 |= (parser->info.seq_hdr.constrained_parameters_flag) <<  2;
    parser->wi.csi1 |= (parser->info.seq_ext.progressive_sequence) <<  3;
    parser->wi.csi1 |= (parser->info.seq_ext.chroma_format) << 16;
    parser->wi.csi1 |= (parser->info.qnt_ext.load_intra_quantiser_matrix) << 19;
    parser->wi.csi1 |= (parser->info.qnt_ext.load_non_intra_quantiser_matrix) << 20;
    parser->wi.csi1 |= (parser->info.qnt_ext.load_chroma_intra_quantiser_matrix) << 21;
    parser->wi.csi1 |= (parser->info.qnt_ext.load_chroma_non_intra_quantiser_matrix) << 22;
    MPEG2_DEB("Core Sequence Info 1: 0x%.8X\n", parser->wi.csi1);

    /* Populate Core Sequence Info 2 */
    parser->wi.csi2 |= (parser->info.seq_hdr.horizontal_size_value & MPEG2_BIT_MASK_11);
    parser->wi.csi2 |= (parser->info.seq_hdr.vertical_size_value & MPEG2_BIT_MASK_11) << 14;
    MPEG2_DEB("Core Sequence Info 2: 0x%.8X\n", parser->wi.csi2);

    /* Populate Core Picture Info */
    parser->wi.cpi1 |= (parser->info.pic_hdr.full_pel_forward_vect);
    parser->wi.cpi1 |= (parser->info.pic_hdr.forward_f_code) <<  1;
    parser->wi.cpi1 |= (parser->info.pic_hdr.full_pel_backward_vect) <<  4;
    parser->wi.cpi1 |= (parser->info.pic_hdr.backward_f_code) <<  5;
    parser->wi.cpi1 |= (parser->info.pic_cod_ext.fcode00) <<  8;
    parser->wi.cpi1 |= (parser->info.pic_cod_ext.fcode01) << 12;
    parser->wi.cpi1 |= (parser->info.pic_cod_ext.fcode10) << 16;
    parser->wi.cpi1 |= (parser->info.pic_cod_ext.fcode11) << 20;
    parser->wi.cpi1 |= (parser->info.pic_cod_ext.intra_dc_precision) << 24;
    parser->wi.cpi1 |= (parser->info.pic_hdr.picture_coding_type-1) << 26;
    MPEG2_DEB("Core Picture Info 1: 0x%.8X\n", parser->wi.cpi1);

    /* Populate Core Picture Extension Info */
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.composite_display_flag);
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.progressive_frame) <<  1;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.chroma_420_type) <<  2;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.repeat_first_field) <<  3;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.alternate_scan) <<  4;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.intra_vlc_format) <<  5;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.q_scale_type) <<  6;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.concealment_motion_vectors) <<  7;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.frame_pred_frame_dct) <<  8;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.top_field_first) <<  9;
    parser->wi.cpce1 |= (parser->info.pic_cod_ext.picture_structure) << 10;
    MPEG2_DEB("Core Picture Ext Info 1: 0x%.8X\n", parser->wi.cpce1);

    return;
}

/* mpeg2_emit_display_frame() - Sends the frame id as a workload item.       */
static inline void mpeg2_emit_frameid(void *parent, int32_t wl_type, uint8_t flag)
{
    viddec_workload_item_t wi;
    wi.vwi_type = wl_type;

    wi.ref_frame.reference_id     = 0;
    wi.ref_frame.luma_phys_addr   = 0;
    wi.ref_frame.chroma_phys_addr = 0;
    viddec_mpeg2_append_workitem( parent, &wi, flag );
}

/* mpeg2_send_ref_reorder() - Reorders reference frames */
static inline void mpeg2_send_ref_reorder(void *parent, uint8_t flag)
{
    viddec_workload_item_t wi;

    wi.vwi_type = VIDDEC_WORKLOAD_REFERENCE_FRAME_REORDER;
    wi.ref_reorder.ref_table_offset = 0;
    /* Reorder index 1 to index 0 only */
    wi.ref_reorder.ref_reorder_00010203 = 0x01010203;
    wi.ref_reorder.ref_reorder_04050607 = 0x04050607;
    viddec_mpeg2_append_workitem( parent, &wi, flag );
}

/* viddec_mpeg2_manage_ref() - Manages frame references by inserting the     */
/* past and future references (if any) for every frame inserted in the       */
/* workload.                                                                 */
static void viddec_mpeg2_manage_ref(void *parent, void *ctxt)
{
    int32_t frame_id = 1;
    int32_t frame_type;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;
    viddec_workload_t *wl = viddec_mpeg2_get_header( parent, parser->mpeg2_use_next_workload );
    wl->is_reference_frame = 0;

    /* Identify the frame type (I, P or B) */
    frame_type = parser->info.pic_hdr.picture_coding_type;

    /* Send reference frame information based on whether the picture is a */
    /* frame picture or field picture. */
    if ((!parser->mpeg2_picture_interlaced)
            || ((parser->mpeg2_picture_interlaced) && (parser->mpeg2_first_field)))
    {
        /* Check if we need to reorder frame references/send frame for display */
        /* in case of I or P type */
        if (frame_type != MPEG2_PC_TYPE_B)
        {
            /* Checking reorder */
            if (parser->mpeg2_ref_table_updated)
            {
                mpeg2_send_ref_reorder(parent, parser->mpeg2_use_next_workload);
            }
        }

        /* Send reference frame workitems */
        switch (frame_type)
        {
        case MPEG2_PC_TYPE_I:
        {
            break;
        }
        case MPEG2_PC_TYPE_P:
        {
            mpeg2_emit_frameid(parent, VIDDEC_WORKLOAD_MPEG2_REF_PAST, parser->mpeg2_use_next_workload);
            break;
        }
        case MPEG2_PC_TYPE_B:
        {
            mpeg2_emit_frameid(parent, VIDDEC_WORKLOAD_MPEG2_REF_PAST, parser->mpeg2_use_next_workload);
            mpeg2_emit_frameid(parent, VIDDEC_WORKLOAD_MPEG2_REF_FUTURE, parser->mpeg2_use_next_workload);
        }
        }

        /* Set reference information updated flag */
        if (!parser->mpeg2_picture_interlaced)
        {
            parser->mpeg2_wl_status |= MPEG2_WL_REF_INFO;
        }
    }
    else
    {
        /* Set reference information updated flag for second fiel */
        parser->mpeg2_wl_status |= MPEG2_WL_REF_INFO;
    }

    /* Set the reference frame flags for I and P types */
    if (frame_type != MPEG2_PC_TYPE_B)
    {
        wl->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (frame_id & WORKLOAD_REFERENCE_FRAME_BMASK);
        parser->mpeg2_ref_table_updated = true;
    }

    return;
}

/* viddec_mpeg2_check_unsupported() - Check for unsupported feature in the stream */
static void viddec_mpeg2_check_unsupported(void *parent, void *ctxt)
{
    unsigned int unsupported_feature_found = 0;

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get workload */
    viddec_workload_t *wl = viddec_mpeg2_get_header( parent, parser->mpeg2_use_next_workload );

    /* Get attributes in workload */
    viddec_frame_attributes_t *attrs = &wl->attrs;

    /* Check for unsupported content size */
    unsupported_feature_found |= (attrs->cont_size.height > MPEG2_MAX_CONTENT_HEIGHT);
    unsupported_feature_found |= (attrs->cont_size.width  > MPEG2_MAX_CONTENT_WIDTH);

    /* Update parser status, if found */
    if (unsupported_feature_found)
    {
        parser->mpeg2_wl_status |= MPEG2_WL_UNSUPPORTED;
    }

    return;
}

/* viddec_mpeg2_append_metadata() - Appends meta data from the stream.       */
void viddec_mpeg2_append_metadata(void *parent, void *ctxt)
{
    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    viddec_workload_item_t  wi;

    /* Append sequence info, if found with current frame */
    if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_SEQ)
    {
        memset(&wi, 0, sizeof(viddec_workload_item_t));
        wi.vwi_type = VIDDEC_WORKLOAD_SEQUENCE_INFO;

        viddec_fw_mp2_sh_set_horizontal_size_value       ( &(wi.mp2_sh) , parser->info.seq_hdr.horizontal_size_value);
        viddec_fw_mp2_sh_set_vertical_size_value         ( &(wi.mp2_sh) , parser->info.seq_hdr.vertical_size_value);
        viddec_fw_mp2_sh_set_aspect_ratio_information    ( &(wi.mp2_sh) , parser->info.seq_hdr.aspect_ratio_information);
        viddec_fw_mp2_sh_set_frame_rate_code             ( &(wi.mp2_sh) , parser->info.seq_hdr.frame_rate_code);
        viddec_fw_mp2_sh_set_bit_rate_value              ( &(wi.mp2_sh) , parser->info.seq_hdr.bit_rate_value);
        viddec_fw_mp2_sh_set_vbv_buffer_size_value       ( &(wi.mp2_sh) , parser->info.seq_hdr.vbv_buffer_size_value);

        viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
    }

    /* Append sequence extension info, if found with current frame */
    if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_SEQ_EXT)
    {
        memset(&wi, 0, sizeof(viddec_workload_item_t));
        wi.vwi_type = VIDDEC_WORKLOAD_MPEG2_SEQ_EXT;

        viddec_fw_mp2_se_set_profile_and_level_indication( &(wi.mp2_se) , parser->info.seq_ext.profile_and_level_indication);
        viddec_fw_mp2_se_set_progressive_sequence        ( &(wi.mp2_se) , parser->info.seq_ext.progressive_sequence);
        viddec_fw_mp2_se_set_chroma_format               ( &(wi.mp2_se) , parser->info.seq_ext.chroma_format);
        viddec_fw_mp2_se_set_horizontal_size_extension   ( &(wi.mp2_se) , parser->info.seq_ext.horizontal_size_extension);
        viddec_fw_mp2_se_set_vertical_size_extension     ( &(wi.mp2_se) , parser->info.seq_ext.vertical_size_extension);
        viddec_fw_mp2_se_set_bit_rate_extension          ( &(wi.mp2_se) , parser->info.seq_ext.bit_rate_extension);
        viddec_fw_mp2_se_set_vbv_buffer_size_extension   ( &(wi.mp2_se) , parser->info.seq_ext.vbv_buffer_size_extension);
        viddec_fw_mp2_se_set_frame_rate_extension_n      ( &(wi.mp2_se) , parser->info.seq_ext.frame_rate_extension_n);
        viddec_fw_mp2_se_set_frame_rate_extension_d      ( &(wi.mp2_se) , parser->info.seq_ext.frame_rate_extension_d);

        viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
    }

    /* Append Display info, if present */
    if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_SEQ_DISP_EXT)
    {
        memset(&wi, 0, sizeof(viddec_workload_item_t));
        wi.vwi_type = VIDDEC_WORKLOAD_DISPLAY_INFO;

        viddec_fw_mp2_sde_set_video_format            ( &(wi.mp2_sde) , parser->info.seq_disp_ext.video_format);
        viddec_fw_mp2_sde_set_color_description       ( &(wi.mp2_sde) , parser->info.seq_disp_ext.colour_description);
        viddec_fw_mp2_sde_set_color_primaries         ( &(wi.mp2_sde) , parser->info.seq_disp_ext.colour_primaries);
        viddec_fw_mp2_sde_set_transfer_characteristics( &(wi.mp2_sde) , parser->info.seq_disp_ext.transfer_characteristics);
        viddec_fw_mp2_sde_set_display_horizontal_size ( &(wi.mp2_sde) , parser->info.seq_disp_ext.display_horizontal_size);
        viddec_fw_mp2_sde_set_display_vertical_size   ( &(wi.mp2_sde) , parser->info.seq_disp_ext.display_vertical_size);

        viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
    }

    /* Append GOP info, if present */
    if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_GOP)
    {
        memset(&wi, 0, sizeof(viddec_workload_item_t));
        wi.vwi_type = VIDDEC_WORKLOAD_GOP_INFO;

        viddec_fw_mp2_gop_set_closed_gop ( &(wi.mp2_gop) , parser->info.gop_hdr.closed_gop);
        viddec_fw_mp2_gop_set_broken_link( &(wi.mp2_gop) , parser->info.gop_hdr.broken_link);

        viddec_mpeg2_append_workitem(parent, &wi, parser->mpeg2_use_next_workload);
    }

    return;
}

/* viddec_mpeg2_append_workitems() - Appends decoder specific workitems      */
/* to the workload starting at the address and length specified.             */
static void viddec_mpeg2_append_workitems
(
    void *parent,
    uint32_t* address,
    int workitem_type,
    int num_items,
    uint8_t flag
)
{
    int32_t                  index=0;
    const uint32_t*          initial_address = address;
    viddec_workload_item_t   wi;

    for (index=0; index < num_items; index++)
    {
        wi.vwi_type = workitem_type;
        wi.data.data_offset = (char *) address - (const char *) initial_address;
        wi.data.data_payload[0] = address[0];
        wi.data.data_payload[1] = address[1];
        address += 2;

        viddec_mpeg2_append_workitem(parent, &wi, flag);
    }

    return;
}

/* viddec_mpeg2_emit_workload() - Emits MPEG2 parser generated work load     */
/* items.                                                                    */
/* Items include: MPEG2 DMEM Data, Quantization Matrices.                    */
/* Pixel ES data sent separately whenever parser sees slice data             */
void viddec_mpeg2_emit_workload(void *parent, void *ctxt)
{
    MPEG2_DEB("Emitting workloads.\n");

    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Append meta data workitems */
    viddec_mpeg2_append_metadata(parent, ctxt);

    /* Transfer metadata into attributes */
    viddec_mpeg2_translate_attr(parent, ctxt);

    /* Check for unsupported features in the stream and update parser status */
    viddec_mpeg2_check_unsupported(parent, ctxt);

    /* Transfer all stored metadata into MPEG2 Hardware Info */
    viddec_mpeg2_trans_metadata_workitems(parser);

    /* Send MPEG2 DMEM workitems */
    viddec_mpeg2_append_workitems(parent,
                                  (uint32_t *) &parser->wi,
                                  VIDDEC_WORKLOAD_MPEG2_DMEM,
                                  MPEG2_NUM_DMEM_WL_ITEMS,
                                  parser->mpeg2_use_next_workload);
    parser->mpeg2_wl_status |= MPEG2_WL_DMEM_DATA;
    MPEG2_DEB("Adding %d items as DMEM Data.\n", MPEG2_NUM_DMEM_WL_ITEMS);

    /* Send MPEG2 Quantization Matrix workitems, if updated */
    viddec_mpeg2_pack_qmat(parser);
    viddec_mpeg2_append_workitems(parent,
                                  (uint32_t *) parser->wi.qmat,
                                  VIDDEC_WORKLOAD_MPEG2_QMAT,
                                  MPEG2_NUM_QMAT_WL_ITEMS,
                                  parser->mpeg2_use_next_workload);
    MPEG2_DEB("Adding %d items as QMAT Data.\n", MPEG2_NUM_QMAT_WL_ITEMS);

    /* Manage reference frames */
    viddec_mpeg2_manage_ref(parent, ctxt);

    return;
}

