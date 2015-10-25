/* INTEL CONFIDENTIAL
* Copyright (c) 2009, 2012 Intel Corporation.  All rights reserved.
*
* The source code contained or described herein and all documents
* related to the source code ("Material") are owned by Intel
* Corporation or its suppliers or licensors.  Title to the
* Material remains with Intel Corporation or its suppliers and
* licensors.  The Material contains trade secrets and proprietary
* and confidential information of Intel or its suppliers and
* licensors. The Material is protected by worldwide copyright and
* trade secret laws and treaty provisions.  No part of the Material
* may be used, copied, reproduced, modified, published, uploaded,
* posted, transmitted, distributed, or disclosed in any way without
* Intel's prior express written permission.
*
* No license under any patent, copyright, trade secret or other
* intellectual property right is granted to or conferred upon you
* by disclosure or delivery of the Materials, either expressly, by
* implication, inducement, estoppel or otherwise. Any license
* under such intellectual property rights must be express and
* approved by Intel in writing.
*
*/



#include <dlfcn.h>

#include <string.h>
#include "vbp_loader.h"
#include "vbp_utils.h"
#include "vbp_mp42_parser.h"
#include "../codecs/mp4/parser/viddec_mp4_parse.h"



typedef struct vbp_mp42_parser_private_t vbp_mp42_parser_private;

struct vbp_mp42_parser_private_t
{
    bool short_video_header;
};

static uint8 mp4_aspect_ratio_table[][2] =
{
    // forbidden
    {0, 0},
    {1, 1},
    {12, 11},
    {10, 11},
    {16, 11},
    {40, 33},

    // reserved
    {0, 0}
};


/*
 * Some divX avi files contains 2 frames in one gstbuffer.
 */


uint32 vbp_get_sc_pos_mp42(
    uint8 *buf,
    uint32 length,
    uint32 *sc_end_pos,
    uint8 *is_normal_sc,
    uint8* resync_marker,
    const bool svh_search);

void vbp_on_vop_mp42(vbp_context *pcontext, int list_index);
void vbp_on_vop_svh_mp42(vbp_context *pcontext, int list_index);
void vbp_fill_codec_data(vbp_context *pcontext);
vbp_picture_data_mp42* vbp_get_mp42_picture_data(vbp_data_mp42 * query_data);
uint32 vbp_process_slices_mp42(vbp_context *pcontext, int list_index);
uint32 vbp_process_slices_svh_mp42(vbp_context *pcontext, int list_index);
uint32 vbp_process_video_packet_mp42(vbp_context *pcontext);

static inline uint32 vbp_sprite_trajectory_mp42(
    void *parent,
    mp4_VideoObjectLayer_t *vidObjLay,
    mp4_VideoObjectPlane_t *vidObjPlane);


static inline uint32 vbp_sprite_dmv_length_mp42(
    void * parent,
    int32_t *dmv_length);


/**
 *
 */
uint32 vbp_init_parser_entries_mp42( vbp_context *pcontext)
{
    if (NULL == pcontext->parser_ops)
    {
        // absolutely impossible, just sanity check
        return VBP_PARM;
    }
    pcontext->parser_ops->init = dlsym(pcontext->fd_parser, "viddec_mp4_init");
    if (pcontext->parser_ops->init == NULL)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#ifdef VBP
    pcontext->parser_ops->parse_sc = NULL;
#else
    pcontext->parser_ops->parse_sc = dlsym(pcontext->fd_parser, "viddec_parse_sc_mp4");
    if (pcontext->parser_ops->parse_sc == NULL)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#endif
    pcontext->parser_ops->parse_syntax = dlsym(pcontext->fd_parser, "viddec_mp4_parse");
    if (pcontext->parser_ops->parse_syntax == NULL)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }

    pcontext->parser_ops->get_cxt_size =dlsym(pcontext->fd_parser, "viddec_mp4_get_context_size");
    if (pcontext->parser_ops->get_cxt_size == NULL)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#ifdef VBP
    pcontext->parser_ops->is_wkld_done = NULL;
#else
    pcontext->parser_ops->is_wkld_done = dlsym(pcontext->fd_parser, "viddec_mp4_wkld_done");
    if (pcontext->parser_ops->is_wkld_done == NULL)
    {
        ETRACE ("Failed to set entry point." );
        return VBP_LOAD;
    }
#endif

    /* entry point not needed */
    pcontext->parser_ops->flush = NULL;

    return VBP_OK;
}


/*
 * For the codec_data passed by gstreamer
 */
uint32 vbp_parse_init_data_mp42(vbp_context *pcontext)
{
    uint32 ret = VBP_OK;
    ret = vbp_parse_start_code_mp42(pcontext);
    return ret;
}

uint32 vbp_process_parsing_result_mp42(vbp_context *pcontext, int list_index)
{
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    viddec_mp4_parser_t *parser =
        (viddec_mp4_parser_t *) &(pcontext->parser_cxt->codec_data[0]);
    vbp_mp42_parser_private *parser_private = (vbp_mp42_parser_private *)pcontext->parser_private;

    uint8 is_svh = 0;
    uint32 current_sc = parser->current_sc;
    is_svh = parser->cur_sc_prefix ? false : true;

    if (!is_svh)
    {
        // remove prefix from current_sc
        current_sc &= 0x0FF;
        switch (current_sc)
        {
        case MP4_SC_VISUAL_OBJECT_SEQUENCE:
            VTRACE ("Visual Object Sequence is parsed.\n");
            query_data->codec_data.profile_and_level_indication
                    = parser->info.profile_and_level_indication;
            VTRACE ("profile_and_level_indication = 0x%x\n", parser->info.profile_and_level_indication);
            break;

        case MP4_SC_VIDEO_OBJECT_PLANE:
            //VTRACE ("Video Object Plane is parsed.\n");
            vbp_on_vop_mp42(pcontext, list_index);
            break;

        default:
            if ((current_sc >= MP4_SC_VIDEO_OBJECT_LAYER_MIN) &&
                (current_sc <= MP4_SC_VIDEO_OBJECT_LAYER_MAX))
            {
                VTRACE ("Video Object Layer is parsed\n");
                parser_private->short_video_header = FALSE;
                vbp_fill_codec_data(pcontext);
            }
            else if (current_sc <= MP4_SC_VIDEO_OBJECT_MAX &&
                     current_sc >= MP4_SC_VIDEO_OBJECT_MIN)
            {
                if (parser->sc_seen == MP4_SC_SEEN_SVH)
                {
                    // this should never happen!!!!
                    WTRACE ("Short video header is parsed.\n");
                    // vbp_on_vop_svh_mp42(pcontext, list_index);
                    return VBP_TYPE;
                }
            }
            break;
        }
    }
    else
    {
        if (parser->sc_seen == MP4_SC_SEEN_SVH)
        {
            //VTRACE ("Short video header is parsed.\n");
            vbp_on_vop_svh_mp42(pcontext, list_index);
        }
    }

    return VBP_OK;
}



