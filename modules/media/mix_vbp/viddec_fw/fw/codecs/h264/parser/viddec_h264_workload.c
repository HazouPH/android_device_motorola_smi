/* Any workload management goes in this file */

#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "h264.h"
#include "h264parse.h"
#include "viddec_fw_item_types.h"
#include "h264parse_dpb.h"


#include "viddec_fw_workload.h"
#include <auto_eas/gen4_mfd.h>
#include "viddec_pm_utils_bstream.h"

// picture parameter 1
#define PUT_BSD_PP1_IMG_DISPOSABLE_FLAG_BIT(w)            (((uint32_t)w)&0x1)
#define PUT_BSD_PP1_SLICE_TYPE_BITS(w)                    ((((uint32_t)w)&0x7)<<1)
#define PUT_BSD_PP1_WEIGHTED_BIPRED_IDC_BITS(w)           ((((uint32_t)w)&0x3)<<4)
#define PUT_BSD_PP1_WEIGHTED_PRED_FLAG_BIT(w)             ((((uint32_t)w)&0x1)<<6)
#define PUT_BSD_PP1_NUM_REF_IDX_L0_BITS(w)                ((((uint32_t)w)&0x3F)<<8)
#define PUT_BSD_PP1_NUM_REF_IDX_L1_BITS(w)                ((((uint32_t)w)&0x3F)<<16)

// picture parameter 2
#define PUT_BSD_PP2_CABAC_INIT_IDC_BITS(w)                (((uint32_t)w)&0x3)
#define PUT_BSD_PP2_QP_BITS(w)                            ((((uint32_t)w)&0x3F)<<2)
#define PUT_BSD_PP2_DISABLE_DBF_IDC_BITS(w)               ((((uint32_t)w)&0x3)<<8)
#define PUT_BSD_PP2_ALPHA_C0_OFFSET_DIV2_BITS(w)          ((((uint32_t)w)&0xF)<<10)
#define PUT_BSD_PP2_BETA_OFFSET_DIV2_BITS(w)              ((((uint32_t)w)&0xF)<<14)
#define PUT_BSD_PP2_IMG_DIRECT_TYPE_BIT(w)                ((((uint32_t)w)&0x1)<<18)
#define PUT_BSD_PP2_CHROMA_QP_OFFSET_BITS(w)              ((((uint32_t)w)&0x1F)<<19)
#define PUT_BSD_PP2_CHROMA_QP_OFFSET_2_BITS(w)            ((((uint32_t)w)&0x1F)<<24)


// slice start parameter
#define PUT_BSD_SS_START_ADDR_BITS(w)                      (((uint32_t)w)&0x7fff)         // 14:0  current slice start address
#define PUT_BSD_SS_SKIP_FS_IDC_BITS(w)                    ((((uint32_t)w)&0x3f)<<16)      // [5:0], [4:0] frame store idc, [5] - 0: top-filed, 1: bottom field
#define PUT_BSD_SS_SKIP_TYPE_BIT(w)                       ((((uint32_t)w)&0x1)<<24)       // 0: P-skip, 1: I-skip
#define PUT_BSD_SS_SKIP_REWIND_BITS(w)                    ((((uint32_t)w)&0xf)<<28)       // number of MB or MBAFF pairs to rewind before skip

//h264_dpb_init
#define PUT_FRAME_WIDTH_MB_BITS(w)                        (((uint32_t)w)&0x7F)
#define PUT_FRAME_HEIGHT_MB_BITS(w)                       ((((uint32_t)w)&0x7F)<<16)

//dpb lut table init
//#define PUT_BSD_IMAGE_FRAME_STORE_IDC_BITS(w)             ((((uint32_t)w)&0x1F)<<8)

//h264 img init
#define PUT_BSD_IMAGE_STRUCTURE_BITS(w)                   (((uint32_t)w)&0x3)
#define PUT_BSD_IMAGE_IDR_BIT(w)                          ((((uint32_t)w)&0x1)<<2)
#define PUT_BSD_IMAGE_MBAFF_FRAME_FLAG_BIT(w)             ((((uint32_t)w)&0x1)<<3)
#define PUT_BSD_IMAGE_ENTROPY_CODING_MODE_FLAG_BIT(w)     ((((uint32_t)w)&0x1)<<4)
#define PUT_BSD_IMAGE_CONSTRAINED_INTRA_PRED_FLAG_BIT(w)  ((((uint32_t)w)&0x1)<<5)
#define PUT_BSD_IMG_FRAME_MBS_ONLY_FLAG_BIT(w)            ((((uint32_t)w)&0x1)<<6)
#define PUT_BSD_IMG_DIRECT_8X8_INFER_FLAG_BIT(w)          ((((uint32_t)w)&0x1)<<7)
#define PUT_BSD_IMAGE_FRAME_STORE_IDC_BITS(w)             ((((uint32_t)w)&0x1F)<<8)

#define PUT_HPD_BSD_IMG_TRANSFORM_8X8_MODE_FLAG_BIT(w)    ((((uint32_t)w)&0x1)<<13)
#define PUT_HPD_BSD_IMG_MONOCHROME_FLAG_BIT(w)            ((((uint32_t)w)&0x1)<<14)
#define PUT_HPD_BSD_IMG_GREY_NONEXISTING_FLAG_BIT(w)      ((((uint32_t)w)&0x1)<<15)
#define PUT_HPD_BSD_IMG_QM_PRESENT_FLAG_BIT(w)            ((((uint32_t)w)&0x1)<<16)
#define PUT_HPD_BSD_IMG_QM_LIST_FLAGS_BITS(w)             ((((uint32_t)w)&0xFF)<<17)
#define PUT_HPD_BSD_IMG_MONOCHROME_PWT_FLAG_BIT(w)        ((((uint32_t)w)&0x1)<<25)


