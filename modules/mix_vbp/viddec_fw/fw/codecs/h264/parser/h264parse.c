/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    h264 parser
//
///////////////////////////////////////////////////////////////////////*/


#include "h264.h"
#include "h264parse.h"
#include "h264parse_dpb.h"


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */



h264_Status h264_Scaling_List(void *parent, uint8_t *scalingList, int32_t sizeOfScalingList, uint8_t *UseDefaultScalingMatrix, h264_Info* pInfo)
{
    int32_t j, scanj;
    int32_t delta_scale, lastScale, nextScale;

    const uint8_t ZZ_SCAN[16]  =
        {  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
        };

    const uint8_t ZZ_SCAN8[64] =
        {  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
           12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
           35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
           58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
        };

    lastScale      = 8;
    nextScale      = 8;
    scanj = 0;

    for (j=0; j<sizeOfScalingList; j++)
    {
        scanj = (sizeOfScalingList==16)?ZZ_SCAN[j]:ZZ_SCAN8[j];

        if (nextScale!=0)
        {
            delta_scale = h264_GetVLCElement(parent, pInfo, true);
            nextScale = (lastScale + delta_scale + 256) % 256;
            *UseDefaultScalingMatrix = (uint8_t) (scanj==0 && nextScale==0);
        }

        scalingList[scanj] = (nextScale==0) ? lastScale:nextScale;
        lastScale = scalingList[scanj];
    }

    return H264_STATUS_OK;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

h264_Status h264_active_par_set(h264_Info*pInfo,h264_Slice_Header_t* SliceHeader)
{
    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;

    ///////////////////////////////////////////////////
    // Reload SPS/PPS while
    // 1) Start of Frame (in case of context switch)
    // 2) PPS id changed
    ///////////////////////////////////////////////////
    if ((SliceHeader->first_mb_in_slice == 0) || (SliceHeader->pic_parameter_id != pInfo->active_PPS.pic_parameter_set_id))
    {
#ifndef WIN32
        h264_Parse_Copy_Pps_From_DDR(pInfo, &pInfo->active_PPS, SliceHeader->pic_parameter_id);

        if (pInfo->active_PPS.seq_parameter_set_id >= MAX_NUM_SPS)
        {
            return H264_PPS_INVALID_PIC_ID;			/// Invalid PPS detected
        }

        if (pInfo->active_PPS.seq_parameter_set_id != pInfo->active_SPS.seq_parameter_set_id)
        {
            pInfo->Is_SPS_updated =1;
            h264_Parse_Copy_Sps_From_DDR(pInfo, &pInfo->active_SPS, pInfo->active_PPS.seq_parameter_set_id);
            h264_Parse_Clear_Sps_Updated_Flag(pInfo, pInfo->active_PPS.seq_parameter_set_id);
        }
        else
        {
            if (h264_Parse_Check_Sps_Updated_Flag(pInfo, pInfo->active_PPS.seq_parameter_set_id))
            {
                pInfo->Is_SPS_updated =1;
                h264_Parse_Copy_Sps_From_DDR(pInfo, &pInfo->active_SPS, pInfo->active_PPS.seq_parameter_set_id);
                h264_Parse_Clear_Sps_Updated_Flag(pInfo, pInfo->active_PPS.seq_parameter_set_id);
            }
        }

#else
        pInfo->active_PPS = PPS_GL[SliceHeader->pic_parameter_id];
        pInfo->active_SPS = SPS_GL[pInfo->active_PPS.seq_parameter_set_id];
#endif

        if (pInfo->active_SPS.seq_parameter_set_id >= MAX_NUM_SPS)
        {
            return H264_PPS_INVALID_PIC_ID;			//// Invalid SPS detected
        }
    }
    else {
        if ((pInfo->active_PPS.seq_parameter_set_id >= MAX_NUM_SPS)  || (pInfo->active_SPS.seq_parameter_set_id >= MAX_NUM_SPS))
        {
            return H264_PPS_INVALID_PIC_ID;			/// Invalid PPS detected
        }
    }


    pInfo->img.PicWidthInMbs    = (pInfo->active_SPS.sps_disp.pic_width_in_mbs_minus1 + 1);
    //pInfo->img.PicHeightInMapUnits = (pInfo->active_SPS.sps_disp.pic_height_in_map_units_minus1 + 1);
    pInfo->img.FrameHeightInMbs = pInfo->active_SPS.sps_disp.frame_mbs_only_flag?				\
                                  (pInfo->active_SPS.sps_disp.pic_height_in_map_units_minus1 + 1):	\
                                  ((pInfo->active_SPS.sps_disp.pic_height_in_map_units_minus1 + 1)<<1);


    return H264_STATUS_OK;
};   //// End of h264_active_par_set

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////
// Parse slice header info
//////////////////////////////////////////////////
h264_Status h264_Parse_Slice_Layer_Without_Partitioning_RBSP(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader)
{
    h264_Status retStatus = H264_STATUS_ERROR;

    ////////////////////////////////////////////////////
    //// Parse slice header info
    //// Part1: not depend on the active PPS/SPS
    //// Part2/3: depend on the active parset
    //////////////////////////////////////////////////

    //retStatus = h264_Parse_Slice_Header_1(pInfo);

    SliceHeader->sh_error = 0;

    if (h264_Parse_Slice_Header_1(parent, pInfo, SliceHeader) == H264_STATUS_OK)
    {
        //////////////////////////////////////////
        //// Active parameter set for this slice
        //////////////////////////////////////////
        retStatus = h264_active_par_set(pInfo, SliceHeader);
    }

    if (retStatus == H264_STATUS_OK) {
        switch (pInfo->active_SPS.profile_idc)
        {
        case h264_ProfileBaseline:
        case h264_ProfileMain:
        case h264_ProfileExtended:
            pInfo->active_PPS.transform_8x8_mode_flag=0;
            pInfo->active_PPS.pic_scaling_matrix_present_flag =0;
            pInfo->active_PPS.second_chroma_qp_index_offset = pInfo->active_PPS.chroma_qp_index_offset;

        default:
            break;
        }

        if ( h264_Parse_Slice_Header_2(parent, pInfo, SliceHeader) != H264_STATUS_OK)
        {
            SliceHeader->sh_error |= 2;
        }
        else	if ( h264_Parse_Slice_Header_3(parent, pInfo, SliceHeader) != H264_STATUS_OK)
        {
            SliceHeader->sh_error |= 4;
        }

    } else 	{
        SliceHeader->sh_error |= 1;
    }


    //if(SliceHeader->sh_error) {
    //pInfo->wl_err_flag |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
    //}



    //////////////////////////////////
    //// Parse slice data (MB loop)
    //////////////////////////////////
    //retStatus = h264_Parse_Slice_Data(pInfo);
    {
        //uint32_t data = 0;
        //if( viddec_pm_peek_bits(parent, &data, 32) == -1)
        //retStatus = H264_STATUS_ERROR;
    }
    //h264_Parse_rbsp_trailing_bits(pInfo);

    return retStatus;
}



/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

h264_Status h264_Parse_NAL_Unit(void *parent, h264_Info* pInfo, uint8_t *nal_ref_idc)
{
    h264_Status ret = H264_STATUS_ERROR;

    //h264_NAL_Unit_t* NAL = &pInfo->NAL;
    uint32_t code;
#if 0
    viddec_pm_get_bits(void * parent,uint32_t * data,uint32_t num_bits)(parent, &code, 24);
    viddec_pm_get_bits(parent, &code, 1);   //forbidden_zero_bit

    viddec_pm_get_bits(parent, &code, 2);
    SliceHeader->nal_ref_idc = (uint8_t)code;

    viddec_pm_get_bits(parent, &code, 5);
    pInfo->nal_unit_type = (uint8_t)code;
#else
#ifdef VBP
    if ( viddec_pm_get_bits(parent, &code, 8) != -1)
#else
    //// 24bit SC, 1 bit: forbidden_zero_bit, 2 bitrs: nal_ref_idc, 5 bits: nal_unit_type
    if ( viddec_pm_get_bits(parent, &code, 32) != -1)
#endif
    {
        *nal_ref_idc = (uint8_t)((code>>5)&0x3);
        pInfo->nal_unit_type = (uint8_t)((code>>0)&0x1f);
        ret = H264_STATUS_OK;
    }
#endif

    return ret;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

/*!
 ************************************************************************
 * \brief
 *    set defaults for old_slice
 *    NAL unit of a picture"
 ************************************************************************
 */
#ifndef INT_MAX
#define INT_MAX 0xFFFFFFFF
#endif

#ifndef UINT_MAX
#define UINT_MAX 0x7FFFFFFF
#endif

void h264_init_old_slice(h264_Info* pInfo)
{
    pInfo->SliceHeader.field_pic_flag = 0;

    pInfo->SliceHeader.pic_parameter_id = 0xFF;

    pInfo->SliceHeader.frame_num = INT_MAX;

    pInfo->SliceHeader.nal_ref_idc = 0xFF;

    pInfo->SliceHeader.idr_flag = 0;

    pInfo->SliceHeader.pic_order_cnt_lsb          = UINT_MAX;
    pInfo->SliceHeader.delta_pic_order_cnt_bottom = INT_MAX;

    pInfo->SliceHeader.delta_pic_order_cnt[0] = INT_MAX;
    pInfo->SliceHeader.delta_pic_order_cnt[1] = INT_MAX;

    return;
}


void h264_init_img(h264_Info* pInfo)
{
    h264_memset(&(pInfo->img), 0x0, sizeof(h264_img_par) );


    return;
}


void h264_init_sps_pps(struct h264_viddec_parser* parser, uint32_t *persist_mem)
{
    int32_t i;

    h264_Info * pInfo = &(parser->info);

    parser->sps_pps_ddr_paddr = (uint32_t)persist_mem;

    pInfo->SPS_PADDR_GL = parser->sps_pps_ddr_paddr;
    pInfo->PPS_PADDR_GL = pInfo->SPS_PADDR_GL + MAX_NUM_SPS * sizeof(seq_param_set_all);
    pInfo->OFFSET_REF_FRAME_PADDR_GL = pInfo->PPS_PADDR_GL + MAX_NUM_PPS * sizeof(pic_param_set);
    pInfo->TMP_OFFSET_REFFRM_PADDR_GL = pInfo->OFFSET_REF_FRAME_PADDR_GL +
                                        MAX_NUM_SPS * sizeof(int32_t) * MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;

    h264_memset( &(pInfo->active_SPS), 0x0, sizeof(seq_param_set_used) );
    h264_memset( &(pInfo->active_PPS), 0x0, sizeof(pic_param_set) );

    /* Global for SPS   & PPS */
    for (i=0; i<MAX_NUM_SPS; i++)
    {
        pInfo->active_SPS.seq_parameter_set_id = 0xff;
        h264_Parse_Copy_Sps_To_DDR (pInfo, &(pInfo->active_SPS), i);
    }
    for (i=0; i<MAX_NUM_PPS; i++)
    {
        pInfo->active_PPS.seq_parameter_set_id = 0xff;
        h264_Parse_Copy_Pps_To_DDR (pInfo, &(pInfo->active_PPS), i);
    }

    pInfo->active_SPS.seq_parameter_set_id = 0xff;
    pInfo->sps_valid = 0;
    pInfo->got_start = 0;

    return;
}


void h264_init_Info_under_sps_pps_level(h264_Info* pInfo)
{
    int32_t i=0;

    h264_memset( &(pInfo->dpb), 0x0, sizeof(h264_DecodedPictureBuffer) );
    h264_memset( &(pInfo->SliceHeader), 0x0, sizeof(h264_Slice_Header_t) );
    h264_memset( &(pInfo->old_slice), 0x0, sizeof(OldSliceParams) );
    h264_memset( &(pInfo->sei_information), 0x0, sizeof(sei_info) );
    h264_memset( &(pInfo->img), 0x0, sizeof(h264_img_par) );

    pInfo->h264_list_replacement = 0;

    pInfo->h264_pwt_start_byte_offset = 0;
    pInfo->h264_pwt_start_bit_offset = 0;
    pInfo->h264_pwt_end_byte_offset = 0;
    pInfo->h264_pwt_end_bit_offset = 0;
    pInfo->h264_pwt_enabled = 0;

    for (i=0; i<32; i++)
    {
        pInfo->slice_ref_list0[i] = 0;
        pInfo->slice_ref_list1[i] = 0;
    }

    pInfo->qm_present_list = 0;

    pInfo->nal_unit_type = 0;
    pInfo->old_nal_unit_type = 0xff;

    pInfo->push_to_cur = 0;
    pInfo->Is_first_frame_in_stream = 1;
    pInfo->Is_SPS_updated = 0;
    pInfo->number_of_first_au_info_nal_before_first_slice = 0;

    pInfo->is_frame_boundary_detected_by_non_slice_nal = 0;
    pInfo->is_frame_boundary_detected_by_slice_nal = 0;
    pInfo->is_current_workload_done = 0;

    pInfo->sei_rp_received = 0;
    pInfo->last_I_frame_idc = 255;
    pInfo->wl_err_curr = 0;
    pInfo->wl_err_next = 0;

    pInfo->primary_pic_type_plus_one = 0;
    pInfo->sei_b_state_ready = 0;

    /* Init old slice structure  */
    h264_init_old_slice(pInfo);

    /* init_dpb */
    h264_init_dpb(&(pInfo->dpb));

    /* init_sei	*/
    h264_sei_stream_initialise(pInfo);

}

void h264_init_Info(h264_Info* pInfo)
{
    h264_memset(pInfo, 0x0, sizeof(h264_Info));

    pInfo->old_nal_unit_type = 0xff;

    pInfo->Is_first_frame_in_stream =1;
    pInfo->img.frame_count = 0;
    pInfo->last_I_frame_idc = 255;

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

/////////////////////////////////////////////////////
//
// Judge whether it is the first VCL of a new picture
//
/////////////////////////////////////////////////////
int32_t h264_is_second_field(h264_Info * pInfo)
{
    h264_Slice_Header_t cur_slice = pInfo->SliceHeader;
    OldSliceParams old_slice = pInfo->old_slice;

    int result = 0;

    //pInfo->img.second_field = 0;

    /// is it second field?

    //OS_INFO( "xxx is_used = %d\n", pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].is_used);

    if (cur_slice.structure != FRAME)
    {
        if ( ( MPD_DPB_FS_NULL_IDC != pInfo->dpb.fs_dec_idc)&&(3 != viddec_h264_get_is_used(&(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc])) )
                &&(0 != viddec_h264_get_is_used(&(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc])) ))
        {
            if ((cur_slice.frame_num == old_slice.frame_num)||(cur_slice.idr_flag))
            {

                if (old_slice.structure != cur_slice.structure)
                {

                    if (((cur_slice.structure == TOP_FIELD &&old_slice.structure == BOTTOM_FIELD) || // Condition 1:
                            (old_slice.structure == TOP_FIELD && cur_slice.structure == BOTTOM_FIELD)) &&    \
                            ((old_slice.nal_ref_idc ==0 && cur_slice.nal_ref_idc == 0)              || // Condition 2:
                             (old_slice.nal_ref_idc !=0 &&cur_slice.nal_ref_idc != 0)))
                    {
                        //pInfo->img.second_field = 1;
                        result = 1;
                    }
                }
            }


        }


    }



    return result;

} //// End of h264_is_second_field



/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

int32_t h264_is_new_picture_start(h264_Info * pInfo, h264_Slice_Header_t cur_slice, h264_Slice_Header_t old_slice)
{
    int result = 0;

    if (pInfo->number_of_first_au_info_nal_before_first_slice)
    {
        pInfo->number_of_first_au_info_nal_before_first_slice = 0;
        return 1;
    }



    result |= (old_slice.pic_parameter_id != cur_slice.pic_parameter_id);
    result |= (old_slice.frame_num != cur_slice.frame_num);
    result |= (old_slice.field_pic_flag != cur_slice.field_pic_flag);
    if (cur_slice.field_pic_flag && old_slice.field_pic_flag)
    {
        result |= (old_slice.bottom_field_flag != cur_slice.bottom_field_flag);
    }

    result |= (old_slice.nal_ref_idc != cur_slice.nal_ref_idc) && \
              ((old_slice.nal_ref_idc == 0) || (cur_slice.nal_ref_idc == 0));
    result |= ( old_slice.idr_flag != cur_slice.idr_flag);

    if (cur_slice.idr_flag && old_slice.idr_flag)
    {
        result |= (old_slice.idr_pic_id != cur_slice.idr_pic_id);
    }

    if (pInfo->active_SPS.pic_order_cnt_type == 0)
    {
        result |=  (old_slice.pic_order_cnt_lsb          != cur_slice.pic_order_cnt_lsb);
        result |=  (old_slice.delta_pic_order_cnt_bottom != cur_slice.delta_pic_order_cnt_bottom);
    }

    if (pInfo->active_SPS.pic_order_cnt_type == 1)
    {
        result |= (old_slice.delta_pic_order_cnt[0] != cur_slice.delta_pic_order_cnt[0]);
        result |= (old_slice.delta_pic_order_cnt[1] != cur_slice.delta_pic_order_cnt[1]);
    }

    return result;
}


int32_t h264_check_previous_frame_end(h264_Info * pInfo)
{
    int result = 0;

    if ( (h264_NAL_UNIT_TYPE_SLICE==pInfo->old_nal_unit_type)||(h264_NAL_UNIT_TYPE_IDR==pInfo->old_nal_unit_type) )
    {

        switch ( pInfo->nal_unit_type )
        {
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
            pInfo->img.current_slice_num = 0;

            if ((pInfo->img.structure == FRAME) || (pInfo->img.second_field)) {
                pInfo->is_frame_boundary_detected_by_non_slice_nal =1;
                pInfo->is_current_workload_done=1;
                result=1;
            }
            break;
        }
        default:
            break;
        }

    }

    return result;

}




