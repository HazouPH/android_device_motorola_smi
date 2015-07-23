/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 syntax elements MVRANGE and DMVRANGE.
//
*/

#include "vc1parse.h"

/*------------------------------------------------------------------------------
 * Parse syntax element MVRANGE, which exists for main and advanced profiles.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_MVRangeDecode(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if (md->EXTENDED_MV == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->MVRANGE);
        if (picLayerHeader->MVRANGE)
        {
            VC1_GET_BITS9(1, picLayerHeader->MVRANGE);
            if (picLayerHeader->MVRANGE)
            {
                VC1_GET_BITS9(1, picLayerHeader->MVRANGE);
                picLayerHeader->MVRANGE += 1;
            }
            picLayerHeader->MVRANGE += 1;
        }
    }
    else
        picLayerHeader->MVRANGE = 0;

    return status;
}

/*------------------------------------------------------------------------------
 * Parse syntax element DMVRANGE.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_DMVRangeDecode(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if (md->EXTENDED_DMV == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->DMVRANGE);
        if (picLayerHeader->DMVRANGE == 0)
            picLayerHeader->DMVRANGE = VC1_DMVRANGE_NONE;
        else
        {
            VC1_GET_BITS9(1, picLayerHeader->DMVRANGE);
            if (picLayerHeader->DMVRANGE == 0)
                picLayerHeader->DMVRANGE = VC1_DMVRANGE_HORIZONTAL_RANGE;
            else
            {
                VC1_GET_BITS9(1, picLayerHeader->DMVRANGE);
                if (picLayerHeader->DMVRANGE == 0)
                    picLayerHeader->DMVRANGE = VC1_DMVRANGE_VERTICAL_RANGE;
                else
                {
                    picLayerHeader->DMVRANGE =
                        VC1_DMVRANGE_HORIZONTAL_VERTICAL_RANGE;
                }
            }
        }
    }

    return status;
}
