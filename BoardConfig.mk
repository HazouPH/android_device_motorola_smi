TARGET_SPECIFIC_HEADER_PATH := hardware/intel/include

include $(GENERIC_X86_CONFIG_MK)

# Board configuration
TARGET_NO_BOOTLOADER := true
TARGET_BOOTLOADER_BOARD_NAME := smi
TARGET_CPU_ABI := x86
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := x86
TARGET_ARCH_VARIANT := x86-atom
TARGET_BOARD_PLATFORM := mrst
TARGET_CPU_SMP := true

# Blutetooth
BOARD_HAVE_BLUETOOTH=true

# NFC
BOARD_HAVE_NFC := true

# Audio
BOARD_USES_ALSA_AUDIO := true
BUILD_WITH_ALSA_UTILS := true
INTEL_AMC := true

# Graphics
BOARD_EGL_CFG := device/motorola/smi/prebuilt/egl.cfg
INTEL_HWC := true

# Connectivity - Wi-Fi
USES_TI_MAC80211 := true
ifdef USES_TI_MAC80211
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_HOSTAPD_DRIVER             := NL80211
PRODUCT_WIRELESS_TOOLS           := true
BOARD_WLAN_DEVICE                := wl128x_mac80211
BOARD_SOFTAP_DEVICE              := wl128x_mac80211
WIFI_DRIVER_MODULE_NAME          := "wl128x_sdio"
WIFI_FIRMWARE_LOADER             := ""
COMMON_GLOBAL_CFLAGS             += -DUSES_TI_MAC80211
endif

# Use boot tools to make Intel-formatted images
DEVICE_BASE_BOOT_IMAGE := device/motorola/smi/blobs/boot.image
DEVICE_BASE_RECOVERY_IMAGE := device/motorola/smi/blobs/recovery.image
BOARD_CUSTOM_BOOTIMG_MK := device/motorola/smi/boottools/boot.mk

# Recovery configuration
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/motorola/smi/recovery_keys.c
BOARD_HAS_NO_SELECT_BUTTON := true
#BOARD_TOUCH_RECOVERY := true

# This is deprecated and will be dropped
TARGET_PREBUILT_KERNEL := device/motorola/smi/blobs/kernel
#TARGET_KERNEL_SOURCE := hardware/intel/linux-2.6
#TARGET_KERNEL_CONFIG := i386_mfld_moto_defconfig
#BOARD_USES_bzImage := true

BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1073741824
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1073741824
BOARD_FLASH_BLOCK_SIZE := 4096

# Recovery options TWRP
DEVICE_RESOLUTION := 540x960
RECOVERY_GRAPHICS_USE_LINELENGTH := true
SP1_NAME := "pds"
SP1_BACKUP_METHOD := files
SP1_MOUNTABLE := 1
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
