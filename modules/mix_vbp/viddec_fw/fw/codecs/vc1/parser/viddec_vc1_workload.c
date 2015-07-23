/* Any workload management goes in this file */

#include "viddec_fw_debug.h"
#include "vc1.h"
#include "vc1parse.h"
#include "viddec_fw_workload.h"
#include <auto_eas/gen4_mfd.h>
#include "viddec_pm_utils_bstream.h"

/* this function returns workload frame types corresponding to VC1 PTYPES (frame types)
 * VC1 frame types: can be found in vc1parse_common_defs.h
 * workload frame types are in viddec_workload.h
*/
static inline uint32_t vc1_populate_frame_type(uint32_t vc1_frame_type)
{
    uint32_t viddec_frame_type;

    switch (vc1_frame_type)
    {
    case VC1_I_FRAME:
        viddec_frame_type = VIDDEC_FRAME_TYPE_I;
        break;
    case VC1_P_FRAME:
        viddec_frame_type = VIDDEC_FRAME_TYPE_P;
        break;
    case VC1_B_FRAME:
        viddec_frame_type = VIDDEC_FRAME_TYPE_B;
        break;
    case VC1_BI_FRAME:
        viddec_frame_type = VIDDEC_FRAME_TYPE_BI;
        break;
    case VC1_SKIPPED_FRAME :
        viddec_frame_type =  VIDDEC_FRAME_TYPE_SKIP;
        break;
    default:
        viddec_frame_type = VIDDEC_FRAME_TYPE_INVALID;
        break;
    } // switch on vc1 frame type

    return(viddec_frame_type);
} // vc1_populate_frame_type

static void translate_parser_info_to_frame_attributes(void *parent, vc1_viddec_parser_t *parser)
{
    viddec_workload_t        *wl = viddec_pm_get_header( parent );
    viddec_frame_attributes_t *attrs = &wl->attrs;
    vc1_Info        *info = &parser->info;
    unsigned i;

    /* typical sequence layer and entry_point data */
    attrs->cont_size.height       = info->metadata.height * 2 + 2;
    attrs->cont_size.width        = info->metadata.width  * 2 + 2;

    /* frame type */
    /* we can have two fileds with different types for field interlace coding mode */
    if (info->picLayerHeader.FCM == VC1_FCM_FIELD_INTERLACE) {
        attrs->frame_type = vc1_populate_frame_type(info->picLayerHeader.PTypeField1);
        attrs->bottom_field_type = vc1_populate_frame_type(info->picLayerHeader.PTypeField2);
    } else {
        attrs->frame_type = vc1_populate_frame_type(info->picLayerHeader.PTYPE);
        attrs->bottom_field_type = VIDDEC_FRAME_TYPE_INVALID; //unknown
    }

    /* frame counter */
    attrs->vc1.tfcntr = info->picLayerHeader.TFCNTR;

    /* TFF, repeat frame, field */
    attrs->vc1.tff = info->picLayerHeader.TFF;
    attrs->vc1.rptfrm = info->picLayerHeader.RPTFRM;
    attrs->vc1.rff = info->picLayerHeader.RFF;

    /* PAN Scan */
    attrs->vc1.ps_present = info->picLayerHeader.PS_PRESENT;
    attrs->vc1.num_of_pan_scan_windows = info->picLayerHeader.number_of_pan_scan_window;
    for (i=0; i<attrs->vc1.num_of_pan_scan_windows; i++) {
        attrs->vc1.pan_scan_window[i].hoffset =  info->picLayerHeader.PAN_SCAN_WINDOW[i].hoffset;
        attrs->vc1.pan_scan_window[i].voffset =  info->picLayerHeader.PAN_SCAN_WINDOW[i].voffset;
        attrs->vc1.pan_scan_window[i].width =  info->picLayerHeader.PAN_SCAN_WINDOW[i].width;
        attrs->vc1.pan_scan_window[i].height =  info->picLayerHeader.PAN_SCAN_WINDOW[i].height;
    } //end for i

    return;
} // translate_parser_info_to_frame_attributes

/* sends VIDDEC_WORKLOAD_VC1_PAST_FRAME item */
static inline void vc1_send_past_ref_items(void *parent)
{
    viddec_workload_item_t wi;
    wi.vwi_type = VIDDEC_WORKLOAD_VC1_PAST_FRAME;
    wi.ref_frame.reference_id = 0;
    wi.ref_frame.luma_phys_addr = 0;
    wi.ref_frame.chroma_phys_addr = 0;
    viddec_pm_append_workitem( parent, &wi, false );
    return;
}

