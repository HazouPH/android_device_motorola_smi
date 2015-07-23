/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
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

#include "vp8.h"
#include "vbp_loader.h"
#include "vbp_utils.h"
#include "vbp_vp8_parser.h"

uint32 vbp_init_parser_entries_vp8(vbp_context *pcontext)
{
    if (NULL == pcontext->parser_ops)
    {
        return VBP_PARM;
    }

    pcontext->parser_ops->init = dlsym(pcontext->fd_parser, "viddec_vp8_init");
    if (NULL == pcontext->parser_ops->init)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->parse_sc = NULL;

    pcontext->parser_ops->parse_syntax = dlsym(pcontext->fd_parser, "viddec_vp8_parse");
    if (NULL == pcontext->parser_ops->parse_syntax)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->get_cxt_size = dlsym(pcontext->fd_parser, "viddec_vp8_get_context_size");
    if (NULL == pcontext->parser_ops->get_cxt_size)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->is_wkld_done = NULL;

    /* entry point not needed */
    pcontext->parser_ops->is_frame_start = NULL;

    pcontext->parser_ops->flush = NULL;

    return VBP_OK;
}

uint32 vbp_allocate_query_data_vp8(vbp_context *pcontext)
{
    if (NULL != pcontext->query_data)
    {
        return VBP_PARM;
    }

    vbp_data_vp8 *query_data = vbp_malloc_set0(vbp_data_vp8, 1);
    if (NULL == query_data)
    {
        goto cleanup;
    }

    /* assign the pointer */
    pcontext->query_data = (void *)query_data;

    query_data->pic_data = vbp_malloc_set0(vbp_picture_data_vp8, VP8_MAX_NUM_PICTURES);
    if (NULL == query_data->pic_data)
    {
        goto cleanup;
    }

    int i = 0;
    for (i = 0; i < VP8_MAX_NUM_PICTURES; i++)
    {
        query_data->pic_data[i].pic_parms = vbp_malloc_set0(VAPictureParameterBufferVP8, 1);
        if (NULL == query_data->pic_data[i].pic_parms)
        {
            goto cleanup;
        }
        query_data->pic_data[i].num_slices = 0;
        query_data->pic_data[i].slc_data = vbp_malloc_set0(vbp_slice_data_vp8, VP8_MAX_NUM_SLICES);
        if (NULL == query_data->pic_data[i].slc_data)
        {
            goto cleanup;
        }
    }

    query_data->codec_data = vbp_malloc_set0(vbp_codec_data_vp8, 1);
    if (NULL == query_data->codec_data)
    {
        goto cleanup;
    }

    query_data->prob_data = vbp_malloc_set0(VAProbabilityDataBufferVP8, 1);
    if (NULL == query_data->prob_data)
    {
        goto cleanup;
    }

    query_data->IQ_matrix_buf = vbp_malloc_set0(VAIQMatrixBufferVP8, 1);
    if (NULL == query_data->IQ_matrix_buf)
    {
        goto cleanup;
    }

    pcontext->parser_private = NULL;

    return VBP_OK;

cleanup:
    vbp_free_query_data_vp8(pcontext);

    return VBP_MEM;
}

uint32 vbp_free_query_data_vp8(vbp_context *pcontext)
{
    if (NULL == pcontext->query_data)
    {
        return VBP_OK;
    }

    vbp_data_vp8 *query_data = (vbp_data_vp8 *)pcontext->query_data;
    if (query_data->pic_data)
    {
        int i = 0;
        for (i = 0; i < VP8_MAX_NUM_PICTURES; i++)
        {
            if (query_data->pic_data[i].pic_parms)
            {
                free(query_data->pic_data[i].pic_parms);
                query_data->pic_data[i].pic_parms = NULL;
            }
            if (query_data->pic_data[i].slc_data)
            {
                free(query_data->pic_data[i].slc_data);
                query_data->pic_data[i].slc_data = NULL;
            }
        }
        free(query_data->pic_data);
        query_data->pic_data = NULL;
    }

    if (query_data->codec_data)
    {
        free(query_data->codec_data);
        query_data->codec_data = NULL;
    }

    if (query_data->prob_data)
    {
        free(query_data->prob_data);
        query_data->prob_data = NULL;
    }

    if (query_data->IQ_matrix_buf)
    {
        free(query_data->IQ_matrix_buf);
        query_data->IQ_matrix_buf = NULL;
    }

    free(query_data);
    pcontext->query_data = NULL;

    return VBP_OK;
}


/**
* parse decoder configuration data
*/
uint32 vbp_parse_init_data_vp8(vbp_context* pcontext)
{
    // could never be there
    return VBP_OK;
}

