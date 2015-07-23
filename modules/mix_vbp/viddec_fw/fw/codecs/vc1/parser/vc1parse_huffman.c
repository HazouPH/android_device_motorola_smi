/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VLC syntax elements within VC-1 bitstream.
//
*/

#include "vc1parse.h"

/*----------------------------------------------------------------------------*/

vc1_Status vc1_DecodeHuffmanOne(void* ctxt, int32_t *pDst, const int32_t *pDecodeTable)
{
    uint32_t tempValue;
    const int32_t *pTable = pDecodeTable;
    vc1_Status status = VC1_STATUS_OK;
    int32_t i, j, maxBits, loopCount, totalBits, value;

    maxBits = *pTable++;
    loopCount = *pTable++;
    totalBits = 0;
    for (i = 0; i < loopCount; i++)
        totalBits += *pTable++;

    if (totalBits != maxBits)
        return VC1_STATUS_PARSE_ERROR;

    value = 0;
    for (i = 0; i < maxBits; i++)
    {
        VC1_GET_BITS9(1, tempValue);
        value = (value << 1) | tempValue;
        loopCount = *pTable++;
        if (loopCount == -1)
            break;
        for (j = 0; j < loopCount; j++)
        {
            if (value == *pTable++)
            {
                *pDst = *pTable;
                return status;
            }
            else
                pTable++;
        }
    }

    return status;
}

/*----------------------------------------------------------------------------*/

vc1_Status vc1_DecodeHuffmanPair(void* ctxt, const int32_t *pDecodeTable,
                                 int8_t *pFirst, int16_t *pSecond)
{
    uint32_t tempValue;
    const int32_t *pTable = pDecodeTable;
    vc1_Status status = VC1_STATUS_OK;
    int32_t i, j, maxBits, loopCount, totalBits, value;

    maxBits = *pTable++;
    loopCount = *pTable++;
    totalBits = 0;
    for (i = 0; i < loopCount; i++)
        totalBits += *pTable++;

    if (totalBits != maxBits)
        return VC1_STATUS_PARSE_ERROR;

    value = 0;
    for (i = 0; i < maxBits; i++)
    {
        VC1_GET_BITS9(1, tempValue);
        value = (value << 1) | tempValue;
        loopCount = *pTable++;
        if (loopCount == -1)
            break;
        for (j = 0; j < loopCount; j++)
        {
            if (value == *pTable++)
            {
                *pFirst = *pTable++;
                *pSecond = *pTable;
                return status;
            }
            else
                pTable += 2;
        }
    }

    return status;
}
