/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 bitstream layers down to but not including
//  macroblock layer.
//
*/

#include "viddec_fw_debug.h"
#include "vc1parse.h"

#define VC1_PIXEL_IN_LUMA 16

/*------------------------------------------------------------------------------
 * Parse modified rcv file, start codes are inserted using rcv2vc1.c.
 * source is in
 * http://svn.jf.intel.com/svn/DHG_Src/CESWE_Src/DEV/trunk/sv/mfd/tools/utils.
 * Assumme rcv file width < 90,112 pixel to differenciate from real VC1
 * advanced profile header.
 * Original rcv description is in annex L
 * Table 263 of SMPTE 421M.
 */
vc1_Status vc1_ParseRCVSequenceLayer (void* ctxt, vc1_Info *pInfo)
{
    uint32_t result;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_RcvSequenceHeader rcv;

    memset(&rcv, 0, sizeof(vc1_RcvSequenceHeader));

    result = viddec_pm_get_bits(ctxt, &rcv.struct_a_rcv, 32);
    md->width = rcv.struct_a.HORIZ_SIZE;
    md->height = rcv.struct_a.VERT_SIZE;
#ifdef VBP
    //The HRD rate and HRD buffer size may be encoded according to a 64 bit sequence header data structure B
    //if there is no data strcuture B metadata contained in the bitstream, we will not be able to get the
    //bitrate data, hence we set it to 0 for now
    md->HRD_NUM_LEAKY_BUCKETS = 0;
    md->hrd_initial_state.sLeakyBucket[0].HRD_RATE = 0;
#endif

    result = viddec_pm_get_bits(ctxt, &rcv.struct_c_rcv, 32);
    md->PROFILE = rcv.struct_c.PROFILE >> 2;
    md->LOOPFILTER = rcv.struct_c.LOOPFILTER;
    md->MULTIRES = rcv.struct_c.MULTIRES;
    md->FASTUVMC = rcv.struct_c.FASTUVMC;
    md->EXTENDED_MV = rcv.struct_c.EXTENDED_MV;
    md->DQUANT = rcv.struct_c.DQUANT;
    md->VSTRANSFORM = rcv.struct_c.VSTRANSFORM;
    md->OVERLAP = rcv.struct_c.OVERLAP;
    md->RANGERED = rcv.struct_c.RANGERED;
    md->MAXBFRAMES = rcv.struct_c.MAXBFRAMES;
    md->QUANTIZER = rcv.struct_c.QUANTIZER;
    md->FINTERPFLAG = rcv.struct_c.FINTERPFLAG;
#ifdef VBP
    md->SYNCMARKER = rcv.struct_c.SYNCMARKER;
#endif

    if ((md->PROFILE == VC1_PROFILE_SIMPLE) ||
            (md->MULTIRES && md->PROFILE == VC1_PROFILE_MAIN))
    {
        md->DQUANT = 0;
    }
    // TODO: NEED TO CHECK RESERVED BITS ARE 0

    md->widthMB = (md->width + 15 )  / VC1_PIXEL_IN_LUMA;
    md->heightMB = (md->height + 15) / VC1_PIXEL_IN_LUMA;

    DEB("rcv: beforemod: res: %dx%d\n", md->width, md->height);

    /* WL takes resolution in unit of 2 pel - sec. 6.2.13.1 */
    md->width = md->width/2 -1;
    md->height = md->height/2 -1;

    DEB("rcv: res: %dx%d\n", md->width, md->height);

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi;

        wi.vwi_type = VIDDEC_WORKLOAD_VC1_SEQ_HDR_STRUCT_A_C;

        wi.vc1_sh_struct_a_c.size = 0;
        wi.vc1_sh_struct_a_c.flags = 0;
        wi.vc1_sh_struct_a_c.pad = 0;

        viddec_fw_vc1_set_rcv_horiz_size(&wi.vc1_sh_struct_a_c, rcv.struct_a.HORIZ_SIZE);
        viddec_fw_vc1_set_rcv_vert_size(&wi.vc1_sh_struct_a_c, rcv.struct_a.VERT_SIZE);

        viddec_fw_vc1_set_rcv_bitrtq_postproc(&wi.vc1_sh_struct_a_c, rcv.struct_c.BITRTQ_POSTPROC);
        viddec_fw_vc1_set_rcv_frmrtq_postproc(&wi.vc1_sh_struct_a_c, rcv.struct_c.FRMRTQ_POSTPROC);
        viddec_fw_vc1_set_rcv_profile(&wi.vc1_sh_struct_a_c, rcv.struct_c.PROFILE);
        viddec_fw_vc1_set_rcv_level(&wi.vc1_sh_struct_a_c, 0);
        viddec_fw_vc1_set_rcv_cbr(&wi.vc1_sh_struct_a_c, 0);
        viddec_fw_vc1_set_rcv_rangered(&wi.vc1_sh_struct_a_c, rcv.struct_c.RANGERED);
        viddec_fw_vc1_set_rcv_maxbframes(&wi.vc1_sh_struct_a_c, rcv.struct_c.MAXBFRAMES);
        viddec_fw_vc1_set_rcv_finterpflag(&wi.vc1_sh_struct_a_c, rcv.struct_c.FINTERPFLAG);

        result = viddec_pm_append_workitem(ctxt, &wi, false);
    }

    return status;
}

