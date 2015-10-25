/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive I picture in simple
//  or main profile bitstream or progressive BI picture in main profile
//  bitstream.
//
*/

#include "vc1parse.h"

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive I picture for simple
 * or main profile bitstream or progressive BI picture in main profile
 * bitstream.  This parser starts after PTYPE was parsed but stops before
 * parsing of macroblock layer.
 * Table 16 of SMPTE 421M after processing up to PTYPE for I picture.
 * Table 17 of SMPTE 421M after processing up to PTYPE for BI picture.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressiveIpicture(void* ctxt, vc1_Info *pInfo)
{
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    /* rounding control is implied for simple and main profile, SMPTE 421M 8.3.7.
    For each I or BI frame, RND shall be set to 1 */
    if (md->PROFILE != VC1_PROFILE_ADVANCED)
    {
        picLayerHeader->RNDCTRL = md->RNDCTRL | 1 ;
        md->RNDCTRL = picLayerHeader->RNDCTRL;
    }


    if (picLayerHeader->PTYPE == VC1_BI_FRAME)
    {
        if ((status = vc1_DecodeHuffmanPair(ctxt, VC1_BFRACTION_TBL,
                                            &picLayerHeader->BFRACTION_NUM, &picLayerHeader->BFRACTION_DEN))
                != VC1_STATUS_OK)
        {
            return status;
        }
        if (picLayerHeader->BFRACTION_DEN != VC1_BFRACTION_BI)
            return VC1_STATUS_PARSE_ERROR;
    }

    VC1_GET_BITS9(7, tempValue); /* BF. */
    VC1_GET_BITS9(5, picLayerHeader->PQINDEX);

    if ((status = vc1_CalculatePQuant(pInfo)) != VC1_STATUS_OK)
        return status;

    if (picLayerHeader->PQINDEX <= 8)
    {
        VC1_GET_BITS9(1, picLayerHeader->HALFQP);
    }
    else picLayerHeader->HALFQP=0;

    if (md->QUANTIZER == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->PQUANTIZER);
        picLayerHeader->UniformQuant = picLayerHeader->PQUANTIZER;
    }

    /* MVRANGE but only for main profile. */
    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if (md->MULTIRES == 1 && picLayerHeader->PTYPE != VC1_BI_FRAME)
    {
        VC1_GET_BITS9(2, tempValue); /* RESPIC. */
    }

    VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM);
    if (picLayerHeader->TRANSACFRM)
    {
        VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM += 2;
    }

    VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM2);
    if (picLayerHeader->TRANSACFRM2)
    {
        VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM2);
        picLayerHeader->TRANSACFRM2 += 2;
    }

    VC1_GET_BITS9(1, picLayerHeader->TRANSDCTAB);

    /* Skip parsing of macroblock layer. */

    return status;
}
