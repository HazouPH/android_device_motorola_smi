/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 ** http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "LIBAMC_AT_MANAGER"

#include "ATManager.h"
#include "AudioModemControl.h"
#include <utils/Log.h>

extern "C" {

// Singleton access
CATManager* getInstance()
{
    static CATManager amcInstance;
    return &amcInstance;
}


AT_STATUS at_start(const char *pATchannel, uint32_t uiIfxI2s1ClkSelect, uint32_t uiIfxI2s2ClkSelect)
{
    AT_STATUS eStatus = getInstance()->start(pATchannel);

    if (eStatus != AT_OK) {

        return eStatus;
    }
    LOGD("%s: *** ATmodemControl started", __FUNCTION__);
    amc_dest_for_source();
    amc_set_default_clocks(uiIfxI2s1ClkSelect, uiIfxI2s2ClkSelect);
    LOGV("After dest for source init matrix");

    return eStatus;
}

AT_STATUS at_send(const char *pATcmd, const char *pRespPrefix)
{
    CATCommand command(pATcmd, pRespPrefix);

    // Use synchronous sendCommand API.
    // In case of asynchronous, it would require to keep the AT command instance
    // until answer is received
    return getInstance()->sendCommand(&command, true);
}

}