/* send future frame item */
static inline void vc1_send_future_ref_items(void *parent)
{
    viddec_workload_item_t wi;
    wi.vwi_type = VIDDEC_WORKLOAD_VC1_FUTURE_FRAME;
    wi.ref_frame.reference_id = 0;
    wi.ref_frame.luma_phys_addr = 0;
    wi.ref_frame.chroma_phys_addr = 0;
    viddec_pm_append_workitem( parent, &wi, false );
    return;
}

/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_reorder_ref_items(void *parent)
{
    viddec_workload_item_t wi;
    wi.vwi_type = VIDDEC_WORKLOAD_REFERENCE_FRAME_REORDER;
    wi.ref_reorder.ref_table_offset = 0;
    wi.ref_reorder.ref_reorder_00010203 = 0x01010203; //put reference frame index 1 as reference index 0
    wi.ref_reorder.ref_reorder_04050607 = 0x04050607; // index 4,5,6,7 stay the same
    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items


/* sends VIDDEC_WORKLOAD_VC1_PAST_FRAME item */
static inline void vc1_send_ref_fcm_items(void *parent, uint32_t past_fcm, uint32_t future_fcm)
{
    viddec_workload_item_t wi;
    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_REF_FRAME_TYPE;
    wi.vwi_payload[0]= 0;
    wi.vwi_payload[1]= past_fcm;
    wi.vwi_payload[2]= future_fcm;
    viddec_pm_append_workitem( parent, &wi, false );
    return;
}



/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_SEQ_ENTRY_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t stream_format1 = 0;
    uint32_t stream_format2 = 0;
    uint32_t entrypoint1 = 0;
    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);



    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, PROFILE, stream_format1, md->PROFILE);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, LEVEL, stream_format1, md->LEVEL);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, CHROMAFORMAT, stream_format1, md->CHROMAFORMAT);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, FRMRTQ, stream_format1, md->FRMRTQ);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, BITRTQ, stream_format1, md->BITRTQ);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_1, POSTPRO, stream_format1, md->POSTPROCFLAG);


    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_2, PULLDOWN, stream_format2, md->PULLDOWN);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_2, INTERLACE, stream_format2, md->INTERLACE);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_2, TFCNTRFLAG, stream_format2, md->TFCNTRFLAG);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_2, FINTERPFLAG, stream_format2, md->FINTERPFLAG);
    BF_WRITE(VC1_0_SEQPIC_STREAM_FORMAT_2, PSF, stream_format2, md->PSF);


    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, BROKEN_LINK,   entrypoint1, md->BROKEN_LINK);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, CLOSED_ENTRY,  entrypoint1, md->CLOSED_ENTRY);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, PANSCAN_FLAG,  entrypoint1, md->PANSCAN_FLAG);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, REFDIST_FLAG,  entrypoint1, md->REFDIST_FLAG);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, LOOPFILTER,    entrypoint1, md->LOOPFILTER);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, FASTUVMC,      entrypoint1, md->FASTUVMC);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, EXTENDED_MV,   entrypoint1, md->EXTENDED_MV);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, DQUANT,        entrypoint1, md->DQUANT);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, VS_TRANSFORM,  entrypoint1, md->VSTRANSFORM);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, OVERLAP,       entrypoint1, md->OVERLAP);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, QUANTIZER,     entrypoint1, md->QUANTIZER);
    BF_WRITE(VC1_0_SEQPIC_ENTRY_POINT_1, EXTENDED_DMV,  entrypoint1, md->EXTENDED_DMV);


    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_SEQ_ENTRY;


    wi.vwi_payload[0] = stream_format1;
    wi.vwi_payload[1] = stream_format2;
    wi.vwi_payload[2] = entrypoint1;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items


/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_SIZE_AND_AP_RANGEMAP_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t coded_size = 0;
    uint32_t ap_range_map = 0;

    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);


    BF_WRITE(VC1_0_SEQPIC_CODED_SIZE, WIDTH, coded_size, md->width);
    BF_WRITE(VC1_0_SEQPIC_CODED_SIZE, HEIGHT, coded_size, md->height);


    /* if range reduction is indicated at seq. layer, populate range reduction registers for the frame*/
    if (VC1_PROFILE_ADVANCED == md->PROFILE)
    {


        BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_Y_FLAG, ap_range_map, md->RANGE_MAPY_FLAG);
        BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_Y, ap_range_map, md->RANGE_MAPY);
        BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_UV_FLAG, ap_range_map, md->RANGE_MAPUV_FLAG);
        BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_UV, ap_range_map, md->RANGE_MAPUV);




    }
    else
    {
        ap_range_map = 0;
    }


    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_SIZE_AND_AP_RANGEMAP;


    wi.vwi_payload[0] = 0;
    wi.vwi_payload[1] = coded_size;
    wi.vwi_payload[2] = ap_range_map;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items