/*
* partial frame handling:
*
* h.263: picture header is lost if the first GOB is discarded, a redudant pic header must be
* conveyed in the packet  (RFC 4629) for each following GOB, otherwise,
* picture can't be decoded.
*
* MPEG4:  VideoObjectPlane header is lost if the first slice is discarded. However, picture
* is still decodable as long as the header_extension_code is 1 in video_packet_header.
*
*MPEG-4 with short header:   video_plane_with_short_header is lost if the first GOB
* is discarded. As this header is not duplicated (RFC 3016), picture is not decodable.
*
* In sum:
* If buffer contains the 32-bit start code (0x000001xx), proceed  as normal.
*
* If buffer contains 22-bits of "0000 0000 0000 0000 1000 00", which indicates h.263
* picture start code or short_video_start_marker, proceed as normal.
*
* If buffer contains 22-bits of "0000 0000 0000 0000 1XXX XX", (when XXX XX starts from 000 01), which
* indicates  h.263 Group Start code or gob_resync_marker of gob_layer in MPEG-4 with
* short header, we should report packet as a partial frame - no more parsing is needed.
*
* If buffer contains a string of 0 between 16 bits and 22 bits, followed by 1-bit of '1', which indicates a resync-marker,
* the buffer will be immeidately parsed and num_items is set to 0.
*/
uint32 vbp_parse_start_code_mp42(vbp_context *pcontext)
{
    viddec_pm_cxt_t *cxt = pcontext->parser_cxt;
    uint8 *buf = NULL;
    uint32 size = 0;
    uint32 sc_end_pos = -1;
    uint32 bytes_parsed = 0;
    viddec_mp4_parser_t *pinfo = NULL;
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    vbp_mp42_parser_private *parser_private = (vbp_mp42_parser_private *)pcontext->parser_private;


    // reset query data for the new sample buffer
    query_data->number_picture_data= 0;
    query_data->number_pictures = 0;

    // emulation prevention byte is not needed
    cxt->getbits.is_emul_reqd = 0;

    cxt->list.num_items = 0;
    cxt->list.data[0].stpos = 0;
    cxt->list.data[0].edpos = cxt->parse_cubby.size;

    buf = cxt->parse_cubby.buf;
    size = cxt->parse_cubby.size;

    pinfo = (viddec_mp4_parser_t *) &(cxt->codec_data[0]);

    uint8 is_normal_sc = 0;
    uint8 resync_marker = 0;
    uint32 found_sc = 0;
    uint32 ret = VBP_OK;

    while (1)
    {
        found_sc = vbp_get_sc_pos_mp42(
                        buf + bytes_parsed,
                        size - bytes_parsed,
                        &sc_end_pos,
                        &is_normal_sc,
                        &resync_marker,
                        parser_private->short_video_header);

        if (found_sc)
        {
            cxt->list.data[cxt->list.num_items].stpos = bytes_parsed + sc_end_pos - 3;
            if (cxt->list.num_items != 0)
            {
                cxt->list.data[cxt->list.num_items - 1].edpos = bytes_parsed + sc_end_pos - 3;
            }
            bytes_parsed += sc_end_pos;

            cxt->list.num_items++;
            pinfo->cur_sc_prefix = is_normal_sc;
        }
        else
        {
            if (cxt->list.num_items != 0)
            {
                cxt->list.data[cxt->list.num_items - 1].edpos = cxt->parse_cubby.size;
                break;
            }
            else
            {
                WTRACE ("No start-code is found in cubby buffer! The size of cubby is %d\n", size);
                cxt->list.num_items = 1;
                cxt->list.data[0].stpos = 0;
                cxt->list.data[0].edpos = cxt->parse_cubby.size;

                if (resync_marker)
                {
                    // either the first slice (GOB) is lost or parser receives a single slice (GOB)
                    if (parser_private->short_video_header)
                    {
                        // TODO: revisit if HW supportd GOB layer decoding for h.263
                        WTRACE("Partial frame: GOB buffer.\n");
                        ret = VBP_PARTIAL;
                    }
                    else
                    {
                        WTRACE("Partial frame: video packet header buffer.\n");
                        ret =  vbp_process_video_packet_mp42(pcontext);
                    }

                    // set num_items to 0 so buffer will not be parsed again
                    cxt->list.num_items = 0;
                }
                else
                {
                    ETRACE("Invalid data received.\n");
                    cxt->list.num_items = 0;
                    return VBP_DATA;
                }

                break;
            }
        }
    }

    return ret;
}

uint32 vbp_populate_query_data_mp42(vbp_context *pcontext)
{
#if 0
    vbp_dump_query_data(pcontext);
#endif
    return VBP_OK;
}

vbp_picture_data_mp42* vbp_get_mp42_picture_data(vbp_data_mp42 * query_data)
{
    vbp_picture_data_mp42 *picture_data = query_data->picture_data;
    int num_pictures = query_data->number_picture_data;
    while (num_pictures > 1)
    {
        picture_data = picture_data->next_picture_data;
        num_pictures--;
    }

    return picture_data;
}

