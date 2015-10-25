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

#ifndef VBP_VC1_PARSER_H
#define VBP_VC1_PARSER_H


/*
 * setup parser's entry pointer
 */
uint32 vbp_init_parser_entries_vc1(vbp_context *pcontext);

/*
 * allocate query data structure - vbp_vc1_data
 */
uint32 vbp_allocate_query_data_vc1(vbp_context *pcontext);

/*
 * free query data structure
 */
uint32 vbp_free_query_data_vc1(vbp_context *pcontext);

/*
 * parse bitstream configuration data
 */
uint32 vbp_parse_init_data_vc1(vbp_context *pcontext);

/*
 * parse bitstream start code and fill the viddec_input_buffer_t list.
 * WMV has no start code so the whole buffer will be treated as a single frame.
 * For VC1 progressive, if start code is not found, the whole buffer will be treated as a
 * single frame as well.
 * For VC1 interlace, the first field is not start code prefixed, but the second field
 * is always start code prefixed.
 */
uint32 vbp_parse_start_code_vc1(vbp_context *pcontext);

/*
 * processe parsing result
 */
uint32 vbp_process_parsing_result_vc1(vbp_context *pcontext, int list_index);

/*
 * populate query data structure
 */
uint32 vbp_populate_query_data_vc1(vbp_context *pcontext);


#endif /*VBP_VC1_PARSER_H*/
