/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 syntax elements VOPDQUANT and DQUANT.
//
*/

#include "vc1parse.h"

#define VC1_UNDEF_PQUANT 0

static const uint8_t MapPQIndToQuant_Impl[] =
{
    VC1_UNDEF_PQUANT,
    1,  2,  3,  4,  5, 6,   7,  8,
    6,  7,  8,  9, 10, 11, 12, 13,
    14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 27, 29, 31
};

/*------------------------------------------------------------------------------
 * Parse syntax element VOPDQuant as defined in Table 24 of SMPTE 421M.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_VOPDQuant(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if (md->DQUANT == 0)
        return status;

    if (md->DQUANT == 2)
    {
        VC1_GET_BITS9(3, picLayerHeader->PQDIFF);
        if (picLayerHeader->PQDIFF == 7)
        {
            VC1_GET_BITS9(5, picLayerHeader->ABSPQ);
        }
    }
    else
    {
        VC1_GET_BITS9(1, picLayerHeader->DQUANTFRM);
        if (picLayerHeader->DQUANTFRM == 1)
        {
            VC1_GET_BITS9(2, picLayerHeader->DQPROFILE);
            if (picLayerHeader->DQPROFILE == VC1_DQPROFILE_SNGLEDGES)
            {
                VC1_GET_BITS9(2, picLayerHeader->DQSBEDGE);
            }
            else if (picLayerHeader->DQPROFILE == VC1_DQPROFILE_DBLEDGES)
            {
#ifdef VBP
                VC1_GET_BITS9(2, picLayerHeader->DQDBEDGE);
#else
                VC1_GET_BITS9(2, picLayerHeader->DQSBEDGE); /* DQDBEDGE. */
#endif
            }
            else if (picLayerHeader->DQPROFILE == VC1_DQPROFILE_ALLMBLKS)
            {
                VC1_GET_BITS9(1, picLayerHeader->DQBILEVEL);
            }
            if (! (picLayerHeader->DQPROFILE == VC1_DQPROFILE_ALLMBLKS &&
                    picLayerHeader->DQBILEVEL == 0))
            {
                VC1_GET_BITS9(3, picLayerHeader->PQDIFF);
                if (picLayerHeader->PQDIFF == 7)
                {
                    VC1_GET_BITS9(5, picLayerHeader->ABSPQ);
                }
            }
        }
    }
#ifdef VBP
    if ((picLayerHeader->DQUANTFRM == 1 && md->DQUANT == 1) || (md->DQUANT == 2))
    {
        if (picLayerHeader->PQDIFF == 7)
        {
            picLayerHeader->ALTPQUANT = picLayerHeader->ABSPQ;
        }
        else
        {
            picLayerHeader->ALTPQUANT = picLayerHeader->PQUANT + picLayerHeader->PQDIFF + 1;
        }
    }
#endif
    return status;
}

/*------------------------------------------------------------------------------
 * Compute value for PQUANT syntax element that does not exist in bitstreams for
 * progressive I and BI pictures.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_CalculatePQuant(vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    picLayerHeader->PQUANT = picLayerHeader->PQINDEX;
    picLayerHeader->UniformQuant = VC1_QUANTIZER_UNIFORM;

    if (md->QUANTIZER == 0)
    {
        if (picLayerHeader->PQINDEX < 9)
            picLayerHeader->UniformQuant = VC1_QUANTIZER_UNIFORM;
        else
        {
            picLayerHeader->UniformQuant = VC1_QUANTIZER_NONUNIFORM;
            picLayerHeader->PQUANT =
                MapPQIndToQuant_Impl[picLayerHeader->PQINDEX];
        }
    }
    else
    {
        if (md->QUANTIZER == 2)
            picLayerHeader->UniformQuant = VC1_QUANTIZER_NONUNIFORM;
    }

    return status;
}