/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_SLICE_FRAME_TYPE_INFO_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t alt_frame_type = 0;
    uint32_t frame_type = 0;

    vc1_PictureLayerHeader *pic = &(parser->info.picLayerHeader);
    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);


    BF_WRITE(VC1_0_SEQPIC_FRAME_TYPE, FCM, frame_type, pic->FCM);
    BF_WRITE(VC1_0_SEQPIC_FRAME_TYPE, PTYPE, frame_type, pic->PTYPE);

    alt_frame_type = frame_type;

    if (VC1_PROFILE_ADVANCED == md->PROFILE)
    {
        if ( (VC1_P_FRAME == pic->PTYPE)||(VC1_B_FRAME == pic->PTYPE) )
        {
            BF_WRITE(VC1_0_SEQPIC_ALT_FRAME_TYPE, PQUANT, alt_frame_type, pic->PQUANT);
        }
    }
    else
    {
        if ( VC1_SKIPPED_FRAME== pic->PTYPE)
        {
            BF_WRITE(VC1_0_SEQPIC_ALT_FRAME_TYPE, PQUANT, alt_frame_type, 0);
        } else {
            BF_WRITE(VC1_0_SEQPIC_ALT_FRAME_TYPE, PQUANT, alt_frame_type, pic->PQUANT);
        }
    }


    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_SLICE_FRAME_TYPE_INFO;


    wi.vwi_payload[0] = 0;
    wi.vwi_payload[1] = frame_type;
    wi.vwi_payload[2] = alt_frame_type;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items

/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_SLICE_CONTROL_INFO_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t recon_control = 0;
    uint32_t mv_control = 0;
    uint32_t blk_control = 0;

    vc1_PictureLayerHeader *pic = &(parser->info.picLayerHeader);
    viddec_workload_item_t wi;

    int is_previous_ref_rr=0;

    vc1_metadata_t *md = &(parser->info.metadata);


    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, RNDCTRL, recon_control, md->RNDCTRL);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, UVSAMP, recon_control, pic->UVSAMP);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, PQUANT, recon_control, pic->PQUANT);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, HALFQP, recon_control, pic->HALFQP);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, UNIFORM_QNT, recon_control, pic->UniformQuant);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, POSTPROC, recon_control, pic->POSTPROC);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, CONDOVER, recon_control, pic->CONDOVER);
    BF_WRITE( VC1_0_SEQPIC_RECON_CONTROL, PQINDEX_LE8, recon_control, (pic->PQINDEX <= 8));

    /* Get the range reduced status of the previous frame */
    switch (pic->PTYPE)
    {
    case VC1_P_FRAME:
    {
        is_previous_ref_rr = parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].rr_frm;
        break;
    }
    case VC1_B_FRAME:
    {
        is_previous_ref_rr = parser->ref_frame[VC1_REF_FRAME_T_MINUS_2].rr_frm;
        break;
    }
    default:
    {
        break;
    }
    }

    if (pic->RANGEREDFRM)
    {

        if (!is_previous_ref_rr)
        {
            BF_WRITE(VC1_0_SEQPIC_RECON_CONTROL, RANGE_REF_RED_EN, recon_control, 1);
            BF_WRITE(VC1_0_SEQPIC_RECON_CONTROL, RANGE_REF_RED_TYPE, recon_control, 1);
        }
    }
    else
    {
        /* if current frame is not RR but previous was RR,  scale up the reference frame ( RANGE_REF_RED_TYPE = 0) */
        if (is_previous_ref_rr)
        {
            BF_WRITE(VC1_0_SEQPIC_RECON_CONTROL, RANGE_REF_RED_EN, recon_control, 1);
            BF_WRITE(VC1_0_SEQPIC_RECON_CONTROL, RANGE_REF_RED_TYPE, recon_control, 0);
        }
    } // end for RR upscale





    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MVRANGE,   mv_control, pic->MVRANGE);
    if ( pic->MVMODE == VC1_MVMODE_INTENSCOMP)
        BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MVMODE,    mv_control, pic->MVMODE2);
    else
        BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MVMODE,    mv_control, pic->MVMODE);
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MVTAB,  mv_control,  pic->MVTAB);
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, DMVRANGE,  mv_control, pic->DMVRANGE);
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MV4SWITCH, mv_control, pic->MV4SWITCH);
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, MBMODETAB, mv_control, pic->MBMODETAB);
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, NUMREF,    mv_control,
              pic->NUMREF || ((pic->PTYPE == VC1_B_FRAME) && ( pic->FCM == VC1_FCM_FIELD_INTERLACE )  ));
    BF_WRITE( VC1_0_SEQPIC_MOTION_VECTOR_CONTROL, REFFIELD,  mv_control, pic->REFFIELD);



    // BLOCK CONTROL REGISTER Offset 0x2C
    BF_WRITE( VC1_0_SEQPIC_BLOCK_CONTROL, CBPTAB, blk_control, pic->CBPTAB);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, TTMFB, blk_control, pic->TTMBF);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, TTFRM, blk_control, pic->TTFRM);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, MV2BPTAB, blk_control, pic->MV2BPTAB);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, MV4BPTAB, blk_control, pic->MV4BPTAB);
    if ((pic->CurrField == 1) && (pic->SLICE_ADDR))
    {
        int mby = md->height * 2 + 2;
        mby = (mby + 15 ) / 16;
        pic->SLICE_ADDR -= (mby/2);
    }
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, INITIAL_MV_Y, blk_control, pic->SLICE_ADDR);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, BP_RAW_ID2, blk_control, md->bp_raw[0]);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, BP_RAW_ID1, blk_control, md->bp_raw[1]);
    BF_WRITE(VC1_0_SEQPIC_BLOCK_CONTROL, BP_RAW_ID0, blk_control, md->bp_raw[2]);

    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_SLICE_CONTROL_INFO;


    wi.vwi_payload[0] = recon_control;
    wi.vwi_payload[1] = mv_control;
    wi.vwi_payload[2] = blk_control;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items

