include $(GENERIC_X86_CONFIG_MK)
LOCAL_PATH := device/motorola/smi
TARGET_SPECIFIC_HEADER_PATH := $(LOCAL_PATH)/include

# Make settings
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
TARGET_NEEDS_PLATFORM_TEXT_RELOCATIONS := true

# We only have 1GB of RAM
MALLOC_SVELTE := true
BOARD_GLOBAL_CFLAGS += -march=bonnell

WITH_DEXPREOPT := true

# USB
TARGET_USES_LEGACY_ADB_INTERFACE := true

# Connectivity - Wi-Fi
USES_TI_MAC80211 := true
ifdef USES_TI_MAC80211
USES_TI_MAC80211                 := true
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WLAN_DEVICE                := wl12xx_compat
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_HOSTAPD_DRIVER             := NL80211
CONFIG_HS20                      := true
BOARD_GLOBAL_CFLAGS              += -DUSES_TI_MAC80211
endif

# Kernel build (source:github.com/oxavelar)
BOARD_KERNEL_BASE := 0x000400
BOARD_KERNEL_PAGESIZE := 4096
TARGET_KERNEL_CONFIG := i386_mfld_hazou_defconfig
BOARD_KERNEL_IMAGE_NAME := bzImage
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
TARGET_USERIMAGES_USE_F2FS := true
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"

# Partitions
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_CACHEIMAGE_PARTITION_SIZE := 671088640
BOARD_BOOTIMAGE_PARTITION_SIZE := 11534336
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 1283457024
BOARD_USERDATAIMAGE_PARTITION_SIZE := 5599640576
BOARD_FLASH_BLOCK_SIZE := 2048

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
TARGET_TINY_ALSA_IGNORE_SILENCE_SIZE := true

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
TARGET_ADDITIONAL_GRALLOC_10_USAGE_BITS := 0x200000U

# Camera
BOARD_GLOBAL_CFLAGS += -DCAMERA_VENDOR_L_COMPAT
TARGET_PROVIDES_CAMERA_HAL := true
TARGET_HAS_LEGACY_CAMERA_HAL1 := true

# RILD
BOARD_PROVIDES_LIBRIL := true

# GPS
BOARD_HAVE_GPS := true

# Power
POWERHAL_MFLD := true

# Lights
TARGET_PROVIDES_LIBLIGHT := true

# Charger
BOARD_CHARGER_ENABLE_SUSPEND := true

# Houdini: enable ARM codegen for x86
BUILD_ARM_FOR_X86 := true

# SHIM
TARGET_LD_SHIM_LIBS := \
    /system/bin/bd_prov|libshim_crypto.so:\
    /system/bin/mmgr|libshim_mmgr.so:\
    /system/lib/libparameter.so|libshim_audio.so:\
    /system/lib/libsecurity_api.so|libshim_crypto.so:\
    /system/lib/parameter-framework-plugins/Audio/libtinyalsa_custom-subsystem.so|libshim_audio.so:\
    /system/lib/hw/camera.vendor.sc1.so|libshim_camera.so:\
    /system/lib/hw/hwcomposer.smi.so|libshim_atomic.so:\
    /system/lib/libhoudini.so|libshim_houdini.so

# HIDL
DEVICE_MANIFEST_FILE := $(LOCAL_PATH)/manifest.xml

# Recovery configuration global
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
BOARD_HAS_NO_SELECT_BUTTON := true
TARGET_RECOVERY_FSTAB := $(LOCAL_PATH)/rootdir/etc/fstab.sc1
BOARD_RECOVERY_SWIPE := true
BOARD_UMS_LUNFILE := "/sys/devices/virtual/android_usb/android0/f_mass_storage/lun%d/file"
BOARD_SUPPRESS_EMMC_WIPE := true
BOARD_GLOBAL_CFLAGS += -DNO_SECURE_DISCARD

# TWRP Support - Optional
ifeq ($(WITH_TWRP),true)
-include $(LOCAL_PATH)/twrp/twrp.mk
endif
