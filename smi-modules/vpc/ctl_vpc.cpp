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

#define LOG_TAG "VPC_Module"
#include <utils/Log.h>

#include <utils/threads.h>
#include <hardware_legacy/AudioSystemLegacy.h>

#include "AudioModemControl.h"
#include "bt.h"
#include "msic.h"
#include "volume_keys.h"
#include "stmd.h"
#include "vpc_hardware.h"
#include "acoustic.h"

namespace android_audio_legacy
{

/*===========================================================================*/
/* API                                                                       */
/*===========================================================================*/

static int vpc_init(uint32_t ifx_i2s1_clk_select, uint32_t ifx_i2s2_clk_select, bool have_modem);
static int vpc_params(int mode, uint32_t device);
static void vpc_set_mode(int mode);
static void vpc_set_input_source(int input_source);
static void vpc_set_modem_state(int state);
static int vpc_route(vpc_route_t);
static int vpc_volume(float);
static int vpc_mixing_disable(bool isOut);
static int vpc_mixing_enable(bool isOut, uint32_t device);
static int vpc_set_tty(vpc_tty_t);
static void translate_to_amc_device(const uint32_t current_device, IFX_TRANSDUCER_MODE_SOURCE* mode_source, IFX_TRANSDUCER_MODE_DEST* mode_dest);
static int vpc_bt_nrec(vpc_bt_nrec_t);
static int vpc_set_hac(vpc_hac_set_t);
static void vpc_set_band(vpc_band_t band, int for_mode);


/*===========================================================================*/
/* Definitions                                                               */
/*===========================================================================*/

#define DEVICE_OUT_BLUETOOTH_SCO_ALL (AudioSystem::DEVICE_OUT_BLUETOOTH_SCO | \
                                      AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET | \
                                      AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT)

#define CURRENT_BAND_FOR_MODE(mode) ((mode) == AudioSystem::MODE_IN_CALL ? current_csv_band:current_voip_band)

#define MODEM_GAIN_0dB     88
#define MODEM_GAIN_6dB     100

static const AMC_TTY_STATE translate_vpc_to_amc_tty[] = {
    AMC_TTY_OFF, /*[VPC_TTY_OFF] */
    AMC_TTY_FULL, /*[VPC_TTY_FULL] */
    AMC_TTY_VCO, /*[VPC_TTY_VCO] */
    AMC_TTY_HCO, /*[VPC_TTY_HCO] */
};

const uint32_t DEFAULT_IS21_CLOCK_SELECTION = IFX_CLK1;
const uint32_t DEFAULT_IS22_CLOCK_SELECTION = IFX_CLK0;

/* Delay in ms to wait after es305b wakeup before to send a command */
#define ES305B_WAKEUP_DELAY     20000

/*---------------------------------------------------------------------------*/
/* Global variables                                                          */
/*---------------------------------------------------------------------------*/
using android::Mutex;
Mutex vpc_lock;


static int       prev_mode             = AudioSystem::MODE_NORMAL;
static int       current_mode          = AudioSystem::MODE_NORMAL;
static int       current_input_source  = AUDIO_SOURCE_DEFAULT;
static uint32_t  prev_device           = 0x0000;
static uint32_t  current_device        = 0x0000;
static uint32_t  device_out_defaut     = 0x8000;
static bool      at_thread_init        = false;
#ifdef ENABLE_TTY_PROFILE
static vpc_tty_t current_tty_call      = VPC_TTY_OFF;
static vpc_tty_t previous_tty_call     = VPC_TTY_OFF;
#else
static AMC_TTY_STATE current_tty_call  = AMC_TTY_OFF;
static AMC_TTY_STATE previous_tty_call = AMC_TTY_OFF;
#endif
static bool      mixing_enable         = false;
static bool      voice_call_record_requested  = false;
// If the audio gateway does not support HFP profile (HSP only),
// this variable is not updated and must be set so that no acoustic is performed in Audience chip
static bool      is_acoustic_in_bt_device = true;
static bool      was_acoustic_in_bt_device = false;
static vpc_hac_set_t current_hac_setting  = VPC_HAC_OFF;
static vpc_hac_set_t previous_hac_setting = VPC_HAC_OFF;
static int       modem_gain_dl            = 0;
#ifdef HAL_VPC_PLUS_6DB_MODEM_UL
/* +6dB on Modem UL path was applied on MFLD and we must keep this setting
 * because all tuning has been done with it */
