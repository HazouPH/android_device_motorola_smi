/* INTEL CONFIDENTIAL
* Copyright (c) 2009, 2012 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/


#include <dlfcn.h>

#include "h264.h"
#include "vbp_loader.h"
#include "vbp_utils.h"
#include "vbp_h264_parser.h"

typedef struct vbp_h264_parser_private_t vbp_h264_parser_private;

typedef enum
{
    H264_BS_LENGTH_PREFIXED,
    H264_BS_SC_PREFIXED,
    H264_BS_SINGLE_NAL
} H264_BS_PATTERN;

struct vbp_h264_parser_private_t
{
    /* number of bytes used to encode length of NAL payload.  If parser does not receive configuration data
    and NAL_length_size is equal to zero when bitstream parsing begins, we assume bitstream is in AnnexB
    byte stream format. */
    int NAL_length_size;

    /* indicate if stream is length prefixed */
    int length_prefix_verified;

    H264_BS_PATTERN bitstream_pattern;
};

/* default scaling list table */
unsigned char Default_4x4_Intra[16] =
{
    6,13,20,28,
    13,20,28,32,
    20,28,32,37,
    28,32,37,42
};

unsigned char Default_4x4_Inter[16] =
{
    10,14,20,24,
    14,20,24,27,
    20,24,27,30,
    24,27,30,34
};

unsigned char Default_8x8_Intra[64] =
{
    6,10,13,16,18,23,25,27,
    10,11,16,18,23,25,27,29,
    13,16,18,23,25,27,29,31,
    16,18,23,25,27,29,31,33,
    18,23,25,27,29,31,33,36,
    23,25,27,29,31,33,36,38,
    25,27,29,31,33,36,38,40,
    27,29,31,33,36,38,40,42
};

unsigned char Default_8x8_Inter[64] =
{
    9,13,15,17,19,21,22,24,
    13,13,17,19,21,22,24,25,
    15,17,19,21,22,24,25,27,
    17,19,21,22,24,25,27,28,
    19,21,22,24,25,27,28,30,
    21,22,24,25,27,28,30,32,
    22,24,25,27,28,30,32,33,
    24,25,27,28,30,32,33,35
};

unsigned char quant_flat[16] =
{
    16,16,16,16,
    16,16,16,16,
    16,16,16,16,
    16,16,16,16
};

unsigned char quant8_flat[64] =
{
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16,
    16,16,16,16,16,16,16,16
};

unsigned char* UseDefaultList[8] =
{
    Default_4x4_Intra, Default_4x4_Intra, Default_4x4_Intra,
    Default_4x4_Inter, Default_4x4_Inter, Default_4x4_Inter,
    Default_8x8_Intra,
    Default_8x8_Inter
};

static uint8 h264_aspect_ratio_table[][2] =
{
    {0, 0},
    {1, 1},
    {12, 11},
    {10, 11},
    {16, 11},
    {40, 33},
    {24, 11},
    {20, 11},
    {32, 11},
    {80, 33},
    {18, 11},
    {15, 11},
    {64, 33},
    {160, 99},
    {4, 3},
    {3, 2},
    {2, 1},
    // reserved
    {0, 0}
};



/**
 *
 */
uint32 vbp_init_parser_entries_h264(vbp_context *pcontext)
{
    if (NULL == pcontext->parser_ops)
    {
        return VBP_PARM;
    }
    pcontext->parser_ops->init = dlsym(pcontext->fd_parser, "viddec_h264_init");
    if (NULL == pcontext->parser_ops->init)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->parse_sc = viddec_parse_sc;

    pcontext->parser_ops->parse_syntax = dlsym(pcontext->fd_parser, "viddec_h264_parse");
    if (NULL == pcontext->parser_ops->parse_syntax)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->get_cxt_size = dlsym(pcontext->fd_parser, "viddec_h264_get_context_size");
    if (NULL == pcontext->parser_ops->get_cxt_size)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#ifdef VBP
    pcontext->parser_ops->is_wkld_done = NULL;
#else
    pcontext->parser_ops->is_wkld_done = dlsym(pcontext->fd_parser, "viddec_h264_wkld_done");
    if (NULL == pcontext->parser_ops->is_wkld_done)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#endif

    pcontext->parser_ops->flush = dlsym(pcontext->fd_parser, "viddec_h264_flush");;
    if (NULL == pcontext->parser_ops->flush)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    /* entry point not needed */
    pcontext->parser_ops->is_frame_start = NULL;
    return VBP_OK;
}


/**
 *
 */
uint32 vbp_allocate_query_data_h264(vbp_context *pcontext)
{
    if (NULL != pcontext->query_data)
    {
        return VBP_PARM;
    }

    pcontext->query_data = NULL;
    vbp_data_h264 *query_data = NULL;

    query_data = vbp_malloc_set0(vbp_data_h264, 1);
    if (NULL == query_data)
    {
        goto cleanup;
    }

    /* assign the pointer */
    pcontext->query_data = (void *)query_data;

    query_data->pic_data = vbp_malloc_set0(vbp_picture_data_h264, MAX_NUM_PICTURES);
    if (NULL == query_data->pic_data)
    {
        goto cleanup;
    }

    int i;
    for (i = 0; i < MAX_NUM_PICTURES; i++)
    {
        query_data->pic_data[i].pic_parms = vbp_malloc_set0(VAPictureParameterBufferH264, 1);
        if (NULL == query_data->pic_data[i].pic_parms)
        {
            goto cleanup;
        }
        query_data->pic_data[i].num_slices = 0;
        query_data->pic_data[i].slc_data = vbp_malloc_set0(vbp_slice_data_h264, MAX_NUM_SLICES);
        if (NULL == query_data->pic_data[i].slc_data)
        {
            goto cleanup;
        }
    }


    query_data->IQ_matrix_buf = vbp_malloc_set0(VAIQMatrixBufferH264, 1);
    if (NULL == query_data->IQ_matrix_buf)
    {
        goto cleanup;
    }

    query_data->codec_data = vbp_malloc_set0(vbp_codec_data_h264, 1);
    if (NULL == query_data->codec_data)
    {
        goto cleanup;
    }

    pcontext->parser_private = NULL;
    vbp_h264_parser_private *parser_private = NULL;

    parser_private = vbp_malloc_set0(vbp_h264_parser_private, 1);
    if (NULL == parser_private)
    {
        goto cleanup;
    }

    /* assign the pointer */
    pcontext->parser_private = (void *)parser_private;

    /* init the pointer */
    parser_private->NAL_length_size = 0;

    parser_private->length_prefix_verified = 0;

    parser_private->bitstream_pattern = H264_BS_SC_PREFIXED;

    return VBP_OK;

cleanup:
    vbp_free_query_data_h264(pcontext);

    return VBP_MEM;
}

uint32 vbp_free_query_data_h264(vbp_context *pcontext)
{
    if (NULL != pcontext->parser_private)
    {
        free(pcontext->parser_private);
        pcontext->parser_private = NULL;
    }

    if (NULL == pcontext->query_data)
    {
        return VBP_OK;
    }

    int i;
    vbp_data_h264 *query_data;
    query_data = (vbp_data_h264 *)pcontext->query_data;

    if (query_data->pic_data)
    {
        for (i = 0; i < MAX_NUM_PICTURES; i++)
        {
            free(query_data->pic_data[i].slc_data);
            free(query_data->pic_data[i].pic_parms);
        }
        free(query_data->pic_data);
    }

    free(query_data->IQ_matrix_buf);
    free(query_data->codec_data);
    free(query_data);

    pcontext->query_data = NULL;

    return VBP_OK;
}


static inline uint16_t vbp_utils_ntohs(uint8_t* p)
{
    uint16_t i = ((*p) << 8) + ((*(p+1)));
    return i;
}

