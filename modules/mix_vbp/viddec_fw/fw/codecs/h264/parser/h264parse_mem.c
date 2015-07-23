
/*!
 ***********************************************************************
 *  \file: h264_dpb_ctl.c
 *
 ***********************************************************************
 */

//#include <limits.h>

#include "h264parse.h"


// ---------------------------------------------------------------------------
// IMPORTANT: note that in this implementation int c is an int not a char
// ---------------------------------------------------------------------------
void* h264_memset( void* buf, uint32_t c, uint32_t num )
{
    uint32_t* buf32 = (uint32_t*)buf;
    uint32_t  size32 = ( num >> 2 );
    uint32_t  i;

    for ( i = 0; i < size32; i++ )
    {
        *buf32++ = c;
    }

    return buf;
}


void* h264_memcpy( void* dest, void* src, uint32_t num )
{
    int32_t*	dest32 = (int32_t*)dest;
    int32_t*    src32 = (int32_t*)src;
    uint32_t	size32 = ( num >> 2 );
    uint32_t	i;

    for ( i = 0; i < size32; i++ )
    {
        *dest32++ = *src32++;
    }

    return dest;
}


#ifndef USER_MODE

//h264_Parse_Copy_Sps_To_DDR () copy local sps to ddr mem
void h264_Parse_Copy_Pps_To_DDR(h264_Info* pInfo, pic_param_set_ptr PPS, uint32_t nPPSId)
{
    uint32_t  copy_size = sizeof(pic_param_set);
    uint32_t  pps_entry_ptr = pInfo->PPS_PADDR_GL+nPPSId*copy_size;

    if (nPPSId < MAX_NUM_PPS)
    {
        cp_using_dma(pps_entry_ptr, (uint32_t)PPS, copy_size, 1, 0);
    }

    return;

}
//end of h264_Parse_Copy_Pps_To_DDR


// h264_Parse_Copy_Pps_From_DDR copy a pps with nPPSId from ddr mem to local PPS
void h264_Parse_Copy_Pps_From_DDR(h264_Info* pInfo, pic_param_set_ptr PPS, uint32_t nPPSId)
{

    uint32_t copy_size= sizeof(pic_param_set);
    uint32_t pps_entry_ptr = pInfo->PPS_PADDR_GL+nPPSId*copy_size;

    if ( nPPSId < MAX_NUM_PPS)
    {
        cp_using_dma(pps_entry_ptr, (uint32_t)PPS, copy_size, 0, 0);
    }

    return;
}
//end of h264_Parse_Copy_Pps_From_DDR


//h264_Parse_Copy_Sps_To_DDR () copy local sps to ddr mem with nSPSId
void h264_Parse_Copy_Sps_To_DDR(h264_Info* pInfo, seq_param_set_used_ptr SPS, uint32_t nSPSId)
{
    uint32_t  copy_size = sizeof(seq_param_set_used);
    uint32_t  sps_entry_ptr = pInfo->SPS_PADDR_GL+nSPSId*sizeof(seq_param_set_all);

    if (nSPSId < MAX_NUM_SPS)
    {
        cp_using_dma(sps_entry_ptr, (uint32_t)SPS, copy_size, 1, 0);
    }

    //OS_INFO("SPS->seq_parameter_set_id = %d\n", SPS->seq_parameter_set_id);


    return;
}

//end of h264_Parse_Copy_Sps_To_DDR


// h264_Parse_Copy_Sps_From_DDR copy a sps with nSPSId from ddr mem to local SPS
void h264_Parse_Copy_Sps_From_DDR(h264_Info* pInfo, seq_param_set_used_ptr SPS, uint32_t nSPSId)
{
    uint32_t copy_size= sizeof(seq_param_set_used);
    uint32_t sps_entry_ptr = pInfo->SPS_PADDR_GL+nSPSId*sizeof(seq_param_set_all);

    if (nSPSId < MAX_NUM_SPS)
    {
        cp_using_dma(sps_entry_ptr, (uint32_t)SPS, copy_size, 0, 0);
    }

    return;

}
//end of h264_Parse_Copy_Sps_From_DDR

//h264_Parse_Copy_Offset_Ref_Frames_To_DDR () copy local offset_ref_frames to ddr mem with nSPSId
void h264_Parse_Copy_Offset_Ref_Frames_To_DDR(h264_Info* pInfo, int32_t* pOffset_ref_frames, uint32_t nSPSId)
{
    uint32_t  copy_size = sizeof(int32_t)*MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;
    uint32_t  offset_ref_frames_entry_ptr = pInfo->OFFSET_REF_FRAME_PADDR_GL+nSPSId*copy_size;

    if (nSPSId < MAX_NUM_SPS)
    {
        //cp_using_dma(offset_ref_frames_entry_ptr, (uint32_t)pOffset_ref_frames, copy_size, 1, 0);
        h264_memcpy((int32_t *)offset_ref_frames_entry_ptr,pOffset_ref_frames, copy_size);
    }

    return;
}

//end of h264_Parse_Copy_Offset_Ref_Frames_To_DDR


// h264_Parse_Copy_Offset_Ref_Frames_From_DDR copy a offset_ref_frames with nSPSId from ddr mem to local offset_ref_frames
void h264_Parse_Copy_Offset_Ref_Frames_From_DDR(h264_Info* pInfo, int32_t* pOffset_ref_frames, uint32_t nSPSId)
{
    uint32_t copy_size= sizeof(int32_t)*MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE;
    uint32_t offset_ref_frames_entry_ptr = pInfo->OFFSET_REF_FRAME_PADDR_GL+nSPSId*copy_size;

    if (nSPSId < MAX_NUM_SPS)
    {
        //cp_using_dma(offset_ref_frames_entry_ptr, (uint32_t)pOffset_ref_frames, copy_size, 0, 0);
        h264_memcpy(pOffset_ref_frames, (int32_t *)offset_ref_frames_entry_ptr, copy_size);
    }

    return;

}
//end of h264_Parse_Copy_Offset_Ref_Frames_From_DDR


//h264_Parse_Check_Sps_Updated_Flag () copy local sps to ddr mem with nSPSId
uint32_t h264_Parse_Check_Sps_Updated_Flag(h264_Info* pInfo, uint32_t nSPSId)
{
    uint32_t  is_updated=0;
    uint32_t  copy_size = sizeof(uint32_t);
    uint32_t  sps_entry_ptr = pInfo->SPS_PADDR_GL+nSPSId*copy_size;


    if (nSPSId < MAX_NUM_SPS)
    {
        cp_using_dma(sps_entry_ptr, (uint32_t)(&is_updated), copy_size, 1, 0);
    }

    //OS_INFO("SPS->seq_parameter_set_id = %d\n", SPS->seq_parameter_set_id);


    return is_updated;
}

//end of h264_Parse_Check_Sps_Updated_Flag


// h264_Parse_Clear_Sps_Updated_Flag copy a sps with nSPSId from ddr mem to local SPS
void h264_Parse_Clear_Sps_Updated_Flag(h264_Info* pInfo, uint32_t nSPSId)
{
    uint32_t  is_updated=0;
    uint32_t copy_size= sizeof(uint32_t);
    uint32_t sps_entry_ptr = pInfo->SPS_PADDR_GL+nSPSId*copy_size;

    if (nSPSId < MAX_NUM_SPS)
    {
        cp_using_dma(sps_entry_ptr, (uint32_t)(&is_updated), copy_size, 0, 0);
    }

    return;

}
//end of h264_Parse_Clear_Sps_Updated_Flag


#endif


