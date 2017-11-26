#! /system/bin/sh

if [ "${1}" != "boot" ]; then
    WIFION=`getprop init.svc.wpa_supplicant`
else
    WIFION="off"
fi

WL12xx_MODULE=/system/lib/modules/wl12xx_sdio.ko
PDS_NVS_FILE=/pds/wifi/nvs_map_mac80211.bin
SOURCE_FW_DIR=/system/etc/firmware/ti-connectivity
TARGET_FW_DIR=/data/misc/wifi
TARGET_NVS_FILE=$TARGET_FW_DIR/wl1271-nvs.bin

case "$WIFION" in
  "running") echo " ****************************************"
             echo " * Turning Wi-Fi OFF before calibration *"
             echo " ****************************************"
             svc wifi disable
             rmmod $WL12xx_MODULE;;
          *) echo " ******************************"
             echo " * Starting Wi-Fi calibration *"
             echo " ******************************";;
esac

if [ -e $WL12xx_MODULE ];
then
    echo ""
else
    echo "********************************************************"
    echo "* wl12xx_sdio module not found !!"
    echo "********************************************************"
    exit
fi

# Fresh install or update copy over the generic nvs
cp $SOURCE_FW_DIR/wl1271-nvs_128x.bin $TARGET_NVS_FILE

# Set the MAC address if nvs_map exists in pds
if [ -e $PDS_NVS_FILE ];
then
    HW_MAC=`calibrator get nvs_mac $PDS_NVS_FILE | grep -o -E "([[:xdigit:]]{1,2}:){5}[[:xdigit:]]{1,2}"`
    calibrator set nvs_mac $TARGET_NVS_FILE $HW_MAC
else
    echo "********************************************************"
    echo "* /pds/wifi/nvs_map.bin not found !! Not setting MAC   *"
    echo "********************************************************"
fi

echo " ******************************"
echo " * Finished Wi-Fi calibration *"
echo " ******************************"
case "$WIFION" in
  "running") echo " *************************"
             echo " * Turning Wi-Fi back on *"
             echo " *************************"
             svc wifi enable;;
esac