static int       modem_gain_ul            = MODEM_GAIN_6dB;
#else
static int       modem_gain_ul            = MODEM_GAIN_0dB;
#endif
static vpc_band_t current_csv_band        = VPC_BAND_NARROW;
static vpc_band_t current_voip_band       = VPC_BAND_NARROW;
static int       modem_status             = MODEM_DOWN;
static bool      call_connected           = false;

static bool      vpc_audio_routed      = false;
// ref counter used only to handle BT route in "NORMAL" mode
static int       vpc_bt_enabled_ref_count = 0;

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
static bool      audience_awake        = false;
#endif
/* Forward declaration */
static void handle_voice_call_record();

static const char * BAND_NAME[] = {"NB", "WB"};

/*---------------------------------------------------------------------------*/
/* Initialization                                                            */
/*---------------------------------------------------------------------------*/
static int vpc_init(uint32_t ifx_i2s1_clk_select, uint32_t ifx_i2s2_clk_select, bool have_modem)
{
    vpc_lock.lock();
    LOGD("Initialize VPC\n");

    if(have_modem){
        if (ifx_i2s1_clk_select == (uint32_t) -1) {
            // Not provided: use default
            ifx_i2s1_clk_select = DEFAULT_IS21_CLOCK_SELECTION;
        }
        if (ifx_i2s2_clk_select == (uint32_t) -1) {
            // Not provided: use default
            ifx_i2s2_clk_select = DEFAULT_IS22_CLOCK_SELECTION;
        }
        if (at_thread_init == false)
        {
            AT_STATUS cmd_status = at_start(AUDIO_AT_CHANNEL_NAME, ifx_i2s1_clk_select, ifx_i2s2_clk_select);
            if (cmd_status != AT_OK) goto return_error;
            LOGD("AT thread started\n");
            at_thread_init = true;
        }
    }else{
        LOGD("VPC with no Modem\n");
    }

    msic::pcm_init();

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    LOGD("Initialize Audience\n");
    int rc;
    rc = acoustic::process_init();
    if (rc) goto return_error;
#endif

    LOGD("VPC Init OK\n");
    vpc_lock.unlock();
    return NO_ERROR;

return_error:

    LOGE("VPC Init failed\n");
    vpc_lock.unlock();
    return NO_INIT;
}

