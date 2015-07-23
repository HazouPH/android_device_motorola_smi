
#include "viddec_pm.h"
#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "viddec_emitter.h"
#include "viddec_fw_workload.h"
#include "viddec_pm_utils_bstream.h"

extern void viddec_pm_utils_list_emit_pixel_tags(viddec_pm_utils_list_t *list, uint32_t start, viddec_emitter *emitter, uint32_t using_next);
extern void viddec_pm_utils_list_emit_slice_tags(viddec_pm_utils_list_t *list, uint32_t start, uint32_t end, viddec_emitter *emitter, uint32_t is_cur_wkld, viddec_workload_item_t *wi);

int32_t viddec_pm_get_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_peekbits(&(cxt->getbits), data, num_bits, 1);
    if (ret == -1)
    {
        DEB("FAILURE!!!! getbits returned %d\n", ret);
    }

    return ret;
}

int32_t viddec_pm_peek_bits(void *parent, uint32_t *data, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_peekbits(&(cxt->getbits), data, num_bits, 0);
    return ret;
}

int32_t viddec_pm_skip_bits(void *parent, uint32_t num_bits)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_skipbits(&(cxt->getbits), num_bits);
    return ret;
}

int32_t viddec_pm_append_workitem(void *parent, viddec_workload_item_t *item, uint32_t next)
{
#ifndef VBP
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;
    viddec_emitter_wkld *emit;

    cxt = (viddec_pm_cxt_t *)parent;
    emit = (next) ? &(cxt->emitter.next) : &(cxt->emitter.cur);
    ret = viddec_emit_append(emit, item);
    return ret;
#else
    return 1;
#endif
}

int32_t viddec_pm_get_au_pos(void *parent, uint32_t *bit, uint32_t *byte, uint8_t *is_emul)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_skip_if_current_is_emulation(&(cxt->getbits));
    viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), bit, byte, is_emul);

    return ret;

}

#ifndef VBP
static inline int32_t viddec_pm_append_restof_pixel_data(void *parent, uint32_t cur_wkld)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;
    uint32_t start=0, b_off=0;
    uint8_t emul=0;
    viddec_workload_item_t wi;

    cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_skip_if_current_is_emulation(&(cxt->getbits));
    viddec_pm_utils_bstream_get_au_offsets(&(cxt->getbits), &b_off, &start, &emul);
    if (emul) start--;

    wi.vwi_type = VIDDEC_WORKLOAD_PIXEL_ES;
    wi.es.es_flags = 0;
    viddec_pm_utils_list_emit_slice_tags(&(cxt->list), start, cxt->list.total_bytes -1, &(cxt->emitter), cur_wkld, &wi);
    return ret;
}

int32_t viddec_pm_append_pixeldata(void *parent)
{
    return viddec_pm_append_restof_pixel_data(parent,  1);
}

int32_t viddec_pm_append_pixeldata_next(void *parent)
{
    return viddec_pm_append_restof_pixel_data(parent,  0);
}
#endif

viddec_workload_t* viddec_pm_get_header(void *parent)
{
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;

    return cxt->emitter.cur.data;
}

viddec_workload_t* viddec_pm_get_next_header(void *parent)
{
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;

    return cxt->emitter.next.data;
}

int32_t viddec_pm_is_nomoredata(void *parent)
{
    int32_t ret=0;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_nomorerbspdata(&(cxt->getbits));
    return ret;
}

uint32_t viddec_pm_get_cur_byte(void *parent, uint8_t *byte)
{
    int32_t ret=-1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    ret = viddec_pm_utils_bstream_get_current_byte(&(cxt->getbits), byte);
    return ret;
}

#ifndef VBP
int32_t viddec_pm_append_misc_tags(void *parent, uint32_t start, uint32_t end, viddec_workload_item_t *wi, uint32_t using_next)
{
    int32_t ret = 1;
    viddec_pm_cxt_t *cxt;

    cxt = (viddec_pm_cxt_t *)parent;
    if (end == VIDDEC_PARSE_INVALID_POS) end = (cxt->list.total_bytes -1);
    viddec_pm_utils_list_emit_slice_tags(&(cxt->list), start, end, &(cxt->emitter), using_next, wi);

    return ret;

}
#endif

void viddec_pm_set_next_frame_error_on_eos(void *parent, uint32_t error)
{
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
    cxt->next_workload_error_eos = error;
}

void viddec_pm_set_late_frame_detect(void *parent)
{
    viddec_pm_cxt_t *cxt;
    cxt = (viddec_pm_cxt_t *)parent;
    cxt->late_frame_detect = true;
}

void viddec_pm_setup_userdata(viddec_workload_item_t *wi)
{
#ifdef  MFDBIGENDIAN
    wi->vwi_payload[0] = SWAP_WORD(wi->vwi_payload[0]);
    wi->vwi_payload[1] = SWAP_WORD(wi->vwi_payload[1]);
    wi->vwi_payload[2] = SWAP_WORD(wi->vwi_payload[2]);
#else
    wi=wi;
#endif
}