extern void h264_dpb_store_previous_picture_in_dpb(h264_Info * pInfo,
        int32_t NonExisting,
        int32_t use_old);

extern void h264_dpb_flush_dpb (h264_Info * pInfo,int32_t output_all, int32_t keep_complement, int32_t num_ref_frames);



void h264_translate_parser_info_to_frame_attributes(viddec_workload_t *wl, h264_Info *pInfo)
{

    viddec_frame_attributes_t *attrs = &wl->attrs;



    //// Cont_size
    attrs->cont_size.height       = pInfo->img.FrameHeightInMbs*16;
    attrs->cont_size.width        = pInfo->img.PicWidthInMbs*16;

    //// The following attributes will be updated in slice level
    attrs->h264.used_for_reference = 0;
    attrs->h264.top_field_first = 0;
    attrs->h264.top_field_poc = 0;
    attrs->h264.bottom_field_poc = 0;
    attrs->h264.field_pic_flag = 0;

#if 1
/// Double check the size late!!!!!
    //attrs->h264.cropped_size.width = pInfo->img.PicWidthInMbs*16;
    //attrs->h264.cropped_size.height = pInfo->img.PicWidthInMbs*16;

    if ( (pInfo->active_SPS.sps_disp.frame_cropping_flag) &&
            (pInfo->active_SPS.sps_disp.chroma_format_idc < 4))
    {
        int32_t CropUnitX, CropUnitY;
        int32_t SubWidthC, SubHeightC;

        if (pInfo->active_SPS.sps_disp.chroma_format_idc == 0)
        {
            CropUnitX = 1;
            CropUnitY = 2 - pInfo->active_SPS.sps_disp.frame_mbs_only_flag;
        }
        else
        {
            SubWidthC = 2 - ((pInfo->active_SPS.sps_disp.chroma_format_idc - 1) >> 1);
            SubHeightC = 2 - ((pInfo->active_SPS.sps_disp.chroma_format_idc - 1) >>1)
                         - ((pInfo->active_SPS.sps_disp.chroma_format_idc - 1) & 0x1);
            CropUnitX = SubWidthC;
            CropUnitY = SubHeightC * (2 - pInfo->active_SPS.sps_disp.frame_mbs_only_flag);
        }

        if ((int32_t)attrs->cont_size.height >(pInfo->active_SPS.sps_disp.frame_crop_rect_bottom_offset*CropUnitY))
        {
            attrs->cont_size.height -= (pInfo->active_SPS.sps_disp.frame_crop_rect_bottom_offset*CropUnitY);
            //attrs->h264.cropped_size.height-= (pInfo->active_SPS.sps_disp.frame_crop_rect_bottom_offset*CropUnitY);
        }
    }
/// Pan-Scan Info

#endif

}


static void h264_parse_update_frame_attributes(void *parent, h264_Info *pInfo)
{
    viddec_workload_t        *wl_cur,  *wl_next;
    viddec_frame_attributes_t *attrs;
    uint8_t    frame_type=0;


    if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
    {
        wl_cur = viddec_pm_get_header( parent );
        attrs = &wl_cur->attrs;
    }
    else
    {
        wl_next = viddec_pm_get_next_header (parent);
        attrs = &wl_next->attrs;
    }

    /////////update frame type
    if ((pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type)&(0x1 << FRAME_TYPE_STRUCTRUE_OFFSET))
    {
        frame_type = ( (pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type)&((0x7 << FRAME_TYPE_FRAME_OFFSET)) )>> FRAME_TYPE_FRAME_OFFSET;
        switch (frame_type)
        {
        case FRAME_TYPE_IDR:
            attrs->frame_type = VIDDEC_FRAME_TYPE_IDR;
            break;
        case FRAME_TYPE_I:
            attrs->frame_type = VIDDEC_FRAME_TYPE_I;
            break;
        case FRAME_TYPE_P:
            attrs->frame_type = VIDDEC_FRAME_TYPE_P;
            break;
        case FRAME_TYPE_B:
            attrs->frame_type = VIDDEC_FRAME_TYPE_B;
            break;
        default:
            attrs->frame_type = VIDDEC_FRAME_TYPE_INVALID;
            break;
        }

        attrs->bottom_field_type = VIDDEC_FRAME_TYPE_INVALID;
    }
    else
    {
        frame_type = ( (pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type)&((0x7 << FRAME_TYPE_TOP_OFFSET)) )>> FRAME_TYPE_TOP_OFFSET;
        switch (frame_type)
        {
        case FRAME_TYPE_IDR:
            attrs->frame_type = VIDDEC_FRAME_TYPE_IDR;
            break;
        case FRAME_TYPE_I:
            attrs->frame_type = VIDDEC_FRAME_TYPE_I;
            break;
        case FRAME_TYPE_P:
            attrs->frame_type = VIDDEC_FRAME_TYPE_P;
            break;
        case FRAME_TYPE_B:
            attrs->frame_type = VIDDEC_FRAME_TYPE_B;
            break;
        default:
            attrs->frame_type = VIDDEC_FRAME_TYPE_INVALID;
            break;

        }

        frame_type = ( (pInfo->dpb.fs[pInfo->dpb.fs_dec_idc].pic_type)&((0x7 << FRAME_TYPE_BOTTOM_OFFSET)) )>> FRAME_TYPE_BOTTOM_OFFSET;
        switch (frame_type)
        {
        case FRAME_TYPE_IDR:
            attrs->bottom_field_type = VIDDEC_FRAME_TYPE_IDR;
            break;
        case FRAME_TYPE_I:
            attrs->bottom_field_type = VIDDEC_FRAME_TYPE_I;
            break;
        case FRAME_TYPE_P:
            attrs->bottom_field_type = VIDDEC_FRAME_TYPE_P;
            break;
        case FRAME_TYPE_B:
            attrs->bottom_field_type = VIDDEC_FRAME_TYPE_B;
            break;
        default:
            attrs->bottom_field_type = VIDDEC_FRAME_TYPE_INVALID;
            break;

        }
    }

    /////////update is_referece flag
    attrs->h264.used_for_reference |= (pInfo->SliceHeader.nal_ref_idc == 0)? 0: 1;

    /////////update POC
    attrs->h264.top_field_poc = pInfo->img.toppoc;
    attrs->h264.bottom_field_poc = pInfo->img.bottompoc;

    //////// update TFF
    if (attrs->h264.top_field_poc <= attrs->h264.bottom_field_poc) {
        attrs->h264.top_field_first = 1;
    } else {
        attrs->h264.top_field_first = 0;
    }

    /////// update field_pic_flag
    //attrs->h264.field_pic_flag |= (pInfo->SliceHeader.field_pic_flag << pInfo->SliceHeader.bottom_field_flag);
    attrs->h264.field_pic_flag |= pInfo->SliceHeader.field_pic_flag;

    return;
}


