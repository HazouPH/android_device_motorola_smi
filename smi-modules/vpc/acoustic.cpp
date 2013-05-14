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

#define LOG_TAG "VPC_Acoustic"
#include <utils/Log.h>

#include <utils/threads.h>
#include <fcntl.h>
#include <linux/a1026.h>
#include <hardware_legacy/AudioSystemLegacy.h>

#include "vpc_hardware.h"
#include "acoustic.h"
#include "Property.h"

/* The time is us before the ACK of es305b can be read: Audience
 * specifies 20ms.
 */
#define ES305B_TIME_FOR_ACK_IN_US    20000

namespace android_audio_legacy
{

#define ES305_DEVICE_PATH "/dev/audience_es305"
using android::Mutex;
Mutex a1026_lock;

bool           acoustic::is_a1026_init = false;
vpc_hac_set_t  acoustic::hac_state = VPC_HAC_OFF;
#ifdef ENABLE_TTY_PROFILE
vpc_tty_t      acoustic::tty_state = VPC_TTY_OFF;
#endif
bool           acoustic::vp_bypass_on = false;
bool           acoustic::vp_tuning_on = false;
int            acoustic::profile_size[profile_number];
unsigned char *acoustic::i2c_cmd_profile[profile_number] = { NULL, };
char           acoustic::bid[80] = "";
const char *   acoustic::vp_bypass_prop_name = "persist.audiocomms.vp.bypass";
const char *   acoustic::vp_tuning_prop_name = "persist.audiocomms.vp.tuning.on";
const char *   acoustic::vp_fw_name_prop_name = "audiocomms.vp.fw_name";
const char *   acoustic::vp_profile_prefix_prop_name = "audiocomms.vp.profile_prefix";

// Command opcodes to enable smooth mute (1600dB/sec)
const char     acoustic::i2c_cmd_smooth_mute_set[] = {0x80, 0x4E, 0x06, 0x40};
// Command opcodes to disable smooth mute
const char     acoustic::i2c_cmd_smooth_mute_unset[] = {0x80, 0x4E, 0x00, 0x00};

const char *acoustic::profile_name[profile_number] = {
    /* CSV NB */
    "close_talk_csv_nb.bin",                // EP in CSV NB
    "close_talk_hac_csv_nb.bin",            // EP+HAC in CSV NB
#ifdef ENABLE_TTY_PROFILE
    "close_talk_tty_vco_csv_nb.bin",        // TTY VCO in CSV NB
    "close_talk_tty_hco_csv_nb.bin",        // TTY HCO in CSV NB
    "close_talk_tty_hco_hac_csv_nb.bin",    // TTY HCO + HAC in CSV NB
#endif
    "speaker_far_talk_csv_nb.bin",          // IHF in CSV NB
    "headset_close_talk_csv_nb.bin",        // Headset in CSV NB
#ifdef ENABLE_TTY_PROFILE
    "headset_tty_full_csv_nb.bin",          // TTY FULL in CSV NB
#endif
    "headphone_close_talk_csv_nb.bin",      // Headphone in CSV NB
    "bt_hsp_csv_nb.bin",                    // BT HSP in CSV NB
    "bt_carkit_csv_nb.bin",                 // BT Car Kit in CSV NB
    "no_acoustic_csv_nb.bin",               // All other devices in CSV NB
    /* CSV WB */
    "close_talk_csv_wb.bin",                // EP in CSV WB
    "close_talk_hac_csv_wb.bin",            // EP+HAC in CSV WB
#ifdef ENABLE_TTY_PROFILE
    "close_talk_tty_vco_csv_wb.bin",        // TTY VCO in CSV WB
    "close_talk_tty_hco_csv_wb.bin",        // TTY HCO in CSV WB
    "close_talk_tty_hco_hac_csv_wb.bin",    // TTY HCO + HAC in CSV WB
#endif
    "speaker_far_talk_csv_wb.bin",          // IHF in CSV WB
    "headset_close_talk_csv_wb.bin",        // Headset in CSV WB
#ifdef ENABLE_TTY_PROFILE
    "headset_tty_full_csv_wb.bin",          // TTY FULL in CSV WB
#endif
    "headphone_close_talk_csv_wb.bin",      // Headphone in CSV WB
    "bt_hsp_csv_wb.bin",                    // BT HSP in CSV WB
    "bt_carkit_csv_wb.bin",                 // BT Car Kit in CSV WB
    "no_acoustic_csv_wb.bin",               // All other devices in CSV WB
    /* VOIP NB */
    "close_talk_voip_nb.bin",               // EP in VOIP NB
    "close_talk_hac_voip_nb.bin",           // EP+HAC in VOIP NB
#ifdef ENABLE_TTY_PROFILE
    NULL,                                   // TTY VCO (NA in VoIP)
    NULL,                                   // TTY HCO (NA in VoIP)
    NULL,                                   // TTY HCO + HAC (NA in VoIP)
#endif
    "speaker_far_talk_voip_nb.bin",         // IHF in VOIP NB
    "headset_close_talk_voip_nb.bin",       // Headset in VOIP NB
#ifdef ENABLE_TTY_PROFILE
    NULL,                                   // TTY FULL (NA in VoIP)
#endif
    "headphone_close_talk_voip_nb.bin",     // Headphone in VOIP NB
    "bt_hsp_voip_nb.bin",                   // BT HSP in VOIP NB
    "bt_carkit_voip_nb.bin",                // BT Car Kit in VOIP NB
    "no_acoustic_voip_nb.bin",              // All other devices in VOIP NB
    /* VOIP WB */
    "close_talk_voip_wb.bin",               // EP in VOIP WB
    "close_talk_hac_voip_wb.bin",           // EP+HAC in VOIP WB
#ifdef ENABLE_TTY_PROFILE
    NULL,                                   // TTY VCO (NA in VoIP)
    NULL,                                   // TTY HCO (NA in VoIP)
    NULL,                                   // TTY HCO + HAC (NA in VoIP)
#endif
    "speaker_far_talk_voip_wb.bin",         // IHF in VOIP WB
    "headset_close_talk_voip_wb.bin",       // Headset in VOIP WB
#ifdef ENABLE_TTY_PROFILE
    NULL,                                   // TTY FULL (NA in VoIP)
#endif
    "headphone_close_talk_voip_wb.bin",     // Headphone in VOIP WB
    "bt_hsp_voip_wb.bin",                   // BT HSP in VOIP WB
    "bt_carkit_voip_wb.bin",                // BT Car Kit in VOIP WB
    "no_acoustic_voip_wb.bin"               // All other devices in VOIP WB
};


/*---------------------------------------------------------------------------*/
/* Load profiles in cache                                                    */
/*---------------------------------------------------------------------------*/
int acoustic::private_cache_profiles()
{
    TProperty<string> vp_profile_prefix(vp_profile_prefix_prop_name, "/system/etc/phonecall_es305b_");

    LOGD("Initialize Audience A1026 profiles cache\n");

    for (int i = 0; i < profile_number; i++)
    {
#ifdef ENABLE_TTY_PROFILE
        if (profile_name[i] == NULL) {
            // Means that this profile is not supported
            i2c_cmd_profile[i] = NULL;
            continue;
        }
#endif
        string strProfilePath = vp_profile_prefix;
        strProfilePath += string(profile_name[i]);

        FILE *fd = fopen(strProfilePath.c_str(), "r");
        if (fd == NULL) {
#ifdef ENABLE_TTY_PROFILE
            LOGW("Cannot open %s\n", strProfilePath.c_str());
            // This profile won't be selectable
            i2c_cmd_profile[i] = NULL;
            continue;
#else
            LOGE("Cannot open %s\n", strProfilePath.c_str());
            goto return_error;
#endif
        }

        fseek(fd, 0, SEEK_END);
        profile_size[i] = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        LOGD("Profile %d : size = %d, \t path = %s", i, profile_size[i], strProfilePath.c_str());

        if (i2c_cmd_profile[i] != NULL)
            free(i2c_cmd_profile[i]);

        i2c_cmd_profile[i] = (unsigned char*)malloc(sizeof(unsigned char) * profile_size[i]);
        if (i2c_cmd_profile[i] == NULL) {
            LOGE("Could not allocate memory\n");
            fclose(fd);
            goto return_error;
        }
        else
            memset(i2c_cmd_profile[i], '\0', profile_size[i]);

        int rc;
        rc = fread(&i2c_cmd_profile[i][0], 1, profile_size[i], fd);
        if (rc < profile_size[i]) {
            LOGE("Error while reading config file\n");
            fclose(fd);
            goto return_error;
        }
        fclose(fd);
    }
#ifdef ENABLE_TTY_PROFILE
    // Check that default profile has been loaded
    if (i2c_cmd_profile[PROFILE_DEFAULT] == NULL) {
        LOGE("Audience default profile not found.\n");
        goto return_error;
    }

#endif
    LOGD("Audience A1026 profiles cache OK\n");
    return 0;

return_error:

    LOGE("Audience A1026 profiles cache failed\n");
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Get profile ID to access cache                                            */
/*---------------------------------------------------------------------------*/
int acoustic::private_get_profile_id(uint32_t device, profile_mode_t mode)
{
    int profile_id = PROFILE_DEFAULT;

        // Associate a profile to the detected device
    if (device & AudioSystem::DEVICE_OUT_EARPIECE) {
        if (hac_state == VPC_HAC_OFF) {
            LOGD("Earpiece device detected, => force use of Earpiece device profile\n");
            profile_id = PROFILE_EARPIECE;
        } else {
            LOGD("Earpiece device detected in HAC mode, => force use of Earpiece+HAC device profile\n");
            profile_id = PROFILE_EARPIECE_HAC;
        }
    } else if (device & AudioSystem::DEVICE_OUT_SPEAKER) {
        LOGD("Speaker device detected, => force use of Speaker device profile\n");
        profile_id = PROFILE_SPEAKER;
    } else if (device & AudioSystem::DEVICE_OUT_WIRED_HEADSET) {
#ifdef ENABLE_TTY_PROFILE
        if (mode == PROFILE_MODE_IN_COMM_NB || mode == PROFILE_MODE_IN_COMM_WB) {
            // In VoIP, TTY is not supported
            LOGD("Headset device detected, => force use of Headset device profile\n");
            profile_id = PROFILE_WIRED_HEADSET;
        } else {
            switch (tty_state) {
                default:
                    // Intended fall through:
                case VPC_TTY_OFF:
                    LOGD("Headset device detected, => force use of Headset device profile\n");
                    profile_id = PROFILE_WIRED_HEADSET;
                    break;
                case VPC_TTY_FULL:
                    LOGD("Headset device detected with TTY_FULL, => force use of TTY_FULL device profile\n");
                    profile_id = PROFILE_WIRED_HEADSET_TTY_FULL;
                    break;
                case VPC_TTY_HCO:
                    if (hac_state == VPC_HAC_OFF) {
                        LOGD("Headset device detected with TTY_HCO, => force use of close talk TTY_HCO device profile\n");
                        profile_id = PROFILE_EARPIECE_TTY_HCO;
                    } else {
                        LOGD("Headset device detected with TTY_HCO + HAC mode, => force use of close talk TTY_HCO_HAC device profile\n");
                        profile_id = PROFILE_EARPIECE_TTY_HCO_HAC;
                    }
                    break;
                case VPC_TTY_VCO:
                    LOGD("Headset device detected with TTY_VCO, => force use of close talk TTY_VCO device profile\n");
                    profile_id = PROFILE_EARPIECE_TTY_VCO;
                    break;
            }
        }
#else
        LOGD("Headset device detected, => force use of Headset device profile\n");
        profile_id = PROFILE_WIRED_HEADSET;
#endif
    } else if (device & AudioSystem::DEVICE_OUT_WIRED_HEADPHONE) {
        LOGD("Headphone device detected, => force use of Headphone device profile\n");
        profile_id = PROFILE_WIRED_HEADPHONE;
    } else if (device & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO) {
        LOGD("BT SCO device detected, => force use of BT HSP device profile\n");
        profile_id = PROFILE_BLUETOOTH_HSP;
    } else if (device & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_HEADSET) {
        LOGD("BT SCO Headset device detected, => force use of BT HSP device profile\n");
        profile_id = PROFILE_BLUETOOTH_HSP;
    } else if (device & AudioSystem::DEVICE_OUT_BLUETOOTH_SCO_CARKIT) {
        LOGD("BT SCO CarKit device detected, => force use of BT CARKIT device profile\n");
        profile_id = PROFILE_BLUETOOTH_CARKIT;
    } else {
        LOGD("No device detected, => force use of DEFAULT device profile\n");
        profile_id = PROFILE_DEFAULT;
    }
    profile_id += PROFILE_NUMBER * mode;

    LOGD("Profile %d : size = %d, name = %s", profile_id, profile_size[profile_id], profile_name[profile_id]);

    return profile_id;
}

/*---------------------------------------------------------------------------*/
/* Private wake method                                                       */
/*---------------------------------------------------------------------------*/
int acoustic::private_wake(int fd)
{
    int rc;
    rc = ioctl(fd, A1026_ENABLE_CLOCK);
    if (rc)
        LOGE("Audience A1026 wake error\n");

    return rc;
}

/*---------------------------------------------------------------------------*/
/* Private suspend method                                                    */
/*---------------------------------------------------------------------------*/
int acoustic::private_suspend(int fd)
{
    int rc;

    rc = write(fd, &i2c_cmd_profile[PROFILE_DEFAULT][0], profile_size[PROFILE_DEFAULT]);
    if (rc != profile_size[PROFILE_DEFAULT]) {
        LOGE("Audience A1026 write error, pass-through mode failed\n");
    } else {
        /* All commands are acknowledged by es305b with responses which are the
         * commands themselves: discard them. Audience recommends to wait 20ms
         * per command before to read the ACK */
        usleep((profile_size[PROFILE_DEFAULT] / 4) * ES305B_TIME_FOR_ACK_IN_US);
        private_discard_ack(fd, profile_size[PROFILE_DEFAULT]);

        if (vp_tuning_on != true) {
            rc = ioctl(fd, A1026_SUSPEND);
            if (rc)
                LOGE("Audience A1026 suspend error\n");
        } else {
            rc = 0;
            LOGW("%s: %s is set: Audience suspend aborted\n", __FUNCTION__, vp_tuning_prop_name);
        }
    }

    return rc;
}

/*---------------------------------------------------------------------------*/
/* Private ACK discard method                                                */
/*---------------------------------------------------------------------------*/
void acoustic::private_discard_ack(int fd_a1026, int discard_size)
{
#define TRASH_BUFFER_SIZE    32
#define min(a,b)             ((a) < (b) ? (a):(b))
    int rc;
    int max_retry = 3;
    char trash[TRASH_BUFFER_SIZE];

    LOGD("Discard %d byte(s) in es305b tx fifo\n", discard_size);

    while (discard_size && max_retry) {
        rc = read(fd_a1026, trash, min(discard_size, TRASH_BUFFER_SIZE));
        if (rc < 0) {
            LOGE("Discard fails due to A1026_READ_DATA error, ret = %d\n", rc);
            break;
        }
        if (rc == 0) {
            max_retry--;
            LOGW("0 bytes discarded in es305b fifo, %d remaining. Retry.\n", discard_size);
            usleep(ES305B_TIME_FOR_ACK_IN_US);
        } else {
            discard_size -= rc;
        }
    }
    if (discard_size)
        LOGE("Discard aborted with %d bytes not discarded\n", discard_size);
}

/*---------------------------------------------------------------------------*/
/* Get Firmware label                                                        */
/*---------------------------------------------------------------------------*/
int acoustic::private_get_fw_label(int fd)
{
    const unsigned char firstCharLabelCmd[4] = {0x80, 0x20, 0x00, 0x00};
    const unsigned char nextCharLabelCmd[4] = {0x80, 0x21, 0x00, 0x00};
    unsigned char label[4] = {0x00, 0x00, 0x00, 0x01};
    unsigned char *label_name;
    int i = 1, size = 4, rc;

    LOGD("Read Audience A1026 FW label\n");

    label_name = (unsigned char*)malloc(sizeof(unsigned char) * fw_max_label_size);
    if (label_name == NULL) {
        LOGE("Unable to allocate FW label buffer (%d bytes)\n",fw_max_label_size);
        goto return_error;
    }
    // Get first build label char
    rc = write(fd, firstCharLabelCmd, size);
    if (rc != size) {
        LOGE("A1026_WRITE_MSG (0x%.2x%.2x%.2x%.2x) error, ret = %d\n", firstCharLabelCmd[0], firstCharLabelCmd[1], firstCharLabelCmd[2], firstCharLabelCmd[3], rc);
        goto return_error;
    }
    usleep(20000);

    rc = read(fd, label, size);
    if (rc != size) {
        LOGE("A1026_READ_DATA error, ret = %d\n", rc);
        goto return_error;
    }
    label_name[0] = label[3];

    // Get next build label char
    while (label[3] && (i < fw_max_label_size))
    {
        rc = write(fd, nextCharLabelCmd, size);
        if (rc != 4) {
            LOGE("A1026_WRITE_MSG (0x%.2x%.2x%.2x%.2x) error, rc = %d\n", nextCharLabelCmd[0], nextCharLabelCmd[1], nextCharLabelCmd[2], nextCharLabelCmd[3], rc);
            goto return_error;
        }
        usleep(20000);

        rc = read(fd, label, size);
        if (rc != 4) {
            LOGE("A1026_READ_DATA error, ret = %d\n", rc);
            goto return_error;
        }
        label_name[i] = label[3];
        i++;
    }

    if (i >= fw_max_label_size) {
        LOGE("FW label invalid or too long\n");
        goto return_error;
    }

    LOGD("Audience A1026 FW label : %s\n",label_name);
    free(label_name);
    return 0;

return_error:

    LOGE("Audience A1026 read FW label failed\n");
    free(label_name);
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Initialization                                                            */
/*---------------------------------------------------------------------------*/
int acoustic::process_init()
{
    a1026_lock.lock();
    LOGD("Initialize Audience A1026\n");

    int fd_a1026 = -1;
    int rc;
    TProperty<bool> vp_bypass_prop(vp_bypass_prop_name, false);
    TProperty<bool> vp_tuning_prop(vp_tuning_prop_name, false);
    TProperty<string> vp_fw_name(vp_fw_name_prop_name, "vpimg_es305b.bin");

    rc = private_cache_profiles();
    if (rc) goto return_error;

    fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR | O_NONBLOCK, 0);
    if (fd_a1026 < 0) {
        LOGE("Cannot open %s %d\n", ES305_DEVICE_PATH, fd_a1026);
        goto return_error;
    }

    LOGD("Wake Audience A1026\n");
    rc = private_wake(fd_a1026);
    if (rc) goto return_error;

    LOGD("Load Audience A1026 FW\n");
    rc = ioctl(fd_a1026, A1026_BOOTUP_INIT, ((string)vp_fw_name).c_str());
    if (rc) goto return_error;

    rc = private_get_fw_label(fd_a1026);
    if (rc) goto return_error;

    LOGD("Suspend Audience A1026\n");
    rc = private_suspend(fd_a1026);
    if (rc) goto return_error;

    LOGD("Audience A1026 init OK\n");
    is_a1026_init = true;
    close(fd_a1026);
    a1026_lock.unlock();

    vp_bypass_on = vp_bypass_prop;
    if (vp_bypass_on == true)
        LOGW("%s: %s is set: Audience digital hardware pass through will be forced\n", __FUNCTION__, vp_bypass_prop_name);

    vp_tuning_on = vp_tuning_prop;
    if (vp_tuning_on == true)
        LOGW("%s: %s is set: Audience tuning mode is on and will never be suspended\n", __FUNCTION__, vp_tuning_prop_name);

    return 0;

return_error:

    LOGE("Audience A1026 init failed\n");
    is_a1026_init = false;
    if (fd_a1026 >= 0)
        close(fd_a1026);
    a1026_lock.unlock();
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Public HAC set method                                                     */
/*---------------------------------------------------------------------------*/
void acoustic::set_hac(vpc_hac_set_t state)
{
    hac_state = state;
}

#ifdef ENABLE_TTY_PROFILE
/*---------------------------------------------------------------------------*/
/* Public TTY set method                                                     */
/*---------------------------------------------------------------------------*/
void acoustic::set_tty(vpc_tty_t state)
{
    tty_state = state;
}
#endif

/*---------------------------------------------------------------------------*/
/* Public smooth mute feature set method                                     */
/*---------------------------------------------------------------------------*/
int acoustic::set_smooth_mute(bool enable)
{
    int fd_a1026 = -1;
    int rc;
    const char * smooth_mute_cmd = enable ? i2c_cmd_smooth_mute_set : i2c_cmd_smooth_mute_unset;

    a1026_lock.lock();
    if (!is_a1026_init) {
        LOGE("Audience A1026 not initialized.\n");
        goto return_error;
    }

    fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR);
    if (fd_a1026 < 0) {
        LOGE("Cannot open audience_a1026 device (%d)\n", fd_a1026);
        goto return_error;
    }

    rc = write(fd_a1026, smooth_mute_cmd, I2C_CMD_SMOOTH_MUTE_SIZE);
    if (rc != I2C_CMD_SMOOTH_MUTE_SIZE) {
        LOGE("Audience write error \n");
        goto return_error;
    }
    usleep(ES305B_TIME_FOR_ACK_IN_US);
    private_discard_ack(fd_a1026, I2C_CMD_SMOOTH_MUTE_SIZE);

    close(fd_a1026);
    a1026_lock.unlock();
    return 0;

return_error:
    LOGE("Audience set smooth mute failed.\n");
    if (fd_a1026 >= 0)
        close(fd_a1026);
    a1026_lock.unlock();
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Public wake method                                                       */
/*---------------------------------------------------------------------------*/
int acoustic::process_wake()
{
    a1026_lock.lock();
    LOGD("Wake Audience A1026\n");

    int fd_a1026 = -1;
    int rc;

    if (!is_a1026_init) {
        LOGE("Audience A1026 not initialized, nothing to wake.\n");
        goto return_error;
    }

    fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR);
    if (fd_a1026 < 0) {
        LOGE("Cannot open audience_a1026 device (%d)\n", fd_a1026);
        goto return_error;
    }

    rc = private_wake(fd_a1026);
    if (rc) goto return_error;

    LOGD("Audience A1026 wake OK\n");
    close(fd_a1026);
    a1026_lock.unlock();
    return 0;

return_error:

    LOGE("Audience A1026 wake failed\n");
    if (fd_a1026 >= 0)
        close(fd_a1026);
    a1026_lock.unlock();
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Public suspend method                                                     */
/*---------------------------------------------------------------------------*/
int acoustic::process_suspend()
{
    a1026_lock.lock();
    LOGD("Suspend Audience A1026\n");

    int fd_a1026 = -1;
    int rc = -1;

    if (!is_a1026_init) {
        LOGE("Audience A1026 not initialized, nothing to suspend.\n");
        goto return_error;
    }

    fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR);
    if (fd_a1026 < 0) {
        LOGE("Cannot open audience_a1026 device (%d)\n", fd_a1026);
        goto return_error;
    }

    static const int retry = 4;
    for (int i = 0; i < retry; i++) {
        rc = private_suspend(fd_a1026);
        if (!rc)
            break;
    }

    if (rc) {
        LOGE("A1026 do hard reset to recover from error!\n");
        close(fd_a1026);
        fd_a1026 = -1;
        a1026_lock.unlock();

        rc = process_init(); // A1026 needs to do hard reset!
        a1026_lock.lock();
        if (rc) {
            LOGE("A1026 Fatal Error: Re-init A1026 Failed\n");
            goto return_error;
        }

        // After process_init(), fd_a1026 is -1
        fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR);
        if (fd_a1026 < 0) {
            LOGE("A1026 Fatal Error: unable to open A1026 after hard reset\n");
            goto return_error;
        }

        rc = private_suspend(fd_a1026);
        if (rc) {
            LOGE("A1026 Fatal Error: unable to A1026_SET_CONFIG after hard reset\n");
            close(fd_a1026);
            fd_a1026 = -1;
            goto return_error;
        }
        else
            LOGD("Set_config Ok after Hard Reset\n");
    }

    LOGD("Audience A1026 suspend OK\n");
    close(fd_a1026);
    a1026_lock.unlock();
    return 0;

return_error:

    LOGE("Audience A1026 suspend failed\n");
    if (fd_a1026 >= 0)
        close(fd_a1026);
    a1026_lock.unlock();
    return -1;
}

/*---------------------------------------------------------------------------*/
/* Set profile                                                               */
/*---------------------------------------------------------------------------*/
int acoustic::process_profile(uint32_t device, uint32_t mode, vpc_band_t band)
{
    a1026_lock.lock();
    LOGD("Set Audience A1026 profile\n");

    int fd_a1026 = -1;
    int rc;
    int profile_id;
    profile_mode_t profile_mode;

    if (!is_a1026_init) {
        LOGE("Audience A1026 not initialized.\n");
        goto return_error;
    }

    fd_a1026 = open(ES305_DEVICE_PATH, O_RDWR);
    if (fd_a1026 < 0) {
        LOGE("Cannot open audience_a1026 device (%d)\n", fd_a1026);
        goto return_error;
    }
    LOGD("%s: Selecting profile for device=0x%08X mode=%d band=%d\n", __FUNCTION__, device, mode, band);

    if (vp_bypass_on == false) {

        if (mode == AudioSystem::MODE_IN_CALL) {
            profile_mode = band == VPC_BAND_NARROW ? PROFILE_MODE_IN_CALL_NB : PROFILE_MODE_IN_CALL_WB;
            profile_id = private_get_profile_id(device, profile_mode);
        } else if (mode == AudioSystem::MODE_IN_COMMUNICATION) {
            profile_mode = band == VPC_BAND_NARROW ? PROFILE_MODE_IN_COMM_NB : PROFILE_MODE_IN_COMM_WB;
            profile_id = private_get_profile_id(device, profile_mode);
        } else {
            /* That case should not occur: fall into digital hardware passthrough */
            profile_id = PROFILE_DEFAULT;
            LOGW("%s: Unhandled mode %d: force Audience in digital hardware pass through\n", __FUNCTION__, mode);
        }
    } else {
        profile_id = PROFILE_DEFAULT;
        LOGW("%s: %s is set: force Audience in digital hardware pass through\n", __FUNCTION__, vp_bypass_prop_name);
    }

#ifdef ENABLE_TTY_PROFILE
    // Check that the selected profile is valid
    if (i2c_cmd_profile[profile_id] == NULL) {
        LOGE("%s: Unsupported profile selected (%d): force DEFAULT profile\n", __FUNCTION__, profile_id);
        profile_id = PROFILE_DEFAULT;
    }
#endif

    rc = write(fd_a1026, &i2c_cmd_profile[profile_id][0], profile_size[profile_id]);
    if (rc != profile_size[profile_id]) {
        LOGE("Audience write error \n");
        goto return_error;
    }
    /* All commands are acknowledged by es305b with responses which are the
     * commands themselves: discard them. Audience recommends to wait 20ms
     * per command before to read the ACK */
    usleep((profile_size[profile_id] / 4) * ES305B_TIME_FOR_ACK_IN_US);
    private_discard_ack(fd_a1026, profile_size[profile_id]);

    LOGD("Audience A1026 set profile OK\n");
    close(fd_a1026);
    a1026_lock.unlock();
    return 0;

return_error:

    LOGE("Audience A1026 set profile failed\n");
    if (fd_a1026 >= 0)
        close(fd_a1026);
    a1026_lock.unlock();
    return -1;
}

}

