//#define H264_PARSE_SPS_C
//#ifdef H264_PARSE_SPS_C

#include "h264.h"
#include "h264parse.h"
#ifdef VBP
#include<math.h>
#endif


/// SPS extension unit (unit_type = 13)
///
#if 0
h264_Status h264_Parse_SeqParameterSet_Extension(void *parent,h264_Info * pInfo)
{
    /*h264_SPS_Extension_RBSP_t* SPS_ext = pInfo->p_active_SPS_ext;

    SPS_ext->seq_parameter_set_id = h264_GetVLCElement(pInfo, false);
    if(SPS_ext->seq_parameter_set_id > MAX_SEQ_PARAMS-1)
    {
    	return H264_SPS_ERROR;
    }
    SPS_ext->aux_format_idc = h264_GetVLCElement(pInfo, false);
    if(SPS_ext->aux_format_idc  > 3)
    {
    	return H264_SPS_ERROR;
    }
    if(SPS_ext->aux_format_idc != 0)
    {
    	SPS_ext->bit_depth_aux_minus8 = h264_GetVLCElement(pInfo, false);
    	if(SPS_ext->bit_depth_aux_minus8 + 8 > 12)
    	{
    		return H264_SPS_ERROR;
    	}

    	SPS_ext->alpha_incr_flag = h264_GetBits(pInfo, 1, "alpha_incr_flag");
    	if(SPS_ext->alpha_incr_flag > 1)
    	{
    		return H264_SPS_ERROR;
    	}

    	SPS_ext->alpha_opaque_value = h264_GetBits(pInfo,(SPS_ext->bit_depth_aux_minus8+8+1), "alpha_opaque_value");		//+8 to get the bit_depth value
    	SPS_ext->alpha_transparent_value = h264_GetBits(pInfo,(SPS_ext->bit_depth_aux_minus8+8+1), "alpha_transparent_value");		//+8 to get the bit_depth value
    }
    SPS_ext->additional_extension_flag = h264_GetBits(pInfo, 1, "additional_extension_flag");
    */
    return H264_STATUS_OK;
}
#endif


h264_Status h264_Parse_HRD_Parameters(void *parent, h264_Info* pInfo, int nal_hrd,seq_param_set_used_ptr SPS, vui_seq_parameters_t_not_used_ptr pVUI_Seq_Not_Used)
{
    //seq_param_set_ptr SPS = pInfo->p_active_SPS;
    int32_t i = 0;
    uint32_t code;


    if (nal_hrd)
    {
        SPS->sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1 = h264_GetVLCElement(parent, pInfo, false);

        if (SPS->sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1 >= MAX_CPB_CNT)
        {
            return H264_SPS_ERROR;
        }

        viddec_pm_get_bits(parent, &code, 8);
        pVUI_Seq_Not_Used->nal_hrd_bit_rate_scale = (uint8_t)(code>>4);
        pVUI_Seq_Not_Used->nal_hrd_cpb_size_scale = (uint8_t)(code & 0xf);

        for (i=0; i<=SPS->sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1; i++)
        {
            pVUI_Seq_Not_Used->nal_hrd_parameters.bit_rate_value_minus1[i] = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->nal_hrd_parameters.cpb_size_value_minus1[i] = h264_GetVLCElement(parent, pInfo, false);

            viddec_pm_get_bits(parent, &code, 1);
            pVUI_Seq_Not_Used->nal_hrd_parameters.cbr_flag[i] = (uint8_t)code;
        }

        if ( viddec_pm_get_bits(parent, &code, 20) == -1)
            return H264_SPS_ERROR;

        SPS->sps_disp.vui_seq_parameters.nal_hrd_initial_cpb_removal_delay_length_minus1 = (uint8_t)((code>>15)&0x1f);
        SPS->sps_disp.vui_seq_parameters.nal_hrd_cpb_removal_delay_length_minus1 = (uint8_t)((code>>10)&0x1f);;
        SPS->sps_disp.vui_seq_parameters.nal_hrd_dpb_output_delay_length_minus1 = (uint8_t)((code>>5)&0x1f);;
        SPS->sps_disp.vui_seq_parameters.nal_hrd_time_offset_length = (uint8_t)(code&0x1f);;

    }
    else
    {
        SPS->sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1 = h264_GetVLCElement(parent, pInfo, false);

        if (SPS->sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1 >= MAX_CPB_CNT)
        {
            return H264_SPS_ERROR;
        }

        viddec_pm_get_bits(parent, &code, 8);
        pVUI_Seq_Not_Used->vcl_hrd_bit_rate_scale = (uint8_t)(code>>4);
        pVUI_Seq_Not_Used->vcl_hrd_cpb_size_scale = (uint8_t)(code&0xf);

        for (i=0; i<=SPS->sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1; i++)
        {
            pVUI_Seq_Not_Used->vcl_hrd_parameters.bit_rate_value_minus1[i] = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->vcl_hrd_parameters.cpb_size_value_minus1[i] = h264_GetVLCElement(parent, pInfo, false);
            viddec_pm_get_bits(parent, &code, 1);
            pVUI_Seq_Not_Used->vcl_hrd_parameters.cbr_flag[i] = (uint8_t)code;
        }

        if ( viddec_pm_get_bits(parent, &code, 20) == -1)
            return H264_SPS_ERROR;

        SPS->sps_disp.vui_seq_parameters.vcl_hrd_initial_cpb_removal_delay_length_minus1 = (uint8_t)((code>>15)&0x1f);
        SPS->sps_disp.vui_seq_parameters.vcl_hrd_cpb_removal_delay_length_minus1 = (uint8_t)((code>>10)&0x1f);;
        SPS->sps_disp.vui_seq_parameters.vcl_hrd_dpb_output_delay_length_minus1 = (uint8_t)((code>>5)&0x1f);;
        SPS->sps_disp.vui_seq_parameters.vcl_hrd_time_offset_length = (uint8_t)(code&0x1f);;
    }

    return H264_STATUS_OK;
}