void vbp_fill_codec_data(vbp_context *pcontext)
{
    viddec_mp4_parser_t *parser =
            (viddec_mp4_parser_t *) &(pcontext->parser_cxt->codec_data[0]);
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    vbp_codec_data_mp42* codec_data = &(query_data->codec_data);
    vbp_mp42_parser_private *parser_private = (vbp_mp42_parser_private *)pcontext->parser_private;

    codec_data->bit_rate = parser->info.VisualObject.VideoObject.VOLControlParameters.bit_rate;

    codec_data->profile_and_level_indication
            = parser->info.profile_and_level_indication;

    codec_data->video_object_layer_width =
            parser->info.VisualObject.VideoObject.video_object_layer_width;

    codec_data->video_object_layer_height =
            parser->info.VisualObject.VideoObject.video_object_layer_height;

    if (parser->info.VisualObject.VideoSignalType.is_video_signal_type)
    {
        codec_data->video_format =
                parser->info.VisualObject.VideoSignalType.video_format;
    }
    else
    {
        // Unspecified video format
        codec_data->video_format =  5;
    }

    codec_data->video_range =
            parser->info.VisualObject.VideoSignalType.video_range;

    if (parser->info.VisualObject.VideoSignalType.is_colour_description)
    {
        codec_data->matrix_coefficients =
                parser->info.VisualObject.VideoSignalType.matrix_coefficients;
    }
    else if (parser_private->short_video_header)
    {
        // SMPTE 170M
        codec_data->matrix_coefficients = 6;
    }
    else
    {
        // ITU-R Recommendation BT.709
        codec_data->matrix_coefficients = 1;
    }

    codec_data->short_video_header = parser_private->short_video_header;

    // aspect ratio
    codec_data->aspect_ratio_info = parser->info.VisualObject.VideoObject.aspect_ratio_info;
    if (codec_data->aspect_ratio_info < 6)
    {
        codec_data->par_width = mp4_aspect_ratio_table[codec_data->aspect_ratio_info][0];
        codec_data->par_height = mp4_aspect_ratio_table[codec_data->aspect_ratio_info][1];
    }
    else if (codec_data->aspect_ratio_info == 15)
    {
        codec_data->par_width = parser->info.VisualObject.VideoObject.aspect_ratio_info_par_width;
        codec_data->par_height = parser->info.VisualObject.VideoObject.aspect_ratio_info_par_height;
    }
    else
    {
        codec_data->par_width = 0;
        codec_data->par_height = 0;
    }
}

void vbp_fill_slice_data(vbp_context *pcontext, int list_index)
{
    viddec_mp4_parser_t *parser =
            (viddec_mp4_parser_t *) &(pcontext->parser_cxt->codec_data[0]);

    if (!parser->info.VisualObject.VideoObject.short_video_header)
    {
        vbp_process_slices_mp42(pcontext, list_index);
    }
    else
    {
        vbp_process_slices_svh_mp42(pcontext, list_index);
    }
}

void vbp_fill_picture_param(vbp_context *pcontext, uint8 new_picture_flag)
{
    viddec_mp4_parser_t *parser =
            (viddec_mp4_parser_t *) &(pcontext->parser_cxt->codec_data[0]);
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;

    vbp_picture_data_mp42 *picture_data = NULL;
    VAPictureParameterBufferMPEG4 *picture_param = NULL;

    if (new_picture_flag)
    {
        query_data->number_pictures++;
    }

    picture_data = query_data->picture_data;
    if (picture_data == NULL || query_data->number_picture_data == 0)
    {
        // first entry
        if (picture_data == NULL)
        {
            picture_data = vbp_malloc_set0(vbp_picture_data_mp42, 1);
            query_data->picture_data = picture_data;
            if (picture_data == NULL) {
                query_data->number_picture_data = 0;
                return;
            }
        }
        query_data->number_picture_data = 1;
    }
    else
    {
        // find the last active one
        int i = query_data->number_picture_data;
        while (i > 1)
        {
            picture_data = picture_data->next_picture_data;
            i--;
        }
        if (picture_data->next_picture_data == NULL)
        {
            picture_data->next_picture_data = vbp_malloc_set0(vbp_picture_data_mp42, 1);
            if (picture_data->next_picture_data == NULL) {
                return;
            }
        }

        query_data->number_picture_data++;

        picture_data = picture_data->next_picture_data;
    }

    picture_param = &(picture_data->picture_param);

    uint8 idx = 0;

    picture_data->new_picture_flag = new_picture_flag;

    picture_data->vop_coded
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_coded;



    picture_data->vop_time_increment =
            parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_time_increment;

    // fill picture_param


    /*
     * NOTE: for short video header, the parser saves vop_width and vop_height
     * to VOL->video_object_layer_width and VOL->video_object_layer_height
     */
    picture_param->vop_width
            = parser->info.VisualObject.VideoObject.video_object_layer_width;
    picture_param->vop_height
            = parser->info.VisualObject.VideoObject.video_object_layer_height;

    picture_param->forward_reference_picture = VA_INVALID_SURFACE;
    picture_param->backward_reference_picture = VA_INVALID_SURFACE;

    // Fill VAPictureParameterBufferMPEG4::vol_fields

    picture_param->vol_fields.bits.short_video_header
            = parser->info.VisualObject.VideoObject.short_video_header;
    picture_param->vol_fields.bits.chroma_format
            = parser->info.VisualObject.VideoObject.VOLControlParameters.chroma_format;

    // TODO: find out why testsuite always set this value to be 0
    picture_param->vol_fields.bits.chroma_format = 0;

    picture_param->vol_fields.bits.interlaced
            = parser->info.VisualObject.VideoObject.interlaced;
    picture_param->vol_fields.bits.obmc_disable
            = parser->info.VisualObject.VideoObject.obmc_disable;
    picture_param->vol_fields.bits.sprite_enable
            = parser->info.VisualObject.VideoObject.sprite_enable;
    picture_param->vol_fields.bits.sprite_warping_accuracy
            = parser->info.VisualObject.VideoObject.sprite_info.sprite_warping_accuracy;
    picture_param->vol_fields.bits.quant_type
            = parser->info.VisualObject.VideoObject.quant_type;
    picture_param->vol_fields.bits.quarter_sample
            = parser->info.VisualObject.VideoObject.quarter_sample;
    picture_param->vol_fields.bits.data_partitioned
            = parser->info.VisualObject.VideoObject.data_partitioned;
    picture_param->vol_fields.bits.reversible_vlc
            = parser->info.VisualObject.VideoObject.reversible_vlc;
    picture_param->vol_fields.bits.resync_marker_disable
            = parser->info.VisualObject.VideoObject.resync_marker_disable;
    picture_param->no_of_sprite_warping_points
            = parser->info.VisualObject.VideoObject.sprite_info.no_of_sprite_warping_points;

    for (idx = 0; idx < 3; idx++)
    {
        picture_param->sprite_trajectory_du[idx]
                = parser->info.VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_du[idx];
        picture_param->sprite_trajectory_dv[idx]
                = parser->info.VisualObject.VideoObject.VideoObjectPlane.warping_mv_code_dv[idx];
    }

    picture_param->quant_precision
            = parser->info.VisualObject.VideoObject.quant_precision;

    // fill VAPictureParameterBufferMPEG4::vop_fields


    if (!parser->info.VisualObject.VideoObject.short_video_header)
    {
        picture_param->vop_fields.bits.vop_coding_type
                = parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_coding_type;
    }
    else
    {
        picture_param->vop_fields.bits.vop_coding_type
                = parser->info.VisualObject.VideoObject.VideoObjectPlaneH263.picture_coding_type;
    }

      // TODO: fill picture_param->vop_fields.bits.backward_reference_vop_coding_type
      // This shall be done in mixvideoformat_mp42. See M42 spec 7.6.7

    if (picture_param->vop_fields.bits.vop_coding_type != MP4_VOP_TYPE_B)
    {
        picture_param->vop_fields.bits.backward_reference_vop_coding_type
                = picture_param->vop_fields.bits.vop_coding_type;
    }

    picture_param->vop_fields.bits.vop_rounding_type
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_rounding_type;
    picture_param->vop_fields.bits.intra_dc_vlc_thr
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.intra_dc_vlc_thr;
    picture_param->vop_fields.bits.top_field_first
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.top_field_first;
    picture_param->vop_fields.bits.alternate_vertical_scan_flag
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.alternate_vertical_scan_flag;

    picture_param->vop_fcode_forward
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_fcode_forward;
    picture_param->vop_fcode_backward
            = parser->info.VisualObject.VideoObject.VideoObjectPlane.vop_fcode_backward;
    picture_param->vop_time_increment_resolution
            = parser->info.VisualObject.VideoObject.vop_time_increment_resolution;

    // short header related
    picture_param->num_gobs_in_vop
            = parser->info.VisualObject.VideoObject.VideoObjectPlaneH263.num_gobs_in_vop;
    picture_param->num_macroblocks_in_gob
            = parser->info.VisualObject.VideoObject.VideoObjectPlaneH263.num_macroblocks_in_gob;

    // for direct mode prediction
    picture_param->TRB = parser->info.VisualObject.VideoObject.TRB;
    picture_param->TRD = parser->info.VisualObject.VideoObject.TRD;
}

