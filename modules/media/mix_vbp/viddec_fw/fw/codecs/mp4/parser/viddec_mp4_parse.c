#include <string.h>

#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "viddec_mp4_parse.h"
#include "viddec_mp4_decodevideoobjectplane.h"
#include "viddec_mp4_shortheader.h"
#include "viddec_mp4_videoobjectlayer.h"
#include "viddec_mp4_videoobjectplane.h"
#include "viddec_mp4_visualobject.h"

#ifndef VBP
extern uint32_t viddec_parse_sc_mp4(void *in, void *pcxt, void *sc_state);
#endif

void viddec_mp4_get_context_size(viddec_parser_memory_sizes_t *size)
{
    /* Should return size of my structure */
    size->context_size = sizeof(viddec_mp4_parser_t);
    size->persist_size = 0;
    return;
} // viddec_mp4_get_context_size

#ifndef VBP
uint32_t viddec_mp4_wkld_done(void *parent, void *ctxt, uint32_t next_sc, uint32_t *codec_specific_errors)
{
    viddec_mp4_parser_t *parser = (viddec_mp4_parser_t *) ctxt;
    int result = VIDDEC_PARSE_SUCESS;
    uint8_t frame_boundary = false;
    uint8_t emit_workload = false;

    //DEB("entering is_wkld_done: next_sc: 0x%x, sc_seen: %d\n", next_sc, parser->sc_seen);

    parent = parent;

    // VS, VO, VOL, VOP or GVOP start codes indicate frame boundary.
    frame_boundary = (  (MP4_SC_VISUAL_OBJECT_SEQUENCE == next_sc) ||
                        (MP4_SC_VISUAL_OBJECT == next_sc) ||
                        ((MP4_SC_VIDEO_OBJECT_LAYER_MIN <= next_sc) && (next_sc <= MP4_SC_VIDEO_OBJECT_LAYER_MAX)) ||
                        (next_sc <= MP4_SC_VIDEO_OBJECT_MAX) ||
                        (MP4_SC_VIDEO_OBJECT_PLANE == next_sc) ||
                        ((SHORT_THIRD_STARTCODE_BYTE & 0xFC) == (next_sc & 0xFC)) ||
                        (MP4_SC_GROUP_OF_VOP == next_sc)    );

    // Mark workload is ready to be emitted based on the start codes seen.
    if (frame_boundary)
    {
        uint8_t vol_error_found = false, frame_complete = false;

        // Frame is considered complete and without errors, if a VOL was received since startup and
        // if a VOP was received for this workload (or) if short video header is found.
        frame_complete = ( ((parser->sc_seen & MP4_SC_SEEN_VOL) && (parser->sc_seen & MP4_SC_SEEN_VOP)) ||
                           (parser->sc_seen & MP4_SC_SEEN_SVH) );

        // For non SVH case, the video object layer data should be followed by video object plane data
        // If not, error occurred and we need to throw the current workload as error.
        vol_error_found = ( (parser->prev_sc == MP4_SC_VIDEO_OBJECT_LAYER_MIN) &&
                            !(MP4_SC_VIDEO_OBJECT_PLANE == next_sc) );

        emit_workload = (frame_complete || vol_error_found);

        //DEB("emit workload: frame_complete: %d, vol_error_found %d\n", frame_complete, vol_error_found);
    }

    // EOS and discontinuity should force workload completion.
    emit_workload |= ((VIDDEC_PARSE_EOS == next_sc) || (VIDDEC_PARSE_DISCONTINUITY == next_sc));

    if (emit_workload)
    {
        *codec_specific_errors = 0;

        // If the frame is not complete but we have received force frame complete because of EOS or
        // discontinuity, we mark the workload as not decodeable.
        if (!((parser->sc_seen & MP4_SC_SEEN_VOL) && (parser->sc_seen & MP4_SC_SEEN_VOP)) && !(parser->sc_seen & MP4_SC_SEEN_SVH))
            *codec_specific_errors |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;

        /*
        This is the strategy for error detection.
        Errors in any field needed by the firmware (parser/decoder) are treated as non-decodable.
        Errors in other fields will be considered decodable.
        Defaults/alternate strategies will be considered on a case-by-case basis as customer content is seen.

        ERROR_TYPE      |       PARSING         |   INVALID/UNSUPPORTED |       BS = Bitstream error
        -----------------------------------------------------------------       UNSUP = Un-supported
        DFLT_PRESENT    |       YES |   NO      |       YES |   NO      |       ND = Non-decodable
        COMPONENT USED  |           |           |           |           |       DFLT = Populate defaults
        -----------------------------------------------------------------
        FIRMWARE        | BS+ND     | BS+ND     | UNSUP+ND  | UNSUP+ND  |
        DRIVER/USER     | BS+DFLT   | BS        | UNSUP     | UNSUP     |
        NONE            | BS        | BS        | UNSUP     | UNSUP     |
                        |           |           | Continue Parsing      |
        */
        if ((parser->bitstream_error & MP4_BS_ERROR_HDR_NONDEC) || (parser->bitstream_error & MP4_BS_ERROR_FRM_NONDEC))
            *codec_specific_errors |= (VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE | VIDDEC_FW_WORKLOAD_ERR_MISSING_DMEM);

        if ((parser->bitstream_error & MP4_BS_ERROR_HDR_UNSUP) || (parser->bitstream_error & MP4_BS_ERROR_FRM_UNSUP))
            *codec_specific_errors |= VIDDEC_FW_WORKLOAD_ERR_UNSUPPORTED;

        if ((parser->bitstream_error & MP4_BS_ERROR_HDR_PARSE) || (parser->bitstream_error & MP4_BS_ERROR_FRM_PARSE))
            *codec_specific_errors |= VIDDEC_FW_WORKLOAD_ERR_BITSTREAM_ERROR;

        parser->bitstream_error &= MP4_HDR_ERROR_MASK;
        parser->sc_seen &= MP4_SC_SEEN_VOL;
        result = VIDDEC_PARSE_FRMDONE;
    }
    //DEB("exiting is_wkld_done: next_sc: 0x%x, sc_seen: %d, err: %d, fr_bnd:%d, force:%d\n",
    //        next_sc, parser->sc_seen, *codec_specific_errors, frame_boundary, force_frame_complete);

    return result;
} // viddec_mp4_wkld_done
#endif

