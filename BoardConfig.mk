include $(GENERIC_X86_CONFIG_MK)
LOCAL_PATH := device/motorola/smi
TARGET_SPECIFIC_HEADER_PATH := $(LOCAL_PATH)/include

# Make settings
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true
TARGET_NO_INITLOGO   := true

# Board configuration
TARGET_BOOTLOADER_BOARD_NAME := smi
TARGET_CPU_ABI := x86
TARGET_CPU_VARIANT := x86
TARGET_CPU_ABI2 := armeabi-v7a
TARGET_ARCH := x86
TARGET_ARCH_VARIANT := x86-atom
TARGET_ARCH_VARIANT_FPU := sse
TARGET_BOARD_PLATFORM := sc1
TARGET_CPU_SMP := true

# Atom optimizations to improve memory benchmarks.
-include $(LOCAL_PATH)/OptAtom.mk

# Connectivity - Wi-Fi
USES_TI_MAC80211                 := true
WPA_SUPPLICANT_VERSION           := VER_0_8_X_TI
BOARD_WLAN_DEVICE                := wl12xx-compat
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_HOSTAPD_DRIVER             := NL80211
COMMON_GLOBAL_CFLAGS             += -DUSES_TI_MAC80211

TARGET_MODULES_SOURCE := "hardware/ti/wlan-intel/wl12xx-compat"
TARGET_MODULES_SOURCE_DIR := "wl12xx-compat"

WIFI_MODULES:
	rm -rf $(KERNEL_OUT)/COMPAT
	mkdir $(KERNEL_OUT)/COMPAT
	cp -rf $(TARGET_MODULES_SOURCE) $(KERNEL_OUT)/COMPAT
	$(MAKE) -C $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR) O=$(KERNEL_OUT)/COMPAT KERNEL_DIR=$(KERNEL_OUT) KLIB=$(KERNEL_OUT) KLIB_BUILD=$(KERNEL_OUT) ARCH=$(TARGET_ARCH) $(KERNEL_CROSS_COMPILE)

	mv $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR)/compat/compat.ko $(KERNEL_MODULES_OUT)
	mv $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR)/net/mac80211/mac80211.ko $(KERNEL_MODULES_OUT)
	mv $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR)/net/wireless/cfg80211.ko $(KERNEL_MODULES_OUT)
	mv $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR)/drivers/net/wireless/wl12xx/wl12xx.ko $(KERNEL_MODULES_OUT)
	mv $(KERNEL_OUT)/COMPAT/$(TARGET_MODULES_SOURCE_DIR)/drivers/net/wireless/wl12xx/wl12xx_sdio.ko $(KERNEL_MODULES_OUT)

	$(ANDROID_EABI_TOOLCHAIN)/i686-linux-android-strip --strip-unneeded $(KERNEL_MODULES_OUT)/compat.ko
	$(ANDROID_EABI_TOOLCHAIN)/i686-linux-android-strip --strip-unneeded $(KERNEL_MODULES_OUT)/mac80211.ko
	$(ANDROID_EABI_TOOLCHAIN)/i686-linux-android-strip --strip-unneeded $(KERNEL_MODULES_OUT)/cfg80211.ko
	$(ANDROID_EABI_TOOLCHAIN)/i686-linux-android-strip --strip-unneeded $(KERNEL_MODULES_OUT)/wl12xx.ko
	$(ANDROID_EABI_TOOLCHAIN)/i686-linux-android-strip --strip-unneeded $(KERNEL_MODULES_OUT)/wl12xx_sdio.ko

TARGET_KERNEL_MODULES := WIFI_MODULES

# Global flags
COMMON_GLOBAL_CFLAGS += -DMOTOROLA_UIDS

TARGET_USES_MOTOROLA_LOG := true

# Use boot tools to make Intel-formatted images
DEVICE_BASE_BOOT_IMAGE := $(LOCAL_PATH)/boottools/image/boot
DEVICE_BASE_RECOVERY_IMAGE := $(LOCAL_PATH)/boottools/image/recovery
BOARD_CUSTOM_BOOTIMG_MK := $(LOCAL_PATH)/boottools/boot.mk
RAZRI_IMAGE := true

