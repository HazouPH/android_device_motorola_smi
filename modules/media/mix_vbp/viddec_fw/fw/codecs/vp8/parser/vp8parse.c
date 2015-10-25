/* INTEL CONFIDENTIAL
* Copyright (c) 2012 Intel Corporation.  All rights reserved.
* Copyright (c) Imagination Technologies Limited, UK
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

#include "vp8_tables.h"
#include "vp8parse.h"

static const uint8_t kVp8SyncCodeByte[] = {0x9d, 0x01, 0x2a};

void vp8_init_Info(vp8_Info *pi)
{
    memset(pi, 0, sizeof(vp8_Info));

    /* Initialise the parser */
    pi->decoded_frame_number = 0;
    pi->refresh_entropy_lf = 1;
}

int32_t vp8_parse_frame_tag(FrameTagHeader *frame_tag, uint8_t *data, uint32_t data_sz)
{
    if (data_sz < 3)
    {
        return VP8_CORRUPT_FRAME;
    }

    /* 1-bit frame type */
    frame_tag->frame_type = (FRAME_TYPE)(data[0] & 1);

    /* 3-bit version number */
    frame_tag->version = (data[0] >> 1) & 7;
    if (frame_tag->version > 3)
    {
        return VP8_UNSUPPORTED_VERSION ;
    }

    /* 1-bit show frame flag */
    frame_tag->show_frame = (data[0] >> 4) & 1;

    /* 19-bit field containing the sie of the first data partition in bytes */
    frame_tag->first_part_size = (data[0] | (data[1] << 8) | (data[2] << 16)) >> 5;

    return VP8_NO_ERROR;
}

void vp8_init_frame(vp8_Info *pi)
{
    pi->golden_copied = BufferCopied_NoneToGolden;
    pi->altref_copied = BufferCopied_NoneToAltref;

    if (pi->frame_tag.frame_type == KEY_FRAME)
    {
        /* Various keyframe initializations */
        /* vp8_prob data initialization */
        memcpy(pi->FrameContext.B_Mode_Prob, VP8_BMode_Const, sizeof(VP8_BMode_Const));
        memcpy(pi->FrameContext.Y_Mode_Prob, VP8_YMode_Const, sizeof(VP8_YMode_Const));
        memcpy(pi->FrameContext.UV_Mode_Prob, VP8_UVMode_Const, sizeof(VP8_UVMode_Const));
        memcpy(pi->FrameContext.MVContext, VP8_MV_DefaultMVContext, sizeof(VP8_MV_DefaultMVContext));
        memcpy(pi->FrameContext.DCT_Coefficients, VP8_Coefficient_Default_Probabilites, sizeof(VP8_Coefficient_Default_Probabilites));

        /* reset the segment feature data to 0 with delta coding (Default state)*/
        memset(pi->Segmentation.FeatureData, 0, sizeof(pi->Segmentation.FeatureData));
        pi->Segmentation.AbsDelta = SEGMENT_DELTADATA;

        /* reset the mode ref deltasa for loop filter */
        memset(pi->LoopFilter.DeltasRef, 0, sizeof(pi->LoopFilter.DeltasRef));
        memset(pi->LoopFilter.DeltasMode, 0, sizeof(pi->LoopFilter.DeltasMode));

        /* All buffers are implicitly updated on key frames */
        pi->refresh_gf = 1;
        pi->refresh_af = 1;

        pi->sign_bias_golden = 0;
        pi->sign_bias_alternate = 0;
    }
    else if (pi->frame_tag.frame_type == INTER_FRAME)
    {
        pi->refresh_gf = 0;
        pi->refresh_af = 0;
    }
}

/* This function provides vp8_prob and value infomation for implementing
 * segment adaptive adjustments to default decoder behaviors.
 * The data parsed here applies to the entire frame. The adjustments can be
 * quantization level or loop filter strength.
 * */