h264_Status h264_Parse_Vui_Parameters(void *parent, h264_Info* pInfo, seq_param_set_used_ptr SPS, vui_seq_parameters_t_not_used_ptr pVUI_Seq_Not_Used)
{
    h264_Status ret = H264_STATUS_OK;
    //seq_param_set_ptr SPS = pInfo->p_active_SPS;
    int32_t nal_hrd = 0;
    uint32_t code;

    do {
        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag = (uint8_t)code;


        if (SPS->sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag)
        {
            viddec_pm_get_bits(parent, &code, 8);
            SPS->sps_disp.vui_seq_parameters.aspect_ratio_idc = (uint8_t)code;

            if (SPS->sps_disp.vui_seq_parameters.aspect_ratio_idc == h264_AR_Extended_SAR)
            {
                viddec_pm_get_bits(parent, &code, 16);
                SPS->sps_disp.vui_seq_parameters.sar_width = (uint16_t)code;

                viddec_pm_get_bits(parent, &code, 16);
                SPS->sps_disp.vui_seq_parameters.sar_height = (uint16_t)code;

            }
        }

        viddec_pm_get_bits(parent, &code, 1);
        pVUI_Seq_Not_Used->overscan_info_present_flag = (uint8_t)code;

        if (pVUI_Seq_Not_Used->overscan_info_present_flag)
        {
            viddec_pm_get_bits(parent, &code, 1);
            pVUI_Seq_Not_Used->overscan_appropriate_flag = (uint8_t)code;
        }

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.video_signal_type_present_flag = (uint8_t)code;

        if (SPS->sps_disp.vui_seq_parameters.video_signal_type_present_flag)
        {
            viddec_pm_get_bits(parent, &code, 3);
            SPS->sps_disp.vui_seq_parameters.video_format = (uint8_t)code;

            viddec_pm_get_bits(parent, &code, 1);
            pVUI_Seq_Not_Used->video_full_range_flag = (uint8_t)code;
#ifdef VBP
            SPS->sps_disp.vui_seq_parameters.video_full_range_flag = (uint8_t)code;
#endif

            viddec_pm_get_bits(parent, &code, 1);
            SPS->sps_disp.vui_seq_parameters.colour_description_present_flag = (uint8_t)code;

            if (SPS->sps_disp.vui_seq_parameters.colour_description_present_flag)
            {
                viddec_pm_get_bits(parent, &code, 8);
                SPS->sps_disp.vui_seq_parameters.colour_primaries = (uint8_t)code;

                viddec_pm_get_bits(parent, &code, 8);
                SPS->sps_disp.vui_seq_parameters.transfer_characteristics = (uint8_t)code;

                viddec_pm_get_bits(parent, &code, 8);
                pVUI_Seq_Not_Used->matrix_coefficients = (uint8_t)code;
#ifdef VBP
                SPS->sps_disp.vui_seq_parameters.matrix_coefficients = (uint8_t)code;
#endif
            }
        }

        viddec_pm_get_bits(parent, &code, 1);
        pVUI_Seq_Not_Used->chroma_location_info_present_flag = (uint8_t)code;

        if (pVUI_Seq_Not_Used->chroma_location_info_present_flag)
        {
            pVUI_Seq_Not_Used->chroma_sample_loc_type_top_field = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->chroma_sample_loc_type_bottom_field = h264_GetVLCElement(parent, pInfo, false);
        }

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.timing_info_present_flag = (uint8_t)code;

        if (SPS->sps_disp.vui_seq_parameters.timing_info_present_flag == 1)
        {
            viddec_pm_get_bits(parent, &code, 32);
            SPS->sps_disp.vui_seq_parameters.num_units_in_tick = (uint32_t)code;

            viddec_pm_get_bits(parent, &code, 32);
            SPS->sps_disp.vui_seq_parameters.time_scale = (uint32_t)code;

            viddec_pm_get_bits(parent, &code, 1);
            SPS->sps_disp.vui_seq_parameters.fixed_frame_rate_flag = (uint8_t)code;
        }

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag = (uint8_t)code;

        if (SPS->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag == 1)
        {
            nal_hrd = 1;
            ret = h264_Parse_HRD_Parameters(parent,pInfo, nal_hrd,SPS, pVUI_Seq_Not_Used);
        }

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag = (uint8_t)code;

        if (SPS->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag == 1)
        {
            nal_hrd = 0;
            ret = (h264_Status)h264_Parse_HRD_Parameters(parent,pInfo, nal_hrd,SPS, pVUI_Seq_Not_Used);
        }

        if ((SPS->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag == 1) || (SPS->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag == 1))
        {
            viddec_pm_get_bits(parent, &code, 1);
            SPS->sps_disp.vui_seq_parameters.low_delay_hrd_flag = (uint8_t)code;
        }

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.vui_seq_parameters.pic_struct_present_flag = (uint8_t)code;

        if (viddec_pm_get_bits(parent, &code, 1) == -1) {
            ret = H264_STATUS_ERROR;
            break;
        }
        SPS->sps_disp.vui_seq_parameters.bitstream_restriction_flag = (uint8_t)code;

        if (SPS->sps_disp.vui_seq_parameters.bitstream_restriction_flag)
        {
            viddec_pm_get_bits(parent, &code, 1);
            pVUI_Seq_Not_Used->motion_vectors_over_pic_boundaries_flag = (uint8_t)code;

            pVUI_Seq_Not_Used->max_bytes_per_pic_denom = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->max_bits_per_mb_denom = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->log2_max_mv_length_horizontal = h264_GetVLCElement(parent, pInfo, false);
            pVUI_Seq_Not_Used->log2_max_mv_length_vertical = h264_GetVLCElement(parent, pInfo, false);
            SPS->sps_disp.vui_seq_parameters.num_reorder_frames = h264_GetVLCElement(parent, pInfo, false);
            SPS->sps_disp.vui_seq_parameters.max_dec_frame_buffering = h264_GetVLCElement(parent, pInfo, false);

            if (SPS->sps_disp.vui_seq_parameters.max_dec_frame_buffering == MAX_INT32_VALUE)
                ret = H264_STATUS_ERROR;
        }
    } while (0);

    return ret;
}


