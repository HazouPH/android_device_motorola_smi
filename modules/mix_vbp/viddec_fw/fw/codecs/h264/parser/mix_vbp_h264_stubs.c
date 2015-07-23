#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "h264.h"
#include "h264parse.h"
#include "viddec_fw_item_types.h"
#include "h264parse_dpb.h"


extern void* h264_memcpy( void* dest, void* src, uint32_t num );

uint32_t cp_using_dma(uint32_t ddr_addr, uint32_t local_addr, uint32_t size, char to_ddr, char swap)
{
    if (swap != 0)
    {
        //g_warning("swap copying is not implemented.");
    }

    if (to_ddr)
    {
        memcpy((void*)ddr_addr, (void*)local_addr, size);
    }
    else
    {
        memcpy((void*)local_addr, (void*)ddr_addr, size);
    }

    return (0);
}

#if 0
void h264_parse_emit_start_new_frame( void *parent, h264_Info *pInfo )
{

    if (pInfo->Is_first_frame_in_stream) //new stream, fill new frame in cur
    {

        pInfo->img.g_new_frame = 0;
        pInfo->Is_first_frame_in_stream =0;
        pInfo->push_to_cur = 1;

    }
    else  // move to next for new frame
    {
        pInfo->push_to_cur = 0;
    }



    //fill dpb managemnt info




    pInfo->dpb.frame_numbers_need_to_be_displayed =0;
    pInfo->dpb.frame_numbers_need_to_be_removed =0;
    pInfo->dpb.frame_numbers_need_to_be_allocated =0;


}

void h264_parse_emit_eos( void *parent, h264_Info *pInfo )
{
    ////
    //// Now we can flush out all frames in DPB fro display
    if (pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].is_used != 3)
    {
        h264_dpb_mark_dangling_field(&pInfo->dpb, pInfo->dpb.fs_dec_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
    }

    h264_dpb_store_previous_picture_in_dpb(pInfo, 0,0);
    h264_dpb_flush_dpb(pInfo, 1, 0, pInfo->active_SPS.num_ref_frames);


    pInfo->dpb.frame_numbers_need_to_be_displayed =0;
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

}

void h264_parse_emit_current_pic( void *parent, h264_Info *pInfo )
{
    pInfo->qm_present_list=0;
}

void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo )
{
#if 1
    uint32_t  i, nitems=0;


    if ( (h264_PtypeB==pInfo->SliceHeader.slice_type)||(h264_PtypeP==pInfo->SliceHeader.slice_type) )
    {
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l0_active;

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->slice_ref_list0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->slice_ref_list0[i]&0xFF)|0x80;
                    break;
                }
            }
        }
        else
        {
            nitems = pInfo->dpb.listXsize[0];

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->dpb.listX_0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->dpb.listX_0[i]&0xFF)|0x80;
                    break;
                }
            }
        }

    }
    else
    {
        nitems =0;
    }
#endif
}
#else


void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;
    h264_slice_data 				slice_data = {};

    uint32_t		i=0, nitems=0, data=0;
    uint32_t 	bits_offset =0, byte_offset =0;
    uint8_t    	is_emul =0;


    ////////////////////// Update Reference list //////////////////
    if ( (h264_PtypeB==pInfo->SliceHeader.slice_type)||(h264_PtypeP==pInfo->SliceHeader.slice_type) )
    {
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l0_active;

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->slice_ref_list0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->slice_ref_list0[i]&0xFF)|0x80;
                    break;
                }
            }
        }
        else
        {
            nitems = pInfo->dpb.listXsize[0];

            for (i=0; i<nitems; i++)
            {
                if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[pInfo->dpb.listX_0[i]&0x1f]))==0)
                {
                    pInfo->h264_list_replacement = (pInfo->dpb.listX_0[i]&0xFF)|0x80;
                    break;
                }
            }
        }

    }
    else
    {
        nitems =0;
    }
    /////file ref list 0
    // h264_parse_emit_ref_list(parent, pInfo, 0);

    /////file ref list 1
    //h264_parse_emit_ref_list(parent, pInfo, 1);

    ///////////////////////////////////// Slice Data ////////////////////////////////
    // h264_fill_slice_data(pInfo, &slice_data);

    wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_H264_SLICE_REG);

    wi.data.data_offset = slice_data.h264_bsd_slice_start;
    wi.data.data_payload[0] = slice_data.h264_bsd_slice_p1;
    wi.data.data_payload[1] = slice_data.h264_bsd_slice_p2;

    if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
    {
        // viddec_pm_append_workitem( parent , &wi);
    }
    else
    {
        // viddec_pm_append_workitem_next( parent , &wi);
    }


    ///////////////////////////predict weight table item and data if have///////////////////////////
    if (pInfo->h264_pwt_enabled)
    {
        wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_H264_PWT_BITS_OFFSET;
        wi.data.data_offset = pInfo->h264_pwt_end_byte_offset- pInfo->h264_pwt_start_byte_offset+1;
        wi.data.data_payload[0] = pInfo->h264_pwt_start_bit_offset;
        wi.data.data_payload[1] = pInfo->h264_pwt_end_bit_offset;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            // viddec_pm_append_workitem( parent , &wi);

            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_H264_PWT_ES_BYTES;
            wi.es.es_flags = 0;
            // viddec_pm_append_misc_tags(parent, pInfo->h264_pwt_start_byte_offset, pInfo->h264_pwt_end_byte_offset,&wi,1);
        }
        else
        {
            //  viddec_pm_append_workitem_next( parent , &wi);

            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_H264_PWT_ES_BYTES;
            wi.es.es_flags = 0;
            //  viddec_pm_append_misc_tags(parent, pInfo->h264_pwt_start_byte_offset, pInfo->h264_pwt_end_byte_offset,&wi,0);
        }
    }


    ////////////////////////////////// Update ES Buffer for Slice ///////////////////////
    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

    //OS_INFO("DEBUG---entropy_coding_mode_flag:%d, bits_offset: %d\n", pInfo->active_PPS.entropy_coding_mode_flag, bits_offset);

    if (pInfo->active_PPS.entropy_coding_mode_flag)
    {
        if (0!=bits_offset)  {
            data = data; // fix compilation warning
            // don't skip byte-aligned bits as those bits are actually
            // part of slice_data
            //viddec_pm_get_bits(parent, &data, 8-bits_offset);
        }
    }
    else
    {
        if (0!=bits_offset)  {
            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_H264_SH_BITS_OFFSET;
            wi.data.data_offset = bits_offset;
            wi.data.data_payload[0]=0;
            wi.data.data_payload[1]=0;

            if (pInfo->push_to_cur) {			//cur is empty, fill new frame in cur
                // viddec_pm_append_workitem( parent , &wi);
            }
            else {
                //viddec_pm_append_workitem_next( parent , &wi);
            }
        }
    }

    if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
    {
        //viddec_pm_append_pixeldata( parent );
    }
    else
    {
        //viddec_pm_append_pixeldata_next( parent);
    }

    return;
}


