#define H264_PARSE_SEI_C

#ifdef H264_PARSE_SEI_C

#include "h264.h"
#include "h264parse.h"
#include "h264parse_dpb.h"

#include "viddec_parser_ops.h"

#include "viddec_fw_item_types.h"
#include "viddec_fw_workload.h"

//////////////////////////////////////////////////////////////////////////////
// avc_sei_stream_initialise ()
//
//

void h264_sei_stream_initialise (h264_Info* pInfo)
{
    pInfo->sei_information.capture_POC     = 0;
    pInfo->sei_information.disp_frozen     = 0;
    pInfo->sei_information.release_POC     = 0;
    pInfo->sei_information.capture_fn      = 0;
    pInfo->sei_information.recovery_fn     = 0xFFFFFFFF;
    pInfo->sei_information.scan_format     = 0;
    pInfo->sei_information.broken_link_pic = 0;
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_buffering_period(void *parent,h264_Info* pInfo)
{
    h264_Status ret = H264_STATUS_SEI_ERROR;

    h264_SEI_buffering_period_t* sei_msg_ptr;
    h264_SEI_buffering_period_t  sei_buffering_period;
    int32_t SchedSelIdx;
    int num_bits = 0;

    sei_msg_ptr = (h264_SEI_buffering_period_t *)(&sei_buffering_period);

    do {
        if (pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag == 1)
        {
            num_bits = pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_initial_cpb_removal_delay_length_minus1 + 1;
        }
        else if (pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
        {
            num_bits = pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_initial_cpb_removal_delay_length_minus1 + 1;
        }

        sei_msg_ptr->seq_param_set_id = h264_GetVLCElement(parent, pInfo, false);
        if (sei_msg_ptr->seq_param_set_id >= NUM_SPS)
            break;

        //check if this id is same as the id of the current SPS  //fix

        if (pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag == 1)
        {
            if (pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1 >= MAX_CPB_CNT)
                break;

            for (SchedSelIdx = 0; SchedSelIdx <= pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_cpb_cnt_minus1; SchedSelIdx++)
            {
                viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->initial_cpb_removal_delay_nal, num_bits);
                viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->initial_cpb_removal_delay_offset_nal, num_bits);
            }
        }

        if (pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag == 1)
        {
            if (pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1 >= MAX_CPB_CNT)
                break;

            for (SchedSelIdx = 0; SchedSelIdx <= pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_cpb_cnt_minus1; SchedSelIdx++)
            {
                viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->initial_cpb_removal_delay_vcl, num_bits);
                viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->initial_cpb_removal_delay_offset_vcl, num_bits);
            }
        }

        ret = H264_STATUS_OK;
    } while (0);

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_pic_timing(void *parent,h264_Info* pInfo)
{
    int32_t CpbDpbDelaysPresentFlag = 0;
    h264_SEI_pic_timing_t* sei_msg_ptr;
    h264_SEI_pic_timing_t  sei_pic_timing;
    int32_t num_bits_cpb = 0, num_bits_dpb = 0, time_offset_length = 0;
    uint32_t code;
    uint32_t clock_timestamp_flag = 0;
    uint32_t full_timestamp_flag = 0;
    uint32_t seconds_flag = 0;
    uint32_t minutes_flag = 0;
    uint32_t hours_flag = 0;
    uint32_t time_offset = 0;




    sei_msg_ptr = (h264_SEI_pic_timing_t *)(&sei_pic_timing);

    if (pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag)
    {
        num_bits_cpb = pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_cpb_removal_delay_length_minus1 +1;
        num_bits_dpb = pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_dpb_output_delay_length_minus1 + 1;
        time_offset_length = pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_time_offset_length;
    }
    else if (pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag)
    {
        num_bits_cpb = pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_cpb_removal_delay_length_minus1 +1;
        num_bits_dpb = pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_dpb_output_delay_length_minus1 + 1;
    }


    CpbDpbDelaysPresentFlag = 1;		// as per amphion code
    if (CpbDpbDelaysPresentFlag)
    {
        viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->cpb_removal_delay, num_bits_cpb);
        viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->dpb_output_delay, num_bits_dpb);
    }

    if (pInfo->active_SPS.sps_disp.vui_seq_parameters.pic_struct_present_flag)
    {
        int32_t i = 0, NumClockTS = 0;

        viddec_workload_item_t     wi;

        wi.vwi_payload[0] = wi.vwi_payload[1] = wi.vwi_payload[2] = 0;
        viddec_pm_get_bits(parent, &code , 4);
        sei_msg_ptr->pic_struct = (uint8_t)code;


        if ((sei_msg_ptr->pic_struct == 0) || (sei_msg_ptr->pic_struct == 7) || (sei_msg_ptr->pic_struct == 8)) {
            pInfo->sei_information.scan_format = SEI_SCAN_FORMAT_PROGRESSIVE;
        } else {
            pInfo->sei_information.scan_format = SEI_SCAN_FORMAT_INTERLACED;
        }

        wi.vwi_type = VIDDEC_WORKLOAD_SEI_PIC_TIMING;
        wi.h264_sei_pic_timing.pic_struct = sei_msg_ptr->pic_struct;

#ifndef VBP
        //Push to current if we are in first frame, or we do not detect previous frame end
        viddec_pm_append_workitem( parent, &wi , !(pInfo->Is_first_frame_in_stream ||(!pInfo->is_current_workload_done)));
#endif

        if (sei_msg_ptr->pic_struct < 3) {
            NumClockTS = 1;
        } else if ((sei_msg_ptr->pic_struct < 5) || (sei_msg_ptr->pic_struct == 7)) {
            NumClockTS = 2;
        } else {
            NumClockTS = 3;
        }

        for (i = 0; i < NumClockTS; i++)
        {
            viddec_pm_get_bits(parent, &code , 1);
            clock_timestamp_flag = code;
            //sei_msg_ptr->clock_timestamp_flag[i] = (uint8_t)code;

            if (clock_timestamp_flag)
            {
                viddec_pm_get_bits(parent, &code , 2);
                //sei_msg_ptr->ct_type[i] = (uint8_t)code;

                viddec_pm_get_bits(parent, &code , 1);
                //sei_msg_ptr->nuit_field_based_flag[i] = (uint8_t)code;

                viddec_pm_get_bits(parent, &code , 5);
                //sei_msg_ptr->counting_type[i] = (uint8_t)code;

                viddec_pm_get_bits(parent, &code , 1);
                //sei_msg_ptr->full_timestamp_flag[i] = (uint8_t)code;
                full_timestamp_flag = code;

                viddec_pm_get_bits(parent, &code , 1);
                //sei_msg_ptr->discontinuity_flag[i] = (uint8_t)code;

                viddec_pm_get_bits(parent, &code , 1);
                //sei_msg_ptr->cnt_dropped_flag[i] = (uint8_t)code;

                viddec_pm_get_bits(parent, &code , 8);
                //sei_msg_ptr->n_frames[i] = (uint8_t)code;


                if (full_timestamp_flag)
                {
                    viddec_pm_get_bits(parent, &code , 6);
                    //sei_msg_ptr->seconds_value[i] = (uint8_t)code;

                    viddec_pm_get_bits(parent, &code , 6);
                    //sei_msg_ptr->minutes_value[i] = (uint8_t)code;

                    viddec_pm_get_bits(parent, &code , 5);
                    //sei_msg_ptr->hours_value[i] = (uint8_t)code;
                }
                else
                {
                    viddec_pm_get_bits(parent, &code , 1);
                    //sei_msg_ptr->seconds_flag[i] = (uint8_t)code;
                    seconds_flag = code;

                    if (seconds_flag)
                    {
                        viddec_pm_get_bits(parent, &code , 6);
                        //sei_msg_ptr->seconds_value[i] = (uint8_t)code;

                        viddec_pm_get_bits(parent, &code , 1);
                        //sei_msg_ptr->minutes_flag[i] = (uint8_t)code;
                        minutes_flag = code;

                        if (minutes_flag)
                        {
                            viddec_pm_get_bits(parent, &code , 6);
                            //sei_msg_ptr->minutes_value[i] = (uint8_t)code;

                            viddec_pm_get_bits(parent, &code , 1);
                            //sei_msg_ptr->hours_flag[i] = (uint8_t)code;
                            hours_flag = code;

                            if (hours_flag) {
                                viddec_pm_get_bits(parent, &code , 6);
                                //sei_msg_ptr->hours_value[i] = (uint8_t)code;
                            }
                        }
                    }
                }

                if (time_offset_length > 0)
                {
                    viddec_pm_get_bits(parent, (uint32_t *)&time_offset, time_offset_length);
                }
            }
        }
    }


    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_pan_scan(void *parent,h264_Info* pInfo)
{
    h264_SEI_pan_scan_rectangle_t* sei_msg_ptr;
    h264_SEI_pan_scan_rectangle_t  sei_pan_scan;
    uint32_t code;

    viddec_workload_item_t     wi;

    h264_memset( &(sei_pan_scan), 0x0, sizeof(h264_SEI_pan_scan_rectangle_t) );

    viddec_fw_reset_workload_item(&wi);
    wi.vwi_type = VIDDEC_WORKLOAD_H264_PAN_SCAN;

    sei_msg_ptr = (h264_SEI_pan_scan_rectangle_t *)(&sei_pan_scan);

    sei_msg_ptr->pan_scan_rect_id = h264_GetVLCElement(parent, pInfo, false);

    wi.h264_sei_pan_scan.pan_scan_rect_id = sei_msg_ptr->pan_scan_rect_id;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->pan_scan_rect_cancel_flag = (uint8_t)code;
    viddec_fw_h264_sei_pan_scan_set_cancel_flag(&(wi.h264_sei_pan_scan), sei_msg_ptr->pan_scan_rect_cancel_flag);

    if (!sei_msg_ptr->pan_scan_rect_cancel_flag)
    {
        int32_t i;
        sei_msg_ptr->pan_scan_cnt_minus1 = h264_GetVLCElement(parent, pInfo, false);

        viddec_fw_h264_sei_pan_scan_set_cnt_minus1(&(wi.h264_sei_pan_scan), sei_msg_ptr->pan_scan_cnt_minus1);
        if (sei_msg_ptr->pan_scan_cnt_minus1 > MAX_PAN_SCAN_CNT -1)
        {
            return H264_STATUS_SEI_ERROR;
        }
        for (i=0; i<= sei_msg_ptr->pan_scan_cnt_minus1; i++)
        {
            sei_msg_ptr->pan_scan_rect_left_offset[i] = h264_GetVLCElement(parent, pInfo, true);
            sei_msg_ptr->pan_scan_rect_right_offset[i] = h264_GetVLCElement(parent, pInfo, true);
            sei_msg_ptr->pan_scan_rect_top_offset[i] = h264_GetVLCElement(parent, pInfo, true);
            sei_msg_ptr->pan_scan_rect_bottom_offset[i] = h264_GetVLCElement(parent, pInfo, true);
        }
        sei_msg_ptr->pan_scan_rect_repetition_period = h264_GetVLCElement(parent, pInfo, false);
        wi.h264_sei_pan_scan.pan_scan_rect_repetition_period = sei_msg_ptr->pan_scan_rect_repetition_period;
    }
#ifndef VBP
    //cur is first frame
    viddec_pm_append_workitem( parent, &wi , !(pInfo->Is_first_frame_in_stream ||(!pInfo->is_current_workload_done)));
#endif

    if (!sei_msg_ptr->pan_scan_rect_cancel_flag)
    {
        int32_t i;

        viddec_fw_reset_workload_item(&wi);
        wi.vwi_type = VIDDEC_WORKLOAD_SEI_PAN_SCAN_RECT;

        for (i=0; i<= sei_msg_ptr->pan_scan_cnt_minus1; i++)
        {
            viddec_fw_h264_pan_scan_set_left(&(wi.h264_pan_scan_rect), sei_msg_ptr->pan_scan_rect_left_offset[i]);
            viddec_fw_h264_pan_scan_set_right(&(wi.h264_pan_scan_rect), sei_msg_ptr->pan_scan_rect_right_offset[i]);
            viddec_fw_h264_pan_scan_set_top(&(wi.h264_pan_scan_rect), sei_msg_ptr->pan_scan_rect_top_offset[i]);
            viddec_fw_h264_pan_scan_set_bottom(&(wi.h264_pan_scan_rect), sei_msg_ptr->pan_scan_rect_bottom_offset[i]);
#ifndef VBP
            //cur is first frame
            viddec_pm_append_workitem( parent, &wi , !pInfo->Is_first_frame_in_stream);
#endif
        }
    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_filler_payload(void *parent,h264_Info* pInfo, uint32_t payload_size)
{

    h264_SEI_filler_payload_t* sei_msg_ptr;
    h264_SEI_filler_payload_t sei_filler_payload;
    uint32_t k;
    uint32_t code;

    //remove warning
    pInfo = pInfo;

    sei_msg_ptr = (h264_SEI_filler_payload_t *)(&sei_filler_payload);
    for (k=0; k < payload_size; k++)
    {
        viddec_pm_get_bits(parent, &code , 8);
        sei_msg_ptr->ff_byte = (uint8_t)code;
    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_userdata_reg(void *parent,h264_Info* pInfo, uint32_t payload_size)
{

    h264_SEI_userdata_registered_t* sei_msg_ptr;
    h264_SEI_userdata_registered_t  sei_userdata_registered;
    uint32_t i;
    int32_t byte = 0;
    uint32_t code = 0;
    viddec_workload_item_t     wi;

    wi.vwi_type = VIDDEC_WORKLOAD_SEI_USER_DATA_REGISTERED;
    wi.vwi_payload[0] = wi.vwi_payload[1] = wi.vwi_payload[2] = 0;
    //remove warning
    pInfo = pInfo;

    sei_msg_ptr = (h264_SEI_userdata_registered_t *)(&sei_userdata_registered);

    viddec_pm_get_bits(parent, &code , 8);
    sei_msg_ptr->itu_t_t35_country_code = (uint8_t)code;

    if (sei_msg_ptr->itu_t_t35_country_code != 0xff)	{
        i = 1;
    } else {
        viddec_pm_get_bits(parent, &code , 8);
        sei_msg_ptr->itu_t_t35_country_code_extension_byte = (uint8_t)code;
        i = 2;
    }


    wi.user_data.size =0;
    do
    {

        viddec_pm_get_bits(parent, (uint32_t *)&byte, 8);
        if (wi.user_data.size < 11)
        {
            wi.user_data.data_payload[wi.user_data.size]=(uint8_t)byte;
        }
        wi.user_data.size++;

        if (11 == wi.user_data.size)
        {
            viddec_pm_setup_userdata(&wi);
#ifndef VBP
            //cur is first frame
            viddec_pm_append_workitem( parent, &wi , !(pInfo->Is_first_frame_in_stream ||(!pInfo->is_current_workload_done)));
#endif
            wi.user_data.size =0;
        }

        i++;
    } while (i < payload_size);

    if (0!=wi.user_data.size)
    {
        viddec_pm_setup_userdata(&wi);
#ifndef VBP
        //cur is first frame
        viddec_pm_append_workitem( parent, &wi , !pInfo->Is_first_frame_in_stream);
#endif
    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_userdata_unreg(void *parent, h264_Info* pInfo, uint32_t payload_size)
{

    h264_SEI_userdata_unregistered_t* sei_msg_ptr;
    h264_SEI_userdata_unregistered_t  sei_userdata_unregistered;
    uint32_t i;
    int32_t byte = 0;
    uint32_t code;

    viddec_workload_item_t     wi;

    wi.vwi_type = VIDDEC_WORKLOAD_SEI_USER_DATA_UNREGISTERED;

    //remove warning
    pInfo = pInfo;

    sei_msg_ptr = (h264_SEI_userdata_unregistered_t *)(&sei_userdata_unregistered);

    for (i = 0; i < 4; i++)
    {
        viddec_pm_get_bits(parent, &code , 32);
        sei_msg_ptr->uuid_iso_iec_11578[i] = (uint8_t)code;
    }

    wi.user_data.size =0;
    for (i = 16; i < payload_size; i++)
    {

        viddec_pm_get_bits(parent, (uint32_t *)&byte, 8);
        if (wi.user_data.size < 11)
        {
            wi.user_data.data_payload[wi.user_data.size]=(uint8_t)byte;
        }
        wi.user_data.size++;

        if (11 == wi.user_data.size)
        {
            viddec_pm_setup_userdata(&wi);
#ifndef VBP
            //cur is first frame
            viddec_pm_append_workitem( parent, &wi , !(pInfo->Is_first_frame_in_stream ||(!pInfo->is_current_workload_done)));
#endif
            wi.user_data.size =0;
        }
    }

    if (0!=wi.user_data.size)
    {
        viddec_pm_setup_userdata(&wi);
#ifndef VBP
        //cur is first frame
        viddec_pm_append_workitem( parent, &wi , !pInfo->Is_first_frame_in_stream);
#endif
    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_recovery_point(void *parent, h264_Info* pInfo)
{

    h264_SEI_recovery_point_t* sei_msg_ptr;
    h264_SEI_recovery_point_t  sei_recovery_point;
    uint32_t code;
    viddec_workload_item_t     wi;


    sei_msg_ptr = (h264_SEI_recovery_point_t *)(&sei_recovery_point);

    sei_msg_ptr->recovery_frame_cnt = h264_GetVLCElement(parent, pInfo, false);

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->exact_match_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->broken_link_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 2);
    sei_msg_ptr->changing_slice_group_idc = (uint8_t)code;

    pInfo->sei_information.recovery_point = 1;
    pInfo->sei_information.recovery_frame_cnt = (int32_t) sei_msg_ptr->recovery_frame_cnt;
    pInfo->sei_information.capture_fn         = 1;
    pInfo->sei_information.broken_link_pic    = sei_msg_ptr->broken_link_flag;

    if (pInfo->got_start)	{
        pInfo->img.recovery_point_found |= 2;

        //// Enable the RP recovery if no IDR ---Cisco
        if ((pInfo->img.recovery_point_found & 1)==0)
            pInfo->sei_rp_received = 1;
    }

    //
    /// Append workload for SEI
    //
    viddec_fw_reset_workload_item(&wi);
    wi.vwi_type = VIDDEC_WORKLOAD_SEI_RECOVERY_POINT;
    wi.h264_sei_recovery_point.recovery_frame_cnt = sei_msg_ptr->recovery_frame_cnt;
    viddec_fw_h264_h264_sei_recovery_set_exact_match_flag(&(wi.h264_sei_recovery_point), sei_msg_ptr->exact_match_flag);
    viddec_fw_h264_h264_sei_recovery_set_broken_link_flag(&(wi.h264_sei_recovery_point), sei_msg_ptr->broken_link_flag);
    wi.h264_sei_recovery_point.changing_slice_group_idc = sei_msg_ptr->changing_slice_group_idc;
#ifndef VBP
    //cur is first frame
    viddec_pm_append_workitem( parent, &wi , !(pInfo->Is_first_frame_in_stream ||(!pInfo->is_current_workload_done)));
#endif

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_dec_ref_pic_marking_rep(void *parent,h264_Info* pInfo)
{

    h264_SEI_decoded_ref_pic_marking_repetition_t* sei_msg_ptr;
    h264_SEI_decoded_ref_pic_marking_repetition_t  sei_ref_pic;
    uint32_t code;

    sei_msg_ptr = (h264_SEI_decoded_ref_pic_marking_repetition_t *)(&sei_ref_pic);

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->original_idr_flag = (uint8_t)code;

    sei_msg_ptr->original_frame_num = h264_GetVLCElement(parent, pInfo, false);

    if (!(pInfo->active_SPS.sps_disp.frame_mbs_only_flag))
    {
        viddec_pm_get_bits(parent, &code , 1);
        sei_msg_ptr->orignal_field_pic_flag = (uint8_t)code;

        if (sei_msg_ptr->orignal_field_pic_flag)
        {
            viddec_pm_get_bits(parent, &code , 1);
            sei_msg_ptr->original_bottom_field_pic_flag = (uint8_t)code;
        }
    }
    h264_Parse_Dec_Ref_Pic_Marking(parent, pInfo, &pInfo->SliceHeader);
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_spare_pic(void *parent,h264_Info* pInfo)
{

    //h264_SEI_spare_picture_t* sei_msg_ptr;

    //remove warning
    pInfo = pInfo;
    parent = parent;

    //sei_msg_ptr = (h264_SEI_spare_picture_t *)(&user_data->user_data[0]);

    //OS_INFO("Not supported SEI\n");
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_scene_info(void *parent,h264_Info* pInfo)
{

    h264_SEI_scene_info_t* sei_msg_ptr;
    h264_SEI_scene_info_t  sei_scene_info;
    uint32_t code;

    sei_msg_ptr = (h264_SEI_scene_info_t*)(&sei_scene_info);

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->scene_info_present_flag = (uint8_t)code;

    if (sei_msg_ptr->scene_info_present_flag)
    {
        sei_msg_ptr->scene_id = h264_GetVLCElement(parent, pInfo, false);
        sei_msg_ptr->scene_transitioning_type= h264_GetVLCElement(parent, pInfo, false);
        if (sei_msg_ptr->scene_transitioning_type > 3)
        {
            sei_msg_ptr->second_scene_id = h264_GetVLCElement(parent, pInfo, false);
        }
    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_sub_seq_info(void *parent,h264_Info* pInfo)
{

    h264_SEI_sub_sequence_info_t* sei_msg_ptr;
    h264_SEI_sub_sequence_info_t  sei_sub_sequence_info;
    uint32_t code;

    sei_msg_ptr = (h264_SEI_sub_sequence_info_t *)(&sei_sub_sequence_info);

    sei_msg_ptr->sub_seq_layer_num = h264_GetVLCElement(parent, pInfo,false);
    sei_msg_ptr->sub_seq_id= h264_GetVLCElement(parent, pInfo,false);

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->first_ref_pic_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->leading_non_ref_pic_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->last_pic_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->sub_seq_frame_num_flag = (uint8_t)code;


    if (sei_msg_ptr->sub_seq_frame_num_flag)
    {
        sei_msg_ptr->sub_seq_frame_num = h264_GetVLCElement(parent, pInfo,false);
    }
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_sub_seq_layer(void *parent,h264_Info* pInfo)
{

    h264_SEI_sub_sequence_layer_t* sei_msg_ptr;
    h264_SEI_sub_sequence_layer_t  sei_sub_sequence_layer;
    int32_t layer;
    uint32_t code;

    sei_msg_ptr = (h264_SEI_sub_sequence_layer_t *)(&sei_sub_sequence_layer);
    sei_msg_ptr->num_sub_seq_layers_minus1 = h264_GetVLCElement(parent, pInfo,false);

    if (sei_msg_ptr->num_sub_seq_layers_minus1 >= MAX_SUB_SEQ_LAYERS)
    {
        return H264_STATUS_SEI_ERROR;
    }

    for (layer = 0; layer <= sei_msg_ptr->num_sub_seq_layers_minus1; layer++)
    {
        viddec_pm_get_bits(parent, &code , 1);
        sei_msg_ptr->accurate_statistics_flag[layer] = (uint8_t)code;

        viddec_pm_get_bits(parent, &code , 16);
        sei_msg_ptr->average_bit_rate[layer] = (uint16_t)code;

        viddec_pm_get_bits(parent, &code , 16);
        sei_msg_ptr->average_frame_rate[layer] = (uint16_t)code;

    }

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_sub_seq(void *parent,h264_Info* pInfo)
{
    int32_t n;
    uint32_t code;

    h264_SEI_sub_sequence_t* sei_msg_ptr;
    h264_SEI_sub_sequence_t  sei_sub_sequence;

    sei_msg_ptr = (h264_SEI_sub_sequence_t *)(&sei_sub_sequence);

    sei_msg_ptr->sub_seq_layer_num = h264_GetVLCElement(parent, pInfo, false);
    sei_msg_ptr->sub_seq_id= h264_GetVLCElement(parent, pInfo, false);

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->duration_flag = (uint8_t)code;

    if (sei_msg_ptr->duration_flag)
    {
        viddec_pm_get_bits(parent, (uint32_t *)&sei_msg_ptr->sub_seq_duration, 32);
    }

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->average_rate_flag = (uint8_t)code;

    if (sei_msg_ptr->average_rate_flag)
    {
        viddec_pm_get_bits(parent, &code , 1);
        sei_msg_ptr->average_statistics_flag = (uint8_t)code;

        viddec_pm_get_bits(parent, &code , 16);
        sei_msg_ptr->average_bit_rate = (uint8_t)code;

        viddec_pm_get_bits(parent, &code , 16);
        sei_msg_ptr->average_frame_rate = (uint8_t)code;

    }
    sei_msg_ptr->num_referenced_subseqs = h264_GetVLCElement(parent, pInfo, false);
    if (sei_msg_ptr->num_referenced_subseqs >= MAX_NUM_REF_SUBSEQS)
    {
        return H264_STATUS_SEI_ERROR;
    }

    for (n = 0; n < sei_msg_ptr->num_referenced_subseqs; n++)
    {
        sei_msg_ptr->ref_sub_seq_layer_num= h264_GetVLCElement(parent, pInfo, false);
        sei_msg_ptr->ref_sub_seq_id= h264_GetVLCElement(parent, pInfo, false);

        viddec_pm_get_bits(parent, &code , 1);
        sei_msg_ptr->ref_sub_seq_direction = (uint8_t)code;
    }
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_full_frame_freeze(void *parent,h264_Info* pInfo)
{

    h264_SEI_full_frame_freeze_t* sei_msg_ptr;
    h264_SEI_full_frame_freeze_t  sei_full_frame_freeze;

    sei_msg_ptr = (h264_SEI_full_frame_freeze_t *)(&sei_full_frame_freeze);

    sei_msg_ptr->full_frame_freeze_repetition_period= h264_GetVLCElement(parent, pInfo, false);

    pInfo->sei_information.capture_POC        = 1;
    pInfo->sei_information.freeze_rep_period  = sei_msg_ptr->full_frame_freeze_repetition_period;
    //pInfo->img.sei_freeze_this_image          = 1;

    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_full_frame_freeze_release(void *parent,h264_Info* pInfo)
{
    //remove warning
    parent = parent;
    pInfo = pInfo;


    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_full_frame_snapshot(void *parent,h264_Info* pInfo)
{

    h264_SEI_full_frame_snapshot_t* sei_msg_ptr;
    h264_SEI_full_frame_snapshot_t  sei_full_frame_snapshot;

    sei_msg_ptr = (h264_SEI_full_frame_snapshot_t *)(&sei_full_frame_snapshot);

    sei_msg_ptr->snapshot_id = h264_GetVLCElement(parent, pInfo, false);
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_progressive_segement_start(void *parent,h264_Info* pInfo)
{

    h264_SEI_progressive_segment_start_t* sei_msg_ptr;
    h264_SEI_progressive_segment_start_t  sei_progressive_segment_start;

    sei_msg_ptr = (h264_SEI_progressive_segment_start_t *)(&sei_progressive_segment_start);

    sei_msg_ptr->progressive_refinement_id= h264_GetVLCElement(parent, pInfo, false);
    sei_msg_ptr->num_refinement_steps_minus1= h264_GetVLCElement(parent, pInfo, false);
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_progressive_segment_end(void *parent,h264_Info* pInfo)
{

    h264_SEI_progressive_segment_end_t* sei_msg_ptr;
    h264_SEI_progressive_segment_end_t  sei_progressive_segment_end;

    sei_msg_ptr = (h264_SEI_progressive_segment_end_t *)(&sei_progressive_segment_end);

    sei_msg_ptr->progressive_refinement_id = h264_GetVLCElement(parent, pInfo, false);
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_motion_constrained_slice_grp_set(void *parent, h264_Info* pInfo)
{
    int32_t i;
    uint32_t code;
    h264_SEI_motion_constrained_slice_group_t* sei_msg_ptr;
    h264_SEI_motion_constrained_slice_group_t  sei_motion_constrained_slice_group;

    sei_msg_ptr = (h264_SEI_motion_constrained_slice_group_t *)(&sei_motion_constrained_slice_group);

    sei_msg_ptr->num_slice_groups_in_set_minus1= h264_GetVLCElement(parent, pInfo, false);
    if (sei_msg_ptr->num_slice_groups_in_set_minus1 >= MAX_NUM_SLICE_GRPS)
    {
        return H264_STATUS_SEI_ERROR;
    }

    for (i=0; i<= sei_msg_ptr->num_slice_groups_in_set_minus1; i++)
    {
        viddec_pm_get_bits(parent, &code , 1);
        sei_msg_ptr->slice_group_id[i] = (uint8_t)code;
    }
    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->exact_sample_value_match_flag = (uint8_t)code;

    viddec_pm_get_bits(parent, &code , 1);
    sei_msg_ptr->pan_scan_rect_flag = (uint8_t)code;


    if (sei_msg_ptr->pan_scan_rect_flag)
    {
        sei_msg_ptr->pan_scan_rect_id= h264_GetVLCElement(parent, pInfo, false);
    }
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_film_grain_characteristics(void *parent,h264_Info* pInfo)
{
    //OS_INFO("Not supported SEI\n");

    //remove warning
    parent = parent;
    pInfo = pInfo;




    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_deblocking_filter_display_preferences(void *parent,h264_Info* pInfo)
{

    //h264_SEI_deblocking_filter_display_pref_t* sei_msg_ptr;

    //remove warning
    parent = parent;
    pInfo = pInfo;

    //sei_msg_ptr = (h264_SEI_deblocking_filter_display_pref_t *)(&user_data->user_data[0]);

    //OS_INFO("Not supported SEI\n");
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_sei_stereo_video_info(void *parent,h264_Info* pInfo)
{

    //h264_SEI_stereo_video_info_t* sei_msg_ptr;

    //remove warning
    parent = parent;
    pInfo = pInfo;


    //sei_msg_ptr = (h264_SEI_stereo_video_info_t *)(&user_data->user_data[0]);

    //OS_INFO("Not supported SEI\n");
    return H264_STATUS_OK;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
uint32_t h264_sei_reserved_sei_message(void *parent, h264_Info* pInfo, uint32_t payload_size)
{
    int32_t k, byte_index, user_data_byte_index;
    uint32_t i;
    int32_t word, bits;
    uint32_t user_data;
    //h264_SEI_reserved_t* sei_msg_ptr;
    //h264_SEI_reserved_t  sei_reserved;

    //remove warning
    pInfo = pInfo;

    //sei_msg_ptr = (h264_SEI_reserved_t *)(&sei_reserved);

    byte_index = 0;
    word = 0;
    user_data_byte_index = 0x0;

    for (i = 0, k = 0; i < payload_size; i++)
    {
        if (byte_index == 0) word = 0;
        viddec_pm_get_bits(parent, (uint32_t *)&bits, 8);

        switch (byte_index)
        {
        case 1:
            word = (bits << 8) | word;
            break;
        case 2:
            word = (bits << 16) | word;
            break;
        case 3:
            word = (bits << 24) | word;
            break;
        default :
            word = bits;
            break;
        }

        if (byte_index == 3)
        {
            byte_index = 0;
            user_data = word;
            k++;
        }
        else
        {
            byte_index++;
        }

        user_data_byte_index++;
        if ( user_data_byte_index == MAX_USER_DATA_SIZE)
        {
            //user_data->user_data_size = user_data_byte_index;
            //sei_msg_ptr = (h264_SEI_reserved_t *)(&user_data->user_data[0]);
            byte_index = 0;
            word = 0;
            user_data_byte_index = 0x0;
        }
    }

    if (byte_index)
        user_data = word;

    //user_data->user_data_size = user_data_byte_index;

    return user_data_byte_index;

//	return H264_STATUS_OK;
}

////// TODO
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_SEI_payload(void *parent, h264_Info* pInfo, h264_sei_payloadtype payloadType, int32_t payloadSize)
{
    //int32_t bit_equal_to_zero;
    h264_Status status = H264_STATUS_OK;

    //removing warning
    payloadSize = payloadSize;

    switch (payloadType)
    {
    case SEI_BUF_PERIOD:
        status = h264_sei_buffering_period(parent, pInfo);
        break;
    case SEI_PIC_TIMING:
        status = h264_sei_pic_timing(parent, pInfo);
        break;
    case SEI_PAN_SCAN:
        status = h264_sei_pan_scan(parent, pInfo);
        break;
    case SEI_FILLER_PAYLOAD:
        status = h264_sei_filler_payload(parent, pInfo, payloadSize);
        break;
    case SEI_REG_USERDATA:
        status = h264_sei_userdata_reg(parent, pInfo, payloadSize);
        break;
    case SEI_UNREG_USERDATA:
        status = h264_sei_userdata_unreg(parent, pInfo, payloadSize);
        break;
    case SEI_RECOVERY_POINT:
        h264_sei_recovery_point(parent, pInfo);
        break;
    case SEI_DEC_REF_PIC_MARKING_REP:
        status = h264_sei_dec_ref_pic_marking_rep(parent, pInfo);
        break;
    case SEI_SPARE_PIC:
        status = h264_sei_spare_pic(parent, pInfo);
        break;
    case SEI_SCENE_INFO:
        status = h264_sei_scene_info(parent, pInfo);
        break;
    case SEI_SUB_SEQ_INFO:
        status = h264_sei_sub_seq_info(parent, pInfo);
        break;
    case SEI_SUB_SEQ_LAYER:
        status = h264_sei_sub_seq_layer(parent, pInfo);
        break;
    case SEI_SUB_SEQ:
        status = h264_sei_sub_seq(parent, pInfo);
        break;
    case SEI_FULL_FRAME_FREEZE:
        status = h264_sei_full_frame_freeze(parent, pInfo);
        break;
    case SEI_FULL_FRAME_FREEZE_RELEASE:
        h264_sei_full_frame_freeze_release(parent, pInfo);
        break;
    case SEI_FULL_FRAME_SNAPSHOT:
        status = h264_sei_full_frame_snapshot(parent, pInfo);
        break;
    case SEI_PROGRESSIVE_SEGMENT_START:
        status = h264_sei_progressive_segement_start(parent, pInfo);
        break;
    case SEI_PROGRESSIVE_SEGMENT_END:
        status = h264_sei_progressive_segment_end(parent, pInfo);
        break;
    case SEI_MOTION_CONSTRAINED_SLICE_GRP_SET:
        status = h264_sei_motion_constrained_slice_grp_set(parent, pInfo);
        break;
    case SEI_FILM_GRAIN_CHARACTERISTICS:
        status = h264_sei_film_grain_characteristics(parent, pInfo);
        break;
    case SEI_DEBLK_FILTER_DISPLAY_PREFERENCE:
        status = h264_sei_deblocking_filter_display_preferences(parent, pInfo);
        break;
    case SEI_STEREO_VIDEO_INFO:
        status = h264_sei_stereo_video_info(parent, pInfo);
        break;
    default:
        status = (h264_Status)h264_sei_reserved_sei_message(parent, pInfo, payloadSize);
        break;
    }

    /*
    	viddec_pm_get_bits(parent, (uint32_t *)&tmp, 1);

    	if(tmp == 0x1)		// if byte is not aligned
    	{
    		while(pInfo->bitoff != 0)
    		{
    			viddec_pm_get_bits(parent, (uint32_t *)&bit_equal_to_zero, 1);
    		}
    	}
    */
    return status;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
h264_Status h264_Parse_Supplemental_Enhancement_Information_Message(void *parent, h264_Info* pInfo)
{
    h264_Status status = H264_STATUS_OK;
    int32_t  payload_type, payload_size;
    uint32_t next_8_bits = 0,bits_offset=0,byte_offset = 0;
    uint8_t  is_emul = 0;
    int32_t  bits_operation_result = 0;

    do {
        //// payload_type
        payload_type = 0;
        viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
        while (next_8_bits == 0xFF)
        {
            bits_operation_result = viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
            if (-1 == bits_operation_result)
            {
                status = H264_STATUS_SEI_ERROR;
                return status;
            }
            payload_type += 255;

        }
        //viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
        payload_type += next_8_bits;

        //// payload_size
        payload_size = 0;
        viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
        while (next_8_bits == 0xFF)
        {
            payload_size += 255;
            bits_operation_result = viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
            if (-1 == bits_operation_result)
            {
                status = H264_STATUS_SEI_ERROR;
                return status;
            }
        }
        //viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8);
        payload_size += next_8_bits;

        //PRINTF(MFD_NONE, " SEI: payload type = %d, payload size = %d \n", payload_type, payload_size);


        /////////////////////////////////
        // Parse SEI payloads
        /////////////////////////////////
        status = h264_SEI_payload(parent, pInfo, (h264_sei_payloadtype)payload_type, payload_size);
        if (status != H264_STATUS_OK)
            break;

        viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);
        // OS_INFO("SEI byte_offset 3= %d, bits_offset=%d\n", byte_offset, bits_offset);

        if (bits_offset!=0)
        {
            viddec_pm_get_bits(parent, (uint32_t *)&next_8_bits, 8-bits_offset);
        }

        bits_operation_result = viddec_pm_peek_bits(parent, (uint32_t *)&next_8_bits, 8);
        if (-1 == bits_operation_result)
        {
            status = H264_STATUS_SEI_ERROR;
            return status;
        }

        // OS_INFO("next_8_bits = %08x\n", next_8_bits);

    } while (next_8_bits != 0x80);

    //} while (h264_More_RBSP_Data(parent, pInfo) && status == H264_STATUS_OK);

    return status;
}

#endif

