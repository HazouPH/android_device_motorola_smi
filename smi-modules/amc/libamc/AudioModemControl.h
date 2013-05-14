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

#ifndef AUDIO_MODEM_CONTROL_H
#define AUDIO_MODEM_CONTROL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ATmodemControl.h"
#include <pthread.h>

#define MODEM_IFX_XMM6160
#define AMC_MAX_CMD_LENGTH   AT_MAX_CMD_LENGTH
#define AMC_MAX_RESP_LENGTH  AT_MAX_RESP_LENGTH
#define MODEM_TTY_RETRY 60
#define AUDIO_AT_CHANNEL_NAME "/dev/gsmtty13"
#define GET_USE_CASE_SRC  "AT+XDRV=40,0,"
#define GET_USE_CASE_DEST  "AT+XDRV=40,1,"
#define EN_SRC  "AT+XDRV=40,2,%i"
#define EN_SRC_RESP "+XDRV: 40,2,"
#define DIS_SRC "AT+XDRV=40,3,%i"
#define DIS_SRC_RESP "+XDRV: 40,3,"
#define SET_SRC_CONF "AT+XDRV=40,4,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i"
#define SET_SRC_CONF_PROBE "AT+XDRV=40,4,%i,%i"
#define SET_SRC_CONF_RESP "+XDRV: 40,4,"
#define SET_DEST_CONF "AT+XDRV=40,5,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i"
#define SET_DEST_CONF_RESP "+XDRV: 40,5,"
#define SET_RX_SRC_CONF "AT+XDRV=40,4,%i,%i"
#define SET_RX_SRC_CONF_RESP "+XDRV: 40,4,"
#define SET_TX_DEST_CONF "AT+XDRV=40,5,%i,%i"
#define SET_TX_DEST_CONF_RESP "+XDRV: 40,5,"
#define SET_SRC_DEST "AT+XDRV=40,6,%i"
#define SET_SRC_DEST_TEMP ",%i"
#define SET_SRC_DEST_RESP "+XDRV: 40,6,"
#define SET_SRC_VOL "AT+XDRV=40,7,%i,%i"
#define SET_SRC_VOL_RESP "+XDRV: 40,7,"
#define SET_DEST_VOL "AT+XDRV=40,8,%i,%i"
#define SET_DEST_VOL_RESP "+XDRV: 40,8,"
#define MODIFY_HF "AT+XDRV=40,11,%i,%i,%i"
#define MODIFY_HF_RESP "+XDRV: 40,11,"
#define SET_RTD "AT+XDRV = 40,12,"
#define NBR_DEST_MAX 6
#define MAX_SOURCE_FOR_DEST 7
#define NBR_ROUTE 7
#define NBR_DEST 3

typedef enum {
    AMC_RADIO_TX = 0,
    AMC_RADIO_ANALOG_OUT = 1,
    AMC_I2S1_TX = 2,
    AMC_I2S2_TX = 3,
    AMC_PCM_GENERALD = 4,
    AMC_SPEECH_DL_EXTRACT = 5,
    AMC_SPEECH_UL_EXTRACT = 6,
    AMC_PROBE_OUT = 7,
    AMC_ENDD = 8
} AMC_DEST;

typedef enum {
    AMC_RADIO_RX = 0,
    AMC_RADIO_ANALOG_IN = 1,
    AMC_DIGITAL_MIC = 2,
    AMC_I2S1_RX = 3,
    AMC_I2S2_RX = 4,
    AMC_SIMPLE_TONES = 5,
    AMC_PCM_GENERALS = 6,
    AMC_SPEECH_DL_INJECT = 7,
    AMC_SPEECH_UL_INJECT = 8,
    AMC_INTERNAL_FM_RADIO= 9,
    AMC_PROBE_IN = 10,
    AMC_DECODER = 11,
    AMC_PROBE_IN_A = 12,
    AMC_PROBE_IN_B = 13,
    AMC_PROBE_IN_C = 14,
    AMC_ENDS = 15
} AMC_SOURCE;

typedef enum {
    IFX_CLK0 = 0,
    IFX_CLK1 = 1
} IFX_CLK ;

