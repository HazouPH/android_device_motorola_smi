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

#include "vc1.h"
#include "h264.h"
#include "vbp_loader.h"
#include "vbp_utils.h"
#include "vbp_vc1_parser.h"
#include "vbp_h264_parser.h"
#include "vbp_mp42_parser.h"
#ifdef USE_HW_VP8
#include "vbp_vp8_parser.h"
#endif
#ifdef USE_AVC_SHORT_FORMAT
#include "vbp_h264secure_parser.h"
#endif


/* buffer counter */
uint32 buffer_counter = 0;


void* vbp_try_malloc0(uint32 size) {
    void* pMem = malloc(size);
    if (pMem)
        memset(pMem, 0, size);
    return pMem;
}

/**
 *
 * uninitialize parser context
 *
 */
static uint32 vbp_utils_uninitialize_context(vbp_context *pcontext)
{
    uint32 error = VBP_OK;

    if (NULL == pcontext)
    {
        return error;
    }

    /* not need to reset parser entry points. */

    free(pcontext->parser_ops);
    pcontext->parser_ops = NULL;


    if (pcontext->fd_parser)
    {
        dlclose(pcontext->fd_parser);
        pcontext->fd_parser = NULL;
    }

    return error;
}

/**
 *
 * initialize parser context
 *
 */
static uint32 vbp_utils_initialize_context(vbp_context *pcontext)
{
    uint32 error = VBP_OK;
    char *parser_name;

    switch (pcontext->parser_type)
    {
    case VBP_VC1:
#ifndef ANDROID
        parser_name = "libmixvbp_vc1.so.0";
#else
        parser_name = "libmixvbp_vc1.so";
#endif
        break;

        /* MPEG-2 parser is not supported. */

        /*  case VBP_MPEG2:
        parser_name = "libmixvbp_mpeg2.so.0";
        break;*/

    case VBP_MPEG4:
#ifndef ANDROID
        parser_name = "libmixvbp_mpeg4.so.0";
#else
        parser_name = "libmixvbp_mpeg4.so";
#endif
        break;

    case VBP_H264:
#ifndef ANDROID
        parser_name = "libmixvbp_h264.so.0";
#else
        parser_name = "libmixvbp_h264.so";
#endif
        break;
#ifdef USE_HW_VP8
    case VBP_VP8:
#ifndef ANDROID
        parser_name = "libmixvbp_vp8.so.0";
#else
        parser_name = "libmixvbp_vp8.so";
#endif
        break;
#endif

#ifdef USE_AVC_SHORT_FORMAT
    case VBP_H264SECURE:
        parser_name = "libmixvbp_h264secure.so";
        break;
#endif

    default:
        WTRACE("Unsupported parser type!");
        return VBP_TYPE;
    }

    pcontext->fd_parser = dlopen(parser_name, RTLD_LAZY);
    if (NULL == pcontext->fd_parser)
    {
        ETRACE("Failed to load parser %s.", parser_name);
        error =  VBP_LOAD;
        goto cleanup;
    }

    pcontext->parser_ops = vbp_malloc(viddec_parser_ops_t, 1);
    if (NULL == pcontext->parser_ops)
    {
        ETRACE("Failed to allocate memory");
        error =  VBP_MEM;
        goto cleanup;
    }

#define SET_FUNC_POINTER(X, Y)\
    case X:\
    pcontext->func_init_parser_entries = vbp_init_parser_entries_##Y;\
    pcontext->func_allocate_query_data = vbp_allocate_query_data_##Y;\
    pcontext->func_free_query_data = vbp_free_query_data_##Y;\
    pcontext->func_parse_init_data = vbp_parse_init_data_##Y;\
    pcontext->func_parse_start_code = vbp_parse_start_code_##Y;\
    pcontext->func_process_parsing_result = vbp_process_parsing_result_##Y;\
    pcontext->func_populate_query_data = vbp_populate_query_data_##Y;\
    break;

    switch (pcontext->parser_type)
    {
        SET_FUNC_POINTER(VBP_VC1, vc1);
        SET_FUNC_POINTER(VBP_MPEG4, mp42);
        SET_FUNC_POINTER(VBP_H264, h264);
#ifdef USE_HW_VP8
        SET_FUNC_POINTER(VBP_VP8, vp8);
#endif
#ifdef USE_AVC_SHORT_FORMAT
        SET_FUNC_POINTER(VBP_H264SECURE, h264secure);
#endif
    }
#ifdef USE_AVC_SHORT_FORMAT
    if (pcontext->parser_type == VBP_H264SECURE) {
        pcontext->func_update_data = vbp_update_data_h264secure;
    }
#endif

    /* set entry points for parser operations:
    	init
    	parse_sc
    	parse_syntax
    	get_cxt_size
    	is_wkld_done
    	is_frame_start
    */
    error = pcontext->func_init_parser_entries(pcontext);

cleanup:

    if (VBP_OK != error)
    {
        /* no need to log error.  the loader would have done so already. */
        vbp_utils_uninitialize_context(pcontext);
    }

    return error;
}

