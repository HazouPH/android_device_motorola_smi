/******************************************************************************

INTEL CONFIDENTIAL
Copyright 2009 - 2011 Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its
suppliers and licensors. The Material may contain trade secrets and
proprietary and confidential information of Intel Corporation and its
suppliers and licensors, and is protected by worldwide copyright and trade
secret laws and treaty provisions. No part of the Material may be used, copied,
reproduced, modified, published, uploaded, posted, transmitted, distributed,
or disclosed in any way without Intel's prior express written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery of
the Materials, either expressly, by implication, inducement, estoppel or
otherwise. Any license under such intellectual property rights must be express
and approved by Intel in writing

File Name:          iasrc_resampler.h
Description:        The resampler header file with API definitions
Author:             Vijay H Sankar - UMG/UMPE
Date Created: April 27, 2011
Revision:   1.0:         File created

******************************************************************************/
#ifndef IASRC_RESAMPLER_H
#define IASRC_RESAMPLER_H

#include <stdio.h>
#include <stdint.h>

/*
    Intel context/state for the optimized resampler
*/
#ifdef __cplusplus
extern "C" {
#endif

	/*
	   To check whether the sample rate conversion is supported
	 */
	int iaresamplib_supported_conversion(int ip_samplerate,
					     int op_samplerate);

	/*
	 * The IA architecture optimized resampler
	 */
	int iaresamplib_process_float(void *ctx, float *inp,
				      unsigned in_n_frames, float *out,
				      unsigned *out_n_frames);

	/*
	 * Create a new resampler for the given input and output sampling rates
	 */
	int iaresamplib_new(void **ctx, int num_channels,
			    int iprate, int oprate);

	/*
	 * Free the resampler
	 */
	int iaresamplib_delete(void **ctx);

	/*
	 * Reset the resampler
	 */
	int iaresamplib_reset(void *ctx);
	/* Convert from short to float */
	void iaresamplib_convert_short_2_float(int16_t *inp,
					       float *out, size_t sz);
	/* convert to output format */
	void iaresamplib_convert_2_output_format(float *inp,
						 int32_t *out,
						 size_t sz,
						 int32_t channels,
						 int16_t *vol);

#ifdef __cplusplus
}
#endif
#endif				/*IASRC_RESAMPLER_H */
