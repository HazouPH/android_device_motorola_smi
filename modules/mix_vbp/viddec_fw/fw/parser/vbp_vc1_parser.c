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
#include <string.h>

#include "vc1.h"
#include "vbp_loader.h"
#include "vbp_utils.h"
#include "vbp_vc1_parser.h"

/* maximum number of Macroblock divided by 2, see va.h */
#define MAX_BITPLANE_SIZE 16384

/* Start code prefix is 001 which is 3 bytes. */
#define PREFIX_SIZE 3

static uint32 b_fraction_table[][9] = {
    /* num       0  1  2  3  4  5   6   7   8   den */
    /* 0 */    { 0, 0, 0, 0, 0, 0,  0,  0,  0 },
    /* 1 */    { 0, 0, 0, 1, 3, 5,  9, 11, 17 },
    /* 2 */    { 0, 0, 0, 2, 0, 6,  0, 12,  0 },
    /* 3 */    { 0, 0, 0, 0, 4, 7,  0, 13, 18 },
    /* 4 */    { 0, 0, 0, 0, 0, 8,  0, 14,  0 },
    /* 5 */    { 0, 0, 0, 0, 0, 0, 10, 15, 19 },
    /* 6 */    { 0, 0, 0, 0, 0, 0,  0, 16,  0 },
    /* 7 */    { 0, 0, 0, 0, 0, 0,  0,  0, 20 }
};


static uint8 vc1_aspect_ratio_table[][2] =
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

    // reserved
    {0, 0}
};



/**
 * set parser entry points
 */
uint32 vbp_init_parser_entries_vc1(vbp_context *pcontext)
{
    if (NULL == pcontext->parser_ops)
    {
        /* impossible, just sanity check */
        return VBP_PARM;
    }

    pcontext->parser_ops->init = dlsym(pcontext->fd_parser, "viddec_vc1_init");
    if (NULL == pcontext->parser_ops->init)
    {
        ETRACE ("Failed to set entry point.");
        return VBP_LOAD;
    }

    pcontext->parser_ops->parse_sc = viddec_parse_sc;

    pcontext->parser_ops->parse_syntax = dlsym(pcontext->fd_parser, "viddec_vc1_parse");
    if (NULL == pcontext->parser_ops->parse_syntax)
    {
        ETRACE ("Failed to set entry point.");
        return VBP_LOAD;
    }

    pcontext->parser_ops->get_cxt_size = dlsym(pcontext->fd_parser, "viddec_vc1_get_context_size");
    if (NULL == pcontext->parser_ops->get_cxt_size)
    {
        ETRACE ("Failed to set entry point.");
        return VBP_LOAD;
    }

    pcontext->parser_ops->is_wkld_done = dlsym(pcontext->fd_parser, "viddec_vc1_wkld_done");
    if (NULL == pcontext->parser_ops->is_wkld_done)
    {
        ETRACE ("Failed to set entry point.");
        return VBP_LOAD;
    }

    pcontext->parser_ops->is_frame_start = dlsym(pcontext->fd_parser, "viddec_vc1_is_start_frame");
    if (NULL == pcontext->parser_ops->is_frame_start)
    {
        ETRACE ("Failed to set entry point.");
        return VBP_LOAD;
    }

    /* entry point not needed */
    pcontext->parser_ops->flush = NULL;

    return VBP_OK;
}

/**
 * allocate query data structure
 */
uint32 vbp_allocate_query_data_vc1(vbp_context *pcontext)
{
    if (NULL != pcontext->query_data)
    {
        /* impossible, just sanity check */
        return VBP_PARM;
    }

    pcontext->query_data = NULL;

    vbp_data_vc1 *query_data = NULL;
    query_data = vbp_malloc_set0(vbp_data_vc1, 1);
    if (NULL == query_data)
    {
        return VBP_MEM;
    }

    /* assign the pointer */
    pcontext->query_data = (void *)query_data;

    query_data->se_data = vbp_malloc_set0(vbp_codec_data_vc1, 1);
    if (NULL == query_data->se_data)
    {
        goto cleanup;
    }
    query_data->pic_data = vbp_malloc_set0(vbp_picture_data_vc1, MAX_NUM_PICTURES);
    if (NULL == query_data->pic_data)
    {
        goto cleanup;
    }

    int i;
    for (i = 0; i < MAX_NUM_PICTURES; i++)
    {
        query_data->pic_data[i].pic_parms = vbp_malloc_set0(VAPictureParameterBufferVC1, 1);
        if (NULL == query_data->pic_data[i].pic_parms)
        {
            goto cleanup;
        }

        query_data->pic_data[i].packed_bitplanes = vbp_try_malloc0(MAX_BITPLANE_SIZE);
        if (NULL == query_data->pic_data[i].packed_bitplanes)
        {
            goto cleanup;
        }

        query_data->pic_data[i].slc_data = vbp_try_malloc0(MAX_NUM_SLICES * sizeof(vbp_slice_data_vc1));
        if (NULL == query_data->pic_data[i].slc_data)
        {
            goto cleanup;
        }
    }

    return VBP_OK;

cleanup:
    vbp_free_query_data_vc1(pcontext);

    return VBP_MEM;
}


