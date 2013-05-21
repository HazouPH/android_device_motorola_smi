DEVICE_FOLDER := device/motorola/smi

## (2) Also get non-open-source specific aspects if available
$(call inherit-product-if-exists, vendor/motorola/smi/smi-vendor.mk)

## overlays
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_FOLDER)/overlay

# Device uses high-density artwork where available
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Files needed for boot image
PRODUCT_COPY_FILES += \
    $(DEVICE_FOLDER)/ramdisk/init.smi.rc:root/init.smi.rc \
    $(DEVICE_FOLDER)/ramdisk/init.smi.usb.rc:root/init.smi.usb.rc \
    $(DEVICE_FOLDER)/ramdisk/init.sc1.rc:root/init.sc1.rc \
    $(DEVICE_FOLDER)/ramdisk/init.sdcard.rc:root/init.sdcard.rc \
    $(DEVICE_FOLDER)/ramdisk/init.wifi.rc:root/init.wifi.rc \
    $(DEVICE_FOLDER)/ramdisk/init.xmm.rc:root/init.xmm.rc \
    $(DEVICE_FOLDER)/ramdisk/ueventd.smi.rc:root/ueventd.smi.rc \
    $(DEVICE_FOLDER)/ramdisk/android.fstab:root/android.fstab

# Lib modules needed for boot
PRODUCT_COPY_FILES += \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/atomisp.ko:root/lib/modules/atomisp.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/btwilink.ko:root/lib/modules/btwilink.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/cfg80211.ko:root/lib/modules/cfg80211.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/compat.ko:root/lib/modules/compat.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/gps_drv.ko:root/lib/modules/gps_drv.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/intel_mid_ssp_test_driver.ko:root/lib/modules/intel_mid_ssp_test_driver.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/ir-kbd-i2c.ko:root/lib/modules/ir-kbd-i2c.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lc898211.ko:root/lib/modules/lc898211.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lib80211.ko:root/lib/modules/lib80211.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lib80211_crypt_ccmp.ko:root/lib/modules/lib80211_crypt_ccmp.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lib80211_crypt_tkip.ko:root/lib/modules/lib80211_crypt_tkip.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lib80211_crypt_wep.ko:root/lib/modules/lib80211_crypt_wep.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lm3554.ko:root/lib/modules/lm3554.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/lm3556.ko:root/lib/modules/lm3556.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/mac80211.ko:root/lib/modules/mac80211.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/mt9e013.ko:root/lib/modules/mt9e013.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/ov7736.ko:root/lib/modules/ov7736.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/videobuf2-core.ko:root/lib/modules/videobuf2-core.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/videobuf2-memops.ko:root/lib/modules/videobuf2-memops.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/wl12xx.ko:root/lib/modules/wl12xx.ko \
    $(DEVICE_FOLDER)/ramdisk/lib/modules/wl12xx_sdio.ko:root/lib/modules/wl12xx_sdio.ko

PRODUCT_COPY_FILES += \
    $(DEVICE_FOLDER)/prebuilt/idc/atmxt-i2c.idc:system/usr/idc/atmxt-i2c.idc \
    $(DEVICE_FOLDER)/prebuilt/idc/mxt224_touchscreen_0.idc:system/usr/idc/mxt224_touchscreen_0.idc

# Files needed for Alsa conf
LOCAL_ALSA_CONF_DIR  := $(LOCAL_PATH)/smi-modules/Alsa-lib
PRODUCT_COPY_FILES += \
        $(LOCAL_ALSA_CONF_DIR)/alsa.conf:system/usr/share/alsa/alsa.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/dsnoop.conf:system/usr/share/alsa/pcm/dsnoop.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/modem.conf:system/usr/share/alsa/pcm/modem.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/dpl.conf:system/usr/share/alsa/pcm/dpl.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/default.conf:system/usr/share/alsa/pcm/default.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/surround51.conf:system/usr/share/alsa/pcm/surround51.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/surround41.conf:system/usr/share/alsa/pcm/surround41.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/surround50.conf:system/usr/share/alsa/pcm/surround50.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/dmix.conf:system/usr/share/alsa/pcm/dmix.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/center_lfe.conf:system/usr/share/alsa/pcm/center_lfe.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/surround40.conf:system/usr/share/alsa/pcm/surround40.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/side.conf:system/usr/share/alsa/pcm/side.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/iec958.conf:system/usr/share/alsa/pcm/iec958.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/rear.conf:system/usr/share/alsa/pcm/rear.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/surround71.conf:system/usr/share/alsa/pcm/surround71.conf \
        $(LOCAL_ALSA_CONF_DIR)/pcm/front.conf:system/usr/share/alsa/pcm/front.conf \
        $(LOCAL_ALSA_CONF_DIR)/cards/aliases.conf:system/usr/share/alsa/cards/aliases.conf

PRODUCT_PACKAGES += \
    apache-harmony-tests \
    bd_prov \
    depmod \
    dhcp6c \
    jcifs-1.3.16 \
    libglib-2.0 \
    libgmodule-2.0 \
    libgobject-2.0 \
    libgthread-2.0 \
    libpsb_drm \
    libmemrar \
    libtinyalsa \
    libtinyxml \
    libwbxmlparser \
    libz \
    pack_intel \
    uim \
    unpack_intel \
    libproperty \
    libevent-listener \
    libext4_utils
#    bthelp \
#    btcmd \
#    libdrm

# Audio
PRODUCT_PACKAGES += \
    alsa.smi \
    alsa_aplay \
    audio.a2dp.default \
    audio.usb.default \
    audio_policy.smi \
    audio.primary.smi \
    libalsa-intf \
    libasound \
    libaudioutils \
    libbluetooth-audio

# NFC Support
PRODUCT_PACKAGES += \
    libnfc \
    libnfc_jni \
    Nfc \
    Tag \
    com.android.nfc_extras

# Wifi
PRODUCT_PACKAGES += \
    lib_driver_cmd_wl12xx \
    dhcpcd.conf \
    hostapd.conf \
    wifical.sh \
    wpa_supplicant.conf \
    TQS_D_1.7.ini \
    TQS_D_1.7_127x.ini \
    crda \
    regulatory.bin \
    calibrator \
    busybox \
    wlan_prov

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab \
	$(LOCAL_PATH)/postrecoveryboot.sh:recovery/root/sbin/postrecoveryboot.sh \
	$(LOCAL_PATH)/bootanimation.zip:system/media/bootanimation.zip \

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/blobs/watchdogd:recovery/root/sbin/watchdogd \
	$(LOCAL_PATH)/blobs/atmxt-r2.tdat:recovery/root/vendor/firmware/atmxt-r2.tdat \

# Inherit dalvik configuration and the rest of the platform
$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)
$(call inherit-product,$(SRC_TARGET_DIR)/product/generic_x86.mk)
$(call inherit-product, build/target/product/full_base_telephony.mk)