/**
*
* free allocated memory.
*
*/
static uint32 vbp_utils_free_parser_memory(vbp_context *pcontext)
{
    if (NULL == pcontext)
    {
        return VBP_OK;
    }

    if (pcontext->func_free_query_data)
    {
        pcontext->func_free_query_data(pcontext);
    }

    free(pcontext->workload2);
    pcontext->workload2 = NULL;

    free(pcontext->workload1);
    pcontext->workload1 = NULL;

    free(pcontext->persist_mem);
    pcontext->persist_mem = NULL;

    free(pcontext->parser_cxt);
    pcontext->parser_cxt = NULL;

    return VBP_OK;
}


/**
 *
 * allocate memory
 *
 */
static uint32 vbp_utils_allocate_parser_memory(vbp_context *pcontext)
{
    /* pcontext is guaranteed to be valid input. */
    uint32 error = VBP_OK;
    viddec_parser_memory_sizes_t sizes;

    pcontext->parser_cxt = vbp_malloc(viddec_pm_cxt_t, 1);
    if (NULL == pcontext->parser_cxt)
    {
        ETRACE("Failed to allocate memory");
        error = VBP_MEM;
        goto cleanup;
    }

    /* invoke parser entry to get context size */
    /* no return value, should always succeed. */
    pcontext->parser_ops->get_cxt_size(&sizes);

    /* allocate persistent memory for parser */
    if (sizes.persist_size)
    {
        pcontext->persist_mem = malloc(sizes.persist_size);
        if (NULL == pcontext->persist_mem)
        {
            ETRACE("Failed to allocate memory");
            error = VBP_MEM;
            goto cleanup;
        }
    }
    else
    {
        /* OK for VC-1, MPEG2 and MPEG4. */
        if ((VBP_VC1 == pcontext->parser_type) ||
            (VBP_MPEG2 == pcontext->parser_type) ||
            (VBP_MPEG4 == pcontext->parser_type)
#ifdef USE_HW_VP8
            || (VBP_VP8 == pcontext->parser_type)
#endif
)
        {
            pcontext->persist_mem = NULL;
        }
        else
        {
            /* mandatory for H.264 */
            ETRACE("Failed to allocate memory");
            error =  VBP_TYPE;
            goto cleanup;
        }
    }

    /* allocate a new workload with 1000 items. */
    pcontext->workload1 = malloc(sizeof(viddec_workload_t) +
                                       (MAX_WORKLOAD_ITEMS * sizeof(viddec_workload_item_t)));
    if (NULL == pcontext->workload1)
    {
        ETRACE("Failed to allocate memory");
        error = VBP_MEM;
        goto cleanup;
    }

    /* allocate a second workload with 1000 items. */
    pcontext->workload2 = malloc(sizeof(viddec_workload_t) +
                                       (MAX_WORKLOAD_ITEMS * sizeof(viddec_workload_item_t)));
    if (NULL == pcontext->workload2)
    {
        ETRACE("Failed to allocate memory");
        error = VBP_MEM;
        goto cleanup;
    }

    /* allocate format-specific query data */
    error = pcontext->func_allocate_query_data(pcontext);

cleanup:
    if (error != VBP_OK)
    {
        vbp_utils_free_parser_memory(pcontext);
    }
    return error;
}



/**
 *
 * parse the elementary sample buffer or codec configuration data
 *
 */