h264_Status h264_Parse_SeqParameterSet(void *parent,h264_Info * pInfo, seq_param_set_used_ptr SPS, vui_seq_parameters_t_not_used_ptr pVUI_Seq_Not_Used, int32_t* pOffset_ref_frame)
{
    h264_Status ret = H264_SPS_ERROR;

    int32_t i = 0, tmp = 0;
    int32_t PicWidthInMbs, PicHeightInMapUnits, FrameHeightInMbs;
    uint32_t code = 0;
    uint32_t data = 0;

    //SPS->profile_idc = h264_GetBits(pInfo, 8, "Profile");
    viddec_pm_get_bits(parent, &code, 8);
    SPS->profile_idc = (uint8_t)code;

    switch (SPS->profile_idc)
    {
    case h264_ProfileBaseline:
    case h264_ProfileMain:
    case h264_ProfileExtended:
    case h264_ProfileHigh10:
    case h264_ProfileHigh422:
    case h264_ProfileHigh444:
    case h264_ProfileHigh:
        break;
    default:
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        pInfo->sw_bail = 1;
#endif
#endif
        return H264_SPS_INVALID_PROFILE;
        break;
    }

    //SPS->constraint_set0_flag = h264_GetBits(pInfo, 1, "constraint_set0_flag");
    //SPS->constraint_set1_flag = h264_GetBits(pInfo, 1, "constraint_set1_flag");		//should be 1
    //SPS->constraint_set2_flag = h264_GetBits(pInfo, 1, "constraint_set2_flag");
    //SPS->constraint_set3_flag = h264_GetBits(pInfo, 1, "constraint_set3_flag");

#ifdef VBP
    viddec_pm_get_bits(parent, &code, 5);	 //constraint flag set0...set4 (h.264 Spec v2009)
    SPS->constraint_set_flags = (uint8_t)code;

    //// reserved_zero_3bits
    viddec_pm_get_bits(parent, (uint32_t *)&code, 3); //3bits zero reserved (h.264 Spec v2009)
#else

    viddec_pm_get_bits(parent, &code, 4);
    SPS->constraint_set_flags = (uint8_t)code;

    //// reserved_zero_4bits
    viddec_pm_get_bits(parent, (uint32_t *)&code, 4);
#endif
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
    if (code != 0)
    {
        pInfo->sw_bail = 1;
    }
#endif
#endif
    viddec_pm_get_bits(parent, &code, 8);
    SPS->level_idc = (uint8_t)code;

    switch (SPS->level_idc)
    {
    case h264_Level1b:
    case h264_Level1:
    case h264_Level11:
    case h264_Level12:
    case h264_Level13:
    case h264_Level2:
    case h264_Level21:
    case h264_Level22:
    case h264_Level3:
    case h264_Level31:
    case h264_Level32:
    case h264_Level4:
    case h264_Level41:
    case h264_Level42:
    case h264_Level5:
    case h264_Level51:
        break;
    default:
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        pInfo->sw_bail = 1;
#endif
#endif
        return H264_SPS_INVALID_LEVEL;
    }

    do {
        SPS->seq_parameter_set_id = h264_GetVLCElement(parent, pInfo, false);

        //// seq_parameter_set_id ---[0,31]
        if (SPS->seq_parameter_set_id > MAX_NUM_SPS -1)
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }
#ifdef VBP
        SPS->sps_disp.separate_colour_plane_flag = 0;
#endif

        if ((SPS->profile_idc == h264_ProfileHigh) || (SPS->profile_idc == h264_ProfileHigh10) ||
                (SPS->profile_idc == h264_ProfileHigh422) || (SPS->profile_idc == h264_ProfileHigh444)   )
        {
            //// chroma_format_idc ---[0,3], currently we don't support 444, so [0,2]
            data = h264_GetVLCElement(parent, pInfo, false);
            if ( data > H264_CHROMA_422)
                break;
            SPS->sps_disp.chroma_format_idc = (uint8_t)data;
            //if(SPS->sps_disp.chroma_format_idc == H264_CHROMA_444) {}

#ifdef VBP
            if(SPS->sps_disp.chroma_format_idc == H264_CHROMA_444) {
                viddec_pm_get_bits(parent, &code, 1);
                SPS->sps_disp.separate_colour_plane_flag = (uint8_t)data;
            }
#endif
            //// bit_depth_luma_minus8 ---[0,4], -----only support 8-bit pixel
            data = h264_GetVLCElement(parent, pInfo, false);
            if ( data)
                break;
            SPS->bit_depth_luma_minus8 = (uint8_t)data;

            //// bit_depth_chroma_minus8 ---[0,4]
            data = h264_GetVLCElement(parent, pInfo, false);
            if ( data )
                break;
            SPS->bit_depth_chroma_minus8 = (uint8_t)data;


            viddec_pm_get_bits(parent, &code, 1);
            SPS->lossless_qpprime_y_zero_flag = (uint8_t)code;

            viddec_pm_get_bits(parent, &code, 1);
            SPS->seq_scaling_matrix_present_flag = (uint8_t)code;

            if (SPS->seq_scaling_matrix_present_flag == 1)
            {
                //int n_ScalingList = (SPS->sps_disp.chroma_format_idc != H264_CHROMA_444) ? 8 : 12;
                int n_ScalingList = 8;				/// We do not support 444 currrently

                for (i=0; i<n_ScalingList; i++)
                {
                    viddec_pm_get_bits(parent, &code, 1);
                    SPS->seq_scaling_list_present_flag[i] = (uint8_t)code;

                    if (SPS->seq_scaling_list_present_flag[i])
                    {
                        if (i<6)
                            h264_Scaling_List(parent, SPS->ScalingList4x4[i], 16, &SPS->UseDefaultScalingMatrix4x4Flag[i], pInfo);
                        else
                            h264_Scaling_List(parent, SPS->ScalingList8x8[i-6], 64, &SPS->UseDefaultScalingMatrix8x8Flag[i-6], pInfo);
                    }
                }
            }
        }
        else
        {
            SPS->sps_disp.chroma_format_idc = 1;
            SPS->seq_scaling_matrix_present_flag = 0;

            SPS->bit_depth_luma_minus8 = 0;
            SPS->bit_depth_chroma_minus8 = 0;
            //h264_SetDefaultScalingLists(pInfo);
        }

        //// log2_max_frame_num_minus4 ---[0,12]
        data = (h264_GetVLCElement(parent, pInfo, false));
        if ( data > 12)
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }
        SPS->log2_max_frame_num_minus4 = (uint8_t)data;

        //// pic_order_cnt_type ---- [0,2]
        data = h264_GetVLCElement(parent, pInfo, false);
        if ( data > 2)
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }
        SPS->pic_order_cnt_type = (uint8_t)data;


        SPS->expectedDeltaPerPOCCycle = 0;
        if (SPS->pic_order_cnt_type == 0)	{
            SPS->log2_max_pic_order_cnt_lsb_minus4 = h264_GetVLCElement(parent, pInfo, false);
        } else if (SPS->pic_order_cnt_type == 1) {
            viddec_pm_get_bits(parent, &code, 1);
            SPS->delta_pic_order_always_zero_flag = (uint8_t)code;

            SPS->offset_for_non_ref_pic = h264_GetVLCElement(parent, pInfo, true);
            SPS->offset_for_top_to_bottom_field = h264_GetVLCElement(parent, pInfo, true);

            //// num_ref_frames_in_pic_order_cnt_cycle ---- [0,255]
            data = h264_GetVLCElement(parent, pInfo, false);
            if ( data > 255)
            {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
                pInfo->sw_bail = 1;
#endif
#endif
                break;
            }
            SPS->num_ref_frames_in_pic_order_cnt_cycle = (uint8_t)data;


            //Alloc memory for frame offset -- FIXME
            for (i=0; i< SPS->num_ref_frames_in_pic_order_cnt_cycle; i++)
            {
                /////SPS->offset_for_ref_frame[i] could be removed from SPS
#ifndef USER_MODE
                tmp = h264_GetVLCElement(parent, pInfo, true);
                pOffset_ref_frame[i]=tmp;
                SPS->expectedDeltaPerPOCCycle += tmp;
#else
                tmp = h264_GetVLCElement(parent, pInfo, true);
                SPS->offset_for_ref_frame[i]=tmp;
                SPS->expectedDeltaPerPOCCycle += tmp;
#endif
            }
        }

        //// num_ref_frames ---[0,16]
        data = h264_GetVLCElement(parent, pInfo, false);
        if ( data > 16)
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }
        SPS->num_ref_frames = (uint8_t)data;

        viddec_pm_get_bits(parent, &code, 1);
        SPS->gaps_in_frame_num_value_allowed_flag = (uint8_t)code;


        SPS->sps_disp.pic_width_in_mbs_minus1 = h264_GetVLCElement(parent, pInfo, false);
        SPS->sps_disp.pic_height_in_map_units_minus1 = h264_GetVLCElement(parent, pInfo, false);
        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.frame_mbs_only_flag = (uint8_t)code;

        /// err check for size
        PicWidthInMbs       = (SPS->sps_disp.pic_width_in_mbs_minus1 + 1);
        PicHeightInMapUnits = (SPS->sps_disp.pic_height_in_map_units_minus1 + 1);
        FrameHeightInMbs    = SPS->sps_disp.frame_mbs_only_flag? PicHeightInMapUnits: (PicHeightInMapUnits<<1);

        if (!SPS->sps_disp.frame_mbs_only_flag)
        {
            viddec_pm_get_bits(parent, &code, 1);
            SPS->sps_disp.mb_adaptive_frame_field_flag = (uint8_t)code;
        }

        //SPS->frame_height_in_mbs = (2-SPS->sps_disp.frame_mbs_only_flag)*(SPS->sps_disp.pic_height_in_map_units_minus1+1);
        //SPS->pic_size_in_map_units = (SPS->sps_disp.pic_width_in_mbs_minus1+1)*SPS->sps_disp.frame_height_in_mbs;

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.direct_8x8_inference_flag = (uint8_t)code;

        viddec_pm_get_bits(parent, &code, 1);
        SPS->sps_disp.frame_cropping_flag = (uint8_t)code;

        if (SPS->sps_disp.frame_cropping_flag)
        {
            SPS->sps_disp.frame_crop_rect_left_offset = h264_GetVLCElement(parent, pInfo, false);
            SPS->sps_disp.frame_crop_rect_right_offset = h264_GetVLCElement(parent, pInfo, false);
            SPS->sps_disp.frame_crop_rect_top_offset = h264_GetVLCElement(parent, pInfo, false);
            SPS->sps_disp.frame_crop_rect_bottom_offset = h264_GetVLCElement(parent, pInfo, false);
        }

        //// when frame_mbs_only_flag is equal to 0, direct_8x8_inference_flag shall be equal to 1
        if (SPS->sps_disp.frame_mbs_only_flag == 0 && SPS->sps_disp.direct_8x8_inference_flag == 0) {
            break;
        }

        ////// vui_parameters
        if (viddec_pm_get_bits(parent, &code, 1) == -1)
            break;
        SPS->sps_disp.vui_parameters_present_flag = (uint8_t)code;
        ret = H264_STATUS_OK;

        if (SPS->sps_disp.vui_parameters_present_flag)
        {
#ifndef VBP
            ret = h264_Parse_Vui_Parameters(parent, pInfo, SPS, pVUI_Seq_Not_Used);
#else
            // Ignore VUI parsing result
            h264_Parse_Vui_Parameters(parent, pInfo, SPS, pVUI_Seq_Not_Used);
            if (SPS->sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag)
            {
                i = SPS->sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1;
                uint32_t bit_rate_value = 0;
                bit_rate_value = pVUI_Seq_Not_Used->nal_hrd_parameters.bit_rate_value_minus1[i] + 1;
                bit_rate_value *= pow(2, 6 + pVUI_Seq_Not_Used->nal_hrd_bit_rate_scale);
                SPS->sps_disp.vui_seq_parameters.bit_rate_value = bit_rate_value;
            }
            /*
            else if (SPS->sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
            {
                i = SPS->sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1;
                uint32_t bit_rate_value = 0;
                bit_rate_value = pVUI_Seq_Not_Used->vcl_hrd_parameters.bit_rate_value_minus1[i] + 1;
                bit_rate_value *= pow(2, 6 + pVUI_Seq_Not_Used->vcl_hrd_bit_rate_scale);
                SPS->sps_disp.vui_seq_parameters.bit_rate_value = bit_rate_value;
             }*/

#endif
        }
    } while (0);
