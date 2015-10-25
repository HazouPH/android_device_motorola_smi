//#define H264_PARSE_SLICE_HDR
//#ifdef H264_PARSE_SLICE_HDR

#include "h264.h"
#include "h264parse.h"

extern int32_t viddec_pm_get_au_pos(void *parent, uint32_t *bit, uint32_t *byte, unsigned char *is_emul);


/*-----------------------------------------------------------------------------------------*/
// Slice header 1----
// 1) first_mb_in_slice, slice_type, pic_parameter_id
/*-----------------------------------------------------------------------------------------*/
h264_Status h264_Parse_Slice_Header_1(void *parent,h264_Info* pInfo, h264_Slice_Header_t *SliceHeader)
{
    h264_Status ret = H264_STATUS_ERROR;

    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    int32_t slice_type =0;
    uint32_t data =0;

    do {
        ///// first_mb_in_slice
        SliceHeader->first_mb_in_slice = h264_GetVLCElement(parent, pInfo, false);

        ///// slice_type
        slice_type = h264_GetVLCElement(parent, pInfo, false);
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (slice_type > 9)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        SliceHeader->slice_type = (slice_type%5);

        if (SliceHeader->slice_type > h264_PtypeI)	{
            ret = H264_STATUS_NOTSUPPORT;
            break;
        }


        ////// pic_parameter_id
        data = h264_GetVLCElement(parent, pInfo, false);
        if (data > MAX_PIC_PARAMS) {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            ret = H264_PPS_INVALID_PIC_ID;
            break;
        }
        SliceHeader->pic_parameter_id  = (uint8_t)data;
        ret = H264_STATUS_OK;
    } while (0);

    return ret;
}

/*-----------------------------------------------------------------------------------------*/
// slice header 2
// 	frame_num
// 	field_pic_flag, structure
// 	idr_pic_id
// 	pic_order_cnt_lsb, delta_pic_order_cnt_bottom
/*-----------------------------------------------------------------------------------------*/

h264_Status h264_Parse_Slice_Header_2(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader)
{
    h264_Status ret = H264_SliceHeader_ERROR;

    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    uint32_t code;
    int32_t max_mb_num=0;

    do {
        //////////////////////////////////// Slice header part 2//////////////////

        /// Frame_num
        viddec_pm_get_bits(parent, &code, pInfo->active_SPS.log2_max_frame_num_minus4+4);
        SliceHeader->frame_num = (int32_t)code;

        /// Picture structure
        SliceHeader->structure = FRAME;
        SliceHeader->field_pic_flag = 0;
        SliceHeader->bottom_field_flag = 0;

        if (!(pInfo->active_SPS.sps_disp.frame_mbs_only_flag))
        {
            /// field_pic_flag
            viddec_pm_get_bits(parent, &code, 1);
            SliceHeader->field_pic_flag = (uint8_t)code;

            if (SliceHeader->field_pic_flag)
            {
                viddec_pm_get_bits(parent, &code, 1);
                SliceHeader->bottom_field_flag = (uint8_t)code;

                SliceHeader->structure = SliceHeader->bottom_field_flag? BOTTOM_FIELD: TOP_FIELD;
            }
        }

        ////// Check valid or not of first_mb_in_slice
        if (SliceHeader->structure == FRAME) {
            max_mb_num = pInfo->img.FrameHeightInMbs * pInfo->img.PicWidthInMbs;
        } else {
            max_mb_num = pInfo->img.FrameHeightInMbs * pInfo->img.PicWidthInMbs/2;
        }


        ///if(pInfo->img.MbaffFrameFlag)
        if (pInfo->active_SPS.sps_disp.mb_adaptive_frame_field_flag & (!(pInfo->SliceHeader.field_pic_flag))) {
            SliceHeader->first_mb_in_slice <<=1;
        }

        if (SliceHeader->first_mb_in_slice >= max_mb_num)
            break;


        if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
        {
            SliceHeader->idr_pic_id = h264_GetVLCElement(parent, pInfo, false);
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if (SliceHeader->idr_pic_id > 65535)
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
        }

        if (pInfo->active_SPS.pic_order_cnt_type == 0)
        {
            viddec_pm_get_bits(parent, &code , pInfo->active_SPS.log2_max_pic_order_cnt_lsb_minus4+4);
            SliceHeader->pic_order_cnt_lsb = (uint32_t)code;


            if ((pInfo->active_PPS.pic_order_present_flag) && !(SliceHeader->field_pic_flag))
            {
                SliceHeader->delta_pic_order_cnt_bottom = h264_GetVLCElement(parent, pInfo, true);
            }
            else
            {
                SliceHeader->delta_pic_order_cnt_bottom = 0;
            }
        }

        if ((pInfo->active_SPS.pic_order_cnt_type == 1) && !(pInfo->active_SPS.delta_pic_order_always_zero_flag))
        {
            SliceHeader->delta_pic_order_cnt[0] = h264_GetVLCElement(parent, pInfo, true);
            if ((pInfo->active_PPS.pic_order_present_flag) && !(SliceHeader->field_pic_flag))
            {
                SliceHeader->delta_pic_order_cnt[1] = h264_GetVLCElement(parent, pInfo, true);
            }
        }

        if (pInfo->active_PPS.redundant_pic_cnt_present_flag)
        {
            SliceHeader->redundant_pic_cnt = h264_GetVLCElement(parent, pInfo, false);
            if (SliceHeader->redundant_pic_cnt > 127)
                break;
        } else {
            SliceHeader->redundant_pic_cnt = 0;
        }

        ret = H264_STATUS_OK;
    } while (0);

    //////////// FMO is not supported curently, so comment out the following code
    //if((pInfo->active_PPS.num_slice_groups_minus1 > 0) && (pInfo->active_PPS.slice_group_map_type >= 3) && (pInfo->active_PPS.slice_group_map_type <= 5) )
    //{
    //	SliceHeader->slice_group_change_cycle = 0;				//one of the variables is not known in the high profile
    //}

    return ret;
}

