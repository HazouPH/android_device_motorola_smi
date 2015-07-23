/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive B picture in advanced
//  profile bitstream.
//
*/

#include "vc1parse.h"
#include "viddec_fw_debug.h"    // For DEB

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive B picture for advanced
 * profile bitstream.
 * Table 22 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressiveBpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    VC1_GET_BITS9(1, picLayerHeader->MVMODE);
    picLayerHeader->MVMODE = (picLayerHeader->MVMODE == 1) ?
                             VC1_MVMODE_1MV : VC1_MVMODE_HPELBI_1MV;

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_DIRECTMB)) != VC1_STATUS_OK)
    {
        return status;
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

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace B frame for advanced
 * profile bitstream.
 * Table 84 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_InterlaceBpicture_Adv(void* ctxt, vc1_Info *pInfo)
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

    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if ((status = vc1_DMVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    VC1_GET_BITS9(1, picLayerHeader->INTCOMP);

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_DIRECTMB)) != VC1_STATUS_OK)
    {
        return status;
    }

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_SKIPMB)) != VC1_STATUS_OK)
    {
        return status;
    }

    // EPC picLayerHeader->MVMODE = VC1_MVMODE_1MV;
    VC1_GET_BITS9(2, picLayerHeader->MBMODETAB);
    VC1_GET_BITS9(2, picLayerHeader->MVTAB); /* IMVTAB. */
    VC1_GET_BITS9(3, picLayerHeader->CBPTAB); /* ICBPTAB. */
    VC1_GET_BITS9(2, picLayerHeader->MV2BPTAB); /* 2MVBPTAB. */
    VC1_GET_BITS9(2, picLayerHeader->MV4BPTAB); /* 4MVBPTAB. */

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

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace B field for advanced
 * profile bitstream.
 * Table 89 of SMPTE 421M after processing up to BFRACTION by
 * vc1_ParseFieldHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseFieldHeader_InterlaceBpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint8_t bit_count;
    const uint8_t *table;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader* picLayerHeader = &pInfo->picLayerHeader;

    VC1_GET_BITS9(5, picLayerHeader->PQINDEX);

    if ((status = vc1_CalculatePQuant(pInfo)) != VC1_STATUS_OK)
        return status;

    if (picLayerHeader->PQINDEX <= 8)
    {
        VC1_GET_BITS9(1, picLayerHeader->HALFQP);
    }
    else
        picLayerHeader->HALFQP = 0;

    if (md->QUANTIZER == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->PQUANTIZER);
        picLayerHeader->UniformQuant = picLayerHeader->PQUANTIZER;
    }

    if (md->POSTPROCFLAG == 1)
    {
        VC1_GET_BITS9(2, picLayerHeader->POSTPROC);
    }

    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if ((status = vc1_DMVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    if (picLayerHeader->PQUANT > 12)
        table = VC1_MVMODE_LOW_TBL;
    else
        table = VC1_MVMODE_HIGH_TBL;

    bit_count = 0;
    VC1_GET_BITS9(1, picLayerHeader->MVMODE);
    while ((picLayerHeader->MVMODE == 0) && (bit_count < 2))
    {
        VC1_GET_BITS9(1, picLayerHeader->MVMODE);
        bit_count++;
    }
    if ((bit_count == 2) && (picLayerHeader->MVMODE == 0))
        bit_count++;
    picLayerHeader->MVMODE = table[bit_count];

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, (md->heightMB+1)/2, BPP_FORWARDMB)) !=
            VC1_STATUS_OK)
    {
        return status;
    }

    VC1_GET_BITS9(3, picLayerHeader->MBMODETAB);
    VC1_GET_BITS9(3, picLayerHeader->MVTAB); /* IMVTAB. */
    VC1_GET_BITS9(3, picLayerHeader->CBPTAB); /* ICBPTAB. */

    if (picLayerHeader->MVMODE == VC1_MVMODE_MIXED_MV)
    {
        VC1_GET_BITS9(2, picLayerHeader->MV4BPTAB); /* 4MVBPTAB. */
    }

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
