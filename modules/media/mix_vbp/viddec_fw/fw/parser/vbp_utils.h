/* INTEL CONFIDENTIAL
* Copyright (c) 2009 Intel Corporation.  All rights reserved.
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


#ifndef VBP_UTILS_H
#define VBP_UTILS_H

#include "viddec_parser_ops.h"
#include "viddec_pm_parse.h"
#include "viddec_pm.h"
#include "vbp_trace.h"
#include <stdlib.h>

#define MAGIC_NUMBER 0x0DEADBEEF
#define MAX_WORKLOAD_ITEMS 1000

/* maximum 256 slices per sample buffer */
#define MAX_NUM_SLICES 256

/* maximum two pictures per sample buffer */
#define MAX_NUM_PICTURES 2


#define vbp_malloc(struct_type, n_structs) \
    ((struct_type *) malloc(sizeof(struct_type) * n_structs))

#define vbp_malloc_set0(struct_type, n_structs) \
    ((struct_type *) vbp_try_malloc0(sizeof(struct_type) * n_structs))



extern uint32 viddec_parse_sc(void *in, void *pcxt, void *sc_state);

/* rolling counter of sample buffer */
extern uint32 buffer_counter;

typedef struct vbp_context_t vbp_context;

typedef uint32 (*function_init_parser_entries)(vbp_context* cxt);
typedef uint32 (*function_allocate_query_data)(vbp_context* cxt);
typedef uint32 (*function_free_query_data)(vbp_context* cxt);
typedef uint32 (*function_parse_init_data)(vbp_context* cxt);
typedef uint32 (*function_parse_start_code)(vbp_context* cxt);
typedef uint32 (*function_process_parsing_result)(vbp_context* cxt, int i);
typedef uint32 (*function_populate_query_data)(vbp_context* cxt);
#ifdef USE_AVC_SHORT_FORMAT
typedef uint32 (*function_update_data)(vbp_context* cxt, void *newdata, uint32 size);
#endif

struct vbp_context_t
{
    /* magic number */
    uint32 identifier;

    /* parser type, eg, MPEG-2, MPEG-4, H.264, VC1 */
    uint32 parser_type;

    /* handle to parser (shared object) */
    void *fd_parser;

    /* parser (shared object) entry points */
    viddec_parser_ops_t *parser_ops;

    /* parser context */
    viddec_pm_cxt_t *parser_cxt;

    /* work load */
    viddec_workload_t *workload1, *workload2;

    /* persistent memory for parser */
    uint32 *persist_mem;

    /* format specific query data */
    void *query_data;

    /* parser type specific data*/
    void *parser_private;

    function_init_parser_entries func_init_parser_entries;
    function_allocate_query_data func_allocate_query_data;
    function_free_query_data func_free_query_data;
    function_parse_init_data func_parse_init_data;
    function_parse_start_code func_parse_start_code;
    function_process_parsing_result func_process_parsing_result;
    function_populate_query_data func_populate_query_data;
#ifdef USE_AVC_SHORT_FORMAT
    function_update_data func_update_data;
#endif
};


void* vbp_try_malloc0(uint32 size);

/**
 * create VBP context
 */
uint32 vbp_utils_create_context(uint32 parser_type, vbp_context **ppcontext);

/*
 * destroy VBP context
 */
uint32 vbp_utils_destroy_context(vbp_context *pcontext);

/*
 * parse bitstream
 */
uint32 vbp_utils_parse_buffer(vbp_context *pcontext, uint8 *data, uint32 size, uint8 init_data_flag);

/*
 * query parsing result
 */
uint32 vbp_utils_query(vbp_context *pcontext, void **data);

/*
 * flush un-parsed bitstream
 */
uint32 vbp_utils_flush(vbp_context *pcontext);

#endif /* VBP_UTILS_H */
