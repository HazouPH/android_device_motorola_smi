#include "viddec_mp4_videoobjectplane.h"

mp4_Status_t mp4_Parse_GroupOfVideoObjectPlane(void *parent, viddec_mp4_parser_t *parser)
{
    mp4_Info_t* pInfo = &(parser->info);
    uint32_t  code;
    int32_t getbits=0;
    mp4_Status_t ret = MP4_STATUS_REQD_DATA_ERROR;
    mp4_GroupOfVideoObjectPlane_t *data;
    uint32_t time_code = 0;

    data = &(pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane);

    do
    {
        getbits = viddec_pm_get_bits(parent, &code, 20);
        BREAK_GETBITS_FAIL(getbits, ret);
        ret = MP4_STATUS_OK;

        data->broken_link = ((code & 0x1) > 0);
        data->closed_gov = ((code & 0x2) > 0);
        time_code = code = code >> 2;
        data->time_code_seconds = code & 0x3F;
        code = code >> 6;
        if ((code & 1) == 0)
        {/* SGA:Should we ignore marker bit? */
            DEB("Error:mp4_Parse_GroupOfVideoObjectPlane: Invalid marker\n");
        }
        code = code >>1;
        data->time_code_minutes = code & 0x3F;
        code = code >> 6;
        data->time_code_hours = code & 0x1F;

        // This is the timebase in full second units
        data->time_base = data->time_code_seconds + (60*data->time_code_minutes) + (3600*data->time_code_hours);
        // Need to convert this into no. of ticks
        data->time_base *= pInfo->VisualObject.VideoObject.vop_time_increment_resolution;

    } while (0);

    mp4_set_hdr_bitstream_error(parser, true, ret);

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi;

        wi.vwi_type = VIDDEC_WORKLOAD_MPEG4_GRP_VIDEO_OBJ;

        wi.mp4_gvop.gvop_info = 0;
        wi.mp4_gvop.pad1 = 0;
        wi.mp4_gvop.pad2 = 0;

        viddec_fw_mp4_gvop_set_broken_link(&wi.mp4_gvop, data->broken_link);
        viddec_fw_mp4_gvop_set_closed_gov(&wi.mp4_gvop, data->closed_gov);
        viddec_fw_mp4_gvop_set_time_code(&wi.mp4_gvop, time_code);

        ret = (mp4_Status_t)viddec_pm_append_workitem(parent, &wi, false);
        if (ret == 1)
            ret = MP4_STATUS_OK;
    }

    return ret;
}

static inline mp4_Status_t mp4_brightness_change(void *parent, int32_t *b_change)
{
    uint32_t code;
    int32_t getbits=0;

    *b_change = 0;
    getbits = viddec_pm_peek_bits(parent, &code, 4);
    if (code == 15)
    {
        getbits = viddec_pm_skip_bits(parent, 4);
        getbits = viddec_pm_get_bits(parent, &code, 10);
        *b_change = 625 + code;
    }
    else if (code == 14)
    {
        getbits = viddec_pm_skip_bits(parent, 4);
        getbits = viddec_pm_get_bits(parent, &code, 9);
        *b_change = 113 + code;
    }
    else if (code >= 12)
    {
        getbits = viddec_pm_skip_bits(parent, 3);
        getbits = viddec_pm_get_bits(parent, &code, 7);
        *b_change = (code < 64) ? ((int32_t)code - 112) : ((int32_t)code - 15);
    }
    else if (code >= 8)
    {
        getbits = viddec_pm_skip_bits(parent, 2);
        getbits = viddec_pm_get_bits(parent, &code, 6);
        *b_change = (code < 32) ? ((int32_t)code - 48) : ((int32_t)code - 15);
    }
    else
    {
        getbits = viddec_pm_skip_bits(parent, 1);
        getbits = viddec_pm_get_bits(parent, &code, 5);
        *b_change = (code < 16) ? ((int32_t)code - 16) : ((int32_t)code - 15);
    }

    return ( (getbits == -1) ? MP4_STATUS_PARSE_ERROR: MP4_STATUS_OK);
}
static inline int32_t mp4_Sprite_dmv_length(void * parent, int32_t *dmv_length)
{
    uint32_t code, skip;
    int32_t getbits=0;
    mp4_Status_t ret= MP4_STATUS_PARSE_ERROR;
    *dmv_length=0;
    skip=3;
    do {
        getbits = viddec_pm_peek_bits(parent, &code, skip);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);

        if (code == 7)
        {
            viddec_pm_skip_bits(parent, skip);
            getbits = viddec_pm_peek_bits(parent, &code, 9);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);

            skip=1;
            while ((code & 256) != 0)
            {/* count number of 1 bits */
                code <<=1;
                skip++;
            }
            *dmv_length = 5 + skip;
        }
        else
        {
            skip=(code <= 1) ? 2 : 3;
            *dmv_length = code - 1;
        }
        viddec_pm_skip_bits(parent, skip);
        ret= MP4_STATUS_OK;

    } while (0);
    return ret;
}