void vbp_fill_iq_matrix_buffer(vbp_context *pcontext)
{
    viddec_mp4_parser_t *parser =
            (viddec_mp4_parser_t *) &(pcontext->parser_cxt->codec_data[0]);
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;

    mp4_VOLQuant_mat_t *quant_mat_info =
            &(parser->info.VisualObject.VideoObject.quant_mat_info);

    VAIQMatrixBufferMPEG4 *iq_matrix = NULL;

    iq_matrix = &(query_data->iq_matrix_buffer);

    iq_matrix->load_intra_quant_mat = 1; //quant_mat_info->load_intra_quant_mat;
    iq_matrix->load_non_intra_quant_mat = 1; // = quant_mat_info->load_nonintra_quant_mat;
    memcpy(iq_matrix->intra_quant_mat, quant_mat_info->intra_quant_mat, 64);
    memcpy(iq_matrix->non_intra_quant_mat, quant_mat_info->nonintra_quant_mat, 64);
}


void vbp_on_vop_mp42(vbp_context *pcontext, int list_index)
{
    vbp_fill_codec_data(pcontext);
    vbp_fill_picture_param(pcontext, 1);
    vbp_fill_iq_matrix_buffer(pcontext);
    vbp_fill_slice_data(pcontext, list_index);
}

void vbp_on_vop_svh_mp42(vbp_context *pcontext, int list_index)
{
    vbp_fill_codec_data(pcontext);
    vbp_fill_picture_param(pcontext, 1);
    vbp_fill_iq_matrix_buffer(pcontext);
    vbp_fill_slice_data(pcontext, list_index);
}

