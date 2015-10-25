/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for progressive P picture in advanced
//  profile bitstream.
//
*/

#include "vc1parse.h"
#include "viddec_fw_debug.h"
/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses progressive P picture for advanced
 * profile bitstream.
 * Table 20 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_ProgressivePpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint8_t bit_count;
    const uint8_t *table;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    /* MVRANGE. */
    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

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
        md->LUMSCALE2 = picLayerHeader->LUMSCALE;
        md->LUMSHIFT2 = picLayerHeader->LUMSHIFT;
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
                                         md->widthMB, md->heightMB, BPP_MVTYPEMB)) !=
                VC1_STATUS_OK)
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

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace P frame for advanced
 * profile bitstream.
 * Table 83 of SMPTE 421M after processing up to POSTPROC by
 * vc1_ParsePictureHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_InterlacePpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    /* MVRANGE. */
    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    /* DMVRANGE. */
    if ((status = vc1_DMVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK)
        return status;

    VC1_GET_BITS9(1, picLayerHeader->MV4SWITCH);

    VC1_GET_BITS9(1, picLayerHeader->INTCOMP);
    if (picLayerHeader->INTCOMP)
    {
        VC1_GET_BITS9(6, picLayerHeader->LUMSCALE);
        VC1_GET_BITS9(6, picLayerHeader->LUMSHIFT);
        md->LUMSCALE2 = picLayerHeader->LUMSCALE;
        md->LUMSHIFT2 = picLayerHeader->LUMSHIFT;
    }

    if ((status = vc1_DecodeBitplane(ctxt, pInfo,
                                     md->widthMB, md->heightMB, BPP_SKIPMB)) != VC1_STATUS_OK)
    {
        return status;
    }

    VC1_GET_BITS9(2, picLayerHeader->MBMODETAB);
    VC1_GET_BITS9(2, picLayerHeader->MVTAB); /* IMVTAB. */
    VC1_GET_BITS9(3, picLayerHeader->CBPTAB); /* ICBPTAB. */
    VC1_GET_BITS9(2, picLayerHeader->MV2BPTAB); /* 2MVBPTAB. */

    if (picLayerHeader->MV4SWITCH == 1)
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

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses interlace P field for advanced
 * profile bitstream.
 * Table 88 of SMPTE 421M after processing up to BFRACTION by
 * vc1_ParseFieldHeader_Adv() but stopping before processing of macroblock
 * layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseFieldHeader_InterlacePpicture_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint8_t bit_count;
    const uint8_t *table;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;


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

    VC1_GET_BITS9(1, picLayerHeader->NUMREF);

    if (picLayerHeader->NUMREF == 0)
    {
        VC1_GET_BITS9(1, picLayerHeader->REFFIELD);
    }

    if ((status = vc1_MVRangeDecode(ctxt, pInfo)) != VC1_STATUS_OK) {
        DEB("Error in vc1_MVRangeDecode \n");
        return status;
    }

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
    if (bit_count == 2 && picLayerHeader->MVMODE == 0) {
        VC1_GET_BITS9(1, picLayerHeader->MVMODE);

        if ( picLayerHeader->MVMODE == 1)
            bit_count ++;

        bit_count++;
    }
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

        VC1_GET_BITS9(1, md->INTCOMPFIELD);
        if (md->INTCOMPFIELD == 1)
            md->INTCOMPFIELD = VC1_INTCOMP_BOTH_FIELD;
        else
        {
            VC1_GET_BITS9(1, md->INTCOMPFIELD);
            if (md->INTCOMPFIELD == 1)
                md->INTCOMPFIELD = VC1_INTCOMP_BOTTOM_FIELD;
            else
                md->INTCOMPFIELD = VC1_INTCOMP_TOP_FIELD;
        }
        VC1_GET_BITS9(6, picLayerHeader->LUMSCALE); /* LUMSCALE1. */
        VC1_GET_BITS9(6, picLayerHeader->LUMSHIFT); /* LUMSHIFT1. */
        if ( md->INTCOMPFIELD == VC1_INTCOMP_BOTTOM_FIELD ) {
            md->LUMSCALE2 = picLayerHeader->LUMSCALE;
            md->LUMSHIFT2 = picLayerHeader->LUMSHIFT;
        }
        if (md->INTCOMPFIELD == VC1_INTCOMP_BOTH_FIELD)
        {
            VC1_GET_BITS9(6, md->LUMSCALE2);
            VC1_GET_BITS9(6, md->LUMSHIFT2);
        }
    }
    else
#ifdef VBP
        picLayerHeader->MVMODE2 = 0;
#else
        picLayerHeader->MVMODE2 = picLayerHeader->MVMODE;
#endif

    VC1_GET_BITS9(3, picLayerHeader->MBMODETAB);

    if (picLayerHeader->NUMREF)
    {
        VC1_GET_BITS9(3, picLayerHeader->MVTAB); /* IMVTAB. */
    }
    else
    {
        VC1_GET_BITS9(2, picLayerHeader->MVTAB); /* IMVTAB. */
    }

    VC1_GET_BITS9(3, picLayerHeader->CBPTAB); /* ICBPTAB. */

#ifdef VBP
    if (picLayerHeader->MVMODE == VC1_MVMODE_MIXED_MV)
#else
    if (picLayerHeader->MVMODE2 == VC1_MVMODE_MIXED_MV)
#endif
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
    picLayerHeader->TRANSACFRM2 = 0;

    VC1_GET_BITS9(1, picLayerHeader->TRANSDCTAB);

    /* Skip parsing of macroblock layer. */

    return status;
}
