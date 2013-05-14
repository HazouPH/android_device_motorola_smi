/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef __VPC_HARDWARE_H__
#define __VPC_HARDWARE_H__

#include <hardware/hardware.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum { VPC_ROUTE_OPEN, VPC_ROUTE_CLOSE } vpc_route_t;
#ifdef ENABLE_TTY_PROFILE
typedef enum { VPC_TTY_OFF, VPC_TTY_FULL, VPC_TTY_VCO, VPC_TTY_HCO, VPC_TTY_INVALID} vpc_tty_t;
#else
typedef enum { VPC_TTY_OFF, VPC_TTY_FULL, VPC_TTY_VCO, VPC_TTY_HCO } vpc_tty_t;
#endif
typedef enum { VPC_BT_NREC_OFF, VPC_BT_NREC_ON } vpc_bt_nrec_t;
typedef enum { VPC_HAC_OFF, VPC_HAC_ON } vpc_hac_set_t;
typedef enum { VPC_BAND_NARROW, VPC_BAND_WIDE, VPC_BAND_INVALID} vpc_band_t;

/* VPC module struct */
#define VPC_HARDWARE_MODULE_ID "vpc"
#define VPC_HARDWARE_NAME      "vpc"

typedef struct vpc_device_t {
    hw_device_t common;

    int (*init)(uint32_t ifx_i2s1_clk_select, uint32_t ifx_i2s2_clk_select, bool have_modem);
    int (*params)(int mode, uint32_t device);
    void (*set_mode)(int mode);
    void (*set_input_source)(int input_source);
    void (*set_call_status)(bool isConnected);
    void (*set_modem_state)(int state);
    int (*route)(vpc_route_t);
    int (*volume)(float);
    int (*mix_disable)(bool isOut);
    int (*mix_enable)(bool isOut, uint32_t device);
    int (*set_tty)(vpc_tty_t);
    int (*bt_nrec)(vpc_bt_nrec_t);
    int (*set_hac)(vpc_hac_set_t);
    int (*set_bt_sco_path)(vpc_route_t);
    void (*set_band)(vpc_band_t band, int for_mode);
} vpc_device_t;

#ifdef __cplusplus
}
#endif

#endif /* __VPC_HARDWARE_H__ */