void vp8_parse_segmentation_adjustments_data(vp8_Info *pi)
{
    int i,j;
    BOOL_CODER *bc = &(pi->bool_coder);

    const int *const mb_feature_data_bits = VP8_MB_FeatureDataBits;

    /* Is segmentation enabled */
    pi->Segmentation.Enabled = (uint8_t)vp8_decode_bool(bc, 128); //chapter 9.2 - macroblock uses segments ?  1: 0

    if(pi->Segmentation.Enabled )
    {
        /* Signal whether or not the segmentation map is being explicitly updated this frame */
        pi->Segmentation.UpdateMap = (uint8_t)vp8_decode_bool(bc, 128);
        pi->Segmentation.UpdateData = (uint8_t)vp8_decode_bool(bc, 128);

        if (pi->Segmentation.UpdateData)
        {
            pi->Segmentation.AbsDelta = (uint8_t)vp8_decode_bool(bc, 128);

            memset(pi->Segmentation.FeatureData, 0, sizeof(pi->Segmentation.FeatureData));

            /* For each segmentation feature (Quant and loop filter level) */
            for (i = 0; i < MB_LVL_MAX; ++i)
            {
                for (j = 0; j < MAX_MB_SEGMENTS; ++j)
                {
                    /* Frame level data */
                    if (vp8_decode_bool(bc, 128))
                    {
                        /* Parse magnitude */
                        pi->Segmentation.FeatureData[i][j] = (int8_t) vp8_read_bits(bc, mb_feature_data_bits[i]) ;

                        /* Parse sign data */
                        if (vp8_decode_bool(bc, 128))
                        {
                            pi->Segmentation.FeatureData[i][j] = -pi->Segmentation.FeatureData[i][j];
                        }
                    }
                    else
                    {
                        pi->Segmentation.FeatureData[i][j] = 0;
                    }
                }
            }

        }

        if (pi->Segmentation.UpdateMap)
        {
            /* Which macro block level features are enabled */
            memset(pi->Segmentation.TreeProbs, 255, sizeof(pi->Segmentation.TreeProbs));

            /* Read the probs used to decode the segment id for each macro block */
            for (i = 0; i < MB_FEATURE_TREE_PROBS; ++i)
            {
                /* If not explicitly set value is defaulted to 255 by memset above */
                if (vp8_decode_bool(bc, 128))
                {
                    pi->Segmentation.TreeProbs[i] = (uint8_t)vp8_read_bits(bc, 8);
                }
            }
        }
    }
}

/* VP8 supprots two types of loop filter. The data parsed in the header
 * to support the selection of the type, strength and sharpness behavior
 * of the loop filter used for the current frame.
 */
void vp8_parse_loop_filter_type_level(vp8_Info *pi)
{
    BOOL_CODER *bc = &(pi->bool_coder);

    /* Read the loop filter level and type */
    pi->LoopFilter.Type = (LoopFilterType)vp8_decode_bool(bc, 128);
    pi->LoopFilter.Level = (uint8_t)vp8_read_bits(bc, 6);
    pi->LoopFilter.Sharpness = (uint8_t)vp8_read_bits(bc, 3);
}

/* This function provides flag and value information for implmenting
 * per-macroblock loop filter level adjustments to default decoder
 * behaviors. Data parsed here applies to the entire frame.
 */
void vp8_parse_loop_filter_adjustments_data(vp8_Info *pi)
{
    int i;
    BOOL_CODER *bc = &(pi->bool_coder);

    /* Read in loop filter deltas applied at the MB level based on mode or ref frame */
    pi->LoopFilter.DeltaUpdate = 0;
    pi->LoopFilter.DeltaEnabled =  (uint8_t)vp8_decode_bool(bc, 128);

    if (pi->LoopFilter.DeltaEnabled)
    {
        /* Do the deltas need to be updated */
        pi->LoopFilter.DeltaUpdate = (uint8_t)vp8_decode_bool(bc, 128);

        if (pi->LoopFilter.DeltaUpdate)
        {
            /* Update based on reference */
            for (i = 0; i < MAX_REF_LF_DELTAS; ++i)
            {
                if (vp8_decode_bool(bc, 128))
                {
                    pi->LoopFilter.DeltasRef[i] = (int8_t)vp8_read_bits(bc, 6);

                    /* Parse sign */
                    if (vp8_decode_bool(bc, 128))
                    {
                        pi->LoopFilter.DeltasRef[i] = -1 * pi->LoopFilter.DeltasRef[i];
                    }
                }
            }

            /* Update based on macroblock mode */
            for (i = 0; i < MAX_MODE_LF_DELTAS; ++i)
            {
                if (vp8_decode_bool(bc, 128))
                {
                    pi->LoopFilter.DeltasMode[i] = (int8_t)vp8_read_bits(bc, 6);

                    /* Parse sign */
                    if (vp8_decode_bool(bc, 128))
                    {
                        pi->LoopFilter.DeltasMode[i] = -1 * pi->LoopFilter.DeltasMode[i];
                    }
                }
            } /* End for (i = 0; i < MAX_MODE_LF_DELTAS; ++i) */
        } /* End if (pi->LoopFilter.DeltaUpdate) */
    }
}

