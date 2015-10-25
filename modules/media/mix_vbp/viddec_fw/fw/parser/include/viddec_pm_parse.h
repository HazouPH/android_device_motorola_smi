#ifndef VIDDEC_PM_PARSE_H
#define VIDDEC_PM_PARSE_H

#include <stdint.h>
/* This structure is used by first pass parsing(sc detect), the pm passes information on number of bytes
   that needs to be parsed and if start code found then sc_end_pos contains the index of last sc code byte
   in the current buffer */
typedef struct
{
    uint32_t size; /* size pointed to by buf */
    uint8_t *buf;  /* ptr to data */
    int32_t sc_end_pos; /* return value end position of sc */
    uint32_t phase; /* phase information(state) for sc */
} viddec_sc_parse_cubby_cxt_t;

typedef struct
{
    uint16_t next_sc;
    uint8_t  second_scprfx_length;
    uint8_t  first_sc_detect;
} viddec_sc_prefix_state_t;

uint32_t viddec_parse_sc(void *in, void *pcxt, void *sc_state);
#endif
