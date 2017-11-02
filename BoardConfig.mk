include $(GENERIC_X86_CONFIG_MK)
LOCAL_PATH := device/motorola/smi
TARGET_SPECIFIC_HEADER_PATH := $(LOCAL_PATH)/include

# Make settings
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# Board configuration
TARGET_BOOTLOADER_BOARD_NAME := smi
TARGET_CPU_ABI := x86
TARGET_CPU_ABI2 := armeabi-v7a
TARGET_CPU_ABI_LIST := x86,armeabi-v7a,armeabi
TARGET_CPU_ABI_LIST_32_BIT := x86,armeabi-v7a,armeabi
TARGET_ARCH := x86
TARGET_ARCH_VARIANT := atom
TARGET_BOARD_PLATFORM := sc1

# Atom optimizations to improve memory benchmarks.
-include $(LOCAL_PATH)/OptAtom.mk

# Connectivity - Wi-Fi
USES_TI_MAC80211                 := true
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WLAN_DEVICE                := wl12xx-compat
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_HOSTAPD_DRIVER             := NL80211
CONFIG_HS20                      := true
BOARD_GLOBAL_CFLAGS              += -DUSES_TI_MAC80211

TARGET_MODULES_SOURCE := "hardware/ti/wlan-intel/wl12xx-compat"

WIFI_MODULES:
	make clean -C $(TARGET_MODULES_SOURCE)
	make -j8 -C $(TARGET_MODULES_SOURCE) KERNEL_DIR=$(KERNEL_OUT) KLIB=$(KERNEL_OUT) KLIB_BUILD=$(KERNEL_OUT) ARCH=$(TARGET_ARCH) $(KERNEL_CROSS_COMPILE) -o $(KERNEL_OUT)/wl12xx-compat
	mv $(TARGET_MODULES_SOURCE)/compat/compat.ko $(KERNEL_MODULES_OUT)
	mv $(TARGET_MODULES_SOURCE)/net/mac80211/mac80211.ko $(KERNEL_MODULES_OUT)
	mv $(TARGET_MODULES_SOURCE)/net/wireless/cfg80211.ko $(KERNEL_MODULES_OUT)
	mv $(TARGET_MODULES_SOURCE)/drivers/net/wireless/wl12xx/wl12xx.ko $(KERNEL_MODULES_OUT)
	mv $(TARGET_MODULES_SOURCE)/drivers/net/wireless/wl12xx/wl12xx_sdio.ko $(KERNEL_MODULES_OUT)
	$(KERNEL_TOOLCHAIN_PREFIX)strip --strip-unneeded $(KERNEL_MODULES_OUT)/compat.ko
	$(KERNEL_TOOLCHAIN_PREFIX)strip --strip-unneeded $(KERNEL_MODULES_OUT)/mac80211.ko
	$(KERNEL_TOOLCHAIN_PREFIX)strip --strip-unneeded $(KERNEL_MODULES_OUT)/cfg80211.ko
	$(KERNEL_TOOLCHAIN_PREFIX)strip --strip-unneeded $(KERNEL_MODULES_OUT)/wl12xx.ko
	$(KERNEL_TOOLCHAIN_PREFIX)strip --strip-unneeded $(KERNEL_MODULES_OUT)/wl12xx_sdio.ko
	make clean -C $(TARGET_MODULES_SOURCE)

TARGET_KERNEL_MODULES := WIFI_MODULES

# Use boot tools to make Intel-formatted images
DEVICE_BASE_BOOT_IMAGE := $(LOCAL_PATH)/boottools/image/boot
DEVICE_BASE_RECOVERY_IMAGE := $(LOCAL_PATH)/boottools/image/recovery
BOARD_CUSTOM_BOOTIMG_MK := $(LOCAL_PATH)/boottools/boot.mk
RAZRI_IMAGE := true