void h264_parse_emit_current_pic( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;

    const uint32_t             *pl;
    uint32_t                   i=0,nitems=0;

    h264_pic_data pic_data;

    pInfo->qm_present_list=0;

    //h264_parse_emit_4X4_scaling_matrix(parent, pInfo);
    // h264_parse_emit_8X8_scaling_matrix(parent, pInfo);

    // h264_fill_pic_data(pInfo, &pic_data);

    // How many payloads must be generated
    nitems = (sizeof(h264_pic_data) + 7) / 8; // In QWORDs rounded up

    pl = (const uint32_t *) &pic_data;

    // Dump slice data to an array of workitems,  to do pl access non valid mem
    for ( i = 0; i < nitems; i++ )
    {
        wi.vwi_type           = (workload_item_type)VIDDEC_WORKLOAD_H264_PIC_REG;
        wi.data.data_offset   = (unsigned int)pl - (unsigned int)&pic_data; // offset within struct
        wi.data.data_payload[0] = pl[0];
        wi.data.data_payload[1] = pl[1];
        pl += 2;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {

            //  viddec_pm_append_workitem( parent, &wi );
        }
        else
        {
            //viddec_pm_append_workitem_next( parent, &wi );
        }
    }

    return;
}

void h264_parse_emit_start_new_frame( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;
    uint32_t                   i=0,nitems=0;

    ///////////////////////// Frame attributes//////////////////////////

    //Push data into current workload if first frame or frame_boundary already detected by non slice nal
    if ( (pInfo->Is_first_frame_in_stream)||(pInfo->is_frame_boundary_detected_by_non_slice_nal))
    {
        //viddec_workload_t			*wl_cur = viddec_pm_get_header( parent );
        //pInfo->img.g_new_frame = 0;
        pInfo->Is_first_frame_in_stream =0;
        pInfo->is_frame_boundary_detected_by_non_slice_nal=0;
        pInfo->push_to_cur = 1;
        //h264_translate_parser_info_to_frame_attributes(wl_cur, pInfo);
    }
    else  // move to cur if frame boundary detected by previous non slice nal, or move to next if not
    {
        //viddec_workload_t        *wl_next = viddec_pm_get_next_header (parent);

        pInfo->push_to_cur = 0;
        //h264_translate_parser_info_to_frame_attributes(wl_next, pInfo);

        pInfo->is_current_workload_done=1;
    }

    ///////////////////// SPS/////////////////////
    // h264_parse_emit_sps(parent, pInfo);

    /////////////////////display frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_displayed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_REF_FRAME_DISPLAY_0 + pInfo->dpb.frame_id_need_to_be_displayed[i]);
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            // viddec_pm_append_workitem( parent, &wi );
        }
        else
        {
            // viddec_pm_append_workitem_next( parent, &wi );
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_removed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_REF_FRAME_RELEASE_0 + pInfo->dpb.frame_id_need_to_be_removed[i]);
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            //viddec_pm_append_workitem( parent, &wi );
        }
        else
        {
            // viddec_pm_append_workitem_next( parent, &wi );
        }

    }
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    /////////////////////flust frames (do not display)/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_dropped;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_REF_FRAME_DROPOUT_0 + pInfo->dpb.frame_id_need_to_be_dropped[i]);
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_dropped[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            //viddec_pm_append_workitem( parent, &wi );
        }
        else
        {
            // viddec_pm_append_workitem_next( parent, &wi );
        }

    }
    pInfo->dpb.frame_numbers_need_to_be_dropped =0;

    /////////////////////updata DPB frames/////////////////////
    nitems = pInfo->dpb.used_size;
    for (i=0; i<nitems; i++)
    {
        uint8_t fs_id = pInfo->dpb.fs_dpb_idc[i];

        if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[fs_id])) == 0)
        {
            wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_DPB_ACTIVE_FRAME_0+fs_id);
            wi.ref_frame.reference_id = fs_id;
            wi.ref_frame.luma_phys_addr = 0;
            wi.ref_frame.chroma_phys_addr = 0;

            if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
            {
                // viddec_pm_append_workitem( parent, &wi );
            }
            else
            {
                //viddec_pm_append_workitem_next( parent, &wi );
            }
        }
    }


    /////////////////////updata dpb frames info (poc)/////////////////////
    nitems = pInfo->dpb.used_size;
    for (i=0; i<nitems; i++)
    {
        uint8_t fs_id = pInfo->dpb.fs_dpb_idc[i];

        if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[fs_id])) == 0)
        {
            wi.vwi_type = (workload_item_type)VIDDEC_WORKLOAD_H264_DPB_FRAME_POC;
            wi.data.data_offset = fs_id;
            //printf("is_used = %d, tpoc = %d, bpoc = %d\n", pInfo->dpb.fs[fs_id].is_used, pInfo->dpb.fs[fs_id].top_field.poc, pInfo->dpb.fs[fs_id].bottom_field.poc);

            switch (viddec_h264_get_is_used(&(pInfo->dpb.fs[fs_id])))
            {
            case (FRAME): {
                wi.data.data_payload[0] = pInfo->dpb.fs[fs_id].top_field.poc;
                wi.data.data_payload[1] = pInfo->dpb.fs[fs_id].bottom_field.poc;
                break;
            };

            case (TOP_FIELD): {
                wi.data.data_payload[0] = pInfo->dpb.fs[fs_id].top_field.poc;
                wi.data.data_payload[1] = 0;
                break;
            };

            case (BOTTOM_FIELD): {
                wi.data.data_payload[0] = 0;
                wi.data.data_payload[1] = pInfo->dpb.fs[fs_id].bottom_field.poc;
                break;
            };

            default : {
                wi.data.data_payload[0] = 0;
                wi.data.data_payload[1] = 0;
                break;
            };
            }


            if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
            {
                //  viddec_pm_append_workitem( parent, &wi );
            }
            else
            {
                //viddec_pm_append_workitem_next( parent, &wi );
            }

        }
    }

    /////////////////////Alloc buffer for current Existing frame/////////////////////
    if (0!=pInfo->dpb.frame_numbers_need_to_be_allocated)
    {
        if (pInfo->push_to_cur)
        {
            // viddec_workload_t        *wl_cur = viddec_pm_get_header (parent);
            //  wl_cur->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (pInfo->dpb.frame_id_need_to_be_allocated & 0x1f);
        }
        else
        {
            // viddec_workload_t        *wl_next = viddec_pm_get_next_header (parent);
            //wl_next->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (pInfo->dpb.frame_id_need_to_be_allocated & 0x1f);
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_allocated =0;

    return;
}



void h264_parse_emit_eos( void *parent, h264_Info *pInfo )
{

    uint32_t nitems=0, i=0;
    viddec_workload_item_t	wi;

    ////
    //// Now we can flush out all frames in DPB fro display
    if (viddec_h264_get_is_used(&(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc])) != 3)
    {
        h264_dpb_mark_dangling_field(&pInfo->dpb, pInfo->dpb.fs_dec_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
    }

    h264_dpb_store_previous_picture_in_dpb(pInfo, 0,0);
    h264_dpb_flush_dpb(pInfo, 1, 0, pInfo->active_SPS.num_ref_frames);


    /////////////////////display frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_displayed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_EOS_DISPLAY_FRAME_0 + pInfo->dpb.frame_id_need_to_be_displayed[i]);
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            //viddec_pm_append_workitem( parent, &wi );
        }
        else
        {
            //viddec_pm_append_workitem_next( parent, &wi );
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_removed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = (workload_item_type)(VIDDEC_WORKLOAD_EOS_RELEASE_FRAME_0 + pInfo->dpb.frame_id_need_to_be_removed[i]);
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            //  viddec_pm_append_workitem( parent, &wi );
            viddec_pm_set_next_frame_error_on_eos(parent, VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE);
        }
        else
        {
            // viddec_pm_append_workitem_next( parent, &wi );
            viddec_pm_set_next_frame_error_on_eos(parent, pInfo->wl_err_next);
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    return;
}
#endif