/**
 * free query data structure
 */
uint32 vbp_free_query_data_vc1(vbp_context *pcontext)
{
    vbp_data_vc1 *query_data = NULL;

    if (NULL == pcontext->query_data)
    {
        return VBP_OK;
    }

    query_data = (vbp_data_vc1 *)pcontext->query_data;

    if (query_data->pic_data)
    {
        int i = 0;
        for (i = 0; i < MAX_NUM_PICTURES; i++)
        {
            free(query_data->pic_data[i].slc_data);
            free(query_data->pic_data[i].packed_bitplanes);
            free(query_data->pic_data[i].pic_parms);
        }
    }

    free(query_data->pic_data);

    free(query_data->se_data);

    free(query_data);

    pcontext->query_data = NULL;

    return VBP_OK;
}


/**
 * We want to create a list of buffer segments where each segment is a start
 * code followed by all the data up to the next start code or to the end of
 * the buffer.  In VC-1, it is common to get buffers with no start codes.  The
 * parser proper, doesn't really handle the situation where there are no SCs.
 * In this case, I will bypass the stripping of the SC code and assume a frame.
 */
static uint32 vbp_parse_start_code_helper_vc1(
    viddec_pm_cxt_t *cxt,
    viddec_parser_ops_t *ops,
    int init_data_flag)
{
    uint32_t ret = VBP_OK;
    viddec_sc_parse_cubby_cxt_t cubby;

    /* make copy of cubby */
    /* this doesn't copy the buffer, merely the structure that holds the buffer */
    /* pointer.  Below, where we call parse_sc() the code starts the search for */
    /* SCs at the beginning of the buffer pointed to by the cubby, so in our */
    /* cubby copy we increment the pointer as we move through the buffer.  If */
    /* you think of each start code followed either by another start code or the */
    /* end of the buffer, then parse_sc() is returning information relative to */
    /* current segment. */

    cubby = cxt->parse_cubby;

    cxt->list.num_items = 0;
    cxt->list.data[0].stpos = 0;
    cxt->getbits.is_emul_reqd = 1;

    /* codec initialization data is always start code prefixed. (may not start at position 0)
     * sample buffer for AP has three start code patterns here:
     * pattern 0: no start code at all, the whole buffer is a single segment item
     * pattern 1: start codes for all segment items
     * pattern 2: no start code for the first segment item, start codes for the rest segment items
     */

    bool is_pattern_two = FALSE;

    unsigned char start_code = 0;

    while (1)
    {
        /* parse the created buffer for sc */
        ret = ops->parse_sc((void *)&cubby, (void *)&(cxt->codec_data[0]), &(cxt->sc_prefix_info));
        if (ret == 1)
        {
            cubby.phase = 0;
            start_code = *(unsigned char*)(cubby.buf + cubby.sc_end_pos);
#if 1
            if (0 == init_data_flag &&
                    PREFIX_SIZE != cubby.sc_end_pos &&
                    0 == cxt->list.num_items)
            {
                /* buffer does not have start code at the beginning */
                vc1_viddec_parser_t *parser = NULL;
                vc1_metadata_t *seqLayerHeader = NULL;

                parser = (vc1_viddec_parser_t *)cxt->codec_data;
                seqLayerHeader = &(parser->info.metadata);
                if (1 == seqLayerHeader->INTERLACE)
                {
                    /* this is a hack for interlaced field coding */
                    /* handle field interlace coding. One sample contains two fields, where:
                     * the first field does not have start code prefix,
                     * the second field has start code prefix.
                     */
                    cxt->list.num_items = 1;
                    cxt->list.data[0].stpos = 0;
                    is_pattern_two = TRUE;
                }
            }
#endif
            if (cxt->list.num_items == 0)  /* found first SC. */
            {
                /* sc_end_pos gets us to the SC type.  We need to back up to the first zero */
                cxt->list.data[0].stpos = cubby.sc_end_pos - PREFIX_SIZE;
            }
            else
            {
                /* First we set the end position of the last segment. */
                /* Since the SC parser searches from SC type to SC type and the */
                /* sc_end_pos is relative to this segment only, we merely add */
                /* sc_end_pos to the start to find the end. */
                cxt->list.data[cxt->list.num_items - 1].edpos =
                    cubby.sc_end_pos + cxt->list.data[cxt->list.num_items - 1].stpos;

                /* Then we set the start position of the current segment. */
                /* So I need to subtract 1 ??? */
                cxt->list.data[cxt->list.num_items].stpos =
                    cxt->list.data[cxt->list.num_items - 1].edpos;

                if (is_pattern_two)
                {
                    cxt->list.data[cxt->list.num_items].stpos -= PREFIX_SIZE;
                    /* restore to normal pattern */
                    is_pattern_two = FALSE;
                }
            }
            /* We need to set up the cubby buffer for the next time through parse_sc(). */
            /* But even though we want the list to contain a segment as described */
            /* above, we want the cubby buffer to start just past the prefix, or it will */
            /* find the same SC again.  So I bump the cubby buffer past the prefix. */
            cubby.buf = /*cubby.buf +*/
                cxt->parse_cubby.buf +
                cxt->list.data[cxt->list.num_items].stpos +
                PREFIX_SIZE;

            cubby.size = cxt->parse_cubby.size -
                         cxt->list.data[cxt->list.num_items].stpos -
                         PREFIX_SIZE;

            if ((start_code >= 0x0A && start_code <= 0x0F) || (start_code >= 0x1B && start_code <= 0x1F))
            {
                /* only put known start code to the list
                 * 0x0A: end of sequence
                 * 0x0B: slice header
                 * 0x0C: frame header
                 * 0x0D: field header
                 * 0x0E: entry point header
                 * 0x0F: sequence header
                 * 0x1B ~ 0x1F: user data
                 */
                cxt->list.num_items++;
            }
            else
            {
                ITRACE("skipping unknown start code :%d", start_code);
            }

            if (cxt->list.num_items >= MAX_IBUFS_PER_SC)
            {
                WTRACE("Num items exceeds the limit!");
                /* not fatal, just stop parsing */
                break;
            }
        }
        else
        {
            /* we get here, if we reach the end of the buffer while looking or a SC. */
            /* If we never found a SC, then num_items will never get incremented. */
            if (cxt->list.num_items == 0)
            {
                /* If we don't find a SC we probably still have a frame of data. */
                /* So let's bump the num_items or else later we will not parse the */
                /* frame.   */
                cxt->list.num_items = 1;
            }
            /* now we can set the end position of the last segment. */
            cxt->list.data[cxt->list.num_items - 1].edpos = cxt->parse_cubby.size;
            break;
        }
    }
    return VBP_OK;
}

