# This is a custom bootimage target file to be used on CM
# Set these variables on your BoardConfig.mk
#
# DEVICE_BASE_BOOT_IMAGE := path/to/stuff/existingboot.img
# DEVICE_BASE_RECOVERY_IMAGE := path/to/stuff/existingrecovery.img
# BOARD_CUSTOM_BOOTIMG_MK := path/to/intel-boot-tools/boot.mk
# DEVICE_RAMDISK_CONTENT := path/to/stuff/ramdisk-content-folder
#

INTEL_PACK := $(HOST_OUT_EXECUTABLES)/pack_intel
BASE_BOOT_IMAGE := $(DEVICE_BASE_BOOT_IMAGE)
BASE_RECOVERY_IMAGE := $(DEVICE_BASE_RECOVERY_IMAGE)
RAMDISK_CONTENT := $(DEVICE_RAMDISK_CONTENT)

$(INSTALLED_RECOVERYIMAGE_TARGET): $(recovery_ramdisk) \
		$(recovery_kernel) \
		$(INTEL_PACK)
	$(call pretty,"Target recovery image: $@")
	$(INTEL_PACK) $(BASE_RECOVERY_IMAGE) $(recovery_kernel) $(recovery_ramdisk) $@
	@echo -e ${CL_CYN}"Made recovery image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)

ifdef USE_PREBUILT_RAMDISK
# the ramdisk
INTERNAL_RAMDISK_FILES_XT890 := $(filter $(RAMDISK_CONTENT)/%, \
	$(ALL_PREBUILT) \
	$(ALL_COPIED_HEADERS) \
	$(ALL_GENERATED_SOURCES) \
	$(ALL_DEFAULT_INSTALLED_MODULES))

BUILT_RAMDISK_TARGET_XT890 := $(PRODUCT_OUT)/ramdiskxt890.img

# We just build this directly to the install location.
INSTALLED_RAMDISK_TARGET_XT890 := $(BUILT_RAMDISK_TARGET_XT890)
$(INSTALLED_RAMDISK_TARGET_XT890): $(MKBOOTFS) $(INTERNAL_RAMDISK_FILES_XT890) | $(MINIGZIP)
	$(call pretty,"Target ram disk xt890: $@")
	$(hide) $(MKBOOTFS) $(RAMDISK_CONTENT) | $(MINIGZIP) > $@

$(INSTALLED_BOOTIMAGE_TARGET): $(INSTALLED_RAMDISK_TARGET_XT890) \
		$(INSTALLED_KERNEL_TARGET) \
		$(INTEL_PACK)
	$(call pretty,"Target boot image xt890: $@")
	$(INTEL_PACK) $(BASE_BOOT_IMAGE) $(INSTALLED_KERNEL_TARGET) $(INSTALLED_RAMDISK_TARGET_XT890) $@
	@echo -e ${CL_CYN}"Made boot image xt890: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
else # USE_PREBUILT_RAMDISK
$(INSTALLED_BOOTIMAGE_TARGET): $(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_KERNEL_TARGET) \
		$(INTEL_PACK)
	$(call pretty,"Target boot image: $@")
	$(INTEL_PACK) $(BASE_BOOT_IMAGE) $(INSTALLED_KERNEL_TARGET) $(INSTALLED_RAMDISK_TARGET) $@
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
endif # USE_PREBUILT_RAMDISK
