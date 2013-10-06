include $(GENERIC_X86_CONFIG_MK)

# Board configuration
TARGET_BOOTLOADER_BOARD_NAME := smi
TARGET_CPU_ABI := x86
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := x86
TARGET_ARCH_VARIANT := x86-atom
TARGET_ARCH_VARIANT_FPU := sse
TARGET_BOARD_PLATFORM := sc1
TARGET_CPU_SMP := true

# Atom optimizations specified (source:oxavelar)
TARGET_GLOBAL_CFLAGS += \
                        -O2 \
                        -flto \
                        -march=atom \
                        -mmmx \
                        -msse \
                        -msse2 \
                        -msse3 \
                        -mssse3 \
                        -mpclmul \
                        -mcx16 \
                        -msahf \
                        -mmovbe \
                        -ftree-vectorize \
                        -fomit-frame-pointer \
                        -finline-functions \
                        -fpredictive-commoning \
                        -fgcse-after-reload \
                        -fforce-addr \
                        -ffast-math \
                        -fsingle-precision-constant \
                        -floop-block \
                        -floop-interchange \
                        -floop-strip-mine \
                        -floop-parallelize-all \
                        -ftree-parallelize-loops=2 \
                        -ftree-loop-if-convert \
                        -ftree-loop-if-convert-stores \
                        -ftree-loop-distribution \
                        -foptimize-register-move \
                        -fgraphite-identity \

# The following are very specific to our z2480 Atom
TARGET_GLOBAL_CFLAGS += \
                        --param l1-cache-line-size=64 \
                        --param l1-cache-size=24 \
                        --param l2-cache-size=512 \

TARGET_GLOBAL_CFLAGS += -DUSE_SSSE3 -DUSE_SSE2

TARGET_GLOBAL_CPPFLAGS += -march=atom -fno-exceptions

TARGET_GLOBAL_LDFLAGS += -Wl,-O1

# Connectivity - Wi-Fi
USES_TI_MAC80211 := true
ifdef USES_TI_MAC80211
WPA_SUPPLICANT_VERSION := 		VER_0_8_X_TI
BOARD_WPA_SUPPLICANT_DRIVER := 		NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := 	lib_driver_cmd_wl12xx
BOARD_HOSTAPD_DRIVER := 		NL80211
BOARD_HOSTAPD_PRIVATE_LIB := 		lib_driver_cmd_wl12xx
PRODUCT_WIRELESS_TOOLS := 		true
BOARD_WLAN_DEVICE := 			wl12xx_mac80211
BOARD_SOFTAP_DEVICE := 			wl12xx_mac80211
WIFI_DRIVER_MODULE_PATH := 		"/system/lib/modules/wl12xx_sdio.ko"
WIFI_DRIVER_MODULE_NAME := 		"wl12xx_sdio"
WIFI_FIRMWARE_LOADER := 		""
COMMON_GLOBAL_CFLAGS += 		-DUSES_TI_MAC80211
endif

# Global flags
COMMON_GLOBAL_VDFLAGS += -DMOTOROLA_UIDS
COMMON_GLOBAL_CFLAGS += -DBOARD_USE_MOTOROLA_DEV_ALIAS

TARGET_USES_MOTOROLA_LOG := true

# Make settings
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true

# Use boot tools to make Intel-formatted images
DEVICE_BASE_BOOT_IMAGE := device/motorola/smi/blobs/boot.image
DEVICE_BASE_RECOVERY_IMAGE := device/motorola/smi/blobs/recovery.image
BOARD_CUSTOM_BOOTIMG_MK := device/motorola/smi/boottools/boot.mk