/*
* parse initialization data (decoder configuration data)
* for VC1 advanced profile, data is sequence header and
* entry pointer header.
* for VC1 main/simple profile, data format
* is defined in VC1 spec: Annex J, (Decoder initialization metadata
* structure 1 and structure 3
*/
uint32 vbp_parse_init_data_vc1(vbp_context *pcontext)
{
    /**
    * init data (aka decoder configuration data) must
    * be start-code prefixed
    */

    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    viddec_parser_ops_t *ops = pcontext->parser_ops;
    return vbp_parse_start_code_helper_vc1(cxt, ops, 1);
}



/**
* Parse start codes, VC1 main/simple profile does not have start code;
* VC1 advanced may not have start code either.
*/
uint32_t vbp_parse_start_code_vc1(vbp_context *pcontext)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    viddec_parser_ops_t *ops = pcontext->parser_ops;

    vc1_viddec_parser_t *parser = NULL;
    vc1_metadata_t *seqLayerHeader = NULL;

    vbp_data_vc1 *query_data = (vbp_data_vc1 *) pcontext->query_data;

    /* Reset query data for the new sample buffer */
    int i = 0;
    for (i = 0; i < MAX_NUM_PICTURES; i++)
    {
        query_data->num_pictures = 0;
        query_data->pic_data[i].num_slices = 0;
        query_data->pic_data[i].picture_is_skipped = 0;
    }

    parser = (vc1_viddec_parser_t *)cxt->codec_data;
    seqLayerHeader = &(parser->info.metadata);


    /* WMV codec data will have a start code, but the WMV picture data won't. */
    if (VC1_PROFILE_ADVANCED == seqLayerHeader->PROFILE)
    {
        return vbp_parse_start_code_helper_vc1(cxt, ops, 0);
    }
    else
    {
        /* WMV: vc1 simple or main profile. No start code present. */

        /* must set is_emul_reqd to 0! */
        cxt->getbits.is_emul_reqd = 0;
        cxt->list.num_items = 1;
        cxt->list.data[0].stpos = 0;
        cxt->list.data[0].edpos = cxt->parse_cubby.size;
    }

    return VBP_OK;
}


/**
 *
 */
static inline uint8 vbp_get_bit_vc1(uint32 *data, uint32 *current_word, uint32 *current_bit)
{
    uint8 value;

    value = (data[*current_word] >> *current_bit) & 1;

    /* Fix up bit/byte offsets.  endianess?? */
    if (*current_bit < 31)
    {
        ++(*current_bit);
    }
    else
    {
        ++(*current_word);
        *current_bit = 0;
    }

    return value;
}


/**
 *
 */
static uint32 vbp_pack_bitplane_vc1(
    uint32 *from_plane,
    uint8 *to_plane,
    uint32 width,
    uint32 height,
    uint32 nibble_shift)
{
    uint32 error = VBP_OK;
    uint32 current_word = 0;
    uint32 current_bit = 0;  /* must agree with number in vbp_get_bit_vc1 */
    uint32 i, j, n;
    uint8 value;
    uint32 stride = 0;

    stride = 32 * ((width + 31) / 32);

    for (i = 0, n = 0; i < height; i++)
    {
        for (j = 0; j < stride; j++)
        {
            if (j < width)
            {
                value = vbp_get_bit_vc1(
                            from_plane,
                            &current_word,
                            &current_bit);

                to_plane[n / 2] |= value << (nibble_shift + ((n % 2) ? 0 : 4));
                n++;
            }
            else
            {
                break;
            }
        }
        if (stride > width)
        {
            current_word++;
            current_bit = 0;
        }
    }

    return error;
}