/*------------------------------------------------------------------------------
 * Parse sequence layer.  This function is only applicable to advanced profile
 * as simple and main profiles use other mechanisms to communicate these
 * metadata.
 * Table 3 of SMPTE 421M.
 * Table 13 of SMPTE 421M for HRD_PARAM().
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseSequenceLayer(void* ctxt, vc1_Info *pInfo)
{
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_SequenceLayerHeader sh;
    uint32_t result;

    memset(&sh, 0, sizeof(vc1_SequenceLayerHeader));

    // PARSE SEQUENCE HEADER
    result = viddec_pm_get_bits(ctxt, &sh.flags, 15);
    if (result == 1)
    {
        md->PROFILE = sh.seq_flags.PROFILE;
        md->LEVEL = sh.seq_flags.LEVEL;
        md->CHROMAFORMAT = sh.seq_flags.COLORDIFF_FORMAT;
        md->FRMRTQ = sh.seq_flags.FRMRTQ_POSTPROC;
        md->BITRTQ = sh.seq_flags.BITRTQ_POSTPROC;
    }

    result = viddec_pm_get_bits(ctxt, &sh.max_size, 32);
    if (result == 1)
    {
        md->POSTPROCFLAG = sh.seq_max_size.POSTPROCFLAG;
        md->width = sh.seq_max_size.MAX_CODED_WIDTH;
        md->height = sh.seq_max_size.MAX_CODED_HEIGHT;
        md->PULLDOWN = sh.seq_max_size.PULLDOWN;
        md->INTERLACE = sh.seq_max_size.INTERLACE;
        md->TFCNTRFLAG = sh.seq_max_size.TFCNTRFLAG;
        md->FINTERPFLAG = sh.seq_max_size.FINTERPFLAG;
        md->PSF = sh.seq_max_size.PSF;
    }

    if (sh.seq_max_size.DISPLAY_EXT == 1)
    {
        result = viddec_pm_get_bits(ctxt, &sh.disp_size, 29);
        if (result == 1)
        {
            if (sh.seq_disp_size.ASPECT_RATIO_FLAG == 1)
            {
                result = viddec_pm_get_bits(ctxt, &tempValue, 4);
                sh.ASPECT_RATIO = tempValue;
                if (sh.ASPECT_RATIO == 15)
                {
                    result = viddec_pm_get_bits(ctxt, &sh.aspect_size, 16);
                }
#ifdef VBP
                md->ASPECT_RATIO_FLAG = 1;
                md->ASPECT_RATIO = sh.ASPECT_RATIO;
                md->ASPECT_HORIZ_SIZE = sh.seq_aspect_size.ASPECT_HORIZ_SIZE;
                md->ASPECT_VERT_SIZE = sh.seq_aspect_size.ASPECT_VERT_SIZE;
#endif
            }

            result = viddec_pm_get_bits(ctxt, &tempValue, 1);
            sh.FRAMERATE_FLAG = tempValue;
            if (sh.FRAMERATE_FLAG == 1)
            {
                result = viddec_pm_get_bits(ctxt, &tempValue, 1);
                sh.FRAMERATEIND = tempValue;
                if (sh.FRAMERATEIND == 0)
                {
                    result = viddec_pm_get_bits(ctxt, &sh.framerate_fraction, 12);
                }
                else
                {
                    result = viddec_pm_get_bits(ctxt, &tempValue, 16);
                    sh.FRAMERATEEXP = tempValue;
                }
            }

            result = viddec_pm_get_bits(ctxt, &tempValue, 1);
            sh.COLOR_FORMAT_FLAG = tempValue;
            if (sh.COLOR_FORMAT_FLAG == 1)
            {
                result = viddec_pm_get_bits(ctxt, &sh.color_format, 24);
            }
#ifdef VBP
            md->COLOR_FORMAT_FLAG = sh.COLOR_FORMAT_FLAG;
            md->MATRIX_COEF = sh.seq_color_format.MATRIX_COEF;
#endif
        } // Successful get of display size
    } // DISPLAY_EXT is 1

    result = viddec_pm_get_bits(ctxt, &tempValue, 1);
    sh.HRD_PARAM_FLAG = tempValue;
    if (sh.HRD_PARAM_FLAG == 1)
    {
        /* HRD_PARAM(). */
        result = viddec_pm_get_bits(ctxt, &tempValue, 5);
        sh.HRD_NUM_LEAKY_BUCKETS = tempValue;
        md->HRD_NUM_LEAKY_BUCKETS = sh.HRD_NUM_LEAKY_BUCKETS;
