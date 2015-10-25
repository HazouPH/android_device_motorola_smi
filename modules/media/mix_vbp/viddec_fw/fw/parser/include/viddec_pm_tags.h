#ifndef VIDDEC_PM_TAGS_H
#define VIDDEC_PM_TAGS_H

#include "viddec_pm.h"
#include "viddec_emitter.h"

/* Define to initalize temporary association list */
#define INVALID_ENTRY ((uint32_t) -1)

void viddec_pm_generate_tags_for_unused_buffers_to_flush(viddec_pm_cxt_t *cxt);
uint32_t viddec_generic_add_association_tags(void *parent);
uint32_t viddec_h264_add_association_tags(void *parent);
uint32_t viddec_mpeg2_add_association_tags(void *parent);
uint32_t viddec_pm_lateframe_generate_contribution_tags(void *parent, uint32_t ignore_partial);
uint32_t viddec_pm_generic_generate_contribution_tags(void *parent, uint32_t ignore_partial);
uint32_t viddec_pm_generate_missed_association_tags(viddec_pm_cxt_t *cxt, uint32_t using_next);
#endif
