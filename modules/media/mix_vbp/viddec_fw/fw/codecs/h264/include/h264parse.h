#ifndef __H264PARSE_H_
#define __H264PARSE_H_

#include "h264.h"

#ifndef MFD_FIRMWARE
#define true 1
#define false 0
#endif

////////////////////////////////////////////////////////////////////
// The following part is only for Parser Debug
///////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif


    enum h264_debug_point_id
    {
        WARNING_H264_GENERAL = 0xff000000,
        WARNING_H264_DPB,
        WARNING_H264_REFLIST,
        WARNING_H264_SPS,
        WARNING_H264_PPS,
        WARNING_H264_SEI,
        WARNING_H264_VCL,

        ERROR_H264_GENERAL = 0xffff0000,
        ERROR_H264_DPB,
        ERROR_H264_REFLIST,
        ERROR_H264_SPS,
        ERROR_H264_PPS,
        ERROR_H264_SEI,
        ERROR_H264_VCL
    };

    static inline void MFD_PARSER_DEBUG(int debug_point_id)
    {
#ifdef H264_MFD_DEBUG

        int p1,p2,p3,p4,p5,p6;

        p1 = 0x0BAD;
        p2 = 0xC0DE;
        p3 = debug_point_id;
        p4=p5=p6 = 0;

        DEBUG_WRITE(p1,p2,p3,p4,p5,p6);
#endif

        debug_point_id = debug_point_id;

        return;
    }




////////////////////////////////////////////////////////////////////
///////////////////////////// Init functions
////////////////////////////////////////////////////////////////////
    extern void h264_init_old_slice(h264_Info* pInfo);
    extern void h264_init_img(h264_Info* pInfo);
    extern void h264_init_Info(h264_Info* pInfo);
    extern void h264_init_Info_under_sps_pps_level(h264_Info* pInfo);
    extern void h264_init_sps_pps(struct h264_viddec_parser* parser, uint32_t *persist_mem);

    extern void h264_update_old_slice(h264_Info * pInfo,h264_Slice_Header_t next_SliceHeader);
    extern void h264_sei_stream_initialise (h264_Info* pInfo);
    extern void h264_update_img_info(h264_Info * pInfo );
    extern void h264_update_frame_type(h264_Info * pInfo );

    extern int32_t h264_check_previous_frame_end(h264_Info * pInfo);


////////////////////////////////////////////////////////////////////
///////////////////////////// bsd functions
////////////////////////////////////////////////////////////////////
    extern uint8_t h264_More_RBSP_Data(void *parent, h264_Info * pInfo);
////// VLE and bit operation
    extern uint32_t h264_get_codeNum(void *parent,h264_Info* pInfo);
    extern int32_t h264_GetVLCElement(void *parent,h264_Info* pInfo, uint8_t bIsSigned);



////////////////////////////////////////////////////////////////////
///////////////////////////// parse functions
////////////////////////////////////////////////////////////////////

//NAL
    extern h264_Status h264_Parse_NAL_Unit(void *parent, h264_Info* pInfo, uint8_t *nal_ref_idc);

////// Slice header
    extern h264_Status h264_Parse_Slice_Layer_Without_Partitioning_RBSP(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader);
    extern h264_Status h264_Parse_Slice_Header_1(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader);
    extern h264_Status h264_Parse_Slice_Header_2(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader);
    extern h264_Status h264_Parse_Slice_Header_3(void *parent, h264_Info* pInfo, h264_Slice_Header_t *SliceHeader);


////// SPS
    extern h264_Status h264_Parse_SeqParameterSet(void *parent, h264_Info * pInfo,seq_param_set_used_ptr SPS, vui_seq_parameters_t_not_used_ptr pVUI_Seq_Not_Used, int32_t* pOffset_ref_frame);
//extern h264_Status h264_Parse_SeqParameterSet_Extension(void *parent, h264_Info * pInfo);
    extern h264_Status h264_Parse_PicParameterSet(void *parent, h264_Info * pInfo,h264_PicParameterSet_t* PictureParameterSet);

