#include "viddec_mpeg2.h"
#include "viddec_fw_item_types.h"


void viddec_mpeg2_append_workitem(void *parent, viddec_workload_item_t *wi, uint8_t flag)
{
    return;
}

void viddec_mpeg2_emit_workload(void *parent, void *ctxt)
{
    return;
}

void viddec_mpeg2_append_pixeldata(void *parent, uint8_t flag)
{
    return;
}

viddec_workload_t*  viddec_mpeg2_get_header (void *parent, uint8_t flag)
{
    viddec_workload_t *ret;
    if (flag)
    {
        ret = viddec_pm_get_next_header(parent);
    }
    else
    {
        ret = viddec_pm_get_header(parent);
    }
    return ret;
}
