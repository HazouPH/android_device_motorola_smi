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

#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"

#include "viddec_fw_workload.h"
#include "viddec_pm.h"

#include <wrs_omxil_core/log.h>
#include "vp8.h"
#include "vp8parse.h"
#include "viddec_vp8_parse.h"

/* Init function which can be called to intialized local context on open and flush and preserve*/
void viddec_vp8_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    vp8_viddec_parser* parser = ctxt;
    vp8_Info *pi = &(parser->info);

    /* Avoid compiler warning */
    persist_mem = persist_mem;

    if (!preserve)
    {
        /* Init frame header information */
        vp8_init_Info(pi);
    }
    else
    {
       /* Initialise the parser */
       pi->decoded_frame_number = 0;
       pi->refresh_entropy_lf = 1;
    }

    parser->got_start = 1;
    return;
}

uint32_t viddec_vp8_parse(void *parent, void *ctxt)
{
    vp8_Status status = VP8_NO_ERROR;

    vp8_viddec_parser *parser = (vp8_viddec_parser*)ctxt;
    if (1 != parser->got_start) return VP8_NO_INITIALIZATION;

    vp8_Info *pi = &(parser->info);
    viddec_pm_cxt_t *pm_cxt = (viddec_pm_cxt_t *)parent;
    pi->source = pm_cxt->parse_cubby.buf;
    pi->source_sz = pm_cxt->parse_cubby.size;

    if (pi->source_sz < 0)
    {
        return VP8_UNEXPECTED_END_OF_BITSTREAM;
    }
    else if (pi->source_sz == 0)
    {
        pi->frame_tag.frame_type = SKIPPED_FRAME;
        status = VP8_NO_ERROR;
    }
    else if (pi->source_sz > 0)
    {
        status = vp8_parse_frame_header(parser);
    }

    return status;
}

uint32_t viddec_vp8_wkld_done(void *parent, void *ctxt, unsigned int next_sc,
                              uint32_t *codec_specific_errors)
{
    return 0;
}

void viddec_vp8_get_context_size(viddec_parser_memory_sizes_t *size)
{
    /* Should return size of my structure */
    size->context_size = sizeof(vp8_viddec_parser);
    size->persist_size = 0;
    return;
}

uint32_t viddec_vp8_is_frame_start(void *ctxt)
{
    vp8_viddec_parser* parser = ctxt;

    return parser->got_start;
}

void viddec_vp8_get_ops(viddec_parser_ops_t *ops)
{
    ops->init = viddec_vp8_init;

    ops->parse_syntax = viddec_vp8_parse;
    ops->get_cxt_size = viddec_vp8_get_context_size;
    ops->is_wkld_done = viddec_vp8_wkld_done;
    ops->is_frame_start = viddec_vp8_is_frame_start;
    return;
}