////// SEI functions
    h264_Status h264_Parse_Supplemental_Enhancement_Information_Message(void *parent,h264_Info* pInfo);
    h264_Status h264_SEI_payload(void *parent, h264_Info* pInfo, h264_sei_payloadtype payloadType, int32_t payloadSize);

//////
    extern h264_Status h264_Scaling_List(void *parent, uint8_t *scalingList, int32_t sizeOfScalingList, uint8_t  *UseDefaultScalingMatrix, h264_Info* pInfo);
    extern h264_Status h264_Parse_Ref_Pic_List_Reordering(void *parent,h264_Info* pInfo,h264_Slice_Header_t *SliceHeader);
    extern h264_Status h264_Parse_Pred_Weight_Table(void *parent,h264_Info* pInfo,h264_Slice_Header_t *SliceHeader);
    extern h264_Status h264_Parse_Dec_Ref_Pic_Marking(void *parent,h264_Info* pInfo,h264_Slice_Header_t *SliceHeader);



///// Mem functions
    extern void* h264_memset( void* buf, uint32_t c, uint32_t num );
    extern void* h264_memcpy( void* dest, void* src, uint32_t num );

    extern void h264_Parse_Copy_Sps_To_DDR(h264_Info* pInfo, seq_param_set_used_ptr SPS, uint32_t nSPSId);
    extern void h264_Parse_Copy_Sps_From_DDR(h264_Info* pInfo, seq_param_set_used_ptr SPS, uint32_t nSPSId);

    extern void h264_Parse_Copy_Pps_To_DDR(h264_Info* pInfo, pic_param_set_ptr PPS, uint32_t nPPSId);
    extern void h264_Parse_Copy_Pps_From_DDR(h264_Info* pInfo, pic_param_set_ptr PPS, uint32_t nPPSId);

    extern void h264_Parse_Copy_Offset_Ref_Frames_To_DDR(h264_Info* pInfo, int32_t* pOffset_ref_frames, uint32_t nSPSId);
    extern void h264_Parse_Copy_Offset_Ref_Frames_From_DDR(h264_Info* pInfo, int32_t* pOffset_ref_frames, uint32_t nSPSId);
    extern uint32_t h264_Parse_Check_Sps_Updated_Flag(h264_Info* pInfo, uint32_t nSPSId);
    extern void h264_Parse_Clear_Sps_Updated_Flag(h264_Info* pInfo, uint32_t nSPSId);


////////////////////////////////////////////////////////////////////
///////////////////////////// workload functions
////////////////////////////////////////////////////////////////////

    extern void h264_parse_emit_current_slice( void *parent, h264_Info *pInfo );

    extern void h264_parse_emit_current_pic( void *parent, h264_Info *pInfo );

    extern void h264_parse_emit_start_new_frame( void *parent, h264_Info *pInfo );
    extern void h264_parse_emit_eos( void *parent, h264_Info *pInfo );
#ifdef __cplusplus
}
#endif

////////////////////////////////////////////////////////////////////
///////////////////////////// utils functions
////////////////////////////////////////////////////////////////////
extern int32_t h264_is_new_picture_start(h264_Info* pInfo, h264_Slice_Header_t cur_slice, h264_Slice_Header_t old_slice);
extern int32_t h264_is_second_field(h264_Info * pInfo);
///// Math functions
uint32_t ldiv_mod_u(uint32_t a, uint32_t b, uint32_t * mod);
uint32_t mult_u(uint32_t var1, uint32_t var2);



////////////////////////////////////////////////////////////////////
///////////////////////////// utils functions outside h264
////////////////////////////////////////////////////////////////////

extern void *memset(void *s, int32_t c, uint32_t n);
extern void *memcpy(void *dest, const void *src, uint32_t n);
extern uint32_t cp_using_dma(uint32_t ddr_addr, uint32_t local_addr, uint32_t size, char to_ddr, char swap);
extern int32_t viddec_pm_get_bits(void *parent, uint32_t *data, uint32_t num_bits);
extern int32_t viddec_pm_peek_bits(void *parent, uint32_t *data, uint32_t num_bits);



////////////////////////////////////////////////////////////////////
///////////////////////////// Second level parse functions
////////////////////////////////////////////////////////////////////

#endif  ////__H264PARSE_H_