/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_SLICE_OTHER_INFO_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t trans_data = 0;
    uint32_t vop_dquant = 0;
    uint32_t ref_bfraction = 0;

    vc1_PictureLayerHeader *pic = &(parser->info.picLayerHeader);
    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);

    BF_WRITE(VC1_0_SEQPIC_REFERENCE_B_FRACTION, BFRACTION_DEN, ref_bfraction, pic->BFRACTION_DEN);
    BF_WRITE(VC1_0_SEQPIC_REFERENCE_B_FRACTION, BFRACTION_NUM, ref_bfraction, pic->BFRACTION_NUM);
    BF_WRITE(VC1_0_SEQPIC_REFERENCE_B_FRACTION, REFDIST, ref_bfraction, md->REFDIST);

    if (md->DQUANT)
    {
        if (pic->PQDIFF == 7)
            BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, PQUANT_ALT, vop_dquant, pic->ABSPQ);
        else if (pic->DQUANTFRM == 1)
            BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, PQUANT_ALT, vop_dquant, pic->PQUANT + pic->PQDIFF + 1);
    }
    BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, DQUANTFRM, vop_dquant, pic->DQUANTFRM);
    BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, DQPROFILE, vop_dquant, pic->DQPROFILE);
    BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, DQES,      vop_dquant, pic->DQSBEDGE);
    BF_WRITE( VC1_0_SEQPIC_VOP_DEQUANT, DQBILEVEL, vop_dquant, pic->DQBILEVEL);

    BF_WRITE( VC1_0_SEQPIC_TRANSFORM_DATA, TRANSACFRM,  trans_data, pic->TRANSACFRM);
    BF_WRITE( VC1_0_SEQPIC_TRANSFORM_DATA, TRANSACFRM2, trans_data, pic->TRANSACFRM2);
    BF_WRITE( VC1_0_SEQPIC_TRANSFORM_DATA, TRANSDCTAB,  trans_data, pic->TRANSDCTAB);


    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_SLICE_OTHER_INFO;


    wi.vwi_payload[0] = trans_data;
    wi.vwi_payload[1] = vop_dquant;
    wi.vwi_payload[2] = ref_bfraction;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items



/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t imgstruct = 0;
    uint32_t fieldref_ctrl_id = 0;
    uint32_t smp_rangemap = 0;

    vc1_PictureLayerHeader *pic = &(parser->info.picLayerHeader);
    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);

    if ( pic->FCM == VC1_FCM_FIELD_INTERLACE ) {
        BF_WRITE(VC1_0_SEQPIC_IMAGE_STRUCTURE, IMG_STRUC, imgstruct, (pic->BottomField) ? 2 : 1);
    }

    BF_WRITE( VC1_0_SEQPIC_FIELD_REF_FRAME_ID, TOP_FIELD,    fieldref_ctrl_id, pic->BottomField);
    BF_WRITE( VC1_0_SEQPIC_FIELD_REF_FRAME_ID, SECOND_FIELD, fieldref_ctrl_id, pic->CurrField);
    if (parser->info.picLayerHeader.PTYPE == VC1_I_FRAME)
    {
        BF_WRITE(VC1_0_SEQPIC_FIELD_REF_FRAME_ID, ANCHOR, fieldref_ctrl_id, 1);
    }
    else
    {
        BF_WRITE(VC1_0_SEQPIC_FIELD_REF_FRAME_ID, ANCHOR, fieldref_ctrl_id, parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].anchor[pic->CurrField]);
    }

    if (VC1_PROFILE_ADVANCED != md->PROFILE)
    {
        if (pic->RANGEREDFRM)
        {
            //BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_Y_FLAG, smp_rangemap, md->RANGE_MAPY_FLAG);
            //BF_WRITE( VC1_0_SEQPIC_RANGE_MAP, RANGE_MAP_UV_FLAG, smp_rangemap, md->RANGE_MAPUV_FLAG);
            smp_rangemap = 0x11;
        }

    }

    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO;


    wi.vwi_payload[0] = imgstruct;
    wi.vwi_payload[1] = fieldref_ctrl_id;
    wi.vwi_payload[2] = smp_rangemap;

    viddec_pm_append_workitem( parent, &wi, false );
    return;
} // send_reorder_ref_items