#ifndef VBP
        // Skip the rest of the parsing - hrdinfo is not required for decode or for attributes
#else
        {
            uint8_t count;
            uint8_t bitRateExponent;
            uint8_t bufferSizeExponent;

            /* bit_rate_exponent */
            result = viddec_pm_get_bits(ctxt, &tempValue, 4);
            bitRateExponent = (uint8_t)(tempValue + 6);

            /* buffer_size_exponent */
            result = viddec_pm_get_bits(ctxt, &tempValue, 4);
            bufferSizeExponent = (uint8_t)(tempValue + 4);
            md->hrd_initial_state.BUFFER_SIZE_EXPONENT = bufferSizeExponent;

            for(count = 0; count < sh.HRD_NUM_LEAKY_BUCKETS; count++)
            {
                /* hrd_rate */
                result = viddec_pm_get_bits(ctxt, &tempValue, 16);
                md->hrd_initial_state.sLeakyBucket[count].HRD_RATE =
                    (uint32_t)(tempValue + 1) << bitRateExponent;

                /* hrd_buffer */
                result = viddec_pm_get_bits(ctxt, &tempValue, 16);
                md->hrd_initial_state.sLeakyBucket[count].HRD_BUFFER =
                    (uint32_t)(tempValue + 1) << bufferSizeExponent;
            }
        }
