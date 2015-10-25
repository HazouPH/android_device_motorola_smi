
#include "viddec_pm.h"
#include "viddec_fw_debug.h"
#include "viddec_fw_common_defs.h"
#include "viddec_pm_tags.h"
#include "viddec_parser_ops.h"
#include "viddec_vc1_parse.h"
#include "viddec_mp4_parse.h"
#include "viddec_mpeg2_parse.h"
#include "viddec_h264_parse.h"
/*
  Overview of Parser manager:
  Parser manager is the glue between Kernel(main.c) and actual codecs. We abstract common functionality as much as we can
  in this module. The parser Manager context allocates memory for Parsers. At any point in time there is only one active stream.
  During open stream we setup all necessary initialisation for the codec we are handling. The parser manager context is
  stored on DDR when the current stream gets swapped out by the kernel. When the next stream comes in it has it's own
  version of parser manager.
  Parser manager is reponsible for providing information on when its a good time to swap a stream.
  High level algorithm of parser Manager once a stream is opened and active(RET's are returns to Kernel):

  1. create a list data structure to hold any incoming ES descriptors.
  2. Check to see if any of the ES buffers Desc in current list has data to be processed. If not request kernel(RET) for a buffer.
  3. If data is present parse until a scprefix+sc is found. If not goto step2.
  4. If startcode detected update list state to make ES data look like Linear buffer.
  5. Setup required state to provide getbits interface for codecs to access bit stream maximum 32bits at a time.
  6. Setup Current & Next workloads provided by Kernel.
  7. Call the codec to parse the data we collected between start codes.
  8. Query to see if we parsed frame worth of data.
  9. Do necessary TAG association and remove used buffers from List.
  10. Send information to kernel on whether workload is done or Not.(RET). When kernel reschedules start from step2.

  Kernel can swap current stream at RET points described above.

  Other additional things supported:
  - Generic start code detect function which is same for most of codecs.
  - Memory Management.
  - Flush of stream.
  - Emulation prevention.
  - Interface to emit necessary tags for codec specific types.
*/


/* check to see if codec needs emulation prevention */
#define EMUL_REQD(codec) ((codec == MFD_STREAM_FORMAT_VC1) || (codec_type == MFD_STREAM_FORMAT_H264) ? 1: 0)

#ifdef RTL_SIMULATION
extern void output_omar_wires( unsigned int value );
#else
#define output_omar_wires(x)
#endif

/* Place to store Function pointers for all supported interfaces for each codec */
viddec_parser_ops_t parser_ops[MFD_STREAM_FORMAT_MAX];



/* we need to define as external function so that for host mode we can use the same code without
   modifications by overloading dma function with a copy function
*/
extern uint32_t cp_using_dma(uint32_t ddr_addr, uint32_t local_addr, uint32_t size, char to_ddr, char swap);

void viddec_pm_init_ops()
{
    viddec_vc1_get_ops(&parser_ops[MFD_STREAM_FORMAT_VC1]);
    parser_ops[MFD_STREAM_FORMAT_VC1].parse_sc = viddec_parse_sc;
    parser_ops[MFD_STREAM_FORMAT_VC1].gen_contrib_tags = viddec_pm_generic_generate_contribution_tags;
    parser_ops[MFD_STREAM_FORMAT_VC1].gen_assoc_tags = viddec_generic_add_association_tags;

    viddec_mpeg2_get_ops(&parser_ops[MFD_STREAM_FORMAT_MPEG]);
    parser_ops[MFD_STREAM_FORMAT_MPEG].parse_sc = viddec_parse_sc;
    parser_ops[MFD_STREAM_FORMAT_MPEG].gen_contrib_tags = viddec_pm_generic_generate_contribution_tags;
    parser_ops[MFD_STREAM_FORMAT_MPEG].gen_assoc_tags = viddec_mpeg2_add_association_tags;

    viddec_h264_get_ops(&parser_ops[MFD_STREAM_FORMAT_H264]);
    parser_ops[MFD_STREAM_FORMAT_H264].parse_sc = viddec_parse_sc;
    parser_ops[MFD_STREAM_FORMAT_H264].gen_contrib_tags = viddec_pm_lateframe_generate_contribution_tags;
    parser_ops[MFD_STREAM_FORMAT_H264].gen_assoc_tags = viddec_h264_add_association_tags;

    viddec_mp4_get_ops(&parser_ops[MFD_STREAM_FORMAT_MPEG42]);
    parser_ops[MFD_STREAM_FORMAT_MPEG42].gen_contrib_tags = viddec_pm_generic_generate_contribution_tags;
    parser_ops[MFD_STREAM_FORMAT_MPEG42].gen_assoc_tags = viddec_generic_add_association_tags;
}

