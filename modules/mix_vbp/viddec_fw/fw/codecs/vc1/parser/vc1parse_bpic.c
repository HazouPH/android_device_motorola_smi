/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive B picture in simple
//  or main profile bitstream.
//
*/

#include "vc1parse.h"
#include "viddec_fw_debug.h"    // For DEB

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive B picture for main
 * profile bitstream.  This parser starts after PTYPE was parsed but stops
 * before parsing of macroblock layer.
 * Table 21 of SMPTE 421M after processing up to PTYPE for B picture.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressiveBpicture(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if ((status = vc1_DecodeHuffmanPair(ctxt, VC1_BFRACTION_TBL,
                                        &picLayerHeader->BFRACTION_NUM, &picLayerHeader->BFRACTION_DEN)) !=
            VC1_STATUS_OK)
    {
        return status;
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
    }

    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    VC1_GET_BITS9(1, picLayerHeader->MVMODE);
    picLayerHeader->MVMODE = (picLayerHeader->MVMODE == 1) ?
                             VC1_MVMODE_1MV : VC1_MVMODE_HPELBI_1MV;

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_DIRECTMB)) != VC1_STATUS_OK)
    {
        return VC1_STATUS_OK;
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
        if (picLayerHeader->TTMBF)
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

