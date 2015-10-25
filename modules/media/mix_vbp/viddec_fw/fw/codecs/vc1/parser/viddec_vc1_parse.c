#include "viddec_fw_debug.h"    // For DEB
#include "viddec_parser_ops.h"  // For parser helper functions
#include "vc1.h"                // For the parser structure
#include "vc1parse.h"           // For vc1 parser helper functions
#ifdef VBP
#include "viddec_pm.h"
#endif
#define vc1_is_frame_start_code( ch )                                   \
    (( vc1_SCField == ch ||vc1_SCSlice == ch || vc1_SCFrameHeader == ch ) ? 1 : 0)

/* init function */
#ifdef VBP
void viddec_vc1_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
#else
static void viddec_vc1_init(void *ctxt, uint32_t *persist_mem, uint32_t preserve)
#endif
{
    vc1_viddec_parser_t *parser = ctxt;
    int i;

    persist_mem = persist_mem;

    for (i=0; i<VC1_NUM_REFERENCE_FRAMES; i++)
    {
        parser->ref_frame[i].id   = -1; /* first I frame checks that value */
        parser->ref_frame[i].anchor[0] = 1;
        parser->ref_frame[i].anchor[1] = 1;
        parser->ref_frame[i].intcomp_top = 0;
        parser->ref_frame[i].intcomp_bot = 0;
        parser->ref_frame[i].tff=0;
    }

    parser->intcomp_top[0] = 0;
    parser->intcomp_bot[0] = 0;
    parser->intcomp_top[1] = 0;
    parser->intcomp_bot[1] = 0;
    parser->is_reference_picture = false;

    memset(&parser->info.picLayerHeader, 0, sizeof(vc1_PictureLayerHeader));

    if (preserve)
    {
        parser->sc_seen &= VC1_EP_MASK;
        parser->sc_seen_since_last_wkld &= VC1_EP_MASK;
    }
    else
    {
        parser->sc_seen = VC1_SC_INVALID;
        parser->sc_seen_since_last_wkld = VC1_SC_INVALID;
        memset(&parser->info.metadata, 0, sizeof(parser->info.metadata));
    }

    return;
} // viddec_vc1_init

static void vc1_swap_intcomp(vc1_viddec_parser_t *parser)
{
    parser->intcomp_top[1] = parser->intcomp_top[0];
    parser->intcomp_bot[1] = parser->intcomp_bot[0];
    parser->intcomp_top[0] = 0;
    parser->intcomp_bot[0] = 0;

    return;
} // vc1_swap_intcomp

#ifdef VBP
uint32_t viddec_vc1_parse(void *parent, void *ctxt)
#else
static uint32_t viddec_vc1_parse(void *parent, void *ctxt)
#endif
{
    vc1_viddec_parser_t *parser = ctxt;
    uint32_t sc=0x0;
    int32_t ret=0, status=0;

#ifdef VBP
    /* This works only if there is one slice and no start codes */
    /* A better fix would be to insert start codes it there aren't any. */
    ret = viddec_pm_peek_bits(parent, &sc, 32);
    if ((sc > 0x0100) && (sc < 0x0200)) /* a Start code will be in this range. */
    {
        ret = viddec_pm_get_bits(parent, &sc, 32);
    }
    else
    {
        /* In cases where we get a buffer with no start codes, we assume */
        /* that this is a frame of data. We may have to fix this later. */
        sc = vc1_SCFrameHeader;
    }
#else
    ret = viddec_pm_get_bits(parent, &sc, 32);
#endif
    sc = sc & 0xFF;
    parser->is_frame_start = 0;
    parser->is_second_start = 0;
    DEB("START_CODE = %02x\n", sc);
    switch ( sc )
    {
    case vc1_SCSequenceHeader:
    {
        uint32_t data;
        parser->ref_frame[0].anchor[0] = 1;
        parser->ref_frame[0].anchor[1] = 1;
        parser->ref_frame[1].anchor[0] = 1;
        parser->ref_frame[1].anchor[1] = 1;
        memset( &parser->info.metadata, 0, sizeof(parser->info.metadata));
        /* look if we have a rcv header for main or simple profile */
        ret = viddec_pm_peek_bits(parent,&data ,2);

        if (data == 3)
        {
            status = vc1_ParseSequenceLayer(parent, &parser->info);
        }
        else
        {
            status = vc1_ParseRCVSequenceLayer(parent, &parser->info);
        }
        parser->sc_seen = VC1_SC_SEQ;
        parser->sc_seen_since_last_wkld |= VC1_SC_SEQ;
#ifdef VBP
        parser->start_code = VC1_SC_SEQ;
        if (parser->info.metadata.HRD_NUM_LEAKY_BUCKETS == 0)
        {
            if (parser->info.metadata.PROFILE == VC1_PROFILE_SIMPLE)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 96000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 384000;
                    break;
                }
            }
            else if (parser->info.metadata.PROFILE == VC1_PROFILE_MAIN)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 2000000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 10000000;
                    break;
                case 2:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 20000000;
                    break;
                }
            }
            else if (parser->info.metadata.PROFILE == VC1_PROFILE_ADVANCED)
            {
                switch(parser->info.metadata.LEVEL)
                {
                case 0:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 2000000;
                    break;
                case 1:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 10000000;
                    break;
                case 2:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 20000000;
                    break;
                case 3:
                    parser->info.metadata.hrd_initial_state.sLeakyBucket[0].HRD_RATE = 45000000;
                    break;
                }
            }
        }