/*
  Returns size of persistent DDR memory required for the codec. If the required memory is less than max allocated
  scratch memory in FW we always give the max scratch size.
*/
uint32_t viddec_pm_get_parser_sizes(uint32_t codec_type, viddec_parser_memory_sizes_t *size)
{
    parser_ops[codec_type].get_cxt_size(size);
    if (size->context_size > MAX_CODEC_CXT_SIZE)
    {
        DEB("ERROR: size(%d) of context for codec=%d is greater than max=%d\n",size->context_size,codec_type,MAX_CODEC_CXT_SIZE);
    }
    size->context_size = sizeof(viddec_pm_cxt_t);
    return 1;
}

/*
  Initialize the scratch memory allocated to the stream based on clean. if clean is true initialize to
  start state, if not then preserve stream information.
*/
void viddec_pm_init_context(viddec_pm_cxt_t *cxt, uint32_t codec_type, uint32_t *persist_mem, uint32_t clean)
{
    int i;

    for (i=0; i<MAX_IBUFS_PER_SC; i++)
    {
        cxt->pending_tags.pending_tags[i] = INVALID_ENTRY;
    }
    cxt->frame_start_found = false;
    cxt->found_fm_st_in_current_au = false;
    cxt->late_frame_detect = (MFD_STREAM_FORMAT_H264 == codec_type) ? true:false;
    cxt->pending_tags.first_buf_aligned = cxt->pending_tags.using_next = cxt->pending_tags.frame_done =false;
    cxt->next_workload_error_eos = VIDDEC_FW_WORKLOAD_ERR_FLUSHED_FRAME | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
    viddec_pm_utils_list_init(&(cxt->list));
    cxt->cur_buf.list_index = -1;
    cxt->parse_cubby.phase=0;
    parser_ops[codec_type].init((void *)&(cxt->codec_data[0]), persist_mem, !clean);
    if (clean)
    {
        cxt->pending_inband_tags = 0;
    }
    else
    {
        /* TODO: Enable this once codecs support this function */
        //parser_ops[codec_type].flush_preserve((void *)&(cxt->codec_data[0]), persist_mem);
    }

}

void viddec_pm_update_time(viddec_pm_cxt_t *cxt, uint32_t time)
{
    viddec_emit_time(&(cxt->emitter), time);
}

/* add an esbuffer to list */
static inline uint32_t viddec_pm_add_es_buf_to_list(viddec_pm_cxt_t *cxt, viddec_input_buffer_t *es_buf)
{
    uint32_t val , ret = PM_OVERFLOW;

    val = viddec_pm_utils_list_addbuf(&(cxt->list), es_buf);
    if (val == 1) ret = PM_SUCCESS;
    return ret;
}

static inline uint32_t viddec_pm_check_inband_messages(viddec_pm_sc_cur_buf_t *cur_buf, uint32_t *type)
{
    uint32_t ret=false;
    if (cur_buf->cur_es->flags != 0)
    {
        /* update offset to point to next position for loading data */
        cur_buf->cur_offset +=(cur_buf->cur_size);
        cur_buf->cur_size = 0;
        switch (cur_buf->cur_es->flags)
        {
        case VIDDEC_STREAM_EOS:
        {
            *type = PM_EOS;
        }
        break;
        case VIDDEC_STREAM_DISCONTINUITY:
        {
            *type = PM_DISCONTINUITY;
        }
        default:
            break;
        }
        ret =true;
    }
    return ret;
}