void viddec_mp4_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
{
    viddec_mp4_parser_t *parser = (viddec_mp4_parser_t *) ctxt;

    persist_mem = persist_mem;
    parser->is_frame_start = false;
    parser->prev_sc = MP4_SC_INVALID;
    parser->current_sc = MP4_SC_INVALID;
    parser->cur_sc_prefix = false;
    parser->next_sc_prefix = false;
    parser->ignore_scs = false;

    if (preserve)
    {
        // Need to maintain information till VOL
        parser->sc_seen &= MP4_SC_SEEN_VOL;
        parser->bitstream_error &= MP4_HDR_ERROR_MASK;

        // Reset only frame related data
        memset(&(parser->info.VisualObject.VideoObject.VideoObjectPlane), 0, sizeof(mp4_VideoObjectPlane_t));
        memset(&(parser->info.VisualObject.VideoObject.VideoObjectPlaneH263), 0, sizeof(mp4_VideoObjectPlaneH263));
    }
    else
    {
        parser->sc_seen = MP4_SC_SEEN_INVALID;
        parser->bitstream_error = MP4_BS_ERROR_NONE;
        memset(&(parser->info), 0, sizeof(mp4_Info_t));
    }

    return;
} // viddec_mp4_init

static uint32_t viddec_mp4_decodevop_and_emitwkld(void *parent, void *ctxt)
{
    int status = MP4_STATUS_OK;
    viddec_mp4_parser_t *cxt = (viddec_mp4_parser_t *)ctxt;

    status = mp4_DecodeVideoObjectPlane(&(cxt->info));

#ifndef VBP
    status = viddec_fw_mp4_emit_workload(parent, ctxt);
#endif

    return status;
} // viddec_mp4_decodevop_and_emitwkld

