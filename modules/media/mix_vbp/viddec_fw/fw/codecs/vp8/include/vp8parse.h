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

#ifndef _VP8PARSE_H_
#define _VP8PARSE_H_

#include "vp8.h"

#ifdef __cplusplus
extern "C" {
#endif

int32_t vp8_parse_frame_tag(FrameTagHeader *frame_tag, uint8_t *data, uint32_t data_sz);

//vp8_Status vp8_translate_parse_status(vp8_Status status);

void vp8_init_Info(vp8_Info *pi);

void vp8_init_frame(vp8_Info *pi);

void vp8_parse_segmentation_adjustments_data(vp8_Info *pi);

void vp8_parse_loop_filter_type_level(vp8_Info *pi);

void vp8_parse_loop_filter_adjustments_data(vp8_Info *pi);

int32_t vp8_read_partition_size(uint8_t *cx_size);

void vp8_parse_token_partition_data(vp8_Info *pi, uint8_t *cx_size);

int read_q_delta(BOOL_CODER *bool_coder);

void vp8_parse_dequantization_indices(vp8_Info *pi);

void vp8_parse_gf_af_refresh_flags(vp8_Info *pi);

void vp8_parse_coef_probs_tree(vp8_Info *pi);

void vp8_parse_mb_mv_info(vp8_Info *pi);

void vp8_parse_yuv_probs_update(vp8_Info *pi);

void vp8_parse_remaining_frame_header_data(vp8_Info *pi);

int32_t vp8_parse_frame_header(vp8_viddec_parser *parser);

#ifdef __cplusplus
}
#endif
#endif