/*-----------------------------------------------------------------------------------------*/
// slice header 3
// (direct_spatial_mv_pred_flag, num_ref_idx, pic_list_reorder, PWT,  ref_pic_remark, alpha, beta, etc)
/*-----------------------------------------------------------------------------------------*/

h264_Status h264_Parse_Slice_Header_3(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader)
{
    h264_Status ret = H264_SliceHeader_ERROR;

    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    int32_t  slice_alpha_c0_offset, slice_beta_offset;
    uint32_t code;
    uint32_t bits_offset =0, byte_offset =0;
    uint8_t  is_emul =0;

    do {
        /// direct_spatial_mv_pred_flag
        if (SliceHeader->slice_type == h264_PtypeB)
        {
            viddec_pm_get_bits(parent, &code , 1);
            SliceHeader->direct_spatial_mv_pred_flag = (uint8_t)code;
        }
        else
        {
            SliceHeader->direct_spatial_mv_pred_flag = 0;
        }

        //
        // Reset ref_idx and Overide it if exist
        //
        SliceHeader->num_ref_idx_l0_active = pInfo->active_PPS.num_ref_idx_l0_active;
        SliceHeader->num_ref_idx_l1_active = pInfo->active_PPS.num_ref_idx_l1_active;

        if ((SliceHeader->slice_type == h264_PtypeP) || (SliceHeader->slice_type == h264_PtypeSP) || (SliceHeader->slice_type == h264_PtypeB))
        {
            viddec_pm_get_bits(parent, &code, 1);
            SliceHeader->num_ref_idx_active_override_flag  = (uint8_t)code;

            if (SliceHeader->num_ref_idx_active_override_flag)
            {
                SliceHeader->num_ref_idx_l0_active = h264_GetVLCElement(parent, pInfo, false) + 1;
                if (SliceHeader->slice_type == h264_PtypeB)
                {
                    SliceHeader->num_ref_idx_l1_active = h264_GetVLCElement(parent, pInfo, false)+1;
                }
            }
        }

        if (SliceHeader->slice_type != h264_PtypeB) {
            SliceHeader->num_ref_idx_l1_active = 0;
        }

        if ((SliceHeader->num_ref_idx_l0_active > MAX_NUM_REF_FRAMES) || (SliceHeader->num_ref_idx_l1_active > MAX_NUM_REF_FRAMES))
        {
            break;
        }

        if (h264_Parse_Ref_Pic_List_Reordering(parent, pInfo, SliceHeader) != H264_STATUS_OK)
        {
            break;
        }


        ////
        //// Parse Pred_weight_table but not store it becasue it will be reparsed in HW
        ////
        if (((pInfo->active_PPS.weighted_pred_flag) && ((SliceHeader->slice_type == h264_PtypeP) || (SliceHeader->slice_type == h264_PtypeSP))) || ((pInfo->active_PPS.weighted_bipred_idc == 1) && (SliceHeader->slice_type == h264_PtypeB)))
        {

            viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

            pInfo->h264_pwt_enabled = 1;
            pInfo->h264_pwt_start_byte_offset = byte_offset;
            pInfo->h264_pwt_start_bit_offset  = bits_offset;

            if (h264_Parse_Pred_Weight_Table(parent, pInfo, SliceHeader) != H264_STATUS_OK)
            {
                break;
            }

            viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

            if (0 == bits_offset)
            {
                pInfo->h264_pwt_end_byte_offset = byte_offset-1;
                pInfo->h264_pwt_end_bit_offset  = 8;
            }
            else
            {
                pInfo->h264_pwt_end_byte_offset = byte_offset;
                pInfo->h264_pwt_end_bit_offset  = bits_offset;
            }

        }



        ////
        //// Parse Ref_pic marking if there
        ////
        if (SliceHeader->nal_ref_idc != 0)
        {
            if (h264_Parse_Dec_Ref_Pic_Marking(parent, pInfo, SliceHeader) != H264_STATUS_OK)
            {
                break;
            }
        }

        if ((pInfo->active_PPS.entropy_coding_mode_flag) && (SliceHeader->slice_type != h264_PtypeI) && (SliceHeader->slice_type != h264_PtypeSI))
        {
            SliceHeader->cabac_init_idc = h264_GetVLCElement(parent, pInfo, false);
        }
        else
        {
            SliceHeader->cabac_init_idc = 0;
        }

        if (SliceHeader->cabac_init_idc > 2)
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }

        SliceHeader->slice_qp_delta = h264_GetVLCElement(parent, pInfo, true);
        if ( (SliceHeader->slice_qp_delta > (25-pInfo->active_PPS.pic_init_qp_minus26)) || (SliceHeader->slice_qp_delta < -(26+pInfo->active_PPS.pic_init_qp_minus26)))
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }

        if ((SliceHeader->slice_type == h264_PtypeSP)|| (SliceHeader->slice_type == h264_PtypeSI) )
        {
            if (SliceHeader->slice_type == h264_PtypeSP)
            {
                viddec_pm_get_bits(parent, &code, 1);
                SliceHeader->sp_for_switch_flag  = (uint8_t)code;

            }
            SliceHeader->slice_qs_delta = h264_GetVLCElement(parent, pInfo, true);

            if ( (SliceHeader->slice_qs_delta > (25-pInfo->active_PPS.pic_init_qs_minus26)) || (SliceHeader->slice_qs_delta < -(26+pInfo->active_PPS.pic_init_qs_minus26)) )
            {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
                pInfo->sw_bail = 1;
#endif
#endif
                break;
            }
        }
        if (pInfo->active_PPS.deblocking_filter_control_present_flag)
        {
            SliceHeader->disable_deblocking_filter_idc = h264_GetVLCElement(parent, pInfo, false);
            if (SliceHeader->disable_deblocking_filter_idc != 1)
            {
                SliceHeader->slice_alpha_c0_offset_div2 = h264_GetVLCElement(parent, pInfo, true);
                slice_alpha_c0_offset = SliceHeader->slice_alpha_c0_offset_div2 << 1;
                if (slice_alpha_c0_offset < -12 || slice_alpha_c0_offset > 12)
                {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
                    pInfo->sw_bail = 1;
#endif
#endif
                    break;
                }

                SliceHeader->slice_beta_offset_div2 = h264_GetVLCElement(parent, pInfo, true);
                slice_beta_offset = SliceHeader->slice_beta_offset_div2 << 1;
                if (slice_beta_offset < -12 || slice_beta_offset > 12)
                {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
                    pInfo->sw_bail = 1;
#endif
#endif
                    break;
                }
            }
            else
            {
                SliceHeader->slice_alpha_c0_offset_div2 = 0;
                SliceHeader->slice_beta_offset_div2 = 0;
            }
        }

        ret = H264_STATUS_OK;
    } while (0);

    //////////// FMO is not supported curently, so comment out the following code
    //if((pInfo->active_PPS.num_slice_groups_minus1 > 0) && (pInfo->active_PPS.slice_group_map_type >= 3) && (pInfo->active_PPS.slice_group_map_type <= 5) )
    //{
    //	SliceHeader->slice_group_change_cycle = 0;				//one of the variables is not known in the high profile
    //}

    return ret;
}