# Kernel build (source:github.com/oxavelar)
BOARD_KERNEL_BASE := 0x000400
BOARD_KERNEL_PAGESIZE := 4096
TARGET_KERNEL_CONFIG := i386_mfld_hazou_defconfig
BOARD_KERNEL_IMAGE_NAME := bzImage
KERNEL_TOOLCHAIN_PREFIX := $(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/x86/i686-linux-android-4.7/bin/i686-linux-android-
BOARD_KERNEL_CMDLINE := init=/init pci=noearly console=logk0 vmalloc=260046848 earlyprintk=nologger
BOARD_KERNEL_CMDLINE += hsu_dma=7 kmemleak=off androidboot.bootmedia=sdcard androidboot.hardware=sc1
BOARD_KERNEL_CMDLINE += androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx emmc_ipanic.ipanic_part_number=6
BOARD_KERNEL_CMDLINE += slub_max_order=2 loglevel=7
BOARD_KERNEL_CMDLINE += androidboot.selinux=permissive

# Storage information
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
BOARD_VOLD_MAX_PARTITIONS := 20
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_BOOTIMAGE_PARTITION_SIZE := 11534336 # 0x00b00000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1263255552
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5643771904
BOARD_FLASH_BLOCK_SIZE := 2048
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"

# Ramdisk
TARGET_NR_SVC_SUPP_GIDS := 28
TARGET_INIT_UMOUNT_AND_FSCK_IS_UNSAFE := true

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_TI := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/configs/bluetooth

# Audio
BOARD_USES_ALSA_AUDIO := true
BOARD_USES_TINY_ALSA_AUDIO := true

# Graphics
BOARD_USES_HWCOMPOSER := true
BOARD_ALLOW_EGL_HIBERNATION := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true

ENABLE_IMG_GRAPHICS := true
BUILD_WITH_FULL_STAGEFRIGHT := true
INTEL_VA := true
BOARD_USES_WRS_OMXIL_CORE := true
BOARD_USES_MRST_OMX := true
USE_INTEL_SECURE_AVC := true
BOARD_GLOBAL_CFLAGS += -DASUS_ZENFONE2_LP_BLOBS

# Camera
BOARD_GLOBAL_CFLAGS += -DCAMERA_VENDOR_L_COMPAT
TARGET_PROVIDES_CAMERA_HAL := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true

# RILD
BOARD_PROVIDES_LIBRIL := true
#BOARD_RIL_CLASS := ../../../device/motorola/smi/ril

# GPS
BOARD_HAVE_GPS := true

# Power
POWERHAL_MFLD := true

# Lights
TARGET_PROVIDES_LIBLIGHT := true

# Charger
BOARD_CHARGER_ENABLE_SUSPEND := true
BOARD_HEALTHD_CUSTOM_CHARGER_RES := $(LOCAL_PATH)/charger/images
BOARD_CHARGER_SHOW_PERCENTAGE := true

# Houdini: enable ARM codegen for x86
BUILD_ARM_FOR_X86 := true

# skip some proccess to speed up build
BOARD_SKIP_ANDROID_DOC_BUILD := true
BUILD_EMULATOR := false

# Recovery configuration global
BOARD_CANT_BUILD_RECOVERY_FROM_BOOT_PATCH := true
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_FSTAB := $(LOCAL_PATH)/rootdir/etc/fstab.sc1
BOARD_RECOVERY_SWIPE := true
BOARD_UMS_LUNFILE := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"
TARGET_PREBUILT_RECOVERY_KERNEL := $(LOCAL_PATH)/boottools/image/bzImage
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_GLOBAL_CFLAGS += -DNO_SECURE_DISCARD

# Recovery options TWRP
#RECOVERY_VARIANT := twrp
DEVICE_RESOLUTION := 540x960
RECOVERY_GRAPHICS_USE_LINELENGTH := true
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/max170xx_battery
TW_MAX_BRIGHTNESS := 100
TW_BRIGHTNESS_PATH := /sys/class/backlight/psb-bl/brightness
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
#TW_USE_TOOLBOX := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
TWHAVE_SELINUX := true

# SELinux
#BOARD_SEPOLICY_DIRS += \
    device/motorola/smi/sepolicy
