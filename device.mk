DEVICE_FOLDER := device/motorola/smi

# Get smi-plus files if available for full build
$(include inherit-product-if-exists, device/motorola/smi-plus/device.mk)

# (2) Also get non-open-source specific aspects if available
$(call inherit-product-if-exists, vendor/motorola/smi/smi-vendor.mk)

## overlays
DEVICE_PACKAGE_OVERLAYS += $(DEVICE_FOLDER)/overlay

# Device uses high-density artwork where available
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

# Audio
PRODUCT_PACKAGES += \
    audio.usb.default \
    libaudioutils \

# GPS
PRODUCT_PACKAGES += \
#    gps.smi

# Tiny Utils
PRODUCT_PACKAGES += \
    libtinyalsa

# Charger
PRODUCT_PACKAGES += charger charger_res_images

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
    wpa_supplicant.conf \
    crda \
    regulatory.bin \
    calibrator \
    wlan_prov

# Filesystem management tools
PRODUCT_PACKAGES += \
    make_ext4fs \
    e2fsck \
    setup_fs \
    libext4_utils

# Build tools
PRODUCT_PACKAGES += \
    pack_intel \
    unpack_intel \

# Misc Packages
PRODUCT_PACKAGES += \
    busybox \
    perf

# Misc Prebuilt
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/vold.fstab:system/etc/vold.fstab \
	$(LOCAL_PATH)/bootanimation.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/blobs/atmxt-r2.tdat:recovery/root/vendor/firmware/atmxt-r2.tdat \
	$(LOCAL_PATH)/blobs/watchdogd:recovery/root/watchdogd \
	$(LOCAL_PATH)/blobs/watchdogd:root/watchdogd \
	$(LOCAL_PATH)/postrecoveryboot.sh:recovery/root/sbin/postrecoveryboot.sh

# Inherit dalvik configuration and the rest of the platform
$(call inherit-product, frameworks/native/build/phone-xhdpi-1024-dalvik-heap.mk)
$(call inherit-product,$(SRC_TARGET_DIR)/product/generic_x86.mk)
$(call inherit-product, build/target/product/full_base_telephony.mk)
