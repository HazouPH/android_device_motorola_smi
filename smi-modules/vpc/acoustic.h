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

#ifndef __VPC_ACOUSTIC_H__
#define __VPC_ACOUSTIC_H__

#include <sys/types.h>

// 'Smooth Mute' command size in bytes
#define I2C_CMD_SMOOTH_MUTE_SIZE    4

namespace android_audio_legacy
{

class acoustic
{
public :
    static int process_init();
    static void set_hac(vpc_hac_set_t state);
#ifdef ENABLE_TTY_PROFILE
    static void set_tty(vpc_tty_t state);
#endif
    static int set_smooth_mute(bool enable);
    static int process_profile(uint32_t device, uint32_t mode, vpc_band_t band);
    static int process_wake();
    static int process_suspend();

private :

    typedef enum {
        PROFILE_EARPIECE         = 0,
        PROFILE_EARPIECE_HAC,
#ifdef ENABLE_TTY_PROFILE
        PROFILE_EARPIECE_TTY_VCO,
        PROFILE_EARPIECE_TTY_HCO,
        PROFILE_EARPIECE_TTY_HCO_HAC,
#endif
        PROFILE_SPEAKER,
        PROFILE_WIRED_HEADSET,
#ifdef ENABLE_TTY_PROFILE
        PROFILE_WIRED_HEADSET_TTY_FULL,
#endif
        PROFILE_WIRED_HEADPHONE,
        PROFILE_BLUETOOTH_HSP,
        PROFILE_BLUETOOTH_CARKIT,
        PROFILE_DEFAULT,
        PROFILE_NUMBER
    } profile_id_t;

    typedef enum {
        PROFILE_MODE_IN_CALL_NB = 0,
        PROFILE_MODE_IN_CALL_WB,
        PROFILE_MODE_IN_COMM_NB,
        PROFILE_MODE_IN_COMM_WB,
        PROFILE_MODE_NUMBER
    } profile_mode_t;

    static int private_cache_profiles();
    static int private_get_profile_id(uint32_t device, profile_mode_t mode);
    static int private_wake(int fd);
    static int private_suspend(int fd);
    static int private_get_fw_label(int fd);
    static void private_discard_ack(int fd_a1026, int discard_size);

    static const int      profile_number    = PROFILE_NUMBER * PROFILE_MODE_NUMBER;
    static const int      fw_max_label_size = 100;

    static const size_t   profile_path_len_max = 80;

    static char           bid[80];
    static bool           is_a1026_init;
    static vpc_hac_set_t  hac_state;
#ifdef ENABLE_TTY_PROFILE
    static vpc_tty_t      tty_state;
#endif
    static bool           vp_bypass_on;
    static bool           vp_tuning_on;
    static const char *   vp_bypass_prop_name;
    static const char *   vp_tuning_prop_name;
    static const char *   vp_fw_name_prop_name;
    static const char *   vp_profile_prefix_prop_name;
    static int            profile_size[profile_number];
    static unsigned char *i2c_cmd_profile[profile_number];

    static const char    *profile_name[profile_number];
    static const char     i2c_cmd_smooth_mute_set[I2C_CMD_SMOOTH_MUTE_SIZE];
    static const char     i2c_cmd_smooth_mute_unset[I2C_CMD_SMOOTH_MUTE_SIZE];
};

}

#endif /* __VPC_ACOUSTIC_H__ */