static uint32 vbp_utils_parse_es_buffer(vbp_context *pcontext, uint8 init_data_flag)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    viddec_parser_ops_t *ops = pcontext->parser_ops;
    uint32 error = VBP_OK;
    int i;

    /* reset list number. func_parse_init_data or func_parse_start_code will
    * set it equal to number of sequence headers, picture headers or slices headers
    * found in the sample buffer
    */
    cxt->list.num_items = 0;

    /**
    * READ THIS NOTE: cxt->getbits.is_emul_reqd must be set to 1
    * for H.264 and MPEG-4, VC1 advanced profile and set to 0
    * for VC1 simple or main profile when parsing the frame
    * buffer. When parsing the sequence header, it must be set to 1
    * always.
    *
    * PARSER IMPLEMENTOR: set this flag in the parser.
    */

    /*
    if ((codec_type == VBP_H264)  || (codec_type == VBP_MPEG4))
    {
    	cxt->getbits.is_emul_reqd = 1;
    }
    */


    /* populate the list.*/
    if (init_data_flag)
    {
        error = pcontext->func_parse_init_data(pcontext);
    }
    else
    {
        error = pcontext->func_parse_start_code(pcontext);
    }

    if (VBP_OK != error)
    {
        ETRACE("Failed to parse the start code!");
        return error;
    }

    /* set up bitstream buffer */
    cxt->getbits.list = &(cxt->list);

    /* setup buffer pointer */
    cxt->getbits.bstrm_buf.buf = cxt->parse_cubby.buf;

    // TODO: check if cxt->getbits.is_emul_reqd is set properly

    for (i = 0; i < cxt->list.num_items; i++)
    {
        /* setup bitstream parser */
        cxt->getbits.bstrm_buf.buf_index = cxt->list.data[i].stpos;
        cxt->getbits.bstrm_buf.buf_st = cxt->list.data[i].stpos;
        cxt->getbits.bstrm_buf.buf_end = cxt->list.data[i].edpos;

        /* It is possible to end up with buf_offset not equal zero. */
        cxt->getbits.bstrm_buf.buf_bitoff = 0;

        cxt->getbits.au_pos = 0;
        cxt->getbits.list_off = 0;
        cxt->getbits.phase = 0;
        cxt->getbits.emulation_byte_counter = 0;

        cxt->list.start_offset = cxt->list.data[i].stpos;
        cxt->list.end_offset = cxt->list.data[i].edpos;
        cxt->list.total_bytes = cxt->list.data[i].edpos - cxt->list.data[i].stpos;

        /* invoke parse entry point to parse the buffer */
        error = ops->parse_syntax((void *)cxt, (void *)&(cxt->codec_data[0]));

        /* can't return error for now. Neet further investigation */
        if (0 != error) {
            WTRACE("failed to parse the syntax: %d!", error);
            if (pcontext->parser_type == VBP_H264
#if (defined USE_AVC_SHORT_FORMAT || defined USE_SLICE_HEADER_PARSING)
                || pcontext->parser_type == VBP_H264SECURE
#endif
) {
                if (error == H264_SPS_INVALID_PROFILE) {
                    return VBP_ERROR;
                }
            }
        }

        /* process parsing result */
        error = pcontext->func_process_parsing_result(pcontext, i);

        if (VBP_MULTI == error) {
            return VBP_OK;
        }
        else if (0 != error)
        {
            ETRACE("Failed to process parsing result.");
            return error;
        }
    }

    return VBP_OK;
}


/**
 *
 * create the parser context
 *
 */