uint32 vbp_get_sc_pos_mp42(
    uint8 *buf,
    uint32 length,
    uint32 *sc_end_pos,
    uint8 *is_normal_sc,
    uint8 *resync_marker,
    const bool svh_search)
{
    uint8 *ptr = buf;
    uint32 size;
    uint32 data_left = 0, phase = 0, ret = 0;
    size = 0;

    data_left = length;
    *sc_end_pos = -1;

    /* parse until there is more data and start code not found */
    while ((data_left > 0) && (phase < 3))
    {
        /* Check if we are byte aligned & phase=0, if thats the case we can check
         work at a time instead of byte*/
        if (((((uint32) ptr) & 0x3) == 0) && (phase == 0))
        {
            while (data_left > 3)
            {
                uint32 data;
                char mask1 = 0, mask2 = 0;

                data = *((uint32 *) ptr);
#ifndef MFDBIGENDIAN
                data = SWAP_WORD(data);
#endif
                mask1 = (FIRST_STARTCODE_BYTE != (data & SC_BYTE_MASK0));
                mask2 = (FIRST_STARTCODE_BYTE != (data & SC_BYTE_MASK1));
                /* If second byte and fourth byte are not zero's then we cannot have a start code here as we need
                 two consecutive zero bytes for a start code pattern */
                if (mask1 && mask2)
                {
                    /* Success so skip 4 bytes and start over */
                    ptr += 4;
                    size += 4;
                    data_left -= 4;
                    continue;
                }
                else
                {
                    break;
                }
            }
        }

        /* At this point either data is not on a word boundary or phase > 0 or On a word boundary but we detected
         two zero bytes in the word so we look one byte at a time*/
        if (data_left > 0)
        {
            if (*ptr == FIRST_STARTCODE_BYTE)
            {
                /* Phase can be 3 only if third start code byte is found */
                phase++;
                ptr++;
                size++;
                data_left--;
                if (phase > 2)
                {
                    phase = 2;

                    if ((((uint32) ptr) & 0x3) == 0)
                    {
                        while (data_left > 3)
                        {
                            if (*((uint32 *) ptr) != 0)
                            {
                                break;
                            }
                            ptr += 4;
                            size += 4;
                            data_left -= 4;
                        }
                    }
                }
            }
            else
            {
                uint8 normal_sc = 0, short_sc = 0;
                if (phase == 2)
                {
                    normal_sc = (*ptr == THIRD_STARTCODE_BYTE);
                    if (svh_search)
                    {
                       short_sc = (SHORT_THIRD_STARTCODE_BYTE == (*ptr & 0xFC));
                    }
                    *is_normal_sc = normal_sc;

                    // at least 16-bit 0, may be GOB start code or
                    // resync marker.
                    *resync_marker = 1;
                }

                if (!(normal_sc | short_sc))
                {
                    phase = 0;
                }
                else
                {
                    /* Match for start code so update context with byte position */
                    *sc_end_pos = size;
                    phase = 3;
                }
                ptr++;
                size++;
                data_left--;
            }
        }
    }
    if ((data_left > 0) && (phase == 3))
    {
        (*sc_end_pos)++;
        phase++;
        ret = 1;
    }

    // Return 1 only if phase is 4, else always return 0
    return ret;
}


uint32 vbp_macroblock_number_length_mp42(uint32 numOfMbs)
{
    uint32 length = 0;
    numOfMbs--;
    do
    {
        numOfMbs >>= 1;
        length++;
    }
    while (numOfMbs);
    return length;
}

uint32 vbp_parse_video_packet_header_mp42(
    void *parent,
    viddec_mp4_parser_t *parser_cxt,
    uint16_t *quant_scale,
    uint32 *macroblock_number)
{
    uint32 ret = VBP_DATA;
    mp4_Info_t *pInfo = &(parser_cxt->info);
    mp4_VideoObjectLayer_t *vidObjLay = &(pInfo->VisualObject.VideoObject);
    mp4_VideoObjectPlane_t *vidObjPlane =
            &(pInfo->VisualObject.VideoObject.VideoObjectPlane);

    uint32 code = 0;
    int32_t getbits = 0;

    uint16_t _quant_scale = 0;
    uint32 _macroblock_number = 0;
    uint32 header_extension_codes = 0;
    uint8 vop_coding_type = vidObjPlane->vop_coding_type;

    if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_RECTANGULAR)
    {
        return VBP_DATA;
    }

    do
    {
        // get macroblock_number
        uint16_t mbs_x = (vidObjLay->video_object_layer_width + 15) >> 4;
        uint16_t mbs_y = (vidObjLay->video_object_layer_height + 15) >> 4;
        uint32 length = vbp_macroblock_number_length_mp42(mbs_x * mbs_y);

        getbits = viddec_pm_get_bits(parent, &code, length);
        BREAK_GETBITS_FAIL(getbits, ret);

        _macroblock_number = code;

        // quant_scale
        if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY)
        {
            getbits = viddec_pm_get_bits(parent, &code, vidObjLay->quant_precision);
            BREAK_GETBITS_FAIL(getbits, ret);
            _quant_scale = code;
        }

        // header_extension_codes
        if (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_RECTANGULAR)
        {
            getbits = viddec_pm_get_bits(parent, &code, 1);
            BREAK_GETBITS_FAIL(getbits, ret);
            header_extension_codes = code;
        }

        if (header_extension_codes)
        {
            // modulo time base
            do
            {
                getbits = viddec_pm_get_bits(parent, &code, 1);
                BREAK_GETBITS_FAIL(getbits, ret);
            } while (code);

            // marker_bit
            getbits = viddec_pm_get_bits(parent, &code, 1);
            BREAK_GETBITS_FAIL(getbits, ret);

            // vop_time_increment
            uint32 numbits = 0;
            numbits = vidObjLay->vop_time_increment_resolution_bits;
            if (numbits == 0)
            {
                // ??
                numbits = 1;
            }
            getbits = viddec_pm_get_bits(parent, &code, numbits);
            BREAK_GETBITS_FAIL(getbits, ret);
            vidObjPlane->vop_time_increment = code;


            // marker_bit
            getbits = viddec_pm_get_bits(parent, &code, 1);
            BREAK_GETBITS_FAIL(getbits, ret);

            // vop_coding_type
            getbits = viddec_pm_get_bits(parent, &code, 2);
            BREAK_GETBITS_FAIL(getbits, ret);

            vop_coding_type = code & 0x3;
            vidObjPlane->vop_coding_type = vop_coding_type;


            if (vidObjLay->video_object_layer_shape != MP4_SHAPE_TYPE_BINARYONLY)
            {
                // intra_dc_vlc_thr
                getbits = viddec_pm_get_bits(parent, &code, 3);
                BREAK_GETBITS_FAIL(getbits, ret);

                vidObjPlane->intra_dc_vlc_thr = code;
                if ((vidObjLay->sprite_enable == MP4_SPRITE_GMC) &&
                    (vop_coding_type == MP4_VOP_TYPE_S) &&
                    (vidObjLay->sprite_info.no_of_sprite_warping_points> 0))
                {
                    if (vbp_sprite_trajectory_mp42(parent, vidObjLay, vidObjPlane) != VBP_OK)
                    {
                        break;
                    }
                }

                if (vidObjLay->reduced_resolution_vop_enable &&
                   (vidObjLay->video_object_layer_shape == MP4_SHAPE_TYPE_RECTANGULAR) &&
                   ((vop_coding_type == MP4_VOP_TYPE_I) ||
                    (vop_coding_type == MP4_VOP_TYPE_P)))
                {
                    // vop_reduced_resolution
                    getbits = viddec_pm_get_bits(parent, &code, 1);
                    BREAK_GETBITS_FAIL(getbits, ret);
                }

                if (vop_coding_type != MP4_VOP_TYPE_I)
                {
                    // vop_fcode_forward
                    getbits = viddec_pm_get_bits(parent, &code, 3);
                    BREAK_GETBITS_FAIL(getbits, ret);
                    vidObjPlane->vop_fcode_forward = code;
                }

                if (vop_coding_type == MP4_VOP_TYPE_B)
                {
                    // vop_fcode_backward
                    getbits = viddec_pm_get_bits(parent, &code, 3);
                    BREAK_GETBITS_FAIL(getbits, ret);
                    vidObjPlane->vop_fcode_backward = code;
                }
            }
        }

        if (vidObjLay->newpred_enable)
        {
            // New pred mode not supported in HW, but, does libva support this?
            ret = VBP_DATA;
            break;
        }

        *quant_scale = _quant_scale;
        *macroblock_number = _macroblock_number;

        ret = VBP_OK;
    }
    while (0);
    return ret;
}

