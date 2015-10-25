#ifndef VBP
#include <string.h>

#include "viddec_fw_workload.h"
#include "viddec_parser_ops.h"
#include "viddec_fw_mp4.h"
#include "viddec_mp4_parse.h"

uint32_t viddec_fw_mp4_populate_attr(viddec_workload_t *wl, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_frame_attributes_t *attr = &(wl->attrs);
    mp4_VideoObjectLayer_t *vol = &(parser->info.VisualObject.VideoObject);

    memset(attr, 0, sizeof(viddec_frame_attributes_t));

    attr->cont_size.width = vol->video_object_layer_width;
    attr->cont_size.height = vol->video_object_layer_height;

    // Translate vop_coding_type
    switch (vol->VideoObjectPlane.vop_coding_type)
    {
    case MP4_VOP_TYPE_B:
        attr->frame_type = VIDDEC_FRAME_TYPE_B;
        break;
    case MP4_VOP_TYPE_P:
        attr->frame_type = VIDDEC_FRAME_TYPE_P;
        break;
    case MP4_VOP_TYPE_S:
        attr->frame_type = VIDDEC_FRAME_TYPE_S;
        break;
    case MP4_VOP_TYPE_I:
        attr->frame_type = VIDDEC_FRAME_TYPE_I;
        break;
    default:
        break;
    } // switch on vop_coding_type

    attr->mpeg4.top_field_first = vol->VideoObjectPlane.top_field_first;

    return result;
} // viddec_fw_mp4_populate_attr