typedef enum {
    IFX_MASTER = 0,
    IFX_SLAVE = 1
} IFX_MASTER_SLAVE;

typedef enum   {
    IFX_SR_8KHZ = 0,
    IFX_SR_11KHZ = 1 ,
    IFX_SR_12KHZ = 2,
    IFX_SR_16KHZ = 3,
    IFX_SR_22KHZ = 4,
    IFX_SR_24KHZ = 5,
    IFX_SR_32KHZ = 6,
    IFX_SR_44KHZ = 7,
    IFX_SR_48KHZ = 8,
    IFX_SR_96KHZ = 9
} IFX_I2S_SR ;

typedef enum   {
    IFX_SW_16 = 0,
    IFX_SW_18 = 1,
    IFX_SW_20 = 2,
    IFX_SW_24 = 3,
    IFX_SW_32 = 4,
    IFX_SW_END = 5
} IFX_I2S_SW ;

typedef enum {
    IFX_PCM = 0,
    IFX_NORMAL = 1,
    IFX_PCM_BURST = 2
}  IFX_I2S_TRANS_MODE ;

typedef enum {
    I2S_SETTING_NORMAL = 0,
    I2S_SETTING_SPECIAL_1 = 1,
    I2S_SETTING_SPECIAL_2 = 2,
    I2S_SETTING_END = 3
} IFX_I2S_SETTINGS;

typedef enum   {
    IFX_MONO = 0,
    IFX_DUAL_MONO = 1,
    IFX_STEREO = 2,
    IFX_END = 3
} IFX_I2S_AUDIO_MODE ;

typedef enum  {
    IFX_UPDATE_ALL = 0,
    IFX_UPDATE_HW = 1,
    IFX_UPDATE_TRANSDUCER = 2
} IFX_I2S_UPDATES ;

typedef enum  {
    UTA_AUDIO_ANALOG_SRC_HANDSET_MIC = 0,
    UTA_AUDIO_ANALOG_SRC_HEADSET_MIC,
    UTA_AUDIO_ANALOG_SRC_HF_MIC,
    UTA_AUDIO_ANALOG_SRC_AUX,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_1,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_2,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_3,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_4,
    UTA_AUDIO_ANALOG_SRC_USER_DEFINED_5,
    UTA_AUDIO_ANALOG_SRC_TTY
} S_UTA_AUDIO_ANALOG_SRC;

typedef enum  {
    UTA_AUDIO_ANALOG_DEST_HANDSET = 0,
    UTA_AUDIO_ANALOG_DEST_HEADSET,
    UTA_AUDIO_ANALOG_DEST_BACKSPEAKER,
    UTA_AUDIO_ANALOG_DEST_HEADSET_PLUS_BACKSPEAKER,
    UTA_AUDIO_ANALOG_DEST_HEADSET_PLUS_HANDSET,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_1,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_2,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_3,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_4,
    UTA_AUDIO_ANALOG_DEST_USER_DEFINED_5,
    UTA_AUDIO_ANALOG_DEST_TTY
} S_UTA_AUDIO_ANALOG_DEST;

typedef enum   {
    IFX_DEFAULT_S = 0,
    IFX_HANDSET_S = 1,
    IFX_HEADSET_S = 2,
    IFX_HF_S = 3,
    IFX_AUX_S = 4,
    IFX_TTY_S = 5,
    IFX_BLUETOOTH_S = 6,
    IFX_USER_DEFINED_1_S = 7,
    IFX_USER_DEFINED_2_S = 8,
    IFX_USER_DEFINED_3_S = 9,
    IFX_USER_DEFINED_4_S = 10,
    IFX_USER_DEFINED_5_S = 11,
    IFX_USER_DEFINED_15_S = 21,
} IFX_TRANSDUCER_MODE_SOURCE;