/* send reorder frame item to host
 * future frame gets push to past   */
static inline void send_INT_COM_registers(void *parent, vc1_viddec_parser_t *parser)
{
    uint32_t intcomp_fwd_top = 0;
    uint32_t intcomp_fwd_bot = 0;
    uint32_t intcomp_bwd_top = 0;
    uint32_t intcomp_bwd_bot = 0;
    uint32_t intcomp_cur = 0;

    uint32_t POS_2nd_INTCOMP = 13;
    uint32_t MASK_1st_INTCOMP = 0x1fff;
    uint32_t MASK_2nd_INTCOMP = 0x3ffe000;

    vc1_PictureLayerHeader *pic = &(parser->info.picLayerHeader);
    viddec_workload_item_t wi;

    vc1_metadata_t *md = &(parser->info.metadata);



    if (VC1_SKIPPED_FRAME == pic->PTYPE)
    {
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_top =0;
        return;
    }

    if ( VC1_FCM_FIELD_INTERLACE != pic->FCM )
    {

        BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, INT_COMP_1, intcomp_cur, 1);
        BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SCALE_1, intcomp_cur, pic->LUMSCALE);
        BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SHIFT_1, intcomp_cur, pic->LUMSHIFT);

        if ( !((pic->MVMODE == VC1_MVMODE_INTENSCOMP) || (pic->INTCOMP)) )
            intcomp_cur = 0;

        if ( (VC1_BI_FRAME==pic->PTYPE)||(VC1_B_FRAME==pic->PTYPE)  )
        {
            parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_top = 0;
            parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_bot = 0;

            intcomp_bwd_top = parser->intcomp_top[0];
            intcomp_bwd_bot = parser->intcomp_bot[0];
            intcomp_fwd_bot = parser->intcomp_bot[1];


            if ( parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].id != (-1) )
            {
                if (VC1_SKIPPED_FRAME != parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].type)
                    intcomp_fwd_top = parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].intcomp_top;
            }
            else
            {
                if (VC1_SKIPPED_FRAME != parser->ref_frame[VC1_REF_FRAME_T_MINUS_2].type)
                    intcomp_fwd_top = parser->intcomp_top[1];
            }
        }
        else
        {  //I,P TYPE

            parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_top = intcomp_cur;

            if (VC1_FCM_FIELD_INTERLACE == parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].fcm)
            {
                intcomp_fwd_top = parser->intcomp_top[1];
                intcomp_fwd_top |= intcomp_cur << POS_2nd_INTCOMP;

                intcomp_fwd_bot = parser->intcomp_bot[1];
                intcomp_fwd_bot |= intcomp_cur << POS_2nd_INTCOMP;
            }
            else
            {
                intcomp_fwd_top = intcomp_cur;// << POS_2nd_INTCOMP;
                intcomp_fwd_bot = 0;
            }
        }
    }
    else
    {
        //FIELD INTERLACE
        //if(0!=md->INTCOMPFIELD)
        //No debugging

        if (md->INTCOMPFIELD == VC1_INTCOMP_BOTTOM_FIELD)
        {
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, INT_COMP_2, intcomp_cur, 1);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SCALE_2, intcomp_cur, md->LUMSCALE2);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SHIFT_2, intcomp_cur, md->LUMSHIFT2);
        }
        else
        {
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, INT_COMP_1, intcomp_cur, 1);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SCALE_1, intcomp_cur, pic->LUMSCALE);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SHIFT_1, intcomp_cur, pic->LUMSHIFT);
        }

        if (md->INTCOMPFIELD == VC1_INTCOMP_BOTH_FIELD)
        {
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, INT_COMP_2, intcomp_cur, 1);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SCALE_2, intcomp_cur, md->LUMSCALE2);
            BF_WRITE(VC1_0_SEQPIC_INTENSITY_COMPENSATION, LUMA_SHIFT_2, intcomp_cur, md->LUMSHIFT2);
        }

        if (pic->MVMODE != VC1_MVMODE_INTENSCOMP)
        {
            intcomp_cur = 0;
        }

        if (pic->CurrField == 0)
        {
            if (pic->TFF)
            {
                parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_top = intcomp_cur;
            }
            else
            {
                parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_bot = intcomp_cur;
            }
        }
        else
        {
            if (pic->TFF)
            {
                parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_bot = intcomp_cur;
            }
            else
            {
                parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].intcomp_top = intcomp_cur;
            }
        }

        if (pic->CurrField == 1)
        {  //SECOND FIELD

            if (VC1_B_FRAME != pic->PTYPE)
            {
                if (pic->TFF)
                {
                    intcomp_bwd_top = intcomp_cur & MASK_1st_INTCOMP;

                    intcomp_fwd_bot = (parser->intcomp_bot[1] & MASK_2nd_INTCOMP) >> POS_2nd_INTCOMP;  //???????
                    intcomp_fwd_bot |= (intcomp_cur & MASK_2nd_INTCOMP);

                    intcomp_fwd_top = parser->intcomp_top[1];
                }
                else
                {
                    intcomp_bwd_bot= (intcomp_cur & MASK_2nd_INTCOMP)>>POS_2nd_INTCOMP;

                    intcomp_fwd_top = (parser->intcomp_top[1] & MASK_2nd_INTCOMP) >> POS_2nd_INTCOMP;
                    intcomp_fwd_top |= (intcomp_cur&MASK_1st_INTCOMP) << POS_2nd_INTCOMP;

                    intcomp_fwd_bot = parser->intcomp_bot[1];
                }
            }
            else
            {    //B TYPE
                intcomp_fwd_top = parser->intcomp_top[1];
                intcomp_fwd_bot = parser->intcomp_bot[1];

                intcomp_bwd_top = parser->intcomp_top[0];
                intcomp_bwd_bot = parser->intcomp_bot[0];
            }
        }
        else
        {  //FIRST FILED

            if ( (VC1_B_FRAME==pic->PTYPE)||(VC1_BI_FRAME==pic->PTYPE) )
            {
                if (VC1_SKIPPED_FRAME!=parser->ref_frame[VC1_REF_FRAME_T_MINUS_2].type)
                {
                    intcomp_fwd_top = parser->intcomp_top[1];
                    intcomp_fwd_bot = parser->intcomp_bot[1];
                }

                intcomp_bwd_top = parser->intcomp_top[0];
                intcomp_bwd_bot = parser->intcomp_bot[0];

            }
            else
            {  //I,P TYPE

                intcomp_fwd_top = parser->intcomp_top[1] & MASK_1st_INTCOMP;
                intcomp_fwd_top |= (intcomp_cur&MASK_1st_INTCOMP)<<POS_2nd_INTCOMP;

                intcomp_fwd_bot = parser->intcomp_bot[1] & MASK_1st_INTCOMP;
                intcomp_fwd_bot |= (intcomp_cur & MASK_2nd_INTCOMP);
            }   //pic->PTYPE == I,P TYPE
        }   //pic->CurrField == 0
    }  //VC1_FCM_FIELD_INTERLACE != pic->FCM

    if ( (VC1_B_FRAME != pic->PTYPE) && (VC1_BI_FRAME != pic->PTYPE) )
    {
        parser->intcomp_top[1] = intcomp_fwd_top;
        parser->intcomp_bot[1] = intcomp_fwd_bot;

        parser->intcomp_top[0] = intcomp_bwd_top;
        parser->intcomp_bot[0] = intcomp_bwd_bot;
    }

    //OS_INFO("intcomp_fwd_top = %d\n", intcomp_fwd_top);
    //OS_INFO("intcomp_fwd_bot = %d\n", intcomp_fwd_bot);


    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_INT_COM_FW;

    wi.vwi_payload[0] = 0;
    wi.vwi_payload[1] = intcomp_fwd_top;
    wi.vwi_payload[2] = intcomp_fwd_bot;

    viddec_pm_append_workitem( parent, &wi, false );

    wi.vwi_type = VIDDEC_WORKLOAD_VC1_REGS_INT_COM_BW;

    wi.vwi_payload[0] = 0;
    wi.vwi_payload[1] = intcomp_bwd_top;
    wi.vwi_payload[2] = intcomp_bwd_bot;

    viddec_pm_append_workitem( parent, &wi, false );


    return;
} // send_reorder_ref_items


