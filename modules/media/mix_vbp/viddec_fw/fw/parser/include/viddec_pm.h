#ifndef VIDDEC_PM_H
#define VIDDEC_PM_H

#include <stdint.h>
#include "viddec_emitter.h"
#include "viddec_pm_utils_list.h"
#include "viddec_pm_utils_bstream.h"
#include "viddec_pm_parse.h"
#include "viddec_parser_ops.h"

#define SC_DETECT_BUF_SIZE 1024
#define MAX_CODEC_CXT_SIZE 4096

typedef enum
{
    PM_SUCCESS = 0,
    /* Messages to indicate more ES data */
    PM_NO_DATA = 0x100,
    /* Messages to indicate SC found */
    PM_SC_FOUND = 0x200,
    PM_FIRST_SC_FOUND = 0x201,
    /* Messages to indicate Frame done */
    PM_WKLD_DONE = 0x300,
    /* Messages to indicate Error conditions */
    PM_OVERFLOW = 0x400,
    /* Messages to indicate inband conditions */
    PM_INBAND_MESSAGES = 0x500,
    PM_EOS = 0x501,
    PM_DISCONTINUITY = 0x502,
} pm_parse_state_t;

/* This is a temporary structure for first pass sc parsing. index tells us where we are in list of es buffers
   cur_es points to current es buffer we are parsing. */
typedef struct
{
    int32_t list_index; /* current index of list */
    uint32_t cur_offset;
    uint32_t cur_size;
    viddec_input_buffer_t *cur_es;
} viddec_pm_sc_cur_buf_t;

typedef struct
{
    uint32_t pending_tags[MAX_IBUFS_PER_SC];
    uint8_t dummy;
    uint8_t frame_done;
    uint8_t first_buf_aligned;
    uint8_t using_next;
} vidded_pm_pending_tags_t;

/* This structure holds all necessary data required by parser manager for stream parsing.
 */
typedef struct
{
    /* Actual buffer where data gets DMA'd. 8 padding bytes for alignment */
    uint8_t scbuf[SC_DETECT_BUF_SIZE + 8];
    viddec_sc_parse_cubby_cxt_t parse_cubby;
    viddec_pm_utils_list_t list;
    /* Place to store tags to be added to next to next workload */
    viddec_pm_sc_cur_buf_t cur_buf;
    viddec_emitter emitter;
    viddec_pm_utils_bstream_cxt_t getbits;
    viddec_sc_prefix_state_t sc_prefix_info;
    vidded_pm_pending_tags_t pending_tags;
    uint8_t word_align_dummy;
    uint8_t late_frame_detect;
    uint8_t frame_start_found;
    uint8_t found_fm_st_in_current_au;
    uint32_t next_workload_error_eos;
    uint32_t pending_inband_tags;
#ifdef VBP
    uint32_t codec_data[MAX_CODEC_CXT_SIZE<<3];
#else
    uint32_t codec_data[MAX_CODEC_CXT_SIZE>>2];
#endif
} viddec_pm_cxt_t;

/*
 *
 * Functions used by Parser kernel
 *
 */

/* This is for initialising parser manager context to default values */
void viddec_pm_init_context(viddec_pm_cxt_t *cxt, uint32_t codec_type, uint32_t *persist_mem, uint32_t clean);

/* This is the main parse function which returns state information that parser kernel can understand.*/
uint32_t viddec_pm_parse_es_buffer(viddec_pm_cxt_t *cxt, uint32_t codec_type, viddec_input_buffer_t *es_buf);

void viddec_pm_init_ops();

void viddec_pm_update_time(viddec_pm_cxt_t *cxt, uint32_t time);

uint32_t viddec_pm_get_parser_sizes(uint32_t codec_type, viddec_parser_memory_sizes_t *size);
#endif
