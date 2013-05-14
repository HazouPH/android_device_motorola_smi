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

#ifndef __VPC_MSIC_H__
#define __VPC_MSIC_H__

#include <sys/types.h>

#include <alsa/asoundlib.h>
#include <alsa/control_external.h>
#include "vpc_hardware.h"

namespace android_audio_legacy
{

class msic
{
public :
    static int pcm_init();
#ifdef ENABLE_TTY_PROFILE
    static int pcm_enable(int mode, uint32_t device, vpc_hac_set_t hac_setting, vpc_tty_t tty_setting);
#else
    static int pcm_enable(int mode, uint32_t device, vpc_hac_set_t hac_setting);
#endif
    static int pcm_disable();

private :
#ifdef ENABLE_TTY_PROFILE
    static const char *deviceNamePlayback(int mode, uint32_t device, vpc_hac_set_t hac_setting, vpc_tty_t tty_setting);
#else
    static const char *deviceNamePlayback(int mode, uint32_t device, vpc_hac_set_t hac_setting);
#endif
    static const char *deviceNameCapture(int mode, uint32_t device);

    static snd_pcm_t *handle_playback;
    static snd_pcm_t *handle_capture;
};

}

#endif /* __VPC_MSIC_H__ */