/* creates an ibuf from the current position in list. Fills sc_parse_cubby_cxt */
uint32_t viddec_pm_create_ibuf(viddec_pm_cxt_t *cxt)
{
    uint32_t ret = PM_NO_DATA;
#ifndef VBP
    viddec_sc_parse_cubby_cxt_t *cubby = &(cxt->parse_cubby);
#endif
    viddec_pm_sc_cur_buf_t *cur_buf = &(cxt->cur_buf);
    viddec_pm_utils_list_t *list = &(cxt->list);

    /* Step1: check if list is Empty, If yes return No data */
    if (list->num_items > 0)
    {
        /* Step 2: Check to see If current index into list is empty & we have data in list,
           if so increment index and initialise it*/
        if (cur_buf->list_index == -1)
        {
            if (viddec_pm_utils_list_getbyte_position(list,
                    list->first_scprfx_length+1,
                    (uint32_t *)&(cur_buf->list_index),
                    &(cur_buf->cur_offset)) != 1)
            {/* This return's offset and index from where we have to start for sc detect */
                cur_buf->cur_size = 0;
                cur_buf->cur_es = &(list->sc_ibuf[cur_buf->list_index]);
            }
            else
            {
                return PM_NO_DATA;
            }
        }

        /* Step3: If we are done with current buffer then try to go to next item in list */
        if ((cur_buf->cur_offset + cur_buf->cur_size) >= cur_buf->cur_es->len)
        {
            /* Need to handle In band messages before going to next buffer */
            //if(viddec_pm_check_inband_messages(cur_buf))
            if (viddec_pm_check_inband_messages(cur_buf, &ret))
            {
                return ret;
            }
            /* If no items in list after the current buffer return no data */
            if ((uint32_t)(cur_buf->list_index + 1) >=  list->num_items)
            {
                return PM_NO_DATA;
            }
            cur_buf->list_index++;
            cur_buf->cur_es = &(list->sc_ibuf[cur_buf->list_index]);
            cur_buf->cur_offset = cur_buf->cur_size = 0;
        }
        /* Step4: Fill the cubby with data to send to parser sc code function */
        {
            int32_t data_left;
            /* data left is the leftout size in current ES buffer */
            data_left = cur_buf->cur_es->len -  (cur_buf->cur_offset + cur_buf->cur_size);

            /* update offset to point to next position for loading data */
            cur_buf->cur_offset +=(cur_buf->cur_size);

#ifndef VBP
            /* Load maximum of array size */
            if (data_left >= SC_DETECT_BUF_SIZE)
            {
                data_left = SC_DETECT_BUF_SIZE;
            }
            /* can be zero if we have zero sized buffers in our list.EX:NEW segment */
            if (data_left > 0)
            {/* do a copy using Linear Dma */
                uint32_t size , ddr_addr = 0, ddr_mask=0;
                /* get ddr adress of current offset in ES buffer */
#ifdef HOST_ONLY
                ddr_addr = cur_buf->cur_offset + (uint32_t)cur_buf->cur_es->buf;
#else
                ddr_addr = cur_buf->cur_offset + cur_buf->cur_es->phys;
#endif
                ddr_mask = (ddr_addr & 3);
                ddr_addr = ddr_addr & ~3;
                /* return from this function can be more bytes based on input buf alignment.
                   The adress for local memory we are sending is on DWORD boundary so it should be safe.
                */

                size = cp_using_dma(ddr_addr, (uint32_t)&(cxt->scbuf[0]), data_left+ddr_mask, 0,1);//false, true);
                cubby->size = data_left;

                /* point to actual memory location which has the data(skip aligment bytes) */
                cubby->buf = &(cxt->scbuf[ddr_mask]);
                cur_buf->cur_size = data_left;
                ret = PM_SUCCESS;
            }
            else
            {
                /* If we completely consumed this buffer or this is a zero sized buffer we want to check inband messages */
                //if(viddec_pm_check_inband_messages(cur_buf))
                if (viddec_pm_check_inband_messages(cur_buf, &ret))
                {
                    return ret;
                }
            }
#else
            ret = PM_SUCCESS;
#endif
        }
    }

    return ret;
}

