
#include "viddec_pm_utils_list.h"
#include "viddec_fw_debug.h"

/*
  Initialize list.
 */
void viddec_pm_utils_list_init(viddec_pm_utils_list_t *cxt)
{
    cxt->num_items = 0;
    cxt->start_offset = 0;
    cxt->end_offset = -1;
    cxt->total_bytes = 0;
    cxt->first_scprfx_length = 0;
}

#ifndef VBP
/*
  Add a new ES buffer to list. If not succesful returns 0.
 */
uint32_t viddec_pm_utils_list_addbuf(viddec_pm_utils_list_t *list, viddec_input_buffer_t *es_buf)
{
    uint32_t ret = 0;
    if ((list->num_items + 1) <= MAX_IBUFS_PER_SC)
    {
        list->num_items +=1;
        list->sc_ibuf[list->num_items - 1] = *es_buf;
        ret = 1;
    }
    return ret;
}

/*
  We return the index of es buffer and the offset into it for the requested byte offset.
  EX: if byte=4, and the first es buffer in list is of length 100, we return lis_index=0, offset=3.
  byte value should range from [1-N].
 */
uint32_t viddec_pm_utils_list_getbyte_position(viddec_pm_utils_list_t *list, uint32_t byte, uint32_t *list_index, uint32_t *offset)
{
    uint32_t index = 0, accumulated_size=0;

    /* First buffer in list is always special case, since start offset is tied to it */
    accumulated_size = list->sc_ibuf[index].len - list->start_offset;
    if ( accumulated_size >= byte)
    {
        /* we found a match in first buffer itself */
        *offset = list->start_offset + byte - 1;
        *list_index = index;
        return 0;
    }
    index++;
    /* walkthrough the list until we find the byte */
    while (index < list->num_items)
    {
        if ((accumulated_size + list->sc_ibuf[index].len) >= byte)
        {
            *offset = byte - accumulated_size - 1;
            *list_index = index;
            return 0;
        }
        accumulated_size += list->sc_ibuf[index].len;
        index++;
    }
    return 1;
}

/*
  Since the stream data can span multiple ES buffers on different DDR locations, for our purpose
  we store start and end position on each ES buffer to make the data look linear.
  The start represents the linear offset of the first byte in list.
  end-1 represents linear offset of last byte in list.
 */
void viddec_pm_utils_list_updatebytepos(viddec_pm_utils_list_t *list, uint8_t sc_prefix_length)
{
    uint32_t items=0;
    uint32_t start=0, end=0;

    if (list->num_items != 0)
    {
        end = list->sc_ibuf[0].len - list->start_offset;
        if ((int32_t)end >= list->total_bytes) end = list->total_bytes;
        list->data[items].stpos = start;
        list->data[items].edpos = end;
        items++;
        while ((int32_t)end < list->total_bytes)
        {
            start = end;
            end += list->sc_ibuf[items].len;
            if ((int32_t)end >= list->total_bytes) end = list->total_bytes;
            list->data[items].stpos = start;
            list->data[items].edpos = end;
            items++;
        }
        while (items < list->num_items)
        {
            if (sc_prefix_length != 0)
            {
                start = end = list->total_bytes+1;
            }
            else
            {
                start = end = list->total_bytes;
            }
            list->data[items].stpos = start;
            list->data[items].edpos = end;
            items++;
        }
        /* Normal access unit sequence is SC+data+SC. We read SC+data+SC bytes so far.
           but the current access unit should be SC+data, the Second SC belongs to next access unit.
           So we subtract SC length to reflect that */
        list->total_bytes -= sc_prefix_length;
    }
}

static inline void viddec_pm_utils_list_emit_slice_tags_append(viddec_emitter_wkld *cur_wkld, viddec_workload_item_t *wi)
{
    /*
      Most of the time len >0. However we can have a condition on EOS where the last buffer can be
      zero sized in which case we want to make sure that we emit END of SLICE information.
     */
    if ((wi->es.es_phys_len != 0) || (wi->es.es_flags&VIDDEC_WORKLOAD_FLAGS_ES_END_SLICE))
    {
        viddec_emit_append(cur_wkld, wi);
    }
}

