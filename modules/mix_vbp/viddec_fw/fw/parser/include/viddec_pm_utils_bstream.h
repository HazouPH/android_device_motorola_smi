#ifndef VIDDEC_PM_UTILS_BSTREAM_H
#define VIDDEC_PM_UTILS_BSTREAM_H

#include "viddec_pm_utils_list.h"

#define CUBBY_SIZE 1024
#define SCRATCH_SIZE 20
#define MIN_DATA     8

typedef struct
{
#ifdef VBP
    uint8_t *buf;
#else
    uint8_t buf[CUBBY_SIZE + 8 + MIN_DATA];/* extra 8 bytes for alignmet, extra 8 bytes for old data */
#endif
    uint32_t buf_st; /* start pos in buf */
    uint32_t buf_end; /* first invalid byte in buf */
    uint32_t buf_index; /* current index in buf */
    uint32_t buf_bitoff; /* bit offset in current index position */
} viddec_pm_utils_bstream_buf_cxt_t;

typedef struct
{
    uint8_t  buf_scratch[SCRATCH_SIZE];/* scratch for boundary reads*/
    uint32_t st; /* start index of valid byte */
    uint32_t size;/* Total number of bytes in current buffer */
    uint32_t bitoff; /* bit offset in first valid byte */
} viddec_pm_utils_bstream_scratch_cxt_t;

typedef struct
{
#ifdef VBP
    /* counter of emulation prevention byte */
    uint32_t emulation_byte_counter;
#endif
    /* After First pass of scan we figure out how many bytes are in the current access unit(N bytes). We store
       the bstream buffer's first valid byte index wrt to accessunit in this variable */
    uint32_t au_pos;
    /* This is for keeping track of which list item was used to load data last */
    uint32_t list_off;
    /* This is for tracking emulation prevention bytes */
    uint32_t phase;
    /* This flag tells us whether to look for emulation prevention or not */
    uint32_t is_emul_reqd;
    /* A pointer to list of es buffers which contribute to current access unit */
    viddec_pm_utils_list_t *list;
    /* scratch buffer to stage data on boundaries and reloads */
    viddec_pm_utils_bstream_scratch_cxt_t scratch;
    /* Actual context which has valid data for get bits functionality */
    viddec_pm_utils_bstream_buf_cxt_t bstrm_buf;
} viddec_pm_utils_bstream_cxt_t;

void viddec_pm_utils_bstream_init(viddec_pm_utils_bstream_cxt_t *cxt, viddec_pm_utils_list_t *list, uint32_t is_emul);

int32_t viddec_pm_utils_bstream_skipbits(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t num_bits);

int32_t viddec_pm_utils_bstream_peekbits(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *out, uint32_t num_bits, uint8_t skip);

int32_t viddec_pm_utils_bstream_get_current_byte(viddec_pm_utils_bstream_cxt_t *cxt, uint8_t *byte);

uint8_t viddec_pm_utils_bstream_nomoredata(viddec_pm_utils_bstream_cxt_t *cxt);

uint8_t viddec_pm_utils_bstream_nomorerbspdata(viddec_pm_utils_bstream_cxt_t *cxt);

void viddec_pm_utils_skip_if_current_is_emulation(viddec_pm_utils_bstream_cxt_t *cxt);

/*
  This function gets bit and byte position of where we are in the current AU. We always return the position of next byte to be
  read.
  is_emul on true indicates we are on second zero byte in emulation prevention sequence.
 */
static inline void viddec_pm_utils_bstream_get_au_offsets(viddec_pm_utils_bstream_cxt_t *cxt, uint32_t *bit, uint32_t *byte, uint8_t *is_emul)
{
    uint32_t phase=cxt->phase;

    *bit = cxt->bstrm_buf.buf_bitoff;
    *byte = cxt->au_pos + (cxt->bstrm_buf.buf_index - cxt->bstrm_buf.buf_st);
    if (cxt->phase > 0)
    {
        phase = phase - ((cxt->bstrm_buf.buf_bitoff != 0)? 1: 0 );
    }
    /* Assumption: we will never be parked on 0x3 byte of emulation prevention sequence */
    *is_emul = (cxt->is_emul_reqd) && (phase > 0) &&
               (cxt->bstrm_buf.buf[cxt->bstrm_buf.buf_index] == 0) &&
               (cxt->bstrm_buf.buf[cxt->bstrm_buf.buf_index+1] == 0x3);
}
#endif
