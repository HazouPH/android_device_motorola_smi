# This is a custom bootimage target file to be used on CM
# Set these variables on your BoardConfig.mk
#
# DEVICE_BASE_BOOT_IMAGE := path/to/stuff/existingboot.img
# DEVICE_BASE_RECOVERY_IMAGE := path/to/stuff/existingrecovery.img
# BOARD_CUSTOM_BOOTIMG_MK := path/to/intel-boot-tools/boot.mk
#

INTEL_PACK := $(HOST_OUT_EXECUTABLES)/pack_intel
BASE_BOOT_IMAGE := $(DEVICE_BASE_BOOT_IMAGE)
BASE_RECOVERY_IMAGE := $(DEVICE_BASE_RECOVERY_IMAGE)

$(INSTALLED_RECOVERYIMAGE_TARGET): $(recovery_ramdisk) \
		$(recovery_kernel) \
		$(INTEL_PACK)
	$(call pretty,"Target recovery image: $@")
	$(INTEL_PACK) $(BASE_RECOVERY_IMAGE) $(recovery_kernel) $(recovery_ramdisk) $@
	@echo -e ${CL_CYN}"Made recovery image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)

$(INSTALLED_BOOTIMAGE_TARGET): $(INSTALLED_RAMDISK_TARGET) \
		$(INSTALLED_KERNEL_TARGET) \
		$(INTEL_PACK)
	$(call pretty,"Target boot image: $@")
	$(INTEL_PACK) $(BASE_BOOT_IMAGE) $(INSTALLED_KERNEL_TARGET) $(INSTALLED_RAMDISK_TARGET) $@
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
