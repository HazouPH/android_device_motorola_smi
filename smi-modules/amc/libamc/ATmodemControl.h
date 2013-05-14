/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **	 http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#ifndef AT_MODEM_CONTROL_H
#define AT_MODEM_CONTROL_H



#ifdef __cplusplus
extern "C"
{
#endif
#include <pthread.h>
#ifndef __cplusplus
#define bool int
#define true 1
#define false 0
#endif /* #ifndef __cplusplus*/

#define AT_MAX_CMD_LENGTH 80
#define AT_MAX_RESP_LENGTH 300

#include "ATCmdStatus.h"

/* TTY status */
typedef enum {
    AMC_TTY_OFF,
    AMC_TTY_FULL,
    AMC_TTY_VCO,
    AMC_TTY_HCO
} AMC_TTY_STATE;

AT_STATUS at_start(const char *pATchannel, uint32_t uiIfxI2s1ClkSelect, uint32_t uiIfxI2s2ClkSelect);

AT_STATUS at_send(const char *pATcmd, const char *pRespPrefix);

#ifdef __cplusplus
}
#endif

#endif /*AT_MODEM_CONTROL_H*/

