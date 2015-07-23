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

#ifndef _BOOL_CODER_H_
#define _BOOL_CODER_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

typedef struct _BOOL_CODER
{
    uint32_t  range; // always idential to encoder's range
    uint32_t  value; // contains at least 24 significant bits
    int32_t   count; // # of bits shifted out of value, at most 7
    uint32_t  pos;
    uint8_t   *buffer; // pointer to next compressed data byte to be read
} BOOL_CODER;

typedef struct _BITREADER
{
    int32_t        bitsinremainder; // # of bits still used in remainder
    uint32_t       remainder;       // remaining bits from original long
    const uint8_t *position;        // character pointer position within data
} BITREADER;

void vp8_start_decode(BOOL_CODER *br, uint8_t *source);
int32_t vp8_decode_bool(BOOL_CODER *br, int32_t probability);
uint32_t vp8_read_bits(BOOL_CODER *br, int32_t bits);

#endif