uint32_t viddec_mp4_parse(void *parent, void *ctxt)
{
    uint32_t sc=0;
    viddec_mp4_parser_t *cxt;
    uint8_t is_svh=0;
    int32_t getbits=0;
    int32_t status = 0;

    cxt = (viddec_mp4_parser_t *)ctxt;
    is_svh = (cxt->cur_sc_prefix) ? false: true;
    if ((getbits = viddec_pm_peek_bits(parent, &sc, 32)) == -1)
    {
        DEB("Start code not found\n");
        return VIDDEC_PARSE_ERROR;
    }

    if (!is_svh)
    {
        viddec_pm_get_bits(parent, &sc, 32);
        sc = sc & 0xFF;
        cxt->current_sc = sc;
        cxt->current_sc |= 0x100;
        DEB("current_sc=0x%.8X, prev_sc=0x%x\n", sc, cxt->prev_sc);

        switch (sc)
        {
        case MP4_SC_VISUAL_OBJECT_SEQUENCE:
        {
            status = mp4_Parse_VisualSequence(parent, cxt);
            cxt->prev_sc = MP4_SC_VISUAL_OBJECT_SEQUENCE;
            DEB("MP4_VISUAL_OBJECT_SEQUENCE_SC: \n");
            break;
        }
        case MP4_SC_VISUAL_OBJECT_SEQUENCE_EC:
        {/* Not required to do anything */
            break;
        }
        case MP4_SC_USER_DATA:
        {   /* Copy userdata to user-visible buffer (EMIT) */
            status = mp4_Parse_UserData(parent, cxt);
            DEB("MP4_USER_DATA_SC: \n");
            break;
        }
        case MP4_SC_GROUP_OF_VOP:
        {
            status = mp4_Parse_GroupOfVideoObjectPlane(parent, cxt);
            cxt->prev_sc = MP4_SC_GROUP_OF_VOP;
            DEB("MP4_GROUP_OF_VOP_SC:0x%.8X\n", status);
            break;
        }
        case MP4_SC_VIDEO_SESSION_ERROR:
        {/* Not required to do anything?? */
            break;
        }
        case MP4_SC_VISUAL_OBJECT:
        {
            status = mp4_Parse_VisualObject(parent, cxt);
            cxt->prev_sc = MP4_SC_VISUAL_OBJECT;
            DEB("MP4_VISUAL_OBJECT_SC: status=%.8X\n", status);
            break;
        }
        case MP4_SC_VIDEO_OBJECT_PLANE:
        {
            /* We must decode the VOP Header information, it does not end  on a byte boundary, so we need to emit
               a starting bit offset after parsing the header. */
            status = mp4_Parse_VideoObjectPlane(parent, cxt);
            status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
            // TODO: Fix this for interlaced
            cxt->is_frame_start = true;
            cxt->sc_seen |= MP4_SC_SEEN_VOP;

            DEB("MP4_VIDEO_OBJECT_PLANE_SC: status=0x%.8X\n", status);
            break;
        }
        case MP4_SC_STUFFING:
        {
            break;
        }
        default:
        {
            if ( (sc >=  MP4_SC_VIDEO_OBJECT_LAYER_MIN) && (sc <=  MP4_SC_VIDEO_OBJECT_LAYER_MAX) )
            {
                status = mp4_Parse_VideoObjectLayer(parent, cxt);
                cxt->sc_seen = MP4_SC_SEEN_VOL;
                cxt->prev_sc = MP4_SC_VIDEO_OBJECT_LAYER_MIN;
                DEB("MP4_VIDEO_OBJECT_LAYER_MIN_SC:status=0x%.8X\n", status);
                sc = MP4_SC_VIDEO_OBJECT_LAYER_MIN;
            }
            // sc is unsigned and will be >= 0, so no check needed for sc >= MP4_SC_VIDEO_OBJECT_MIN
            else if (sc <= MP4_SC_VIDEO_OBJECT_MAX)
            {
                // If there is more data, it is short video header, else the next start code is expected to be VideoObjectLayer
                getbits = viddec_pm_get_bits(parent, &sc, 22);
                if (getbits != -1)
                {
                    cxt->current_sc = sc;
                    status = mp4_Parse_VideoObject_svh(parent, cxt);
                    status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
                    cxt->sc_seen = MP4_SC_SEEN_SVH;
                    cxt->is_frame_start = true;
                    DEB("MP4_SCS_SVH: status=0x%.8X 0x%.8X %.8X\n", status, cxt->current_sc, sc);
                    DEB("MP4_VIDEO_OBJECT_MIN_SC:status=0x%.8X\n", status);
                }
            }
            else
            {
                DEB("UNKWON Cod:0x%08X\n", sc);
            }
        }
        break;
        }
    }
    else
    {
        viddec_pm_get_bits(parent, &sc, 22);
        cxt->current_sc = sc;
        DEB("current_sc=0x%.8X, prev_sc=0x%x\n", sc, cxt->prev_sc);
        status = mp4_Parse_VideoObject_svh(parent, cxt);
        status = viddec_mp4_decodevop_and_emitwkld(parent, cxt);
        cxt->sc_seen = MP4_SC_SEEN_SVH;
        cxt->is_frame_start = true;
        DEB("SVH: MP4_SCS_SVH: status=0x%.8X 0x%.8X %.8X\n", status, cxt->current_sc, sc);
    }

    // Current sc becomes the previous sc
    cxt->prev_sc = sc;

    return VIDDEC_PARSE_SUCESS;
} // viddec_mp4_parse

#ifndef VBP
uint32_t viddec_mp4_is_frame_start(void *ctxt)
{
    viddec_mp4_parser_t *parser = (viddec_mp4_parser_t *)ctxt;
    return parser->is_frame_start;
} // viddec_mp4_is_frame_start

void viddec_mp4_get_ops(viddec_parser_ops_t *ops)
{
    ops->parse_syntax = viddec_mp4_parse;
    ops->get_cxt_size = viddec_mp4_get_context_size;
    ops->is_wkld_done = viddec_mp4_wkld_done;
    ops->parse_sc = viddec_parse_sc_mp4;
    ops->is_frame_start = viddec_mp4_is_frame_start;
    ops->init = viddec_mp4_init;
    return;
} // viddec_mp4_get_ops
#endif