uint32 vbp_parse_start_code_vp8(vbp_context *pcontext)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    uint8 *buf = cxt->parse_cubby.buf;
    uint32 length = cxt->parse_cubby.size;
    if (length < 3)
    {
        return VBP_DATA;
    }

    // check whether it is a key frame
    if ((length >= 10) && !(buf[0] & 0x01))
    {
        uint8 *c = buf + 3;

        // check start code
        if ((c[0] != 0x9d) || (c[1] != 0x01) || (c[2] != 0x2a))
        {
            return VBP_PARM;
        }
    }

    // ugly behavior
    cxt->list.num_items = 1;

    vbp_data_vp8 *query_data = (vbp_data_vp8*)pcontext->query_data;
    query_data->num_pictures = 0;

    return VBP_OK;
}

/**
*
* process parsing result after a NAL unit is parsed
*
*/
uint32 vbp_process_parsing_result_vp8( vbp_context *pcontext, int i)
{
    vp8_viddec_parser *parser = (vp8_viddec_parser *)pcontext->parser_cxt->codec_data;
    switch (parser->info.frame_tag.frame_type)
    {
    case KEY_FRAME:
        //ITRACE("This is a key frame.");
        parser->info.decoded_frame_number++;
        break;
    case INTER_FRAME:
        //ITRACE("This is an inter frame.");
        parser->info.decoded_frame_number++;
        break;
    case SKIPPED_FRAME:
        WTRACE("This is skipped frame. We have done nothing.");
        break;
    default:
        ETRACE("Unknown frame type %d", parser->info.frame_tag.frame_type);
        break;
    }

    //ITRACE("Decoded frame ID = %d", parser->info.decoded_frame_number);

    return VBP_OK;
}

