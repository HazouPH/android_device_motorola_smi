#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"

#include "viddec_fw_workload.h"
#include "viddec_pm.h"

#include "h264.h"
#include "h264parse.h"

#include "viddec_h264_parse.h"
#include "h264parse_dpb.h"

/* Init function which can be called to intialized local context on open and flush and preserve*/
#ifdef VBP
void viddec_h264_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
#else
static void viddec_h264_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
#endif
{
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);

    if (!preserve)
    {
        /* we don't initialize this data if we want to preserve
           sequence and gop information */
        h264_init_sps_pps(parser,persist_mem);
    }
    /* picture level info which will always be initialized */
    h264_init_Info_under_sps_pps_level(pInfo);
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
   pInfo->sw_bail = 0;
#endif
#endif
    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
#ifdef VBP
uint32_t viddec_h264_parse(void *parent, void *ctxt)
#else
static uint32_t viddec_h264_parse(void *parent, void *ctxt)
#endif
{
    struct h264_viddec_parser* parser = ctxt;

    h264_Info * pInfo = &(parser->info);

    h264_Status status = H264_STATUS_ERROR;


    uint8_t nal_ref_idc = 0;

    ///// Parse NAL Unit header
    pInfo->img.g_new_frame = 0;
    pInfo->push_to_cur = 1;
    pInfo->is_current_workload_done =0;
    pInfo->nal_unit_type = 0;

    h264_Parse_NAL_Unit(parent, pInfo, &nal_ref_idc);

    ///// Check frame bounday for non-vcl elimitter
    h264_check_previous_frame_end(pInfo);

    //OS_INFO("========================nal_type: %d=================\n", pInfo->nal_unit_type);
    //DEBUG_WRITE(pInfo->nal_unit_type, pInfo->got_start, pInfo->wl_err_flag, pInfo->is_current_workload_done, 0, 0);
#if 0
    devh_SVEN_WriteModuleEvent( NULL,
                                SVEN_MODULE_EVENT_GV_FW_PARSER_DEBUG_P0,
                                pInfo->got_start,pInfo->nal_unit_type,  pInfo->wl_err_curr, pInfo->is_current_workload_done, 0, pInfo->img.frame_num);
#endif

    //////// Parse valid NAL unit
    switch ( pInfo->nal_unit_type )
    {
    case h264_NAL_UNIT_TYPE_IDR:
        if (pInfo->got_start)	{
            pInfo->img.recovery_point_found |= 1;
        }

        pInfo->sei_rp_received = 0;

    case h264_NAL_UNIT_TYPE_SLICE:
        ////////////////////////////////////////////////////////////////////////////
        // Step 1: Check start point
        ////////////////////////////////////////////////////////////////////////////
        //
        /// Slice parsing must start from the valid start point( SPS, PPS,  IDR or recovery point or primary_I)
        /// 1) No start point reached, append current ES buffer to workload and release it
        /// 2) else, start parsing
        //
        //if(pInfo->got_start && ((pInfo->sei_information.recovery_point) || (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)))
        //{
        //pInfo->img.recovery_point_found = 1;
        //}
    {

        h264_Slice_Header_t next_SliceHeader;

        /// Reset next slice header
        h264_memset(&next_SliceHeader, 0x0, sizeof(h264_Slice_Header_t));
        next_SliceHeader.nal_ref_idc = nal_ref_idc;

        if ( (1==pInfo->primary_pic_type_plus_one)&&(pInfo->got_start))
        {
            pInfo->img.recovery_point_found |=4;
        }
        pInfo->primary_pic_type_plus_one = 0;



#ifndef VBP
        if (pInfo->img.recovery_point_found == 0) {
            pInfo->img.structure = FRAME;
            pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
            pInfo->wl_err_curr |= (FRAME << FIELD_ERR_OFFSET);
            break;
        }
#endif

        ////////////////////////////////////////////////////////////////////////////
        // Step 2: Parsing slice header
        ////////////////////////////////////////////////////////////////////////////
        /// PWT
        pInfo->h264_pwt_start_byte_offset=0;
        pInfo->h264_pwt_start_bit_offset=0;
        pInfo->h264_pwt_end_byte_offset=0;
        pInfo->h264_pwt_end_bit_offset=0;
        pInfo->h264_pwt_enabled =0;
        /// IDR flag
        next_SliceHeader.idr_flag = (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR);


        /// Pass slice header
        status = h264_Parse_Slice_Layer_Without_Partitioning_RBSP(parent, pInfo, &next_SliceHeader);

        pInfo->sei_information.recovery_point = 0;

        if (next_SliceHeader.sh_error & 3) {
            pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;

            // Error type definition, refer to viddec_fw_common_defs.h
            //		if error in top field, VIDDEC_FW_WORKLOAD_ERR_TOPFIELD			= (1 << 17)
            //		if error in bottom field, VIDDEC_FW_WORKLOAD_ERR_BOTTOMFIELD	   = (1 << 18)
            //		if this is frame based, both 2 bits should be set
            pInfo->wl_err_curr |= (FRAME << FIELD_ERR_OFFSET);

            break;
        }
        pInfo->img.current_slice_num++;


#ifdef DUMP_HEADER_INFO
        dump_slice_header(pInfo, &next_SliceHeader);
////h264_print_decoder_values(pInfo);
#endif


        ////////////////////////////////////////////////////////////////////////////
        // Step 3: Processing if new picture coming
        //  1) if it's the second field
        //	2) if it's a new frame
        ////////////////////////////////////////////////////////////////////////////
        //AssignQuantParam(pInfo);
        if (h264_is_new_picture_start(pInfo, next_SliceHeader, pInfo->SliceHeader))
        {
            //
            ///----------------- New Picture.boundary detected--------------------
            //
            pInfo->img.g_new_pic++;

            //
            // Complete previous picture
            h264_dpb_store_previous_picture_in_dpb(pInfo, 0, 0); //curr old
            //h264_hdr_post_poc(0, 0, use_old);

            //
            // Update slice structures:
            h264_update_old_slice(pInfo, next_SliceHeader);  	//cur->old; next->cur;

            //
            // 1) if resolution change: reset dpb
            // 2) else: init frame store
            h264_update_img_info(pInfo);								//img, dpb

            //
            ///----------------- New frame.boundary detected--------------------
            //
            pInfo->img.second_field = h264_is_second_field(pInfo);
            if (pInfo->img.second_field == 0)
            {
                pInfo->img.g_new_frame = 1;
                h264_dpb_update_queue_dangling_field(pInfo);

                //
                /// DPB management
                ///	1) check the gaps
                ///	2) assign fs for non-exist frames
                ///	3) fill the gaps
                ///	4) store frame into DPB if ...
                //
                //if(pInfo->SliceHeader.redundant_pic_cnt)
                {
                    h264_dpb_gaps_in_frame_num_mem_management(pInfo);
                }

#ifdef DUMP_HEADER_INFO
                dump_new_picture_attr(pInfo, pInfo->SliceHeader.frame_num);
#endif
            }
            //
            /// Decoding POC
            h264_hdr_decoding_poc (pInfo, 0, 0);

            //
            /// Init Frame Store for next frame
            h264_dpb_init_frame_store (pInfo);
            pInfo->img.current_slice_num = 1;

            if (pInfo->SliceHeader.first_mb_in_slice != 0)
            {
                ////Come here means we have slice lost at the beginning, since no FMO support
                pInfo->SliceHeader.sh_error |= (pInfo->SliceHeader.structure << 17);
            }

            //
            /// Emit out the New Frame
            if (pInfo->img.g_new_frame)
            {
                h264_parse_emit_start_new_frame(parent, pInfo);
            }

            h264_parse_emit_current_pic(parent, pInfo);
        }
        else ///////////////////////////////////////////////////// If Not a picture start
        {
            //
            /// Update slice structures: cur->old; next->cur;
            h264_update_old_slice(pInfo, next_SliceHeader);

            //
            /// 1) if resolution change: reset dpb
            /// 2) else: update img info
            h264_update_img_info(pInfo);
        }


        //////////////////////////////////////////////////////////////
        // Step 4: DPB reference list init and reordering
        //////////////////////////////////////////////////////////////

        //////////////////////////////////////////////// Update frame Type--- IDR/I/P/B for frame or field
        h264_update_frame_type(pInfo);


        h264_dpb_update_ref_lists( pInfo);

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if ((pInfo->dpb.ltref_frames_in_buffer + pInfo->dpb.ref_frames_in_buffer ) > pInfo->active_SPS.num_ref_frames)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
#ifdef DUMP_HEADER_INFO
        dump_ref_list(pInfo);
#endif
        /// Emit out the current "good" slice
        h264_parse_emit_current_slice(parent, pInfo);

    }
    break;

    ///// * Main profile doesn't support Data Partition, skipped.... *////
    case h264_NAL_UNIT_TYPE_DPA:
    case h264_NAL_UNIT_TYPE_DPB:
    case h264_NAL_UNIT_TYPE_DPC:
        //OS_INFO("***********************DP feature, not supported currently*******************\n");
        pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
        status = H264_STATUS_NOTSUPPORT;
        break;

        //// * Parsing SEI info *////
    case h264_NAL_UNIT_TYPE_SEI:
        status = H264_STATUS_OK;

        //OS_INFO("*****************************SEI**************************************\n");
        if (pInfo->sps_valid) {
            //h264_user_data_t user_data; /// Replace with tmp buffer while porting to FW
            pInfo->number_of_first_au_info_nal_before_first_slice++;
            /// parsing the SEI info
            status = h264_Parse_Supplemental_Enhancement_Information_Message(parent, pInfo);
        }

        //h264_rbsp_trailing_bits(pInfo);
        break;
    case h264_NAL_UNIT_TYPE_SPS:
    {
        //OS_INFO("*****************************SPS**************************************\n");
        ///
        /// Can not define local SPS since the Current local stack size limitation!
        /// Could be changed after the limitation gone
        ///
        uint8_t  old_sps_id=0;
        vui_seq_parameters_t_not_used vui_seq_not_used;

        old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));


        status = h264_Parse_SeqParameterSet(parent, pInfo, &(pInfo->active_SPS), &vui_seq_not_used, (int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL);
        if (status == H264_STATUS_OK) {
            h264_Parse_Copy_Sps_To_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_SPS.seq_parameter_set_id);
            pInfo->sps_valid = 1;

            if (1==pInfo->active_SPS.pic_order_cnt_type) {
                h264_Parse_Copy_Offset_Ref_Frames_To_DDR(pInfo,(int32_t *)pInfo->TMP_OFFSET_REFFRM_PADDR_GL,pInfo->active_SPS.seq_parameter_set_id);
            }

#ifdef DUMP_HEADER_INFO
            dump_sps(&(pInfo->active_SPS));
#endif

        }
        ///// Restore the active SPS if new arrival's id changed
        if (old_sps_id>=MAX_NUM_SPS) {
            h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used));
            pInfo->active_SPS.seq_parameter_set_id = 0xff;
        }
        else {
            if (old_sps_id!=pInfo->active_SPS.seq_parameter_set_id)  {
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            }
            else  {
                //h264_memset(&(pInfo->active_SPS), 0x0, sizeof(seq_param_set));
                pInfo->active_SPS.seq_parameter_set_id = 0xff;
            }
        }

        pInfo->number_of_first_au_info_nal_before_first_slice++;
    }
    break;
    case h264_NAL_UNIT_TYPE_PPS:
    {
        //OS_INFO("*****************************PPS**************************************\n");

        uint32_t old_sps_id = pInfo->active_SPS.seq_parameter_set_id;
        uint32_t old_pps_id = pInfo->active_PPS.pic_parameter_set_id;

        h264_memset(&pInfo->active_PPS, 0x0, sizeof(pic_param_set));
        pInfo->number_of_first_au_info_nal_before_first_slice++;

        if (h264_Parse_PicParameterSet(parent, pInfo, &pInfo->active_PPS)== H264_STATUS_OK)
        {
            h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), pInfo->active_PPS.seq_parameter_set_id);
            if (old_sps_id != pInfo->active_SPS.seq_parameter_set_id)
            {
                pInfo->Is_SPS_updated = 1;
            }
            if (pInfo->active_SPS.seq_parameter_set_id != 0xff) {
                h264_Parse_Copy_Pps_To_DDR(pInfo, &pInfo->active_PPS, pInfo->active_PPS.pic_parameter_set_id);
                pInfo->got_start = 1;
                if (pInfo->sei_information.recovery_point)
                {
                    pInfo->img.recovery_point_found |= 2;

                    //// Enable the RP recovery if no IDR ---Cisco
                    if ((pInfo->img.recovery_point_found & 1)==0)
                        pInfo->sei_rp_received = 1;
                }
            }
            else
            {
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            }
#ifdef DUMP_HEADER_INFO
            dump_pps(&(pInfo->active_PPS));