static inline mp4_Status_t
mp4_Sprite_Trajectory(void *parent, mp4_VideoObjectLayer_t *vidObjLay, mp4_VideoObjectPlane_t *vidObjPlane)
{
    uint32_t code, i;
    int32_t dmv_length=0, dmv_code=0, getbits=0;
    mp4_Status_t ret = MP4_STATUS_OK;
    for (i=0; i < (uint32_t)vidObjLay->sprite_info.no_of_sprite_warping_points; i++ )
    {
        ret = (mp4_Status_t)mp4_Sprite_dmv_length(parent, &dmv_length);
        if (ret != MP4_STATUS_OK)
        {
            break;
        }
        if (dmv_length <= 0)
        {
            dmv_code = 0;
        }
        else
        {
            getbits = viddec_pm_get_bits(parent, &code, (uint32_t)dmv_length);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            dmv_code = (int32_t)code;
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
            {
                dmv_code -= (1 << dmv_length) - 1;
            }
        }
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        if (code != 1)
        {
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }
        vidObjPlane->warping_mv_code_du[i] = dmv_code;
        /* TODO: create another inline function to avoid code duplication */
        ret = (mp4_Status_t)mp4_Sprite_dmv_length(parent, &dmv_length);
        if (ret != MP4_STATUS_OK)
        {
            break;
        }
        if (dmv_length <= 0)
        {
            dmv_code = 0;
        }
        else
        {
            getbits = viddec_pm_get_bits(parent, &code, (uint32_t)dmv_length);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            dmv_code = (int32_t)code;
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
            {
                dmv_code -= (1 << dmv_length) - 1;
            }
        }
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        if (code != 1)
        {
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }
        vidObjPlane->warping_mv_code_dv[i] = dmv_code;

    }
    return ret;
}

static inline mp4_Status_t mp4_pvt_extract_modulotimebase_from_VideoObjectPlane(void *parent, uint32_t *base)
{
    mp4_Status_t ret= MP4_STATUS_OK;
    int32_t getbits=0;
    uint32_t  code = 0;

    *base = 0;
    do
    {
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        *base += code;
    } while (code != 0);
    return ret;
}

