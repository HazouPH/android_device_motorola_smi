#ifndef VIDDEC_PM_COMMON_LIST_H
#define VIDDEC_PM_COMMON_LIST_H

#include "viddec_emitter.h"

/* Limitation:This is the maximum numbers of es buffers between start codes. Needs to change if we encounter
   a case if this is not sufficent */
#ifdef VBP
#define MAX_IBUFS_PER_SC 512
#else
#define MAX_IBUFS_PER_SC 64
#endif

/* This structure is for storing information on byte position in the current access unit.
   stpos is the au byte index of first byte in current es buffer.edpos is the au byte index+1 of last
   valid byte in current es buffer.*/
typedef struct
{
    uint32_t stpos;
    uint32_t edpos;
} viddec_pm_utils_au_bytepos_t;

/* this structure is for storing all necessary information for list handling */
typedef struct
{
    uint16_t num_items;                  /* Number of buffers in List */
    uint16_t first_scprfx_length;        /* Length of first sc prefix in this list */
    int32_t start_offset;                /* starting offset of unused data including sc prefix in first buffer */
    int32_t end_offset;                  /* Offset of unsused data in last buffer including 2nd sc prefix */
    viddec_input_buffer_t sc_ibuf[MAX_IBUFS_PER_SC]; /* Place to store buffer descriptors */
    viddec_pm_utils_au_bytepos_t data[MAX_IBUFS_PER_SC]; /* place to store au byte positions */
    int32_t total_bytes;                 /* total bytes for current access unit including first sc prefix*/
} viddec_pm_utils_list_t;

/* This function initialises the list to default values */
void viddec_pm_utils_list_init(viddec_pm_utils_list_t *cxt);
#ifndef VBP
/* This function adds a new entry to list and will emit tags if needed */
uint32_t viddec_pm_utils_list_addbuf(viddec_pm_utils_list_t *list, viddec_input_buffer_t *es_buf);

/* This function updates au byte position of the current list. This should be called after sc codes are detected and before
   syntax parsing as get bits requires this to be initialized. */
void viddec_pm_utils_list_updatebytepos(viddec_pm_utils_list_t *list, uint8_t sc_prefix_length);

/* This function walks through the list and removes consumed buffers based on total bytes. It then moves
   unused entires to the top of list. */
void viddec_pm_utils_list_remove_used_entries(viddec_pm_utils_list_t *list, uint32_t length);

/* this function returns 1 if the requested byte is not found. If found returns list and offset into list */
uint32_t viddec_pm_utils_list_getbyte_position(viddec_pm_utils_list_t *list, uint32_t byte, uint32_t *list_index, uint32_t *offset);
#endif
#endif