static inline uint32_t vbp_utils_ntohl(uint8_t* p)
{
    uint32_t i = ((*p) << 24) + ((*(p+1)) << 16) + ((*(p+2)) << 8) + ((*(p+3)));
    return i;
}


static inline void vbp_set_VAPicture_h264(
    int curr_picture_structure,
    int bottom_field,
    frame_store* store,
    VAPictureH264* pic)
{
    if (FRAME == curr_picture_structure)
    {
        if (FRAME != viddec_h264_get_dec_structure(store))
        {
            WTRACE("Reference picture structure is not frame for current frame picture!");
        }
        pic->flags = 0;
        pic->TopFieldOrderCnt = store->top_field.poc;
        pic->BottomFieldOrderCnt = store->bottom_field.poc;
    }
    else
    {
        if (FRAME == viddec_h264_get_dec_structure(store))
        {
            WTRACE("reference picture structure is frame for current field picture!");
        }
        if (bottom_field)
        {
            pic->flags = VA_PICTURE_H264_BOTTOM_FIELD;
            pic->TopFieldOrderCnt = store->top_field.poc;
            pic->BottomFieldOrderCnt = store->bottom_field.poc;
        }
        else
        {
            pic->flags = VA_PICTURE_H264_TOP_FIELD;
            pic->TopFieldOrderCnt = store->top_field.poc;
            pic->BottomFieldOrderCnt = store->bottom_field.poc;
        }
    }
}

static inline void vbp_set_slice_ref_list_h264(
    struct h264_viddec_parser* h264_parser,
    VASliceParameterBufferH264 *slc_parms)
{
    int i, j;
    int num_ref_idx_active = 0;
    h264_Slice_Header_t* slice_header = &(h264_parser->info.SliceHeader);
    uint8_t* p_list = NULL;
    VAPictureH264* refPicListX = NULL;
    frame_store* fs = NULL;

    /* initialize ref picutre list, set picture id and flags to invalid. */

    for (i = 0; i < 2; i++)
    {
        refPicListX = (i == 0) ? &(slc_parms->RefPicList0[0]) : &(slc_parms->RefPicList1[0]);
        for (j = 0; j < 32; j++)
        {
            refPicListX->picture_id = VA_INVALID_SURFACE;
            refPicListX->frame_idx = 0;
            refPicListX->flags = VA_PICTURE_H264_INVALID;
            refPicListX->TopFieldOrderCnt = 0;
            refPicListX->BottomFieldOrderCnt = 0;
            refPicListX++;
        }
    }

    for (i = 0; i < 2; i++)
    {
        refPicListX = (i == 0) ? &(slc_parms->RefPicList0[0]) : &(slc_parms->RefPicList1[0]);

        if ((i == 0) &&
            ((h264_PtypeB == slice_header->slice_type) ||
             (h264_PtypeP == slice_header->slice_type)))
        {
            num_ref_idx_active = slice_header->num_ref_idx_l0_active;
            if (slice_header->sh_refpic_l0.ref_pic_list_reordering_flag)
            {
                p_list = h264_parser->info.slice_ref_list0;
            }
            else
            {
                p_list = h264_parser->info.dpb.listX_0;
            }
        }
        else if ((i == 1) && (h264_PtypeB == slice_header->slice_type))
        {
            num_ref_idx_active = slice_header->num_ref_idx_l1_active;
            if (slice_header->sh_refpic_l1.ref_pic_list_reordering_flag)
            {
                p_list = h264_parser->info.slice_ref_list1;
            }
            else
            {
                p_list = h264_parser->info.dpb.listX_1;
            }
        }
        else
        {
            num_ref_idx_active = 0;
            p_list = NULL;
        }


        for (j = 0; j < num_ref_idx_active; j++)
        {
            fs = &(h264_parser->info.dpb.fs[(p_list[j] & 0x1f)]);

            /* bit 5 indicates if reference picture is bottom field */
            vbp_set_VAPicture_h264(
                h264_parser->info.img.structure,
                (p_list[j] & 0x20) >> 5,
                fs,
                refPicListX);

            refPicListX->frame_idx = fs->frame_num;
            refPicListX->flags |= viddec_h264_get_is_long_term(fs) ? VA_PICTURE_H264_LONG_TERM_REFERENCE : VA_PICTURE_H264_SHORT_TERM_REFERENCE;
            refPicListX++;
        }
    }
}

static inline void vbp_set_pre_weight_table_h264(
    struct h264_viddec_parser* h264_parser,
    VASliceParameterBufferH264 *slc_parms)
{
    h264_Slice_Header_t* slice_header = &(h264_parser->info.SliceHeader);
    int i, j;

    if ((((h264_PtypeP == slice_header->slice_type) ||
          (h264_PtypeB == slice_header->slice_type)) &&
          h264_parser->info.active_PPS.weighted_pred_flag) ||
         ((h264_PtypeB == slice_header->slice_type) &&
         (1 == h264_parser->info.active_PPS.weighted_bipred_idc)))
    {
        slc_parms->luma_log2_weight_denom = slice_header->sh_predwttbl.luma_log2_weight_denom;
        slc_parms->chroma_log2_weight_denom = slice_header->sh_predwttbl.chroma_log2_weight_denom;
        slc_parms->luma_weight_l0_flag = slice_header->sh_predwttbl.luma_weight_l0_flag;
        slc_parms->chroma_weight_l0_flag = slice_header->sh_predwttbl.chroma_weight_l0_flag;
        slc_parms->luma_weight_l1_flag = slice_header->sh_predwttbl.luma_weight_l1_flag;
        slc_parms->chroma_weight_l1_flag = slice_header->sh_predwttbl.chroma_weight_l1_flag;

        for (i = 0; i < 32; i++)
        {
            slc_parms->luma_weight_l0[i] = 	slice_header->sh_predwttbl.luma_weight_l0[i];
            slc_parms->luma_offset_l0[i] = 	slice_header->sh_predwttbl.luma_offset_l0[i];
            slc_parms->luma_weight_l1[i] = 	slice_header->sh_predwttbl.luma_weight_l1[i];
            slc_parms->luma_offset_l1[i] = 	slice_header->sh_predwttbl.luma_offset_l1[i];

            for (j = 0; j < 2; j++)
            {
                slc_parms->chroma_weight_l0[i][j] = slice_header->sh_predwttbl.chroma_weight_l0[i][j];
                slc_parms->chroma_offset_l0[i][j] = slice_header->sh_predwttbl.chroma_offset_l0[i][j];
                slc_parms->chroma_weight_l1[i][j] = slice_header->sh_predwttbl.chroma_weight_l1[i][j];
                slc_parms->chroma_offset_l1[i][j] = slice_header->sh_predwttbl.chroma_offset_l1[i][j];
            }
        }
    }
    else
    {
        /* default weight table */
        slc_parms->luma_log2_weight_denom = 5;
        slc_parms->chroma_log2_weight_denom = 5;
        slc_parms->luma_weight_l0_flag = 0;
        slc_parms->luma_weight_l1_flag = 0;
        slc_parms->chroma_weight_l0_flag = 0;
        slc_parms->chroma_weight_l1_flag = 0;
        for (i = 0; i < 32; i++)
        {
            slc_parms->luma_weight_l0[i] = 0;
            slc_parms->luma_offset_l0[i] = 0;
            slc_parms->luma_weight_l1[i] = 0;
            slc_parms->luma_offset_l1[i] = 0;

            for (j = 0; j < 2; j++)
            {
                slc_parms->chroma_weight_l0[i][j] = 0;
                slc_parms->chroma_offset_l0[i][j] = 0;
                slc_parms->chroma_weight_l1[i][j] = 0;
                slc_parms->chroma_offset_l1[i][j] = 0;
            }
        }
    }
}


