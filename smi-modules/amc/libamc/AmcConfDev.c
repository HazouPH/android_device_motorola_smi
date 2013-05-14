/*
 **
 ** Copyright 2010 Intel Corporation
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

#include "AudioModemControl.h"
#include "ATmodemControl.h"
#define LOG_TAG "Amc_dev_conf"
#include <utils/Log.h>
#include <stdarg.h>
#include <malloc.h>
#include <unistd.h>

static  destForSourceRoute *pdestForSource[NBR_ROUTE] = { NULL, };
// Default clock selection
static uint32_t guiIfxI2s1ClkSelect, guiIfxI2s2ClkSelect;

static void get_route_id(AMC_ROUTE_ID route, destForSourceRoute *pdestForSource);

void amc_dest_for_source()
{
    int i, j;
    LOGD("Dest for source init enter");
    destForSourceRoute *pdestForSourceTmp[NBR_ROUTE] = { NULL, };

    for (i = 0; i < NBR_ROUTE; i++) {
        pdestForSourceTmp[i] = (destForSourceRoute*) malloc(sizeof(destForSourceRoute));
        if (!pdestForSourceTmp[i]) {
            for (j = i; j >= 0; j--) {
                free(pdestForSourceTmp[j]);
            }
            LOGE("Dest for source error 1rst malloc");
            return;
        }
        else {
            pdestForSourceTmp[i]->dests = (AMC_DEST*) malloc(sizeof(AMC_DEST) * NBR_DEST);
            if (!pdestForSourceTmp[i]->dests) {
                for (j = i; j >= 0; j--) {
                    free(pdestForSourceTmp[j]);
                }
                LOGE("Dest for source error 2nd malloc");
                return;
            }
        }
        get_route_id((AMC_ROUTE_ID)i, &pdestForSourceTmp[i][0]);
        pdestForSource[i] = pdestForSourceTmp[i];
    }
}

void amc_set_default_clocks(uint32_t uiIfxI2s1ClkSelect, uint32_t uiIfxI2s2ClkSelect)
{
    guiIfxI2s1ClkSelect = uiIfxI2s1ClkSelect;
    guiIfxI2s2ClkSelect = uiIfxI2s2ClkSelect;
}

void get_route_id(AMC_ROUTE_ID route, destForSourceRoute *pdestForSource)
{
    switch (route)
    {
    case ROUTE_RADIO:
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_RADIO_RX;
        pdestForSource->dests[0] = AMC_I2S1_TX;
        break;
    case ROUTE_I2S1:
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_I2S1_RX;
        pdestForSource->dests[0] = AMC_RADIO_TX;
        break;
    case ROUTE_I2S2:
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_I2S2_RX;
        pdestForSource->dests[0] = AMC_I2S1_TX;
        break;
    case ROUTE_TONE:
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_SIMPLE_TONES;
        pdestForSource->dests[0] = AMC_I2S1_TX;
        break;
    case ROUTE_RECORD_RADIO:
        pdestForSource->nbrDest = 2;
        pdestForSource->source = AMC_RADIO_RX;
        pdestForSource->dests[0] = AMC_I2S1_TX;
        pdestForSource->dests[1] = AMC_I2S2_TX;
        break;
    case ROUTE_RECORD_I2S1:
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
        // Record uplink before modem voice processing
        pdestForSource->nbrDest = 2;
        pdestForSource->source = AMC_I2S1_RX;
        pdestForSource->dests[0] = AMC_RADIO_TX;
        pdestForSource->dests[1] = AMC_I2S2_TX;
#else
        // Record uplink after modem voice processing
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_PROBE_IN;
        pdestForSource->dests[0] = AMC_I2S2_TX;
#endif
        break;
    case ROUTE_DISCONNECT_RADIO:
        pdestForSource->nbrDest = 1;
        pdestForSource->source = AMC_RADIO_RX;
        pdestForSource->dests[0] = AMC_PCM_GENERALD;
        break;

    default :
        break;
    }
}

int amc_conf_i2s1(AMC_TTY_STATE tty, IFX_TRANSDUCER_MODE_SOURCE modeSource, IFX_TRANSDUCER_MODE_DEST modeDest)
{
    // Check TTY mode
    switch (tty)
    {
    case AMC_TTY_OFF:
        break;
    case AMC_TTY_FULL:
        modeSource = IFX_TTY_S;
        modeDest = IFX_TTY_D;
        break;
    case AMC_TTY_VCO:
        modeDest = IFX_TTY_D;
        break;
    case AMC_TTY_HCO:
        modeSource = IFX_TTY_S;
        break;
    default:
        return -1;
    }

    // Configure I2S1
    LOGD("mode_Source: %d\nmode_Dest:%d\n tty_mode:%d",modeSource, modeDest, tty);
    amc_configure_source(AMC_I2S1_RX, guiIfxI2s1ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, modeSource);
    amc_configure_dest(AMC_I2S1_TX, guiIfxI2s1ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, modeDest);

#ifndef CUSTOM_BOARD_WITH_AUDIENCE
    amc_configure_source_probe(AMC_PROBE_IN_A, PROBING_POINT_SPEECH_ENCODER_IN);
#endif

    return 0;
}

int amc_conf_i2s2_route()
{
    // Configure I2S2
    amc_configure_source(AMC_I2S2_RX, guiIfxI2s2ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, IFX_USER_DEFINED_15_S);
    amc_configure_dest(AMC_I2S2_TX, guiIfxI2s2ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, IFX_USER_DEFINED_15_D);

    // Route
    amc_route(&pdestForSource[ROUTE_DISCONNECT_RADIO][0]);
    amc_route(&pdestForSource[ROUTE_I2S1][0]);
    amc_route(&pdestForSource[ROUTE_I2S2][0]);
    amc_route(&pdestForSource[ROUTE_TONE][0]);
    return 0;
}

int amc_modem_conf_bt_dev(IFX_TRANSDUCER_MODE_SOURCE modeSource, IFX_TRANSDUCER_MODE_DEST modeDest)
{
    // Configure I2S1
    LOGD("mode_Source(BT): %d\nmode_Dest(BT):%d\n",modeSource, modeDest);
    amc_configure_source(AMC_I2S1_RX, guiIfxI2s1ClkSelect, IFX_MASTER, IFX_SR_8KHZ, IFX_SW_16, IFX_PCM, I2S_SETTING_NORMAL, IFX_MONO, IFX_UPDATE_ALL, modeSource);
    amc_configure_dest(AMC_I2S1_TX, guiIfxI2s1ClkSelect, IFX_MASTER, IFX_SR_8KHZ, IFX_SW_16, IFX_PCM, I2S_SETTING_NORMAL, IFX_MONO, IFX_UPDATE_ALL, modeDest);

    // Configure I2S2
    amc_configure_source(AMC_I2S2_RX, guiIfxI2s2ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, IFX_USER_DEFINED_15_S);
    amc_configure_dest(AMC_I2S2_TX, guiIfxI2s2ClkSelect, IFX_MASTER, IFX_SR_48KHZ, IFX_SW_16, IFX_NORMAL, I2S_SETTING_NORMAL, IFX_STEREO, IFX_UPDATE_ALL, IFX_USER_DEFINED_15_D);

    // Route
    amc_route(&pdestForSource[ROUTE_DISCONNECT_RADIO][0]);
    amc_route(&pdestForSource[ROUTE_I2S1][0]);
    amc_route(&pdestForSource[ROUTE_I2S2][0]);
    amc_route(&pdestForSource[ROUTE_TONE][0]);
    return 0;
}

int amc_off()
{
    usleep(1000); // Time to disable MSIC...
    amc_route(&pdestForSource[ROUTE_DISCONNECT_RADIO][0]);
    amc_disable(AMC_I2S1_RX);
    amc_disable(AMC_I2S2_RX);
    usleep(80000); // Time to Disable modem I2S...
    return 0;
}

int amc_on()
{
    amc_enable(AMC_I2S2_RX);
    amc_enable(AMC_I2S1_RX);
    amc_route(&pdestForSource[ROUTE_RADIO][0]);
    usleep(40000); // Time to Enable modem I2S...
    return 0;
}

int amc_mute()
{
    amc_setGaindest(AMC_I2S1_TX, 0);
    amc_setGaindest(AMC_RADIO_TX, 0);
    usleep(1000); // Time to mute
    return 0;
}

int amc_unmute(int gainDL, int gainUL)
{
    amc_setGaindest(AMC_I2S1_TX, gainDL);
    amc_setGaindest(AMC_RADIO_TX, gainUL);
    return 0;
}

int amc_voice_record_source_enable(AMC_VOICE_RECORD_SOURCE source, bool enable)
{
    int route_index;
#ifndef CUSTOM_BOARD_WITH_AUDIENCE
    if( (source != AMC_VOICE_DOWNLINK_SOURCE)) {
        if(enable){
            amc_enable(AMC_PROBE_IN);
            amc_enable(AMC_PROBE_IN_A);
        }else{
            amc_disable(AMC_PROBE_IN);
            amc_disable(AMC_PROBE_IN_A);
        }
    }
#endif /* CUSTOM_BOARD_WITH_AUDIENCE */

    if (source == AMC_VOICE_DOWNLINK_SOURCE || source == AMC_VOICE_CALL_SOURCE) {
        route_index = ( (enable) ? ROUTE_RECORD_RADIO : ROUTE_RADIO);
        amc_route(&pdestForSource[route_index][0]);
    }
    if (source == AMC_VOICE_UPLINK_SOURCE || source == AMC_VOICE_CALL_SOURCE) {
        route_index = ( (enable) ? ROUTE_RECORD_I2S1 : ROUTE_I2S1);
        amc_route(&pdestForSource[route_index][0]);
    }

    return 0;
}