/*--------------------------------------------------------------------------------------------------*/
//
// The syntax elements reordering_of_pic_nums_idc, abs_diff_pic_num_minus1, and long_term_pic_num
// specify the change from the initial reference picture lists to the reference picture lists to be used
// for decoding the slice

// reordering_of_pic_nums_idc:
//		0:	abs_diff_pic_num_minus1 is present and corresponds to a difference to subtract from a picture number prediction value
//		1:	abs_diff_pic_num_minus1 is present and corresponds to a difference to add to a picture number prediction value
//		2:	long_term_pic_num is present and specifies the long-term picture number for a reference picture
//		3:	End loop for reordering of the initial reference picture list
//
/*--------------------------------------------------------------------------------------------------*/

h264_Status h264_Parse_Ref_Pic_List_Reordering(void *parent, h264_Info* pInfo,h264_Slice_Header_t *SliceHeader)
{
    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    int32_t reorder= -1;
    uint32_t code;


    if ((SliceHeader->slice_type != h264_PtypeI) && (SliceHeader->slice_type != h264_PtypeSI))
    {
        viddec_pm_get_bits(parent, &code, 1);
        SliceHeader->sh_refpic_l0.ref_pic_list_reordering_flag = (uint8_t)code;

        if (SliceHeader->sh_refpic_l0.ref_pic_list_reordering_flag)
        {

            reorder= -1;
            do
            {
                reorder++;

                if (reorder > MAX_NUM_REF_FRAMES)
                {
                    return H264_SliceHeader_ERROR;
                }

                SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] = h264_GetVLCElement(parent, pInfo, false);
                if ((SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 0) || (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 1))
                {
                    SliceHeader->sh_refpic_l0.list_reordering_num[reorder].abs_diff_pic_num_minus1 = h264_GetVLCElement(parent, pInfo, false);
                }
                else if (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] == 2)
                {
                    SliceHeader->sh_refpic_l0.list_reordering_num[reorder].long_term_pic_num = h264_GetVLCElement(parent, pInfo, false);
                }

            } while (SliceHeader->sh_refpic_l0.reordering_of_pic_nums_idc[reorder] != 3);
        }
    }

    if (SliceHeader->slice_type == h264_PtypeB)
    {
        viddec_pm_get_bits(parent, &code, 1);
        SliceHeader->sh_refpic_l1.ref_pic_list_reordering_flag = (uint8_t)code;

        if (SliceHeader->sh_refpic_l1.ref_pic_list_reordering_flag)
        {

            reorder = -1;
            do
            {
                reorder++;
                if (reorder > MAX_NUM_REF_FRAMES)
                {
                    return H264_SliceHeader_ERROR;
                }
                SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] = h264_GetVLCElement(parent, pInfo, false);
                if ((SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 0) || (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 1))
                {
                    SliceHeader->sh_refpic_l1.list_reordering_num[reorder].abs_diff_pic_num_minus1 = h264_GetVLCElement(parent, pInfo, false);
                }
                else if (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] == 2)
                {
                    SliceHeader->sh_refpic_l1.list_reordering_num[reorder].long_term_pic_num = h264_GetVLCElement(parent, pInfo, false);
                }
            } while (SliceHeader->sh_refpic_l1.reordering_of_pic_nums_idc[reorder] != 3);
        }
    }

    //currently just two reference frames but in case mroe than two, then should use an array for the above structures that is why reorder
    return H264_STATUS_OK;

}