# Kernel build (source:github.com/oxavelar)
BOARD_KERNEL_BASE := 0x1200000
BOARD_KERNEL_BASE := 0x000400
BOARD_KERNEL_PAGESIZE := 4096
TARGET_PREBUILT_KERNEL := device/motorola/smi/blobs/kernel
#TARGET_KERNEL_CONFIG := i386_mfld_hazou_defconfig
#TARGET_KERNEL_SOURCE := kernel/XT809-Gamma/kernel/linux-3.0
KERNEL_MODULES_IN_ROOT := true
BOARD_KERNEL_CMDLINE := init=/init pci=noearly console=logk0 vmalloc=272M earlyprintk=nologger hsu_dma=7 kmemleak=off androidboot.bootmedia=sdcard androidboot.hardware=sc1 emmc_ipanic.ipanic_part_number=6 loglevel=4

WLAN_MODULES:
	make clean -C hardware/ti/wlan/mac80211/compat_wl12xx
	make -j8 -C hardware/ti/wlan/mac80211/compat_wl12xx KERNEL_DIR=$(KERNEL_OUT) KLIB=$(KERNEL_OUT) KLIB_BUILD=$(KERNEL_OUT) ARCH=i386
	mv hardware/ti/wlan/mac80211/compat_wl12xx/compat/compat.ko $(KERNEL_MODULES_OUT)
	mv hardware/ti/wlan/mac80211/compat_wl12xx/net/mac80211/mac80211.ko $(KERNEL_MODULES_OUT)
	mv hardware/ti/wlan/mac80211/compat_wl12xx/net/wireless/cfg80211.ko $(KERNEL_MODULES_OUT)
	mv hardware/ti/wlan/mac80211/compat_wl12xx/drivers/net/wireless/wl12xx/wl12xx.ko $(KERNEL_MODULES_OUT)
	mv hardware/ti/wlan/mac80211/compat_wl12xx/drivers/net/wireless/wl12xx/wl12xx_sdio.ko $(KERNEL_MODULES_OUT)

#TARGET_KERNEL_MODULES += WLAN_MODULES

# Partition information
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 11534336 # 0x00b00000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 536870912 # 0x20000000
BOARD_FLASH_BLOCK_SIZE := 2048

# Enabling Houdini by default
ADDITIONAL_BUILD_PROPERTIES += ro.product.cpu.abi2=armeabi-v7a

# Ramdisk
TARGET_PROVIDES_INIT_RC := true

# Customize the malloced address to be 16-byte aligned
BOARD_MALLOC_ALIGNMENT := 16

# Blutetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/motorola/smi/Bluetooth

# NFC
BOARD_HAVE_NFC := true

# Audio
BOARD_USES_ALSA_AUDIO := true
BUILD_WITH_ALSA_UTILS := true
BOARD_USES_SEPERATED_AUDIO_INPUT := true

# Security
BUILD_WITH_CHAABI_SUPPORT := true
BUILD_WITH_WATCHDOG_DAEMON_SUPPORT := true

# Graphics
USE_OPENGL_RENDERER := true
BOARD_EGL_CFG := device/motorola/smi/prebuilt/egl.cfg

# Enable WEBGL in WebKit
ENABLE_WEBGL := true
TARGET_FORCE_CPU_UPLOAD := true

# Turn on GR_STATIC_RECT_VB flag in skia to boost performance
TARGET_USE_GR_STATIC_RECT_VB := true

# Recovery configuration global
TARGET_RECOVERY_PIXEL_FORMAT := "BGRA_8888"
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/motorola/smi/recovery_keys.c
BOARD_HAS_NO_SELECT_BUTTON := true
#BOARD_TOUCH_RECOVERY := true
# Recovery options TWRP
DEVICE_RESOLUTION := 540x960
RECOVERY_GRAPHICS_USE_LINELENGTH := true
TW_CUSTOM_BATTERY_PATH := /sys/class/power_supply/max170xx_battery
RECOVERY_SDCARD_ON_DATA := true
BOARD_HAS_NO_REAL_SDCARD := true
TW_INTERNAL_STORAGE_PATH := "/data/media"
TW_INTERNAL_STORAGE_MOUNT_POINT := "data"
TW_EXTERNAL_STORAGE_PATH := "/external_sd"
TW_EXTERNAL_STORAGE_MOUNT_POINT := "external_sd"
TW_DEFAULT_EXTERNAL_STORAGE := true