static void h264_fill_slice_data(h264_Info *pInfo, h264_slice_data * p_slice_data)
{
    uint32_t data=0;
    uint32_t first_mb_in_slice =0;



    ////////////fill pic parameters 1
    data =   PUT_BSD_PP1_IMG_DISPOSABLE_FLAG_BIT( (pInfo->SliceHeader.nal_ref_idc == 0) ) +
             PUT_BSD_PP1_SLICE_TYPE_BITS(pInfo->SliceHeader.slice_type) +
             PUT_BSD_PP1_WEIGHTED_BIPRED_IDC_BITS(pInfo->active_PPS.weighted_bipred_idc) +
             PUT_BSD_PP1_WEIGHTED_PRED_FLAG_BIT(pInfo->active_PPS.weighted_pred_flag)  +
             PUT_BSD_PP1_NUM_REF_IDX_L0_BITS(pInfo->SliceHeader.num_ref_idx_l0_active)  +
             PUT_BSD_PP1_NUM_REF_IDX_L1_BITS(pInfo->SliceHeader.num_ref_idx_l1_active);
    p_slice_data->h264_bsd_slice_p1 = data;


    ///////////fill pic parameters 2
    data =   PUT_BSD_PP2_CABAC_INIT_IDC_BITS(pInfo->SliceHeader.cabac_init_idc) +
             PUT_BSD_PP2_QP_BITS( (pInfo->SliceHeader.slice_qp_delta + pInfo->active_PPS.pic_init_qp_minus26+26) ) +
             PUT_BSD_PP2_DISABLE_DBF_IDC_BITS(pInfo->SliceHeader.disable_deblocking_filter_idc) +
             PUT_BSD_PP2_ALPHA_C0_OFFSET_DIV2_BITS(pInfo->SliceHeader.slice_alpha_c0_offset_div2) +
             PUT_BSD_PP2_BETA_OFFSET_DIV2_BITS(pInfo->SliceHeader.slice_beta_offset_div2) +
             PUT_BSD_PP2_IMG_DIRECT_TYPE_BIT(pInfo->SliceHeader.direct_spatial_mv_pred_flag) +
             PUT_BSD_PP2_CHROMA_QP_OFFSET_BITS(pInfo->active_PPS.chroma_qp_index_offset) +
             PUT_BSD_PP2_CHROMA_QP_OFFSET_2_BITS(pInfo->active_PPS.second_chroma_qp_index_offset);

    p_slice_data->h264_bsd_slice_p2 = data;

    /////////fill slice start
    first_mb_in_slice = pInfo->SliceHeader.first_mb_in_slice;

    data =   PUT_BSD_SS_START_ADDR_BITS(first_mb_in_slice);
    data |=  PUT_BSD_SS_SKIP_FS_IDC_BITS( pInfo->h264_list_replacement) |
             PUT_BSD_SS_SKIP_TYPE_BIT(0) |
             PUT_BSD_SS_SKIP_REWIND_BITS((pInfo->img.MbaffFrameFlag? 2: 3));

    p_slice_data->h264_bsd_slice_start = data;

}