static inline void vbp_set_reference_frames_h264(
    struct h264_viddec_parser *parser,
    VAPictureParameterBufferH264* pic_parms)
{
    int buffer_idx;
    int frame_idx;
    frame_store* store = NULL;
    h264_DecodedPictureBuffer* dpb = &(parser->info.dpb);
    /* initialize reference frames */
    for (frame_idx = 0; frame_idx < 16; frame_idx++)
    {
        pic_parms->ReferenceFrames[frame_idx].picture_id = VA_INVALID_SURFACE;
        pic_parms->ReferenceFrames[frame_idx].frame_idx = 0;
        pic_parms->ReferenceFrames[frame_idx].flags = VA_PICTURE_H264_INVALID;
        pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = 0;
        pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = 0;
    }
    pic_parms->num_ref_frames = 0;

    frame_idx = 0;

    /* ITRACE("short term frame in dpb %d", dpb->ref_frames_in_buffer);  */
    /* set short term reference frames */
    for (buffer_idx = 0; buffer_idx < dpb->ref_frames_in_buffer; buffer_idx++)
    {
        if (frame_idx >= 16 || buffer_idx >= 16)
        {
            WTRACE("Frame index is out of bound.");
            break;
        }

        store = &dpb->fs[dpb->fs_ref_idc[buffer_idx]];
        /* if (store->is_used == 3 && store->frame.used_for_reference == 3) */
        if (viddec_h264_get_is_used(store))
        {
            pic_parms->ReferenceFrames[frame_idx].frame_idx = store->frame_num;
            pic_parms->ReferenceFrames[frame_idx].flags = VA_PICTURE_H264_SHORT_TERM_REFERENCE;
            if (FRAME == parser->info.img.structure)
            {
                pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = store->top_field.poc;
                pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = store->bottom_field.poc;
            }
            else
            {
                pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = store->top_field.poc;
                pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = store->bottom_field.poc;
                if (store->top_field.used_for_reference && store->bottom_field.used_for_reference)
                {
                    /* if both fields are used for reference, just set flag to be frame (0) */
                }
                else
                {
                    if (store->top_field.used_for_reference)
                        pic_parms->ReferenceFrames[frame_idx].flags |= VA_PICTURE_H264_TOP_FIELD;
                    if (store->bottom_field.used_for_reference)
                        pic_parms->ReferenceFrames[frame_idx].flags |= VA_PICTURE_H264_BOTTOM_FIELD;
                }
            }
        }
        frame_idx++;
    }

    /* set long term reference frames */
    for (buffer_idx = 0; buffer_idx < dpb->ltref_frames_in_buffer; buffer_idx++)
    {
        if (frame_idx >= 16 || buffer_idx >= 16)
        {
            WTRACE("Frame index is out of bound.");
            break;
        }
        store = &dpb->fs[dpb->fs_ltref_idc[buffer_idx]];
        if (!viddec_h264_get_is_long_term(store))
        {
            WTRACE("long term frame is not marked as long term.");
        }
        /*if (store->is_used == 3 && store->is_long_term && store->frame.used_for_reference == 3) */
        if (viddec_h264_get_is_used(store))
        {
            pic_parms->ReferenceFrames[frame_idx].flags = VA_PICTURE_H264_LONG_TERM_REFERENCE;
            if (FRAME == parser->info.img.structure)
            {
                pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = store->frame.poc;
                pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = store->frame.poc;
            }
            else
            {
                pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = store->top_field.poc;
                pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = store->bottom_field.poc;
                if (store->top_field.used_for_reference && store->bottom_field.used_for_reference)
                {
                    /* if both fields are used for reference, just set flag to be frame (0)*/
                }
                else
                {
                    if (store->top_field.used_for_reference)
                        pic_parms->ReferenceFrames[frame_idx].flags |= VA_PICTURE_H264_TOP_FIELD;
                    if (store->bottom_field.used_for_reference)
                        pic_parms->ReferenceFrames[frame_idx].flags |= VA_PICTURE_H264_BOTTOM_FIELD;
                }
            }
        }
        frame_idx++;
    }

    pic_parms->num_ref_frames = frame_idx;

    if (frame_idx > parser->info.active_SPS.num_ref_frames)
    {
        WTRACE("actual num_ref_frames (%d) exceeds the value in the sequence header (%d).",
               frame_idx, parser->info.active_SPS.num_ref_frames);
    }
}


static inline void vbp_set_scaling_list_h264(
    struct h264_viddec_parser *parser,
    VAIQMatrixBufferH264* IQ_matrix_buf)
{
    int i;
    int lists_to_set = 6 + 2 * (parser->info.active_PPS.transform_8x8_mode_flag ? 1 : 0);

    if (parser->info.active_PPS.pic_scaling_matrix_present_flag)
    {
        for (i = 0; i < lists_to_set; i++)
        {
            if (parser->info.active_PPS.pic_scaling_list_present_flag[i])
            {
                if (((i < 6) && parser->info.active_PPS.UseDefaultScalingMatrix4x4Flag[i]) ||
                        ((i >= 6) && parser->info.active_PPS.UseDefaultScalingMatrix8x8Flag[i-6]))
                {
                    /* use default scaling list */
                    if (i < 6)
                    {
                        memcpy(IQ_matrix_buf->ScalingList4x4[i], UseDefaultList[i], 16);
                    }
                    else
                    {
                        memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], UseDefaultList[i], 64);
                    }
                }
                else
                {
                    /* use PPS list */
                    if (i < 6)
                    {
                        memcpy(IQ_matrix_buf->ScalingList4x4[i], parser->info.active_PPS.ScalingList4x4[i], 16);
                    }
                    else
                    {
                        memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], parser->info.active_PPS.ScalingList8x8[i - 6], 64);
                    }
                }
            }
            else /* pic_scaling_list not present */
            {
                if (parser->info.active_SPS.seq_scaling_matrix_present_flag)
                {
                    /* SPS matrix present - use fallback rule B */
                    switch (i)
                    {
                    case 0:
                    case 3:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i],
                               parser->info.active_SPS.seq_scaling_list_present_flag[i] ? parser->info.active_PPS.ScalingList4x4[i] : UseDefaultList[i],
                               16);
                        break;

                    case 6:
                    case 7:
                        memcpy(IQ_matrix_buf->ScalingList8x8[i - 6],
                               parser->info.active_SPS.seq_scaling_list_present_flag[i] ? parser->info.active_PPS.ScalingList8x8[i - 6] : UseDefaultList[i],
                               64);
                        break;

                    case 1:
                    case 2:
                    case 4:
                    case 5:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i],
                               IQ_matrix_buf->ScalingList4x4[i - 1],
                               16);
                        break;

                    default:
                        //g_warning("invalid scaling list index.");
                        break;
                    }
                }
                else /* seq_scaling_matrix not present */
                {
                    /* SPS matrix not present - use fallback rule A */
                    switch (i)
                    {
                    case 0:
                    case 3:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i], UseDefaultList[i], 16);
                        break;

                    case 6:
                    case 7:
                        memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], UseDefaultList[i], 64);
                        break;

                    case 1:
                    case 2:
                    case 4:
                    case 5:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i],
                               IQ_matrix_buf->ScalingList4x4[i - 1],
                               16);
                        break;

                    default:
                        WTRACE("invalid scaling list index.");
                        break;
                    }
                } /* end of seq_scaling_matrix not present */
            } /* end of  pic_scaling_list not present */
        } /* for loop for each index from 0 to 7 */
    } /* end of pic_scaling_matrix present */
    else
    {
        /* PPS matrix not present, use SPS information */
        if (parser->info.active_SPS.seq_scaling_matrix_present_flag)
        {
            for (i = 0; i < lists_to_set; i++)
            {
                if (parser->info.active_SPS.seq_scaling_list_present_flag[i])
                {
                    if (((i < 6) && parser->info.active_SPS.UseDefaultScalingMatrix4x4Flag[i]) ||
                            ((i >= 6) && parser->info.active_SPS.UseDefaultScalingMatrix8x8Flag[i - 6]))
                    {
                        /* use default scaling list */
                        if (i < 6)
                        {
                            memcpy(IQ_matrix_buf->ScalingList4x4[i], UseDefaultList[i], 16);
                        }
                        else
                        {
                            memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], UseDefaultList[i], 64);
                        }
                    }
                    else
                    {
                        /* use SPS list */
                        if (i < 6)
                        {
                            memcpy(IQ_matrix_buf->ScalingList4x4[i], parser->info.active_SPS.ScalingList4x4[i], 16);
                        }
                        else
                        {
                            memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], parser->info.active_SPS.ScalingList8x8[i - 6], 64);
                        }
                    }
                }
                else
                {
                    /* SPS list not present - use fallback rule A */
                    switch (i)
                    {
                    case 0:
                    case 3:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i], UseDefaultList[i], 16);
                        break;

                    case 6:
                    case 7:
                        memcpy(IQ_matrix_buf->ScalingList8x8[i - 6], UseDefaultList[i], 64);
                        break;

                    case 1:
                    case 2:
                    case 4:
                    case 5:
                        memcpy(IQ_matrix_buf->ScalingList4x4[i],
                               IQ_matrix_buf->ScalingList4x4[i - 1],
                               16);
                        break;

                    default:
                        WTRACE("invalid scaling list index.");
                        break;
                    }
                }
            }
        }
        else
        {
            /* SPS matrix not present - use flat lists */
            for (i = 0; i < 6; i++)
            {
                memcpy(IQ_matrix_buf->ScalingList4x4[i], quant_flat, 16);
            }
            for (i = 0; i < 2; i++)
            {
                memcpy(IQ_matrix_buf->ScalingList8x8[i], quant8_flat, 64);
            }
        }
    }

    if ((0 == parser->info.active_PPS.transform_8x8_mode_flag) &&
            (parser->info.active_PPS.pic_scaling_matrix_present_flag ||
             parser->info.active_SPS.seq_scaling_matrix_present_flag))
    {
        for (i = 0; i < 2; i++)
        {
            memcpy(IQ_matrix_buf->ScalingList8x8[i], quant8_flat, 64);
        }
    }
}