uint32 vbp_resync_marker_Length_mp42(viddec_mp4_parser_t *parser_cxt)
{
    mp4_Info_t *pInfo = &(parser_cxt->info);
    mp4_VideoObjectPlane_t *vidObjPlane =
            &(pInfo->VisualObject.VideoObject.VideoObjectPlane);

    uint32 resync_marker_length = 0;
    if (vidObjPlane->vop_coding_type == MP4_VOP_TYPE_I)
    {
        resync_marker_length = 17;
    }
    else if (vidObjPlane->vop_coding_type == MP4_VOP_TYPE_B)
    {
        uint8 fcode_max = vidObjPlane->vop_fcode_forward;
        if (fcode_max < vidObjPlane->vop_fcode_backward)
        {
            fcode_max = vidObjPlane->vop_fcode_backward;
        }
        resync_marker_length = 16 + fcode_max;

        // resync_marker is max(15+fcode,17) zeros followed by a one
        if (resync_marker_length < 18)
            resync_marker_length = 18;
    }
    else
    {
        resync_marker_length = 16 + vidObjPlane->vop_fcode_forward;
    }
    return resync_marker_length;
}

uint32 vbp_process_slices_svh_mp42(vbp_context *pcontext, int list_index)
{
    uint32 ret = VBP_OK;

    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    viddec_pm_cxt_t *parent = pcontext->parser_cxt;
    viddec_mp4_parser_t *parser_cxt =
            (viddec_mp4_parser_t *) &(parent->codec_data[0]);

    vbp_picture_data_mp42 *picture_data = vbp_get_mp42_picture_data(query_data);
    vbp_slice_data_mp42 *slice_data = &(picture_data->slice_data);
    VASliceParameterBufferMPEG4* slice_param = &(slice_data->slice_param);

    uint8 is_emul = 0;
    uint32 bit_offset = 0;
    uint32 byte_offset = 0;

    // The offsets are relative to parent->parse_cubby.buf
    viddec_pm_get_au_pos(parent, &bit_offset, &byte_offset, &is_emul);

    slice_data->buffer_addr = parent->parse_cubby.buf;

    slice_data->slice_offset =
            byte_offset + parent->list.data[list_index].stpos;
    slice_data->slice_size =
            parent->list.data[list_index].edpos - parent->list.data[list_index].stpos - byte_offset;

    slice_param->slice_data_size = slice_data->slice_size;
    slice_param->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    slice_param->slice_data_offset = 0;
    slice_param->macroblock_offset = bit_offset;
    slice_param->macroblock_number = 0;
    slice_param->quant_scale
            = parser_cxt->info.VisualObject.VideoObject.VideoObjectPlaneH263.vop_quant;

    return ret;
}
#define SEARCH_SYNC_OPT
uint32 vbp_process_slices_mp42(vbp_context *pcontext, int list_index)
{
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    viddec_pm_cxt_t *parent = pcontext->parser_cxt;
    viddec_mp4_parser_t *parser_cxt = (viddec_mp4_parser_t *) &(parent->codec_data[0]);

    vbp_picture_data_mp42 *picture_data = NULL;
    vbp_slice_data_mp42 *slice_data = NULL;
    VASliceParameterBufferMPEG4* slice_param = NULL;

    uint32 ret = VBP_OK;

    uint8 is_emul = 0;
    uint32 bit_offset = 0;
    uint32 byte_offset = 0;

    uint32 code = 0;
    int32_t getbits = 0;
    uint32 resync_marker_length = 0;

    /* The offsets are relative to parent->parse_cubby.buf */
    viddec_pm_get_au_pos(parent, &bit_offset, &byte_offset, &is_emul);

    picture_data = vbp_get_mp42_picture_data(query_data);
    slice_data = &(picture_data->slice_data);
    slice_param = &(slice_data->slice_param);

    slice_data->buffer_addr = parent->parse_cubby.buf;

    slice_data->slice_offset = byte_offset + parent->list.data[list_index].stpos;
    slice_data->slice_size =
            parent->list.data[list_index].edpos - parent->list.data[list_index].stpos - byte_offset;

    slice_param->slice_data_size = slice_data->slice_size;
    slice_param->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
    slice_param->slice_data_offset = 0;
    slice_param->macroblock_offset = bit_offset;
    slice_param->macroblock_number = 0;
    slice_param->quant_scale
            = parser_cxt->info.VisualObject.VideoObject.VideoObjectPlane.vop_quant;

    if (parser_cxt->info.VisualObject.VideoObject.resync_marker_disable)
    {
        // no resync_marker
        return VBP_OK;
    }

    // scan for resync_marker
    viddec_pm_get_au_pos(parent, &bit_offset, &byte_offset, &is_emul);
    if (bit_offset)
    {
        // byte-aligned
        getbits = viddec_pm_get_bits(parent, &code, 8 - bit_offset);
        if (getbits == -1)
        {
            return VBP_DATA;
        }
    }

    // get resync_marker_length
    resync_marker_length = vbp_resync_marker_Length_mp42(parser_cxt);

    uint16_t quant_scale = 0;
    uint32 macroblock_number = 0;

    while (1)
    {
#ifndef SEARCH_SYNC_OPT
        getbits = viddec_pm_peek_bits(parent, &code, resync_marker_length);

        // return VBP_OK as resync_marker may not be present
        BREAK_GETBITS_FAIL(getbits, ret);

        if (code != 1)
        {
            getbits = viddec_pm_get_bits(parent, &code, 8);
            BREAK_GETBITS_FAIL(getbits, ret);
            continue;
        }
#else

        // read 3 bytes since resync_marker_length is between 17 bits and 23 bits
        if (parent->getbits.bstrm_buf.buf_index + 3 > parent->getbits.bstrm_buf.buf_end)
        {
            break;
        }

        code = parent->getbits.bstrm_buf.buf[parent->getbits.bstrm_buf.buf_index] << 16 |
                parent->getbits.bstrm_buf.buf[parent->getbits.bstrm_buf.buf_index+1] << 8 |
                parent->getbits.bstrm_buf.buf[parent->getbits.bstrm_buf.buf_index+2];

        if (code >> (24-resync_marker_length) != 1)
        {
            int byte0 = code & 0xff;
            int byte1 = (code >> 8) & 0xff;
            if (byte0 != 0)
            {
                parent->getbits.bstrm_buf.buf_index += 3;
            }
            else if (byte1 != 0)
            {
                parent->getbits.bstrm_buf.buf_index += 2;
            }
            else
            {
                parent->getbits.bstrm_buf.buf_index += 1;
            }
            continue;
        }
#endif
        // We found resync_marker
        viddec_pm_get_au_pos(parent, &bit_offset, &byte_offset, &is_emul);

        // update slice data as we found resync_marker
        slice_data->slice_size -=
                (parent->list.data[list_index].edpos - parent->list.data[list_index].stpos - byte_offset);
        slice_param->slice_data_size = slice_data->slice_size;

        // skip resync marker
        getbits = viddec_pm_get_bits(parent, &code, resync_marker_length);

        // return VBP_DATA, this should never happen!
        BREAK_GETBITS_FAIL(getbits, ret);

        // parse video_packet_header
        ret = vbp_parse_video_packet_header_mp42(parent, parser_cxt,
                &quant_scale, &macroblock_number);

        if (ret != VBP_OK)
        {
            ETRACE("Failed to parse video packet header.\n");
            return ret;
        }

        // new_picture_flag = 0, this is not the first slice of a picture
        vbp_fill_picture_param(pcontext, 0);

        picture_data = vbp_get_mp42_picture_data(query_data);
        slice_data = &(picture_data->slice_data);
        slice_param = &(slice_data->slice_param);


        viddec_pm_get_au_pos(parent, &bit_offset, &byte_offset, &is_emul);

        slice_data->buffer_addr = parent->parse_cubby.buf;

        slice_data->slice_offset =
                    byte_offset + parent->list.data[list_index].stpos;
        slice_data->slice_size =
                    parent->list.data[list_index].edpos - parent->list.data[list_index].stpos - byte_offset;

        slice_param->slice_data_size = slice_data->slice_size;
        slice_param->slice_data_flag = VA_SLICE_DATA_FLAG_ALL;
        slice_param->slice_data_offset = 0;
        slice_param->macroblock_offset = bit_offset;
        slice_param->macroblock_number = macroblock_number;
        slice_param->quant_scale = quant_scale;

        if (bit_offset)
        {
            // byte-align parsing position
            getbits = viddec_pm_skip_bits(parent,  8 - bit_offset);
            if (getbits == -1)
            {
                ETRACE("Failed to align parser to byte position.\n");
                return VBP_DATA;
            }
        }

    }

    return VBP_OK;
}