static void h264_parse_emit_4X4_scaling_matrix( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;

    uint32_t                   i=0, n_items=0;
    uint32_t                   qm_type=0;


    for ( i = 0; i < 6; i++ )
    {
        qm_type = FB_QM;
        if (pInfo->active_SPS.seq_scaling_matrix_present_flag) // check sps first
        {
            if (pInfo->active_SPS.seq_scaling_list_present_flag[i])
            {
                pInfo->qm_present_list |= ((0x1)<<i);

                if (pInfo->active_SPS.UseDefaultScalingMatrix4x4Flag[i]) {
                    qm_type = DEFAULT_QM;
                } else {
                    qm_type = SPS_QM;
                }
            }
        }

        if (pInfo->active_PPS.pic_scaling_matrix_present_flag) // then check pps
        {
            if (pInfo->active_PPS.pic_scaling_list_present_flag[i])
            {
                pInfo->qm_present_list |= ((0x1)<<i);
                if (pInfo->active_PPS.UseDefaultScalingMatrix4x4Flag[i]) {
                    qm_type = DEFAULT_QM;
                } else {
                    qm_type = PPS_QM;
                }
            }
            else
            {
                if ((i != 0) && (i != 3) && (i < 6)) {
                    pInfo->qm_present_list  &= ~((0x1)<<i);
                    qm_type = FB_QM;
                }
            }
        }


        ///////////////////// Emit out Scaling_matrix//////////////////////
        wi.vwi_type = VIDDEC_WORKLOAD_H264_SCALING_MATRIX;
        //    data_offset    0x aa  bb  cc  dd
        //    bb    is the workload item offset
        //    cc    is the qm_type
        //    dd    is the matrix number
        //
        switch (qm_type)
        {
        case (SPS_QM): {

            for (n_items =0; n_items<2; n_items++)
            {
                wi.data.data_offset = i + (SPS_QM << 4) + (n_items <<8);
                wi.data.data_payload[0] = ((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+0]))+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+1]))<<8)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+2]))<<16)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+3]))<<24);
                wi.data.data_payload[1] = ((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+4]))+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+5]))<<8)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+6]))<<16)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList4x4[i][n_items*8+7]))<<24);
                //cur is empty, fill new frame in cur
                viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            }

            break;
        }
        case (PPS_QM): {

            for (n_items =0; n_items<2; n_items++)
            {
                wi.data.data_offset = i + (PPS_QM << 4) + (n_items <<8);
                wi.data.data_payload[0] = ((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+0]))+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+1]))<<8)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+2]))<<16)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+3]))<<24);
                wi.data.data_payload[1] = ((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+4]))+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+5]))<<8)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+6]))<<16)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList4x4[i][n_items*8+7]))<<24);
                //cur is empty, fill new frame in cur
                viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            }

            break;
        }
        case (DEFAULT_QM):
        {

            wi.data.data_offset = i + (DEFAULT_QM << 4);
            wi.data.data_payload[0] = 0;
            wi.data.data_payload[1] = 0;
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            break;
        }
        default:
        {
            break;
        }
        }
    }

}

static void h264_parse_emit_8X8_scaling_matrix( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;

    uint32_t                   i=0, n_items=0;
    uint32_t                   qm_type=0;

    for ( i = 6; i < 8; i++ )
    {
        qm_type = FB_QM;
        if (pInfo->active_SPS.seq_scaling_matrix_present_flag) // check sps first
        {
            if (pInfo->active_SPS.seq_scaling_list_present_flag[i])
            {
                pInfo->qm_present_list |= ((0x1)<<i);

                if (pInfo->active_SPS.UseDefaultScalingMatrix8x8Flag[i-6])
                {
                    qm_type = DEFAULT_QM;
                }
                else
                {
                    qm_type = SPS_QM;
                }
            }
        }

        if (pInfo->active_PPS.pic_scaling_matrix_present_flag) // then check pps
        {
            if (pInfo->active_PPS.pic_scaling_list_present_flag[i])
            {
                pInfo->qm_present_list |= ((0x1)<<i);

                if (pInfo->active_PPS.UseDefaultScalingMatrix8x8Flag[i-6])
                {
                    qm_type = DEFAULT_QM;
                }
                else
                {
                    qm_type = PPS_QM;
                }
            }
        }
        wi.vwi_type = VIDDEC_WORKLOAD_H264_SCALING_MATRIX;

        //    data_offset    0x aa  bb  cc  dd
        //    bb    is the workload item offset
        //    cc    is the qm_type
        //    dd    is the matrix number
        //
        switch (qm_type)
        {
        case (SPS_QM):
        {
            for (n_items =0; n_items<8; n_items++)
            {
                wi.data.data_offset = i + (SPS_QM << 4) + (n_items <<8);
                wi.data.data_payload[0] = ((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+0]))+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+1]))<<8)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+2]))<<16)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+3]))<<24);
                wi.data.data_payload[1] = ((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+4]))+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+5]))<<8)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+6]))<<16)+
                                          (((uint32_t)(pInfo->active_SPS.ScalingList8x8[i-6][n_items*8+7]))<<24);
                //cur is empty, fill new frame in cur
                viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            }
            break;
        }
        case (PPS_QM):
        {
            for (n_items =0; n_items<8; n_items++)
            {
                wi.data.data_offset = i + (PPS_QM << 4) + (n_items <<8);
                wi.data.data_payload[0] = ((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+0]))+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+1]))<<8)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+2]))<<16)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+3]))<<24);
                wi.data.data_payload[1] = ((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+4]))+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+5]))<<8)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+6]))<<16)+
                                          (((uint32_t)(pInfo->active_PPS.ScalingList8x8[i-6][n_items*8+7]))<<24);
                //cur is empty, fill new frame in cur
                viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            }
            break;
        }
        case (DEFAULT_QM):
        {
            wi.data.data_offset = i + (DEFAULT_QM << 4);
            wi.data.data_payload[0] = 0;
            wi.data.data_payload[1] = 0;
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            break;
        }
        default: {
            break;
        }
        }
    }

}