static void vbp_set_codec_data_h264(
    struct h264_viddec_parser *parser,
     vbp_data_h264 *query_data)
{
    vbp_codec_data_h264* codec_data = query_data->codec_data;

    /* The following variables are used to detect if there is new SPS or PPS */
    uint8 seq_parameter_set_id = codec_data->seq_parameter_set_id;
    uint8 pic_parameter_set_id = codec_data->pic_parameter_set_id;
    int frame_width = codec_data->frame_width;
    int frame_height = codec_data->frame_height;

    /* parameter id */
    codec_data->seq_parameter_set_id = parser->info.active_SPS.seq_parameter_set_id;
    codec_data->pic_parameter_set_id = parser->info.active_PPS.pic_parameter_set_id;

    /* profile and level */
    codec_data->profile_idc = parser->info.active_SPS.profile_idc;
    codec_data->level_idc = parser->info.active_SPS.level_idc;


    /*constraint flag sets (h.264 Spec v2009)*/
    codec_data->constraint_set0_flag = (parser->info.active_SPS.constraint_set_flags & 0x10) >> 4;
    codec_data->constraint_set1_flag = (parser->info.active_SPS.constraint_set_flags & 0x8) >> 3;
    codec_data->constraint_set2_flag = (parser->info.active_SPS.constraint_set_flags & 0x4) >> 2;
    codec_data->constraint_set3_flag = (parser->info.active_SPS.constraint_set_flags & 0x2) >> 1;
    codec_data->constraint_set4_flag = parser->info.active_SPS.constraint_set_flags & 0x1;

    /* reference frames */
    codec_data->num_ref_frames = parser->info.active_SPS.num_ref_frames;

    if (!parser->info.active_SPS.sps_disp.frame_mbs_only_flag &&
        !parser->info.active_SPS.sps_disp.mb_adaptive_frame_field_flag)
    {
        /* no longer necessary: two fields share the same interlaced surface */
        /* codec_data->num_ref_frames *= 2; */
    }

    codec_data->gaps_in_frame_num_value_allowed_flag = parser->info.active_SPS.gaps_in_frame_num_value_allowed_flag;

    /* frame coding */
    codec_data->frame_mbs_only_flag = parser->info.active_SPS.sps_disp.frame_mbs_only_flag;
    codec_data->mb_adaptive_frame_field_flag = parser->info.active_SPS.sps_disp.mb_adaptive_frame_field_flag;

    /* frame dimension */
    codec_data->frame_width = (parser->info.active_SPS.sps_disp.pic_width_in_mbs_minus1 + 1 ) * 16;

    codec_data->frame_height = (2 - parser->info.active_SPS.sps_disp.frame_mbs_only_flag) *
                               (parser->info.active_SPS.sps_disp.pic_height_in_map_units_minus1 + 1) * 16;

    /* cropping information */
    codec_data->crop_left = 0;
    codec_data->crop_right = 0;
    codec_data->crop_top = 0;
    codec_data->crop_bottom = 0;
    if(parser->info.active_SPS.sps_disp.frame_cropping_flag) {
        int CropUnitX = 0, CropUnitY = 0, SubWidthC = 0, SubHeightC = 0;
        int ChromaArrayType = 0;
        if(parser->info.active_SPS.sps_disp.separate_colour_plane_flag == 0) {
            if(parser->info.active_SPS.sps_disp.chroma_format_idc == 1) {
                SubWidthC = 2;
                SubHeightC = 2;
            } else if( parser->info.active_SPS.sps_disp.chroma_format_idc == 2) {
                SubWidthC = 2;
                SubHeightC = 1;
            } else if( parser->info.active_SPS.sps_disp.chroma_format_idc == 3) {
                SubWidthC = 1;
                SubHeightC = 1;
            }
            ChromaArrayType = parser->info.active_SPS.sps_disp.chroma_format_idc;
        }

        if(ChromaArrayType == 0) {
            CropUnitX = 1;
            CropUnitY = 2 - parser->info.active_SPS.sps_disp.frame_mbs_only_flag;
        } else {
            CropUnitX = SubWidthC;
            CropUnitY = SubHeightC * ( 2 - parser->info.active_SPS.sps_disp.frame_mbs_only_flag);
        }

        codec_data->crop_left = CropUnitX * parser->info.active_SPS.sps_disp.frame_crop_rect_left_offset;
        codec_data->crop_right = CropUnitX * parser->info.active_SPS.sps_disp.frame_crop_rect_right_offset; // + 1;
        codec_data->crop_top = CropUnitY * parser->info.active_SPS.sps_disp.frame_crop_rect_top_offset;
        codec_data->crop_bottom = CropUnitY * parser->info.active_SPS.sps_disp.frame_crop_rect_bottom_offset; // + 1;
    }

    /* aspect ratio */
    if (parser->info.active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_info_present_flag)
    {
        codec_data->aspect_ratio_idc =
            parser->info.active_SPS.sps_disp.vui_seq_parameters.aspect_ratio_idc;

        if (codec_data->aspect_ratio_idc < 17)
        {
            codec_data->sar_width = h264_aspect_ratio_table[codec_data->aspect_ratio_idc][0];
            codec_data->sar_height = h264_aspect_ratio_table[codec_data->aspect_ratio_idc][1];
        }
        else if (codec_data->aspect_ratio_idc == 255)
        {
            codec_data->sar_width =
                parser->info.active_SPS.sps_disp.vui_seq_parameters.sar_width;

            codec_data->sar_height =
                parser->info.active_SPS.sps_disp.vui_seq_parameters.sar_height;
        }
        else
        {
            codec_data->sar_width = 0;
            codec_data->sar_height = 0;
        }
    }
    else
    {
        // unspecified
        codec_data->aspect_ratio_idc = 0;
        codec_data->sar_width = 0;
        codec_data->sar_height = 0;
    }

    /* video format */
    if (parser->info.active_SPS.sps_disp.vui_seq_parameters.video_signal_type_present_flag)
    {
        codec_data->video_format =
            parser->info.active_SPS.sps_disp.vui_seq_parameters.video_format;
    }
    else
    {
        // Unspecified video format
        codec_data->video_format = 5;
    }

    codec_data->video_full_range_flag =
        parser->info.active_SPS.sps_disp.vui_seq_parameters.video_full_range_flag;


    if (parser->info.active_SPS.sps_disp.vui_seq_parameters.colour_description_present_flag)
    {
        codec_data->matrix_coefficients =
            parser->info.active_SPS.sps_disp.vui_seq_parameters.matrix_coefficients;
    }
    else
    {
        // Unspecified
        codec_data->matrix_coefficients = 2;
    }

    codec_data->bit_rate = parser->info.active_SPS.sps_disp.vui_seq_parameters.bit_rate_value;

    /* picture order type and count */
    codec_data->log2_max_pic_order_cnt_lsb_minus4 = parser->info.active_SPS.log2_max_pic_order_cnt_lsb_minus4;
    codec_data->pic_order_cnt_type = parser->info.active_SPS.pic_order_cnt_type;


    /* udpate sps and pps status */
    query_data->new_sps = 0;
    query_data->new_pps = 0;
    if (query_data->has_sps && query_data->has_pps)
    {
        query_data->new_sps = (seq_parameter_set_id != parser->info.active_PPS.seq_parameter_set_id) ? 1 : 0;
        query_data->new_pps = (pic_parameter_set_id != parser->info.active_PPS.pic_parameter_set_id) ? 1 : 0;
    }
    query_data->has_sps = parser->info.active_SPS.seq_parameter_set_id != 0xff;
    query_data->has_pps = parser->info.active_PPS.seq_parameter_set_id != 0xff;
    if (frame_width != codec_data->frame_width || frame_height != codec_data->frame_height)
    {
        query_data->new_sps = 1;
        query_data->new_pps = 1;
    }
}