# Kernel build (source:github.com/oxavelar)
BOARD_KERNEL_BASE := 0x1200000
BOARD_KERNEL_BASE := 0x000400
BOARD_KERNEL_PAGESIZE := 4096
TARGET_KERNEL_CONFIG := i386_mfld_hazou_defconfig
BOARD_KERNEL_IMAGE_NAME := bzImage
BOARD_KERNEL_CMDLINE := init=/init pci=noearly console=logk0 vmalloc=260046848 earlyprintk=nologger
BOARD_KERNEL_CMDLINE += hsu_dma=7 kmemleak=off androidboot.bootmedia=sdcard androidboot.hardware=sc1
BOARD_KERNEL_CMDLINE += androidboot.spid=xxxx:xxxx:xxxx:xxxx:xxxx:xxxx emmc_ipanic.ipanic_part_number=6
BOARD_KERNEL_CMDLINE += slub_max_order=2 loglevel=7

# Storage information
BOARD_VOLD_EMMC_SHARES_DEV_MAJOR := true
BOARD_VOLD_DISC_HAS_MULTIPLE_MAJORS := true
BOARD_VOLD_MAX_PARTITIONS := 20
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 11534336 # 0x00b00000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1263255552
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5643771904
BOARD_FLASH_BLOCK_SIZE := 2048
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"

BOARD_DATA_DEVICE                       := /dev/block/mmcblk0p17
BOARD_DATA_FILESYSTEM                   := ext4
BOARD_CACHE_DEVICE                      := /dev/block/mmcblk0p14
BOARD_CACHE_FILESYSTEM                  := ext4
BOARD_SYSTEM_DEVICE                     := /dev/block/mmcblk0p16
BOARD_SYSTEM_FILESYSTEM                 := ext4

# Ramdisk
TARGET_PROVIDES_INIT_RC := true
TARGET_NR_SVC_SUPP_GIDS := 28

# Blutetooth
BOARD_HAVE_BLUETOOTH := true
#BOARD_HAVE_BLUETOOTH_TI := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(LOCAL_PATH)/configs/bluetooth

# Audio
BOARD_USES_ALSA_AUDIO := true
BOARD_USES_TINY_ALSA_AUDIO := true
USE_INTEL_RSP := true
USE_FEATURE_ALAC := true

# Graphics
USE_OPENGL_RENDERER	:= true
BOARD_EGL_CFG := $(LOCAL_PATH)/prebuilt/lib/egl/egl.cfg
BOARD_USES_HWCOMPOSER := true
BOARD_ALLOW_EGL_HIBERNATION := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true

ENABLE_IMG_GRAPHICS := true
BUILD_WITH_FULL_STAGEFRIGHT := true
INTEL_VA := true
USE_INTEL_MDP := true
BOARD_USES_WRS_OMXIL_CORE := true
BOARD_USES_MRST_OMX := true
USE_INTEL_SECURE_AVC := true
COMMON_GLOBAL_CFLAGS += -DASUS_ZENFONE2_LP_BLOBS
USE_LIBJPEG_TURBO := true

# Enable WEBGL in WebKit
ENABLE_WEBGL := true
TARGET_FORCE_CPU_UPLOAD := true

# RILD
RIL_SUPPORTS_SEEK := true

# GPS
BOARD_HAVE_GPS := true

# skip doc from building
BOARD_SKIP_ANDROID_DOC_BUILD := true

# Recovery configuration global
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_FSTAB := $(LOCAL_PATH)/rootdir/etc/fstab.sc1
BOARD_RECOVERY_SWIPE := true
BOARD_UMS_LUNFILE := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"
TARGET_PREBUILT_RECOVERY_KERNEL := $(LOCAL_PATH)/boottools/image/bzImage
BOARD_SUPPRESS_EMMC_WIPE := true

# Recovery options TWRP
#RECOVERY_VARIANT := twrp
#TARGET_RECOVERY_FSTAB := $(LOCAL_PATH)/rootdir/etc/twrp.fstab
COMMON_GLOBAL_CFLAGS += -DNO_SECURE_DISCARD
DEVICE_RESOLUTION := 540x960
RECOVERY_GRAPHICS_USE_LINELENGTH := true
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/max170xx_battery
TW_MAX_BRIGHTNESS := 100
TW_BRIGHTNESS_PATH := /sys/class/backlight/psb-bl/brightness
#TW_NO_SCREEN_BLANK := true
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
HAVE_SELINUX := true

# SELinux
BOARD_SEPOLICY_DIRS += \
    device/motorola/smi/sepolicy

BOARD_SEPOLICY_UNION += \
    file_contexts \
    domain.te \
    init_shell.te \
    netd.te