/** update workload with more workload items for ref and update values to store...
 */
void vc1_parse_emit_frame_start(void *parent, vc1_viddec_parser_t *parser)
{
    vc1_metadata_t *md = &(parser->info.metadata);
    viddec_workload_t *wl = viddec_pm_get_header(parent);
    int frame_type = parser->info.picLayerHeader.PTYPE;
    int frame_id = 1; // new reference frame is assigned index 1

    /* init */
    memset(&parser->spr, 0, sizeof(parser->spr));
    wl->is_reference_frame = 0;

    /* set flag - extra ouput frame needed for range adjustment (range mapping or range reduction */
    if (parser->info.metadata.RANGE_MAPY_FLAG ||
            parser->info.metadata.RANGE_MAPUV_FLAG ||
            parser->info.picLayerHeader.RANGEREDFRM)
    {
        wl->is_reference_frame |= WORKLOAD_FLAGS_RA_FRAME;
    }

    LOG_CRIT("vc1_start_new_frame: frame_type=%d \n",frame_type);

    parser->is_reference_picture = ((VC1_B_FRAME != frame_type) && (VC1_BI_FRAME != frame_type));

    /* reference / anchor frames processing
     * we need to send reorder before reference frames */
    if (parser->is_reference_picture)
    {
        /* one frame has been sent */
        if (parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].id != -1)
        {
            /* there is a frame in the reference buffer, move it to the past */
            send_reorder_ref_items(parent);
        }
    }

    /* send workitems for reference frames */
    switch ( frame_type )
    {
    case VC1_B_FRAME:
    {
        vc1_send_past_ref_items(parent);
        vc1_send_future_ref_items(parent);
        vc1_send_ref_fcm_items(parent, parser->ref_frame[VC1_REF_FRAME_T_MINUS_2].fcm, parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].fcm);
        break;
    }
    case VC1_SKIPPED_FRAME:
    {
        wl->is_reference_frame |= WORKLOAD_SKIPPED_FRAME;
        vc1_send_past_ref_items(parent);
        vc1_send_ref_fcm_items(parent, parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].fcm, vc1_PictureFormatNone);
        break;
    }
    case VC1_P_FRAME:
    {
        vc1_send_past_ref_items( parent);
        vc1_send_ref_fcm_items(parent, parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].fcm, vc1_PictureFormatNone);
        break;
    }
    default:
        break;
    }

    /* reference / anchor frames from previous code
     * we may need it for frame reduction */
    if (parser->is_reference_picture)
    {
        wl->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (frame_id & WORKLOAD_REFERENCE_FRAME_BMASK);

        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].id      = frame_id;
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].fcm     = parser->info.picLayerHeader.FCM;
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].anchor[0]  = (parser->info.picLayerHeader.PTYPE == VC1_I_FRAME);
        if (parser->info.picLayerHeader.FCM == VC1_FCM_FIELD_INTERLACE)
        {
            parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].anchor[1] = (parser->info.picLayerHeader.PTypeField2 == VC1_I_FRAME);
        }
        else
        {
            parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].anchor[1] = parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].anchor[0];
        }

        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].type = parser->info.picLayerHeader.PTYPE;
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].rr_en = md->RANGERED;
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].rr_frm = parser->info.picLayerHeader.RANGEREDFRM;
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_0].tff = parser->info.picLayerHeader.TFF;

        LOG_CRIT("anchor[0] = %d, anchor[1] = %d",
                 parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].anchor[0],
                 parser->ref_frame[VC1_REF_FRAME_T_MINUS_1].anchor[1] );
    }

    if ( parser->info.picLayerHeader.PTYPE == VC1_SKIPPED_FRAME )
    {
        translate_parser_info_to_frame_attributes( parent, parser );
        return;
    }

    translate_parser_info_to_frame_attributes( parent, parser );


    send_SEQ_ENTRY_registers(parent, parser);
    send_SIZE_AND_AP_RANGEMAP_registers(parent, parser);
    send_SLICE_FRAME_TYPE_INFO_registers(parent, parser);
    send_SLICE_CONTROL_INFO_registers(parent, parser);
    send_SLICE_OTHER_INFO_registers(parent, parser);
    send_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO_registers(parent, parser);
    send_INT_COM_registers(parent, parser);

    {
        viddec_workload_item_t wi;
        uint32_t bit, byte;
        uint8_t is_emul = 0;

        viddec_pm_get_au_pos(parent, &bit, &byte, &is_emul);

        // Send current bit offset and current slice
        wi.vwi_type          = VIDDEC_WORKLOAD_VC1_BITOFFSET;

        // If slice data starts in the middle of the emulation prevention sequence -
        // Special Case1----[is_emul = 1]:
        // Eg: 00 00 03 01 - slice data starts at the second byte of 0s, we still feed the data
        // to the decoder starting at the first byte of 0s so that the decoder can detect the
        // emulation prevention. But the actual data starts are offset 8 in this bit sequence.

        // Specail Case 2----[is_emul = 2]:
        // If slice data starts in the middle of the emulation prevention sequence -
        // Eg: [00 00] 03 00 - slice data starts at the third byte (0x03), we need readout this byte.
        //

        wi.vwi_payload[0]    = bit + (is_emul*8) ;
        wi.vwi_payload[1]    = 0xdeaddead;
        wi.vwi_payload[2]    = 0xdeaddead;
        viddec_pm_append_workitem( parent, &wi, false );
    }


    viddec_pm_append_pixeldata( parent );

    return;
} // vc1_start_new_frame