/* Token partition and partition data offsets */
void vp8_parse_token_partition_data(vp8_Info *pi, uint8_t *cx_size)
{
    BOOL_CODER *bc = &(pi->bool_coder);
    uint8_t *partition = NULL;
    uint8_t *source_end = pi->source + pi->source_sz;
    uint32_t partition_size = 0, i = 0;
    uint8_t *partition_size_ptr = NULL;

    /* Parse number of token partitions to use */
    pi->partition_count = 1 << (uint8_t)vp8_read_bits(bc, 2);

    /* Set up pointers to the first partition */
    partition = cx_size;
    if (pi->partition_count > 1)
    {
        /* Each partition offset is written in 3 bytes */
        partition += 3 * (pi->partition_count - 1);
    }

    for (i = 0; i < pi->partition_count; i++)
    {
        partition_size_ptr = cx_size + i * 3;

        if (i < pi->partition_count - 1)
        {
            pi->partition_size[i] = vp8_read_partition_size(partition_size_ptr);
        }
        else
        {
            /* Last offset can be calculated implictly */
            pi->partition_size[i] = source_end - partition;
        }

        partition += pi->partition_size[i];
    }
}

int32_t vp8_read_partition_size(uint8_t  *cx_size)
{
    uint32_t size = cx_size[0] + (cx_size[1] << 8) + (cx_size[2] << 16);

    return size;
}

int read_q_delta(BOOL_CODER   *bool_coder)
{
    int q_delta = 0;

    /* presence flag */
    if (vp8_decode_bool(bool_coder, 128))
    {
        /* magnitude */
        q_delta = (uint8_t)vp8_read_bits(bool_coder, 4) ;

        /* sign */
        if (vp8_decode_bool(bool_coder, 128))
        {
            q_delta = -q_delta;
        }
    }

    return q_delta;
}

/* Read the default quantizers */
void vp8_parse_dequantization_indices(vp8_Info *pi)
{
    BOOL_CODER *bc = &(pi->bool_coder);

    /* AC 1st order Q = default as a baseline for other 5 items */
    pi->Quantization.Y1_AC       = (int8_t)vp8_read_bits(bc, 7);
    pi->Quantization.Y1_DC_Delta = (int8_t)read_q_delta(bc);
    pi->Quantization.Y2_DC_Delta = (int8_t)read_q_delta(bc);
    pi->Quantization.Y2_AC_Delta = (int8_t)read_q_delta(bc);
    pi->Quantization.UV_DC_Delta = (int8_t)read_q_delta(bc);
    pi->Quantization.UV_AC_Delta = (int8_t)read_q_delta(bc);
}


/* Determine if the golden frame or ARF buffer should be updated and how.
 * For all non key frames the GF and ARF refresh flags and sign bias
 * flags must be set explicitly.
 */
void vp8_parse_gf_af_refresh_flags(vp8_Info *pi)
{
    BOOL_CODER *bc = &(pi->bool_coder);

    /* Read Golden and AltRef frame refresh */
    pi->refresh_gf = (uint8_t)vp8_decode_bool(bc, 128);
    pi->refresh_af = (uint8_t)vp8_decode_bool(bc, 128);

    /* If not refreshed using the current reconstructed frame */
    if (0 == pi->refresh_gf)
    {
        /* 2 bit indicating which buffer is copied to golden frame */
        pi->golden_copied = (GoldenBufferCopiedType)(int8_t)vp8_read_bits(bc, 2);
    }
    else
    {
        /* No buffer is copied */
        pi->golden_copied = (GoldenBufferCopiedType)0;
    }

    if (0 == pi->refresh_af)
    {
        /* 2 bit indicating which buffer is copied to alternative frame */
        pi->altref_copied = (AltRefBufferCopiedType)vp8_read_bits(bc, 2);
    }
    else
    {
        pi->altref_copied = (AltRefBufferCopiedType)0;
    }

    pi->sign_bias_golden = (uint8_t)vp8_decode_bool(bc, 128);
    pi->sign_bias_alternate = (uint8_t)vp8_decode_bool(bc, 128);

}

