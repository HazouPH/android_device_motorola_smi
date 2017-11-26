#! /system/bin/sh
# In some cases calibration is getting interupted before it finishes

if [ ! -f /data/misc/wifi/wl1271-nvs.bin -o ! -s /data/misc/wifi/wl1271-nvs.bin ]; then
	if [ "${1}" != "boot" ]; then
		rmmod wl12xx_sdio
	fi

        wifical.sh "${1}"
fi