/**
 *
 */
static inline uint32 vbp_map_bfraction(uint32 numerator, uint32 denominator)
{
    uint32 b_fraction = 0;

    if ((numerator < 8) && (denominator < 9))
    {
        b_fraction = b_fraction_table[numerator][denominator];
    }

    return b_fraction;
}

/**
 *
 */
static uint32 vbp_pack_bitplanes_vc1(
    vbp_context *pcontext,
    int index,
    vbp_picture_data_vc1* pic_data)
{
    uint32 error = VBP_OK;
    if (0 == pic_data->pic_parms->bitplane_present.value)
    {
        /* return if bitplane is not present */
        pic_data->size_bitplanes = 0;
        memset(pic_data->packed_bitplanes, 0, MAX_BITPLANE_SIZE);
        return error;
    }

    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)pcontext->parser_cxt->codec_data;
    vc1_metadata_t *seqLayerHeader = &(parser->info.metadata);
    vc1_PictureLayerHeader *picLayerHeader = &(parser->info.picLayerHeader);


    /* set bit plane size */
    pic_data->size_bitplanes = ((seqLayerHeader->widthMB * seqLayerHeader->heightMB) + 1) / 2;


    memset(pic_data->packed_bitplanes, 0, pic_data->size_bitplanes);

    /* see libva library va.h for nibble bit */
    switch (picLayerHeader->PTYPE)
    {
    case VC1_I_FRAME:
    case VC1_BI_FRAME:
        if (picLayerHeader->OVERFLAGS.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->OVERFLAGS.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                2);
        }
        if (picLayerHeader->ACPRED.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->ACPRED.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                1);
        }
        if (picLayerHeader->FIELDTX.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->FIELDTX.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                0);
        }
        /* sanity check */
        if (picLayerHeader->MVTYPEMB.imode ||
                picLayerHeader->DIRECTMB.imode ||
                picLayerHeader->SKIPMB.imode ||
                picLayerHeader->FORWARDMB.imode)
        {
            ETRACE("Unexpected bit-plane type.");
            error = VBP_TYPE;
        }
        break;

    case VC1_P_FRAME:
        if (picLayerHeader->MVTYPEMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->MVTYPEMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                2);
        }
        if (picLayerHeader->SKIPMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->SKIPMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                1);
        }
        if (picLayerHeader->DIRECTMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->DIRECTMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                0);
        }
        /* sanity check */
        if (picLayerHeader->FIELDTX.imode ||
                picLayerHeader->FORWARDMB.imode ||
                picLayerHeader->ACPRED.imode ||
                picLayerHeader->OVERFLAGS.imode )
        {
            ETRACE("Unexpected bit-plane type.");
            error = VBP_TYPE;
        }
        break;

    case VC1_B_FRAME:
        if (picLayerHeader->FORWARDMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->FORWARDMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                2);
        }
        if (picLayerHeader->SKIPMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->SKIPMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                1);
        }
        if (picLayerHeader->DIRECTMB.imode)
        {
            vbp_pack_bitplane_vc1(
                picLayerHeader->DIRECTMB.databits,
                pic_data->packed_bitplanes,
                seqLayerHeader->widthMB,
                seqLayerHeader->heightMB,
                0);
        }
        /* sanity check */
        if (picLayerHeader->MVTYPEMB.imode ||
                picLayerHeader->FIELDTX.imode ||
                picLayerHeader->ACPRED.imode ||
                picLayerHeader->OVERFLAGS.imode)
        {
            ETRACE("Unexpected bit-plane type.");
            error = VBP_TYPE;
        }
        break;
    }
    return error;
}


/**
 * fill the query data structure after sequence header, entry point header
 * or a complete frame is parsed.
 * NOTE: currently partial frame is not handled properly
 */