static uint32_t vbp_add_pic_data_h264(vbp_context *pcontext, int list_index)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;

    vbp_data_h264 *query_data = (vbp_data_h264 *)pcontext->query_data;
    struct h264_viddec_parser* parser = NULL;
    vbp_picture_data_h264* pic_data = NULL;
    VAPictureParameterBufferH264* pic_parms = NULL;

    parser = (struct h264_viddec_parser *)cxt->codec_data;

    if (0 == parser->info.SliceHeader.first_mb_in_slice)
    {
        /* a new picture is parsed */
        query_data->num_pictures++;
    }

    if (query_data->num_pictures == 0)
    {
        /* partial frame */
        query_data->num_pictures = 1;
    }

    if (query_data->num_pictures > MAX_NUM_PICTURES)
    {
        ETRACE("num of pictures exceeds the limit (%d).", MAX_NUM_PICTURES);
        return VBP_DATA;
    }

    int pic_data_index = query_data->num_pictures - 1;
    if (pic_data_index < 0)
    {
        WTRACE("MB address does not start from 0!");
        return VBP_DATA;
    }

    pic_data = &(query_data->pic_data[pic_data_index]);
    pic_parms = pic_data->pic_parms;

    // relax this condition to support partial frame parsing

    //if (parser->info.SliceHeader.first_mb_in_slice == 0)
    {
        /**
        * picture parameter only needs to be set once,
        * even multiple slices may be encoded
        */

        /* VAPictureParameterBufferH264 */
        pic_parms->CurrPic.picture_id = VA_INVALID_SURFACE;
        pic_parms->CurrPic.frame_idx = 0;
        if (parser->info.img.field_pic_flag == 1)
        {
            if (parser->info.img.bottom_field_flag)
            {
                pic_parms->CurrPic.flags = VA_PICTURE_H264_BOTTOM_FIELD;
            }
            else
            {
                /* also OK set to 0 (from test suite) */
                pic_parms->CurrPic.flags = VA_PICTURE_H264_TOP_FIELD;
            }
        }
        else
        {
            pic_parms->CurrPic.flags = 0; /* frame picture */
        }
        pic_parms->CurrPic.TopFieldOrderCnt = parser->info.img.toppoc;
        pic_parms->CurrPic.BottomFieldOrderCnt = parser->info.img.bottompoc;
        pic_parms->CurrPic.frame_idx = parser->info.SliceHeader.frame_num;

        /* don't care if current frame is used as long term reference */
        if (parser->info.SliceHeader.nal_ref_idc != 0)
        {
            pic_parms->CurrPic.flags |= VA_PICTURE_H264_SHORT_TERM_REFERENCE;
        }

        pic_parms->picture_width_in_mbs_minus1 = parser->info.active_SPS.sps_disp.pic_width_in_mbs_minus1;

        /* frame height in MBS */
        pic_parms->picture_height_in_mbs_minus1 = (2 - parser->info.active_SPS.sps_disp.frame_mbs_only_flag) *
                (parser->info.active_SPS.sps_disp.pic_height_in_map_units_minus1 + 1) - 1;

        pic_parms->bit_depth_luma_minus8 = parser->info.active_SPS.bit_depth_luma_minus8;
        pic_parms->bit_depth_chroma_minus8 = parser->info.active_SPS.bit_depth_chroma_minus8;


        pic_parms->seq_fields.value = 0;
        pic_parms->seq_fields.bits.chroma_format_idc = parser->info.active_SPS.sps_disp.chroma_format_idc;
        pic_parms->seq_fields.bits.residual_colour_transform_flag = parser->info.active_SPS.residual_colour_transform_flag;
        pic_parms->seq_fields.bits.frame_mbs_only_flag = parser->info.active_SPS.sps_disp.frame_mbs_only_flag;
        pic_parms->seq_fields.bits.mb_adaptive_frame_field_flag = parser->info.active_SPS.sps_disp.mb_adaptive_frame_field_flag;
        pic_parms->seq_fields.bits.direct_8x8_inference_flag = parser->info.active_SPS.sps_disp.direct_8x8_inference_flag;

        /* new fields in libva 0.31 */
        pic_parms->seq_fields.bits.gaps_in_frame_num_value_allowed_flag = parser->info.active_SPS.gaps_in_frame_num_value_allowed_flag;
        pic_parms->seq_fields.bits.log2_max_frame_num_minus4 = parser->info.active_SPS.log2_max_frame_num_minus4;
        pic_parms->seq_fields.bits.pic_order_cnt_type = parser->info.active_SPS.pic_order_cnt_type;
        pic_parms->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 = parser->info.active_SPS.log2_max_pic_order_cnt_lsb_minus4;
        pic_parms->seq_fields.bits.delta_pic_order_always_zero_flag =parser->info.active_SPS.delta_pic_order_always_zero_flag;


        /* referened from UMG_Moorstown_TestSuites */
        pic_parms->seq_fields.bits.MinLumaBiPredSize8x8 = (parser->info.active_SPS.level_idc > 30) ? 1 : 0;

        pic_parms->num_slice_groups_minus1 = parser->info.active_PPS.num_slice_groups_minus1;
        pic_parms->slice_group_map_type = parser->info.active_PPS.slice_group_map_type;
        pic_parms->slice_group_change_rate_minus1 = 0;
        pic_parms->pic_init_qp_minus26 = parser->info.active_PPS.pic_init_qp_minus26;
        pic_parms->pic_init_qs_minus26 = 0;
        pic_parms->chroma_qp_index_offset = parser->info.active_PPS.chroma_qp_index_offset;
        pic_parms->second_chroma_qp_index_offset = parser->info.active_PPS.second_chroma_qp_index_offset;

        pic_parms->pic_fields.value = 0;
        pic_parms->pic_fields.bits.entropy_coding_mode_flag = parser->info.active_PPS.entropy_coding_mode_flag;
        pic_parms->pic_fields.bits.weighted_pred_flag = parser->info.active_PPS.weighted_pred_flag;
        pic_parms->pic_fields.bits.weighted_bipred_idc = parser->info.active_PPS.weighted_bipred_idc;
        pic_parms->pic_fields.bits.transform_8x8_mode_flag = parser->info.active_PPS.transform_8x8_mode_flag;

        /* new LibVA fields in v0.31*/
        pic_parms->pic_fields.bits.pic_order_present_flag = parser->info.active_PPS.pic_order_present_flag;
        pic_parms->pic_fields.bits.deblocking_filter_control_present_flag = parser->info.active_PPS.deblocking_filter_control_present_flag;
        pic_parms->pic_fields.bits.redundant_pic_cnt_present_flag = parser->info.active_PPS.redundant_pic_cnt_present_flag;
        pic_parms->pic_fields.bits.reference_pic_flag = parser->info.SliceHeader.nal_ref_idc != 0;

        /* all slices in the pciture have the same field_pic_flag */
        pic_parms->pic_fields.bits.field_pic_flag = parser->info.SliceHeader.field_pic_flag;
        pic_parms->pic_fields.bits.constrained_intra_pred_flag = parser->info.active_PPS.constrained_intra_pred_flag;

        pic_parms->frame_num = parser->info.SliceHeader.frame_num;
    }


    /* set reference frames, and num_ref_frames */
    vbp_set_reference_frames_h264(parser, pic_parms);
    if (parser->info.nal_unit_type == h264_NAL_UNIT_TYPE_IDR)
    {
        int frame_idx;
        for (frame_idx = 0; frame_idx < 16; frame_idx++)
        {
            pic_parms->ReferenceFrames[frame_idx].picture_id = VA_INVALID_SURFACE;
            pic_parms->ReferenceFrames[frame_idx].frame_idx = 0;
            pic_parms->ReferenceFrames[frame_idx].flags = VA_PICTURE_H264_INVALID;
            pic_parms->ReferenceFrames[frame_idx].TopFieldOrderCnt = 0;
            pic_parms->ReferenceFrames[frame_idx].BottomFieldOrderCnt = 0;
        }
        /* num of reference frame is 0 if current picture is IDR */
        pic_parms->num_ref_frames = 0;
    }
    else
    {
        /* actual num_ref_frames is set in vbp_set_reference_frames_h264 */
    }

    return VBP_OK;
}

