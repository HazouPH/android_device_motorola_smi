#include "fw_pvt.h"
#include "viddec_fw_parser_ipclib_config.h"
#include "viddec_fw_debug.h"

extern uint32_t timer;

void enable_intr(void)
{
    TRAPS_ENABLE;
    TRAPS_INT_ENABLE;
    //reg_write(INT_REG, 0);
}

/*------------------------------------------------------------------------------
 * Function:  mfd_trap_handler
 * This is the FW's ISR, Currently we don't support any INT as we are running parsers only on GV which
 * are pure SW modules.
 *------------------------------------------------------------------------------
 */
void mfd_trap_handler()
{
    uint32_t reg=0, temp=0;
    temp = reg_read(INT_STATUS);
    //DEBUG_WRITE(0xff, temp, timer, 0, 0, 0);
    if (temp & INT_WDOG_ENABLE)
    {
        timer++;
        set_wdog(VIDDEC_WATCHDOG_COUNTER_MAX);
        reg = reg_read(INT_STATUS);
    }
    if (temp & 0x4)
    {

        temp = temp & (~0x4);
        reg_write(INT_REG, temp);
        //val = reg_read(DMA_CONTROL_STATUS);
        //val |=DMA_CTRL_STATUS_DONE;
        //reg_write(DMA_CONTROL_STATUS, val);
        //reg = reg_read(INT_STATUS);
    }
    if (temp & 0x2)
    {

        temp = temp & (~0x2);
        reg_write(INT_REG, temp);
    }

    if (temp & 0x1)
    {
        temp = temp & (~0x1);
        reg_write(INT_REG, temp);
    }
    //DEBUG_WRITE(0xff, timer, temp, reg, 0, val);
    __asm__("nop");

}
