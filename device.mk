DEVICE_FOLDER := device/motorola/smi

# Get smi-plus files if available for full build
$(call inherit-product-if-exists, device/motorola/smi-plus/device.mk)

# (2) Also get non-open-source specific aspects if available
$(call inherit-product-if-exists, vendor/motorola/smi/smi-vendor.mk)

## overlays
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_FOLDER)/overlay

# Device uses high-density artwork where available
PRODUCT_LOCALES := en_US
PRODUCT_LOCALES += hdpi
PRODUCT_AAPT_CONFIG := normal hdpi xhdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Prebuilt files required
INR_X86_FILES := $(wildcard $(DEVICE_FOLDER)/ramdisk/init-files/*.*)
MDR_X86_FILES := $(wildcard $(DEVICE_FOLDER)/ramdisk/lib/modules/*.ko)
PMS_MOT_FILES := $(wildcard $(DEVICE_FOLDER)/prebuilt/permissions/*.xml)

# Copying grouped files
PRODUCT_COPY_FILES += \
	$(foreach i, $(INR_X86_FILES), $(i):root/$(notdir $(i))) \
	$(foreach i, $(PMS_MOT_FILES), $(i):system/etc/permissions/$(notdir $(i))) \
	$(foreach i, $(MDR_X86_FILES), $(i):root/lib/modules/$(notdir $(i))) \

# Touchscreen
PRODUCT_COPY_FILES += \
    $(DEVICE_FOLDER)/prebuilt/idc/atmxt-i2c.idc:system/usr/idc/atmxt-i2c.idc \
    $(DEVICE_FOLDER)/prebuilt/idc/mxt224_touchscreen_0.idc:system/usr/idc/mxt224_touchscreen_0.idc

# APN List, Telephony permissions
PRODUCT_COPY_FILES += \
    device/sample/etc/apns-full-conf.xml:system/etc/apns-conf.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml

# Audio
PRODUCT_PACKAGES += \
    audio.usb.default \
    audio.a2sp.default \
    libaudioutils \
    libasound \
    alsa_aplay \
    alsa_ctl \
    alsa_amixer

# GPS
PRODUCT_PACKAGES += \
#    gps.smi

# Tiny Utils
PRODUCT_PACKAGES += \
    libtinyalsa

# Charger
PRODUCT_PACKAGES += charger charger_res_images

# Wifi
PRODUCT_PACKAGES += \
    lib_driver_cmd_wl12xx \
    dhcpcd.conf \
    hostapd.conf \
    wpa_supplicant.conf \
    crda \
    regulatory.bin \
    calibrator \
    wlan_prov

# Wifi symlink
$(shell ln -sf /pds/wifi/nvs_map_mac80211.bin out/target/product/smi/system/etc/firmware/ti-connectivity/wl12xx-fac-nvs.bin)

# Filesystem management tools
PRODUCT_PACKAGES +=\
    make_ext4fs \
    e2fsck \
    setup_fs \
    libext4_utils

# Build tools
PRODUCT_PACKAGES += \
    pack_intel \
    unpack_intel \

# Video
PRODUCT_PACKAGES += \
    libwrs_omxil_core_pvwrapped \
    libwrs_omxil_common \
    libva \
    libva-tpi \
    libva-android

# Ramdisk
PRODUCT_PACKAGES += \
    fstab.sc1 \
    init.avc.rc \
    init.debug.rc \
    init.duag.rc \
    init.oom.rc \
    init.sc1.rc \
    init.smi.usb.rc \
    init.smi.usb.sh \
    init.tcmd.rc \
    init.wifi.rc \
    init.wireless.rc \
    ueventd.sc1.rc

# Misc Packages
PRODUCT_PACKAGES += \
    busybox \
    perf

# Misc Prebuilt
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab \
	$(LOCAL_PATH)/blobs/atmxt-r2.tdat:recovery/root/vendor/firmware/atmxt-r2.tdat \
	$(LOCAL_PATH)/blobs/watchdogd:recovery/root/watchdogd \
	$(LOCAL_PATH)/blobs/watchdogd:root/watchdogd \
	$(LOCAL_PATH)/postrecoveryboot.sh:recovery/root/sbin/postrecoveryboot.sh

# For userdebug/eng builds
ADDITIONAL_DEFAULT_PROPERTIES += \
	panel.physicalWidthmm=52 \
	panel.physicalHeightmm=89 \
	ro.opengles.version=131072 \
	gsm.net.interface=rmnet0 \
	persist.system.at-proxy.mode=0 \
	persist.ril-daemon.disable=0 \
	ro.secure=0 \
	ro.adb.secure=0 \
	ro.allow.mock.location=1 \
	ro.debuggable=1 \
	wifi.interface=wlan0:0 \
	persist.sys.usb.config=mass_storage

PRODUCT_PROPERTY_OVERRIDES += \
	qemu.hw.mainkeys=0 \
	ro.sf.lcd_density=240

# Inherit dalvik configuration and the rest of the platform
$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# Get Arm translator
$(call inherit-product-if-exists, vendor/intel/houdini.mk)

# FM Radio Support TI
$(call inherit-product-if-exists, vendor/ti/fmradio/fmradio.mk)