/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////
// 1) Update old slice structure for frame boundary detection
//////////////////////////////////////////////////////////////
void h264_update_old_slice(h264_Info * pInfo,h264_Slice_Header_t next_SliceHeader)
{
    pInfo->old_slice.pic_parameter_id = pInfo->SliceHeader.pic_parameter_id;

    pInfo->old_slice.frame_num = pInfo->SliceHeader.frame_num;

    pInfo->old_slice.field_pic_flag = pInfo->SliceHeader.field_pic_flag;

    if (pInfo->SliceHeader.field_pic_flag)
    {
        pInfo->old_slice.bottom_field_flag = pInfo->SliceHeader.bottom_field_flag;
    }

    pInfo->old_slice.nal_ref_idc   = pInfo->SliceHeader.nal_ref_idc;

    pInfo->old_slice.structure = pInfo->SliceHeader.structure;

    pInfo->old_slice.idr_flag = pInfo->SliceHeader.idr_flag;
    if (pInfo->SliceHeader.idr_flag)
    {
        pInfo->old_slice.idr_pic_id = pInfo->SliceHeader.idr_pic_id;
    }

    if (pInfo->active_SPS.pic_order_cnt_type == 0)
    {
        pInfo->old_slice.pic_order_cnt_lsb          = pInfo->SliceHeader.pic_order_cnt_lsb;
        pInfo->old_slice.delta_pic_order_cnt_bottom = pInfo->SliceHeader.delta_pic_order_cnt_bottom;
    }

    if (pInfo->active_SPS.pic_order_cnt_type == 1)
    {
        pInfo->old_slice.delta_pic_order_cnt[0] = pInfo->SliceHeader.delta_pic_order_cnt[0];
        pInfo->old_slice.delta_pic_order_cnt[1] = pInfo->SliceHeader.delta_pic_order_cnt[1];
    }

    ////////////////////////////// Next to current
    memcpy(&pInfo->SliceHeader, &next_SliceHeader, sizeof(h264_Slice_Header_t));

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// Initialization for new picture
//////////////////////////////////////////////////////////////////////////////
void h264_update_img_info(h264_Info * pInfo )
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    pInfo->img.frame_num = pInfo->SliceHeader.frame_num;
    pInfo->img.structure = pInfo->SliceHeader.structure;

    pInfo->img.field_pic_flag = pInfo->SliceHeader.field_pic_flag;
    pInfo->img.bottom_field_flag = pInfo->SliceHeader.bottom_field_flag;

    pInfo->img.MbaffFrameFlag  = pInfo->active_SPS.sps_disp.mb_adaptive_frame_field_flag & (!(pInfo->SliceHeader.field_pic_flag));
    pInfo->img.pic_order_cnt_type = pInfo->active_SPS.pic_order_cnt_type;

    if (pInfo->img.pic_order_cnt_type == 1) {
        pInfo->img.num_ref_frames_in_pic_order_cnt_cycle = pInfo->active_SPS.num_ref_frames_in_pic_order_cnt_cycle;
        pInfo->img.delta_pic_order_always_zero_flag = pInfo->active_SPS.delta_pic_order_always_zero_flag;
        pInfo->img.offset_for_non_ref_pic = pInfo->active_SPS.offset_for_non_ref_pic;
        pInfo->img.offset_for_top_to_bottom_field = pInfo->active_SPS.offset_for_top_to_bottom_field;
    }

    pInfo->img.pic_order_cnt_lsb = pInfo->SliceHeader.pic_order_cnt_lsb;
    //pInfo->img.pic_order_cnt_msb = pInfo->SliceHeader.pic_order_cnt_msb;
    pInfo->img.delta_pic_order_cnt_bottom = pInfo->SliceHeader.delta_pic_order_cnt_bottom;
    pInfo->img.delta_pic_order_cnt[0] = pInfo->SliceHeader.delta_pic_order_cnt[0];
    pInfo->img.delta_pic_order_cnt[1] = pInfo->SliceHeader.delta_pic_order_cnt[1];


    pInfo->img.PreviousFrameNum = pInfo->old_slice.frame_num;

    pInfo->img.no_output_of_prior_pics_flag = pInfo->SliceHeader.sh_dec_refpic.no_output_of_prior_pics_flag;

    ////////////////////////////////////////////////// Check SEI recovery point
    if (pInfo->sei_information.recovery_point) {
        int32_t MaxFrameNum = 1 << (pInfo->active_SPS.log2_max_frame_num_minus4 + 4);
        pInfo->sei_information.recovery_frame_num = (pInfo->img.frame_num + pInfo->sei_information.recovery_frame_cnt) % MaxFrameNum;
    }

    if (pInfo->SliceHeader.idr_flag)
        pInfo->sei_information.recovery_frame_num = pInfo->img.frame_num;



    /////////////////////////////////////////////////Resolution Change
    pInfo->img.curr_has_mmco_5 = 0;

    if ( (pInfo->img.PicWidthInMbs != p_dpb->PicWidthInMbs)||
            (pInfo->img.FrameHeightInMbs != p_dpb->FrameHeightInMbs) )
    {
        int32_t no_output_old_pics = (pInfo->SliceHeader.idr_flag)? pInfo->img.no_output_of_prior_pics_flag : 0;

        // If resolution changed, reset the soft DPB here
        h264_dpb_reset_dpb(pInfo, pInfo->img.PicWidthInMbs, pInfo->img.FrameHeightInMbs, 1, no_output_old_pics);
    }

    return;

} ///// End of init new frame