static void vbp_add_quantization_data_vp8(vp8_viddec_parser *parser, vbp_data_vp8 *query_data)
{
    vp8_Info *pi = &(parser->info);
    VAIQMatrixBufferVP8 *IQ_buf = query_data->IQ_matrix_buf;

    int i = 0;
    if (pi->Segmentation.Enabled)
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {
            if (SEGMENT_ABSDATA == pi->Segmentation.AbsDelta)
            {
                IQ_buf->quantization_index[i][0] = pi->Segmentation.FeatureData[MB_LVL_ALT_Q][i];
            }
            else
            {
                int temp = pi->Quantization.Y1_AC + pi->Segmentation.FeatureData[MB_LVL_ALT_Q][i];
                IQ_buf->quantization_index[i][0] = (temp >= 0) ? ((temp <= MAX_QINDEX) ? temp : MAX_QINDEX) : 0;
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {
            IQ_buf->quantization_index[i][0] = pi->Quantization.Y1_AC;
        }
    }

    for (i = 0; i < MAX_MB_SEGMENTS; i++)
    {
        IQ_buf->quantization_index[i][1] = IQ_buf->quantization_index[i][0] + pi->Quantization.Y1_DC_Delta;
        IQ_buf->quantization_index[i][2] = IQ_buf->quantization_index[i][0] + pi->Quantization.Y2_DC_Delta;
        IQ_buf->quantization_index[i][3] = IQ_buf->quantization_index[i][0] + pi->Quantization.Y2_AC_Delta;
        IQ_buf->quantization_index[i][4] = IQ_buf->quantization_index[i][0] + pi->Quantization.UV_DC_Delta;
        IQ_buf->quantization_index[i][5] = IQ_buf->quantization_index[i][0] + pi->Quantization.UV_AC_Delta;
    }
}

static void vbp_add_probs_data_vp8(vp8_viddec_parser *parser, vbp_data_vp8 *query_data)
{
    FrameContextData *fc = &(parser->info.FrameContext);
    VAProbabilityDataBufferVP8 *prob_data = query_data->prob_data;

    /* DCT coefficients probability */
    memcpy(prob_data->dct_coeff_probs, fc->DCT_Coefficients, 4*8*3*11*sizeof(uint8_t));
}

static void vbp_set_codec_data_vp8(vp8_viddec_parser *parser, vbp_codec_data_vp8* codec_data)
{
    vp8_Info *pi = &(parser->info);

    codec_data->frame_type = pi->frame_tag.frame_type;
    codec_data->version_num = pi->frame_tag.version;
    codec_data->show_frame = pi->frame_tag.show_frame;

    codec_data->frame_width = ((pi->width + 15) / 16) * 16;
    codec_data->frame_height = ((pi->height + 15) / 16) * 16;

    codec_data->crop_top = 0;
    codec_data->crop_bottom = codec_data->frame_height - pi->height;
    codec_data->crop_left = 0;
    codec_data->crop_right = codec_data->frame_width - pi->width;

    codec_data->refresh_alt_frame = pi->refresh_af;
    codec_data->refresh_golden_frame = pi->refresh_gf;
    codec_data->refresh_last_frame = pi->refresh_lf;

    codec_data->golden_copied = pi->golden_copied;
    codec_data->altref_copied = pi->altref_copied;
}

static uint32_t vbp_add_pic_data_vp8(vp8_viddec_parser *parser, vbp_data_vp8 *query_data)
{
    vp8_Info *pi = &(parser->info);
    query_data->num_pictures++;

    if (query_data->num_pictures > 1)
    {
        ETRACE("Num of pictures (%d) per sample buffer exceeds the limit %d.", query_data->num_pictures, VP8_MAX_NUM_PICTURES);
        return VBP_DATA;
    }

    int i = 0;
    int pic_data_index = query_data->num_pictures - 1;
    if (pic_data_index < 0)
    {
        ETRACE("MB address does not start from 0!");
        return VBP_DATA;
    }

    vbp_picture_data_vp8 *pic_data = &(query_data->pic_data[pic_data_index]);
    VAPictureParameterBufferVP8 *pic_parms = pic_data->pic_parms;

    pic_parms->frame_width = pi->width;
    pic_parms->frame_height = pi->height;

    pic_parms->pic_fields.value = 0;
    pic_parms->pic_fields.bits.key_frame = pi->frame_tag.frame_type;
    pic_parms->pic_fields.bits.version = pi->frame_tag.version;

    /* Segmentation */
    pic_parms->pic_fields.bits.segmentation_enabled = pi->Segmentation.Enabled;
    pic_parms->pic_fields.bits.update_mb_segmentation_map = pi->Segmentation.UpdateMap;
    pic_parms->pic_fields.bits.update_segment_feature_data = pi->Segmentation.UpdateData;
    memcpy(pic_parms->mb_segment_tree_probs, pi->Segmentation.TreeProbs, sizeof(unsigned char) * MB_FEATURE_TREE_PROBS);

    /* Loop filter data */
    pic_parms->pic_fields.bits.filter_type = pi->LoopFilter.Type;
    pic_parms->pic_fields.bits.sharpness_level = pi->LoopFilter.Sharpness;
    pic_parms->pic_fields.bits.loop_filter_adj_enable = pi->LoopFilter.DeltaEnabled;
    pic_parms->pic_fields.bits.mode_ref_lf_delta_update = pi->LoopFilter.DeltaUpdate;

    int baseline_filter_level[MAX_MB_SEGMENTS];
    if (pi->Segmentation.Enabled)
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {
            if (SEGMENT_ABSDATA == pi->Segmentation.AbsDelta)
            {
                baseline_filter_level[i] = pi->Segmentation.FeatureData[MB_LVL_ALT_LF][i];
            }
            else
            {
                baseline_filter_level[i] = pi->LoopFilter.Level + pi->Segmentation.FeatureData[MB_LVL_ALT_LF][i];
                baseline_filter_level[i] = (baseline_filter_level[i] >= 0) ? ((baseline_filter_level[i] <= MAX_LOOP_FILTER) ? baseline_filter_level[i] : MAX_LOOP_FILTER) : 0;  /* Clamp to valid range */
            }
        }
    }
    else
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {
            baseline_filter_level[i] = pi->LoopFilter.Level;
        }
    }
    for (i = 0; i < MAX_MB_SEGMENTS; i++)
    {
        pic_parms->loop_filter_level[i] = baseline_filter_level[i];
    }
    if ((pic_parms->pic_fields.bits.version == 0) || (pic_parms->pic_fields.bits.version == 1))
    {
        pic_parms->pic_fields.bits.loop_filter_disable = pic_parms->loop_filter_level[0] > 0 ? true : false;
    }
    memcpy(pic_parms->loop_filter_deltas_ref_frame, pi->LoopFilter.DeltasRef, sizeof(char) * MAX_REF_LF_DELTAS);
    memcpy(pic_parms->loop_filter_deltas_mode, pi->LoopFilter.DeltasMode, sizeof(char) * MAX_MODE_LF_DELTAS);

    pic_parms->pic_fields.bits.sign_bias_golden = pi->sign_bias_golden;
    pic_parms->pic_fields.bits.sign_bias_alternate = pi->sign_bias_alternate;

    pic_parms->pic_fields.bits.mb_no_coeff_skip = pi->mb_no_coeff_skip;
    pic_parms->pic_fields.bits.mb_skip_coeff = pi->mb_skip_coeff;

    pic_parms->prob_skip_false = pi->prob_skip_false;
    pic_parms->prob_intra = pi->prob_intra;
    pic_parms->prob_last = pi->prob_lf;
    pic_parms->prob_gf = pi->prob_gf;

    FrameContextData *fc = &(parser->info.FrameContext);
    memcpy(pic_parms->y_mode_probs, fc->Y_Mode_Prob, sizeof(unsigned char) * 4);
    memcpy(pic_parms->uv_mode_probs, fc->UV_Mode_Prob, sizeof(unsigned char) * 3);
    /* Motion vector context */
    for (i = 0; i < 2; i++)
    {
        memcpy(pic_parms->mv_probs[i], fc->MVContext[i], sizeof(unsigned char) * 19);
    }

    /* Bool coder */
    pic_parms->bool_coder_ctx.range = pi->bool_coder.range;
    pic_parms->bool_coder_ctx.value = (pi->bool_coder.value >> 24) & 0xFF;
    pic_parms->bool_coder_ctx.count = pi->bool_coder.count;

    //pic_parms->current_picture = VA_INVALID_SURFACE;
    pic_parms->last_ref_frame = VA_INVALID_SURFACE;
    pic_parms->golden_ref_frame = VA_INVALID_SURFACE;
    pic_parms->alt_ref_frame = VA_INVALID_SURFACE;
    pic_parms->out_of_loop_frame = VA_INVALID_SURFACE; //Reserved for future use

    /* specify the slice number */
    pic_data->num_slices = 0;

    return VBP_OK;
}

