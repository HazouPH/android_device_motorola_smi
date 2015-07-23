#include <string.h>
#include "viddec_mp4_videoobjectlayer.h"
#ifndef VBP
const unsigned char mp4_DefaultIntraQuantMatrix[64] = {
    8, 17, 18, 19, 21, 23, 25, 27,
    17, 18, 19, 21, 23, 25, 27, 28,
    20, 21, 22, 23, 24, 26, 28, 30,
    21, 22, 23, 24, 26, 28, 30, 32,
    22, 23, 24, 26, 28, 30, 32, 35,
    23, 24, 26, 28, 30, 32, 35, 38,
    25, 26, 28, 30, 32, 35, 38, 41,
    27, 28, 30, 32, 35, 38, 41, 45
};
const unsigned char mp4_DefaultNonIntraQuantMatrix[64] = {
    16, 17, 18, 19, 20, 21, 22, 23,
    17, 18, 19, 20, 21, 22, 23, 24,
    18, 19, 20, 21, 22, 23, 24, 25,
    19, 20, 21, 22, 23, 24, 26, 27,
    20, 21, 22, 23, 25, 26, 27, 28,
    21, 22, 23, 24, 26, 27, 28, 30,
    22, 23, 24, 26, 27, 28, 30, 31,
    23, 24, 25, 27, 28, 30, 31, 33
};

#else
const unsigned char mp4_DefaultIntraQuantMatrix[64] = {
    8, 17, 17, 20, 18, 18, 19, 19,
    21, 21, 22, 22, 22, 21, 21, 23,
    23, 23, 23, 23, 23, 25, 24, 24,
    24, 24, 25, 25, 27, 27, 26, 26,
    26, 26, 26, 27, 28, 28, 28, 28,
    28, 28, 28, 30, 30, 30, 30, 30,
    30, 32, 32, 32, 32, 32, 35, 35,
    35, 35, 38, 38, 38, 41, 41, 45
};

const unsigned char mp4_DefaultNonIntraQuantMatrix[64] = {
    16, 17, 17, 18, 18, 18, 19, 19,
    19, 19, 20, 20, 20, 20, 20, 21,
    21, 21, 21, 21, 21, 22, 22, 22,
    22, 22, 22, 22, 23, 23, 23, 23,
    23, 23, 23, 23, 24, 24, 24, 25,
    24, 24, 24, 25, 26, 26, 26, 26,
    25, 27, 27, 27, 27, 27, 28, 28,
    28, 28, 30, 30, 30, 31, 31, 33
};