#endif
    }
    else
    {
        md->HRD_NUM_LEAKY_BUCKETS = 0;
#ifdef VBP
        md->hrd_initial_state.sLeakyBucket[0].HRD_RATE = 0;
#endif
    }

    md->widthMB = (((md->width + 1) * 2) + 15) / VC1_PIXEL_IN_LUMA;
    md->heightMB = (((md->height + 1) * 2) + 15) / VC1_PIXEL_IN_LUMA;

    DEB("md: res: %dx%d\n", md->width, md->height);
    DEB("sh: dispres: %dx%d\n", sh.seq_disp_size.DISP_HORIZ_SIZE, sh.seq_disp_size.DISP_VERT_SIZE);

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi_sl, wi_de;

        wi_sl.vwi_type = VIDDEC_WORKLOAD_SEQUENCE_INFO;

        wi_sl.vc1_sl.size = 0;
        wi_sl.vc1_sl.flags = 0;
        wi_sl.vc1_sl.pad = 0;

        viddec_fw_vc1_set_profile(&wi_sl.vc1_sl, sh.seq_flags.PROFILE);
        viddec_fw_vc1_set_level(&wi_sl.vc1_sl, sh.seq_flags.LEVEL);
        viddec_fw_vc1_set_colordiff_format(&wi_sl.vc1_sl, sh.seq_flags.COLORDIFF_FORMAT);
        viddec_fw_vc1_set_pulldown(&wi_sl.vc1_sl, sh.seq_max_size.PULLDOWN);
        viddec_fw_vc1_set_max_coded_width(&wi_sl.vc1_sl, sh.seq_max_size.MAX_CODED_WIDTH);
        viddec_fw_vc1_set_max_coded_height(&wi_sl.vc1_sl, sh.seq_max_size.MAX_CODED_HEIGHT);

        viddec_fw_vc1_set_bitrtq_postproc(&wi_sl.vc1_sl, sh.seq_flags.BITRTQ_POSTPROC);
        viddec_fw_vc1_set_frmrtq_postproc(&wi_sl.vc1_sl, sh.seq_flags.FRMRTQ_POSTPROC);
        viddec_fw_vc1_set_interlace(&wi_sl.vc1_sl, sh.seq_max_size.INTERLACE);
        viddec_fw_vc1_set_tfcntrflag(&wi_sl.vc1_sl, sh.seq_max_size.TFCNTRFLAG);
        viddec_fw_vc1_set_finterpflag(&wi_sl.vc1_sl, sh.seq_max_size.FINTERPFLAG);
        viddec_fw_vc1_set_psf(&wi_sl.vc1_sl, sh.seq_max_size.PSF);
        viddec_fw_vc1_set_display_ext(&wi_sl.vc1_sl, sh.seq_max_size.DISPLAY_EXT);

        result = viddec_pm_append_workitem(ctxt, &wi_sl, false);

        // send DISPLAY EXTENSION metadata if present
        if (sh.seq_max_size.DISPLAY_EXT)
        {
            wi_de.vwi_type = VIDDEC_WORKLOAD_DISPLAY_INFO;

            wi_de.vc1_sl_de.size = 0;
            wi_de.vc1_sl_de.framerate = 0;
            wi_de.vc1_sl_de.aspectsize = 0;

            viddec_fw_vc1_set_disp_horiz_size(&wi_de.vc1_sl_de, sh.seq_disp_size.DISP_HORIZ_SIZE);
            viddec_fw_vc1_set_disp_vert_size(&wi_de.vc1_sl_de, sh.seq_disp_size.DISP_VERT_SIZE);
            viddec_fw_vc1_set_disp_aspect_ratio_flag(&wi_de.vc1_sl_de, sh.seq_disp_size.ASPECT_RATIO_FLAG);
            viddec_fw_vc1_set_disp_color_format_flag(&wi_de.vc1_sl_de, sh.COLOR_FORMAT_FLAG);
            viddec_fw_vc1_set_disp_framerate_flag(&wi_de.vc1_sl_de, sh.FRAMERATE_FLAG);
            viddec_fw_vc1_set_disp_framerateind(&wi_de.vc1_sl_de, sh.FRAMERATEIND);

            viddec_fw_vc1_set_disp_aspect_ratio(&wi_de.vc1_sl_de, sh.ASPECT_RATIO);
            viddec_fw_vc1_set_disp_frameratenr(&wi_de.vc1_sl_de, sh.seq_framerate_fraction.FRAMERATENR);
            viddec_fw_vc1_set_disp_frameratedr(&wi_de.vc1_sl_de, sh.seq_framerate_fraction.FRAMERATEDR);
            viddec_fw_vc1_set_disp_framerateexp(&wi_de.vc1_sl_de, sh.FRAMERATEEXP);

            viddec_fw_vc1_set_disp_aspect_ratio_horiz_size(&wi_de.vc1_sl_de, sh.seq_aspect_size.ASPECT_HORIZ_SIZE);
            viddec_fw_vc1_set_disp_aspect_ratio_vert_size(&wi_de.vc1_sl_de, sh.seq_aspect_size.ASPECT_VERT_SIZE);
            viddec_fw_vc1_set_disp_color_prim(&wi_de.vc1_sl_de, sh.seq_color_format.COLOR_PRIM);
            viddec_fw_vc1_set_disp_transfer_char(&wi_de.vc1_sl_de, sh.seq_color_format.TRANSFER_CHAR);

            result = viddec_pm_append_workitem(ctxt, &wi_de, false);
        }
    }

    return status;
}