#endif
        break;
    }

    case vc1_SCEntryPointHeader:
    {
        status = vc1_ParseEntryPointLayer(parent, &parser->info);
        parser->sc_seen |= VC1_SC_EP;
        // Clear all bits indicating data below ep header
        parser->sc_seen &= VC1_EP_MASK;
        parser->sc_seen_since_last_wkld |= VC1_SC_EP;
#ifdef VBP
        parser->start_code = VC1_SC_EP;
#endif
        break;
    }

    case vc1_SCFrameHeader:
    {
        memset(&parser->info.picLayerHeader, 0, sizeof(vc1_PictureLayerHeader));
        status = vc1_ParsePictureLayer(parent, &parser->info);
        if ((parser->info.picLayerHeader.PTypeField1 == VC1_I_FRAME) ||
                (parser->info.picLayerHeader.PTypeField1 == VC1_P_FRAME) ||
                (parser->info.picLayerHeader.PTYPE == VC1_I_FRAME) ||
                (parser->info.picLayerHeader.PTYPE == VC1_P_FRAME))
        {
            vc1_swap_intcomp(parser);
        }
        parser->sc_seen |= VC1_SC_FRM;
        // Clear all bits indicating data below frm header
        parser->sc_seen &= VC1_FRM_MASK;
        parser->sc_seen_since_last_wkld |= VC1_SC_FRM;
        //vc1_start_new_frame ( parent, parser );

        parser->is_frame_start = 1;
        vc1_parse_emit_frame_start( parent, parser );
#ifdef VBP
        parser->start_code = VC1_SC_FRM;
#endif
        break;
    }

    case vc1_SCSlice:
    {
        status = vc1_ParseSliceLayer(parent, &parser->info);
        parser->sc_seen_since_last_wkld |= VC1_SC_SLC;

        vc1_parse_emit_current_slice( parent, parser );

#ifdef VBP
        parser->start_code = VC1_SC_SLC;
#endif
        break;
    }

    case vc1_SCField:
    {
        parser->info.picLayerHeader.SLICE_ADDR = 0;
        parser->info.picLayerHeader.CurrField = 1;
        parser->info.picLayerHeader.REFFIELD = 0;
        parser->info.picLayerHeader.NUMREF = 0;
        parser->info.picLayerHeader.MBMODETAB = 0;
        parser->info.picLayerHeader.MV4SWITCH = 0;
        parser->info.picLayerHeader.DMVRANGE = 0;
        parser->info.picLayerHeader.MVTAB = 0;
        parser->info.picLayerHeader.MVMODE = 0;
        parser->info.picLayerHeader.MVRANGE = 0;
#ifdef VBP
        parser->info.picLayerHeader.raw_MVTYPEMB = 0;
        parser->info.picLayerHeader.raw_DIRECTMB = 0;
        parser->info.picLayerHeader.raw_SKIPMB = 0;
        parser->info.picLayerHeader.raw_ACPRED = 0;
        parser->info.picLayerHeader.raw_FIELDTX = 0;
        parser->info.picLayerHeader.raw_OVERFLAGS = 0;
        parser->info.picLayerHeader.raw_FORWARDMB = 0;

        memset(&(parser->info.picLayerHeader.MVTYPEMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.DIRECTMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.SKIPMB), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.ACPRED), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.FIELDTX), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.OVERFLAGS), 0, sizeof(vc1_Bitplane));
        memset(&(parser->info.picLayerHeader.FORWARDMB), 0, sizeof(vc1_Bitplane));

        parser->info.picLayerHeader.ALTPQUANT = 0;
        parser->info.picLayerHeader.DQDBEDGE = 0;
#endif

        status = vc1_ParseFieldLayer(parent, &parser->info);
        if ((parser->info.picLayerHeader.PTypeField2 == VC1_I_FRAME) ||
                (parser->info.picLayerHeader.PTypeField2 == VC1_P_FRAME))
        {
            //vc1_swap_intcomp(parser);
        }
        parser->sc_seen |= VC1_SC_FLD;
        parser->sc_seen_since_last_wkld |= VC1_SC_FLD;

        parser->is_second_start = 1;
        vc1_parse_emit_second_field_start( parent, parser );