#ifdef VBP
h264_Status h264_Parse_Pred_Weight_Table(void *parent, h264_Info* pInfo,h264_Slice_Header_t *SliceHeader)
{
    uint32_t i =0, j=0;
    uint32_t flag;

    SliceHeader->sh_predwttbl.luma_log2_weight_denom = h264_GetVLCElement(parent, pInfo, false);

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
    if (SliceHeader->sh_predwttbl.luma_log2_weight_denom > 7)
    {
        pInfo->sw_bail = 1;
    }
#endif
#endif
    if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
    {
        SliceHeader->sh_predwttbl.chroma_log2_weight_denom = h264_GetVLCElement(parent,pInfo, false);
    }
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
    if (SliceHeader->sh_predwttbl.chroma_log2_weight_denom > 7)
    {
        pInfo->sw_bail = 1;
    }
#endif
#endif
    for (i=0; i< SliceHeader->num_ref_idx_l0_active; i++)
    {
        viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
        SliceHeader->sh_predwttbl.luma_weight_l0_flag = flag;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if ((-128 > flag) || (127 < flag))
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        if (SliceHeader->sh_predwttbl.luma_weight_l0_flag)
        {
            SliceHeader->sh_predwttbl.luma_weight_l0[i] = h264_GetVLCElement(parent, pInfo, true);
            SliceHeader->sh_predwttbl.luma_offset_l0[i] = h264_GetVLCElement(parent, pInfo, true);
        }
        else
        {
            SliceHeader->sh_predwttbl.luma_weight_l0[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
            SliceHeader->sh_predwttbl.luma_offset_l0[i] = 0;
        }

        if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
        {
            viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
            SliceHeader->sh_predwttbl.chroma_weight_l0_flag = flag;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if ((-128 > flag) || (127 < flag))
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
            if (SliceHeader->sh_predwttbl.chroma_weight_l0_flag)
            {
                for (j=0; j <2; j++)
                {
                    SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = h264_GetVLCElement(parent, pInfo, true);
                    SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = h264_GetVLCElement(parent, pInfo, true);
                }
            }
            else
            {
                for (j=0; j <2; j++)
                {
                    SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                    SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = 0;
                }
            }
        }

    }

    if (SliceHeader->slice_type == h264_PtypeB)
    {
        for (i=0; i< SliceHeader->num_ref_idx_l1_active; i++)
        {
            viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
            SliceHeader->sh_predwttbl.luma_weight_l1_flag = flag;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if ((-128 > flag) || (127 < flag))
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
            if (SliceHeader->sh_predwttbl.luma_weight_l1_flag)
            {
                SliceHeader->sh_predwttbl.luma_weight_l1[i] = h264_GetVLCElement(parent, pInfo, true);
                SliceHeader->sh_predwttbl.luma_offset_l1[i] = h264_GetVLCElement(parent, pInfo, true);
            }
            else
            {
                SliceHeader->sh_predwttbl.luma_weight_l1[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
                SliceHeader->sh_predwttbl.luma_offset_l1[i] = 0;
            }

            if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
            {
                viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
                SliceHeader->sh_predwttbl.chroma_weight_l1_flag = flag;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
                if ((-128 > flag) || (127 < flag))
                {
                    pInfo->sw_bail = 1;
                }
#endif
#endif
                if (SliceHeader->sh_predwttbl.chroma_weight_l1_flag)
                {
                    for (j=0; j <2; j++)
                    {
                        SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = h264_GetVLCElement(parent, pInfo, true);
                        SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = h264_GetVLCElement(parent, pInfo, true);
                    }
                }
                else
                {
                    for (j=0; j <2; j++)
                    {
                        SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                        SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = 0;
                    }
                }
            }

        }
    }

    return H264_STATUS_OK;
} ///// End of h264_Parse_Pred_Weight_Table

#else

/*--------------------------------------------------------------------------------------------------*/
//
// Parse Prediction weight table
// Note: This table will be reparsed in HW Accelerator, so needn't keep it in parser
//
/*--------------------------------------------------------------------------------------------------*/


h264_Status h264_Parse_Pred_Weight_Table(void *parent, h264_Info* pInfo,h264_Slice_Header_t *SliceHeader)
{
    uint32_t i =0, j=0;
    uint32_t flag, val;
    //h264_Slice_Header_t* SliceHeader = &pInfo->SPS.SliceHeader;

    //SliceHeader->sh_predwttbl.luma_log2_weight_denom = h264_GetVLCElement(pInfo, false, "luma_log2_weight_denom");
    val = h264_GetVLCElement(parent, pInfo, false);

    if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
    {
        //SliceHeader->sh_predwttbl.chroma_log2_weight_denom = h264_GetVLCElement(pInfo, false, "chroma_log2_weight_denom");
        val = h264_GetVLCElement(parent,pInfo, false);
    }

    for (i=0; i< SliceHeader->num_ref_idx_l0_active; i++)
    {
        //SliceHeader->sh_predwttbl.luma_weight_l0_flag = h264_GetBits(pInfo, 1, "luma_weight_l0_flag");
        viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);

        //if(SliceHeader->sh_predwttbl.luma_weight_l0_flag)
        if (flag)
        {
            //SliceHeader->sh_predwttbl.luma_weight_l0[i] = h264_GetVLCElement(pInfo, true, "luma_weight_l0");
            val = h264_GetVLCElement(parent, pInfo, true);
            //SliceHeader->sh_predwttbl.luma_offset_l0[i] = h264_GetVLCElement(pInfo, true, "luma_offset_l0");
            val = h264_GetVLCElement(parent, pInfo, true);
        }
        else
        {
            //SliceHeader->sh_predwttbl.luma_weight_l0[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
            //SliceHeader->sh_predwttbl.luma_offset_l0[i] = 0;
        }

        if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
        {
            //SliceHeader->sh_predwttbl.chroma_weight_l0_flag = h264_GetBits(pInfo, 1, "chroma_weight_l0_flag");
            viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
            if (flag)
            {
                for (j=0; j <2; j++)
                {
                    //SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = h264_GetVLCElement(pInfo, true, "chroma_weight_l0");
                    val = h264_GetVLCElement(parent, pInfo, true);
                    //SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = h264_GetVLCElement(pInfo, true, "chroma_offset_l0");
                    val = h264_GetVLCElement(parent, pInfo, true);
                }
            }
            else
            {
                for (j=0; j <2; j++)
                {
                    //SliceHeader->sh_predwttbl.chroma_weight_l0[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                    //SliceHeader->sh_predwttbl.chroma_offset_l0[i][j] = 0;
                }
            }
        }

    }

    if (SliceHeader->slice_type == h264_PtypeB)
    {
        for (i=0; i< SliceHeader->num_ref_idx_l1_active; i++)
        {
            //SliceHeader->sh_predwttbl.luma_weight_l1_flag = h264_GetBits(pInfo, 1, "luma_weight_l1_flag");
            viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
            if (flag)
            {
                //SliceHeader->sh_predwttbl.luma_weight_l1[i] = h264_GetVLCElement(pInfo, true, "luma_weight_l1");
                val = h264_GetVLCElement(parent, pInfo, true);
                //SliceHeader->sh_predwttbl.luma_offset_l1[i] = h264_GetVLCElement(pInfo, true, "luma_offset_l1");
                val = h264_GetVLCElement(parent, pInfo, true);
            }
            else
            {
                //SliceHeader->sh_predwttbl.luma_weight_l1[i] = (1 << SliceHeader->sh_predwttbl.luma_log2_weight_denom);
                //SliceHeader->sh_predwttbl.luma_offset_l1[i] = 0;
            }

            if (pInfo->active_SPS.sps_disp.chroma_format_idc != 0)
            {
                //SliceHeader->sh_predwttbl.chroma_weight_l1_flag = h264_GetBits(pInfo, 1, "chroma_weight_l1_flag");
                viddec_pm_get_bits(parent, (uint32_t *)&flag, 1);
                if (flag)
                {
                    for (j=0; j <2; j++)
                    {
                        //SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = h264_GetVLCElement(pInfo, true, "chroma_weight_l1");
                        val = h264_GetVLCElement(parent, pInfo, true);
                        //SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = h264_GetVLCElement(pInfo, true, "chroma_offset_l1");
                        val = h264_GetVLCElement(parent, pInfo, true);
                    }
                }
                else
                {
                    for (j=0; j <2; j++)
                    {
                        //SliceHeader->sh_predwttbl.chroma_weight_l1[i][j] = (1 << SliceHeader->sh_predwttbl.chroma_log2_weight_denom);
                        //SliceHeader->sh_predwttbl.chroma_offset_l1[i][j] = 0;
                    }
                }
            }

        }
    }

    return H264_STATUS_OK;
} ///// End of h264_Parse_Pred_Weight_Table

#endif

/*--------------------------------------------------------------------------------------------------*/
// The syntax elements specify marking of the reference pictures.
//			1)IDR:		no_output_of_prior_pics_flag,
//						long_term_reference_flag,
//			2)NonIDR:	adaptive_ref_pic_marking_mode_flag,
//						memory_management_control_operation,
//						difference_of_pic_nums_minus1,
//						long_term_frame_idx,
//						long_term_pic_num, and
//						max_long_term_frame_idx_plus1
//
//The marking of a reference picture can be "unused for reference", "used for short-term reference", or "used for longterm
// reference", but only one among these three.
/*--------------------------------------------------------------------------------------------------*/


h264_Status h264_Parse_Dec_Ref_Pic_Marking(void *parent, h264_Info* pInfo,h264_Slice_Header_t *SliceHeader)
{
    //h264_Slice_Header_t* SliceHeader = &pInfo->SliceHeader;
    uint8_t i = 0;
    uint32_t code = 0;

    if (pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
    {
        viddec_pm_get_bits(parent, &code, 1);
        SliceHeader->sh_dec_refpic.no_output_of_prior_pics_flag = (uint8_t)code;

        viddec_pm_get_bits(parent, &code, 1);
        SliceHeader->sh_dec_refpic.long_term_reference_flag = (uint8_t)code;
        pInfo->img.long_term_reference_flag = (uint8_t)code;
    }
    else
    {
        viddec_pm_get_bits(parent, &code, 1);
        SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag = (uint8_t)code;

        ///////////////////////////////////////////////////////////////////////////////////////
        //adaptive_ref_pic_marking_mode_flag 			Reference picture marking mode specified
        //	0 						Sliding window reference picture marking mode: A marking mode
        //							providing a first-in first-out mechanism for short-term reference pictures.
        //  	1 						Adaptive reference picture marking mode: A reference picture
        //							marking mode providing syntax elements to specify marking of
        //							reference pictures as �unused for reference?and to assign long-term
        //							frame indices.
        ///////////////////////////////////////////////////////////////////////////////////////

        if (SliceHeader->sh_dec_refpic.adaptive_ref_pic_marking_mode_flag)
        {
            do
            {
                if (i < NUM_MMCO_OPERATIONS)
                {
                    SliceHeader->sh_dec_refpic.memory_management_control_operation[i] = h264_GetVLCElement(parent, pInfo, false);
                    if ((SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 1) || (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 3))
                    {
                        SliceHeader->sh_dec_refpic.difference_of_pic_num_minus1[i] = h264_GetVLCElement(parent, pInfo, false);
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 2)
                    {
                        SliceHeader->sh_dec_refpic.long_term_pic_num[i] = h264_GetVLCElement(parent, pInfo, false);
                    }

                    if ((SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 3) || (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 6))
                    {
                        SliceHeader->sh_dec_refpic.long_term_frame_idx[i] = h264_GetVLCElement(parent, pInfo, false);
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 4)
                    {
                        SliceHeader->sh_dec_refpic.max_long_term_frame_idx_plus1[i] = h264_GetVLCElement(parent, pInfo, false);
                    }

                    if (SliceHeader->sh_dec_refpic.memory_management_control_operation[i] == 5)
                    {
                        pInfo->img.curr_has_mmco_5 = 1;
                    }
                }

                if (i >= NUM_MMCO_OPERATIONS) {
                    return H264_STATUS_ERROR;
                }

            } while (SliceHeader->sh_dec_refpic.memory_management_control_operation[i++] != 0);
        }
    }




    SliceHeader->sh_dec_refpic.dec_ref_pic_marking_count = i;

    return H264_STATUS_OK;
}



//#endif