#endif
        } else {
            if (old_sps_id<MAX_NUM_SPS)
                h264_Parse_Copy_Sps_From_DDR(pInfo, &(pInfo->active_SPS), old_sps_id);
            if (old_pps_id<MAX_NUM_PPS)
                h264_Parse_Copy_Pps_From_DDR(pInfo, &(pInfo->active_PPS), old_pps_id);
        }

    } //// End of PPS parsing
    break;


    case h264_NAL_UNIT_TYPE_EOSeq:
    case h264_NAL_UNIT_TYPE_EOstream:

        h264_parse_emit_eos(parent, pInfo);
        h264_init_dpb(&(pInfo->dpb));

        pInfo->is_current_workload_done=1;

        /* picture level info which will always be initialized */
        //h264_init_Info_under_sps_pps_level(pInfo);

        ////reset the pInfo here
        //viddec_h264_init(ctxt, (uint32_t *)parser->sps_pps_ddr_paddr, false);


        status = H264_STATUS_OK;
        pInfo->number_of_first_au_info_nal_before_first_slice++;
        break;

    case h264_NAL_UNIT_TYPE_Acc_unit_delimiter:
#if 1
        ///// primary_pic_type
        {
            uint32_t code = 0xff;
            int32_t ret = 0;
            ret = viddec_pm_get_bits(parent, (uint32_t *)&(code), 3);

            if (ret != -1) {
                //if(pInfo->got_start && (code == 0))
                //{
                //pInfo->img.recovery_point_found |= 4;
                //}
                pInfo->primary_pic_type_plus_one = (uint8_t)(code)+1;
                status = H264_STATUS_OK;
            }
            pInfo->number_of_first_au_info_nal_before_first_slice++;
            break;
        }