#ifdef VBP
        parser->start_code = VC1_SC_FLD;
#endif
        break;
    }

    case vc1_SCSequenceUser:
    case vc1_SCEntryPointUser:
    case vc1_SCFrameUser:
    case vc1_SCSliceUser:
    case vc1_SCFieldUser:
    {/* Handle user data */
        status = vc1_ParseAndAppendUserData(parent, sc); //parse and add items
        parser->sc_seen_since_last_wkld |= VC1_SC_UD;
#ifdef VBP
        parser->start_code = VC1_SC_UD;
#endif
        break;
    }

    case vc1_SCEndOfSequence:
    {
        parser->sc_seen = VC1_SC_INVALID;
        parser->sc_seen_since_last_wkld |= VC1_SC_INVALID;
#ifdef VBP
        parser->start_code = VC1_SC_INVALID;
#endif
        break;
    }
    default: /* Any other SC that is not handled */
    {
        DEB("SC = %02x - unhandled\n", sc );
#ifdef VBP
        parser->start_code = VC1_SC_INVALID;
#endif
        break;
    }
    }



    return VIDDEC_PARSE_SUCESS;
} // viddec_vc1_parse

/**
   If a picture header was seen and the next start code is a sequence header, entrypoint header,
   end of sequence or another frame header, this api returns frame done.
   If a sequence header and a frame header was not seen before this point, all the
   information needed for decode is not present and parser errors are reported.
*/
#ifdef VBP
uint32_t viddec_vc1_wkld_done(void *parent, void *ctxt, unsigned int next_sc, uint32_t *codec_specific_errors)
#else
static uint32_t viddec_vc1_wkld_done(void *parent, void *ctxt, unsigned int next_sc, uint32_t *codec_specific_errors)
#endif
{
    vc1_viddec_parser_t *parser = ctxt;
    int ret = VIDDEC_PARSE_SUCESS;
    parent = parent;
    switch (next_sc)
    {
    case vc1_SCFrameHeader:
        if (((parser->sc_seen_since_last_wkld & VC1_SC_EP) ||
                (parser->sc_seen_since_last_wkld & VC1_SC_SEQ)) &&
                (!(parser->sc_seen_since_last_wkld & VC1_SC_FRM)))
        {
            break;
        }
        // Deliberate fall-thru case
    case vc1_SCEntryPointHeader:
        if ((next_sc == vc1_SCEntryPointHeader) &&
                (parser->sc_seen_since_last_wkld & VC1_SC_SEQ) &&
                (!(parser->sc_seen_since_last_wkld & VC1_SC_EP)))
        {
            break;
        }
        // Deliberate fall-thru case
    case vc1_SCSequenceHeader:
    case vc1_SCEndOfSequence:
    case VIDDEC_PARSE_EOS:
    case VIDDEC_PARSE_DISCONTINUITY:
        ret = VIDDEC_PARSE_FRMDONE;
        // Set errors for progressive
        if ((parser->sc_seen & VC1_SC_SEQ) && (parser->sc_seen & VC1_SC_FRM))
            *codec_specific_errors = 0;
        else
            *codec_specific_errors |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
        vc1_end_frame(parser);
        parser->sc_seen_since_last_wkld = VC1_SC_INVALID;
        // TODO: Need to check for interlaced
        break;
    default:
        ret = VIDDEC_PARSE_SUCESS;
        break;
    } //switch
    DEB("sc: 0x%x, sc_seen: 0x%x, sc_since_last_wkld:%d, error:%d, ret: %d\n",
        next_sc, parser->sc_seen, parser->sc_seen_since_last_wkld,
        *codec_specific_errors, ret);

    return ret;
} // viddec_vc1_wkld_done

#ifdef VBP
void viddec_vc1_get_context_size(viddec_parser_memory_sizes_t *size)
#else
static void viddec_vc1_get_context_size(viddec_parser_memory_sizes_t *size)
#endif
{
    size->context_size = sizeof(vc1_viddec_parser_t);
    size->persist_size = 0;
    return;
} // viddec_vc1_get_context_size

#ifdef VBP
uint32_t viddec_vc1_is_start_frame(void *ctxt)
#else
static uint32_t viddec_vc1_is_start_frame(void *ctxt)
#endif
{
    vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *) ctxt;
    return parser->is_frame_start;
} // viddec_vc1_is_start_frame

void viddec_vc1_get_ops(viddec_parser_ops_t *ops)
{
    ops->init = viddec_vc1_init;
    ops->parse_syntax = viddec_vc1_parse;
    ops->get_cxt_size = viddec_vc1_get_context_size;
    ops->is_wkld_done = viddec_vc1_wkld_done;
    ops->is_frame_start = viddec_vc1_is_start_frame;
    return;
} // viddec_vc1_get_ops

