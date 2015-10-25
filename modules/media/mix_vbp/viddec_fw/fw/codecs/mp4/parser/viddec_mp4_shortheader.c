#include "viddec_mp4_shortheader.h"

typedef struct
{
    uint16_t vop_width;
    uint16_t vop_height;
    uint16_t num_macroblocks_in_gob;
    uint16_t num_gobs_in_vop;
    uint8_t  num_rows_in_gob;
} svh_src_fmt_params_t;

const svh_src_fmt_params_t svh_src_fmt_defaults[5] =
{
    {128,    96,   8,  6, 1},
    {176,   144,  11,  9, 1},
    {352,   288,  22, 18, 1},
    {704,   576,  88, 18, 2},
    {1408, 1152, 352, 18, 4},
};

mp4_Status_t mp4_Parse_VideoObjectPlane_svh(void *parent, viddec_mp4_parser_t *parser)
{
    mp4_Status_t ret = MP4_STATUS_OK;
    unsigned int data;
    mp4_VideoObjectPlaneH263 *svh = &(parser->info.VisualObject.VideoObject.VideoObjectPlaneH263);
    int32_t getbits = 0;
    uint8_t pei = 0;
    uint8_t optional_indicators_8bits = 0;

    do
    {
        //temporal reference
        getbits = viddec_pm_get_bits(parent, &data, 8);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        svh->temporal_reference = (data & 0xff);
        //marker bit
        getbits = viddec_pm_get_bits(parent, &data, 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        if ( 1 != (data & 0x1))
        {
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }
        //zero bit
        getbits = viddec_pm_get_bits(parent, &data, 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        if ( 0 != (data & 0x1))
        {
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }
        //split_screen_indicator, document_camera_indicator, full_picture_freeze_release
        getbits = viddec_pm_get_bits(parent, &data, 3);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        //source format
        getbits = viddec_pm_get_bits(parent, &data, 3);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        svh->source_format = (data & 0x7);
        if (svh->source_format == 0 || svh->source_format == 6)
        {
            DEB("Error: Bad value for VideoPlaneWithShortHeader.source_format\n");
            ret = MP4_STATUS_NOTSUPPORT;
            break;
        }

        if (svh->source_format != 7)
        {
            //picture coding type
            getbits = viddec_pm_get_bits(parent, &data, 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            svh->picture_coding_type = (data & 0x1);
            //reserved zero bits
            getbits = viddec_pm_get_bits(parent, &data, 4);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            if ( 0 != (data & 0xf))
            {
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }
            //vop quant
            getbits = viddec_pm_get_bits(parent, &data, 5);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            svh->vop_quant = (data & 0x1f);
            //cpm
            getbits = viddec_pm_get_bits(parent, &data, 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            if ( 0 != (data & 0x1))
            {
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }
        }
        else //extended PTYPE (PLUSPTYPE)
        {
            //ufep
            getbits = viddec_pm_get_bits(parent, &data, 3);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            svh->ufep = (data & 0x7); //ufep
            if (svh->ufep == 1 || svh->ufep == 0)
            {
                //OPPTYPE
                if (svh->ufep == 1)
                {
                    //source format
                    getbits = viddec_pm_get_bits(parent, &data, 3);
                    BREAK_GETBITS_REQD_MISSING(getbits, ret);
                    svh->source_format = (data & 0x7);
                    if (svh->source_format < 1 || svh->source_format > 6)
                    {
                        DEB("Error: bad value of source_format\n");
                        ret = MP4_STATUS_PARSE_ERROR;
                        break;
                    }
                    //optional indicators
                    getbits = viddec_pm_get_bits(parent, &data, 8);
                    BREAK_GETBITS_REQD_MISSING(getbits, ret);
                    optional_indicators_8bits = data;
                    //reserved zero bits
                    getbits = viddec_pm_get_bits(parent, &data, 3);
                    BREAK_GETBITS_REQD_MISSING(getbits, ret);
                    if ( 0 != (data & 0x7))
                    {
                        ret = MP4_STATUS_PARSE_ERROR;
                        break;
                    }
                    //marker bit
                    getbits = viddec_pm_get_bits(parent, &data, 1);
                    BREAK_GETBITS_REQD_MISSING(getbits, ret);
                    if ( 1 != (data & 0x1))
                    {
                        ret = MP4_STATUS_PARSE_ERROR;
                        break;
                    }
                    //reserved zero bits
                    getbits = viddec_pm_get_bits(parent, &data, 3);
                    BREAK_GETBITS_REQD_MISSING(getbits, ret);
                    if ( 0 != (data & 0x7))
                    {
                        ret = MP4_STATUS_PARSE_ERROR;
                        break;
                    }
                }

                //MPPTYPE
                //picture coding type
                getbits = viddec_pm_get_bits(parent, &data, 3);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                svh->picture_coding_type = (data & 0x7);
                if (svh->picture_coding_type > 1)
                {
                    DEB("Info: only support I and P frames\n");
                    ret = MP4_STATUS_NOTSUPPORT;
                    break;
                }
                //optional RPR mode
                getbits = viddec_pm_get_bits(parent, &data, 1);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                if ( 0 != (data & 0x1))
                {
                    ret = MP4_STATUS_PARSE_ERROR;
                    break;
                }
                //optional PRU mode
                getbits = viddec_pm_get_bits(parent, &data, 1);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                if ( 0 != (data & 0x1))
                {
                    ret = MP4_STATUS_PARSE_ERROR;
                    break;
                }
                //vop rounding type
                getbits = viddec_pm_get_bits(parent, &data, 1);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                svh->vop_rounding_type = (data & 0x1);
                //reserved zero bits
                getbits = viddec_pm_get_bits(parent, &data, 2);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                if ( 0 != (data & 0x3))
                {
                    ret = MP4_STATUS_PARSE_ERROR;
                    break;
                }
                //marker bit
                getbits = viddec_pm_get_bits(parent, &data, 1);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                if ( 1 != (data & 0x1))
                {
                    ret = MP4_STATUS_PARSE_ERROR;
                    break;
                }
            }
            else
            {
                DEB("Info: don't support to handle the other case of Update Full Extended PTYPE\n");
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }

            //cpm
            getbits = viddec_pm_get_bits(parent, &data, 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            if ( 0 != (data & 0x1))
            {
                ret = MP4_STATUS_NOTSUPPORT;
                break;
            }

            //CPFMT
            if (svh->ufep == 1 && svh->source_format == 6)
            {   //Pixel Aspect Ratio
                getbits = viddec_pm_get_bits(parent, &data, 4);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                svh->pixel_aspect_ratio_code = (data & 0xf);
                //Picture Width Indication
                getbits = viddec_pm_get_bits(parent, &data, 9);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                svh->picture_width_indication = (data & 0x1ff);
                //marker bit
                getbits = viddec_pm_get_bits(parent, &data, 1);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                if ( 1 != (data & 0x1))
                {
                    ret = MP4_STATUS_PARSE_ERROR;
                    break;
                }
                //Picture Height Indication
                getbits = viddec_pm_get_bits(parent, &data, 9);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                svh->picture_height_indication = (data & 0x1ff);

                if (svh->pixel_aspect_ratio_code == 0xf)
                {
                    //EPAR
                    viddec_pm_get_bits(parent, &data, 16);
                }
            }

            //custom PCF
            if (optional_indicators_8bits & 0x80) {
                viddec_pm_get_bits(parent, &data, 8);
                viddec_pm_get_bits(parent, &data, 2);
            }

            viddec_pm_get_bits(parent, &data, 5);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            svh->vop_quant = (data & 0x1f);
        }
        //PEI
        do
        {
            getbits = viddec_pm_get_bits(parent, &data, 1); // pei
            BREAK_GETBITS_FAIL(getbits, ret);
            pei = (data & 0x1);
            if (0 != pei)
            {
                getbits = viddec_pm_get_bits(parent, &data, 8); // psupp
                BREAK_GETBITS_FAIL(getbits, ret);
            }
        } while ( 1 == pei);

        // Anything after this needs to be fed to the decoder as PIXEL_ES
    } while (0);

    return ret;
}

mp4_Status_t mp4_Parse_VideoObject_svh(void *parent, viddec_mp4_parser_t *parser)
{
    mp4_Status_t             ret=MP4_STATUS_OK;
    mp4_Info_t              *pInfo = &(parser->info);
    mp4_VideoSignalType_t *vst = &(pInfo->VisualObject.VideoSignalType);
    mp4_VideoObjectLayer_t  *vol = &(pInfo->VisualObject.VideoObject);
    mp4_VideoObjectPlane_t  *vop = &(pInfo->VisualObject.VideoObject.VideoObjectPlane);
    mp4_VideoObjectPlaneH263 *svh = &(pInfo->VisualObject.VideoObject.VideoObjectPlaneH263);
    uint8_t index = 0;
    uint8_t k = 0;

    ret = mp4_Parse_VideoObjectPlane_svh(parent, parser);
    if (ret == MP4_STATUS_OK)
    {
        // Populate defaults for the svh
        vol->short_video_header = 1;
        vol->video_object_layer_shape = MP4_SHAPE_TYPE_RECTANGULAR;
        vol->obmc_disable = 1;
        vol->quant_type = 0;
        vol->resync_marker_disable = 1;
        vol->data_partitioned = 0;
        vol->reversible_vlc = 0;
        vol->interlaced = 0;
        vol->complexity_estimation_disable = 1;
        vol->scalability = 0;
        vol->not_8_bit = 0;
        vol->bits_per_pixel = 8;
        vol->quant_precision = 5;
        vol->vop_time_increment_resolution = 30000;
        vol->fixed_vop_time_increment = 1001;
        vol->aspect_ratio_info = MP4_ASPECT_RATIO_12_11;

        vop->vop_rounding_type = svh->vop_rounding_type;
        vop->vop_fcode_forward = 1;
        vop->vop_coded = 1;
        vop->vop_coding_type = svh->picture_coding_type ? MP4_VOP_TYPE_P: MP4_VOP_TYPE_I;
        vop->vop_quant = svh->vop_quant;

        vst->colour_primaries = 1;
        vst->transfer_characteristics = 1;
        vst->matrix_coefficients = 6;

        if (svh->source_format >= 1 && svh->source_format <= 5)
        {
            index = svh->source_format - 1;
            vol->video_object_layer_width = svh_src_fmt_defaults[index].vop_width;
            vol->video_object_layer_height = svh_src_fmt_defaults[index].vop_height;
            svh->num_macroblocks_in_gob = svh_src_fmt_defaults[index].num_macroblocks_in_gob;
            svh->num_gobs_in_vop = svh_src_fmt_defaults[index].num_gobs_in_vop;
            svh->num_rows_in_gob = svh_src_fmt_defaults[index].num_rows_in_gob;
        }
        else if (svh->source_format == 6) //custom format
        {
            vol->video_object_layer_width = (svh->picture_width_indication + 1)*4;
            vol->video_object_layer_height = (svh->picture_height_indication)*4;
            if (vol->video_object_layer_height < 404)
            {
                k = 1;
            }
            else if (vol->video_object_layer_height < 804)
            {
                k = 2;
            }
            else
            {
                k = 4;
            }
	     svh->num_macroblocks_in_gob = (((vol->video_object_layer_width + 15) & ~15) /16)*k;
            svh->num_gobs_in_vop = (((vol->video_object_layer_height + 15) & ~15)/(16*k));
            svh->num_rows_in_gob = k;
        }
        else
        {
            DEB("Error: Bad value for VideoPlaneWithShortHeader.source_format\n");
            ret = MP4_STATUS_NOTSUPPORT;
            return ret;
        }
    }

    mp4_set_hdr_bitstream_error(parser, false, ret);

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi;

        wi.vwi_type = VIDDEC_WORKLOAD_MPEG4_VIDEO_PLANE_SHORT;

        wi.mp4_vpsh.info = 0;
        wi.mp4_vpsh.pad1 = 0;
        wi.mp4_vpsh.pad2 = 0;

        viddec_fw_mp4_vpsh_set_source_format(&wi.mp4_vpsh, svh->source_format);

        ret = (mp4_Status_t)viddec_pm_append_workitem(parent, &wi, false);
        if (ret == 1)
            ret = MP4_STATUS_OK;
    }

    return ret;
}
