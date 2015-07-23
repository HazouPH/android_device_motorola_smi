

#include "viddec_emitter.h"
#include "viddec_fw_workload.h"
#include "viddec_fw_debug.h"

int32_t viddec_emit_flush_current_wkld(viddec_emitter *emit)
{
    if (emit->cur.data != NULL)
    {
        emit->cur.data->num_items = emit->cur.num_items;
    }
    if (emit->next.data != NULL)
    {
        emit->next.data->num_items = emit->next.num_items;
    }
    emit->cur.num_items = emit->next.num_items;
    emit->next.num_items = 0;
    if (emit->cur.data != NULL)
    {
        emit->cur.data->result = emit->cur.result;
    }
    if (emit->next.data != NULL)
    {
        emit->next.data->result = emit->next.result;
    }
    emit->cur.result = emit->next.result;
    emit->next.result = 0;
    return 1;
}

int32_t viddec_emit_append(viddec_emitter_wkld *cxt, viddec_workload_item_t *item)
{
    int32_t ret =0;
    if ((cxt->num_items < cxt->max_items) && (cxt->data != NULL))
    {
        cxt->data->item[cxt->num_items] = *item;
        cxt->num_items++;
        ret = 1;
        CDEB(0, "%s: item(%02d) = [%08x %08x %08x %08x]\n",__FUNCTION__, cxt->num_items - 1, item->vwi_type, item->vwi_payload[0], item->vwi_payload[1], item->vwi_payload[2]);
    }
    else
    {
        cxt->result |= (VIDDEC_FW_WORKLOAD_ERR_ITEMS_OVERFLOW | VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE);
        WRITE_SVEN(SVEN_MODULE_EVENT_GV_FW_FATAL_WKLD_OVERLFOW, (int)item->vwi_type, (int)(cxt->data), 0, 0, 0, 0);
    }
    return ret;
}

int32_t viddec_emit_contr_tag(viddec_emitter *emit, viddec_input_buffer_t *ibuf, uint8_t incomplete, uint32_t using_next)
{
    viddec_workload_item_t   item;
    viddec_emitter_wkld *cur_wkld;

    cur_wkld = (using_next == 0)? &(emit->cur):&(emit->next);

    if (!incomplete)
        item.vwi_type           = VIDDEC_WORKLOAD_IBUF_DONE;
    else
        item.vwi_type           = VIDDEC_WORKLOAD_IBUF_CONTINUED;
    item.tag.tag_phys_addr  = ibuf->phys;
    item.tag.tag_phys_len   = ibuf->len;
    item.tag.tag_value      = ibuf->id;

    return viddec_emit_append(cur_wkld, &item);
}

int32_t viddec_emit_assoc_tag(viddec_emitter *emit, uint32_t id, uint32_t using_next)
{
    viddec_workload_item_t   item;
    viddec_emitter_wkld *cur_wkld;

    cur_wkld = (using_next == false)? &(emit->cur):&(emit->next);
    item.vwi_type           = VIDDEC_WORKLOAD_TAG;
    item.tag.tag_phys_addr  = -1;
    item.tag.tag_phys_len   = -1;
    item.tag.tag_value      = id;
    return viddec_emit_append(cur_wkld, &item);
}