void vp8_parse_coef_probs_tree(vp8_Info *pi)
{
    int i, j, k, l;

    BOOL_CODER *bc = &(pi->bool_coder);

    /* DCT coeffienct probability tree update */
    for (i = 0; i < BLOCK_TYPES; i++)
    {
        for (j = 0; j < COEF_BANDS; j++)
        {
            for (k = 0; k < PREV_COEF_CONTEXTS; k++)
            {
                for (l = 0; l < MAX_COEF_TOKENS - 1; l++)
                {
                    if (vp8_decode_bool(bc, VP8_Coefficient_Update_Probabilites[i][j][k][l]))
                    {
                        pi->FrameContext.DCT_Coefficients[i][j][k][l] = (vp8_prob)vp8_read_bits(bc, 8);
                    }
                }
            }
        }
    }
}

/* Parse remaining non-key-frame only data from frame header */
void vp8_parse_mb_mv_info(vp8_Info *pi)
{
    // read_mvcontexts
    int i = 0;

    BOOL_CODER *bc = &(pi->bool_coder);

    do
    {
        const vp8_prob *up = VP8_MV_UpdateProbs[i];
        vp8_prob *p = pi->FrameContext.MVContext[i];
        vp8_prob *const pstop = p + VP8_MV_Pcount;

        do
        {
            if (vp8_decode_bool(bc , *up++ ))
            {
                const vp8_prob x = (vp8_prob)vp8_read_bits(bc, 7);

                *p = x ? x << 1 : 1;
            }
        }
        while (++p < pstop);
    }
    while (++i < 2);
}

/* Parse remaining non-key-frame only data from frame header */
void vp8_parse_yuv_probs_update(vp8_Info *pi)
{
    BOOL_CODER *bc = &(pi->bool_coder);

    /* Read probabilities */
    pi->prob_intra = (vp8_prob)vp8_read_bits(bc, 8);
    pi->prob_lf = (vp8_prob)vp8_read_bits(bc, 8);
    pi->prob_gf = (vp8_prob)vp8_read_bits(bc, 8);

    pi->y_prob_valid = (uint8_t)vp8_decode_bool(bc , 128);
    if (1 == pi->y_prob_valid)
    {
        pi->FrameContext.Y_Mode_Prob[0] = (vp8_prob)vp8_read_bits(bc, 8);
        pi->FrameContext.Y_Mode_Prob[1] = (vp8_prob)vp8_read_bits(bc, 8);
        pi->FrameContext.Y_Mode_Prob[2] = (vp8_prob)vp8_read_bits(bc, 8);
        pi->FrameContext.Y_Mode_Prob[3] = (vp8_prob)vp8_read_bits(bc, 8);
    }

    pi->c_prob_valid = (uint8_t)vp8_decode_bool(bc , 128);
    if (1 == pi->c_prob_valid)
    {
        pi->FrameContext.UV_Mode_Prob[0] = (vp8_prob)vp8_read_bits(bc, 8);
        pi->FrameContext.UV_Mode_Prob[1] = (vp8_prob)vp8_read_bits(bc, 8);
        pi->FrameContext.UV_Mode_Prob[2] = (vp8_prob)vp8_read_bits(bc, 8);
    }
}


void vp8_parse_remaining_frame_header_data(vp8_Info *pi)
{
    BOOL_CODER *bc = &(pi->bool_coder);

    /* MB no coefficients skip */
    pi->mb_no_coeff_skip = (uint8_t)vp8_decode_bool(bc, 128);

    if (1 == pi->mb_no_coeff_skip)
    {
        pi->prob_skip_false = (vp8_prob)vp8_read_bits(bc, 8);
    }
    else
    {
        pi->mb_skip_coeff = 0;
    }

    if (pi->frame_tag.frame_type == INTER_FRAME)
    {
        vp8_parse_yuv_probs_update(pi);

        /* Read motion vector info */
        vp8_parse_mb_mv_info(pi);
    }

}

#if 0
vp8_Status vp8_translate_parse_status(vp8_Status status)
{
    switch (status)
    {
    case VP8_UNSUPPORTED_VERSION:
        LOGE("Parser returned VP8_UNSUPPORTED_VERSION");
       return VP8_UNSUPPORTED_VERSION;
    case VP8_UNSUPPORTED_BITSTREAM:
        LOGE("Parser returned VP8_UNSUPPORTED_BITSTREAM");
        return VP8_UNSUPPORTED_BITSTREAM;
    case VP8_INVALID_FRAME_SYNC_CODE:
        LOGE("Parser returned VP8_INVALID_FRAME_SYNC_CODE");
        return VP8_INVALID_FRAME_SYNC_CODE;
    case VP8_UNEXPECTED_END_OF_BITSTREAM:
        LOGE("Parser returned VP8_UNEXPECTED_END_OF_BITSTREAM");
        return VP8_UNEXPECTED_END_OF_BITSTREAM;
    default:
        LOGE("Parser returned VP8_UNKNOWN_ERROR");
        return VP8_UNKNOWN_ERROR;
    }
}
#endif