#endif

    case h264_NAL_UNIT_TYPE_Reserved1:
    case h264_NAL_UNIT_TYPE_Reserved2:
    case h264_NAL_UNIT_TYPE_Reserved3:
    case h264_NAL_UNIT_TYPE_Reserved4:
    case h264_NAL_UNIT_TYPE_Reserved5:
        status = H264_STATUS_OK;
        pInfo->number_of_first_au_info_nal_before_first_slice++;
        break;

    case h264_NAL_UNIT_TYPE_filler_data:
        status = H264_STATUS_OK;
        break;
    case h264_NAL_UNIT_TYPE_ACP:
        break;
    case h264_NAL_UNIT_TYPE_SPS_extension:
    case h264_NAL_UNIT_TYPE_unspecified:
    case h264_NAL_UNIT_TYPE_unspecified2:
        status = H264_STATUS_OK;
        //nothing
        break;
    default:
        status = H264_STATUS_OK;
        break;
    }

    //pInfo->old_nal_unit_type = pInfo->nal_unit_type;
    switch ( pInfo->nal_unit_type )
    {
    case h264_NAL_UNIT_TYPE_IDR:
    case h264_NAL_UNIT_TYPE_SLICE:
    case h264_NAL_UNIT_TYPE_Acc_unit_delimiter:
    case h264_NAL_UNIT_TYPE_SPS:
    case h264_NAL_UNIT_TYPE_PPS:
    case h264_NAL_UNIT_TYPE_SEI:
    case h264_NAL_UNIT_TYPE_EOSeq:
    case h264_NAL_UNIT_TYPE_EOstream:
    case h264_NAL_UNIT_TYPE_Reserved1:
    case h264_NAL_UNIT_TYPE_Reserved2:
    case h264_NAL_UNIT_TYPE_Reserved3:
    case h264_NAL_UNIT_TYPE_Reserved4:
    case h264_NAL_UNIT_TYPE_Reserved5:
    {
        pInfo->old_nal_unit_type = pInfo->nal_unit_type;
        break;
    }
    default:
        break;
    }

    return status;
}