#ifdef VBP
    if (SPS->sps_disp.vui_seq_parameters.bit_rate_value == 0)
    {
        int maxBR = 0;
        switch(SPS->level_idc)
        {
        case h264_Level1:
            maxBR = 64;
            break;

        case h264_Level1b:
            maxBR = 128;
            break;

        case h264_Level11:
            maxBR = 192;
            break;

        case h264_Level12:
            maxBR = 384;
            break;

        case h264_Level13:
            maxBR = 768;
            break;

        case h264_Level2:
            maxBR = 2000;
            break;

        case h264_Level21:
        case h264_Level22:
            maxBR = 4000;
            break;

        case h264_Level3:
            maxBR = 10000;
            break;

        case h264_Level31:
            maxBR = 14000;
            break;

        case h264_Level32:
        case h264_Level4:
            maxBR = 20000;
            break;

        case h264_Level41:
        case h264_Level42:
            maxBR = 50000;
            break;

        case h264_Level5:
            maxBR = 135000;
            break;

        case h264_Level51:
            maxBR = 240000;
            break;
        }

        uint32_t cpbBrVclFactor = 1200;
        if (SPS->profile_idc == 100)
        {
            cpbBrVclFactor = 1500; // HIGH
        }
        else if (SPS->profile_idc == 110)
        {
            cpbBrVclFactor = 3600; // HIGH 10
        }
        else if (SPS->profile_idc == 122 ||
                 SPS->profile_idc == 144)
        {
            cpbBrVclFactor = 4800; // HIGH 4:2:2 and HIGH 4:4:4
        }

        SPS->sps_disp.vui_seq_parameters.bit_rate_value = maxBR *  cpbBrVclFactor;
    }
#endif

    //h264_Parse_rbsp_trailing_bits(pInfo);

    return ret;
}

//#endif

