# Recovery copy files when building with "mka recoveryimage"
# Create directory
    $(shell mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware/)
    $(shell mkdir -p $(TARGET_RECOVERY_ROOT_OUT)/etc/)
# Copy firmware and fstab
    $(shell cp $(LOCAL_PATH)/prebuilt/usr/idc/atmxt-i2c.idc $(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware/)
    $(shell cp $(LOCAL_PATH)/prebuilt/usr/idc/atmxt-r2.tdat $(TARGET_RECOVERY_ROOT_OUT)/vendor/firmware/)
    $(shell cp $(LOCAL_PATH)/twrp/twrp.fstab $(TARGET_RECOVERY_ROOT_OUT)/etc/)

# Recovery options TWRP
RECOVERY_VARIANT := twrp
DEVICE_RESOLUTION := 540x960
RECOVERY_GRAPHICS_USE_LINELENGTH := true
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/max170xx_battery
TW_MAX_BRIGHTNESS := 100
TW_BRIGHTNESS_PATH := /sys/class/backlight/psb-bl/brightness
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
HAVE_SELINUX := true
TW_THEME := portrait_hdpi
