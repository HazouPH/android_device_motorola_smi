/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 picture layer for advanced profile.
//
*/

#include "vc1parse.h"
#include "viddec_fw_debug.h"

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses the picture header for advanced
 * profile down to POSTPROC syntax element.
 * Table 18 of SMPTE 421M for progressive I or BI picture.
 * Table 20 of SMPTE 421M for progressive P picture.
 * Table 22 of SMPTE 421M for progressive B picture.
 * Table 23 of SMPTE 421M for skipped picture.
 * Table 82 of SMPTE 421M for interlace I or BI frame.
 * Table 83 of SMPTE 421M for interlace P frame.
 * Table 84 of SMPTE 421M for interlace B frame.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureHeader_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint32_t i = 0;
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    uint32_t number_of_pan_scan_window;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if (md->INTERLACE == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->FCM);
        if (picLayerHeader->FCM)
        {
            VC1_GET_BITS9(1, picLayerHeader->FCM);
            if (picLayerHeader->FCM)
            {
                picLayerHeader->FCM = VC1_FCM_FIELD_INTERLACE;
                return VC1_STATUS_PARSE_ERROR;
            }
            else
                picLayerHeader->FCM = VC1_FCM_FRAME_INTERLACE;
        }
        else
            picLayerHeader->FCM = VC1_FCM_PROGRESSIVE;
    }
    else
        picLayerHeader->FCM = VC1_FCM_PROGRESSIVE;


    VC1_GET_BITS9(1, picLayerHeader->PTYPE);
    if (picLayerHeader->PTYPE)
    {
        VC1_GET_BITS9(1, picLayerHeader->PTYPE);
        if (picLayerHeader->PTYPE)
        {
            VC1_GET_BITS9(1, picLayerHeader->PTYPE);
            if (picLayerHeader->PTYPE)
            {
                VC1_GET_BITS9(1, picLayerHeader->PTYPE);
                if (picLayerHeader->PTYPE)
                    picLayerHeader->PTYPE = VC1_SKIPPED_FRAME;
                else
                    picLayerHeader->PTYPE = VC1_BI_FRAME;
            }
            else
                picLayerHeader->PTYPE = VC1_I_FRAME;
        }
        else
            picLayerHeader->PTYPE = VC1_B_FRAME;
    }
    else
        picLayerHeader->PTYPE = VC1_P_FRAME;

    if (picLayerHeader->PTYPE != VC1_SKIPPED_FRAME)
    {
        if (md->TFCNTRFLAG)
        {
            VC1_GET_BITS9(8, picLayerHeader->TFCNTR); /* TFCNTR. */
        }
    }

    if (md->PULLDOWN)
    {
        if ((md->INTERLACE == 0) || (md->PSF == 1))
        {
            VC1_GET_BITS9(2, picLayerHeader->RPTFRM);
        }
        else
        {
            VC1_GET_BITS9(1, picLayerHeader->TFF);
            VC1_GET_BITS9(1, picLayerHeader->RFF);
        }
    }

    if (md->PANSCAN_FLAG == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->PS_PRESENT); /* PS_PRESENT. */
        if (picLayerHeader->PS_PRESENT == 1)
        {
            if ((md->INTERLACE == 1) &&
                    (md->PSF == 0))
            {
                if (md->PULLDOWN == 1)
                    number_of_pan_scan_window = 2 + picLayerHeader->RFF;
                else
                    number_of_pan_scan_window = 2;
            }
            else
            {
                if (md->PULLDOWN == 1)
                    number_of_pan_scan_window = 1 + picLayerHeader->RPTFRM;
                else
                    number_of_pan_scan_window = 1;
            }
            picLayerHeader->number_of_pan_scan_window = number_of_pan_scan_window;

            for (i = 0; i < number_of_pan_scan_window; i++)
            {
                VC1_GET_BITS(18, picLayerHeader->PAN_SCAN_WINDOW[i].hoffset); /* PS_HOFFSET. */
                VC1_GET_BITS(18, picLayerHeader->PAN_SCAN_WINDOW[i].voffset); /* PS_VOFFSET. */
                VC1_GET_BITS(14, picLayerHeader->PAN_SCAN_WINDOW[i].width); /* PS_WIDTH. */
                VC1_GET_BITS(14, picLayerHeader->PAN_SCAN_WINDOW[i].height); /* PS_HEIGHT. */
            }
        }
    }

    if (picLayerHeader->PTYPE != VC1_SKIPPED_FRAME)
    {
        VC1_GET_BITS9(1, picLayerHeader->RNDCTRL);
        md->RNDCTRL =  picLayerHeader->RNDCTRL;

        if ((md->INTERLACE == 1) ||
                (picLayerHeader->FCM != VC1_FCM_PROGRESSIVE))
        {
            VC1_GET_BITS9(1, picLayerHeader->UVSAMP);
        }

        if ((md->FINTERPFLAG == 1) &&
                (picLayerHeader->FCM == VC1_FCM_PROGRESSIVE))
        {
            VC1_GET_BITS9(1, tempValue); /* INTERPFRM. */
        }

        if ((picLayerHeader->PTYPE == VC1_B_FRAME) &&
                (picLayerHeader->FCM == VC1_FCM_PROGRESSIVE))
        {
            if ((status = vc1_DecodeHuffmanPair(ctxt, VC1_BFRACTION_TBL,
                                                &picLayerHeader->BFRACTION_NUM, &picLayerHeader->BFRACTION_DEN))
                    != VC1_STATUS_OK)
            {
                return status;
            }
        }

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
    }

    return vc1_ParsePictureFieldHeader_Adv(ctxt, pInfo);
}

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses the picture header for advanced
 * profile down to BFRACTION syntax element.
 * Table 85 of SMPTE 421M.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseFieldHeader_Adv(void* ctxt, vc1_Info *pInfo)
{
    uint32_t i = 0;
    vc1_Status status = VC1_STATUS_OK;
    uint32_t number_of_pan_scan_window;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    VC1_GET_BITS9(1, picLayerHeader->FCM);
    if (picLayerHeader->FCM)
    {
        VC1_GET_BITS9(1, picLayerHeader->FCM);
        if (picLayerHeader->FCM)
            picLayerHeader->FCM = VC1_FCM_FIELD_INTERLACE;
        else
            picLayerHeader->FCM = VC1_FCM_FRAME_INTERLACE;
    }
    else
        picLayerHeader->FCM = VC1_FCM_PROGRESSIVE;
    if (picLayerHeader->FCM != VC1_FCM_FIELD_INTERLACE)
        return VC1_STATUS_PARSE_ERROR;

    VC1_GET_BITS9(3, picLayerHeader->FPTYPE);
    if (picLayerHeader->FPTYPE == 0)
    {
        picLayerHeader->PTypeField1 = VC1_I_FRAME;
        picLayerHeader->PTypeField2 = VC1_I_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 1)
    {
        picLayerHeader->PTypeField1 = VC1_I_FRAME;
        picLayerHeader->PTypeField2 = VC1_P_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 2)
    {
        picLayerHeader->PTypeField1 = VC1_P_FRAME;
        picLayerHeader->PTypeField2 = VC1_I_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 3)
    {
        picLayerHeader->PTypeField1 = VC1_P_FRAME;
        picLayerHeader->PTypeField2 = VC1_P_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 4)
    {
        picLayerHeader->PTypeField1 = VC1_B_FRAME;
        picLayerHeader->PTypeField2 = VC1_B_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 5)
    {
        picLayerHeader->PTypeField1 = VC1_B_FRAME;
        picLayerHeader->PTypeField2 = VC1_BI_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 6)
    {
        picLayerHeader->PTypeField1 = VC1_BI_FRAME;
        picLayerHeader->PTypeField2 = VC1_B_FRAME;
    }
    else if (picLayerHeader->FPTYPE == 7)
    {
        picLayerHeader->PTypeField1 = VC1_BI_FRAME;
        picLayerHeader->PTypeField2 = VC1_BI_FRAME;
    }

    if (md->TFCNTRFLAG)
    {
        VC1_GET_BITS9(8, picLayerHeader->TFCNTR);
    }

    if (md->PULLDOWN == 1)
    {
        if (md->PSF == 1)
        {
            VC1_GET_BITS9(2, picLayerHeader->RPTFRM);
        }
        else
        {
            VC1_GET_BITS9(1, picLayerHeader->TFF);
            VC1_GET_BITS9(1, picLayerHeader->RFF);
        }
    } else
        picLayerHeader->TFF = 1;

    if (md->PANSCAN_FLAG == 1)
    {
        VC1_GET_BITS9(1, picLayerHeader->PS_PRESENT);
        if (picLayerHeader->PS_PRESENT)
        {
            if (md->PULLDOWN)
                number_of_pan_scan_window = 2 + picLayerHeader->RFF;
            else
                number_of_pan_scan_window = 2;
            picLayerHeader->number_of_pan_scan_window =number_of_pan_scan_window;

            for (i = 0; i < number_of_pan_scan_window; i++)
            {
                VC1_GET_BITS(18, picLayerHeader->PAN_SCAN_WINDOW[i].hoffset); /* PS_HOFFSET. */
                VC1_GET_BITS(18, picLayerHeader->PAN_SCAN_WINDOW[i].voffset); /* PS_VOFFSET. */
                VC1_GET_BITS(14, picLayerHeader->PAN_SCAN_WINDOW[i].width); /* PS_WIDTH. */
                VC1_GET_BITS(14, picLayerHeader->PAN_SCAN_WINDOW[i].height); /* PS_HEIGHT. */
            }
        }
    }
    VC1_GET_BITS9(1, md->RNDCTRL);

#ifdef VBP
    picLayerHeader->RNDCTRL = md->RNDCTRL;
#endif

    VC1_GET_BITS9(1, picLayerHeader->UVSAMP);

    if ((md->REFDIST_FLAG == 1) && (picLayerHeader->FPTYPE <= 3))
    {
        int32_t tmp;
        if ((status = vc1_DecodeHuffmanOne(ctxt, &tmp,
                                           VC1_REFDIST_TBL)) != VC1_STATUS_OK)
        {
            return status;
        }
        md->REFDIST = tmp;
    } else if (md->REFDIST_FLAG == 0) {
        md->REFDIST = 0;
    }

    if ((picLayerHeader->FPTYPE >= 4) && (picLayerHeader->FPTYPE <= 7))
    {
        if ((status = vc1_DecodeHuffmanPair(ctxt, VC1_BFRACTION_TBL,
                                            &picLayerHeader->BFRACTION_NUM, &picLayerHeader->BFRACTION_DEN)) !=
                VC1_STATUS_OK)
        {
            return status;
        }
    }

    if (picLayerHeader->CurrField == 0)
    {
        picLayerHeader->PTYPE = picLayerHeader->PTypeField1;
        picLayerHeader->BottomField = (uint8_t) (1 - picLayerHeader->TFF);
    }
    else
    {
        picLayerHeader->BottomField = (uint8_t) (picLayerHeader->TFF);
        picLayerHeader->PTYPE = picLayerHeader->PTypeField2;
    }

    return vc1_ParsePictureFieldHeader_Adv(ctxt, pInfo);
}

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function calls the appropriate function to further
 * parse the picture header for advanced profile down to macroblock layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureFieldHeader_Adv(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_PARSE_ERROR;

    if (pInfo->picLayerHeader.FCM == VC1_FCM_PROGRESSIVE)
    {
        if ((pInfo->picLayerHeader.PTYPE == VC1_I_FRAME) ||
                (pInfo->picLayerHeader.PTYPE == VC1_BI_FRAME))
        {
            status = vc1_ParsePictureHeader_ProgressiveIpicture_Adv(ctxt, pInfo);
        }
        else if (pInfo->picLayerHeader.PTYPE == VC1_P_FRAME)
            status = vc1_ParsePictureHeader_ProgressivePpicture_Adv(ctxt, pInfo);
        else if (pInfo->picLayerHeader.PTYPE == VC1_B_FRAME)
            status = vc1_ParsePictureHeader_ProgressiveBpicture_Adv(ctxt, pInfo);
        else if (pInfo->picLayerHeader.PTYPE == VC1_SKIPPED_FRAME)
            status = VC1_STATUS_OK;
    }
    else if (pInfo->picLayerHeader.FCM == VC1_FCM_FRAME_INTERLACE)
    {
        if ((pInfo->picLayerHeader.PTYPE == VC1_I_FRAME) ||
                (pInfo->picLayerHeader.PTYPE == VC1_BI_FRAME))
        {
            status = vc1_ParsePictureHeader_InterlaceIpicture_Adv(ctxt, pInfo);
        }
        else if (pInfo->picLayerHeader.PTYPE == VC1_P_FRAME)
            status = vc1_ParsePictureHeader_InterlacePpicture_Adv(ctxt, pInfo);
        else if (pInfo->picLayerHeader.PTYPE == VC1_B_FRAME)
            status = vc1_ParsePictureHeader_InterlaceBpicture_Adv(ctxt, pInfo);
        else if (pInfo->picLayerHeader.PTYPE == VC1_SKIPPED_FRAME)
            status = VC1_STATUS_OK;
    }
    else if (pInfo->picLayerHeader.FCM == VC1_FCM_FIELD_INTERLACE)
    {
        int ptype;
        if ( pInfo->picLayerHeader.CurrField == 0)
            ptype = pInfo->picLayerHeader.PTypeField1;
        else
            ptype = pInfo->picLayerHeader.PTypeField2;

        if ((ptype == VC1_I_FRAME) ||
                (ptype == VC1_BI_FRAME))
        {
            status = vc1_ParseFieldHeader_InterlaceIpicture_Adv(ctxt, pInfo);
        }
        else if (ptype == VC1_P_FRAME)
            status = vc1_ParseFieldHeader_InterlacePpicture_Adv(ctxt, pInfo);
        else if (ptype == VC1_B_FRAME)
            status = vc1_ParseFieldHeader_InterlaceBpicture_Adv(ctxt, pInfo);
        else if (ptype == VC1_SKIPPED_FRAME)
            status = VC1_STATUS_OK;
    }

    return status;
}
