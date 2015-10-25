

#include "h264.h"
#include "h264parse.h"

/*---------------------------------------------*/
/*---------------------------------------------*/
/*---------------------------------------------*/
h264_Status h264_Parse_PicParameterSet(void *parent,h264_Info * pInfo,h264_PicParameterSet_t* PictureParameterSet)
{
    h264_Status ret = H264_PPS_ERROR;

    //h264_PicParameterSet_t* PictureParameterSet = &pInfo->PictureParameterSet;
    uint32_t code=0, i = 0;

    do {
        ///// PPS par1: pic_parameter_set_id & seq_parameter_set_id
        code = h264_GetVLCElement(parent, pInfo, false);
        if (code > MAX_PIC_PARAMS) {
            break;
        }
        PictureParameterSet->pic_parameter_set_id = (uint8_t)code;

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (code > 255)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif

        code = h264_GetVLCElement(parent, pInfo, false);
        if (code > MAX_NUM_SPS-1) {
            break;
        }
        PictureParameterSet->seq_parameter_set_id = (uint8_t)code;

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (code > 31)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        ///// entropy_coding_mode_flag
        viddec_pm_get_bits(parent, &code, 1);
        PictureParameterSet->entropy_coding_mode_flag = (uint8_t)code;
        ///// pic_order_present_flag
        viddec_pm_get_bits(parent, &code, 1);
        PictureParameterSet->pic_order_present_flag = (uint8_t)code;

        PictureParameterSet->num_slice_groups_minus1 = h264_GetVLCElement(parent, pInfo, false);

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (PictureParameterSet->num_slice_groups_minus1 > 8)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        //
        // In main profile, FMO is excluded and num_slice_groups_minus1 should be 0
        //
        if (PictureParameterSet->num_slice_groups_minus1 > 0) //MAX_NUM_SLICE_GRPS)
            break;

        PictureParameterSet->num_ref_idx_l0_active = h264_GetVLCElement(parent, pInfo, false)+1;
        PictureParameterSet->num_ref_idx_l1_active = h264_GetVLCElement(parent, pInfo, false)+1;

        //// PPS->num_ref_idx_l0_active --- [0,32]
        if (((PictureParameterSet->num_ref_idx_l0_active) > MAX_NUM_REF_FRAMES) || ((PictureParameterSet->num_ref_idx_l1_active) > MAX_NUM_REF_FRAMES))
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }

        //// weighting prediction
        viddec_pm_get_bits(parent, &code, 1);
        PictureParameterSet->weighted_pred_flag = (uint8_t)code;

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (code > 2)
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        viddec_pm_get_bits(parent, &code, 2);
        PictureParameterSet->weighted_bipred_idc = (uint8_t)code;

        //// QP
        PictureParameterSet->pic_init_qp_minus26 = h264_GetVLCElement(parent, pInfo, true);
        PictureParameterSet->pic_init_qs_minus26 = h264_GetVLCElement(parent, pInfo, true);
        if (((PictureParameterSet->pic_init_qp_minus26+26) > MAX_QP) || ((PictureParameterSet->pic_init_qs_minus26+26) > MAX_QP))
        {
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            pInfo->sw_bail = 1;
#endif
#endif
            break;
        }
        PictureParameterSet->chroma_qp_index_offset = h264_GetVLCElement(parent, pInfo, true);

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if ((12 < PictureParameterSet->chroma_qp_index_offset) || (-12 > PictureParameterSet->chroma_qp_index_offset) )
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        //// Deblocking ctl parameters
        viddec_pm_get_bits(parent, &code, 1);
        PictureParameterSet->deblocking_filter_control_present_flag = (uint8_t)code;

        viddec_pm_get_bits(parent, &code, 1);
        PictureParameterSet->constrained_intra_pred_flag = (uint8_t)code;

        if ( viddec_pm_get_bits(parent, &code, 1) == -1)
            break;
        PictureParameterSet->redundant_pic_cnt_present_flag = (uint8_t)code;

#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        if (code && (pInfo->active_SPS.profile_idc != h264_ProfileBaseline))
        {
            pInfo->sw_bail = 1;
        }
#endif
#endif
        //// Check if have more RBSP Data for additional parameters
        if (h264_More_RBSP_Data(parent, pInfo))
        {
            viddec_pm_get_bits(parent,  &code, 1);
            PictureParameterSet->transform_8x8_mode_flag = (uint8_t)code;

            if ( viddec_pm_get_bits(parent, &code, 1) == -1)
                break;
            PictureParameterSet->pic_scaling_matrix_present_flag = (uint8_t)code;

            if (PictureParameterSet->pic_scaling_matrix_present_flag)
            {
                uint32_t n_ScalingList = 6 + (PictureParameterSet->transform_8x8_mode_flag << 1);
                for (i=0; i<n_ScalingList; i++)
                {
                    if ( viddec_pm_get_bits(parent, &code, 1) == -1)
                        break;
                    PictureParameterSet->pic_scaling_list_present_flag[i] = (uint8_t)code;

                    if (PictureParameterSet->pic_scaling_list_present_flag[i])
                    {
                        if (i<6)
                            h264_Scaling_List(parent, PictureParameterSet->ScalingList4x4[i], 16, &PictureParameterSet->UseDefaultScalingMatrix4x4Flag[i], pInfo);
                        else
                            h264_Scaling_List(parent, PictureParameterSet->ScalingList8x8[i-6], 64, &PictureParameterSet->UseDefaultScalingMatrix8x8Flag[i-6], pInfo);
                    }
                }
            }

            PictureParameterSet->second_chroma_qp_index_offset = h264_GetVLCElement(parent, pInfo, true); //fix
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if((PictureParameterSet->second_chroma_qp_index_offset>12) || (PictureParameterSet->second_chroma_qp_index_offset < -12))
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
        }
        else
        {
            PictureParameterSet->transform_8x8_mode_flag = 0;
            PictureParameterSet->pic_scaling_matrix_present_flag = 0;
            PictureParameterSet->second_chroma_qp_index_offset = PictureParameterSet->chroma_qp_index_offset;
        }

        ret = H264_STATUS_OK;
    } while (0);

    //h264_Parse_rbsp_trailing_bits(pInfo);
    return ret;
}

////////// EOF///////////////