/*
  Read data from esbuffer list and parse for start codes or EOS. If we consumed all the data we return no data left.
*/
static inline uint32_t viddec_pm_parse_for_sccode(viddec_pm_cxt_t *cxt, viddec_parser_ops_t *func)
{
    uint32_t ret = PM_NO_DATA;
    uint32_t sc_boundary_found = 0;

    while (!sc_boundary_found)
    {
        /* Create an buffer from list to parse */
        ret = viddec_pm_create_ibuf(cxt);
        switch (ret)
        {
        case PM_NO_DATA:
        {/* No data in esbuffer list for parsing sc */
            sc_boundary_found = 1;
        }
        break;
        case PM_EOS:
        case PM_DISCONTINUITY:
        {
            sc_boundary_found = 1;
            cxt->list.end_offset = cxt->cur_buf.cur_offset+1;
            cxt->parse_cubby.phase = 0;
            /* we didn't find a start code so second start code length would be 0 */
            cxt->sc_prefix_info.second_scprfx_length = 0;
            //cxt->sc_prefix_info.next_sc = VIDDEC_PARSE_EOS;
            if (ret == PM_EOS)
            {
                cxt->sc_prefix_info.next_sc = VIDDEC_PARSE_EOS;
            }
            if (ret == PM_DISCONTINUITY)
            {
                cxt->sc_prefix_info.next_sc = VIDDEC_PARSE_DISCONTINUITY;
            }
        }
        break;
        case PM_SUCCESS:
        default:
        {
            /* parse the created buffer for sc */
            ret = func->parse_sc((void *)&(cxt->parse_cubby), (void *)&(cxt->codec_data[0]), &(cxt->sc_prefix_info));
            if (ret == 1)
            {
                cxt->list.end_offset = cxt->parse_cubby.sc_end_pos + cxt->cur_buf.cur_offset;
                cxt->parse_cubby.phase = 0;
                cxt->list.total_bytes+=cxt->parse_cubby.sc_end_pos;
                ret = PM_SC_FOUND;
                sc_boundary_found = 1;
                break;
            }
            else
            {
                cxt->list.total_bytes+=cxt->cur_buf.cur_size;
            }
        }
        break;
        }
    }

    return ret;
}

/*
  Once we are ready to flush the current workload, we update current workload on DDR with our internal information
  that was not written before like num of items in workload, errors in stream etc...
*/
void viddec_pm_finalize_workload(viddec_pm_cxt_t *cxt, uint32_t codec_type, uint32_t codec_errors)
{
    viddec_emit_set_codec(&(cxt->emitter), codec_type);
    viddec_emit_set_codec_errors(&(cxt->emitter), codec_errors);
    viddec_emit_flush_current_wkld(&(cxt->emitter));
    output_omar_wires( 0x5 );
    output_omar_wires( 0x1 );
}

/*
  After parsing between start codes we cleanup our list so that it has only buffers that are not consumed yet.
*/
uint32_t viddec_pm_finalize_list(viddec_pm_cxt_t *cxt)
{
    uint32_t ret=1;

    viddec_pm_utils_list_remove_used_entries(&(cxt->list), cxt->sc_prefix_info.second_scprfx_length);
    cxt->cur_buf.list_index = -1;
    cxt->list.first_scprfx_length = cxt->sc_prefix_info.second_scprfx_length;
    return ret;
}

