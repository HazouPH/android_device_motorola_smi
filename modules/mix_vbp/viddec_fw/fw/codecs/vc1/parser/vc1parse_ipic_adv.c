/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive I or BI picture in
//  advanced profile bitstream.
//
*/

#include "vc1parse.h"
#include "viddec_fw_debug.h"
/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive I or BI picture for
 * advanced profile bitstream.
 * Table 18 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressiveIpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_ACPRED)) != VC1_STATUS_OK)
    {
        return status;
    }

    if ((md->OVERLAP == 1) && (picLayerHeader->PQUANT <= 8))
    {
        VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
        if (picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
            if (! picLayerHeader->CONDOVER)
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_ALL;
            else
            {
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_SOME;
                if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                                 md->widthMB,
                                                 md->heightMB, BPP_OVERFLAGS)) != VC1_STATUS_OK)
                {
                    return status;
                }
            }
        }
        else
            picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_NONE;
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

    status = vc1_VOPDQuant(ctxt, pInfo);

    /* Skip parsing of macroblock layer. */

    return status;
}

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace I or BI frame for
 * advanced profile bitstream.
 * Table 82 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_InterlaceIpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_FIELDTX)) != VC1_STATUS_OK)
    {
        return status;
    }

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_ACPRED)) != VC1_STATUS_OK)
    {
        return status;
    }

    if ((md->OVERLAP == 1) && (picLayerHeader->PQUANT <= 8))
    {
        VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
        if (picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
            if (! picLayerHeader->CONDOVER)
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_ALL;
            else
            {
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_SOME;
                if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                                 md->widthMB,
                                                 md->heightMB, BPP_OVERFLAGS)) != VC1_STATUS_OK)
                {
                    return status;
                }
            }
        }
        else
            picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_NONE;
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

    status = vc1_VOPDQuant(ctxt, pInfo);

    /* Skip parsing of macroblock layer. */

    return status;
}

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace I or BI field for
 * advanced profile bitstream.
 * Table 87 of SMPTE 421M after processing up to BFRACTION by
 * vc1_ParseFieldHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseFieldHeader_InterlaceIpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    // Reset MVMODE when the second field is an I picture
    // to avoid carrying forward the mvmode values from previous field
    // especially the intensity compensation value
    picLayerHeader->MVMODE = 0;

    VC1_GET_BITS9(5, picLayerHeader->PQINDEX);
    if ((status = vc1_CalculatePQuant(pInfo)) != VC1_STATUS_OK) {
        DEB("Error parsing I field \n");
        return status;
    }

    if (picLayerHeader->PQINDEX <= 8)
    {
        VC1_GET_BITS9(1, picLayerHeader->HALFQP);
    }
    else
        picLayerHeader->HALFQP = 0;

    if (md->QUANTIZER == 1) {
        VC1_GET_BITS9(1, picLayerHeader->PQUANTIZER);
        picLayerHeader->UniformQuant = picLayerHeader->PQUANTIZER;
    }

    if (md->POSTPROCFLAG == 1)
        VC1_GET_BITS9(2, tempValue); /* POSTPROC. */

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, (md->heightMB+1)/2, BPP_ACPRED)) !=
            VC1_STATUS_OK)
    {
        DEB("Error parsing I field \n");
        return status;
    }

    if ((md->OVERLAP == 1) && (picLayerHeader->PQUANT <= 8))
    {
        VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
        if (picLayerHeader->CONDOVER)
        {
            VC1_GET_BITS9(1, picLayerHeader->CONDOVER);
            if (! picLayerHeader->CONDOVER)
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_ALL;
            else
            {
                picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_SOME;

                if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                                 md->widthMB,
                                                 (md->heightMB+1)/2, BPP_OVERFLAGS)) !=
                        VC1_STATUS_OK)
                {
                    DEB("Error parsing I field \n");
                    return status;
                }
            }
        }
        else
            picLayerHeader->CONDOVER = VC1_CONDOVER_FLAG_NONE;
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

    status = vc1_VOPDQuant(ctxt, pInfo);
    if (status != VC1_STATUS_OK) {
        DEB("Error parsing I field \n");
        return status;
    }

    /* Skip parsing of macroblock layer. */

    return status;
}
