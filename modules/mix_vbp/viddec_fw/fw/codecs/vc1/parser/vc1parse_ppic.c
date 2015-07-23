/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive P picture in simple
//  or main profile bitstream.
//
*/

#include "vc1parse.h"

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive P picture for simple
 * or main profile bitstream.  This parser starts after PTYPE was parsed but
 * stops before parsing of macroblock layer.
 * Table 19 of SMPTE 421M after processing up to PTYPE for P picture.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressivePpicture(void* ctxt, vc1_Info *pInfo)
{
    uint8_t bit_count;
    const uint8_t *table;
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    /* rounding control is implied for simple and main profile, SMPTE 421M 8.3.7.
       It toggles back and forth between 0 and 1 for P frames */
    if (md->PROFILE != VC1_PROFILE_ADVANCED)
    {
        picLayerHeader->RNDCTRL = md->RNDCTRL ^ 1 ;
        md->RNDCTRL = picLayerHeader->RNDCTRL;
    }

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

    /* MVRANGE. */
    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if (md->MULTIRES == 1)
        VC1_GET_BITS9(2, tempValue); /* RESPIC. */

    if (picLayerHeader->PQUANT > 12)
        table = VC1_MVMODE_LOW_TBL;
    else
        table = VC1_MVMODE_HIGH_TBL;

    bit_count = 0;
    VC1_GET_BITS9(1, picLayerHeader->MVMODE);
    while ((picLayerHeader->MVMODE == 0) && (bit_count < 3))
    {
        VC1_GET_BITS9(1, picLayerHeader->MVMODE);
        bit_count++;
    }
    if (bit_count == 3)
        bit_count += picLayerHeader->MVMODE;
    picLayerHeader->MVMODE = table[bit_count];

    if (picLayerHeader->MVMODE == VC1_MVMODE_INTENSCOMP)
    {
        bit_count = 0;
        VC1_GET_BITS9(1, picLayerHeader->MVMODE2);
        while ((picLayerHeader->MVMODE2 == 0) && (bit_count < 2))
        {
            VC1_GET_BITS9(1, picLayerHeader->MVMODE2);
            bit_count++;
        }
        if (bit_count == 2 && picLayerHeader->MVMODE2 == 0)
            bit_count++;
        picLayerHeader->MVMODE2 = table[bit_count];
        VC1_GET_BITS9(6, picLayerHeader->LUMSCALE);
        VC1_GET_BITS9(6, picLayerHeader->LUMSHIFT);
    }
    else
#ifdef VBP
        picLayerHeader->MVMODE2 = 0;
#else
        picLayerHeader->MVMODE2 = picLayerHeader->MVMODE;
#endif

    if ((picLayerHeader->MVMODE == VC1_MVMODE_MIXED_MV) ||
            ((picLayerHeader->MVMODE == VC1_MVMODE_INTENSCOMP) &&
             (picLayerHeader->MVMODE2 == VC1_MVMODE_MIXED_MV)))
    {
        if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                         md->widthMB, md->heightMB, BPP_MVTYPEMB))
                != VC1_STATUS_OK)
        {
            return status;
        }
    }

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_SKIPMB)) != VC1_STATUS_OK)
    {
        return status;
    }

    VC1_GET_BITS9(2, picLayerHeader->MVTAB);
    VC1_GET_BITS9(2, picLayerHeader->CBPTAB);

    if ((status = vc1_VOPDQuant(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if (md->VSTRANSFORM == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->TTMBF);
        if (picLayerHeader->TTMBF == 1)
        {
            VC1_GET_BITS9(2, picLayerHeader->TTFRM);
        }
    }

    VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM);
    if (picLayerHeader->TRANSACFRM == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->TRANSACFRM);
        picLayerHeader->TRANSACFRM += 2;
    }

    VC1_GET_BITS9(1, picLayerHeader->TRANSDCTAB);

    /* Skip parsing of macroblock layer. */

    return status;
}