/* Parse VP8 frame header */
int32_t vp8_parse_frame_header(vp8_viddec_parser *parser)
{
    vp8_Status ret = VP8_NO_ERROR;

    vp8_Info *pi = &(parser->info);

    uint8_t *data = pi->source;
    uint32_t data_sz = pi->source_sz;

    if (0 == pi->refresh_entropy_lf)
    {
        memcpy(&(pi->FrameContext), &(pi->LastFrameContext), sizeof(FrameContextData));
    }

    /* Step 1 : parse frame tag containing 3 bytes*/
    ret = vp8_parse_frame_tag(&(pi->frame_tag), data, data_sz);
    if (ret != VP8_NO_ERROR)
    {
        return ret;
    }

    /* Pointer advances 3 bytes */
    data += 3;

    /* Start the frame data offset */
    pi->frame_data_offset = 3;

    /* Step 2 : parse key frame parameters*/
    if (pi->frame_tag.frame_type == KEY_FRAME)
    {
        /* Check sync code containg 3 bytes*/
        if ((data[0] != kVp8SyncCodeByte[0]) || (data[1] != kVp8SyncCodeByte[1]) || (data[2] != kVp8SyncCodeByte[2]))
        {
            return VP8_INVALID_FRAME_SYNC_CODE;
        }

        pi->width = (data[3] | (data[4] << 8)) & 0x3fff;
        pi->horiz_scale = data[4] >> 6;
        pi->height = (data[5] | (data[6] << 8)) & 0x3fff;
        pi->vert_scale = data[6] >> 6;

        /* Pointer advances 7 bytes in this case*/
        data += 7;
        pi->frame_data_offset += 7;
    }

    if (0 == pi->width || 0 == pi->height)
    {
        return VP8_UNSUPPORTED_BITSTREAM;
    }

    /* Initialize frame parameters*/
    vp8_init_frame(pi);

    /* Initialize bool coder */
    BOOL_CODER *bc = &(pi->bool_coder);
    vp8_start_decode(bc, (uint8_t*)data);

    /* Parse key frame parameters */
    if (pi->frame_tag.frame_type == KEY_FRAME)
    {
        pi->clr_type   = (YUV_TYPE)vp8_decode_bool(bc, 128);
        pi->clamp_type = (CLAMP_TYPE)vp8_decode_bool(bc, 128);
    }

    /* Step 3 : parse macroblock-level segmentation flag */
    vp8_parse_segmentation_adjustments_data(pi);

    /* Step 4 : parse loop filter type and levels */
    vp8_parse_loop_filter_type_level(pi);

    /* Step 5 : parse macroblock-level loop filter adjustments */
    vp8_parse_loop_filter_adjustments_data(pi);

    /* Step 6: parse token partition and partition data offsets */
    vp8_parse_token_partition_data(pi, data + pi->frame_tag.first_part_size);

    /* Step 7: parse dequantization indices */
    vp8_parse_dequantization_indices(pi);

    /* For key frames, both golden frame and altref frame are refreshed/replaced by the current reconstructed frame, by default */
    if (pi->frame_tag.frame_type == INTER_FRAME)
    {
        /* Step 8: parse golden frame and altref frame refresh flags */
        vp8_parse_gf_af_refresh_flags(pi);
    }

    /* Step 9: update proability to decode DCT coef */
    pi->refresh_entropy = (uint8_t)vp8_decode_bool(bc, 128);
    if (pi->refresh_entropy  == 0)
    {
        memcpy(&(pi->LastFrameContext), &(pi->FrameContext), sizeof(FrameContextData));
    }

    /* Step 10: refresh last frame buffer */
    pi->refresh_lf = (pi->frame_tag.frame_type == KEY_FRAME) || (uint8_t)(vp8_decode_bool(bc, 128));

    /* Step 11: read coef vp8_prob tree */
    vp8_parse_coef_probs_tree(pi);

    /* Step 12: read remaining frame header data */
    vp8_parse_remaining_frame_header_data(pi);

    /* Hold the current offset in the bitstream */
    pi->frame_data_offset += pi->bool_coder.pos;

    /* Get the frame header bits */
    pi->header_bits = pi->frame_data_offset * 8 - 16 - pi->bool_coder.count;

    pi->refresh_entropy_lf = pi->refresh_entropy;

    return ret;
}