/*------------------------------------------------------------------------------
 * Parse entry point layer.  This function is only applicable for advanced
 * profile and is used to signal a random access point and changes in coding
 * control parameters.
 * Table 14 of SMPTE 421M.
 * Table 15 of SMPTE 421M for HRD_FULLNESS().
 *------------------------------------------------------------------------------
 */
vc1_Status vc1_ParseEntryPointLayer(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    vc1_metadata_t *md = &pInfo->metadata;
    vc1_EntryPointHeader ep;
    uint32_t result;
    uint32_t temp;

    memset(&ep, 0, sizeof(vc1_EntryPointHeader));

    // PARSE ENTRYPOINT HEADER
    result = viddec_pm_get_bits(ctxt, &ep.flags, 13);
    if (result == 1)
    {
        // Skip the flags already peeked at (13) and the unneeded hrd_full data
        // NOTE: HRD_NUM_LEAKY_BUCKETS is initialized to 0 when HRD_PARAM_FLAG is not present
        int hrd_bits = md->HRD_NUM_LEAKY_BUCKETS * 8;
        while (hrd_bits >= 32)
        {
            result = viddec_pm_skip_bits(ctxt, 32);
            hrd_bits -= 32;
        }
        result = viddec_pm_skip_bits(ctxt, hrd_bits);

        md->REFDIST = 0;
        md->BROKEN_LINK = ep.ep_flags.BROKEN_LINK;
        md->CLOSED_ENTRY = ep.ep_flags.CLOSED_ENTRY;
        md->PANSCAN_FLAG = ep.ep_flags.PANSCAN_FLAG;
        md->REFDIST_FLAG = ep.ep_flags.REFDIST_FLAG;
        md->LOOPFILTER = ep.ep_flags.LOOPFILTER;
        md->FASTUVMC = ep.ep_flags.FASTUVMC;
        md->EXTENDED_MV = ep.ep_flags.EXTENDED_MV;
        md->DQUANT = ep.ep_flags.DQUANT;
        md->VSTRANSFORM = ep.ep_flags.VSTRANSFORM;
        md->OVERLAP = ep.ep_flags.OVERLAP;
        md->QUANTIZER = ep.ep_flags.QUANTIZER;

        result = viddec_pm_get_bits(ctxt, &temp, 1);
        if (result == 1)
        {
            ep.CODED_SIZE_FLAG = temp;
            if (ep.CODED_SIZE_FLAG)
            {
                result = viddec_pm_get_bits(ctxt, &ep.size, 24);
                md->width = ep.ep_size.CODED_WIDTH;
                md->height = ep.ep_size.CODED_HEIGHT;
            }
        }
        if (ep.ep_flags.EXTENDED_MV)
        {
            result = viddec_pm_get_bits(ctxt, &temp, 1);
            md->EXTENDED_DMV = ep.EXTENDED_DMV = temp;
        }

        result = viddec_pm_get_bits(ctxt, &temp, 1);
        if (result == 1)
        {
            md->RANGE_MAPY_FLAG = ep.RANGE_MAPY_FLAG = temp;
            if (ep.RANGE_MAPY_FLAG)
            {
                result = viddec_pm_get_bits(ctxt, &temp, 3);
                md->RANGE_MAPY = ep.RANGE_MAPY = temp;
            }
        }

        result = viddec_pm_get_bits(ctxt, &temp, 1);
        if (result == 1)
        {
            md->RANGE_MAPUV_FLAG = ep.RANGE_MAPUV_FLAG = temp;
            if (ep.RANGE_MAPUV_FLAG)
            {
                result = viddec_pm_get_bits(ctxt, &temp, 3);
                md->RANGE_MAPUV = ep.RANGE_MAPUV = temp;
            }
        }
    }

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi;

        wi.vwi_type = VIDDEC_WORKLOAD_GOP_INFO;

        wi.vc1_ep.size = 0;
        wi.vc1_ep.flags = 0;
        wi.vc1_ep.pad = 0;

        viddec_fw_vc1_set_ep_size_flag(&wi.vc1_ep, ep.CODED_SIZE_FLAG);
        viddec_fw_vc1_set_ep_horiz_size(&wi.vc1_ep, ep.ep_size.CODED_WIDTH);
        viddec_fw_vc1_set_ep_vert_size(&wi.vc1_ep, ep.ep_size.CODED_HEIGHT);

        viddec_fw_vc1_set_ep_broken_link(&wi.vc1_ep, ep.ep_flags.BROKEN_LINK);
        viddec_fw_vc1_set_ep_closed_entry(&wi.vc1_ep, ep.ep_flags.CLOSED_ENTRY);
        viddec_fw_vc1_set_ep_panscan_flag(&wi.vc1_ep, ep.ep_flags.PANSCAN_FLAG);
        viddec_fw_vc1_set_ep_range_mapy_flag(&wi.vc1_ep, ep.RANGE_MAPY_FLAG);
        viddec_fw_vc1_set_ep_range_mapy(&wi.vc1_ep, ep.RANGE_MAPY);
        viddec_fw_vc1_set_ep_range_mapuv_flag(&wi.vc1_ep, ep.RANGE_MAPUV_FLAG);
        viddec_fw_vc1_set_ep_range_mapuv(&wi.vc1_ep, ep.RANGE_MAPUV);

        result = viddec_pm_append_workitem(ctxt, &wi, false);
    }

    DEB("ep: res: %dx%d\n", ep.ep_size.CODED_WIDTH, ep.ep_size.CODED_HEIGHT);
    DEB("md: after ep: res: %dx%d\n", md->width, md->height);
    return status;
}