static uint32_t vbp_add_slice_data_h264(vbp_context *pcontext, int index)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    uint32 bit, byte;
    uint8 is_emul;

    vbp_data_h264 *query_data = (vbp_data_h264 *)pcontext->query_data;
    VASliceParameterBufferH264 *slc_parms = NULL;
    vbp_slice_data_h264 *slc_data = NULL;
    struct h264_viddec_parser* h264_parser = NULL;
    h264_Slice_Header_t* slice_header = NULL;
    vbp_picture_data_h264* pic_data = NULL;


    h264_parser = (struct h264_viddec_parser *)cxt->codec_data;
    int pic_data_index = query_data->num_pictures - 1;
    if (pic_data_index < 0)
    {
        ETRACE("invalid picture data index.");
        return VBP_DATA;
    }

    pic_data = &(query_data->pic_data[pic_data_index]);

    slc_data = &(pic_data->slc_data[pic_data->num_slices]);
    slc_data->buffer_addr = cxt->parse_cubby.buf;
    slc_parms = &(slc_data->slc_parms);

    /* byte: how many bytes have been parsed */
    /* bit: bits parsed within the current parsing position */
    viddec_pm_get_au_pos(cxt, &bit, &byte, &is_emul);

    slc_data->nal_unit_type = h264_parser->info.nal_unit_type;

    slc_parms->slice_data_size = slc_data->slice_size =
                                     pcontext->parser_cxt->list.data[index].edpos -
                                     pcontext->parser_cxt->list.data[index].stpos;

    /* the offset to the NAL start code for this slice */
    slc_data->slice_offset = cxt->list.data[index].stpos;
    slc_parms->slice_data_offset = 0;

    /* whole slice is in this buffer */
    slc_parms->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

    /* bit offset from NAL start code to the beginning of slice data */
    slc_parms->slice_data_bit_offset = bit + byte * 8;

    if (is_emul)
    {
        WTRACE("next byte is emulation prevention byte.");
        /*slc_parms->slice_data_bit_offset += 8; */
    }

    if (cxt->getbits.emulation_byte_counter != 0)
    {
        slc_parms->slice_data_bit_offset -= cxt->getbits.emulation_byte_counter * 8;
    }

    slice_header = &(h264_parser->info.SliceHeader);
    slc_parms->first_mb_in_slice = slice_header->first_mb_in_slice;

    if (h264_parser->info.active_SPS.sps_disp.mb_adaptive_frame_field_flag &
            (!(h264_parser->info.SliceHeader.field_pic_flag)))
    {
        slc_parms->first_mb_in_slice /= 2;
    }

    slc_parms->slice_type = slice_header->slice_type;

    slc_parms->direct_spatial_mv_pred_flag = slice_header->direct_spatial_mv_pred_flag;

    slc_parms->num_ref_idx_l0_active_minus1 = 0;
    slc_parms->num_ref_idx_l1_active_minus1 = 0;
    if (slice_header->slice_type == h264_PtypeI)
    {
    }
    else if (slice_header->slice_type == h264_PtypeP)
    {
        slc_parms->num_ref_idx_l0_active_minus1 = slice_header->num_ref_idx_l0_active - 1;
    }
    else if (slice_header->slice_type == h264_PtypeB)
    {
        slc_parms->num_ref_idx_l0_active_minus1 = slice_header->num_ref_idx_l0_active - 1;
        slc_parms->num_ref_idx_l1_active_minus1 = slice_header->num_ref_idx_l1_active - 1;
    }
    else
    {
        WTRACE("slice type %d is not supported.", slice_header->slice_type);
    }

    slc_parms->cabac_init_idc = slice_header->cabac_init_idc;
    slc_parms->slice_qp_delta = slice_header->slice_qp_delta;
    slc_parms->disable_deblocking_filter_idc = slice_header->disable_deblocking_filter_idc;
    slc_parms->slice_alpha_c0_offset_div2 = slice_header->slice_alpha_c0_offset_div2;
    slc_parms->slice_beta_offset_div2 = slice_header->slice_beta_offset_div2;


    vbp_set_pre_weight_table_h264(h264_parser, slc_parms);
    vbp_set_slice_ref_list_h264(h264_parser, slc_parms);


    pic_data->num_slices++;

    //vbp_update_reference_frames_h264_methodB(pic_data);
    if (pic_data->num_slices > MAX_NUM_SLICES)
    {
        ETRACE("number of slices per picture exceeds the limit (%d).", MAX_NUM_SLICES);
        return VBP_DATA;
    }

    /*if (pic_data->num_slices > 1)
    {
        ITRACE("number of slices per picture is %d.", pic_data->num_slices);
    }*/
    return VBP_OK;
}