uint32 vbp_utils_create_context(uint32 parser_type, vbp_context **ppcontext)
{
    uint32 error = VBP_OK;
    vbp_context *pcontext = NULL;

    /* prevention from the failure */
    *ppcontext =  NULL;

    pcontext = vbp_malloc_set0(vbp_context, 1);
    if (NULL == pcontext)
    {
        error = VBP_MEM;
        goto cleanup;
    }

    pcontext->parser_type = parser_type;

    /* load parser, initialize parser operators and entry points */
    error = vbp_utils_initialize_context(pcontext);
    if (VBP_OK != error)
    {
        goto cleanup;
    }

    /* allocate parser context, persistent memory, query data and workload */
    error = vbp_utils_allocate_parser_memory(pcontext);
    if (VBP_OK != error)
    {
        goto cleanup;
    }

    viddec_pm_utils_list_init(&(pcontext->parser_cxt->list));
    viddec_pm_utils_bstream_init(&(pcontext->parser_cxt->getbits), NULL, 0);
    pcontext->parser_cxt->cur_buf.list_index = -1;
    pcontext->parser_cxt->parse_cubby.phase = 0;

    /* invoke the entry point to initialize the parser. */
    pcontext->parser_ops->init(
        (uint32_t *)pcontext->parser_cxt->codec_data,
        (uint32_t *)pcontext->persist_mem,
        FALSE);

    viddec_emit_init(&(pcontext->parser_cxt->emitter));

    /* overwrite init with our number of items. */
    pcontext->parser_cxt->emitter.cur.max_items = MAX_WORKLOAD_ITEMS;
    pcontext->parser_cxt->emitter.next.max_items = MAX_WORKLOAD_ITEMS;

    /* set up to find the first start code. */
    pcontext->parser_cxt->sc_prefix_info.first_sc_detect = 1;

    /* indicates initialized OK. */
    pcontext->identifier = MAGIC_NUMBER;
    *ppcontext = pcontext;
    error = VBP_OK;

cleanup:

    if (VBP_OK != error)
    {
        vbp_utils_free_parser_memory(pcontext);
        vbp_utils_uninitialize_context(pcontext);
        free(pcontext);
        pcontext = NULL;
    }

    return error;
}

/**
 *
 * destroy the context.
 *
 */
uint32 vbp_utils_destroy_context(vbp_context *pcontext)
{
    /* entry point, not need to validate input parameters. */
    vbp_utils_free_parser_memory(pcontext);
    vbp_utils_uninitialize_context(pcontext);
    free(pcontext);
    pcontext = NULL;

    return VBP_OK;
}


/**
 *
 * parse the sample buffer or parser configuration data.
 *
 */
uint32 vbp_utils_parse_buffer(vbp_context *pcontext, uint8 *data, uint32 size,  uint8 init_data_flag)
{
    /* entry point, not need to validate input parameters. */

    uint32 error = VBP_OK;

    //ITRACE("buffer counter: %d",buffer_counter);

    /* set up emitter. */
    pcontext->parser_cxt->emitter.cur.data = pcontext->workload1;
    pcontext->parser_cxt->emitter.next.data = pcontext->workload2;

    /* reset bit offset */
    pcontext->parser_cxt->getbits.bstrm_buf.buf_bitoff = 0;


    /* set up cubby. */
    pcontext->parser_cxt->parse_cubby.buf = data;
    pcontext->parser_cxt->parse_cubby.size = size;
    pcontext->parser_cxt->parse_cubby.phase = 0;

    error = vbp_utils_parse_es_buffer(pcontext, init_data_flag);

    /* rolling count of buffers. */
    if (0 == init_data_flag)
    {
        buffer_counter++;
    }
    return error;
}

/**
 *
 * provide query data back to the consumer
 *
 */
uint32 vbp_utils_query(vbp_context *pcontext, void **data)
{
    /* entry point, not need to validate input parameters. */
    uint32 error = VBP_OK;

    error = pcontext->func_populate_query_data(pcontext);
    if (VBP_OK == error)
    {
        *data = pcontext->query_data;
    }
    else
    {
        *data = NULL;
    }
    return error;
}

/**
 *
 * flush parsing buffer. Currently always succeed.
 *
 */
uint32 vbp_utils_flush(vbp_context *pcontext)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    viddec_parser_ops_t *ops = pcontext->parser_ops;
    if (ops->flush != NULL) {
        ops->flush((void *)cxt, (void *)&(cxt->codec_data[0]));
    }
    return VBP_OK;
}


#ifdef USE_AVC_SHORT_FORMAT
/**
 *
 * provide query data back to the consumer
 *
 */
uint32 vbp_utils_update(vbp_context *pcontext, void *newdata, uint32 size, void **data)
{
    /* entry point, not need to validate input parameters. */
    uint32 error = VBP_OK;

    error = pcontext->func_update_data(pcontext,newdata,size);

    if (VBP_OK == error)
    {
        *data = pcontext->query_data;
    }
    else
    {
        *data = NULL;
    }
    return error;
}
#endif