/*---------------------------------------------------------------------------*/
/* State machine parameters                                                  */
/*---------------------------------------------------------------------------*/
static int vpc_params(int mode, uint32_t device)
{
    vpc_lock.lock();

    current_mode = mode;
    current_device = device;

    LOGD("vpc_params mode = %d device = 0x%X\n", current_mode, current_device);
    LOGD("vpc_params previous mode = %d previous device = 0x%X\n", prev_mode, prev_device);

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* State machine parameters                                                  */
/*---------------------------------------------------------------------------*/
static void vpc_set_mode(int mode)
{
    vpc_lock.lock();

    current_mode = mode;

    LOGD("%s: mode = %d\n", __FUNCTION__, current_mode);
    LOGD("%s: previous mode = %d\n", __FUNCTION__, prev_mode);

    vpc_lock.unlock();
}

/*---------------------------------------------------------------------------*/
/* Set input source                                                  */
/*---------------------------------------------------------------------------*/
static void vpc_set_input_source(int input_source)
{
    vpc_lock.lock();

    current_input_source = input_source;

    vpc_lock.unlock();

    LOGD("%s: input_source = %d\n", __FUNCTION__, input_source);

}

/*---------------------------------------------------------------------------*/
/* Call status parameters                                                    */
/* Corresponds to the XPROGRESS information                                  */
/*---------------------------------------------------------------------------*/
static void vpc_set_call_status(bool isConnected)
{
    vpc_lock.lock();

    call_connected = isConnected;

    LOGD("%s: call_status = %d\n", __FUNCTION__, call_connected);

    vpc_lock.unlock();
}

/*---------------------------------------------------------------------------*/
/* Modem Status: each time modem status is changed, this fonction will be    */
/* called from HAL                                                           */
/*---------------------------------------------------------------------------*/
static void vpc_set_modem_state(int status)
{
    LOGD("vpc_set_modem_state modem_status");
    vpc_lock.lock();

    if(status != modem_status) {
        modem_status = status;
    }

    LOGD("vpc_set_modem_state modem_status = %d \n", modem_status);

    vpc_lock.unlock();
}

/*-------------------------------*/
/* Get audio voice routing state */
/*-------------------------------*/

static bool vpc_get_audio_routed()
{
    return vpc_audio_routed;
}

/*-----------------------------------------------------------------------------------*/
/* Set audio voice routing state                                                     */
/* Enable/disable volume keys wake up capability given the audio voice routing state */
/* Set internal state variables                                                      */
/*-----------------------------------------------------------------------------------*/

static void vpc_set_audio_routed(bool isRouted)
{
    // Update internal state variables
    prev_mode = current_mode;
    prev_device = current_device;
    previous_tty_call = current_tty_call;
    previous_hac_setting = current_hac_setting;
    was_acoustic_in_bt_device = is_acoustic_in_bt_device;


    if (vpc_audio_routed != isRouted) {
        // Volume buttons & power management
        if (isRouted) {
            volume_keys::wakeup_enable();
        } else {
            volume_keys::wakeup_disable();
        }
        vpc_audio_routed = isRouted;
    }
}

/*---------------------------------------------------------------------------*/
/* Route/unroute functions                                                   */
/*---------------------------------------------------------------------------*/
static int vpc_unroute_voip()
{
    LOGD("%s", __FUNCTION__);

    if (!vpc_get_audio_routed())
        return NO_ERROR;

    msic::pcm_disable();
    // need to put the I2S from BT chip in high Z when not used
    bt::pcm_disable();

    // Update internal state variables
    vpc_set_audio_routed(false);

    return NO_ERROR;
}

static int vpc_unroute_csvcall()
{
    LOGD("%s", __FUNCTION__);

    if (!vpc_get_audio_routed())
        return NO_ERROR;

    int ret = volume_keys::wakeup_disable();

    if(modem_status == MODEM_UP)
        amc_mute();
    msic::pcm_disable();
    // need to put the I2S from BT chip in high Z when not used
    bt::pcm_disable();

    if(modem_status == MODEM_UP)
        amc_off();
    mixing_enable = false;

    // Update internal state variables
    vpc_set_audio_routed(false);

    return ret;
}

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
static int vpc_wakeup_audience()
{
    if (audience_awake)
        return NO_ERROR;

    if (acoustic::process_wake())
        return NO_INIT;

    audience_awake = true;

    // Disable smooth mute
    usleep(ES305B_WAKEUP_DELAY);
    acoustic::set_smooth_mute(false);

    return NO_ERROR;
}
#endif

static void vpc_suspend()
{
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    if (!audience_awake)
        return ;

    acoustic::process_suspend();
    audience_awake = false;
#endif

}

static inline bool vpc_route_conditions_changed()
{
    return (prev_mode != current_mode ||
            prev_device != current_device ||
            !vpc_get_audio_routed() ||
            current_tty_call != previous_tty_call ||
            current_hac_setting != previous_hac_setting ||
            is_acoustic_in_bt_device != was_acoustic_in_bt_device);

}

static void* profile_thread_func(void* pData)
{
#ifdef ENABLE_TTY_PROFILE
    pthread_setname_np(pthread_self(), "AudienceProfile");
    // Sendind the profile to audience
    return (void*)acoustic::process_profile(current_device, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
#else
    uint32_t device_profile;
    pthread_setname_np(pthread_self(), "AudienceProfile");
    // Computing the profile
    // In case of TTY call, use pass-through profile
    if ((current_device & AudioSystem::DEVICE_OUT_WIRED_HEADSET) && current_tty_call != AMC_TTY_OFF)
        device_profile = device_out_defaut;
    else
        device_profile = current_device;
    // Sending the profile to audience
    return (void*)acoustic::process_profile(device_profile, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
#endif
}

static int process_profile(long* session)
{
    // Create the thread for Audience profile setting
    return pthread_create(session, NULL, profile_thread_func, NULL);
}

static int wait_end_of_session(long session)
{
    int ret;
    // Joining with Audience profile thread
    pthread_join(session, (void**)&ret);
    return ret;
}

/*---------------------------------------------------------------------------*/
/* Platform voice paths control                                              */
/*---------------------------------------------------------------------------*/
static int vpc_route(vpc_route_t route)
{
    vpc_lock.lock();

    int ret;
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    long session = 0;
#endif
    uint32_t device_profile;

    /* -------------------------------------------------------------- */
    /* Enter in this loop only for output device as they trig the     */
    /* establishment of the route                                     */
    /* -------------------------------------------------------------- */
    if (current_device & AudioSystem::DEVICE_OUT_ALL)
    {
        LOGD("mode = %d device = 0x%X modem status = %d\n", current_mode, current_device, modem_status);
        LOGD("previous mode = %d previous device = 0x%X\n", prev_mode, prev_device);
        LOGD("current tty = %d previous tty = %d\n", current_tty_call, previous_tty_call);

        if (route == VPC_ROUTE_OPEN)
        {
            LOGD("VPC_ROUTE_OPEN request\n");

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
            if (vpc_wakeup_audience())
                goto return_error;
#endif

            /* --------------------------------------------- */
            /* Voice paths control for MODE_IN_CALL          */
            /* --------------------------------------------- */
            if (current_mode == AudioSystem::MODE_IN_CALL)
            {
                LOGD("VPC IN_CALL\n");
                if(modem_status != MODEM_UP)
                {
                    LOGD("MODEM_DOWN or IN_RESET, cannot set a voicecall path!!!\n");
                    goto return_error;
                }
                /* MODEM is UP, apply the changes only if devices, or mode, or audio is not route due to modem reset or call disconnected */
                if (vpc_route_conditions_changed())
                {
                    IFX_TRANSDUCER_MODE_SOURCE mode_source;
                    IFX_TRANSDUCER_MODE_DEST mode_dest;
                    switch (current_device)
                    {
                        /* ------------------------------------ */
                        /* Voice paths control for MSIC devices */
                        /* ------------------------------------ */
                        case AudioSystem::DEVICE_OUT_EARPIECE:
                        case AudioSystem::DEVICE_OUT_SPEAKER:
                        case AudioSystem::DEVICE_OUT_WIRED_HEADSET:
                        case AudioSystem::DEVICE_OUT_WIRED_HEADPHONE:

                            /* If VPC was not routed previously, avoid uncessary steps!!! */
                            if (vpc_get_audio_routed()) {
                                amc_mute();
                                msic::pcm_disable();
                                /* Disable SCO path if a MSIC device is in use  */
                                bt::pcm_disable();
                                amc_off();
                            }
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
                            // Apply audience profile in separated thread
                            process_profile(&session);

                            mode_source = IFX_USER_DEFINED_15_S;
                            mode_dest = IFX_USER_DEFINED_15_D;
#else
                            // No Audience, Acoustics in modem
                            translate_to_amc_device(current_device, &mode_source, &mode_dest);
#endif
#ifdef ENABLE_TTY_PROFILE
                            // Configure modem I2S1
                            amc_conf_i2s1(translate_vpc_to_amc_tty[current_tty_call], mode_source, mode_dest);
#else
                            // Configure modem I2S1
                            amc_conf_i2s1(current_tty_call, mode_source, mode_dest);
#endif

                            if ((prev_mode != AudioSystem::MODE_IN_CALL) || (prev_device & DEVICE_OUT_BLUETOOTH_SCO_ALL) || (!vpc_get_audio_routed()))
                            {
                                 // Configure modem I2S2 and do the modem routing
                                 amc_conf_i2s2_route();
                            }

                            amc_on();

#ifdef ENABLE_TTY_PROFILE
                            msic::pcm_enable(current_mode, current_device, current_hac_setting, current_tty_call);
#else
                            msic::pcm_enable(current_mode, current_device, current_hac_setting);
#endif
                            mixing_enable = true;
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
                            // Join with Audience thread
                            wait_end_of_session(session);
#endif
#ifdef ENABLE_TTY_PROFILE
#ifdef HAL_VPC_PLUS_6DB_MODEM_UL
                            // TTY does not support the +6dB set on modem side. They must
                            // be removed when TTY is used in FULL or HCO modes
                            if (current_device == AudioSystem::DEVICE_OUT_WIRED_HEADSET &&
                                (current_tty_call == VPC_TTY_FULL || current_tty_call == VPC_TTY_HCO))
                                amc_unmute(modem_gain_dl, MODEM_GAIN_0dB);
                            else
#endif
                                amc_unmute(modem_gain_dl, modem_gain_ul);
#else //ENABLE_TTY_PROFILE
                            amc_unmute(modem_gain_dl, modem_gain_ul);
#endif
                            break;

                        /* ------------------------------------ */
                        /* Voice paths control for BT devices   */
                        /* ------------------------------------ */
                        case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO:
                        case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                        case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT:

                            /* If VPC was not routed previously, avoid uncessary steps!!! */
                            if (vpc_get_audio_routed()) {
                                amc_mute();
                                msic::pcm_disable();
                                amc_off();
                            }

                            // If is_acoustic_in_bt_device is true, bypass phone embedded algorithms
                            // and use acoustic alogrithms from Bluetooth headset.
                            device_profile = (is_acoustic_in_bt_device == true) ? device_out_defaut : current_device;

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
                            ret = acoustic::process_profile(device_profile, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
                            if (ret) goto return_error;

                            mode_source = IFX_USER_DEFINED_15_S;
                            mode_dest = IFX_USER_DEFINED_15_D;
#else
                            // No Audience, Acoustics in modem
                            translate_to_amc_device(device_profile, &mode_source, &mode_dest);
#endif
                            // Do the modem config for BT devices
                            amc_modem_conf_bt_dev(mode_source, mode_dest);

                            amc_on();
                            bt::pcm_enable();
                            mixing_enable = true;

                            amc_unmute(modem_gain_dl, modem_gain_ul);
                            break;
                        default:
                            break;
                    }
                    // Restore record path if required
                    handle_voice_call_record();

                    // Update internal state variables
                    vpc_set_audio_routed(true);
                }
                /* Else: nothing to do, input params of VPC did not change */
            }
            /* --------------------------------------------- */
            /* Voice paths control for MODE_IN_COMMUNICATION */
            /* --------------------------------------------- */
            else if (current_mode == AudioSystem::MODE_IN_COMMUNICATION)
            {
                LOGD("VPC IN_COMMUNICATION\n");
                if(modem_status == MODEM_COLD_RESET)
                {
                    LOGD("MODEM_COLD_RESET, cannot set VoIP path !!!\n");
                    goto return_error;
                }

                /* MODEM is not in cold reset, apply the changes only if devices, or mode, or modem status was changed */
                if (vpc_route_conditions_changed())
                {
                    switch (current_device)
                    {
                    /* ------------------------------------ */
                    /* Voice paths control for MSIC devices */
                    /* ------------------------------------ */
                    case AudioSystem::DEVICE_OUT_EARPIECE:
                    case AudioSystem::DEVICE_OUT_SPEAKER:
                    case AudioSystem::DEVICE_OUT_WIRED_HEADSET:
                    case AudioSystem::DEVICE_OUT_WIRED_HEADPHONE:

                        /* If VPC was not routed previously, avoid uncessary steps!!! */
                        if (vpc_get_audio_routed()) {

                            msic::pcm_disable();
                            /* Disable SCO path if a MSIC device is in use  */
                            bt::pcm_disable();
                        }

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
#ifdef ENABLE_TTY_PROFILE
                        ret = acoustic::process_profile(current_device, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
#else //ENABLE_TTY_PROFILE
                        if ((current_device & AudioSystem::DEVICE_OUT_WIRED_HEADSET) && current_tty_call != AMC_TTY_OFF)
                            device_profile = device_out_defaut;
                        else
                            device_profile = current_device;
                        ret = acoustic::process_profile(device_profile, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
#endif
                        if (ret) goto return_error;
#endif

#ifdef ENABLE_TTY_PROFILE
                        msic::pcm_enable(current_mode, current_device, current_hac_setting, current_tty_call);
#else
                        msic::pcm_enable(current_mode, current_device, current_hac_setting);
#endif
                        break;
                    /* ------------------------------------ */
                    /* Voice paths control for BT devices   */
                    /* ------------------------------------ */
                    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO:
                    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
                        if (vpc_get_audio_routed()) {
                            msic::pcm_disable();
                        }
#ifdef CUSTOM_BOARD_WITH_AUDIENCE
                        // If is_acoustic_in_bt_device is true, bypass phone embedded algorithms
                        // and use acoustic alogrithms from Bluetooth headset.
                        device_profile = (is_acoustic_in_bt_device == true) ? device_out_defaut : current_device;
                        ret = acoustic::process_profile(device_profile, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
                        if (ret) goto return_error;
#endif
                        bt::pcm_enable();
                        break;
                    default:
                        break;
                    }

                    // Update internal state variables
                    vpc_set_audio_routed(true);
                }
                /* else: nothing to do, VPC input params did not change */
            }
            /* else: nothing to do, mode not handled */
        }
        /* Disable voice paths at the end of the call */
        else if (route == VPC_ROUTE_CLOSE)
        {
            LOGD("VPC_ROUTE_CLOSE request\n");
            if(current_mode == AudioSystem::MODE_IN_COMMUNICATION)
            {
                LOGD("current_mode: IN_COMMUNICATION\n");
                /*
                * We are still in VoIP call but a modem cold reset
                * is going to be performed.
                * Need to close immediately MSIC / BT
                */
                if(modem_status == MODEM_COLD_RESET)
                {
                    LOGD("VPC IN_COMMUNICATION & MODEM COLD RESET\n");
                    vpc_unroute_voip();

                    vpc_suspend();
                }
                else if(prev_mode == AudioSystem::MODE_IN_CALL && call_connected)
                {

                    LOGD("VPC SWAP FROM IN_CALL TO IN_COMMUNICATION");
                    if (vpc_unroute_csvcall())
                        goto return_error;

                    /* Keep audience awaken */
                }
                /* Else: ignore this close request */
            }
            else if(current_mode == AudioSystem::MODE_IN_CALL)
            {
                LOGD("current_mode: IN_CALL");
                if(prev_mode == AudioSystem::MODE_IN_COMMUNICATION)
                {
                    LOGD("VPC SWAP from IN_COMMUNICATION to IN_CALL");
                    vpc_unroute_voip();

                    /* Keep audience awaken */
                }
                /* We are still in call but an accessory change occured
                 * and a close request was initiated
                 * Do not do anything except if the modem is not up anymore
                 */
                else if(modem_status != MODEM_UP || !call_connected)
                {
                    LOGD("VPC from IN_CALL to IN_CALL with MODEM_DOWN or CALL NOT CONNECTED");
                    if (vpc_unroute_csvcall())
                        goto return_error;

                    vpc_suspend();
                }
                /* Else: ignore this close request */

            } else
            {
                LOGD("current mode: out of CSV/VoIP call");
                if (prev_mode == AudioSystem::MODE_IN_CALL)
                {
                    LOGD("VPC from IN_CALL to NORMAL\n");
                    if (vpc_unroute_csvcall())
                        goto return_error;
                }
                else
                {
                    LOGD("VPC from IN_COMMUNICATION to NORMAL\n");
                    vpc_unroute_voip();
                }

                vpc_suspend();
            }
        }
        else
        {
            LOGW("%s: Unknown route request %d: bail out\n", __FUNCTION__,route);
        }
    }
    else
    {
        LOGW("%s: VPC called for input device 0x%X in mode %d: should not occur\n", __FUNCTION__, current_device, current_mode);
    }

    vpc_lock.unlock();
    return NO_ERROR;

return_error:

    vpc_lock.unlock();
    return NO_INIT;
}

/*---------------------------------------------------------------------------*/
/* Volume managment                                                          */
/*---------------------------------------------------------------------------*/
static int vpc_volume(float volume)
{
    vpc_lock.lock();

    int gain = 0;
    int range = 48;

    gain = volume * range + 40;
    gain = (gain >= 88) ? 88 : gain;
    gain = (gain <= 40) ? 40 : gain;

    if ((at_thread_init == true) && (current_mode == AudioSystem::MODE_IN_CALL))
    {
        amc_setGaindest(AMC_I2S1_TX, gain);
        LOGD("%s: change modem volume", __FUNCTION__);
    }

    // Backup modem gain
    modem_gain_dl = gain;

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Voice Call Record Enable/disable                                          */
/*---------------------------------------------------------------------------*/
static void handle_voice_call_record()
{
    AMC_VOICE_RECORD_SOURCE input_source = AMC_VOICE_INVALID_SOURCE;

    switch(current_input_source) {
    case AUDIO_SOURCE_VOICE_CALL:
        input_source = AMC_VOICE_CALL_SOURCE;
        break;

    case AUDIO_SOURCE_VOICE_UPLINK:
        input_source = AMC_VOICE_UPLINK_SOURCE;
        break;

    case AUDIO_SOURCE_VOICE_DOWNLINK:
        input_source = AMC_VOICE_DOWNLINK_SOURCE;
        break;

    default:
        LOGE("%s: Unknown Input Source.", __FUNCTION__);
        input_source = AMC_VOICE_INVALID_SOURCE;
    }

    LOGD("%s: %s recording for input source %d", __FUNCTION__, (voice_call_record_requested) ? "enable" : "disable", input_source);

    amc_voice_record_source_enable(input_source, voice_call_record_requested);
}

/*---------------------------------------------------------------------------*/
/* Mixing Enable/disable                                                     */
/*---------------------------------------------------------------------------*/
static void mixing_on()
{
    if (!mixing_enable)
    {
        // Enable alert mixing
        LOGD("%s: enable mixing", __FUNCTION__);
        amc_enable(AMC_I2S2_RX);
        mixing_enable = true;
    }
}

static void mixing_off()
{
    if (mixing_enable)
    {
        LOGD("%s: disable mixing", __FUNCTION__);
        amc_disable(AMC_I2S2_RX);
        mixing_enable = false;
    }
}


/*---------------------------------------------------------------------------*/
/* I2S2 disable                                                              */
/*---------------------------------------------------------------------------*/
static int vpc_mixing_disable(bool isOut)
{
    vpc_lock.lock();

    // Request from an outStream? -> mix disable request
    if (isOut)
    {
        mixing_off();
    }
    // Request from an instream? -> record disable request
    else
    {
        if (voice_call_record_requested) {
            voice_call_record_requested = false;
            handle_voice_call_record();
        }
    }

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* I2S2 enable                                                               */
/*---------------------------------------------------------------------------*/
static int vpc_mixing_enable(bool isOut, uint32_t device)
{
    vpc_lock.lock();

    // Request from an outstream? -> mix request
    if (isOut)
    {
        mixing_on();
    }
    // Request from an instream? -> record request
    else
    {
        if (device == AudioSystem::DEVICE_IN_VOICE_CALL)
        {
            if (!voice_call_record_requested) {
                voice_call_record_requested = true;
                handle_voice_call_record();
            }
        }
    }

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Enable TTY                                                                */
/*---------------------------------------------------------------------------*/
static int vpc_set_tty(vpc_tty_t tty)
{
    vpc_lock.lock();

#ifdef ENABLE_TTY_PROFILE
    if (tty >= VPC_TTY_INVALID) {
        LOGE("%s: Invalid tty mode set (%d): force TTY OFF\n", __FUNCTION__, tty);
        tty = VPC_TTY_OFF;
    }
    current_tty_call = tty;

    LOGD("TTY set for audio route (%d)", current_tty_call);

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    acoustic::set_tty(current_tty_call);
    // Audience profile switch is not needed since in-call TTY switch triggers a
    // rerouting sequence in which the correct Audience profile will be applied.
#endif

    vpc_lock.unlock();
#else //ENABLE_TTY_PROFILE
    current_tty_call = translate_vpc_to_amc_tty[tty];
    vpc_lock.unlock();
#endif

    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Translate Device                                                          */
/*---------------------------------------------------------------------------*/
static void translate_to_amc_device(const uint32_t current_device, IFX_TRANSDUCER_MODE_SOURCE* mode_source, IFX_TRANSDUCER_MODE_DEST* mode_dest)
{
    switch (current_device)
    {
    /* ------------------------------------ */
    /* Voice paths control for MSIC devices */
    /* ------------------------------------ */
    case AudioSystem::DEVICE_OUT_EARPIECE:
        *mode_source = IFX_HANDSET_S;
        *mode_dest = IFX_HANDSET_D;
        break;
    case AudioSystem::DEVICE_OUT_SPEAKER:
        *mode_source = IFX_HF_S;
        *mode_dest = IFX_BACKSPEAKER_D;
        break;
    case AudioSystem::DEVICE_OUT_WIRED_HEADSET:
        *mode_source = IFX_HEADSET_S;
        *mode_dest = IFX_HEADSET_D;
        break;
    case AudioSystem::DEVICE_OUT_WIRED_HEADPHONE:
        *mode_source = IFX_HF_S;
        *mode_dest = IFX_HEADSET_D;
        break;
    /* ------------------------------------ */
    /* Voice paths control for BT devices   */
    /* ------------------------------------ */
    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO:
    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
    case AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
        *mode_source = IFX_BLUETOOTH_S;
        *mode_dest = IFX_BLUETOOTH_D;
        break;
    case AudioSystem::DEVICE_OUT_DEFAULT:
        // Used for BT with acoustics devices
        *mode_source = IFX_USER_DEFINED_15_S;
        *mode_dest = IFX_USER_DEFINED_15_D;
        break;
    default:
        *mode_source  = IFX_DEFAULT_S;
        *mode_dest = IFX_DEFAULT_D;
        break;
    }
}

/*---------------------------------------------------------------------------*/
/* Enable/Disable BT SCO path on route manager request                       */
/*---------------------------------------------------------------------------*/

static int vpc_set_bt_sco_path(vpc_route_t bt_route)
{
    vpc_lock.lock();

    if (bt_route == VPC_ROUTE_OPEN)
    {
        if(!vpc_bt_enabled_ref_count++)
        {
            bt::pcm_enable();
        }
        LOGD("%s: increment ref count %d\n", __FUNCTION__, vpc_bt_enabled_ref_count);
    }
    else //VPC_ROUTE_CLOSE
    {
        if(!--vpc_bt_enabled_ref_count)
        {
            bt::pcm_disable();
        }
        LOGD("%s: decrement ref count %d\n", __FUNCTION__, vpc_bt_enabled_ref_count);

    }

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Enable/Disable BT acoustic with AT+NREC command in handset                */
/*---------------------------------------------------------------------------*/

static int vpc_bt_nrec(vpc_bt_nrec_t bt_nrec)
{
    vpc_lock.lock();

    if (bt_nrec == VPC_BT_NREC_ON) {

        LOGD("Enable in handset echo cancellation/noise reduction for BT\n");
        is_acoustic_in_bt_device = false;
    }
    else {
        LOGD("Disable in handset echo cancellation/noise reduction for BT. Use BT device embedded acoustics\n");
        is_acoustic_in_bt_device = true;
    }

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Enable/Disable HAC                                                        */
/*---------------------------------------------------------------------------*/

static int vpc_set_hac(vpc_hac_set_t set_hac)
{
    vpc_lock.lock();

    current_hac_setting = set_hac;

    LOGD("HAC set for audio route (%d)", current_hac_setting);

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    acoustic::set_hac(current_hac_setting);
    // Audience profile switch is not needed since in-call HAC switch triggers a
    // rerouting sequence in which the correct Audience profile will be applied.
#endif

    vpc_lock.unlock();
    return NO_ERROR;
}

/*---------------------------------------------------------------------------*/
/* Set VOIP or CSV band in use                                               */
/*---------------------------------------------------------------------------*/

static void vpc_set_band(vpc_band_t band, int for_mode)
{
    vpc_lock.lock();

    // Is it really a band change request ?
    if (band == CURRENT_BAND_FOR_MODE(for_mode))
        goto unlock;
    // Is it a correct band ?
    if (band >= VPC_BAND_INVALID) {
        LOGE("Invalid band: %d\n", band);
        goto unlock;
    }

    LOGD("New band: %s for mode %d\n", BAND_NAME[band], for_mode);

    CURRENT_BAND_FOR_MODE(for_mode) = band;

#ifdef CUSTOM_BOARD_WITH_AUDIENCE
    // Request the band-specific Audience profile only if the path is already established
    if (vpc_get_audio_routed() && (current_mode == for_mode)) {
        // Do not request a preset if we are in bypass because of:
        // - BT device with embedded acoustics
#ifdef ENABLE_TTY_PROFILE
        if (!((current_device & DEVICE_OUT_BLUETOOTH_SCO_ALL) && is_acoustic_in_bt_device == true)) {
            // Request new band-specific preset
#else //ENABLE_TTY_PROFILE
        // - TTY device in-use
        if (!((current_device & DEVICE_OUT_BLUETOOTH_SCO_ALL) && is_acoustic_in_bt_device == true) &&
            !((current_device & AudioSystem::DEVICE_OUT_WIRED_HEADSET) && current_tty_call != AMC_TTY_OFF) ) {
#endif
            acoustic::process_profile(current_device, current_mode, CURRENT_BAND_FOR_MODE(current_mode));
        }
    }
#endif

unlock:
    vpc_lock.unlock();
}

/*===========================================================================*/
/* HW module interface definition                                            */
/*===========================================================================*/

static int s_device_open(const hw_module_t*, const char*, hw_device_t**);
static int s_device_close(hw_device_t*);

static hw_module_methods_t s_module_methods =
{
    open : s_device_open
};

extern "C" hw_module_t HAL_MODULE_INFO_SYM;

hw_module_t HAL_MODULE_INFO_SYM =
{
    tag           : HARDWARE_MODULE_TAG,
    version_major : 1,
    version_minor : 0,
    id            : VPC_HARDWARE_MODULE_ID,
    name          : "mfld vpc module",
    author        : "Intel Corporation",
    methods       : &s_module_methods,
    dso           : 0,
    reserved      : { 0, },
};

static int s_device_open(const hw_module_t* module, const char* name,
                         hw_device_t** device)
{
    vpc_device_t *dev;
    dev = (vpc_device_t *) malloc(sizeof(*dev));
    if (!dev) return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    dev->common.tag     = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module  = (hw_module_t *) module;
    dev->common.close   = s_device_close;
    dev->init           = vpc_init;
    dev->params         = vpc_params;
    dev->set_mode       = vpc_set_mode;
    dev->set_input_source = vpc_set_input_source;
    dev->set_call_status = vpc_set_call_status;
    dev->set_modem_state = vpc_set_modem_state;
    dev->route          = vpc_route;
    dev->volume         = vpc_volume;
    dev->mix_disable    = vpc_mixing_disable;
    dev->mix_enable     = vpc_mixing_enable;
    dev->set_tty        = vpc_set_tty;
    dev->bt_nrec        = vpc_bt_nrec;
    dev->set_hac        = vpc_set_hac;
    dev->set_bt_sco_path= vpc_set_bt_sco_path;
    dev->set_band       = vpc_set_band;
    *device = &dev->common;
    return 0;
}

static int s_device_close(hw_device_t* device)
{
    free(device);
    return 0;
}

}; // namespace android_audio_legacy