/**
* parse decoder configuration data
*/
uint32 vbp_parse_init_data_h264(vbp_context* pcontext)
{
    /* parsing AVCDecoderConfigurationRecord structure (see MPEG-4 part 15 spec) */

    uint8 configuration_version = 0;
    uint8 AVC_profile_indication = 0;
    uint8 profile_compatibility = 0;
    uint8 AVC_level_indication = 0;
    uint8 length_size_minus_one = 0;
    uint8 num_of_sequence_parameter_sets = 0;
    uint8 num_of_picture_parameter_sets = 0;
    uint16 sequence_parameter_set_length = 0;
    uint16 picture_parameter_set_length = 0;

    int i = 0;
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;

    vbp_h264_parser_private *parser_private = (vbp_h264_parser_private *)pcontext->parser_private;
    //Enable emulation prevention
    cxt->getbits.is_emul_reqd = 1;

    /* check if configuration data is start code prefix */
    viddec_sc_parse_cubby_cxt_t cubby = cxt->parse_cubby;
    viddec_parser_ops_t *ops = pcontext->parser_ops;
    int ret = ops->parse_sc((void *)&cubby,
                            NULL, /* context, not used */
                            &(cxt->sc_prefix_info));
    if (ret == 1)
    {
        WTRACE("configuration data is start-code prefixed.\n");
        parser_private->bitstream_pattern = H264_BS_SC_PREFIXED;
        return vbp_parse_start_code_h264(pcontext);
    }


    uint8* cur_data = cxt->parse_cubby.buf;


    if (cxt->parse_cubby.size < 6)
    {
        /* need at least 6 bytes to start parsing the structure, see spec 15 */
        return VBP_DATA;
    }

    configuration_version = *cur_data++;
    AVC_profile_indication = *cur_data++;

    /*ITRACE("Profile indication: %d", AVC_profile_indication); */

    profile_compatibility = *cur_data++;
    AVC_level_indication = *cur_data++;

    /* ITRACE("Level indication: %d", AVC_level_indication);*/
    /* 2 bits of length_size_minus_one, 6 bits of reserved (11111) */
    length_size_minus_one = (*cur_data) & 0x3;

    if (length_size_minus_one != 3)
    {
        WTRACE("length size (%d) is not equal to 4.", length_size_minus_one + 1);
    }

    parser_private->NAL_length_size = length_size_minus_one + 1;

    cur_data++;

    /* 3 bits of reserved (111) and 5 bits of num_of_sequence_parameter_sets */
    num_of_sequence_parameter_sets = (*cur_data) & 0x1f;
    if (num_of_sequence_parameter_sets > 1)
    {
        WTRACE("num_of_sequence_parameter_sets is %d.", num_of_sequence_parameter_sets);
    }
    if (num_of_sequence_parameter_sets > MAX_NUM_SPS)
    {
        /* this would never happen as MAX_NUM_SPS = 32 */
        WTRACE("num_of_sequence_parameter_sets (%d) exceeds the limit (%d).", num_of_sequence_parameter_sets, MAX_NUM_SPS);
    }
    cur_data++;

    cxt->list.num_items = 0;
    for (i = 0; i < num_of_sequence_parameter_sets; i++)
    {
        if (cur_data - cxt->parse_cubby.buf + 2 > cxt->parse_cubby.size)
        {
            /* need at least 2 bytes to parse sequence_parameter_set_length */
            ETRACE("Not enough data to parse SPS length.");
            return VBP_DATA;
        }

        /* 16 bits */
        sequence_parameter_set_length = vbp_utils_ntohs(cur_data);


        cur_data += 2;

        if (cur_data - cxt->parse_cubby.buf + sequence_parameter_set_length > cxt->parse_cubby.size)
        {
            /* need at least sequence_parameter_set_length bytes for SPS */
            ETRACE("Not enough data to parse SPS.");
            return VBP_DATA;
        }

        cxt->list.data[cxt->list.num_items].stpos = cur_data - cxt->parse_cubby.buf;

        /* end pos is exclusive */
        cxt->list.data[cxt->list.num_items].edpos =
            cxt->list.data[cxt->list.num_items].stpos + sequence_parameter_set_length;

        cxt->list.num_items++;

        cur_data += sequence_parameter_set_length;
    }

    if (cur_data - cxt->parse_cubby.buf + 1 > cxt->parse_cubby.size)
    {
        /* need at least one more byte to parse num_of_picture_parameter_sets */
        ETRACE("Not enough data to parse number of PPS.");
        return VBP_DATA;
    }

    num_of_picture_parameter_sets = *cur_data++;
    if (num_of_picture_parameter_sets > 1)
    {
        /* g_warning("num_of_picture_parameter_sets is %d.", num_of_picture_parameter_sets); */
    }

    for (i = 0; i < num_of_picture_parameter_sets; i++)
    {
        if (cur_data - cxt->parse_cubby.buf + 2 > cxt->parse_cubby.size)
        {
            /* need at least 2 bytes to parse picture_parameter_set_length */
            ETRACE("Not enough data to parse PPS length.");
            return VBP_DATA;
        }

        /* 16 bits */
        picture_parameter_set_length = vbp_utils_ntohs(cur_data);

        cur_data += 2;

        if (cur_data - cxt->parse_cubby.buf + picture_parameter_set_length > cxt->parse_cubby.size)
        {
            /* need at least picture_parameter_set_length bytes for PPS */
            ETRACE("Not enough data to parse PPS.");
            return VBP_DATA;
        }

        cxt->list.data[cxt->list.num_items].stpos = cur_data - cxt->parse_cubby.buf;

        /* end pos is exclusive */
        cxt->list.data[cxt->list.num_items].edpos =
            cxt->list.data[cxt->list.num_items].stpos + picture_parameter_set_length;

        cxt->list.num_items++;

        cur_data += picture_parameter_set_length;
    }

    if ((cur_data - cxt->parse_cubby.buf) !=  cxt->parse_cubby.size)
    {
        WTRACE("Not all initialization data is parsed. Size = %d, parsed = %d.",
               cxt->parse_cubby.size, (cur_data - cxt->parse_cubby.buf));
    }

    parser_private->bitstream_pattern = H264_BS_LENGTH_PREFIXED;
    return VBP_OK;
}

static inline uint32_t vbp_get_NAL_length_h264(uint8_t* p, int *NAL_length_size)
{
    switch (*NAL_length_size)
    {
    case 4:
        return vbp_utils_ntohl(p);

    case 3:
    {
        uint32_t i = ((*p) << 16) + ((*(p+1)) << 8) + ((*(p+2)));
        return i;
    }

    case 2:
        return vbp_utils_ntohs(p);

    case 1:
        return *p;

    default:
        WTRACE("invalid NAL_length_size: %d.", NAL_length_size);
        /* default to 4 bytes for length */
        *NAL_length_size = 4;
        return vbp_utils_ntohl(p);
    }
}

/**
** H.264 elementary stream does not have start code.
* instead, it is comprised of size of NAL unit and payload
* of NAL unit. See spec 15 (Sample format)
*/

