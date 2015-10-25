
#include "fw_pvt.h"
#include "viddec_fw_parser_ipclib_config.h"
#include "viddec_fw_common_defs.h"
#include "viddec_pm_tags.h"
#include "viddec_fw_parser.h"

extern dmem_t _dmem;
extern viddec_parser_ops_t parser_ops[MFD_STREAM_FORMAT_MAX];

static void viddec_fw_parser_peekmessages(viddec_pm_cxt_t *pm, ipc_msg_data *wkld_cur, ipc_msg_data *wkld_next, int32_t *ret_cur, int32_t *ret_next, uint32_t stream_id)
{
    FW_IPC_Handle *fwipc = GET_IPC_HANDLE(_dmem);
    wkld_cur->phys = wkld_next->phys = 0;
    /* read current and next workloads by peeking to free wkld queue.This would only give us a copy
       of message but won't actually pull it out of queue*/

    *ret_cur = FwIPC_PeekReadMessage(fwipc, &(fwipc->wkld_q[stream_id]), (char *)wkld_cur, sizeof(ipc_msg_data), 0);
    *ret_next = FwIPC_PeekReadMessage(fwipc, &(fwipc->wkld_q[stream_id]), (char *)wkld_next, sizeof(ipc_msg_data), 1);
    /* NOTE: I am passing length of current workload as size for next, since next workload might not exist. This is safe since in flush we always append to current workload */
    viddec_emit_update(&(pm->emitter), wkld_cur->phys, wkld_next->phys, wkld_cur->len, wkld_cur->len);
}

static void viddec_fw_parser_push_error_workload(viddec_pm_cxt_t *pm, ipc_msg_data *wkld_cur, uint32_t stream_id)
{
    FW_IPC_Handle *fwipc = GET_IPC_HANDLE(_dmem);
    /* Push the current wkld */
    viddec_emit_set_workload_error(&(pm->emitter),
                                   (VIDDEC_FW_WORKLOAD_ERR_FLUSHED_FRAME | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE),
                                   false);
    viddec_emit_flush_current_wkld(&(pm->emitter));
    FwIPC_SendMessage(fwipc, stream_id, (char *)wkld_cur, sizeof(ipc_msg_data));
    FwIPC_ReadMessage(fwipc, &(fwipc->wkld_q[stream_id]), (char *)wkld_cur, sizeof(ipc_msg_data));
}

int viddec_fw_parser_flush(unsigned int stream_id, unsigned int flush_type)
{
    FW_IPC_Handle *fwipc = GET_IPC_HANDLE(_dmem);
    mfd_pk_strm_cxt *cxt;
    mfd_stream_info *cxt_swap;
    viddec_pm_cxt_t *pm;
    int32_t pos=0, ret = VIDDEC_FW_SUCCESS;/* success */
    uint32_t workloads_in_input_q = 0;
    cxt = (mfd_pk_strm_cxt *)&(_dmem.srm_cxt);
    cxt_swap = (mfd_stream_info *)&(_dmem.stream_info[stream_id]);
    pm = &(cxt->pm);

    workloads_in_input_q = ipc_mq_read_avail(&fwipc->wkld_q[stream_id].mq, (int32_t *)&pos);
    pos = 0;
    /* Check to see if output queue has space for next message */
    if (ipc_mq_write_avail(&fwipc->snd_q[stream_id].mq,&pos) >= workloads_in_input_q)
    {
        /* Check how many free workloads are available. Need at least 1 */
        if (workloads_in_input_q  >= CONFIG_IPC_MESSAGE_MAX_SIZE)
        {
            ipc_msg_data wkld_cur, wkld_next, cur_es;
            int32_t ret_cur=0,ret_next=0;

            {/* Swap context into local memory */
                cp_using_dma(cxt_swap->ddr_cxt, (uint32_t) pm, sizeof(viddec_pm_cxt_t), false, false);
            }

            viddec_fw_parser_peekmessages(pm, &wkld_cur, &wkld_next, &ret_cur, &ret_next, stream_id);
            if (workloads_in_input_q >= (CONFIG_IPC_MESSAGE_MAX_SIZE << 1))
            {/* If we have more than 2 workloads, most likely current workload has partial data. To avoid overflow
                lets push current and use next which is most likely empty .If there's only one workload it was
                next for previous frame so most likely its empty in which case we don't do this logic*/
                viddec_fw_parser_push_error_workload(pm, &wkld_cur, stream_id);
                viddec_fw_parser_peekmessages(pm, &wkld_cur, &wkld_next, &ret_cur, &ret_next, stream_id);
            }
            /* Empty current es buffers in list */
            /* TODO(Assumption): we have to make sure that list flush is really succesful by checking return values.
               If our workload size is big enough to to accomadate buf done tags then its not necessary
               since we will guaranteed succesful writes for all es buffers */
            viddec_pm_generate_tags_for_unused_buffers_to_flush(pm);
            /* Check the number of ES buffers and append them to current wkld */
            while (FwIPC_ReadMessage(fwipc, &(fwipc->rcv_q[stream_id]), (char *)&cur_es, sizeof(ipc_msg_data)) != 0)
            {
                /* NOTE(Assumption): Again we have to define workload size to be big enough to make sure we can fit
                   all the es buffers into current workload */
                viddec_emit_contr_tag(&(pm->emitter), &cur_es, 0, false);
                viddec_emit_assoc_tag(&(pm->emitter), cur_es.id, false);
            }
            viddec_fw_parser_push_error_workload(pm, &wkld_cur, stream_id);
            do
            {/* Read until no workloads left */
                viddec_fw_parser_peekmessages(pm, &wkld_cur, &wkld_next, &ret_cur, &ret_next, stream_id);
                if (ret_cur == 0)
                {
                    break;
                }
                viddec_fw_parser_push_error_workload(pm, &wkld_cur, stream_id);
            } while (1);
            switch (flush_type)
            {
            case VIDDEC_STREAM_FLUSH_DISCARD:
            {
                /* Reset pm_context */
                viddec_fw_init_swap_memory(stream_id, 0, 1);
            }
            break;
            case VIDDEC_STREAM_FLUSH_PRESERVE:
            {
                /* Reset just stream information */
                viddec_fw_init_swap_memory(stream_id, 0, 0);
            }
            default:
                break;
            }
            {/* swap context into DDR */
                cp_using_dma(cxt_swap->ddr_cxt, (uint32_t) pm, sizeof(viddec_pm_cxt_t), true, false);
            }
        }
        else
        {
            pos = 0;
            /* check to see if I have any es buffers on input queue. If none are present we don't have to do anything */
            if (ipc_mq_read_avail(&fwipc->rcv_q[stream_id].mq, (int32_t *)&pos)  != 0)
                ret = VIDDEC_FW_NEED_FREE_WKLD;
        }
    }
    else
    {
        /* data present in output queue. */
        ret =VIDDEC_FW_PORT_FULL;
    }
    return ret;
}