static void h264_fill_pic_data(h264_Info *pInfo, h264_pic_data * p_pic_data)
{
    uint32_t data=0;
    uint32_t dec_idc =0;
    uint32_t frame_structure =0;

    //fill h264_dpb_init
    data =   PUT_FRAME_WIDTH_MB_BITS(pInfo->dpb.PicWidthInMbs) +
             PUT_FRAME_HEIGHT_MB_BITS(pInfo->dpb.FrameHeightInMbs);

    p_pic_data->h264_dpb_init = data;

    ////////////////////////////////file current pic info
    data = 0;
    dec_idc = pInfo->dpb.fs_dec_idc;
    frame_structure = pInfo->img.structure;
    if (frame_structure == FRAME)
        frame_structure=0;
    //data =  PUT_BSD_IMAGE_FRAME_STORE_IDC_BITS(pInfo->dpb.fs[dec_idc].fs_idc);

    //p_pic_data->h264_cur_bsd_img_init= data;

    data  =  PUT_BSD_IMAGE_STRUCTURE_BITS(frame_structure)  +
             PUT_BSD_IMAGE_IDR_BIT(pInfo->nal_unit_type == h264_NAL_UNIT_TYPE_IDR) +
             PUT_BSD_IMAGE_MBAFF_FRAME_FLAG_BIT(pInfo->img.MbaffFrameFlag) +
             PUT_BSD_IMAGE_ENTROPY_CODING_MODE_FLAG_BIT(pInfo->active_PPS.entropy_coding_mode_flag) +
             PUT_BSD_IMAGE_CONSTRAINED_INTRA_PRED_FLAG_BIT(pInfo->active_PPS.constrained_intra_pred_flag) +
             PUT_BSD_IMG_FRAME_MBS_ONLY_FLAG_BIT(pInfo->active_SPS.sps_disp.frame_mbs_only_flag) +
             PUT_BSD_IMG_DIRECT_8X8_INFER_FLAG_BIT(pInfo->active_SPS.sps_disp.direct_8x8_inference_flag) +
             PUT_HPD_BSD_IMG_TRANSFORM_8X8_MODE_FLAG_BIT(pInfo->active_PPS.transform_8x8_mode_flag) +
             PUT_HPD_BSD_IMG_MONOCHROME_FLAG_BIT(((pInfo->active_SPS.sps_disp.chroma_format_idc==0)? 0x1: 0x0)) +
             PUT_HPD_BSD_IMG_GREY_NONEXISTING_FLAG_BIT(0x0) +
             PUT_HPD_BSD_IMG_QM_PRESENT_FLAG_BIT((pInfo->active_PPS.pic_scaling_matrix_present_flag||pInfo->active_SPS.seq_scaling_matrix_present_flag)) +
             PUT_HPD_BSD_IMG_QM_LIST_FLAGS_BITS(pInfo->qm_present_list) +
             PUT_HPD_BSD_IMG_MONOCHROME_PWT_FLAG_BIT(0x1) +
             PUT_BSD_IMAGE_FRAME_STORE_IDC_BITS(pInfo->dpb.fs[dec_idc].fs_idc);

    p_pic_data->h264_cur_bsd_img_init= data;

    //to do: add qm list
    //PUT_HPD_BSD_IMG_QM_LIST_FLAGS_BITS(pInfo->img.q .qm_present_list) +
    //printf("structure = %d, tpoc = %d, bpoc = %d\n", pInfo->img.structure, pInfo->img.toppoc, pInfo->img.bottompoc);

    if (pInfo->img.structure == FRAME)
    {
        // Write down POC
        p_pic_data->h264_cur_mpr_tf_poc = pInfo->img.toppoc;
        p_pic_data->h264_cur_mpr_bf_poc = pInfo->img.bottompoc;
    } else if (pInfo->img.structure == TOP_FIELD)
    {
        // Write down POC
        p_pic_data->h264_cur_mpr_tf_poc = pInfo->img.toppoc;
        p_pic_data->h264_cur_mpr_bf_poc = 0;
    }
    else if (pInfo->img.structure ==  BOTTOM_FIELD)
    {
        // Write down POC
        p_pic_data->h264_cur_mpr_tf_poc = 0;
        p_pic_data->h264_cur_mpr_bf_poc = pInfo->img.bottompoc;
    }
    else
    {
        // Write down POC
        p_pic_data->h264_cur_mpr_tf_poc = 0;
        p_pic_data->h264_cur_mpr_bf_poc = 0;
    }

    return;
}