uint32 vbp_process_video_packet_mp42(vbp_context *pcontext)
{
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    viddec_pm_cxt_t *parent = pcontext->parser_cxt;
    viddec_mp4_parser_t *parser_cxt = (viddec_mp4_parser_t *) &(parent->codec_data[0]);
    uint32 code = 0;
    int32_t getbits = 0;

    uint32 ret = VBP_DATA;


    // setup bitstream parser
    parent->getbits.list = &(parent->list);

    parent->getbits.bstrm_buf.buf = parent->parse_cubby.buf;
    parent->getbits.bstrm_buf.buf_index = 0;
    parent->getbits.bstrm_buf.buf_st = 0;
    parent->getbits.bstrm_buf.buf_end = parent->parse_cubby.size;
    parent->getbits.bstrm_buf.buf_bitoff = 0;

    parent->getbits.au_pos = 0;
    parent->getbits.list_off = 0;
    parent->getbits.phase = 0;
    parent->getbits.emulation_byte_counter = 0;

    parent->list.start_offset = 0;
    parent->list.end_offset = parent->parse_cubby.size;
    parent->list.total_bytes = parent->parse_cubby.size;


    // skip leading zero-byte
    while (code == 0)
    {
        getbits = viddec_pm_get_bits(parent, &code, 8);
        BREAK_GETBITS_FAIL(getbits, ret);
        getbits = viddec_pm_peek_bits(parent, &code, 8);
        BREAK_GETBITS_FAIL(getbits, ret);
    }

    if (getbits != 0)
    {
        return VBP_DATA;
    }

    // resync-marker is represented as 17-23 bits. (16-22 bits of 0)
    // as 16-bit '0' has been skipped, we try to parse buffer bit by bit
    // until bit 1 is encounted or up to 7 bits are parsed.
    code = 0;
    uint8 count = 0;
    while (code == 0  && count < 7)
    {
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        count++;
    }

    if (code == 0 || getbits != 0)
    {
        ETRACE("no resync-marker in the buffer.\n");
        return ret;
    }

    // resync marker is skipped
    uint16_t quant_scale = 0;
    uint32 macroblock_number = 0;

    // parse video_packet_header
    vbp_parse_video_packet_header_mp42(parent, parser_cxt, &quant_scale, &macroblock_number);

    // new_picture_flag = 0, this is not the first slice of a picture
    vbp_fill_picture_param(pcontext, 0);

    vbp_picture_data_mp42 *picture_data = NULL;
    vbp_slice_data_mp42 *slice_data = NULL;
    VASliceParameterBufferMPEG4* slice_param = NULL;

    picture_data = vbp_get_mp42_picture_data(query_data);
    slice_data = &(picture_data->slice_data);
    slice_param = &(slice_data->slice_param);

    ret = vbp_process_slices_mp42(pcontext, 0);

    // update slice's QP and macro_block number as it is set to 0 by default.
    slice_param->macroblock_number = macroblock_number;
    slice_param->quant_scale = quant_scale;

    // VOP must be coded!
    picture_data->vop_coded = 1;
    return ret;

}


