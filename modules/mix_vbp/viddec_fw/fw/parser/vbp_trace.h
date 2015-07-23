/*
 INTEL CONFIDENTIAL
 Copyright 2009 Intel Corporation All Rights Reserved.
 The source code contained or described herein and all documents related to the source code ("Material") are owned by Intel Corporation or its suppliers or licensors. Title to the Material remains with Intel Corporation or its suppliers and licensors. The Material contains trade secrets and proprietary and confidential information of Intel or its suppliers and licensors. The Material is protected by worldwide copyright and trade secret laws and treaty provisions. No part of the Material may be used, copied, reproduced, modified, published, uploaded, posted, transmitted, distributed, or disclosed in any way without Intel’s prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual property right is granted to or conferred upon you by disclosure or delivery of the Materials, either expressly, by implication, inducement, estoppel or otherwise. Any license under such intellectual property rights must be express and approved by Intel in writing.
 */


#ifndef VBP_TRACE_H_
#define VBP_TRACE_H_



#define VBP_TRACE


#ifdef VBP_TRACE /* if VBP_TRACE is defined*/

#ifndef ANDROID

#include <stdio.h>
#include <stdarg.h>

extern void vbp_trace_util(const char* cat, const char* fun, int line, const char* format, ...);
#define VBP_TRACE_UTIL(cat, format, ...) \
vbp_trace_util(cat, __FUNCTION__, __LINE__, format,  ##__VA_ARGS__)


#define ETRACE(format, ...) VBP_TRACE_UTIL("ERROR:   ",  format, ##__VA_ARGS__)
#define WTRACE(format, ...) VBP_TRACE_UTIL("WARNING: ",  format, ##__VA_ARGS__)
#define ITRACE(format, ...) VBP_TRACE_UTIL("INFO:    ",  format, ##__VA_ARGS__)
#define VTRACE(format, ...) VBP_TRACE_UTIL("VERBOSE: ",  format, ##__VA_ARGS__)


#else

// For Android OS

//#define LOG_NDEBUG 0

#define LOG_TAG "MixVBP"

#include <wrs_omxil_core/log.h>
#define ETRACE(...) LOGE(__VA_ARGS__)
#define WTRACE(...) LOGW(__VA_ARGS__)
#define ITRACE(...) LOGI(__VA_ARGS__)
#define VTRACE(...) LOGV(__VA_ARGS__)

#endif


#else /* if VBP_TRACE is not defined */

#define ETRACE(format, ...)
#define WTRACE(format, ...)
#define ITRACE(format, ...)
#define VTRACE(format, ...)


#endif /* VBP_TRACE*/


#endif /*VBP_TRACE_H_*/