#endif
const unsigned char mp4_ClassicalZigzag[64] = {
    0,   1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

static inline int mp4_GetMacroBlockNumberSize(int nmb)
{
    int  nb = 0;
    nmb --;
    do {
        nmb >>= 1;
        nb ++;
    } while (nmb);
    return nb;
}

static inline void mp4_copy_default_table(const uint8_t *src, uint8_t *dst, uint32_t len)
{
    uint32_t i;
    for (i=0; i< len; i++)
        dst[i] = src[i];
}


static inline mp4_Status_t mp4_Parse_QuantMatrix(void *parent, uint8_t *pQM)
{
    uint32_t i,code=0;
    uint8_t last=0;
    int32_t                 getbits=0;
    mp4_Status_t            ret = MP4_STATUS_OK;

    for (i = 0; i < 64; i ++)
    {
        getbits = viddec_pm_get_bits(parent, &code, 8);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        if (code == 0) break;
        pQM[mp4_ClassicalZigzag[i]] = (uint8_t)(code & 0xFF);
    }
    last = pQM[mp4_ClassicalZigzag[i-1]];
    for (; i < 64; i ++)
    {
        pQM[mp4_ClassicalZigzag[i]] = last;
    }
    return ret;;
}

static inline uint8_t mp4_pvt_valid_object_type_indication(uint8_t val)
{
    return ((1 <= val) || (val <= 18));
}

static inline uint8_t mp4_pvt_valid_object_layer_verid(uint8_t val)
{
    uint8_t ret=false;
    switch (val)
    {
    case 1:
    case 2:
    case 4:
    case 5:
    {
        ret = true;
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

static mp4_Status_t
mp4_pvt_VOL_volcontrolparameters(void *parent, viddec_mp4_parser_t *parser)
{
    mp4_VOLControlParameters_t *cxt = &(parser->info.VisualObject.VideoObject.VOLControlParameters);
    mp4_Status_t            ret = MP4_STATUS_PARSE_ERROR;
    int32_t                 getbits=0;
    uint32_t                code=0;

    do
    {
        getbits = viddec_pm_get_bits(parent, &(code), 4);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        cxt->chroma_format = (code >> 2) & 0x3;
        cxt->low_delay = ((code & 0x2) > 0);
        cxt->vbv_parameters = code & 0x1;

        if (cxt->chroma_format != MP4_CHROMA_FORMAT_420)
        {
            DEB("Warning: mp4_Parse_VideoObject:vol_control_parameters.chroma_format != 4:2:0\n");
            cxt->chroma_format= MP4_CHROMA_FORMAT_420;
            parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
            ret = MP4_STATUS_NOTSUPPORT;
        }

        if (cxt->vbv_parameters)
        {/* TODO: Check for validity of marker bits */
            getbits = viddec_pm_get_bits(parent, &(code), 32);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            /* 32 bits= firsthalf(15) + M + LatterHalf(15) + M */
            cxt->bit_rate = (code & 0xFFFE) >> 1; // Get rid of 1 marker bit
            cxt->bit_rate |= ((code & 0xFFFE0000) >> 2); // Get rid of 2 marker bits

            if (cxt->bit_rate == 0)
            {
                DEB("Error: mp4_Parse_VideoObject:vidObjLay->VOLControlParameters.bit_rate = 0\n");
                parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
                ret = MP4_STATUS_NOTSUPPORT;
                // Do we need to really break here? Why not just set an error and proceed
                //break;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 19);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            /* 19 bits= firsthalf(15) + M + LatterHalf(3)*/
            cxt->vbv_buffer_size = code & 0x7;
            cxt->vbv_buffer_size |= ( (code >> 4) & 0x7FFF);
            if (cxt->vbv_buffer_size == 0)
            {
                DEB("Error: mp4_Parse_VideoObject:vidObjLay->VOLControlParameters.vbv_buffer_size = 0\n");
                parser->bitstream_error |= MP4_BS_ERROR_HDR_UNSUP;
                ret = MP4_STATUS_NOTSUPPORT;
                // Do we need to really break here? Why not just set an error and proceed
                //break;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 28);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            /* 28 bits= firsthalf(11) + M + LatterHalf(15) + M */
            code = code >>1;
            cxt->vbv_occupancy = code & 0x7FFF;
            code = code >>16;
            cxt->vbv_occupancy |= (code & 0x07FF);
        }
        ret = MP4_STATUS_OK;
    } while (0);

    return ret;
}

static uint32_t mp4_pvt_count_number_of_bits(uint32_t val)
{
    uint32_t num_bits=0;
    do {
        val >>= 1;
        num_bits++;
    } while (val);
    return num_bits;
}

static mp4_Status_t
mp4_Parse_VOL_sprite(void *parent,  viddec_mp4_parser_t *parser)
{
    mp4_VideoObjectLayer_t  *vidObjLay = (&parser->info.VisualObject.VideoObject);
    mp4_VOLSpriteInfo_t     *cxt = &(vidObjLay->sprite_info);
    uint32_t                sprite_enable = vidObjLay->sprite_enable;
    uint32_t                code;
    mp4_Status_t            ret = MP4_STATUS_PARSE_ERROR;
    int32_t                 getbits=0;

    do {
        if ((sprite_enable == MP4_SPRITE_STATIC) ||
                (sprite_enable == MP4_SPRITE_GMC))
        {
            if (sprite_enable != MP4_SPRITE_GMC)
            {
                /* This is not a supported type by HW */
                DEB("ERROR: mp4_Parse_VideoObject:sprite_enable = %.2X\n", sprite_enable);
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 9);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            cxt->sprite_brightness_change = code & 0x1;
            cxt->sprite_warping_accuracy = (code >> 1) & 0x3;
            cxt->no_of_sprite_warping_points = code >> 3;
            if (cxt->no_of_sprite_warping_points > 1)
            {
                DEB("Warning: mp4_Parse_VideoObject:bad no_of_sprite_warping_points %d\n",
                    cxt->no_of_sprite_warping_points);
            }

            if ((vidObjLay->sprite_enable == MP4_SPRITE_GMC) && (cxt->sprite_brightness_change))
            {
                DEB("Error: mp4_Parse_VideoObject:sprite_brightness_change should be 0 for GMC sprites\n");
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }

            if (vidObjLay->sprite_enable != MP4_SPRITE_GMC)
            {
                DEB("ERROR: mp4_Parse_VideoObject:sprite_enable = %.2X\n", sprite_enable);
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }
        }
        ret = MP4_STATUS_OK;
    } while (0);

    return ret;
}

static mp4_Status_t mp4_Parse_VOL_quant_mat(void *parent, mp4_VideoObjectLayer_t  *vidObjLay)
{
    uint32_t                code;
    mp4_Status_t            ret = MP4_STATUS_PARSE_ERROR;
    int32_t                 getbits=0;
    mp4_VOLQuant_mat_t      *quant = &(vidObjLay->quant_mat_info);

    do {
        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        quant->load_intra_quant_mat = code;
        if (quant->load_intra_quant_mat)
        {
            mp4_Parse_QuantMatrix(parent, &(quant->intra_quant_mat[0]));
        }
        else
        {
            mp4_copy_default_table((const uint8_t *)&mp4_DefaultIntraQuantMatrix[0], (uint8_t *)&(quant->intra_quant_mat[0]), 64);
        }

        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        quant->load_nonintra_quant_mat = code;
        if (quant->load_nonintra_quant_mat)
        {
            mp4_Parse_QuantMatrix(parent, &(quant->nonintra_quant_mat[0]));
        }
        else
        {
            mp4_copy_default_table((const uint8_t *)&mp4_DefaultNonIntraQuantMatrix[0], (uint8_t *)&(quant->nonintra_quant_mat[0]), 64);
        }
        ret = MP4_STATUS_OK;
    } while (0);
    return ret;
}

static mp4_Status_t mp4_Parse_VOL_notbinaryonly(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t                code;
    mp4_Info_t              *pInfo = &(parser->info);
    mp4_VideoObjectLayer_t  *vidObjLay = &(pInfo->VisualObject.VideoObject);
    mp4_Status_t            ret = MP4_STATUS_PARSE_ERROR;
    int32_t                 getbits=0;

    do {
        if (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_RECTANGULAR)
        {
            /* TODO: check for validity of marker bits */
            getbits = viddec_pm_get_bits(parent, &(code), 29);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->video_object_layer_height = (code >> 1) & 0x1FFF;
            vidObjLay->video_object_layer_width = (code >> 15) & 0x1FFF;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 2);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjLay->interlaced = ((code & 0x2) > 0);
        vidObjLay->obmc_disable = ((code & 0x1) > 0);

        {
            uint32_t num_bits=1;
            if (vidObjLay->video_object_layer_verid != 1) num_bits=2;
            getbits = viddec_pm_get_bits(parent, &(code), num_bits);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->sprite_enable = code;
        }

        ret = mp4_Parse_VOL_sprite(parent, parser);
        if (ret != MP4_STATUS_OK)
        {
            break;
        }

        if ((vidObjLay->video_object_layer_verid != 1) &&
                (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_RECTANGULAR))
        {
            /*  not supported shape*/
            DEB("Error: mp4_Parse_VideoObject: sadct_disable, not supp\n");
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        vidObjLay->not_8_bit = (code  > 0 );
        if (vidObjLay->not_8_bit)
        {
            /*  8 bit is only supported mode*/
            DEB("Error: mp4_Parse_VideoObject: not_8_bit, not supp\n");
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }
        else
        {/* We use default values since only 8 bit mode is supported */
            vidObjLay->quant_precision = 5;
            vidObjLay->bits_per_pixel = 8;
        }

        if (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_GRAYSCALE)
        {
            /* Should not get here as shape is checked earlier */
            DEB("Error: mp4_Parse_VideoObject: GRAYSCALE, not supp\n");
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjLay->quant_type = code;
        if (vidObjLay->quant_type)
        {
            ret = mp4_Parse_VOL_quant_mat(parent, vidObjLay);
            if (ret != MP4_STATUS_OK)
            {
                break;
            }
        }

        if (vidObjLay->video_object_layer_verid != 1)
        {
            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->quarter_sample = code;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjLay->complexity_estimation_disable = code;
        if (!vidObjLay->complexity_estimation_disable)
        {/*  complexity estimation not supported */
            DEB("Error: mp4_Parse_VideoObject: vidObjLay->complexity_estimation_disable, not supp\n");
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 2);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjLay->resync_marker_disable = ((code & 0x2) > 0);
        vidObjLay->data_partitioned = code & 0x1;
        if (vidObjLay->data_partitioned)
        {
            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->reversible_vlc = code;
        }

        if (vidObjLay->video_object_layer_verid != 1)
        {
            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_FAIL(getbits, ret);
            vidObjLay->newpred_enable = code;
            if (vidObjLay->newpred_enable)
            {
                DEB("Error: NEWPRED mode is not supported\n");
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }
            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_FAIL(getbits, ret);
            vidObjLay->reduced_resolution_vop_enable = code;
        }

        getbits = viddec_pm_get_bits(parent, &(code), 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        vidObjLay->scalability = code;
        if (vidObjLay->scalability)
        {
            DEB("Error: VOL scalability is not supported\n");
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }

        // No need to parse further - none of the fields are interesting to parser/decoder/user
        ret = MP4_STATUS_OK;
    } while (0);
    return ret;
}

mp4_Status_t mp4_Parse_VideoObjectLayer(void *parent, viddec_mp4_parser_t *parser)
{
    uint32_t                code;
    mp4_Info_t              *pInfo = &(parser->info);
    mp4_VisualObject_t      *visObj = &(pInfo->VisualObject);
    mp4_VideoObjectLayer_t  *vidObjLay = &(pInfo->VisualObject.VideoObject);
    mp4_Status_t            ret = MP4_STATUS_PARSE_ERROR;
    int32_t                 getbits=0;

//DEB("entering mp4_Parse_VideoObjectLayer: bs_err: %d, ret: %d\n", parser->bitstream_error, ret);

    // Trying to parse more header data as it is more important than frame data
    if (parser->bitstream_error > MP4_HDR_ERROR_MASK)
        return ret;

    do {
        vidObjLay->VideoObjectPlane.sprite_transmit_mode = MP4_SPRITE_TRANSMIT_MODE_PIECE;

        vidObjLay->short_video_header = 0;
        vidObjLay->video_object_layer_id = (parser->current_sc & 0xF);

        getbits = viddec_pm_get_bits(parent, &code, 9);
        BREAK_GETBITS_REQD_MISSING(getbits, ret);
        vidObjLay->video_object_type_indication = code & 0xFF;
        vidObjLay->random_accessible_vol = ((code & 0x100) > 0);

        if (!mp4_pvt_valid_object_type_indication(vidObjLay->video_object_type_indication))
        {        /* Streams with "unknown" type mismatch with ref */
            DEB("Warning: video_object_type_indication = %d, forcing to 1\n",
                vidObjLay->video_object_type_indication);
            vidObjLay->video_object_type_indication = 1;
        }

        if (vidObjLay->video_object_type_indication == MP4_VIDEO_OBJECT_TYPE_FINE_GRANULARITY_SCALABLE)
        {/* This is not a supported type by HW */
            DEB("ERROR: mp4_Parse_VideoObject:video_object_type_indication = %.2X\n",
                vidObjLay->video_object_type_indication);
            ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
            break;
        }
        else
        {
            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->is_object_layer_identifier = code;
            vidObjLay->video_object_layer_verid =
                (mp4_pvt_valid_object_layer_verid(visObj->visual_object_verid)) ? visObj->visual_object_verid : 1;

            if (vidObjLay->is_object_layer_identifier)
            {
                getbits = viddec_pm_get_bits(parent, &(code), 7);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjLay->video_object_layer_priority = code & 0x7;
                vidObjLay->video_object_layer_verid = (code >> 3) & 0xF;
                if (!mp4_pvt_valid_object_layer_verid(vidObjLay->video_object_layer_verid))
                {
                    DEB("Error: mp4_Parse_VideoObject:is_identifier = %d, expected[1,5]\n",
                        vidObjLay->video_object_layer_verid);
                    ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                    break;
                }
                /* Video object layer ID supercedes visual object ID */
                visObj->visual_object_verid = vidObjLay->video_object_layer_verid;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 4);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->aspect_ratio_info = code & 0xF;
            if (vidObjLay->aspect_ratio_info == MP4_ASPECT_RATIO_EXTPAR)
            {
                getbits = viddec_pm_get_bits(parent, &(code), 16);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjLay->aspect_ratio_info_par_width = (code >> 8) & 0xFF;
                vidObjLay->aspect_ratio_info_par_height = code & 0xFF;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 1);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->is_vol_control_parameters = code;
            if (vidObjLay->is_vol_control_parameters)
            {
                ret = mp4_pvt_VOL_volcontrolparameters(parent, parser);
                if (ret != MP4_STATUS_OK)
                {
                    break;
                }
            }

            getbits = viddec_pm_get_bits(parent, &(code), 2);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            vidObjLay->video_object_layer_shape = code;
            /* If shape is not rectangluar exit early without parsing */
            if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_RECTANGULAR)
            {
                DEB("Error: mp4_Parse_VideoObject: shape not rectangluar(%d):%d\n",
                    MP4_SHAPE_TYPE_RECTANGULAR, vidObjLay->video_object_layer_shape);
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }

            if ((vidObjLay->video_object_layer_verid != 1) &&
                    (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_GRAYSCALE))
            {/* Grayscale not supported */
                DEB("Error: MP4_SHAPE_TYPE_GRAYSCALE not supported\n");
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }

            getbits = viddec_pm_get_bits(parent, &(code), 19);
            BREAK_GETBITS_REQD_MISSING(getbits, ret);
            /* TODO: check validity of marker */
            vidObjLay->vop_time_increment_resolution = (code >> 2) & 0xFFFF;
            vidObjLay->fixed_vop_rate = code & 0x1;

            if (vidObjLay->vop_time_increment_resolution == 0)
            {
                DEB("Error: 0 value for vop_time_increment_resolution\n");
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }
            /* calculate number bits in vop_time_increment_resolution */
            vidObjLay->vop_time_increment_resolution_bits = (uint8_t)mp4_pvt_count_number_of_bits(
                        (uint32_t)(vidObjLay->vop_time_increment_resolution -1));

            if (vidObjLay->fixed_vop_rate)
            {
                getbits = viddec_pm_get_bits(parent, &(code), vidObjLay->vop_time_increment_resolution_bits);
                BREAK_GETBITS_REQD_MISSING(getbits, ret);
                vidObjLay->fixed_vop_time_increment = code;
            }

            if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY)
            {
                ret = mp4_Parse_VOL_notbinaryonly(parent, parser);
                if (ret != MP4_STATUS_OK)
                {
                    break;
                }
            }
            else
            {
                DEB("Error: MP4_SHAPE_TYPE_BINARYONLY not supported\n");
                ret = (mp4_Status_t)(MP4_STATUS_NOTSUPPORT | MP4_STATUS_REQD_DATA_ERROR);
                break;
            }
        }

        vidObjLay->VideoObjectPlane.sprite_transmit_mode = MP4_SPRITE_TRANSMIT_MODE_PIECE;
        ret = MP4_STATUS_OK;
    } while (0);

    mp4_set_hdr_bitstream_error(parser, true, ret);
    if (ret != MP4_STATUS_OK) {
        parser->bitstream_error |= MP4_BS_ERROR_HDR_NONDEC;
        return ret;
    }
//DEB("before wkld mp4_Parse_VideoObjectLayer: bs_err: %d, ret: %d\n", parser->bitstream_error, ret);

    // POPULATE WORKLOAD ITEM
    {
        viddec_workload_item_t wi;
        viddec_workload_t *wl = viddec_pm_get_header(parent);

        wi.vwi_type = VIDDEC_WORKLOAD_MPEG4_VIDEO_OBJ;

        wi.mp4_vol.vol_aspect_ratio = 0;
        wi.mp4_vol.vol_bit_rate = 0;
        wi.mp4_vol.vol_frame_rate = 0;

        viddec_fw_mp4_vol_set_aspect_ratio_info(&wi.mp4_vol, vidObjLay->aspect_ratio_info);
        viddec_fw_mp4_vol_set_par_width(&wi.mp4_vol, vidObjLay->aspect_ratio_info_par_width);
        viddec_fw_mp4_vol_set_par_height(&wi.mp4_vol, vidObjLay->aspect_ratio_info_par_height);
        viddec_fw_mp4_vol_set_control_param(&wi.mp4_vol, vidObjLay->is_vol_control_parameters);
        viddec_fw_mp4_vol_set_chroma_format(&wi.mp4_vol, vidObjLay->VOLControlParameters.chroma_format);
        viddec_fw_mp4_vol_set_interlaced(&wi.mp4_vol, vidObjLay->interlaced);
        viddec_fw_mp4_vol_set_fixed_vop_rate(&wi.mp4_vol, vidObjLay->fixed_vop_rate);

        viddec_fw_mp4_vol_set_vbv_param(&wi.mp4_vol, vidObjLay->VOLControlParameters.vbv_parameters);
        viddec_fw_mp4_vol_set_bit_rate(&wi.mp4_vol, vidObjLay->VOLControlParameters.bit_rate);

        viddec_fw_mp4_vol_set_fixed_vop_time_increment(&wi.mp4_vol, vidObjLay->fixed_vop_time_increment);
        viddec_fw_mp4_vol_set_vop_time_increment_resolution(&wi.mp4_vol, vidObjLay->vop_time_increment_resolution);

        ret = (mp4_Status_t)viddec_pm_append_workitem(parent, &wi, false);
        if (ret == 1)
            ret = MP4_STATUS_OK;

        memset(&(wl->attrs), 0, sizeof(viddec_frame_attributes_t));

        wl->attrs.cont_size.width = vidObjLay->video_object_layer_width;
        wl->attrs.cont_size.height = vidObjLay->video_object_layer_height;
    }

    return ret;
}
