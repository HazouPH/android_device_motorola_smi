#!/system/bin/sh

PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin
ORIG_NVS_BIN=/system/etc/firmware/ti-connectivity/wl1271-nvs_128x.bin
NVS_BIN=/system/etc/firmware/ti-connectivity/wl1271-nvs.bin
PDS_NVS_BIN=/pds/wifi/nvs_map_mac80211.bin

if busybox [ ! -f "$NVS_BIN" ]; then
    toolbox mount -o remount,rw /system
    toolbox cp ${ORIG_NVS_BIN} ${NVS_BIN}
    MAC=`calibrator get nvs_mac $PDS_NVS_BIN | grep -o -E "([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}"`
    calibrator set nvs_mac $NVS_BIN $MAC
    toolbox mount -o remount,ro /system
fi
