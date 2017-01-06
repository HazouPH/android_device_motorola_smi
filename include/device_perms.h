/*
 * Copyright (C) 2012 The CyanogenMod Project
 * Copyright (C) 2017 The Lineage Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DEVICE_PERMS_H
#define DEVICE_PERMS_H

#define PROPERTY_PERMS_APPEND \
    { "persist.audio.",   AID_SYSTEM,   0 }, \
    { "tcmd.",            AID_MOT_TCMD, AID_MOT_WHISPER }, \
    { "hw.whisper",       AID_MOT_WHISPER, 0 }, \
    { "net.rmnet0.",      AID_RADIO,    0 }, \
    { "sys.media.",       AID_RADIO, 0 }, \
    { "persist.atvc.",    AID_MOT_ATVC,  0 }, \
    { "persist.atvc.",    AID_SYSTEM, 0 }, \
    { "net.vpnclient",    AID_VPN,      0 }, \
    { "wpa_supplicant.",  AID_WIFI,     0 }, \
    { "gps.",             AID_GPS,      0 }, \
    { "bluetooth.",       AID_BLUETOOTH,   0 }, \
    { "persist.gps.",      AID_GPS,      0 }, \
    { "media.",            AID_MEDIA,    0 }, \
    { "camera.",           AID_MEDIA,    0 }, \
    { "nfc.",             AID_NFC,      0 }, \
    { "hw.",              AID_MOT_OSH,  0 }, \
    { "mot.",             AID_MOT_TCMD, 0 }, \
    { "hw.",              AID_MOT_WHISPER, 0 }, \
    { "AudioComms.",       AID_MEDIA,    0 }, \
    { "audiocomms.",       AID_MEDIA,    0 }, \
    { "persist.log.",     AID_SHELL,    0 }, \
    { "persist.log.",     AID_LOG,      0 }, \
    { "persist.security.", AID_SYSTEM,   0 }, \
    { "persist.mot.proximity.", AID_RADIO, 0 }, \
    { "persist.tcmd.", AID_MOT_TCMD, AID_SYSTEM }, \
    { "persist.camera.", AID_MEDIA, 0 },

#define CONTROL_PERMS_APPEND \
    { "hciattach", AID_MOT_TCMD, AID_MOT_TCMD }, \
    { "bluetoothd",AID_MOT_TCMD, AID_MOT_TCMD }, \
    { "bt_start", AID_MOT_TCMD, AID_MOT_TCMD }, \
    { "bt_stop", AID_MOT_TCMD, AID_MOT_TCMD }, \
    { "whisperd", AID_MOT_TCMD, AID_MOT_TCMD }, \
    { "mdm_usb_suspend", AID_RADIO, AID_RADIO }, \
    { "uim",AID_BLUETOOTH, AID_BLUETOOTH },

#endif /* DEVICE_PERMS_H */
