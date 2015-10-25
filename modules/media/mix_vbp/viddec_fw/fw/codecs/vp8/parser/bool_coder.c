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

#include "bool_coder.h"

uint32_t vp8_read_bits(BOOL_CODER *br, int32_t bits)
{
    uint32_t z = 0;
    int bit;
    for (bit=bits-1; bit>=0; bit--)
    {
        z |= (vp8_decode_bool(br, 128)<<bit);
    }
    return z;
}

void vp8_start_decode(BOOL_CODER *br, uint8_t *source)
{
    br->range    = 255;
    br->count    = 8;
    br->buffer   = source;
    br->pos      = 0;
    br->value    = (br->buffer[0]<<24)+(br->buffer[1]<<16)+(br->buffer[2]<<8)+(br->buffer[3]);
    br->pos     += 4;
}

int32_t vp8_decode_bool(BOOL_CODER *br, int32_t probability)
{
    uint32_t bit=0;
    uint32_t split;
    uint32_t bigsplit;
    uint32_t count = br->count;
    uint32_t range = br->range;
    uint32_t value = br->value;

    split = 1 +  (((range-1) * probability) >> 8);
    bigsplit = (split<<24);

    range = split;
    if(value >= bigsplit)
    {
        range = br->range-split;
        value = value-bigsplit;
        bit = 1;
    }

    if(range>=0x80)
    {
        br->value = value;
        br->range = range;
        return bit;
    }
    else
    {
        do
        {
            range +=range;
            value +=value;

            if (!--count)
            {
                count = 8;
                value |= br->buffer[br->pos];
                br->pos++;
            }
        }
        while(range < 0x80 );
    }
    br->count = count;
    br->value = value;
    br->range = range;
    return bit;
}