void h264_update_frame_type(h264_Info * pInfo )
{

//update frame type
    if (pInfo->img.structure == FRAME)
    {
        if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
        {
            if (pInfo->dpb.fs_dec_idc < NUM_DPB_FRAME_STORES)
            {
                pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (0x1 << FRAME_TYPE_STRUCTRUE_OFFSET)|(FRAME_TYPE_IDR << FRAME_TYPE_FRAME_OFFSET);
            //pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = 0xff;
            //pInfo->dpb.fs[0].pic_type = pInfo->dpb.fs_dec_idc;
            }

        }
        else
        {
#if 1
            switch (pInfo->SliceHeader.slice_type)
            {
            case h264_PtypeB:
                pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (0x1 << FRAME_TYPE_STRUCTRUE_OFFSET)|(FRAME_TYPE_B << FRAME_TYPE_FRAME_OFFSET);
                break;
            case h264_PtypeSP:
            case h264_PtypeP:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_FRAME_OFFSET))>>FRAME_TYPE_FRAME_OFFSET != FRAME_TYPE_B)
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (0x1 << FRAME_TYPE_STRUCTRUE_OFFSET)|(FRAME_TYPE_P << FRAME_TYPE_FRAME_OFFSET);
                break;
            case h264_PtypeI:
            case h264_PtypeSI:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_FRAME_OFFSET))>>FRAME_TYPE_FRAME_OFFSET == FRAME_TYPE_INVALID)
                {
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (0x1 << FRAME_TYPE_STRUCTRUE_OFFSET)|(FRAME_TYPE_I << FRAME_TYPE_FRAME_OFFSET);
                }
                pInfo->last_I_frame_idc = pInfo->dpb.fs_dec_idc;

                break;
            default:
                break;

            }
