/*
 **
 ** Copyright 2010 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **	 http: www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */


#include "AudioModemControl.h"
#ifdef MODEM_IFX_XMM6160

#define LOG_TAG "AudiomodemControlIFX"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "ATmodemControl.h"
#include <stdarg.h>
#include <utils/Log.h>
#include <assert.h>


AT_STATUS amc_enable(AMC_SOURCE source)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    sprintf(cmdStr, EN_SRC, source);
    rts = at_send(cmdStr, EN_SRC_RESP);
    return rts;
}
AT_STATUS amc_disable(AMC_SOURCE source)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    sprintf(cmdStr, DIS_SRC, source);
    rts = at_send(cmdStr, DIS_SRC_RESP);
    return rts;
}

AT_STATUS amc_configure_dest(AMC_DEST dest, IFX_CLK clk, IFX_MASTER_SLAVE mode, IFX_I2S_SR sr, IFX_I2S_SW sw, IFX_I2S_TRANS_MODE trans, IFX_I2S_SETTINGS settings, IFX_I2S_AUDIO_MODE audio, IFX_I2S_UPDATES update, IFX_TRANSDUCER_MODE_DEST transducer_dest)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    snprintf(cmdStr, sizeof(cmdStr), SET_DEST_CONF, dest, 0, clk, mode, sr, sw, trans, settings, audio, update, transducer_dest);
    rts = at_send(cmdStr, SET_DEST_CONF_RESP);
    return rts;
}


AT_STATUS amc_configure_source(AMC_SOURCE source, IFX_CLK clk, IFX_MASTER_SLAVE mode, IFX_I2S_SR sr, IFX_I2S_SW sw, IFX_I2S_TRANS_MODE trans, IFX_I2S_SETTINGS settings, IFX_I2S_AUDIO_MODE audio, IFX_I2S_UPDATES update, IFX_TRANSDUCER_MODE_SOURCE transducer_source)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    snprintf(cmdStr, sizeof(cmdStr), SET_SRC_CONF, source, 0, clk, mode, sr, sw, trans, settings, audio, update, transducer_source);
    rts = at_send(cmdStr, SET_SRC_CONF_RESP);
    return rts;
}

AT_STATUS amc_configure_source_probe(AMC_SOURCE source, AMC_PROBING_POINT probe)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    snprintf(cmdStr, sizeof(cmdStr), SET_SRC_CONF_PROBE, source, probe);
    rts = at_send(cmdStr, SET_SRC_CONF_RESP);
    return rts;
}

AT_STATUS amc_route(destForSourceRoute *destForSource)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    char cmdStrtemp[AT_MAX_CMD_LENGTH];
    int j = 0, nbrdest = 0;
    AT_STATUS rts;
    if(destForSource == NULL)
        return AT_ERROR;
    nbrdest = destForSource->nbrDest;
    sprintf(cmdStr, SET_SRC_DEST, destForSource->source);

    do {
        sprintf(cmdStrtemp, SET_SRC_DEST_TEMP, destForSource->dests[j++]);
        strcat(cmdStr, cmdStrtemp);
        LOGV("ROUTE ARG = %s", cmdStrtemp);
    } while(--nbrdest > 0);

    LOGV("ROUTE CMDSTR = %s",cmdStr);
    rts = at_send(cmdStr, SET_SRC_DEST_RESP);
    return rts;
}


AT_STATUS amc_setGainsource(
    AMC_SOURCE source, int gainDDB)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    sprintf(cmdStr, SET_SRC_VOL, source, gainDDB);
    rts = at_send(cmdStr, SET_SRC_VOL_RESP);
    return rts;
}

AT_STATUS amc_setGaindest(
    AMC_DEST dest, int gainDDB)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    sprintf(cmdStr, SET_DEST_VOL, dest, gainDDB);
    rts = at_send(cmdStr, SET_DEST_VOL_RESP);
    return rts;
}


AT_STATUS amc_setAcoustic(
    AMC_ACOUSTIC acousticProfile)
{
    char cmdStr[AT_MAX_CMD_LENGTH];
    AT_STATUS rts;
    /*,<allow_echo_canceller>,<allow_agc>,<allow_tx_noise_reduction>*/
    if (acousticProfile == AMC_HANDFREE)
        sprintf(cmdStr, MODIFY_HF,1,1,1);
    else
        sprintf(cmdStr, MODIFY_HF,0,0,0);
    rts = at_send(cmdStr, MODIFY_HF_RESP);
    return rts;
}

#endif /*MODEM_IFX_XMM6160*/