/*------------------------------------------------------------------------------
 * Parse picture layer.  This function parses the picture layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParsePictureLayer(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_OK;
    uint32_t temp;
    int i;

    for (i=0; i<VC1_MAX_BITPLANE_CHUNKS; i++)
    {
        pInfo->metadata.bp_raw[i] = true;
    }

    if (pInfo->metadata.PROFILE == VC1_PROFILE_ADVANCED)
    {
        VC1_PEEK_BITS(2, temp); /* fcm */
        if ( (pInfo->metadata.INTERLACE == 1) && (temp == VC1_FCM_FIELD_INTERLACE))
        {
            status = vc1_ParseFieldHeader_Adv(ctxt, pInfo);
        }
        else
        {
            status = vc1_ParsePictureHeader_Adv(ctxt, pInfo);
        }
    }
    else
    {
        status = vc1_ParsePictureHeader(ctxt, pInfo);
    }

    return status;
}

/*------------------------------------------------------------------------------
 * Parse field picture layer.  This function parses the field picture layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseFieldLayer(void* ctxt, vc1_Info *pInfo)
{
    vc1_Status status = VC1_STATUS_PARSE_ERROR;
    vc1_PictureLayerHeader *picLayerHeader = &pInfo->picLayerHeader;

    if (pInfo->metadata.PROFILE == VC1_PROFILE_ADVANCED) {
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
        status = vc1_ParsePictureFieldHeader_Adv(ctxt, pInfo);
    }

    return status;
}

/*------------------------------------------------------------------------------
 * Parse slice layer.  This function parses the slice layer, which is only
 * supported by advanced profile.
 * Table 26 of SMPTE 421M but skipping parsing of macroblock layer.
 *------------------------------------------------------------------------------
 */