static void h264_parse_emit_sps(void *parent, h264_Info *pInfo)
{
    viddec_workload_item_t     wi;

    if (pInfo->Is_SPS_updated)
    {
        viddec_fw_reset_workload_item(&wi);
        wi.vwi_type = VIDDEC_WORKLOAD_SEQUENCE_INFO;

        viddec_fw_h264_sps_set_profile_idc(&(wi.h264_sps), pInfo->active_SPS.profile_idc);
        viddec_fw_h264_sps_set_level_idc(&(wi.h264_sps), pInfo->active_SPS.level_idc);
        viddec_fw_h264_sps_set_chroma_format_idc(&(wi.h264_sps), pInfo->active_SPS.sps_disp.chroma_format_idc);
        viddec_fw_h264_sps_set_num_ref_frames(&(wi.h264_sps), pInfo->active_SPS.num_ref_frames);
        viddec_fw_h264_sps_set_gaps_in_frame_num_value_allowed_flag(&(wi.h264_sps), pInfo->active_SPS.gaps_in_frame_num_value_allowed_flag);
        viddec_fw_h264_sps_set_frame_mbs_only_flag(&(wi.h264_sps), pInfo->active_SPS.sps_disp.frame_mbs_only_flag);
        viddec_fw_h264_sps_set_frame_cropping_flag(&(wi.h264_sps), pInfo->active_SPS.sps_disp.frame_cropping_flag);
        viddec_fw_h264_sps_set_vui_parameters_present_flag(&(wi.h264_sps), pInfo->active_SPS.sps_disp.vui_parameters_present_flag);
        wi.h264_sps.pic_width_in_mbs_minus1 = pInfo->active_SPS.sps_disp.pic_width_in_mbs_minus1;
        wi.h264_sps.pic_height_in_map_units_minus1 = pInfo->active_SPS.sps_disp.pic_height_in_map_units_minus1;

        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);

        viddec_fw_reset_workload_item(&wi);
        if (pInfo->active_SPS.sps_disp.frame_cropping_flag)
        {
            wi.vwi_type = VIDDEC_WORKLOAD_H264_CROPPING;
            viddec_fw_h264_cropping_set_left(&(wi.h264_cropping), pInfo->active_SPS.sps_disp.frame_crop_rect_left_offset);
            viddec_fw_h264_cropping_set_right(&(wi.h264_cropping), pInfo->active_SPS.sps_disp.frame_crop_rect_right_offset);
            viddec_fw_h264_cropping_set_top(&(wi.h264_cropping), pInfo->active_SPS.sps_disp.frame_crop_rect_top_offset);
            viddec_fw_h264_cropping_set_bottom(&(wi.h264_cropping), pInfo->active_SPS.sps_disp.frame_crop_rect_bottom_offset);
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }
        viddec_fw_reset_workload_item(&wi);
        if (pInfo->active_SPS.sps_disp.vui_parameters_present_flag == 1)
        {
            wi.vwi_type = VIDDEC_WORKLOAD_DISPLAY_INFO;
            viddec_fw_h264_vui_set_aspect_ratio_info_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag);
            viddec_fw_h264_vui_set_video_signal_type_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.video_signal_type_present_flag);
            viddec_fw_h264_vui_set_pic_struct_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.pic_struct_present_flag);
            viddec_fw_h264_vui_set_timing_info_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.timing_info_present_flag);
            viddec_fw_h264_vui_set_nal_hrd_parameters_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag);
            viddec_fw_h264_vui_set_vcl_hrd_parameters_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag);

            if (pInfo->active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag == 1)
            {
                viddec_fw_h264_vui_set_aspect_ratio_idc(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_idc);
                if (h264_AR_Extended_SAR == pInfo->active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_idc)
                {
                    viddec_fw_h264_vui_set_sar_width(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.sar_width);
                    viddec_fw_h264_vui_set_sar_height(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.sar_height);
                }
            }


            if (pInfo->active_SPS.sps_disp.vui_seq_parameters.video_signal_type_present_flag)
            {
                viddec_fw_h264_vui_set_colour_description_present_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.colour_description_present_flag);
                if (pInfo->active_SPS.sps_disp.vui_seq_parameters.colour_description_present_flag)
                {
                    viddec_fw_h264_vui_set_colour_primaries(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.colour_primaries);
                    viddec_fw_h264_vui_set_transfer_characteristics(&(wi.h264_vui),  pInfo->active_SPS.sps_disp.vui_seq_parameters.transfer_characteristics);
                }
                viddec_fw_h264_vui_set_video_format(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.video_format);
            }

            if (pInfo->active_SPS.sps_disp.vui_seq_parameters.timing_info_present_flag == 1)
            {
                viddec_fw_h264_vui_set_fixed_frame_rate_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.fixed_frame_rate_flag);
            }

            if ( (pInfo->active_SPS.sps_disp.vui_seq_parameters.nal_hrd_parameters_present_flag == 1)
                    || (pInfo->active_SPS.sps_disp.vui_seq_parameters.vcl_hrd_parameters_present_flag == 1))
            {
                viddec_fw_h264_vui_set_low_delay_hrd_flag(&(wi.h264_vui), pInfo->active_SPS.sps_disp.vui_seq_parameters.low_delay_hrd_flag);
            }

            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }

        viddec_fw_reset_workload_item(&wi);

        if (pInfo->active_SPS.sps_disp.vui_seq_parameters.timing_info_present_flag == 1)
        {
            wi.vwi_type = VIDDEC_WORKLOAD_H264_VUI_TIMING_INFO;

            wi.h264_vui_time_info.num_units_in_tick = pInfo->active_SPS.sps_disp.vui_seq_parameters.num_units_in_tick;
            wi.h264_vui_time_info.time_scale = pInfo->active_SPS.sps_disp.vui_seq_parameters.time_scale;
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }
        pInfo->Is_SPS_updated =0;

    }

    return;
}




static void h264_parse_emit_ref_list( void *parent, h264_Info *pInfo, uint32_t list_id)
{
    uint32_t  i=0, nitems=0, byte_index=0, data=0, data_writed=0;
    uint8_t    *p_list;
    viddec_workload_item_t     wi;

    if (0 == list_id)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_H264_REFR_LIST_0;

        if ( (h264_PtypeB==pInfo->SliceHeader.slice_type)||(h264_PtypeP==pInfo->SliceHeader.slice_type) )
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l0_active;
            if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
            {
                p_list = pInfo->slice_ref_list0;
            }
            else
            {
                p_list = pInfo->dpb.listX_0;
            }
        }
        else
        {
            nitems =0;
            p_list = pInfo->dpb.listX_0;
        }
    }
    else
    {
        wi.vwi_type = VIDDEC_WORKLOAD_H264_REFR_LIST_1;

        if ( h264_PtypeB==pInfo->SliceHeader.slice_type)
        {
            nitems = pInfo->SliceHeader.num_ref_idx_l1_active;
            if (pInfo->SliceHeader.sh_refpic_l1.ref_pic_list_reordering_flag)
            {
                p_list = pInfo->slice_ref_list1;
            }
            else
            {
                p_list = pInfo->dpb.listX_1;
            }
        }
        else
        {
            nitems = 0;
            p_list = pInfo->dpb.listX_1;
        }

    }

    if (0 == nitems)
    {
        return;
    }

    byte_index =0;
    data_writed=0;


    for (i=0; i < 32; i++)
    {
        if (byte_index == 0) data = 0;

        if (i<nitems)
        {
            if ( viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[ (p_list[i]&0x1f) ])))
            {
                data |= (pInfo->h264_list_replacement) << byte_index;
            }
            else
            {
                data |= (p_list[i] & 0x7f) << byte_index;
            }
        }
        else
        {
            data |= (0x80) << byte_index;
        }


        if (byte_index == 24)
        {
            byte_index = 0;
            wi.data.data_offset = data_writed&(~0x1);
            wi.data.data_payload[data_writed&0x1]=data;

            data =0;

            if (data_writed&0x1)
            {
                //cur is empty, fill new frame in cur
                viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
            }
            data_writed ++;
        }
        else
        {
            byte_index += 8;
        }
    }

}