static uint32_t vbp_add_slice_data_vp8(vp8_viddec_parser *parser, vbp_data_vp8 *query_data)
{
    vp8_Info *pi = &(parser->info);
    uint32_t pic_index = query_data->num_pictures - 1;
    uint32_t part_index = 0;
    if (pic_index < 0)
    {
        ETRACE("Invalid picture data index.");
        return VBP_DATA;
    }

    vbp_picture_data_vp8 *pic_data = &(query_data->pic_data[pic_index]);
    vbp_slice_data_vp8 *slc_data = &(pic_data->slc_data[pic_data->num_slices]);

    slc_data->buffer_addr = pi->source;
    slc_data->slice_offset = 0;
    slc_data->slice_size = pi->source_sz;

    VASliceParameterBufferVP8 *slc_parms = &(slc_data->slc_parms);
    /* number of bytes in the slice data buffer for this slice */
    slc_parms->slice_data_size = slc_data->slice_size;

    /* the offset to the first byte of slice data */
    slc_parms->slice_data_offset = 0;

    /* see VA_SLICE_DATA_FLAG_XXX definitions */
    slc_parms->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

    /* the offset to the first bit of MB from the first byte of slice data */
    slc_parms->macroblock_offset = pi->header_bits;

    /* Token Partitions */
    slc_parms->num_of_partitions = pi->partition_count;
    slc_parms->partition_size[0] = pi->frame_tag.first_part_size;
    for (part_index = 1; part_index < 9; part_index++)
    {
        slc_parms->partition_size[part_index] = pi->partition_size[part_index - 1];
    }

    pic_data->num_slices++;
    if (pic_data->num_slices > VP8_MAX_NUM_SLICES) {
        ETRACE("Number of slices (%d) per picture exceeds the limit (%d).", pic_data->num_slices, VP8_MAX_NUM_SLICES);
        return VBP_DATA;
    }
    return VBP_OK;
}

/*
*
* fill query data structure after sample buffer is parsed
*
*/
uint32 vbp_populate_query_data_vp8(vbp_context *pcontext)
{
    int32_t error = VBP_OK;

    vbp_data_vp8 *query_data = NULL;
    vp8_viddec_parser *parser = NULL;

    parser = (vp8_viddec_parser *)pcontext->parser_cxt->codec_data;
    query_data = (vbp_data_vp8 *)pcontext->query_data;

    /* buffer number */
    query_data->buf_number = buffer_counter;

    /* Populate picture data */
    error = vbp_add_pic_data_vp8(parser, query_data);

    /* Populate slice data */
    if (error == VBP_OK)
    {
        error = vbp_add_slice_data_vp8(parser, query_data);
        if (error != VBP_OK)
            return error;
    }

    /* Populate codec data */
    vbp_set_codec_data_vp8(parser, query_data->codec_data);

    /* Populate probability table */
    vbp_add_probs_data_vp8(parser, query_data);

    /* Populate quantization */
    vbp_add_quantization_data_vp8(parser, query_data);

    return VBP_OK;
}
