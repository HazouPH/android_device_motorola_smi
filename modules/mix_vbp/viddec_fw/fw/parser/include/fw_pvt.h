#ifndef FW_PVT_H
#define FW_PVT_H

#include <stdint.h>
#include "viddec_fw_parser_fw_ipc.h"
#include "viddec_fw_parser_ipclib_config.h"
#include "viddec_emitter.h"
#include "viddec_pm.h"
#include "viddec_fw_debug.h"

#define GET_IPC_HANDLE(x) (FW_IPC_Handle *)&(x.fwIpc)
#define GV_DDR_MEM_MASK 0x80000000
/* Macros for Interrupts */
#define TRAPS_ENABLE  __asm__ volatile ("mov %%psr, %%l0; or  %%l0,  0x20, %%l0; mov %%l0, %%psr; nop; nop; nop;":::"l0")
#define TRAPS_DISABLE __asm__ volatile ("mov %%psr, %%l0; and %%l0, ~0x20, %%l0; mov %%l0, %%psr; nop; nop; nop;":::"l0")

#define TRAPS_INT_ENABLE  __asm__ volatile ("mov %%psr, %%l0; and %%l0, ~0xF00, %%l0; mov %%l0, %%psr; nop; nop; nop;":::"l0")
#define TRAPS_INT_DISABLE __asm__ volatile ("mov %%psr, %%l0; or  %%l0,  0xF00, %%l0; mov %%l0, %%psr; nop; nop; nop;":::"l0")

#define TRAPS_ENABLED(enabled) __asm__ volatile ("mov %%psr, %0; and %0, 0x20, %0": "=r" (enabled):)

#define TRAPS_INT_DISABLED(enabled) __asm__ volatile ("mov %%psr, %0; and %0, 0xF00, %0": "=r" (enabled):)

#define VIDDEC_WATCHDOG_COUNTER_MAX    (0x000FFFFF)

/* Synchronous message buffer, which is shared by both Host and Fw for handling synchronous messages */
typedef struct
{
    uint8_t data[CONFIG_IPC_SYNC_MESSAGE_BUF_SIZE];
} mfd_sync_msg_t;

/* Required Information needed by Parser Kernel for each stream */
typedef struct
{
    uint32_t  ddr_cxt;    /* phys addr of swap space where Parser kernel stores pvt information */
    uint32_t  cxt_size;   /* size of context buffer */
    uint32_t  strm_type;  /* Current stream information*/
    uint32_t  wl_time;    /* ticks for processing current workload */
    uint32_t  es_time;    /* ticks for processing current workload */
    uint32_t  low_watermark; /* On crossing this value we generate low watermark interrupt */
    uint8_t   state;      /* Current state of stream ... start(1), stop(0).. */
    uint8_t   priority;   /* Priority of current stream Real time or Non real time */
    uint8_t   buffered_data;/* Do we have data from past buffer */
    uint8_t   pending_interrupt;/* Whether an Interrupt needs to be generated for this stream */
} mfd_stream_info;

/* Global data for Parser kernel */
typedef struct
{
    int32_t  low_id; /* last scheduled low priority stream id */
    int32_t  high_id;/* last scheduled high priority stream id */
    uint32_t g_parser_tables; /* should point to global_parser_table in DDR */
} mfd_pk_data_t;

typedef struct
{
    ipc_msg_data input;
    ipc_msg_data wkld1;
    ipc_msg_data wkld2;
    viddec_pm_cxt_t pm;
} mfd_pk_strm_cxt;

/* This structure defines the layout of local memory */
typedef struct
{
    mfd_sync_msg_t  buf;
    _IPC_int_state_t int_status[FW_SUPPORTED_STREAMS];
    FW_IPC_Handle   fwIpc;
    mfd_stream_info stream_info[FW_SUPPORTED_STREAMS];
    mfd_pk_data_t   g_pk_data;
    mfd_pk_strm_cxt  srm_cxt;
} dmem_t;

/* Pvt Functions which will be used by multiple modules */

static inline void reg_write(uint32_t offset, uint32_t value)
{
    *((volatile uint32_t*) (GV_SI_MMR_BASE_ADDRESS + offset)) = value;
}

static inline uint32_t reg_read(uint32_t offset)
{
    uint32_t value=0;
    value = *((volatile uint32_t*) (GV_SI_MMR_BASE_ADDRESS + offset));
    return value;
}


static inline void DEBUG(uint32_t print, uint32_t code, uint32_t val)
{
    if (print > 0)
    {
        DUMP_TO_MEM(code);
        DUMP_TO_MEM(val);
        dump_ptr = (dump_ptr + 7) & ~0x7;
    }
}

void *memcpy(void *dest, const void *src, uint32_t n);

void *memset(void *s, int32_t c, uint32_t n);

uint32_t cp_using_dma(uint32_t ddr_addr, uint32_t local_addr, uint32_t size, char to_ddr, char swap);

uint32_t set_wdog(uint32_t offset);

void get_wdog(uint32_t *value);

void enable_intr(void);

uint32_t get_total_ticks(uint32_t start, uint32_t end);

void viddec_fw_init_swap_memory(unsigned int stream_id, unsigned int swap, unsigned int clean);
#endif