/*
  Emit requested tags for data from start to end position. The tags should include end byte too.
 */
void viddec_pm_utils_list_emit_slice_tags(viddec_pm_utils_list_t *list, uint32_t start, uint32_t end, viddec_emitter *emitter, uint32_t is_cur_wkld, viddec_workload_item_t *wi)
{
    if ((list->num_items != 0) && ((int32_t)start < (list->total_bytes)) && ((int32_t)end <= (list->total_bytes)))
    {
        uint32_t flags=0, items=0;
        viddec_emitter_wkld *cur_wkld;

        flags = wi->es.es_flags;
        cur_wkld = (is_cur_wkld != 0) ? &(emitter->cur):&(emitter->next);
        /* Seek until we find a ES buffer entry which has the start position */
        while (start >= list->data[items].edpos) items++;

        if (end < list->data[items].edpos)
        { /* One ES buffer has both start and end in it. So dump a single entry */
            wi->es.es_phys_len = end - start + 1;
            wi->es.es_phys_addr = list->sc_ibuf[items].phys + start - list->data[items].stpos;
            /* Account for start_offset if its the first buffer in List */
            if (items == 0) wi->es.es_phys_addr += list->start_offset;

            wi->es.es_flags = flags | VIDDEC_WORKLOAD_FLAGS_ES_START_SLICE | VIDDEC_WORKLOAD_FLAGS_ES_END_SLICE;
            viddec_pm_utils_list_emit_slice_tags_append(cur_wkld, wi);
        }
        else
        {
            /* We know that there are at least two buffers for the requested data. Dump the first item */
            wi->es.es_phys_len = list->data[items].edpos - start;
            wi->es.es_phys_addr = list->sc_ibuf[items].phys + start - list->data[items].stpos;
            if (items == 0) wi->es.es_phys_addr += list->start_offset;
            wi->es.es_flags = flags | VIDDEC_WORKLOAD_FLAGS_ES_START_SLICE;
            viddec_pm_utils_list_emit_slice_tags_append(cur_wkld, wi);
            items++;
            /* Dump everything in between if any until the last buffer */
            while (end >= list->data[items].edpos)
            {
                wi->es.es_phys_len = list->data[items].edpos - list->data[items].stpos;
                wi->es.es_phys_addr = list->sc_ibuf[items].phys;
                wi->es.es_flags = flags;
                viddec_pm_utils_list_emit_slice_tags_append(cur_wkld, wi);
                items++;
            }
            /* Dump ES buffer which has end in it along with end slice flag */
            wi->es.es_phys_len = end - list->data[items].stpos + 1;
            wi->es.es_phys_addr = list->sc_ibuf[items].phys;
            wi->es.es_flags = flags | VIDDEC_WORKLOAD_FLAGS_ES_END_SLICE;
            viddec_pm_utils_list_emit_slice_tags_append(cur_wkld, wi);
        }
    }
}

/*
  We delete the consumed buffers in our list. If there are any buffers left over which have more data
  the get moved to the top of the list array.
 */
void viddec_pm_utils_list_remove_used_entries(viddec_pm_utils_list_t *list, uint32_t length)
{
    list->end_offset = -1;

    if (list->num_items != 0)
    {
        if (length != 0)
        {
            uint32_t items = list->num_items-1, byte_pos;
            uint32_t index=0;
            viddec_input_buffer_t *es_buf;
            byte_pos = list->total_bytes;
            while ((list->data[items].edpos > byte_pos) && (list->data[items].stpos > byte_pos))
            {
                items--;
            }
            if (items != 0)
            {
                list->start_offset = byte_pos - list->data[items].stpos;
                while (items < list->num_items)
                {
                    if (index < MAX_IBUFS_PER_SC)
                    {
                        es_buf = &(list->sc_ibuf[items]);
                        list->sc_ibuf[index] = *es_buf;
                        index++;
                        items++;
                    }
                }
                list->num_items = index;
            }
            else
            {
                list->start_offset += (byte_pos - list->data[items].stpos);
            }
        }
        else
        {
            list->num_items = 0;
            list->start_offset = 0;
        }
        list->total_bytes = length;
    }
}
#endif