static inline uint32 vbp_sprite_dmv_length_mp42(
    void * parent,
    int32_t *dmv_length)
{
    uint32 code, skip;
    int32_t getbits = 0;
    uint32 ret = VBP_DATA;
    *dmv_length = 0;
    skip = 3;
    do
    {
        getbits = viddec_pm_peek_bits(parent, &code, skip);
        BREAK_GETBITS_FAIL(getbits, ret);

        if (code == 7)
        {
            viddec_pm_skip_bits(parent, skip);
            getbits = viddec_pm_peek_bits(parent, &code, 9);
            BREAK_GETBITS_FAIL(getbits, ret);

            skip = 1;
            while ((code & 256) != 0)
            {
                // count number of 1 bits
                code <<= 1;
                skip++;
            }
            *dmv_length = 5 + skip;
        }
        else
        {
            skip = (code <= 1) ? 2 : 3;
            *dmv_length = code - 1;
        }
        viddec_pm_skip_bits(parent, skip);
        ret = VBP_OK;

    }
    while (0);
    return ret;
}


static inline uint32 vbp_sprite_trajectory_mp42(
    void *parent,
    mp4_VideoObjectLayer_t *vidObjLay,
    mp4_VideoObjectPlane_t *vidObjPlane)
{
    uint32 code, i;
    int32_t dmv_length = 0, dmv_code = 0, getbits = 0;
    uint32 ret = VBP_OK;
    for (i = 0; i < (uint32) vidObjLay->sprite_info.no_of_sprite_warping_points; i++)
    {
        ret = VBP_DATA;
        ret = vbp_sprite_dmv_length_mp42(parent, &dmv_length);
        if (ret != VBP_OK)
        {
            break;
        }
        if (dmv_length <= 0)
        {
            dmv_code = 0;
        }
        else
        {
            getbits = viddec_pm_get_bits(parent, &code, (uint32) dmv_length);
            BREAK_GETBITS_FAIL(getbits, ret);
            dmv_code = (int32_t) code;
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
            {
                dmv_code -= (1 << dmv_length) - 1;
            }
        }
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        if (code != 1)
        {
            ret = VBP_DATA;
            break;
        }
        vidObjPlane->warping_mv_code_du[i] = dmv_code;
        // TODO: create another inline function to avoid code duplication
        ret = vbp_sprite_dmv_length_mp42(parent, &dmv_length);
        if (ret != VBP_OK)
        {
            break;
        }
        // reset return value in case early break
        ret = VBP_DATA;
        if (dmv_length <= 0)
        {
            dmv_code = 0;
        }
        else
        {
            getbits = viddec_pm_get_bits(parent, &code, (uint32) dmv_length);
            BREAK_GETBITS_FAIL(getbits, ret);
            dmv_code = (int32_t) code;
            if ((dmv_code & (1 << (dmv_length - 1))) == 0)
            {
                dmv_code -= (1 << dmv_length) - 1;
            }
        }
        getbits = viddec_pm_get_bits(parent, &code, 1);
        BREAK_GETBITS_FAIL(getbits, ret);
        if (code != 1)
        {
            break;
        }
        vidObjPlane->warping_mv_code_dv[i] = dmv_code;

        // set to VBP_OK
        ret = VBP_OK;

    }
    return ret;
}


/*
 * free memory of vbp_data_mp42 structure and its members
 */
uint32 vbp_free_query_data_mp42(vbp_context *pcontext)
{
    vbp_data_mp42 *query_data = (vbp_data_mp42 *) pcontext->query_data;
    vbp_picture_data_mp42* current = NULL;
    vbp_picture_data_mp42* next = NULL;

    if (pcontext->parser_private)
    {
        free(pcontext->parser_private);
        pcontext->parser_private = NULL;
    }
    if (query_data)
    {
        current = query_data->picture_data;
        while (current != NULL)
        {
            next = current->next_picture_data;
            free(current);
            current = next;
        }

        free(query_data);
    }

    pcontext->query_data = NULL;
    return VBP_OK;
}

/*
 * Allocate memory for vbp_data_mp42 structure and all its members.
 */
uint32 vbp_allocate_query_data_mp42(vbp_context *pcontext)
{
    vbp_data_mp42 *query_data;
    pcontext->query_data = NULL;

    query_data = vbp_malloc_set0(vbp_data_mp42, 1);
    if (query_data == NULL)
    {
        goto cleanup;
    }

    pcontext->query_data = (void *) query_data;
    query_data->picture_data = NULL;
    query_data->number_picture_data = 0;
    query_data->number_pictures = 0;

    pcontext->parser_private = NULL;
    vbp_mp42_parser_private *parser_private = NULL;

    parser_private = vbp_malloc_set0(vbp_mp42_parser_private, 1);
    if (NULL == parser_private)
    {
        goto cleanup;
    }

    /* assign the pointer */
    pcontext->parser_private = (void *)parser_private;

    /* init the pointer */
    parser_private->short_video_header = TRUE;
    return VBP_OK;

cleanup:

    vbp_free_query_data_mp42(pcontext);

    return VBP_MEM;
}