#endif

        }

    }
    else if (pInfo->img.structure == TOP_FIELD)
    {
        if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
        {
            pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_IDR << FRAME_TYPE_TOP_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_BOTTOM_OFFSET));;
        }
        else
        {
            switch (pInfo->SliceHeader.slice_type)
            {
            case h264_PtypeB:
                pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_B << FRAME_TYPE_TOP_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_BOTTOM_OFFSET));
                break;
            case h264_PtypeSP:
            case h264_PtypeP:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_TOP_OFFSET))>>FRAME_TYPE_TOP_OFFSET != FRAME_TYPE_B)
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_P << FRAME_TYPE_TOP_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_BOTTOM_OFFSET));
                break;
            case h264_PtypeI:
            case h264_PtypeSI:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_TOP_OFFSET))>>FRAME_TYPE_TOP_OFFSET == FRAME_TYPE_INVALID)
                {
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_I << FRAME_TYPE_TOP_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_BOTTOM_OFFSET));
                }
                if (pInfo->sei_rp_received)
                    pInfo->last_I_frame_idc = pInfo->dpb.fs_dec_idc;
                else
                    pInfo->last_I_frame_idc = 255;
                break;
            default:
                break;

            }

        }


    } else if (pInfo->img.structure == BOTTOM_FIELD)
    {
        if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
        {
            pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_IDR << FRAME_TYPE_BOTTOM_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_TOP_OFFSET));;
        }
        else
        {
            switch (pInfo->SliceHeader.slice_type)
            {
            case h264_PtypeB:
                pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_B << FRAME_TYPE_BOTTOM_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_TOP_OFFSET));
                break;
            case h264_PtypeSP:
            case h264_PtypeP:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_BOTTOM_OFFSET))>>FRAME_TYPE_BOTTOM_OFFSET != FRAME_TYPE_B)
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_P << FRAME_TYPE_BOTTOM_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_TOP_OFFSET));
                break;
            case h264_PtypeI:
            case h264_PtypeSI:
                if ( ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type) & (0x7 << FRAME_TYPE_BOTTOM_OFFSET))>>FRAME_TYPE_BOTTOM_OFFSET == FRAME_TYPE_INVALID)
                {
                    pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type = (FRAME_TYPE_I << FRAME_TYPE_BOTTOM_OFFSET)|(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type & (0x7 << FRAME_TYPE_TOP_OFFSET));
                }
                if (pInfo->sei_rp_received)
                    pInfo->last_I_frame_idc = pInfo->dpb.fs_dec_idc + PUT_LIST_INDEX_FIELD_BIT(1);
                else
                    pInfo->last_I_frame_idc = 255;

                break;
            default:
                break;

            }

        }

    }
    return;

}


//////#endif ///////////// IFDEF H264_PARSE_C///////////////////