vc1_Status vc1_ParseSliceLayer(void* ctxt, vc1_Info *pInfo)
{
    uint32_t tempValue;
    uint32_t SLICE_ADDR;
    vc1_Status status = VC1_STATUS_OK;

    VC1_GET_BITS9(9, SLICE_ADDR);
    VC1_GET_BITS9(1, tempValue); /* PIC_HEADER_FLAG. */
    if (tempValue == 1) {
        uint8_t *last_bufptr = pInfo->bufptr;
        uint32_t last_bitoff = pInfo->bitoff;
        status = vc1_ParsePictureLayer(ctxt, pInfo);
        pInfo->picture_info_has_changed = 1;
        if ( status ) {
            /* FIXME - is this a good way of handling this? Failed, see if it's for fields */
            pInfo->bufptr = last_bufptr;
            pInfo->bitoff = last_bitoff;
            status = vc1_ParseFieldHeader_Adv(ctxt, pInfo);
        }
    } else
        pInfo->picture_info_has_changed = 0;

    pInfo->picLayerHeader.SLICE_ADDR = SLICE_ADDR;

    return status;
}

/*------------------------------------------------------------------------------
 * This function parses the user data information as defined in SMPTE 421M annex F.
 * It then appends that data to the workload.
 * Assume the flush byte 0x80 is within the 3 bytes before next start code.
 * let's put 1 byte per item first
 *------------------------------------------------------------------------------
 */
vc1_Status vc1_ParseAndAppendUserData(void* ctxt, uint32_t sc)
{
    vc1_Status status = VC1_STATUS_OK;
    uint32_t user_data;
    viddec_workload_item_t wi;
    uint32_t ud_id;

    /* find the scope based on start code sc */
    switch (sc) {
    case vc1_SCSequenceUser:
        wi.vwi_type = VIDDEC_WORKLOAD_SEQ_USER_DATA;
        break;
    case vc1_SCEntryPointUser:
        wi.vwi_type = VIDDEC_WORKLOAD_GOP_USER_DATA;
        break;
    case vc1_SCFrameUser:
        wi.vwi_type = VIDDEC_WORKLOAD_FRM_USER_DATA;
        break;
    case vc1_SCFieldUser:
        wi.vwi_type = VIDDEC_WORKLOAD_FLD_USER_DATA;
        break;
    case vc1_SCSliceUser:
        wi.vwi_type = VIDDEC_WORKLOAD_SLC_USER_DATA;
        break;
    default:
        wi.vwi_type = VIDDEC_WORKLOAD_INVALID; //ERROR - should not happen
        break;
    }

    /* get identifier - 4 bytes*/
    // Extract this information but discard it for now
    VC1_GET_BITS(32, ud_id);

    /* Read 1 byte of user data and store it in workitem for the current stream level (SEQ/GOP/PIC).
       Keep adding data payloads till it reaches size 11. When it is 11, the maximum user data payload size,
       append the workitem. This loop is repeated till all user data is extracted and appended. */
    wi.user_data.size = 0;
    while (viddec_pm_get_bits(ctxt, &user_data, 8) != -1)
    {
        /* Store the valid byte in data payload */
        wi.user_data.data_payload[wi.user_data.size] = user_data;
        wi.user_data.size++;

        /* When size exceeds payload size, append workitem and continue */
        if (wi.user_data.size >= 11)
        {
            viddec_pm_setup_userdata(&wi);
            viddec_pm_append_workitem(ctxt, &wi,false);
            wi.user_data.size = 0;
        }
        if (user_data == 0x80) // flushing byte
            break;
    }
    /* If size is not 0, append remaining user data. */
    if (wi.user_data.size > 0)
    {
        int i;
        for (i=wi.user_data.size; i<11; i++)
        {
            wi.user_data.data_payload[i] = 0;
        }
        viddec_pm_setup_userdata(&wi);
        viddec_pm_append_workitem(ctxt, &wi,false);
        wi.user_data.size = 0;
    }

    return(status);
} // vc1_ParseAndAppendUserData
