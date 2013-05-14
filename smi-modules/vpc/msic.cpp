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

#define LOG_TAG "VPC_MSIC"
#include <utils/Log.h>

#include <hardware_legacy/AudioSystemLegacy.h>

#include "msic.h"

namespace android_audio_legacy
{

#define MEDFIELDAUDIO "medfieldaudio"

snd_pcm_t *msic::handle_playback = NULL;
snd_pcm_t *msic::handle_capture  = NULL;

#ifdef ENABLE_TTY_PROFILE
const char *msic::deviceNamePlayback(int mode, uint32_t device, vpc_hac_set_t hac_setting, vpc_tty_t tty_setting)
#else
const char *msic::deviceNamePlayback(int mode, uint32_t device, vpc_hac_set_t hac_setting)
#endif
{
    const char *devName;

    if (mode == AudioSystem::MODE_IN_CALL)
    {
        switch (device)
        {
        case AudioSystem::DEVICE_OUT_EARPIECE :
            if (hac_setting == VPC_HAC_ON) {
                devName = "VoicePlayback_HAC_incall";
                LOGV("HAC has been activated, change device to %s\n", devName);
            }
            else {
                devName = "VoicePlayback_Earpiece_incall";
            }
            break;
        case AudioSystem::DEVICE_OUT_SPEAKER :
            devName = "VoicePlayback_Speaker_incall";
            break;
        case AudioSystem::DEVICE_OUT_WIRED_HEADSET :
#ifdef ENABLE_TTY_PROFILE
            switch (tty_setting) {
                default:
                // Intended fall through
                case VPC_TTY_FULL:
                // Intended fall through: TTY FULL needs same codec configuration than headset
                case VPC_TTY_OFF:
                    devName = "VoicePlayback_Headset_incall";
                    break;
                case VPC_TTY_VCO:
                    devName = "VoicePlayback_Earpiece_tty_vco_incall";
                    break;
                case VPC_TTY_HCO:
                    if (hac_setting == VPC_HAC_ON)
                        devName = "VoicePlayback_Earpiece_tty_hco_hac_incall";
                    else
                        devName = "VoicePlayback_Earpiece_tty_hco_incall";
                    break;
            }
#else
            devName = "VoicePlayback_Headset_incall";
#endif
            break;
        case AudioSystem::DEVICE_OUT_WIRED_HEADPHONE :
            devName = "VoicePlayback_Headphone_incall";
            break;
        default :
            LOGE("  Device not handled by MSIC Lib: %x\n", device);
            devName = "";
            break;
        };
    }
    else if (mode == AudioSystem::MODE_IN_COMMUNICATION)
    {
        switch (device)
        {
        case AudioSystem::DEVICE_OUT_EARPIECE :
            if (hac_setting == VPC_HAC_ON) {
                devName = "VoicePlayback_HAC_incommunication";
                LOGV("HAC has been activated, change device to %s\n", devName);
            }
            else {
                devName = "VoicePlayback_Earpiece_incommunication";
            }
            break;
        case AudioSystem::DEVICE_OUT_SPEAKER :
            devName = "VoicePlayback_Speaker_incommunication";
            break;
        case AudioSystem::DEVICE_OUT_WIRED_HEADSET :
            devName = "VoicePlayback_Headset_incommunication";
            break;
        case AudioSystem::DEVICE_OUT_WIRED_HEADPHONE :
            devName = "VoicePlayback_Headphone_incommunication";
            break;
        default :
            LOGE("  Device not handled by MSIC Lib: %x\n", device);
            devName = "";
            break;
        };
    }
    else
    {
        LOGE("  Mode not handled by MSIC Lib: %x\n", device);
        devName = "";
    }

    return devName;
}

const char *msic::deviceNameCapture(int mode, uint32_t device)
{
    const char *devName;

    if (mode == AudioSystem::MODE_IN_CALL)
    {
        // No distinction as alsa mixer control are set during playback path opening
        devName = "VoiceCapture_incall";
    }
    else if (mode == AudioSystem::MODE_IN_COMMUNICATION)
    {
        // No distinction as alsa mixer control are set during playback path opening
        devName = "VoiceCapture_incommunication";
    }
    else
    {
        LOGE("  Mode not handled by MSIC Lib: %x\n", device);
        devName = "";
    }

    return devName;
}

int msic::pcm_init()
{
#ifdef ENABLE_TTY_PROFILE
    pcm_enable(AudioSystem::MODE_IN_CALL, AudioSystem::DEVICE_OUT_SPEAKER, VPC_HAC_OFF, VPC_TTY_OFF);
#else
    pcm_enable(AudioSystem::MODE_IN_CALL, AudioSystem::DEVICE_OUT_SPEAKER, VPC_HAC_OFF);
#endif
    pcm_disable();

    return 0;
}

#ifdef ENABLE_TTY_PROFILE
int msic::pcm_enable(int mode, uint32_t device, vpc_hac_set_t hac_setting, vpc_tty_t tty_setting)
#else
int msic::pcm_enable(int mode, uint32_t device, vpc_hac_set_t hac_setting)
#endif
{
    const char *device_playback;
    const char *device_capture;
    int err;

    LOGD("Enable MSIC voice path ~~~ Entry\n");

    if (handle_playback || handle_capture)
        pcm_disable();

#ifdef ENABLE_TTY_PROFILE
    device_playback = deviceNamePlayback(mode, device, hac_setting, tty_setting);
#else
    device_playback = deviceNamePlayback(mode, device, hac_setting);
#endif
    device_capture = deviceNameCapture(mode, device);
    LOGD("  %s \n", device_playback);

    if ((err = snd_pcm_open(&handle_playback, device_playback, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        LOGE("  Playback open error: %s\n", snd_strerror(err));
        pcm_disable();
    }
    else if ((err = snd_pcm_open(&handle_capture, device_capture, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        LOGE("  Capture open error: %s\n", snd_strerror(err));
        pcm_disable();
    }
    else if ((err = snd_pcm_set_params(handle_playback,
                                      SND_PCM_FORMAT_S16_LE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      2,
                                      48000,
                                      0,
                                      500000)) < 0) {    /* 0.5sec */
        LOGE("  [P]set params error: %s\n", snd_strerror(err));
        pcm_disable();
    }
    else if ((err = snd_pcm_set_params(handle_capture,
                                      SND_PCM_FORMAT_S16_LE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      1,
                                      48000,
                                      1,
                                      500000)) < 0) { /* 0.5sec */
        LOGE("  [C]set params error: %s\n", snd_strerror(err));
        pcm_disable();
    }

    usleep(40000); // Time to have MSIC in a "stable" state...
    LOGD("Enable MSIC voice path ~~~ Exit\n");

    return err;
}

int msic::pcm_disable()
{
    LOGD("Disable MSIC voice path ~~~ Entry\n");

    if (handle_playback) {
        snd_pcm_close(handle_playback);
        LOGD("  snd_pcm_close(handle_playback)\n");
    }
    if (handle_capture) {
        snd_pcm_close(handle_capture);
        LOGD("  snd_pcm_close(handle_capture)\n");
    }
    handle_playback = NULL;
    handle_capture  = NULL;

    LOGD("Disable MSIC voice path ~~~ Exit\n");

    return 0;
}

}