/* Start code prefix is 001 which is 3 bytes. */
#define H264_SC_SIZE 3
uint32 vbp_parse_start_code_h264(vbp_context *pcontext)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    vbp_h264_parser_private *parser_private = (vbp_h264_parser_private *)pcontext->parser_private;

    /* reset query data for the new sample buffer */
    vbp_data_h264* query_data = (vbp_data_h264*)pcontext->query_data;
    int i;

    for (i = 0; i < MAX_NUM_PICTURES; i++)
    {
        query_data->pic_data[i].num_slices = 0;
    }
    query_data->num_pictures = 0;

    cxt->list.num_items = 0;

    /* reset start position of first item to 0 in case there is only one item */
    cxt->list.data[0].stpos = 0;

    /* start code emulation prevention byte is present in NAL */
    cxt->getbits.is_emul_reqd = 1;

    if (parser_private->bitstream_pattern == H264_BS_LENGTH_PREFIXED)
    {
        viddec_sc_parse_cubby_cxt_t* cubby = NULL;
        int32_t size_left = 0;
        int32_t size_parsed = 0;
        int32_t NAL_length = 0;

        cubby = &(cxt->parse_cubby);

        size_left = cubby->size;

        while (size_left >= parser_private->NAL_length_size)
        {
            NAL_length = vbp_get_NAL_length_h264(cubby->buf + size_parsed, &parser_private->NAL_length_size);
            if (NAL_length <= 0 || NAL_length > size_left - parser_private->NAL_length_size)
            {
                ETRACE("Invalid NAL_length parsed.");
                break;
            }

            size_parsed += parser_private->NAL_length_size;
            cxt->list.data[cxt->list.num_items].stpos = size_parsed;
            size_parsed += NAL_length; /* skip NAL bytes */
            /* end position is exclusive */
            cxt->list.data[cxt->list.num_items].edpos = size_parsed;
            cxt->list.num_items++;
            if (cxt->list.num_items >= MAX_IBUFS_PER_SC)
            {
                ETRACE("num of list items exceeds the limit (%d).", MAX_IBUFS_PER_SC);
                break;
            }

            size_left = cubby->size - size_parsed;
        }

        if (size_left != 0 && parser_private->length_prefix_verified == 0)
        {
            WTRACE("Elementary stream is not aligned (%d).", size_left);

            /* attempt to correct length prefix to start-code prefix only once, if it succeeds, we will
                    * alway treat bit stream as start-code prefixed; otherwise, treat bit stream as length prefixed
                    */
            parser_private->length_prefix_verified = 1;
            viddec_sc_parse_cubby_cxt_t temp_cubby = cxt->parse_cubby;

            viddec_parser_ops_t *ops = pcontext->parser_ops;
            int ret = ops->parse_sc((void *)&temp_cubby,
                                    NULL, /* context, not used */
                                    &(cxt->sc_prefix_info));

            /* found start code */
            if (ret == 1)
            {
                WTRACE("Stream was supposed to be length prefixed, but actually is start-code prefixed.");
                parser_private->NAL_length_size = 0;
                parser_private->bitstream_pattern = H264_BS_SC_PREFIXED;
                /* reset parsing data */
                for (i = 0; i < MAX_NUM_PICTURES; i++)
                {
                    query_data->pic_data[i].num_slices = 0;
                }
                query_data->num_pictures = 0;
                cxt->list.num_items = 0;
            }
        }
    }


    if (parser_private->bitstream_pattern == H264_BS_SC_PREFIXED)
    {
        viddec_sc_parse_cubby_cxt_t cubby;
        /*  memory copy without updating cxt->parse_cubby */
        cubby = cxt->parse_cubby;
        viddec_parser_ops_t *ops = pcontext->parser_ops;
        int ret = 0;

        while (1)
        {
            ret = ops->parse_sc((void *)&cubby,
                                NULL, /* context, not used */
                                &(cxt->sc_prefix_info));
            if (ret == 1)
            {
                if (cxt->list.num_items == 0)
                {
                    cxt->list.data[0].stpos = cubby.sc_end_pos;
                }
                else
                {
                    cxt->list.data[cxt->list.num_items].stpos =
                        cubby.sc_end_pos + cxt->list.data[cxt->list.num_items - 1].stpos;
                    cxt->list.data[cxt->list.num_items - 1].edpos = cxt->list.data[cxt->list.num_items].stpos - H264_SC_SIZE;
                }

                cubby.phase = 0;
                cubby.buf = cxt->parse_cubby.buf +
                            cxt->list.data[cxt->list.num_items].stpos;

                cubby.size = cxt->parse_cubby.size -
                             cxt->list.data[cxt->list.num_items].stpos;

                cxt->list.num_items++;
                if (cxt->list.num_items >= MAX_IBUFS_PER_SC)
                {
                    WTRACE("Num items exceeds the limit!");
                    /* not fatal, just stop parsing */
                    break;
                }
            }
            else
            {
                if (cxt->list.num_items == 0)
                {
                    cxt->list.num_items = 1;
                    parser_private->bitstream_pattern = H264_BS_SINGLE_NAL;
                    WTRACE("Stream was supposed to be SC prefixed, but actually contains a single NAL.");
                }
                cxt->list.data[cxt->list.num_items - 1].edpos = cxt->parse_cubby.size;
                break;
            }
        }

    }

    if (parser_private->bitstream_pattern == H264_BS_SINGLE_NAL)
    {
        cxt->list.num_items = 1;
        cxt->list.data[0].stpos = 0;
        cxt->list.data[0].edpos = cxt->parse_cubby.size;
    }


    return VBP_OK;
}

/**
*
* process parsing result after a NAL unit is parsed
*
*/
uint32 vbp_process_parsing_result_h264( vbp_context *pcontext, int i)
{
    if (i >= MAX_NUM_SLICES)
    {
        return VBP_PARM;
    }

    uint32 error = VBP_OK;

    struct h264_viddec_parser* parser = NULL;
    parser = (struct h264_viddec_parser *)&( pcontext->parser_cxt->codec_data[0]);
    vbp_data_h264* query_data = (vbp_data_h264 *)pcontext->query_data;
    switch (parser->info.nal_unit_type)
    {
    case h264_NAL_UNIT_TYPE_SLICE:
        //ITRACE("slice header is parsed.");
        error = vbp_add_pic_data_h264(pcontext, i);
        if (VBP_OK == error)
        {
            error = vbp_add_slice_data_h264(pcontext, i);
        }
        break;

    case  h264_NAL_UNIT_TYPE_IDR:
        //ITRACE("IDR header is parsed.");
        error = vbp_add_pic_data_h264(pcontext, i);
        if (VBP_OK == error)
        {
            error = vbp_add_slice_data_h264(pcontext, i);
        }
        break;
    case h264_NAL_UNIT_TYPE_SEI:
        //ITRACE("SEI header is parsed.");
        break;

    case h264_NAL_UNIT_TYPE_SPS:
        ITRACE("SPS header is parsed.");
        break;

    case h264_NAL_UNIT_TYPE_PPS:
        ITRACE("PPS header is parsed.");
        break;

    case h264_NAL_UNIT_TYPE_Acc_unit_delimiter:
        //ITRACE("ACC unit delimiter is parsed.");
        break;

    case h264_NAL_UNIT_TYPE_EOSeq:
        ITRACE("EOSeq is parsed.");
        break;

    case h264_NAL_UNIT_TYPE_EOstream:
        ITRACE("EOStream is parsed");
        break;

    default:
        WTRACE("unknown header %d is parsed.", parser->info.nal_unit_type);
        break;
    }

    if (query_data->num_pictures == MAX_NUM_PICTURES && parser->info.img.field_pic_flag != 1)
    {
        WTRACE("more than one frame in the buffer is found(%d)", query_data->num_pictures);
        return (error == VBP_OK ? VBP_MULTI : error);
    }
    return error;
}

/*
*
* fill query data structure after sample buffer is parsed
*
*/
uint32 vbp_populate_query_data_h264(vbp_context *pcontext)
{
    vbp_data_h264 *query_data = NULL;
    struct h264_viddec_parser *parser = NULL;
    struct vbp_h264_parser_private_t* private = NULL;

    parser = (struct h264_viddec_parser *)pcontext->parser_cxt->codec_data;
    query_data = (vbp_data_h264 *)pcontext->query_data;
    private = (struct vbp_h264_parser_private_t *)pcontext->parser_private;

    vbp_set_codec_data_h264(parser, query_data);

    /* buffer number */
    query_data->buf_number = buffer_counter;

    /* VQIAMatrixBufferH264 */
    vbp_set_scaling_list_h264(parser, query_data->IQ_matrix_buf);

    if (query_data->num_pictures > 0)
    {
        /*
        * picture parameter buffer and slice parameter buffer have been populated
        */
    }
    else
    {
        /**
        * add a dummy picture that contains picture parameters parsed
          from SPS and PPS.
        */
        vbp_add_pic_data_h264(pcontext, 0);
    }

    return VBP_OK;
}