/* Case to handle if we encounter list overflow without seeing second start code */
void viddec_pm_handle_buffer_overflow(viddec_pm_cxt_t *cxt, uint32_t codec_type, viddec_input_buffer_t *es_buf)
{
    uint32_t indx=0;
    while (indx< (uint32_t)cxt->list.num_items)
    {/* Dump tags for all entries in list to prevent buffer leak */
        viddec_emit_contr_tag(&(cxt->emitter), &(cxt->list.sc_ibuf[indx]), false, true);
        viddec_emit_assoc_tag(&(cxt->emitter), cxt->list.sc_ibuf[indx].id, true);
        indx++;
    }
    /* Dump tags for the new buffer that was received */
    viddec_emit_contr_tag(&(cxt->emitter), es_buf, 0, true);
    viddec_emit_assoc_tag(&(cxt->emitter), es_buf->id, true);
    /* Set errors on both current and next as both can be invalid */
    viddec_emit_set_workload_error(&(cxt->emitter),
                                   (VIDDEC_FW_WORKLOAD_ERR_BUFFERS_OVERFLOW | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE),
                                   true);
    viddec_emit_set_workload_error(&(cxt->emitter),
                                   (VIDDEC_FW_WORKLOAD_ERR_BUFFERS_OVERFLOW | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE),
                                   false);
    /* cleanup the pending tags */
    viddec_pm_generate_missed_association_tags(cxt, true);
    viddec_pm_finalize_workload(cxt, codec_type, 0);
    WRITE_SVEN(SVEN_MODULE_EVENT_GV_FW_FATAL_BUFFER_OVERLFOW, (int)es_buf->phys, (int)es_buf->len, 0, 0, 0, 0);
}

static inline void viddec_pm_handle_post_inband_messages(viddec_pm_cxt_t *cxt, uint32_t m_type)
{
    if ((m_type & ~(0xFF))== PM_INBAND_MESSAGES)
    {
        /* If EOS decide set error on next workload too */
        viddec_emit_set_workload_error(&(cxt->emitter), cxt->next_workload_error_eos, true);
        if (m_type == PM_EOS)
        {
            viddec_emit_set_inband_tag(&(cxt->emitter), VIDDEC_WORKLOAD_IBUF_EOS, true);
        }
        if (m_type == PM_DISCONTINUITY)
        {
            cxt->pending_inband_tags = PM_DISCONTINUITY;
        }
    }
}

static inline uint32_t viddec_pm_handle_new_es_buffer(viddec_pm_cxt_t *cxt, uint32_t codec_type, viddec_input_buffer_t *es_buf)
{
    uint32_t state = PM_SUCCESS;
    if (es_buf != NULL)
    {
        state = viddec_pm_add_es_buf_to_list(cxt, es_buf);
        if (state == PM_OVERFLOW)
        {
            viddec_pm_handle_buffer_overflow(cxt, codec_type, es_buf);
        }
    }
    return state;
}

static inline void viddec_pm_handle_pre_inband_messages(viddec_pm_cxt_t *cxt)
{
    if (cxt->pending_inband_tags == PM_DISCONTINUITY)
    {
        viddec_emit_set_inband_tag(&(cxt->emitter), VIDDEC_WORKLOAD_IBUF_DISCONTINUITY, false);
        cxt->pending_inband_tags = 0;
    }
}