uint32_t viddec_fw_mp4_insert_vol_workitem(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    viddec_fw_mp4_vol_info_t vol_info;
    mp4_VideoObjectLayer_t *vol = &(parser->info.VisualObject.VideoObject);

    memset(&vol_info, 0, sizeof(viddec_fw_mp4_vol_info_t));

    // Get vol_flags
    viddec_fw_mp4_set_reversible_vlc(&vol_info, vol->reversible_vlc);
    viddec_fw_mp4_set_data_partitioned(&vol_info, vol->data_partitioned);
    viddec_fw_mp4_set_resync_marker_disable(&vol_info, vol->resync_marker_disable);
    viddec_fw_mp4_set_quarter_sample(&vol_info, vol->quarter_sample);
    viddec_fw_mp4_set_obmc_disable(&vol_info, vol->obmc_disable);
    viddec_fw_mp4_set_interlaced(&vol_info, vol->interlaced);
    viddec_fw_mp4_set_vol_shape(&vol_info, vol->video_object_layer_shape);
    viddec_fw_mp4_set_short_video_header_flag(&vol_info, vol->short_video_header);

    // Get vol_size
    viddec_fw_mp4_set_vol_width(&vol_info, vol->video_object_layer_width);
    viddec_fw_mp4_set_vol_height(&vol_info, vol->video_object_layer_height);

    // Get vol_item
    viddec_fw_mp4_set_quant_type(&vol_info, vol->quant_type);
    viddec_fw_mp4_set_quant_precision(&vol_info, vol->quant_precision);
    viddec_fw_mp4_set_sprite_warping_accuracy(&vol_info, vol->sprite_info.sprite_warping_accuracy);
    viddec_fw_mp4_set_sprite_warping_points(&vol_info, vol->sprite_info.no_of_sprite_warping_points);
    viddec_fw_mp4_set_sprite_enable(&vol_info, vol->sprite_enable);
    viddec_fw_mp4_set_vop_time_increment_resolution(&vol_info, vol->vop_time_increment_resolution);


    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_VOL_INFO;
    wi.vwi_payload[0] = vol_info.vol_flags;
    wi.vwi_payload[1] = vol_info.vol_size;
    wi.vwi_payload[2] = vol_info.vol_item;

    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_vol_workitem

uint32_t viddec_fw_mp4_insert_vop_workitem(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    viddec_fw_mp4_vop_info_t vop_info;
    mp4_VideoObjectPlane_t *vop = &(parser->info.VisualObject.VideoObject.VideoObjectPlane);
    uint32_t byte = 0;
    unsigned char is_emul;

    memset(&vop_info, 0, sizeof(viddec_fw_mp4_vop_info_t));

    // Get frame_info
    viddec_fw_mp4_set_past_field_frame(&vop_info, parser->ref_frame[VIDDEC_MP4_INDX_2].is_field);
    viddec_fw_mp4_set_past_frame_id(&vop_info, VIDDEC_MP4_FRAME_PAST);
    viddec_fw_mp4_set_future_field_frame(&vop_info, parser->ref_frame[VIDDEC_MP4_INDX_1].is_field);
    viddec_fw_mp4_set_future_frame_id(&vop_info, VIDDEC_MP4_FRAME_FUTURE);
    viddec_fw_mp4_set_current_field_frame(&vop_info, parser->ref_frame[VIDDEC_MP4_INDX_0].is_field);
    viddec_fw_mp4_set_current_frame_id(&vop_info, VIDDEC_MP4_FRAME_CURRENT);

    // HW has a limitation that the enums for PAST(1), FUTURE(2) and CURRENT(0) cannot be changed and
    // the spec does not support field pictures. Hence the field_frame bits are always zero.
    // This gives us the constant 0x10200.
    vop_info.frame_info = 0x10200;

    // Get vop_data
    // Quant scale is in the video_packet_header or the gob_layer - both of which are parsed by the BSP
    viddec_fw_mp4_set_vop_quant_scale(&vop_info, 0);
    viddec_fw_mp4_set_vop_fcode_backward(&vop_info, vop->vop_fcode_backward);
    viddec_fw_mp4_set_vop_fcode_forward(&vop_info, vop->vop_fcode_forward);
    viddec_fw_mp4_set_vop_quant(&vop_info, vop->vop_quant);
    viddec_fw_mp4_set_alternate_vertical_scan_flag(&vop_info, vop->alternate_vertical_scan_flag);
    viddec_fw_mp4_set_top_field_first(&vop_info, vop->top_field_first);
    viddec_fw_mp4_set_intra_dc_vlc_thr(&vop_info, vop->intra_dc_vlc_thr);
    viddec_fw_mp4_set_vop_rounding_type(&vop_info, vop->vop_rounding_type);
    viddec_fw_mp4_set_vop_coding_type(&vop_info, vop->vop_coding_type);

    // Get vol_item
    result = viddec_pm_get_au_pos(parent, &vop_info.bit_offset, &byte, &is_emul);

    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_VOP_INFO;
    wi.vwi_payload[0] = vop_info.frame_info;
    wi.vwi_payload[1] = vop_info.vop_data;
    wi.vwi_payload[2] = vop_info.bit_offset;

    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_vop_workitem

uint32_t viddec_fw_mp4_insert_vpsh_workitem(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    viddec_fw_mp4_svh_t svh_info;
    mp4_VideoObjectPlaneH263 *svh = &(parser->info.VisualObject.VideoObject.VideoObjectPlaneH263);

    memset(&svh_info, 0, sizeof(viddec_fw_mp4_svh_t));

    // Get svh_data
    viddec_fw_mp4_set_temporal_reference(&svh_info, svh->temporal_reference);
    viddec_fw_mp4_set_num_macroblocks_in_gob(&svh_info, svh->num_macroblocks_in_gob);
    viddec_fw_mp4_set_num_gobs_in_vop(&svh_info, svh->num_gobs_in_vop);
    viddec_fw_mp4_set_num_rows_in_gob(&svh_info, svh->num_rows_in_gob);

    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_SVH;
    wi.vwi_payload[0] = svh_info.svh_data;
    wi.vwi_payload[1] = svh_info.pad1;
    wi.vwi_payload[2] = svh_info.pad2;

    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_vpsh_workitem

uint32_t viddec_fw_mp4_insert_sprite_workitem(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    viddec_fw_mp4_sprite_trajectory_t sprite_info;
    mp4_VideoObjectLayer_t *vol = &(parser->info.VisualObject.VideoObject);
    mp4_VideoObjectPlane_t *vop = &(parser->info.VisualObject.VideoObject.VideoObjectPlane);
    uint8_t no_of_entries_per_item = 3;
    uint8_t no_of_sprite_workitems = 0;
    uint8_t warp_index = 0;
    int i, j;

    if (!vol->sprite_info.no_of_sprite_warping_points)
        return result;

    no_of_sprite_workitems = (vol->sprite_info.no_of_sprite_warping_points > 3) ? 2 : 1;

    for (i=0; i<no_of_sprite_workitems; i++)
    {
        memset(&sprite_info, 0, sizeof(viddec_fw_mp4_sprite_trajectory_t));

        for (j=0; j<no_of_entries_per_item; j++)
        {
            if (warp_index < vol->sprite_info.no_of_sprite_warping_points)
            {
                if (warp_index < 4)
                {
                    viddec_fw_mp4_set_warping_point_index(sprite_info.warping_mv_code[j], warp_index);
                    viddec_fw_mp4_set_warping_mv_code_du(sprite_info.warping_mv_code[j], vop->warping_mv_code_du[warp_index]);
                    viddec_fw_mp4_set_warping_mv_code_dv(sprite_info.warping_mv_code[j], vop->warping_mv_code_dv[warp_index]);
                }
            }
            else
            {
                sprite_info.warping_mv_code[j] = 0xF << 28;
            }
            warp_index++;
        }

        wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_SPRT_TRAJ;
        wi.vwi_payload[0] = sprite_info.warping_mv_code[0];
        wi.vwi_payload[1] = sprite_info.warping_mv_code[1];
        wi.vwi_payload[2] = sprite_info.warping_mv_code[2];

        result = viddec_pm_append_workitem(parent, &wi, false);
    }

    return result;
} // viddec_fw_mp4_insert_sprite_workitem

uint32_t viddec_fw_mp4_insert_bvop_workitem(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    mp4_VideoObjectLayer_t *vol = &(parser->info.VisualObject.VideoObject);

    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_BVOP_INFO;
    wi.vwi_payload[0] = vol->Tframe;
    wi.vwi_payload[1] = vol->TRD;
    wi.vwi_payload[2] = vol->TRB;

    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_bvop_workitem

uint32_t viddec_fw_mp4_insert_qmat(void *parent, uint8_t intra_quant_flag, uint32_t *qmat)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;
    uint8_t i;

    // No of items = (64/4 Dwords / 3 entries per workload item)
    // 64 8b entries => 64 * 8 / 32 DWORDS => 64/4 DWORDS => 16 DWORDS
    // Each item can store 3 DWORDS, 16 DWORDS => 16/3 items => 6 items
    for (i=0; i<6; i++)
    {
        memset(&wi, 0, sizeof(viddec_workload_item_t));

        if (intra_quant_flag)
            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_IQUANT;
        else
            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_NIQUANT;

        if (i == 6)
        {
            wi.vwi_payload[0] = qmat[0];
            wi.vwi_payload[1] = 0;
            wi.vwi_payload[2] = 0;
        }
        else
        {
            wi.vwi_payload[0] = qmat[0];
            wi.vwi_payload[1] = qmat[1];
            wi.vwi_payload[2] = qmat[2];
        }

        qmat += 3;

        result = viddec_pm_append_workitem(parent, &wi, false);
    }

    return result;
} // viddec_fw_mp4_insert_qmat

uint32_t viddec_fw_mp4_insert_inversequant_workitem(void *parent, mp4_VOLQuant_mat_t *qmat)
{
    uint32_t result = MP4_STATUS_OK;

    if (qmat->load_intra_quant_mat)
    {
        result = viddec_fw_mp4_insert_qmat(parent, true, (uint32_t *) &(qmat->intra_quant_mat));
    }

    if (qmat->load_nonintra_quant_mat)
    {
        result = viddec_fw_mp4_insert_qmat(parent, false, (uint32_t *) &(qmat->nonintra_quant_mat));
    }

    return result;
} // viddec_fw_mp4_insert_inversequant_workitem

uint32_t viddec_fw_mp4_insert_past_frame_workitem(void *parent)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;

    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_PAST_FRAME;
    wi.ref_frame.reference_id = 0;
    wi.ref_frame.luma_phys_addr = 0;
    wi.ref_frame.chroma_phys_addr = 0;
    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_past_frame_workitem

uint32_t viddec_fw_mp4_insert_future_frame_workitem(void *parent)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;

    wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_MP4_FUTURE_FRAME;
    wi.ref_frame.reference_id = 0;
    wi.ref_frame.luma_phys_addr = 0;
    wi.ref_frame.chroma_phys_addr = 0;
    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_future_frame_workitem

uint32_t viddec_fw_mp4_insert_reorder_workitem(void *parent)
{
    uint32_t result = MP4_STATUS_OK;
    viddec_workload_item_t wi;

    // Move frame at location 1 of the reference table to location 0
    wi.vwi_type = VIDDEC_WORKLOAD_REFERENCE_FRAME_REORDER;
    wi.ref_reorder.ref_table_offset = 0;
    wi.ref_reorder.ref_reorder_00010203 = 0x01010203;
    wi.ref_reorder.ref_reorder_04050607 = 0x04050607;

    result = viddec_pm_append_workitem(parent, &wi, false);

    return result;
} // viddec_fw_mp4_insert_reorder_workitem

uint32_t viddec_fw_mp4_emit_workload(void *parent, void *ctxt)
{
    uint32_t result = 0;
    viddec_mp4_parser_t *parser = (viddec_mp4_parser_t *) ctxt;
    viddec_workload_t *wl = viddec_pm_get_header(parent);

    result = viddec_fw_mp4_populate_attr(wl, parser);
    result = viddec_fw_mp4_insert_vol_workitem(parent, parser);
    result = viddec_fw_mp4_insert_vop_workitem(parent, parser);
    result = viddec_fw_mp4_insert_sprite_workitem(parent, parser);
    result = viddec_fw_mp4_insert_inversequant_workitem(parent, &(parser->info.VisualObject.VideoObject.quant_mat_info));

    if (parser->info.VisualObject.VideoObject.short_video_header)
        result = viddec_fw_mp4_insert_vpsh_workitem(parent, parser);

    if (!parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_coded)
        wl->is_reference_frame |= WORKLOAD_SKIPPED_FRAME;

    // Send reference re-order tag for all reference frame types
    if (parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_coding_type != MP4_VOP_TYPE_B)
    {
        result = viddec_fw_mp4_insert_reorder_workitem(parent);
    }

    // Handle vop_coding_type based information
    switch (parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_coding_type)
    {
    case MP4_VOP_TYPE_B:
        result = viddec_fw_mp4_insert_bvop_workitem(parent, parser);
        result = viddec_fw_mp4_insert_past_frame_workitem(parent);
        result = viddec_fw_mp4_insert_future_frame_workitem(parent);
        break;
    case MP4_VOP_TYPE_P:
    case MP4_VOP_TYPE_S:
        result = viddec_fw_mp4_insert_past_frame_workitem(parent);
        // Deliberate fall-thru to type I
    case MP4_VOP_TYPE_I:
        wl->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (1 & WORKLOAD_REFERENCE_FRAME_BMASK);
        // Swap reference information
        parser->ref_frame[VIDDEC_MP4_INDX_2] = parser->ref_frame[VIDDEC_MP4_INDX_1];
        parser->ref_frame[VIDDEC_MP4_INDX_1] = parser->ref_frame[VIDDEC_MP4_INDX_0];
        break;
        break;
    default:
        break;
    } // switch on vop_coding_type

    result = viddec_pm_append_pixeldata(parent);

    return result;
} // viddec_fw_mp4_emit_workload
#endif