mp4_Status_t mp4_Parse_VideoObjectPlane(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t  code;
    mp4_Info_t               *pInfo = &(parser->info);
    mp4_VideoObjectLayer_t   *vidObjLay  = &(pInfo->VisualObject.VideoObject);
    mp4_VideoObjectPlane_t   *vidObjPlane = &(pInfo->VisualObject.VideoObject.VideoObjectPlane);
    int32_t getbits=0;
    mp4_Status_t ret= MP4_STATUS_PARSE_ERROR;

    do
    {
        getbits = viddec_pm_get_bits(parent, &code, 2);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjPlane->vop_coding_type = code & 0x3;
        if ( mp4_pvt_extract_modulotimebase_from_VideoObjectPlane(parent,
                &(vidObjPlane->modulo_time_base)) == MP4_STATUS_REQD_DATA_ERROR)
        {
            break;
        }

        getbits = viddec_pm_get_bits(parent, &code, 1);
        /* TODO: check for marker bit validity */
        {
            uint32_t numbits=0;
            numbits = vidObjLay->vop_time_increment_resolution_bits;
            if (numbits == 0) numbits=1; /*TODO:check if its greater than 16 bits ?? */
            getbits = viddec_pm_get_bits(parent, &code, numbits);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjPlane->vop_time_increment = code;
        }

        getbits = viddec_pm_get_bits(parent, &code, 2);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);

        vidObjPlane->vop_coded = code & 0x1;
        if (vidObjPlane->vop_coded == 0)
        {
            ret = MP4_STATUS_OK;/* Exit point 1 */
            break;
        }

        if (vidObjLay->newpred_enable)
        {
            /* New pred mode not supported in HW */
            DEB("Error: mp4_Parse_VideoObjectPlane: New pred in vidObjPlane is not supported\n");
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }

        if ((vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY) &&
                ((vidObjPlane->vop_coding_type == MP4_VOP_TYPE_P) ||
                 ((vidObjPlane->vop_coding_type == MP4_VOP_TYPE_S) &&
                  (vidObjLay->sprite_enable == MP4_SPRITE_GMC))))
        {
            getbits = viddec_pm_get_bits(parent, &code, 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjPlane->vop_rounding_type = code;
        }

        if (vidObjLay->reduced_resolution_vop_enable &&
                (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_RECTANGULAR) &&
                ((vidObjPlane->vop_coding_type == MP4_VOP_TYPE_I) ||
                 (vidObjPlane->vop_coding_type == MP4_VOP_TYPE_P)))
        {
            getbits = viddec_pm_get_bits(parent, &code, 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjPlane->vop_reduced_resolution = code;
            if (vidObjPlane->vop_reduced_resolution)
            {
                DEB("Error: mp4_Parse_VideoObjectPlane: Reduced Resolution vidObjPlane is not supported\n");
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }
        }

        if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_RECTANGULAR)
        {
            /* we support only rectangular shapes so the following logic is not required */
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }

        if ((vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY) &&
                (!vidObjLay->complexity_estimation_disable))
        {
            /* Not required according to DE team */
            //read_vop_complexity_estimation_header();
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }

        if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY)
        {
            getbits = viddec_pm_get_bits(parent, &code, 3);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjPlane->intra_dc_vlc_thr = code;
            if (vidObjLay->interlaced)
            {
                getbits = viddec_pm_get_bits(parent, &code, 2);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjPlane->top_field_first = ((code & 0x2) > 0);
                vidObjPlane->alternate_vertical_scan_flag = code & 0x1;
            }
        }

        if (((vidObjLay->sprite_enable == MP4_SPRITE_STATIC) || (vidObjLay->sprite_enable == MP4_SPRITE_GMC)) &&
                (vidObjPlane->vop_coding_type == MP4_VOP_TYPE_S))
        {
            if (vidObjLay->sprite_info.no_of_sprite_warping_points > 0) {
                if (mp4_Sprite_Trajectory(parent, vidObjLay, vidObjPlane) != MP4_STATUS_OK) {
                    break;
                }
            }
            vidObjPlane->brightness_change_factor = 0;
            if (vidObjLay->sprite_info.sprite_brightness_change)
            {
                int32_t change=0;
                if (mp4_brightness_change(parent, &change) == MP4_STATUS_PARSE_ERROR)
                {
                    break;
                }
                vidObjPlane->brightness_change_factor = change;
            }

            if (vidObjLay->sprite_enable == MP4_SPRITE_STATIC)
            {
                /* SGA: IS decode sprite not required. Is static even supported */
                ret = MP4_STATUS_OK;/* Exit point 2 */
                break;
            }
        }

        if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY)
        {
            // Length of vop_quant is specified by quant_precision
            getbits = viddec_pm_get_bits(parent, &code, vidObjLay->quant_precision);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjPlane->vop_quant = code;
            if (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_GRAYSCALE)
            {
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }
            if (vidObjPlane->vop_coding_type != MP4_VOP_TYPE_I)
            {
                vidObjPlane->vop_fcode_forward = 0;
                getbits = viddec_pm_get_bits(parent, &code, 3);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjPlane->vop_fcode_forward = code & 0x7;
                if (vidObjPlane->vop_fcode_forward == 0)
                {
                    DEB("Error: vop_fcode_forward == 0\n");
                    break;
                }
            }
            if (vidObjPlane->vop_coding_type == MP4_VOP_TYPE_B)
            {
                vidObjPlane->vop_fcode_backward = 0;
                getbits = viddec_pm_get_bits(parent, &code, 3);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjPlane->vop_fcode_backward = code &0x7;
                if (vidObjPlane->vop_fcode_backward == 0)
                {
                    DEB("Error: vop_fcode_backward == 0\n");
                    break;
                }
            }
            if (!vidObjLay->scalability)
            {
                if ((vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_RECTANGULAR) &&
                        (vidObjPlane->vop_coding_type != MP4_VOP_TYPE_I))
                {
                    ret = MP4_STATUS_NOTSUPPORT;
                    break;
                }
                // The remaining data contains the macroblock information that is handled by the BSP
                // The offsets to be sent to the BSP are obtained in the workload population
            }
            else
            {
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }
        }
        else
        {/* Binary Not supported */
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }
        /* Since we made it all the way here it a success condition */
        ret = MP4_STATUS_OK;  /* Exit point 3 */
    } while (0);

    mp4_set_hdr_bitstream_error(parser, false, ret);

    return ret;
} // mp4_Parse_VideoObjectPlane