uint32 vbp_populate_query_data_vc1(vbp_context *pcontext)
{
    uint32 error = VBP_OK;

    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)pcontext->parser_cxt->codec_data;
    vc1_metadata_t *seqLayerHeader = &(parser->info.metadata);

    vbp_data_vc1 *query_data = (vbp_data_vc1 *)pcontext->query_data;

    /* first we get the SH/EP data.  Can we cut down on this? */
    vbp_codec_data_vc1 *se_data = query_data->se_data;


    uint32_t curHrdNum = seqLayerHeader->HRD_NUM_LEAKY_BUCKETS;

    se_data->bit_rate = curHrdNum ?
                        seqLayerHeader->hrd_initial_state.sLeakyBucket[curHrdNum -1].HRD_RATE :
                        seqLayerHeader->hrd_initial_state.sLeakyBucket[0].HRD_RATE;

    se_data->PROFILE = seqLayerHeader->PROFILE;
    se_data->LEVEL = seqLayerHeader->LEVEL;
    se_data->POSTPROCFLAG = seqLayerHeader->POSTPROCFLAG;
    se_data->PULLDOWN = seqLayerHeader->PULLDOWN;
    se_data->INTERLACE = seqLayerHeader->INTERLACE;
    se_data->TFCNTRFLAG = seqLayerHeader->TFCNTRFLAG;
    se_data->FINTERPFLAG = seqLayerHeader->FINTERPFLAG;
    se_data->PSF = seqLayerHeader->PSF;

    // color matrix
    if (seqLayerHeader->COLOR_FORMAT_FLAG)
    {
        se_data->MATRIX_COEF = seqLayerHeader->MATRIX_COEF;
    }
    else
    {
        //ITU-R BT. 601-5.
        se_data->MATRIX_COEF = 6;
    }

    // aspect ratio
    if (seqLayerHeader->ASPECT_RATIO_FLAG == 1)
    {
        se_data->ASPECT_RATIO = seqLayerHeader->ASPECT_RATIO;
        if (se_data->ASPECT_RATIO < 14)
        {
            se_data->ASPECT_HORIZ_SIZE = vc1_aspect_ratio_table[se_data->ASPECT_RATIO][0];
            se_data->ASPECT_VERT_SIZE = vc1_aspect_ratio_table[se_data->ASPECT_RATIO][1];
        }
        else if (se_data->ASPECT_RATIO == 15)
        {
            se_data->ASPECT_HORIZ_SIZE = seqLayerHeader->ASPECT_HORIZ_SIZE;
            se_data->ASPECT_VERT_SIZE = seqLayerHeader->ASPECT_VERT_SIZE;
        }
        else  // se_data->ASPECT_RATIO == 14
        {
            se_data->ASPECT_HORIZ_SIZE = 0;
            se_data->ASPECT_VERT_SIZE = 0;
        }
    }
    else
    {
        // unspecified
        se_data->ASPECT_RATIO = 0;
        se_data->ASPECT_HORIZ_SIZE = 0;
        se_data->ASPECT_VERT_SIZE = 0;
    }

    se_data->BROKEN_LINK = seqLayerHeader->BROKEN_LINK;
    se_data->CLOSED_ENTRY = seqLayerHeader->CLOSED_ENTRY;
    se_data->PANSCAN_FLAG = seqLayerHeader->PANSCAN_FLAG;
    se_data->REFDIST_FLAG = seqLayerHeader->REFDIST_FLAG;
    se_data->LOOPFILTER = seqLayerHeader->LOOPFILTER;
    se_data->FASTUVMC = seqLayerHeader->FASTUVMC;
    se_data->EXTENDED_MV = seqLayerHeader->EXTENDED_MV;
    se_data->DQUANT = seqLayerHeader->DQUANT;
    se_data->VSTRANSFORM = seqLayerHeader->VSTRANSFORM;
    se_data->OVERLAP = seqLayerHeader->OVERLAP;
    se_data->QUANTIZER = seqLayerHeader->QUANTIZER;
    se_data->CODED_WIDTH = (seqLayerHeader->width + 1) << 1;
    se_data->CODED_HEIGHT = (seqLayerHeader->height + 1) << 1;
    se_data->EXTENDED_DMV = seqLayerHeader->EXTENDED_DMV;
    se_data->RANGE_MAPY_FLAG = seqLayerHeader->RANGE_MAPY_FLAG;
    se_data->RANGE_MAPY = seqLayerHeader->RANGE_MAPY;
    se_data->RANGE_MAPUV_FLAG = seqLayerHeader->RANGE_MAPUV_FLAG;
    se_data->RANGE_MAPUV = seqLayerHeader->RANGE_MAPUV;
    se_data->RANGERED = seqLayerHeader->RANGERED;
    se_data->MAXBFRAMES = seqLayerHeader->MAXBFRAMES;
    se_data->MULTIRES = seqLayerHeader->MULTIRES;
    se_data->SYNCMARKER = seqLayerHeader->SYNCMARKER;
    se_data->RNDCTRL = seqLayerHeader->RNDCTRL;
    se_data->REFDIST = seqLayerHeader->REFDIST;
    se_data->widthMB = seqLayerHeader->widthMB;
    se_data->heightMB = seqLayerHeader->heightMB;
    se_data->INTCOMPFIELD = seqLayerHeader->INTCOMPFIELD;
    se_data->LUMSCALE2 = seqLayerHeader->LUMSCALE2;
    se_data->LUMSHIFT2 = seqLayerHeader->LUMSHIFT2;

    /* update buffer number */
    query_data->buf_number = buffer_counter;

    if (query_data->num_pictures > 2)
    {
        WTRACE("sampe buffer contains %d pictures", query_data->num_pictures);
    }
    return error;
}



