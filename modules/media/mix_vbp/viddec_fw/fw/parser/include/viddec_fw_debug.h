#ifndef VIDDEC_FW_DEBUG_H
#define VIDDEC_FW_DEBUG_H

//#define SWAP_BYTE(x,y,z)   (( ( (x)>>(8*y))& 0xFF)  << (8*z))
#define SWAP_BYTE(x,y,z)   (( ( (x) >> ((y) << 3))& 0xFF)  << ((z) << 3))
#define SWAP_WORD(x)      ( SWAP_BYTE((x),0,3) | SWAP_BYTE((x),1,2) |SWAP_BYTE((x),2,1) |SWAP_BYTE((x),3,0))

#ifndef VBP

#ifndef HOST_ONLY
#define _OSAL_IO_MEMMAP_H  /* to prevent errors when including sven_devh.h */
#define _OSAL_ASSERT_H     /* to prevent errors when including sven_devh.h */
#endif
#include <stdint.h>
#include "viddec_debug.h"
#include "sven_devh.h"
#include "auto_eas/gen4_gv.h"

#ifdef HOST_ONLY
#define DUMP_TO_MEM(x) DEB("0x%.08X ",x);
#define WRITE_SVEN(event, p1, p2, p3, p4, p5, p6) DEB("Sven evnt=0x%.8X p1=%d p2=%d p3=%d p4=%d p5=%d p6=%d\n",event, p1, p2, p3, p4, p5, p6)
#define read_ret(x)
#define read_fp(x)
#define read_sp(x)
#define read_wim(x)
#define read_psr(x)
#else
extern uint32_t dump_ptr;
/* Macros for Dumping data to DDR */
#define DUMP_TO_MEM(x) ((volatile unsigned int *)0x8F000000)[dump_ptr++] = SWAP_WORD(x);
#define read_ret(x) asm("mov %%i7, %0\n":"=r" (x))
#define read_fp(x) asm("mov %%i6, %0\n":"=r" (x))
#define read_sp(x) asm("mov %%sp, %0\n":"=r" (x))
#define read_wim(x) asm("mov %%wim, %0\n":"=r" (x))
#define read_psr(x) asm("mov %%psr, %0\n":"=r" (x))
#define WRITE_SVEN(event, p1, p2, p3, p4, p5, p6) devh_SVEN_WriteModuleEvent( NULL, event, p1, p2, p3, p4, p5, p6)
#endif

#else // VBP is defined

#include <stdint.h>
#include "viddec_debug.h"
#define DUMP_TO_MEM(x)
#define WRITE_SVEN(event, p1, p2, p3, p4, p5, p6)
#define read_ret(x)
#define read_fp(x)
#define read_sp(x)
#define read_wim(x)
#define read_psr(x)


#endif

static inline void DEBUG_WRITE(uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, uint32_t p5, uint32_t p6)
{
    //uint32_t temp;
    DUMP_TO_MEM(0xCACAFEED);
    DUMP_TO_MEM(p1);
    DUMP_TO_MEM(p2);
    DUMP_TO_MEM(p3);
    DUMP_TO_MEM(p4);
    DUMP_TO_MEM(p5);
    DUMP_TO_MEM(p6);
    DUMP_TO_MEM(0xCACA0000);
    //temp = dump_ptr;
    //DUMP_TO_MEM(temp);
}
static inline void DUMP_SPARC_REG(void)
{
    uint32_t ret1, fp, sp, wim, psr;
    read_ret(ret1);
    read_fp(fp);
    read_sp(sp);
    read_wim(wim);
    read_psr(psr);
    //crash = (uint32_t *)0x1000bf0c;
    //DEBUG_WRITE(sp, wim, fp, ret1, (*crash), 0xFED);
    DEBUG_WRITE(sp, wim, fp, ret1, psr, 0xFFFFFFFF);
}
#endif
