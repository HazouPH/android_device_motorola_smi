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


#include "vbp_loader.h"
#include "vbp_utils.h"

/**
 *
 */
uint32 vbp_open(uint32 parser_type, Handle *hcontext)
{
    vbp_context **ppcontext;
    uint32 error;

    if (NULL == hcontext)
    {
        return VBP_PARM;
    }

    *hcontext = NULL;  /* prepare for failure. */

    ppcontext = (vbp_context **)hcontext;

    // TODO: check if vbp context has been created.


    error = vbp_utils_create_context(parser_type, ppcontext);
    if (VBP_OK != error)
    {
        ETRACE("Failed to create context: %d.", error);
    }

    return error;
}

/**
 *
 */
uint32 vbp_close(Handle hcontext)
{
    uint32 error;

    if (NULL == hcontext)
    {
        return VBP_PARM;
    }

    vbp_context *pcontext = (vbp_context *)hcontext;

    if (MAGIC_NUMBER != pcontext->identifier)
    {
        /* not a valid vbp context. */
        ETRACE("context is not initialized");
        return VBP_INIT;
    }
    error = vbp_utils_destroy_context(pcontext);
    if (VBP_OK != error)
    {
        ETRACE("Failed to destroy context: %d.", error);
    }

    return error;
}


/**
 *
 */
uint32 vbp_parse(Handle hcontext, uint8 *data, uint32 size, uint8 init_data_flag)
{
    vbp_context *pcontext;
    uint32 error = VBP_OK;

    if ((NULL == hcontext) || (NULL == data) || (0 == size))
    {
        ETRACE("Invalid input parameters.");
        return VBP_PARM;
    }

    pcontext = (vbp_context *)hcontext;

    if (MAGIC_NUMBER != pcontext->identifier)
    {
        ETRACE("context is not initialized");
        return VBP_INIT;
    }

    error = vbp_utils_parse_buffer(pcontext, data, size, init_data_flag);

    if (VBP_OK != error)
    {
        ETRACE("Failed to parse buffer: %d.", error);
    }
    return error;
}

/**
 *
 */
uint32 vbp_query(Handle hcontext, void **data)
{
    vbp_context *pcontext;
    uint32 error = VBP_OK;

    if ((NULL == hcontext) || (NULL == data))
    {
        ETRACE("Invalid input parameters.");
        return VBP_PARM;
    }

    pcontext = (vbp_context *)hcontext;

    if (MAGIC_NUMBER != pcontext->identifier)
    {
        ETRACE("context is not initialized");
        return VBP_INIT;
    }

    error = vbp_utils_query(pcontext, data);

    if (VBP_OK != error)
    {
        ETRACE("Failed to query parsing result: %d.", error);
    }
    return error;
}

/**
 *
 */
uint32 vbp_flush(Handle hcontext)
{
    vbp_context *pcontext;
    uint32 error = VBP_OK;

    if (NULL == hcontext)
    {
        ETRACE("Invalid input parameters.");
        return VBP_PARM;
    }

    pcontext = (vbp_context *)hcontext;

    if (MAGIC_NUMBER != pcontext->identifier)
    {
        ETRACE("context is not initialized");
        return VBP_INIT;
    }

    error = vbp_utils_flush(pcontext);

    return error;
}

#ifdef USE_AVC_SHORT_FORMAT
uint32 vbp_update(Handle hcontext, void *newdata, uint32 size, void **data)
{
    vbp_context *pcontext;
    uint32 error = VBP_OK;

    if ((NULL == hcontext) || (NULL == newdata) || (0 == size) || (NULL == data))
    {
        ETRACE("Invalid input parameters.");
        return VBP_PARM;
    }

    pcontext = (vbp_context *)hcontext;

    if (MAGIC_NUMBER != pcontext->identifier)
    {
        ETRACE("context is not initialized");
        return VBP_INIT;
    }

    error = vbp_utils_update(pcontext, newdata, size, data);

    if (VBP_OK != error)
    {
        ETRACE("Failed to query parsing result: %d.", error);
    }
    return error;
}
#endif