/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
#ifndef VBP
static uint32_t viddec_h264_is_frame_start(void *ctxt)
{
    struct h264_viddec_parser* parser = ctxt;
    uint32_t ret = 0;

    h264_Info * pInfo = &(parser->info);

    if (pInfo->img.g_new_frame) {
        ret = 1;
    }

    return ret;
}
#endif

#ifndef VBP
uint32_t viddec_h264_wkld_done(void *parent, void *ctxt, unsigned int next_sc,
                               uint32_t *codec_specific_errors)
{
    struct h264_viddec_parser* parser = ctxt;
    uint32_t ret = VIDDEC_PARSE_SUCESS;
    h264_Info * pInfo = &(parser->info);
    uint8_t is_stream_forced_to_complete=false;

    is_stream_forced_to_complete = (VIDDEC_PARSE_EOS == next_sc) || (VIDDEC_PARSE_DISCONTINUITY == next_sc);

    if (is_stream_forced_to_complete || (pInfo->is_current_workload_done))
    {
        viddec_workload_t 		 *wl;
        viddec_frame_attributes_t *attrs;

        wl = viddec_pm_get_header( parent );
        attrs = &wl->attrs;

        if ((attrs->cont_size.width < 32) || (attrs->cont_size.width > 2048) || (attrs->cont_size.height < 32) || (attrs->cont_size.height>2048))
        {
            attrs->cont_size.width = 32;
            attrs->cont_size.height = 32;
            pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
            pInfo->wl_err_curr |= (FRAME << FIELD_ERR_OFFSET);
        }

        *codec_specific_errors = pInfo->wl_err_curr;
        pInfo->wl_err_curr = pInfo->wl_err_next;
        pInfo->wl_err_next = 0;

        if (is_stream_forced_to_complete)
        {
            h264_parse_emit_eos(parent, pInfo);
        }
        ret = VIDDEC_PARSE_FRMDONE;
    }

    return ret;
}
#endif