void vc1_parse_emit_second_field_start(void *parent, vc1_viddec_parser_t *parser)
{

    send_SLICE_FRAME_TYPE_INFO_registers(parent, parser);
    send_SLICE_CONTROL_INFO_registers(parent, parser);
    send_SLICE_OTHER_INFO_registers(parent, parser);
    send_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO_registers(parent, parser);
    send_INT_COM_registers(parent, parser);

    {
        viddec_workload_item_t wi;
        uint32_t bit, byte;
        uint8_t is_emul = 0;

        viddec_pm_get_au_pos(parent, &bit, &byte, &is_emul);


        // Send current bit offset and current slice
        wi.vwi_type          = VIDDEC_WORKLOAD_VC1_BITOFFSET;
        // If slice data starts in the middle of the emulation prevention sequence -
        // Special Case1----[is_emul = 1]:
        // Eg: 00 00 03 01 - slice data starts at the second byte of 0s, we still feed the data
        // to the decoder starting at the first byte of 0s so that the decoder can detect the
        // emulation prevention. But the actual data starts are offset 8 in this bit sequence.

        // Specail Case 2----[is_emul = 2]:
        // If slice data starts in the middle of the emulation prevention sequence -
        // Eg: [00 00] 03 00 - slice data starts at the third byte (0x03), we need readout this byte.
        //


        wi.vwi_payload[0]	 = bit + (is_emul*8);
        wi.vwi_payload[1]	 = 0xdeaddead;
        wi.vwi_payload[2]	 = 0xdeaddead;
        viddec_pm_append_workitem( parent, &wi, false );
    }

    viddec_pm_append_pixeldata( parent );

    return;

}