void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo )
{

    viddec_workload_item_t     wi;
    h264_slice_data 				slice_data;

    uint32_t		i=0, nitems=0, data=0;
    uint32_t 	bits_offset =0, byte_offset =0;
    uint8_t    	is_emul =0;

    ////////////////////// Update frame attributes/////////////////
    h264_parse_update_frame_attributes(parent,pInfo);


    if (pInfo->SliceHeader.sh_error) {
        // Error type definition, refer to viddec_fw_common_defs.h
        //		if error in top field, VIDDEC_FW_WORKLOAD_ERR_TOPFIELD			= (1 << 17)
        //		if error in bottom field, VIDDEC_FW_WORKLOAD_ERR_BOTTOMFIELD	   = (1 << 18)
        //		if this is frame based, both 2 bits should be set

        if (pInfo->push_to_cur) {
            pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
            pInfo->wl_err_curr |= (pInfo->SliceHeader.structure << FIELD_ERR_OFFSET);
        } else {
            pInfo->wl_err_next |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
            pInfo->wl_err_next |= (pInfo->SliceHeader.structure << FIELD_ERR_OFFSET);
        }
    }


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
    h264_parse_emit_ref_list(parent, pInfo, 0);

    /////file ref list 1
    h264_parse_emit_ref_list(parent, pInfo, 1);

    ///////////////////////////////////// Slice Data ////////////////////////////////
    h264_fill_slice_data(pInfo, &slice_data);

    wi.vwi_type = VIDDEC_WORKLOAD_H264_SLICE_REG;

    wi.data.data_offset = slice_data.h264_bsd_slice_start;
    wi.data.data_payload[0] = slice_data.h264_bsd_slice_p1;
    wi.data.data_payload[1] = slice_data.h264_bsd_slice_p2;

    //cur is empty, fill new frame in cur
    viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);

    ///////////////////////////predict weight table item and data if have///////////////////////////
    if (pInfo->h264_pwt_enabled)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_H264_PWT_BITS_OFFSET;
        wi.data.data_offset = pInfo->h264_pwt_end_byte_offset- pInfo->h264_pwt_start_byte_offset+1;
        wi.data.data_payload[0] = pInfo->h264_pwt_start_bit_offset;
        wi.data.data_payload[1] = pInfo->h264_pwt_end_bit_offset;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            viddec_pm_append_workitem( parent , &wi, false);

            wi.vwi_type = VIDDEC_WORKLOAD_H264_PWT_ES_BYTES;
            wi.es.es_flags = 0;
            viddec_pm_append_misc_tags(parent, pInfo->h264_pwt_start_byte_offset, pInfo->h264_pwt_end_byte_offset,&wi,1);
        }
        else
        {
            viddec_pm_append_workitem( parent , &wi, true);

            wi.vwi_type = VIDDEC_WORKLOAD_H264_PWT_ES_BYTES;
            wi.es.es_flags = 0;
            viddec_pm_append_misc_tags(parent, pInfo->h264_pwt_start_byte_offset, pInfo->h264_pwt_end_byte_offset,&wi,0);
        }
    }


    ////////////////////////////////// Update ES Buffer for Slice ///////////////////////
    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

    //OS_INFO("DEBUG---entropy_coding_mode_flag:%d, bits_offset: %d\n", pInfo->active_PPS.entropy_coding_mode_flag, bits_offset);

    if (pInfo->active_PPS.entropy_coding_mode_flag)
    {
        if (0!=bits_offset)  {
            viddec_pm_get_bits(parent, &data, 8-bits_offset);
        }
    }
    else
    {
        if (0!=bits_offset)  {
            wi.vwi_type = VIDDEC_WORKLOAD_H264_SH_BITS_OFFSET;
            wi.data.data_offset = bits_offset;
            wi.data.data_payload[0]=0;
            wi.data.data_payload[1]=0;
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }
    }

    if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
    {
        viddec_pm_append_pixeldata( parent );
    }
    else
    {
        viddec_pm_append_pixeldata_next( parent);
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

    h264_parse_emit_4X4_scaling_matrix(parent, pInfo);
    h264_parse_emit_8X8_scaling_matrix(parent, pInfo);

    h264_fill_pic_data(pInfo, &pic_data);

    // How many payloads must be generated
    nitems = (sizeof(h264_pic_data) + 7) / 8; // In QWORDs rounded up

    pl = (const uint32_t *) &pic_data;

    // Dump slice data to an array of workitems,  to do pl access non valid mem
    for ( i = 0; i < nitems; i++ )
    {
        wi.vwi_type           = VIDDEC_WORKLOAD_H264_PIC_REG;
        wi.data.data_offset   = (unsigned int)pl - (unsigned int)&pic_data; // offset within struct
        wi.data.data_payload[0] = pl[0];
        wi.data.data_payload[1] = pl[1];
        pl += 2;
        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
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
        viddec_workload_t			*wl_cur = viddec_pm_get_header( parent );
        //pInfo->img.g_new_frame = 0;
        pInfo->Is_first_frame_in_stream =0;
        pInfo->is_frame_boundary_detected_by_non_slice_nal=0;
        pInfo->push_to_cur = 1;
        h264_translate_parser_info_to_frame_attributes(wl_cur, pInfo);
    }
    else  // move to cur if frame boundary detected by previous non slice nal, or move to next if not
    {
        viddec_workload_t        *wl_next = viddec_pm_get_next_header (parent);

        pInfo->push_to_cur = 0;
        h264_translate_parser_info_to_frame_attributes(wl_next, pInfo);

        pInfo->is_current_workload_done=1;
    }

    ///////////////////// SPS/////////////////////
    h264_parse_emit_sps(parent, pInfo);

    /////////////////////display frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_displayed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_REF_FRAME_DISPLAY_0 + pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;
        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
    }
    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_removed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_REF_FRAME_RELEASE_0 + pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;
        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
    }
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    /////////////////////flust frames (do not display)/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_dropped;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_REF_FRAME_DROPOUT_0 + pInfo->dpb.frame_id_need_to_be_dropped[i];
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_dropped[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;
        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
    }
    pInfo->dpb.frame_numbers_need_to_be_dropped =0;

    /////////////////////updata DPB frames/////////////////////
    nitems = pInfo->dpb.used_size;
    for (i=0; i<nitems; i++)
    {
        uint8_t fs_id = pInfo->dpb.fs_dpb_idc[i];

        if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[fs_id])) == 0)
        {
            wi.vwi_type = VIDDEC_WORKLOAD_DPB_ACTIVE_FRAME_0+fs_id;
            wi.ref_frame.reference_id = fs_id;
            wi.ref_frame.luma_phys_addr = 0;
            wi.ref_frame.chroma_phys_addr = 0;
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }
    }

    /////////////////////updata dpb frames info (poc)/////////////////////
    nitems = pInfo->dpb.used_size;
    for (i=0; i<nitems; i++)
    {
        uint8_t fs_id = pInfo->dpb.fs_dpb_idc[i];

        if (viddec_h264_get_is_non_existent(&(pInfo->dpb.fs[fs_id])) == 0)
        {
            wi.vwi_type = VIDDEC_WORKLOAD_H264_DPB_FRAME_POC;
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
            //cur is empty, fill new frame in cur
            viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
        }
    }

    /////////////////////Alloc buffer for current Existing frame/////////////////////
    if (0!=pInfo->dpb.frame_numbers_need_to_be_allocated)
    {
        if (pInfo->push_to_cur)
        {
            viddec_workload_t        *wl_cur = viddec_pm_get_header (parent);
            wl_cur->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (pInfo->dpb.frame_id_need_to_be_allocated & 0x1f);
        }
        else
        {
            viddec_workload_t        *wl_next = viddec_pm_get_next_header (parent);
            wl_next->is_reference_frame |= WORKLOAD_REFERENCE_FRAME | (pInfo->dpb.frame_id_need_to_be_allocated & 0x1f);
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_allocated =0;

    return;
}



void h264_parse_emit_eos( void *parent, h264_Info *pInfo )
{

    uint32_t nitems=0, i=0;
    viddec_workload_item_t	wi;


    wi.vwi_type = VIDDEC_WORKLOAD_EOS_BEGIN_BOUNDARY;
    wi.ref_frame.reference_id = 0;
    wi.ref_frame.luma_phys_addr = 0;
    wi.ref_frame.chroma_phys_addr = 0;

    //cur is empty, fill new frame in cur
    viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);

    //// Now we can flush out all frames in DPB fro display

    if (MPD_DPB_FS_NULL_IDC != pInfo->dpb.fs_dec_idc)
    {
        if (viddec_h264_get_is_used(&(pInfo->dpb.fs[pInfo->dpb.fs_dec_idc])) != 3)
        {
            h264_dpb_mark_dangling_field(&pInfo->dpb, pInfo->dpb.fs_dec_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
        }
    }


    h264_dpb_store_previous_picture_in_dpb(pInfo, 0,0);
    h264_dpb_flush_dpb(pInfo, 1, 0, pInfo->active_SPS.num_ref_frames);


    /////////////////////display frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_displayed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_EOS_DISPLAY_FRAME_0 + pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_displayed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;
        //cur is empty, fill new frame in cur
        viddec_pm_append_workitem( parent, &wi , !pInfo->push_to_cur);
    }
    pInfo->dpb.frame_numbers_need_to_be_displayed =0;


    /////////////////////release frames/////////////////////
    nitems = pInfo->dpb.frame_numbers_need_to_be_removed;

    for (i=0; i<nitems; i++)
    {
        wi.vwi_type = VIDDEC_WORKLOAD_EOS_RELEASE_FRAME_0 + pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.reference_id = pInfo->dpb.frame_id_need_to_be_removed[i];
        wi.ref_frame.luma_phys_addr = 0;
        wi.ref_frame.chroma_phys_addr = 0;

        if (pInfo->push_to_cur) //cur is empty, fill new frame in cur
        {
            viddec_pm_append_workitem( parent, &wi , false);
            viddec_pm_set_next_frame_error_on_eos(parent, VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE);
        }
        else
        {
            viddec_pm_append_workitem( parent, &wi , true);
            viddec_pm_set_next_frame_error_on_eos(parent, pInfo->wl_err_next);
        }
    }
    pInfo->dpb.frame_numbers_need_to_be_removed =0;

    return;
}