#ifdef VBP
void viddec_h264_get_context_size(viddec_parser_memory_sizes_t *size)
#else
static void viddec_h264_get_context_size(viddec_parser_memory_sizes_t *size)
#endif
{
    /* Should return size of my structure */
    size->context_size = sizeof(struct h264_viddec_parser);
    size->persist_size = MAX_NUM_SPS * sizeof(seq_param_set_all)
                         + MAX_NUM_PPS * sizeof(pic_param_set)
                         + MAX_NUM_SPS * sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE
                         + sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
#ifdef VBP
void viddec_h264_flush(void *parent, void *ctxt)
#else
static void viddec_h264_flush(void *parent, void *ctxt)
#endif
{
    int i;
    struct h264_viddec_parser* parser = ctxt;
    h264_Info * pInfo = &(parser->info);

    /* just flush dpb and disable output */
    h264_dpb_flush_dpb(pInfo, 0, pInfo->img.second_field, pInfo->active_SPS.num_ref_frames);

    /* reset the dpb to the initial state, avoid parser store
       wrong data to dpb in next slice parsing */
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    for (i = 0; i < NUM_DPB_FRAME_STORES; i++)
    {
        p_dpb->fs[i].fs_idc = MPD_DPB_FS_NULL_IDC;
        p_dpb->fs_dpb_idc[i] = MPD_DPB_FS_NULL_IDC;
    }
    p_dpb->used_size = 0;
    p_dpb->fs_dec_idc = MPD_DPB_FS_NULL_IDC;
    p_dpb->fs_non_exist_idc = MPD_DPB_FS_NULL_IDC;

    return;
}

#ifndef VBP
void viddec_h264_get_ops(viddec_parser_ops_t *ops)
{
    ops->init = viddec_h264_init;

    ops->parse_syntax = viddec_h264_parse;
    ops->get_cxt_size = viddec_h264_get_context_size;
    ops->is_wkld_done = viddec_h264_wkld_done;
    ops->is_frame_start = viddec_h264_is_frame_start;
    ops->flush = viddec_h264_flush;
    return;
}
#endif