void vc1_parse_emit_current_slice(void *parent, vc1_viddec_parser_t *parser)
{
    send_SLICE_FRAME_TYPE_INFO_registers(parent, parser);
    send_SLICE_CONTROL_INFO_registers(parent, parser);
    send_SLICE_OTHER_INFO_registers(parent, parser);
    //send_STRUCT_FIELD_AND_SMP_RANGEMAP_INFO_registers(parent, parser);
    //send_INT_COM_registers(parent, parser);

    {
        viddec_workload_item_t wi;
        uint32_t bit, byte;
        uint8_t is_emul = 0;

        viddec_pm_get_au_pos(parent, &bit, &byte, &is_emul);

        // Send current bit offset and current slice
        wi.vwi_type          = VIDDEC_WORKLOAD_VC1_BITOFFSET;

        // If slice data starts in the middle of the emulation prevention sequence -
        // Special Case1----[is_emul = 1]:
        // Eg: 00 00 03 01 - slice data starts at the second byte of 0s, we still feed the data
        // to the decoder starting at the first byte of 0s so that the decoder can detect the
        // emulation prevention. But the actual data starts are offset 8 in this bit sequence.

        // Specail Case 2----[is_emul = 2]:
        // If slice data starts in the middle of the emulation prevention sequence -
        // Eg: [00 00] 03 00 - slice data starts at the third byte (0x03), we need readout this byte.
        //

        wi.vwi_payload[0]    = bit + (is_emul*8);
        wi.vwi_payload[1]    = 0xdeaddead;
        wi.vwi_payload[2]    = 0xdeaddead;
        viddec_pm_append_workitem( parent, &wi, false );
    }

    viddec_pm_append_pixeldata( parent );

    return;
}


void vc1_end_frame(vc1_viddec_parser_t *parser)
{
    /* update status of reference frames */
    if (parser->is_reference_picture)
    {
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_2] = parser->ref_frame[VC1_REF_FRAME_T_MINUS_1];
        parser->ref_frame[VC1_REF_FRAME_T_MINUS_1] = parser->ref_frame[VC1_REF_FRAME_T_MINUS_0];
    }

    return;
} // vc1_end_frame

