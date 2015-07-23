#ifndef VIDDEC_EMITTER_H
#define VIDDEC_EMITTER_H
#ifndef ANDROID
#include <stdint.h>
#else
#include "../../include/stdint.h"
#endif
#ifndef HOST_ONLY
#define DDR_MEM_MASK 0x80000000
#else
#define DDR_MEM_MASK 0x0
#endif
#include "viddec_fw_workload.h"
#include "viddec_fw_common_defs.h"
#include "viddec_fw_debug.h"

typedef struct
{
    viddec_workload_t *data;
    uint32_t  max_items;
    uint32_t  num_items;
    uint32_t result;
} viddec_emitter_wkld;

typedef struct
{
    viddec_emitter_wkld cur;
    viddec_emitter_wkld next;
} viddec_emitter;

/*
  whats this for? Emitting current tag for ES buffer
*/
int32_t viddec_emit_assoc_tag(viddec_emitter *emit, uint32_t id, uint32_t using_next);

int32_t viddec_emit_contr_tag(viddec_emitter *emit, viddec_input_buffer_t *ibuf, uint8_t incomplete, uint32_t using_next);

int32_t viddec_emit_flush_current_wkld(viddec_emitter *emit);

int32_t viddec_emit_append(viddec_emitter_wkld *cxt, viddec_workload_item_t *item);

/*
  Init function for setting up emitter context.
*/
static inline void viddec_emit_init(viddec_emitter *cxt)
{
    cxt->cur.data = cxt->next.data = 0;
    cxt->cur.max_items = cxt->next.max_items = 0;
    cxt->cur.num_items = cxt->next.num_items = 0;
    cxt->cur.result = cxt->next.result = VIDDEC_FW_WORKLOAD_SUCCESS;
}

static inline void viddec_emit_update(viddec_emitter *cxt, uint32_t cur, uint32_t next, uint32_t cur_size, uint32_t next_size)
{
    cxt->cur.data = (cur != 0) ? (viddec_workload_t *)(cur | DDR_MEM_MASK) : NULL;
    cxt->next.data = (next != 0) ? (viddec_workload_t *)(next | DDR_MEM_MASK): NULL;
    cxt->cur.max_items = (cur_size - sizeof(viddec_workload_t))/sizeof(viddec_workload_item_t);
    cxt->next.max_items = (next_size - sizeof(viddec_workload_t))/sizeof(viddec_workload_item_t);
}

static inline void viddec_emit_time(viddec_emitter *cxt, uint32_t time)
{
    viddec_emitter_wkld *cur;
    cur = &(cxt->cur);
    cur->data->time = time;
}

static inline void viddec_emit_set_codec(viddec_emitter *emit, uint32_t codec_type)
{
    emit->cur.data->codec = (viddec_stream_format)(codec_type);
}

static inline void viddec_emit_set_codec_errors(viddec_emitter *emit, uint32_t codec_error)
{
    emit->cur.result |= codec_error;
    WRITE_SVEN(SVEN_MODULE_EVENT_GV_FW_PM_WORKLOAD_STATUS, (int)emit->cur.result, (int)emit->cur.data,
               (int)emit->cur.num_items, 0, 0, 0);
}

static inline void viddec_emit_set_workload_error(viddec_emitter *emit, uint32_t error, uint32_t using_next)
{
    viddec_emitter_wkld *cur_wkld;
    cur_wkld = (using_next == false)? &(emit->cur):&(emit->next);
    cur_wkld->result |= error;
    WRITE_SVEN(SVEN_MODULE_EVENT_GV_FW_PM_WORKLOAD_STATUS, (int)cur_wkld->result, (int)cur_wkld->data,
               (int)cur_wkld->num_items, using_next, 0, 0);
}

static inline void viddec_emit_set_inband_tag(viddec_emitter *emit, uint32_t type, uint32_t using_next)
{
    viddec_emitter_wkld *cur_wkld;
    viddec_workload_item_t item;
    cur_wkld = (using_next == false)? &(emit->cur):&(emit->next);
    item.vwi_type = (workload_item_type)(type);
    item.vwi_payload[0] = item.vwi_payload[1] = item.vwi_payload[2] = 0;
    viddec_emit_append(cur_wkld, &item);
}

#endif /* VIDDEC_EMITTER_H */
