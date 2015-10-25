/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for simple and main profiles.
//
*/

#include "vc1parse.h"

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses the picture header for simple or
 * main profile down to macroblock layer.
 * Table 16 of SMPTE 421M after processing up to PTYPE for I picture.
 * Table 17 of SMPTE 421M after processing up to PTYPE for BI picture.
 * Table 19 of SMPTE 421M after processing up to PTYPE for P picture.
 * Table 21 of SMPTE 421M after processing up to PTYPE for B picture.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader(void* ctxt, vc1_Info *pInfo)
{
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;
    int32_t result;

    if (md->PROFILE != VC1_PROFILE_ADVANCED)
    {
        // As per spec, for main/simple profile, if the size of the coded picture is <= 1B,
        // it shall be treated as a skipped frame.
        // In content with skipped frames, the data is "00".
        // rcv to vc1 conversion process adds an additional byte (0x80) to the picture, hence
        // the data looks like "00 80"
        // Hence if data is <= 2B, we will consider it skipped (check for 16+1b, if it fails, the frame is skipped).
        result = viddec_pm_peek_bits(ctxt, &tempValue, 17);
        if (result == -1)
        {
            picLayerHeader->PTYPE = VC1_SKIPPED_FRAME;
            return status;
        }
    }

    if (md->FINTERPFLAG == 1)
    {
        VC1_GET_BITS9(1, tempValue); /* INTERPFRM. */
    }

    VC1_GET_BITS9(2, tempValue); /* FRMCNT. */

    if (md->RANGERED == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->RANGEREDFRM);
    }

    if (md->MAXBFRAMES == 0)
    {
        VC1_GET_BITS9(1, picLayerHeader->PTYPE);
        if (picLayerHeader->PTYPE == 0)
            picLayerHeader->PTYPE = VC1_I_FRAME;
        else
            picLayerHeader->PTYPE = VC1_P_FRAME;
    }
    else
    {
        VC1_GET_BITS9(1, picLayerHeader->PTYPE);
        if (picLayerHeader->PTYPE == 0)
        {
            VC1_GET_BITS9(1, picLayerHeader->PTYPE);
            if (picLayerHeader->PTYPE == 0) {
                picLayerHeader->PTYPE = VC1_B_FRAME; /* Or VC1_BI_FRAME. */
                /* if peek(7) = 0b1111111 then ptype = bi */
                VC1_PEEK_BITS( 7, tempValue );
                if ( tempValue == 0x7f )
                    picLayerHeader->PTYPE = VC1_BI_FRAME;
            } else
                picLayerHeader->PTYPE = VC1_I_FRAME;
        }
        else
            picLayerHeader->PTYPE = VC1_P_FRAME;
    }

    if (picLayerHeader->PTYPE == VC1_I_FRAME ||
            picLayerHeader->PTYPE == VC1_BI_FRAME)
    {
        status = vc1_ParsePictureHeader_ProgressiveIpicture(ctxt, pInfo);
    }
    else if (picLayerHeader->PTYPE == VC1_P_FRAME)
        status = vc1_ParsePictureHeader_ProgressivePpicture(ctxt, pInfo);
    else if (picLayerHeader->PTYPE == VC1_B_FRAME)
        status = vc1_ParsePictureHeader_ProgressiveBpicture(ctxt, pInfo);
    else
        status = VC1_STATUS_PARSE_ERROR;

    return status;
}