static void vbp_pack_picture_params_vc1(
    vbp_context *pcontext,
    int index,
    vbp_picture_data_vc1* pic_data)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)cxt->codec_data;
    vc1_metadata_t *seqLayerHeader = &(parser->info.metadata);
    vc1_PictureLayerHeader *picLayerHeader = &(parser->info.picLayerHeader);


    VAPictureParameterBufferVC1 *pic_parms = pic_data->pic_parms;

    /* Then we get the picture header data.  Picture type need translation. */
    pic_parms->forward_reference_picture = VA_INVALID_SURFACE;
    pic_parms->backward_reference_picture = VA_INVALID_SURFACE;
    pic_parms->inloop_decoded_picture = VA_INVALID_SURFACE;

    pic_parms->sequence_fields.value = 0;
    pic_parms->sequence_fields.bits.pulldown = seqLayerHeader->PULLDOWN;
    pic_parms->sequence_fields.bits.interlace = seqLayerHeader->INTERLACE;
    pic_parms->sequence_fields.bits.tfcntrflag =  seqLayerHeader->TFCNTRFLAG;
    pic_parms->sequence_fields.bits.finterpflag = seqLayerHeader->FINTERPFLAG;
    pic_parms->sequence_fields.bits.psf = seqLayerHeader->PSF;
    pic_parms->sequence_fields.bits.multires = seqLayerHeader->MULTIRES;
    pic_parms->sequence_fields.bits.overlap = seqLayerHeader->OVERLAP;
    pic_parms->sequence_fields.bits.syncmarker = seqLayerHeader->SYNCMARKER;
    pic_parms->sequence_fields.bits.rangered = seqLayerHeader->RANGERED;
    pic_parms->sequence_fields.bits.max_b_frames = seqLayerHeader->MAXBFRAMES;

    pic_parms->coded_width = (seqLayerHeader->width + 1) << 1;
    pic_parms->coded_height = (seqLayerHeader->height + 1) << 1;

    pic_parms->entrypoint_fields.value = 0;
    pic_parms->entrypoint_fields.bits.closed_entry = seqLayerHeader->CLOSED_ENTRY;
    pic_parms->entrypoint_fields.bits.broken_link = seqLayerHeader->BROKEN_LINK;
    pic_parms->entrypoint_fields.bits.loopfilter = seqLayerHeader->LOOPFILTER;
    pic_parms->entrypoint_fields.bits.panscan_flag = seqLayerHeader->PANSCAN_FLAG;

    pic_parms->conditional_overlap_flag = picLayerHeader->CONDOVER;
    pic_parms->fast_uvmc_flag = seqLayerHeader->FASTUVMC;

    pic_parms->range_mapping_fields.value = 0;
    pic_parms->range_mapping_fields.bits.luma_flag = seqLayerHeader->RANGE_MAPY_FLAG;
    pic_parms->range_mapping_fields.bits.luma = seqLayerHeader->RANGE_MAPY;
    pic_parms->range_mapping_fields.bits.chroma_flag = seqLayerHeader->RANGE_MAPUV_FLAG;
    pic_parms->range_mapping_fields.bits.chroma = seqLayerHeader->RANGE_MAPUV;

    pic_parms->b_picture_fraction =
        vbp_map_bfraction(picLayerHeader->BFRACTION_NUM, picLayerHeader->BFRACTION_DEN);

    pic_parms->cbp_table = picLayerHeader->CBPTAB;
    pic_parms->mb_mode_table = picLayerHeader->MBMODETAB;
    pic_parms->range_reduction_frame = picLayerHeader->RANGEREDFRM;
    pic_parms->rounding_control = picLayerHeader->RNDCTRL;
    pic_parms->post_processing = picLayerHeader->POSTPROC;
    pic_parms->post_processing = seqLayerHeader->POSTPROCFLAG;
    /* fix this.  Add RESPIC to parser.  */
    pic_parms->picture_resolution_index = 0;
    pic_parms->luma_scale = picLayerHeader->LUMSCALE;
    pic_parms->luma_shift = picLayerHeader->LUMSHIFT;

    pic_parms->picture_fields.value = 0;
    switch (picLayerHeader->PTYPE)
    {
    case VC1_I_FRAME:
        pic_parms->picture_fields.bits.picture_type = VC1_PTYPE_I;
        break;

    case VC1_P_FRAME:
        pic_parms->picture_fields.bits.picture_type = VC1_PTYPE_P;
        break;

    case VC1_B_FRAME:
        pic_parms->picture_fields.bits.picture_type = VC1_PTYPE_B;
        break;

    case VC1_BI_FRAME:
        pic_parms->picture_fields.bits.picture_type = VC1_PTYPE_BI;
        break;

    case VC1_SKIPPED_FRAME:
        pic_data->picture_is_skipped = VC1_PTYPE_SKIPPED;
        break;

    default:
        // TODO: handle this case
        break;
    }
    pic_parms->picture_fields.bits.frame_coding_mode = picLayerHeader->FCM;
    if (0 == seqLayerHeader->PROFILE || 1 == seqLayerHeader->PROFILE)
    {
        /* simple or main profile, top field flag is not present, default to 1.*/
        pic_parms->picture_fields.bits.top_field_first = 1;
    }
    else
    {
        pic_parms->picture_fields.bits.top_field_first = picLayerHeader->TFF;
    }

    pic_parms->picture_fields.bits.is_first_field = !(picLayerHeader->CurrField);
    /* This seems to be set based on the MVMODE and MVMODE2 syntax. */
    /* This is a hack.  Probably will need refining. */
    if ((VC1_MVMODE_INTENSCOMP == picLayerHeader->MVMODE) ||
            (VC1_MVMODE_INTENSCOMP == picLayerHeader->MVMODE2))
    {
        pic_parms->picture_fields.bits.intensity_compensation = 1;
    }
    else
    {
        pic_parms->picture_fields.bits.intensity_compensation = picLayerHeader->INTCOMP;
    }

    /* Lets store the raw-mode BP bits. */
    pic_parms->raw_coding.value = 0;
    pic_parms->raw_coding.flags.mv_type_mb = picLayerHeader->raw_MVTYPEMB;
    pic_parms->raw_coding.flags.direct_mb = picLayerHeader->raw_DIRECTMB;
    pic_parms->raw_coding.flags.skip_mb = picLayerHeader->raw_SKIPMB;
    pic_parms->raw_coding.flags.field_tx = picLayerHeader->raw_FIELDTX;
    pic_parms->raw_coding.flags.forward_mb = picLayerHeader->raw_FORWARDMB;
    pic_parms->raw_coding.flags.ac_pred = picLayerHeader->raw_ACPRED;
    pic_parms->raw_coding.flags.overflags = picLayerHeader->raw_OVERFLAGS;

    /* imode 1/0 indicates bitmap presence in Pic Hdr. */
    pic_parms->bitplane_present.value = 0;

    pic_parms->bitplane_present.flags.bp_mv_type_mb =
        pic_parms->raw_coding.flags.mv_type_mb ? 1 :
        (picLayerHeader->MVTYPEMB.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_direct_mb =
        pic_parms->raw_coding.flags.direct_mb ? 1 :
        (picLayerHeader->DIRECTMB.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_skip_mb =
        pic_parms->raw_coding.flags.skip_mb ? 1 :
        (picLayerHeader->SKIPMB.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_field_tx =
        pic_parms->raw_coding.flags.field_tx ? 1 :
        (picLayerHeader->FIELDTX.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_forward_mb =
        pic_parms->raw_coding.flags.forward_mb ? 1 :
        (picLayerHeader->FORWARDMB.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_ac_pred =
        pic_parms->raw_coding.flags.ac_pred ? 1 :
        (picLayerHeader->ACPRED.imode ? 1: 0);

    pic_parms->bitplane_present.flags.bp_overflags =
        pic_parms->raw_coding.flags.overflags ? 1 :
        (picLayerHeader->OVERFLAGS.imode ? 1: 0);

    pic_parms->reference_fields.value = 0;
    pic_parms->reference_fields.bits.reference_distance_flag =
        seqLayerHeader->REFDIST_FLAG;

    pic_parms->reference_fields.bits.reference_distance =
        seqLayerHeader->REFDIST;

    pic_parms->reference_fields.bits.num_reference_pictures =
        picLayerHeader->NUMREF;

    pic_parms->reference_fields.bits.reference_field_pic_indicator =
        picLayerHeader->REFFIELD;

    pic_parms->mv_fields.value = 0;
    pic_parms->mv_fields.bits.mv_mode = picLayerHeader->MVMODE;
    pic_parms->mv_fields.bits.mv_mode2 = picLayerHeader->MVMODE2;

    pic_parms->mv_fields.bits.mv_table = picLayerHeader->MVTAB;
    pic_parms->mv_fields.bits.two_mv_block_pattern_table = picLayerHeader->MV2BPTAB;
    pic_parms->mv_fields.bits.four_mv_switch = picLayerHeader->MV4SWITCH;
    pic_parms->mv_fields.bits.four_mv_block_pattern_table = picLayerHeader->MV4BPTAB;
    pic_parms->mv_fields.bits.extended_mv_flag = seqLayerHeader->EXTENDED_MV;
    pic_parms->mv_fields.bits.extended_mv_range = picLayerHeader->MVRANGE;
    pic_parms->mv_fields.bits.extended_dmv_flag = seqLayerHeader->EXTENDED_DMV;
    pic_parms->mv_fields.bits.extended_dmv_range = picLayerHeader->DMVRANGE;

    pic_parms->pic_quantizer_fields.value = 0;
    pic_parms->pic_quantizer_fields.bits.dquant = seqLayerHeader->DQUANT;
    pic_parms->pic_quantizer_fields.bits.quantizer = seqLayerHeader->QUANTIZER;
    pic_parms->pic_quantizer_fields.bits.half_qp = picLayerHeader->HALFQP;
    pic_parms->pic_quantizer_fields.bits.pic_quantizer_scale = picLayerHeader->PQUANT;
    pic_parms->pic_quantizer_fields.bits.pic_quantizer_type = picLayerHeader->UniformQuant;
    pic_parms->pic_quantizer_fields.bits.dq_frame = picLayerHeader->DQUANTFRM;
    pic_parms->pic_quantizer_fields.bits.dq_profile = picLayerHeader->DQPROFILE;
    pic_parms->pic_quantizer_fields.bits.dq_sb_edge = picLayerHeader->DQSBEDGE;
    pic_parms->pic_quantizer_fields.bits.dq_db_edge = picLayerHeader->DQDBEDGE;
    pic_parms->pic_quantizer_fields.bits.dq_binary_level = picLayerHeader->DQBILEVEL;
    pic_parms->pic_quantizer_fields.bits.alt_pic_quantizer = picLayerHeader->ALTPQUANT;

    pic_parms->transform_fields.value = 0;
    pic_parms->transform_fields.bits.variable_sized_transform_flag =
        seqLayerHeader->VSTRANSFORM;

    pic_parms->transform_fields.bits.mb_level_transform_type_flag = picLayerHeader->TTMBF;
    pic_parms->transform_fields.bits.frame_level_transform_type = picLayerHeader->TTFRM;

    pic_parms->transform_fields.bits.transform_ac_codingset_idx1 =
        (picLayerHeader->TRANSACFRM > 0) ? picLayerHeader->TRANSACFRM - 1 : 0;

    pic_parms->transform_fields.bits.transform_ac_codingset_idx2 =
        (picLayerHeader->TRANSACFRM2 > 0) ? picLayerHeader->TRANSACFRM2 - 1 : 0;

    pic_parms->transform_fields.bits.intra_transform_dc_table = picLayerHeader->TRANSDCTAB;
    pic_parms->sequence_fields.bits.profile = seqLayerHeader->PROFILE;
}


static void vbp_pack_slice_data_vc1(
    vbp_context *pcontext,
    int index,
    vbp_picture_data_vc1* pic_data)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    uint32 slice_size = cxt->list.data[index].edpos - cxt->list.data[index].stpos;
    uint32 bit;
    uint32 byte;
    uint8 is_emul;
    viddec_pm_get_au_pos(cxt, &bit, &byte, &is_emul);

    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)cxt->codec_data;
    vc1_PictureLayerHeader *picLayerHeader = &(parser->info.picLayerHeader);

    vbp_slice_data_vc1 *slc_data = &(pic_data->slc_data[pic_data->num_slices]);
    VASliceParameterBufferVC1 *slc_parms = &(slc_data->slc_parms);

    /*uint32 data_offset = byte - cxt->list.data[index].stpos;*/

    slc_data->buffer_addr = cxt->parse_cubby.buf + cxt->list.data[index].stpos;
    slc_data->slice_size = slice_size;
    slc_data->slice_offset = 0;

    slc_parms->slice_data_size = slc_data->slice_size;
    slc_parms->slice_data_offset = 0;

    /* fix this.  we need to be able to handle partial slices. */
    slc_parms->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;

    slc_parms->macroblock_offset = bit + byte * 8;

    /* get the slice_vertical_position from the code */
    slc_parms->slice_vertical_position = (picLayerHeader->SLICE_ADDR % (pic_data->pic_parms->coded_height / 16));

    pic_data->num_slices++;
}

/**
 * process parsing result
 */
uint32_t vbp_process_parsing_result_vc1(vbp_context *pcontext, int index)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    uint32 error = VBP_OK;

    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)cxt->codec_data;
    if (parser->start_code != VC1_SC_FRM && 
        parser->start_code != VC1_SC_FLD &&
        parser->start_code != VC1_SC_SLC)
    {
        /* only handle frame data, field data and slice data here
         */
        return VBP_OK;
    }
    vbp_data_vc1 *query_data = (vbp_data_vc1 *)pcontext->query_data;

    if (parser->start_code == VC1_SC_FRM || parser->start_code == VC1_SC_FLD)
    {
        query_data->num_pictures++;
    }

    if (query_data->num_pictures > MAX_NUM_PICTURES)
    {
        ETRACE("Num of pictures per sample buffer exceeds the limit (%d).", MAX_NUM_PICTURES);
        return VBP_DATA;
    }

    if (query_data->num_pictures == 0)
    {
        ETRACE("Unexpected num of pictures.");
        return VBP_DATA;
    }

    /* start packing data */
    int picture_index = query_data->num_pictures - 1;
    vbp_picture_data_vc1* pic_data = &(query_data->pic_data[picture_index]);

    if (parser->start_code == VC1_SC_FRM || parser->start_code == VC1_SC_FLD)
    {
        /* setup picture parameter first*/
        vbp_pack_picture_params_vc1(pcontext, index, pic_data);

        /* setup bitplane after setting up picture parameter (so that bitplane_present is updated) */
        error = vbp_pack_bitplanes_vc1(pcontext, index, pic_data);
        if (VBP_OK != error)
        {
            ETRACE("Failed to pack bitplane.");
            return error;
        }

    }

    /* Always pack slice parameter. The first macroblock in the picture CANNOT
     * be preceeded by a slice header, so we will have first slice parsed always.
     *
     */

    if (pic_data->num_slices >= MAX_NUM_SLICES)
    {
        ETRACE("Num of slices exceeds the limit (%d).", MAX_NUM_SLICES);
        return VBP_DATA;
    }

    /* set up slice parameter */
    vbp_pack_slice_data_vc1(pcontext, index, pic_data);


    return VBP_OK;
}