typedef enum   {
    IFX_DEFAULT_D = 0,
    IFX_HANDSET_D = 1,
    IFX_HEADSET_D = 2,
    IFX_BACKSPEAKER_D = 3,
    IFX_HEADSET_PLUS_BACKSPEAKER_D = 4,
    IFX_HEADSET_PLUS_HANDSET_D = 5,
    IFX_TTY_D = 6,
    IFX_BLUETOOTH_D = 7,
    IFX_USER_DEFINED_1_D = 8,
    IFX_USER_DEFINED_2_D = 9,
    IFX_USER_DEFINED_3_D = 10,
    IFX_USER_DEFINED_4_D = 11,
    IFX_USER_DEFINED_5_D = 12,
    IFX_USER_DEFINED_15_D = 22,
} IFX_TRANSDUCER_MODE_DEST;

typedef enum {
    AMC_NORMAL,
    AMC_HANDSET,
    AMC_HEADSET,
    AMC_HANDFREE
} AMC_ACOUSTIC;

typedef enum {
    ROUTE_RADIO,
    ROUTE_I2S1,
    ROUTE_I2S2,
    ROUTE_TONE,
    ROUTE_RECORD_RADIO,
    ROUTE_RECORD_I2S1,
    ROUTE_DISCONNECT_RADIO,
} AMC_ROUTE_ID;

typedef enum {
    PROBING_POINT_DEFAULT = 0,
    PROBING_POINT_SPEECH_ENCODER_IN = 13
} AMC_PROBING_POINT;

typedef struct destForSourceRoute {
    int nbrDest;
    AMC_SOURCE source;
    AMC_DEST *dests;
} destForSourceRoute;


typedef enum {
    AMC_VOICE_CALL_SOURCE,
    AMC_VOICE_UPLINK_SOURCE,
    AMC_VOICE_DOWNLINK_SOURCE,
    AMC_VOICE_INVALID_SOURCE
} AMC_VOICE_RECORD_SOURCE;

AT_STATUS amc_enable(AMC_SOURCE source);
AT_STATUS amc_disable(AMC_SOURCE source);
AT_STATUS amc_configure_dest(AMC_DEST dest, IFX_CLK clk, IFX_MASTER_SLAVE mode,
        IFX_I2S_SR sr, IFX_I2S_SW sw, IFX_I2S_TRANS_MODE trans, IFX_I2S_SETTINGS settings,
        IFX_I2S_AUDIO_MODE audio, IFX_I2S_UPDATES update, IFX_TRANSDUCER_MODE_DEST transducer_dest);
AT_STATUS amc_configure_source(AMC_SOURCE source, IFX_CLK clk, IFX_MASTER_SLAVE mode, IFX_I2S_SR sr,
        IFX_I2S_SW sw, IFX_I2S_TRANS_MODE trans, IFX_I2S_SETTINGS settings, IFX_I2S_AUDIO_MODE audio,
        IFX_I2S_UPDATES update, IFX_TRANSDUCER_MODE_SOURCE transducer_source);
AT_STATUS amc_configure_mic(AMC_SOURCE source, int mode);
AT_STATUS amc_configure_source_probe(AMC_SOURCE source, AMC_PROBING_POINT probe);
AT_STATUS amc_route(destForSourceRoute *destForSource);
AT_STATUS amc_setGainsource(AMC_SOURCE source, int gainDDB);
AT_STATUS amc_setGaindest(AMC_DEST dest, int gainDDB);
AT_STATUS amc_setAcoustic(AMC_ACOUSTIC acousticProfile);

int amc_voice_record_source_enable(AMC_VOICE_RECORD_SOURCE source, bool enable);
void amc_dest_for_source(void);
int amc_conf_i2s1(AMC_TTY_STATE tty, IFX_TRANSDUCER_MODE_SOURCE modeSource, IFX_TRANSDUCER_MODE_DEST modeDest);
int amc_conf_i2s2_route();
int amc_modem_conf_bt_dev(IFX_TRANSDUCER_MODE_SOURCE modeSource, IFX_TRANSDUCER_MODE_DEST modeDest);
int amc_off();
int amc_on();
int amc_mute();
int amc_unmute(int gainDL, int gainUL);
void amc_set_default_clocks(uint32_t uiIfxI2s1ClkSelect, uint32_t uiIfxI2s2ClkSelect);

#ifdef __cplusplus
}
#endif

#endif /*AUDIO_MODEM_CONTROL_H*/