/*
  Main function of parser manager.
  It searches until start codes are found int he list if not through return type indicates kernel to provide more buffers.
  If a start code is found it calls the codec to parse the syntax data it accumulated so far.
  If codec says a frame is not done then continues to find the next start code.
  If codec says frame is done it does tag association and indicates kernel a frame is done.
*/
uint32_t viddec_pm_parse_es_buffer(viddec_pm_cxt_t *cxt, uint32_t codec_type, viddec_input_buffer_t *es_buf)
{
    uint32_t state = PM_SUCCESS;

    /* Step1: Append Es buffer to list */
    viddec_pm_handle_pre_inband_messages(cxt);
    state = viddec_pm_handle_new_es_buffer(cxt, codec_type, es_buf);
    if (state == PM_SUCCESS)
    {
        uint32_t scdetect_ret;
        output_omar_wires( 0x3 );
        /* Step2: Phase1 of parsing, parse until a sc is found */
        scdetect_ret = viddec_pm_parse_for_sccode(cxt,&parser_ops[codec_type]);
        switch (scdetect_ret)
        {
        case PM_NO_DATA:
        {
            /* Step3: If we consumed all the data indicate we need more buffers */
            state = PM_NO_DATA;
            break;
        }
        case PM_EOS:
        case PM_DISCONTINUITY:
        case PM_SC_FOUND:
        {
            uint32_t codec_errors=0;
            /* Create necessary state information to make the ES buffers look like linear data */
            viddec_pm_utils_list_updatebytepos(&(cxt->list), cxt->sc_prefix_info.second_scprfx_length);
            if (cxt->sc_prefix_info.first_sc_detect != 1)
            {
                /* Step4: If we saw two start codes init state and call codec to parse */
                uint32_t codec_ret;
                /* Initialise the state to provide get bits for codecs */
                viddec_pm_utils_bstream_init(&(cxt->getbits), &(cxt->list), EMUL_REQD(codec_type));
                output_omar_wires( 0x1 );
                /* call the codec to do synatax parsing */
                parser_ops[codec_type].parse_syntax((void *)cxt, (void *)&(cxt->codec_data[0]));
                /* Check and see if frame start was detected. If we did update frame start in current au */
                if (parser_ops[codec_type].is_frame_start((void *)&(cxt->codec_data[0])) == true)
                {
                    cxt->frame_start_found += 1;
                    cxt->found_fm_st_in_current_au = true;
                }
                /* Query to see if we reached end of current frame */
                codec_ret = parser_ops[codec_type].is_wkld_done((void *)cxt,
                            (void *)&(cxt->codec_data[0]),
                            (uint32_t)(cxt->sc_prefix_info.next_sc),
                            &codec_errors);

                state = (codec_ret == VIDDEC_PARSE_FRMDONE) ? PM_WKLD_DONE : PM_SUCCESS;
                /* generate contribution and association tags */
                cxt->pending_tags.frame_done = (codec_ret == VIDDEC_PARSE_FRMDONE);
                parser_ops[codec_type].gen_assoc_tags(cxt);
                parser_ops[codec_type].gen_contrib_tags(cxt, (state != PM_WKLD_DONE));
            }
            else
            {
                /* Step4: If this is the first start code in this stream, clean up and return */
                if (cxt->list.total_bytes != 0)
                {
                    viddec_pm_generic_generate_contribution_tags(cxt, true);
                    viddec_generic_add_association_tags(cxt);
                }
                else
                {
                    if (cxt->list.num_items >= 1)
                    {
                        uint32_t indx=0;
                        while ((indx< (uint32_t)cxt->list.num_items) && (cxt->list.sc_ibuf[indx].len == 0))
                        {/* Dump all zero sized buffers until we see a buffer with valid data */
                            viddec_emit_contr_tag(&(cxt->emitter), &(cxt->list.sc_ibuf[indx]), false, false);
                            viddec_emit_assoc_tag(&(cxt->emitter), cxt->list.sc_ibuf[indx].id, false);
                            indx++;
                        }
                    }
                }
                if ((scdetect_ret & ~(0xFF))!= PM_INBAND_MESSAGES)
                {
                    state = PM_SUCCESS;//state = PM_FIRST_SC_FOUND;
                    cxt->sc_prefix_info.first_sc_detect = 0;
                }
                else
                {
                    state = PM_WKLD_DONE;
                }
            }

            viddec_pm_handle_post_inband_messages(cxt, scdetect_ret);

            /* Step 5: If current frame is done, finalise the workload state with necessary information */
            if (state == PM_WKLD_DONE)
            {
                DEB("\nFRAME ... DONE\n");
                /* we decrement frame start. This can be 0 in cases like sending junk data with EOS */
                cxt->frame_start_found -= (cxt->frame_start_found)? 1: 0;
                if ((scdetect_ret & ~(0xFF))== PM_INBAND_MESSAGES)
                {/* If EOS dump pending tags and set state */
                    viddec_pm_generate_missed_association_tags(cxt, false);
                    state = scdetect_ret;
                }
                /* Write back stored state of workloads to memory to prepare for psuhing to output queue */
                viddec_pm_finalize_workload(cxt, codec_type, codec_errors);
            }
            /* Step 6: Reset the list to prepare for next iteration */
            viddec_pm_finalize_list(cxt);
            break;
        }
        default:
            break;
        }
    }//if(state == PM_SUCCESS)
    return state;
} // viddec_pm_parse_es_buffer
